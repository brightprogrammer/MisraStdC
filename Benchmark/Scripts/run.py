#!/usr/bin/env python3
# Run every bench-* binary under the given builddir, collect median
# timings, and rewrite Benchmark/README.md from README.template.md
# with the numbers substituted in.
#
# Usage:
#   python3 Benchmark/Scripts/run.py <builddir>
# e.g.
#   meson setup build -Dbenchmark=true -Dbuildtype=release -Doptimization=3
#   ninja  -C build
#   python3 Benchmark/Scripts/run.py build
#
# Output:
#   Benchmark/README.md  (overwritten)
#
# Dependencies: python3 (>= 3.7), no third-party libs.

import argparse
import json
import os
import platform
import re
import shutil
import subprocess
import sys
from datetime import datetime, timezone
from pathlib import Path

# Allocators in column order. Misra columns last so improvements stand
# out next to the production baselines.
BACKENDS = [
    "glibc",
    "jemalloc",
    "mimalloc",
    "tcmalloc",
    "misra",            # HeapAllocator for every bench
    "misra-correct",    # Heap or Slab per bench (the "right tool" comparison)
    "misra-arena",      # ArenaAllocator (bump + bulk-reset)
    "misra-page",       # PageAllocator (mmap-per-alloc)
]

MIN_TIME_OVERRIDES: dict[str, str] = {}

# Per-backend benchmark filter (gbench --benchmark_filter regex).
# Use a POSITIVE include list -- gbench's filter is `re_search`, not
# subtraction, so we enumerate the benches each backend can run honestly.
#
# misra-arena: ArenaAllocator's `arena_allocator_deallocate` rewinds the
#   bump cursor only when the freed ptr is `last_ptr` (the single most
#   recent allocation). Pair-style benches (alloc, free-same-ptr) work
#   -- every free rewinds, next alloc bumps back to the same address.
#   But BM_BatchAllocFree alloc-N-then-free-N can only rewind ONE of
#   the N frees (the last allocated); the other (N-1) frees are no-ops,
#   accumulating in the arena until Reset/Deinit. With GBench's auto-
#   scaled iteration counts the arena leaks gigabytes and thrashes.
#   This isn't a perf bug, it's arena's contract -- arena's
#   batch-equivalent workload is BM_ArenaBumpReset (one Reset releases
#   everything). Skip BatchAllocFree for arena, include the rest.
BENCHMARK_FILTERS = {
    # Only the benches arena can run honestly with its single-deep LIFO
    # rewind:
    #   AllocFreePair / AllocTouchFree -- alloc-then-free-immediately,
    #     last_ptr matches every time, zero net growth.
    #   ArenaBumpReset -- the workload arena is designed for.
    #   ReallocGrow -- a single buffer growing in place; arena's
    #     last-ptr remap fast-path handles each step.
    #
    # Excluded:
    #   BatchAllocFree / MixedPareto / Frag_* -- all hold N allocations
    #     live and then free them, which arena's single-deep LIFO can't
    #     rewind for the (N-1) older items. Memory would grow without
    #     bound. These are workloads arena isn't built for; the table
    #     shows n/a for arena on those rows.
    "misra-arena": "BM_(AllocFreePair|AllocTouchFree|ArenaBumpReset|ReallocGrow)",
}

