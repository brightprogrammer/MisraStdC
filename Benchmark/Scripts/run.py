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

# Allocators in column order. One column per allocator -- libc-shape
# baselines first, then every MisraStdC allocator type the library
# exposes. The decision about which allocator is "right" for a given
# workload is left to the reader: rows the allocator's contract can't
# serve honestly come out as n/a (the per-backend filters below + each
# backend's bench_alloc-returns-NULL fallback).
LIBC_BACKENDS = ["glibc", "jemalloc", "mimalloc", "tcmalloc"]
MISRA_BACKENDS = [
    "misra-heap",
    "misra-slab",
    "misra-arena",
    "misra-page",
    "misra-budget",
]
BACKENDS = LIBC_BACKENDS + MISRA_BACKENDS


def binary_name_for(column: str) -> str:
    """The bench-* binary that backs a given column."""
    return column

MIN_TIME_OVERRIDES: dict[str, str] = {}

# Per-backend benchmark filter (gbench --benchmark_filter regex).
# POSITIVE include list -- gbench's filter is `re_search`, not subtraction,
# so each entry enumerates the benches that backend can run honestly.
# Missing rows show up as n/a in the README table.
BENCHMARK_FILTERS = {
    # SlabAllocator: fixed slot in [16, 4096], power-of-two only.
    #   AllocFreePair / AllocTouchFree -- sizes <= 4096 only.
    #   BatchAllocFree -- always slot=64, fits.
    # Excluded:
    #   AllocFreePair/16384, /65536, AllocTouchFree/65536 -- slot exceeds
    #     PAGE_SIZE; SlabAllocator can't be constructed for them.
    #   MixedPareto / ReallocGrow / Frag_* -- mixed sizes; a slab has
    #     one slot for life. Reader who wants those workloads with a
    #     slab-like fast path can read the misra-heap column.
    "misra-slab":       "BM_AllocFreePair/(16|64|256|1024|4096)$|BM_BatchAllocFree|BM_AllocTouchFree/(64|4096)$",

    # ArenaAllocator: bump-pointer with single-deep LIFO rewind on free.
    #   AllocFreePair / AllocTouchFree -- alloc-then-free-immediately,
    #     last_ptr matches every time, zero net growth.
    #   ArenaBumpReset -- the workload arena is designed for.
    #   ReallocGrow -- single buffer growing in place; arena's last-ptr
    #     remap fast-path handles each step.
    # Excluded:
    #   BatchAllocFree / MixedPareto / Frag_* -- hold N allocations
    #     live and then free them; arena's single-deep LIFO can't
    #     rewind for the (N-1) older items, and memory would grow
    #     without bound under gbench's auto-scaled iteration count.
    "misra-arena":      "BM_(AllocFreePair|AllocTouchFree|ArenaBumpReset|ReallocGrow)",

    # PageAllocator: one mmap per alloc, page-rounded.
    #   AllocFreePair at page sizes and up.
    #   AllocTouchFree at page sizes and up.
    # Excluded:
    #   Everything sub-page -- a 64 B request gets a whole page back;
    #     measuring that says more about the rounding waste than the
    #     allocator. Many-live-allocs (BatchAllocFree) and mixed-size
    #     (MixedPareto / Frag_*) would also waste pages; ReallocGrow's
    #     ladder fights the retain-on-free policy across step sizes.
    "misra-page":       "BM_AllocFreePair/(4096|16384|65536)$|BM_AllocTouchFree/(4096|65536)$",

    # BudgetAllocator: caller-buffer fixed-budget pool, no growth.
    #   AllocFreePair / AllocTouchFree -- single slot live at a time,
    #     trivially fits the 16 MiB backing buffer at any slot size.
    #   BatchAllocFree -- 8192 * 64 B = 512 KiB live, fits with margin.
    # Excluded:
    #   MixedPareto / Frag_* -- mixed sizes; the pool has one slot for
    #     life. ReallocGrow's ladder grows past the configured slot at
    #     each step. ArenaBumpReset -- no bulk reset.
    "misra-budget":     "BM_AllocFreePair|BM_BatchAllocFree|BM_AllocTouchFree",
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


def render_timing_table(rows: list, unit_hint: str, data: dict, backends: list[str]) -> str:
    """Render one markdown table with allocators as columns, benchmark rows as rows."""
    hdr = ["benchmark"] + backends
    aligns = ["---"] + [f"---:" for _ in backends]
    out = ["| " + " | ".join(hdr) + " |"]
    out.append("|" + "|".join(aligns) + "|")
    for bench_name, label, unit in rows:
        cells = [label]
        for be in backends:
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


def render_frag_table(data: dict, backends: list[str]) -> str:
    """Fragmentation table: per backend, cell shows "live / footprint" in MB.

    Both numbers come from the backend's OWN stats API -- no harness
    bookkeeping covers for a missing API. A backend that doesn't expose
    live (mimalloc 3.3.0's broken `mi_stats_get`; MisraStdC backends
    built with FEATURE_ALLOC_STATS=false) renders "n/a / <fp>" or just
    "n/a" if footprint is also missing.
    """
    cols = ["benchmark"] + [f"{be} (live / fp)" for be in backends]
    aligns = ["---"] + ["---:" for _ in backends]
    out = ["| " + " | ".join(cols) + " |"]
    out.append("|" + "|".join(aligns) + "|")
    for bench_name, label in FRAG_ROWS:
        cells = [label]
        for be in backends:
            j = data.get(be)
            if j is None:
                cells.append("n/a")
                continue
            live = get_counter(j, bench_name, "live_MB")
            fp   = get_counter(j, bench_name, "footprint_MB")
            live_s = f"{live:.1f}" if live not in (None, 0.0) else "n/a"
            fp_s   = f"{fp:.1f}"   if fp   not in (None, 0.0) else "n/a"
            if live_s == "n/a" and fp_s == "n/a":
                cells.append("n/a")
            else:
                cells.append(f"{live_s} / {fp_s}")
        out.append("| " + " | ".join(cells) + " |")
    out.append("")
    out.append("_Per backend: live (allocator's reported outstanding bytes) / footprint (committed bytes from OS). MB. Lower footprint at the same live = less fragmentation. `n/a` means that backend's stats API didn't expose the number._")
    return "\n".join(out)


def build_tldr(data: dict) -> str:
    """Pull a couple of headline numbers for the TL;DR.

    One row per backend that's present in `data`, in the same order as
    BACKENDS. No editorialising about which is "correct" -- the row is
    just the 16 B single alloc/free pair number; the reader picks.
    """
    def pair_at(be: str, name: str) -> str:
        t = median_time_ns(data.get(be, {}), name)
        return f"{t:.1f} ns" if t is not None else "n/a"

    lines = [
        "Single alloc/free pair, 16 B:",
        "",
        "| backend | time |",
        "|---|---:|",
    ]
    for be in BACKENDS:
        if be not in data:
            continue
        lines.append(f"| {be:<13} | {pair_at(be, 'BM_AllocFreePair/16')} |")
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
    def opts_string(d: Path) -> str:
        opts_intro = d / "meson-info" / "intro-buildoptions.json"
        if not opts_intro.exists():
            return "unknown"
        try:
            opts = json.loads(opts_intro.read_text())
            picks = []
            # Capture both meson-builtin knobs (buildtype/optimization/etc.)
            # and the project's safety/perf-relevant flags so the misra
            # numbers are interpretable.
            wanted = (
                "buildtype",
                "optimization",
                "b_lto",
                "b_sanitize",
                "alloc_debug",
                "alloc_stats",
            )
            for o in opts:
                if o.get("name") in wanted:
                    picks.append(f"{o['name']}={o['value']}")
            return " ".join(picks) or "default"
        except Exception:
            return "unknown"

    build_options = opts_string(builddir)
    return {
        "TIMESTAMP": datetime.now(timezone.utc).strftime("%Y-%m-%d %H:%M:%S UTC"),
        "COMMIT": commit,
        "CPU": cpu,
        "KERNEL": kernel,
        "COMPILER": compiler,
        "BUILD_OPTIONS": build_options,
    }


PERF_BENCHES_FILTER = "BM_(AllocFreePair|BatchAllocFree|AllocTouchFree|MixedPareto|ReallocGrow|ArenaBumpReset)"
FRAG_BENCHES_FILTER = "BM_Frag_"


def main() -> int:
    ap = argparse.ArgumentParser(
        description="Run bench-* binaries and refresh Benchmark/README.md. "
                    "Uses two builds: timing rows from the perf builddir "
                    "(alloc_stats=false), fragmentation rows from the frag "
                    "builddir (alloc_stats=true)."
    )
    ap.add_argument("builddir", type=Path,
                    help="Perf build directory (alloc_stats=false). Used for "
                         "all timing-row binaries.")
    ap.add_argument("--frag-builddir", type=Path, default=None,
                    help="Fragmentation build directory (alloc_stats=true). "
                         "Used for fragmentation-row binaries so misra "
                         "columns can report live bytes. "
                         "Defaults to <builddir>-frag.")
    ap.add_argument("--reps", type=int, default=10, help="Repetitions per benchmark (default 10)")
    ap.add_argument(
        "--out",
        type=Path,
        default=None,
        help="Output README path (default Benchmark/README.md relative to repo root)",
    )
    args = ap.parse_args()

    builddir: Path = args.builddir.resolve()
    if not (builddir / "Benchmark").is_dir():
        print(f"error: no Benchmark/ subdir under perf builddir {builddir}. "
              f"Did you forget -Dbenchmark=true?", file=sys.stderr)
        return 1

    if args.frag_builddir is not None:
        frag_builddir: Path = args.frag_builddir.resolve()
    else:
        frag_builddir = builddir.parent / (builddir.name + "-frag")
    if not (frag_builddir / "Benchmark").is_dir():
        print(f"error: no Benchmark/ subdir under frag builddir {frag_builddir}. "
              f"Set up a second build with -Dalloc_stats=true, or pass "
              f"--frag-builddir.", file=sys.stderr)
        return 1

    backends = list(BACKENDS)

    # Each column maps directly to its bench-* binary name.
    # Perf rows come from the perf builddir; frag rows from the frag builddir.
    # The two JSONs are merged into one `benchmarks` list per column so the
    # downstream renderers see a unified view.
    data: dict[str, dict] = {}
    for col in backends:
        bin_name       = binary_name_for(col)
        perf_bin       = builddir / "Benchmark" / f"bench-{bin_name}"
        frag_bin       = frag_builddir / "Benchmark" / f"bench-{bin_name}"
        backend_filter = BENCHMARK_FILTERS.get(bin_name)
        min_time       = MIN_TIME_OVERRIDES.get(bin_name, "0.5s")
        merged: list = []

        # Perf run: backend's existing filter (already excludes BM_Frag_*
        # by construction), AND'd with PERF_BENCHES_FILTER when the backend
        # has no filter of its own.
        perf_filter_re = backend_filter if backend_filter is not None else PERF_BENCHES_FILTER
        if perf_bin.exists():
            print(f"[perf] bench-{bin_name}  (min_time={min_time}, filter={perf_filter_re})",
                  file=sys.stderr)
            perf_data = run_binary(perf_bin, args.reps, min_time=min_time, filter_re=perf_filter_re)
            merged.extend(perf_data.get("benchmarks", []))
        else:
            print(f"warn: {perf_bin} missing -- skipping perf for {col}", file=sys.stderr)

        # Frag run: only backends whose contract allows BM_Frag_* (i.e. those
        # with NO per-backend filter). The slab/arena/page/budget filters all
        # exclude Frag explicitly; running them against the frag builddir would
        # produce empty JSON anyway, so save the time.
        if backend_filter is None:
            if frag_bin.exists():
                print(f"[frag] bench-{bin_name}  (min_time={min_time}, filter={FRAG_BENCHES_FILTER})",
                      file=sys.stderr)
                frag_data = run_binary(frag_bin, args.reps, min_time=min_time, filter_re=FRAG_BENCHES_FILTER)
                merged.extend(frag_data.get("benchmarks", []))
            else:
                print(f"warn: {frag_bin} missing -- skipping frag for {col}", file=sys.stderr)

        if merged:
            data[col] = {"benchmarks": merged}

    # Build substitution map. Env metadata from the perf builddir; frag
    # builddir's config (b_lto, alloc_stats, etc.) reported separately
    # so the reader can see which build each row group used.
    perf_env = gather_env(builddir)
    frag_env = gather_env(frag_builddir)
    subs = {f"{{{{{k}}}}}": v for k, v in perf_env.items()}
    subs["{{BUILD_OPTIONS_FRAG}}"] = frag_env["BUILD_OPTIONS"]
    subs["{{REPS}}"] = str(args.reps)
    subs["{{TLDR}}"] = build_tldr(data)
    for placeholder, rows in TABLES.items():
        unit_hint = rows[0][2]
        subs[f"{{{{{placeholder}}}}}"] = render_timing_table(rows, unit_hint, data, backends)
    subs["{{TABLE_FRAG}}"] = render_frag_table(data, backends)

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