# Per-test column groups. Each entry: (template-placeholder, list of
# (benchmark-name, display-label, time-unit-to-format)).
#
# `benchmark-name` matches Google Benchmark's reported `name` field
# (with the Arg suffix like "/16"). `time-unit-to-format` decides
# whether we print as ns or us in the result table.
TABLES = {
    "TABLE_ALLOC_FREE_PAIR": [
        ("BM_AllocFreePair/16",    "16 B",    "ns"),
        ("BM_AllocFreePair/64",    "64 B",    "ns"),
        ("BM_AllocFreePair/256",   "256 B",   "ns"),
        ("BM_AllocFreePair/1024",  "1 KiB",   "ns"),
        ("BM_AllocFreePair/4096",  "4 KiB",   "ns"),
        ("BM_AllocFreePair/16384", "16 KiB",  "ns"),
        ("BM_AllocFreePair/65536", "64 KiB",  "ns"),
    ],
    "TABLE_BATCH_ALLOC_FREE": [
        ("BM_BatchAllocFree/128",  "128 × 64 B",   "us"),
        ("BM_BatchAllocFree/1024", "1024 × 64 B",  "us"),
        ("BM_BatchAllocFree/8192", "8192 × 64 B",  "us"),
    ],
    "TABLE_ALLOC_TOUCH_FREE": [
        ("BM_AllocTouchFree/64",    "64 B",   "ns"),
        ("BM_AllocTouchFree/4096",  "4 KiB",  "ns"),
        ("BM_AllocTouchFree/65536", "64 KiB", "ns"),
    ],
    "TABLE_MIXED_PARETO": [
        ("BM_MixedPareto", "Pareto(1.16, 24)", "us"),
    ],
    "TABLE_REALLOC_GROW": [
        ("BM_ReallocGrow", "8 B → 1 MiB", "ns"),
    ],
    "TABLE_ARENA_BUMP_RESET": [
        ("BM_ArenaBumpReset/128",  "128 × 32 B",  "us"),
        ("BM_ArenaBumpReset/1024", "1024 × 32 B", "us"),
        ("BM_ArenaBumpReset/8192", "8192 × 32 B", "us"),
    ],
}

# Fragmentation table is structurally different (custom counters in
# the JSON, not pure timings). Handled separately in render_frag_table.
FRAG_ROWS = [
    ("BM_Frag_Checkerboard/4096",  "Checkerboard (4 K small)"),
    ("BM_Frag_Checkerboard/16384", "Checkerboard (16 K small)"),
    ("BM_Frag_Checkerboard/65536", "Checkerboard (64 K small)"),
    ("BM_Frag_LifetimeMix",        "Lifetime mix"),
    ("BM_Frag_PageOverhang",       "Page overhang"),
]


def run_binary(binary: Path, repetitions: int, min_time: str = "0.5s",
               filter_re: str | None = None) -> dict:
    """Run one bench-X binary; return its parsed JSON."""
    argv = [
        str(binary),
        "--benchmark_format=json",
        f"--benchmark_repetitions={repetitions}",
        "--benchmark_report_aggregates_only=true",
        f"--benchmark_min_time={min_time}",
    ]
    if filter_re:
        argv.append(f"--benchmark_filter={filter_re}")
    out = subprocess.run(argv, check=True, capture_output=True, text=True)
    return json.loads(out.stdout)


def median_time_ns(j: dict, bench_name: str) -> float | None:
    """Find the median aggregate row for `bench_name`; return real_time in ns."""
    for entry in j.get("benchmarks", []):
        if entry.get("name") == f"{bench_name}_median":
            unit = entry.get("time_unit", "ns")
            t = float(entry["real_time"])
            return t * {"ns": 1.0, "us": 1e3, "ms": 1e6, "s": 1e9}[unit]
    return None


def get_counter(j: dict, bench_name: str, key: str) -> float | None:
    """Pull a custom counter (e.g. footprint_MB) off the median row."""
    for entry in j.get("benchmarks", []):
        if entry.get("name") == f"{bench_name}_median":
            v = entry.get(key)
            return float(v) if v is not None else None
    return None


def fmt_time(t_ns: float | None, unit: str) -> str:
    if t_ns is None:
        return "n/a"
    if unit == "ns":
        return f"{t_ns:.1f}"
    if unit == "us":
        return f"{t_ns / 1e3:.1f}"
    if unit == "ms":
        return f"{t_ns / 1e6:.2f}"
    return f"{t_ns:.0f}"


def render_timing_table(rows: list, unit_hint: str, data: dict) -> str:
    """Render one markdown table with allocators as columns, benchmark rows as rows."""
    hdr = ["benchmark"] + BACKENDS
    aligns = ["---"] + [f"---:" for _ in BACKENDS]
    out = ["| " + " | ".join(hdr) + " |"]
    out.append("|" + "|".join(aligns) + "|")
    for bench_name, label, unit in rows:
        cells = [label]
        for be in BACKENDS:
            j = data.get(be)
            if j is None:
                cells.append("n/a")
                continue
            t = median_time_ns(j, bench_name)
            cells.append(fmt_time(t, unit))
        out.append("| " + " | ".join(cells) + " |")
    # Trailing line with the unit so readers don't have to guess.
    out.append("")
    sample = rows[0][2] if rows else "ns"
    out.append(f"_Values in {sample}._")
    return "\n".join(out)


def render_frag_table(data: dict) -> str:
    """Fragmentation table: live, then committed per backend."""
    cols = ["benchmark", "live MB"] + [f"{be} MB" for be in BACKENDS]
    aligns = ["---", "---:"] + ["---:" for _ in BACKENDS]
    out = ["| " + " | ".join(cols) + " |"]
    out.append("|" + "|".join(aligns) + "|")
    for bench_name, label in FRAG_ROWS:
        # Pull live_MB from whichever backend reported it (all should agree).
        live = None
        for be in BACKENDS:
            j = data.get(be)
            if j is None:
                continue
            live = get_counter(j, bench_name, "live_MB")
            if live is not None:
                break
        cells = [label, f"{live:.1f}" if live is not None else "n/a"]
        for be in BACKENDS:
            j = data.get(be)
            if j is None:
                cells.append("n/a")
                continue
            fp = get_counter(j, bench_name, "footprint_MB")
            # Backends without a working stats API (mimalloc 3.3.0's
            # mi_stats_get is broken; see the long comment in
            # Allocator_libc.c::bench_footprint_bytes) report 0.0.
            # That would flatter them in a fragmentation table -- they
            # didn't allocate zero bytes, we just can't read what they
            # did. Render those as n/a, not 0.0.
            if fp is None or fp == 0.0:
                cells.append("n/a")
            else:
                cells.append(f"{fp:.1f}")
        out.append("| " + " | ".join(cells) + " |")
    return "\n".join(out)


def build_tldr(data: dict) -> str:
    """Pull a couple of headline numbers for the TL;DR."""
    def pair_at(be: str, name: str) -> str:
        t = median_time_ns(data.get(be, {}), name)
        return f"{t:.1f} ns" if t is not None else "n/a"

    lines = [
        "Single alloc/free pair, 16 B:",
        "",
        "| backend | time |",
        "|---|---:|",
        f"| tcmalloc           | {pair_at('tcmalloc', 'BM_AllocFreePair/16')} |",
        f"| glibc              | {pair_at('glibc',    'BM_AllocFreePair/16')} |",
        f"| jemalloc           | {pair_at('jemalloc', 'BM_AllocFreePair/16')} |",
        f"| mimalloc           | {pair_at('mimalloc', 'BM_AllocFreePair/16')} |",
        f"| misra (Heap only)  | {pair_at('misra',         'BM_AllocFreePair/16')} |",
        f"| misra-correct (Slab) | {pair_at('misra-correct', 'BM_AllocFreePair/16')} |",
    ]
    return "\n".join(lines)


def gather_env(builddir: Path) -> dict:
    """Collect environment metadata for the README footer."""
    repo = builddir.parent  # builddir is `<repo>/build`, README lives in `<repo>/Benchmark`.
    try:
        commit = subprocess.check_output(
            ["git", "-C", str(repo), "rev-parse", "--short=12", "HEAD"], text=True
        ).strip()
    except Exception:
        commit = "unknown"
    try:
        branch = subprocess.check_output(
            ["git", "-C", str(repo), "rev-parse", "--abbrev-ref", "HEAD"], text=True
        ).strip()
        commit = f"{commit} ({branch})"
    except Exception:
        pass
    cpu = "unknown"
    try:
        with open("/proc/cpuinfo") as f:
            for line in f:
                if line.startswith("model name"):
                    cpu = line.split(":", 1)[1].strip()
                    break
    except Exception:
        cpu = platform.processor() or platform.machine() or "unknown"
    kernel = f"{platform.system()} {platform.release()}"
    compiler = "unknown"
    # Pull the compiler version from meson's own intro file.
    intro = builddir / "meson-info" / "intro-compilers.json"
    if intro.exists():
        try:
            ic = json.loads(intro.read_text())
            c = ic.get("host", {}).get("c", {})
            if c:
                compiler = f"{c.get('id', '')} {c.get('version', '')}".strip()
        except Exception:
            pass
    build_options = "unknown"
    opts_intro = builddir / "meson-info" / "intro-buildoptions.json"
    if opts_intro.exists():
        try:
            opts = json.loads(opts_intro.read_text())
            picks = []
            # Capture both meson-builtin knobs (buildtype/optimization/etc.)
            # and the project's safety/perf-relevant flags. Readers need
            # heap_validate_full in particular to interpret the misra
            # numbers honestly -- "release validate-full" and "release
            # validate-fast" are very different rows.
            wanted = (
                "buildtype",
                "optimization",
                "b_lto",
                "b_sanitize",
                "heap_validate_full",
                "alloc_debug",
            )
            for o in opts:
                if o.get("name") in wanted:
                    picks.append(f"{o['name']}={o['value']}")
            build_options = " ".join(picks) or "default"
        except Exception:
            pass
    return {
        "TIMESTAMP": datetime.now(timezone.utc).strftime("%Y-%m-%d %H:%M:%S UTC"),
        "COMMIT": commit,
        "CPU": cpu,
        "KERNEL": kernel,
        "COMPILER": compiler,
        "BUILD_OPTIONS": build_options,
    }


def main() -> int:
    ap = argparse.ArgumentParser(
        description="Run bench-* binaries and refresh Benchmark/README.md"
    )
    ap.add_argument("builddir", type=Path, help="Meson build directory (must contain Benchmark/bench-*)")
    ap.add_argument("--reps", type=int, default=10, help="Repetitions per benchmark (default 10)")
    ap.add_argument(
        "--out",
        type=Path,
        default=None,
        help="Output README path (default Benchmark/README.md relative to repo root)",
    )
    args = ap.parse_args()

    builddir: Path = args.builddir.resolve()
    bench_dir = builddir / "Benchmark"
    if not bench_dir.is_dir():
        print(f"error: no Benchmark/ subdir under {builddir}. Did you forget -Dbenchmark=true?",
              file=sys.stderr)
        return 1

    # Each bench binary lives at builddir/Benchmark/bench-<name>.
    data = {}
    for be in BACKENDS:
        binary = bench_dir / f"bench-{be}"
        if not binary.exists():
            print(f"warn: {binary} missing -- skipping {be}", file=sys.stderr)
            continue
        min_time  = MIN_TIME_OVERRIDES.get(be, "0.5s")
        filter_re = BENCHMARK_FILTERS.get(be)
        print(f"[run] bench-{be}  (min_time={min_time}"
              + (f", filter={filter_re}" if filter_re else "") + ")",
              file=sys.stderr)
        data[be] = run_binary(binary, args.reps, min_time=min_time, filter_re=filter_re)

    # Build substitution map.
    subs = {f"{{{{{k}}}}}": v for k, v in gather_env(builddir).items()}
    subs["{{REPS}}"] = str(args.reps)
    subs["{{TLDR}}"] = build_tldr(data)
    for placeholder, rows in TABLES.items():
        unit_hint = rows[0][2]
        subs[f"{{{{{placeholder}}}}}"] = render_timing_table(rows, unit_hint, data)
    subs["{{TABLE_FRAG}}"] = render_frag_table(data)

    # Load template + substitute.
    repo = builddir.parent
    tpl_path = repo / "Benchmark" / "README.template.md"
    out_path = args.out or (repo / "Benchmark" / "README.md")
    tpl = tpl_path.read_text()
    for key, val in subs.items():
        tpl = tpl.replace(key, val)
    out_path.write_text(tpl)
    print(f"[write] {out_path}", file=sys.stderr)
    return 0


if __name__ == "__main__":
    sys.exit(main())
