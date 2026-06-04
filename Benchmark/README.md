# MisraStdC Allocator Benchmark

> Generated automatically by `Scripts/run.py`. Do not edit by hand.

One column per allocator. The libc-shape columns (`glibc`, `jemalloc`, `mimalloc`, `tcmalloc`) sit next to one column for each MisraStdC allocator type (`misra-heap`, `misra-slab`, `misra-arena`, `misra-page`, `misra-budget`). Rows that fall outside an allocator's contract -- mixed sizes for a fixed-slot allocator, sub-page requests for `PageAllocator`, many-live-allocs for `ArenaAllocator`, and so on -- read `n/a`. The benchmark does not decide which allocator is the "right tool" for a workload; the reader does.

`DebugAllocator` is intentionally not benchmarked. It's a diagnostic wrapper (leak / canary / trace tracking) for tests and fuzz, not a production allocator -- the column would measure diagnostic overhead, not allocator design.

## Headline

Single alloc/free pair, 16 B:

| backend | time |
|---|---:|
| glibc         | 5.3 ns |
| jemalloc      | 3.7 ns |
| mimalloc      | 3.0 ns |
| tcmalloc      | 3.7 ns |
| misra-heap    | 13.3 ns |
| misra-slab    | 8.3 ns |
| misra-arena   | 6.9 ns |
| misra-page    | n/a |
| misra-budget  | 11.2 ns |

## Timing

### Single alloc/free pair

One `alloc(size)` immediately followed by `free(ptr)`, repeated. Hot reuse — the same slot churns. Time per pair, lower is better.

| benchmark | glibc | jemalloc | mimalloc | tcmalloc | misra-heap | misra-slab | misra-arena | misra-page | misra-budget |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| 16 B | 5.3 | 3.7 | 3.0 | 3.7 | 13.3 | 8.3 | 6.9 | n/a | 11.2 |
| 64 B | 5.1 | 3.8 | 3.2 | 3.7 | 13.0 | 8.1 | 6.9 | n/a | 11.2 |
| 256 B | 5.1 | 4.3 | 13.9 | 3.6 | 13.1 | 8.1 | 6.9 | n/a | 11.2 |
| 1 KiB | 5.0 | 5.7 | 15.3 | 3.5 | 13.0 | 8.1 | 6.9 | n/a | 11.2 |
| 4 KiB | 16.8 | 10.4 | 11.0 | 3.6 | 17.6 | 8.1 | 6.9 | 11.5 | 11.2 |
| 16 KiB | 17.0 | 20.6 | 11.0 | 3.6 | 17.6 | n/a | 6.9 | 11.5 | 11.2 |
| 64 KiB | 17.1 | 168.4 | 19.9 | 3.7 | 17.6 | n/a | 6.9 | 11.6 | 10.0 |

_Values in ns._

### Batch alloc-N-then-free-N

`N` allocs of fixed size, then `N` frees, then repeat. Holds `N` allocations live at peak. Time per batch, lower is better.

| benchmark | glibc | jemalloc | mimalloc | tcmalloc | misra-heap | misra-slab | misra-arena | misra-page | misra-budget |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| 128 × 64 B | 1.5 | 0.5 | 0.4 | 0.5 | 1.4 | 1.2 | n/a | n/a | 1.5 |
| 1024 × 64 B | 10.7 | 11.9 | 3.6 | 3.9 | 11.5 | 21.3 | n/a | n/a | 12.8 |
| 8192 × 64 B | 83.0 | 103.8 | 32.7 | 39.1 | 91.4 | 650.3 | n/a | n/a | 168.6 |

_Values in us._

### Alloc + write every byte + free

Same shape as the pair test but writes every byte of the allocation before freeing. Catches page-fault cost that pure alloc/free hides. Time per item, lower is better.

| benchmark | glibc | jemalloc | mimalloc | tcmalloc | misra-heap | misra-slab | misra-arena | misra-page | misra-budget |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| 64 B | 6.0 | 4.7 | 3.3 | 4.4 | 13.7 | 10.6 | 9.4 | n/a | 13.3 |
| 4 KiB | 41.2 | 23.9 | 24.7 | 19.1 | 36.0 | 26.6 | 50.7 | 30.7 | 31.0 |
| 64 KiB | 861.3 | 1063.8 | 870.3 | 840.3 | 848.3 | n/a | 907.5 | 843.9 | 841.6 |

_Values in ns._

### Mixed-size Pareto

512-allocation batch with sizes drawn from a Pareto(α=1.16, xm=24) distribution capped at 256 KiB. Time per batch, lower is better.

| benchmark | glibc | jemalloc | mimalloc | tcmalloc | misra-heap | misra-slab | misra-arena | misra-page | misra-budget |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| Pareto(1.16, 24) | 27.8 | 11.3 | 11.1 | 11.6 | 24.6 | n/a | n/a | n/a | n/a |

_Values in us._

### Realloc growth

Geometric realloc ladder from 8 B up to 1 MiB. Time per full ladder, lower is better.

| benchmark | glibc | jemalloc | mimalloc | tcmalloc | misra-heap | misra-slab | misra-arena | misra-page | misra-budget |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| 8 B → 1 MiB | 5721.1 | 5296.2 | 17982.3 | 15251.9 | 16539.7 | n/a | 93.3 | n/a | n/a |

_Values in ns._

### Arena bump + bulk reset

Allocate N small (32 B) objects, then release them all. Arena does this as one O(1) reset; every other backend has to free each pointer individually. Time per batch, lower is better.

| benchmark | glibc | jemalloc | mimalloc | tcmalloc | misra-heap | misra-slab | misra-arena | misra-page | misra-budget |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| 128 × 32 B | n/a | n/a | n/a | n/a | n/a | n/a | 0.5 | n/a | n/a |
| 1024 × 32 B | n/a | n/a | n/a | n/a | n/a | n/a | 3.9 | n/a | n/a |
| 8192 × 32 B | n/a | n/a | n/a | n/a | n/a | n/a | 31.0 | n/a | n/a |

_Values in us._

## Fragmentation

Each cell shows `live / footprint` for one backend, both numbers as the backend's OWN introspection API reports them. `live` is what's currently outstanding to the user; `footprint` is what the allocator pulled from the OS. Lower footprint at the same live = less fragmentation. `n/a` means the backend's stats API didn't expose the number -- the harness does not synthesize a value to fill the cell.

| benchmark | glibc (live / fp) | jemalloc (live / fp) | mimalloc (live / fp) | tcmalloc (live / fp) | misra-heap (live / fp) | misra-slab (live / fp) | misra-arena (live / fp) | misra-page (live / fp) | misra-budget (live / fp) |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| Checkerboard (4 K small) | 0.8 / 1.1 | 0.8 / 9.1 | n/a | 0.8 / 2.0 | 0.6 / 0.8 | n/a | n/a | n/a | n/a |
| Checkerboard (16 K small) | 3.0 / 3.7 | 2.8 / 11.7 | n/a | 2.8 / 4.0 | 2.5 / 3.1 | n/a | n/a | n/a | n/a |
| Checkerboard (64 K small) | 11.8 / 14.4 | 10.9 / 21.1 | n/a | 10.8 / 14.0 | 10.0 / 12.5 | n/a | n/a | n/a | n/a |
| Lifetime mix | 4.4 / 4.7 | 4.3 / 14.7 | n/a | 4.1 / 14.0 | 3.9 / 4.6 | n/a | n/a | n/a | n/a |
| Page overhang | 19.7 / 19.8 | 23.5 / 31.7 | n/a | 21.3 / 27.0 | 36.3 / 38.4 | n/a | n/a | n/a | n/a |

_Per backend: live (allocator's reported outstanding bytes) / footprint (committed bytes from OS). MB. Lower footprint at the same live = less fragmentation. `n/a` means that backend's stats API didn't expose the number._

## How to read

Each `misra-*` column is one MisraStdC allocator type, not a choice between "fast" and "correct". The libc-shape columns are general-purpose by design and run every row; the MisraStdC columns vary by contract:

- **`misra-heap`** — `HeapAllocator`. General-purpose binned heap; runs every workload. The baseline for an apples-to-apples comparison against glibc / jemalloc / mimalloc / tcmalloc.
- **`misra-slab`** — `SlabAllocator(slot)`. Fixed slot, power of two in [16, 4096]. Runs the fixed-size rows whose slot fits the cap (`AllocFreePair/16..4096`, all `BatchAllocFree`, `AllocTouchFree/64,4096`). `n/a` everywhere else: super-page slots, mixed sizes, and any realloc that crosses the slot.
- **`misra-arena`** — `ArenaAllocator`. Bump-pointer with single-deep LIFO rewind on free and one bulk O(1) `Reset`. Runs the pair-style rows (last-ptr rewind keeps growth flat), `ReallocGrow` (last-ptr remap fast-path), and `ArenaBumpReset`. `n/a` on many-live-allocs and mixed sizes -- the workload would grow unbounded.
- **`misra-page`** — `PageAllocator`. One mmap per alloc, page-rounded. Restricted to page-aligned sizes (4 KiB and up) so the row reflects mmap dispatch and not rounding waste from sub-page requests. `n/a` everywhere else.
- **`misra-budget`** — `BudgetAllocator`. Caller-buffer fixed-budget pool, no growth. Runs the fixed-size rows that fit the 16 MiB backing buffer. `n/a` on mixed sizes and realloc (one slot for life).

`n/a` is a deliberate signal that the row falls outside the allocator's contract. The reader picks the workload-matched column instead of relying on the benchmark to pick one for them.

## Reproduce

Two builds: timing rows want `-Dalloc_stats=false` so per-allocator counters don't bias the hot path; fragmentation rows want `-Dalloc_stats=true` so the misra columns can report live bytes. `run.py` runs each row group against the matching binary.

```sh
# perf build (timing rows)
meson setup build      -Dbenchmark=true -Dbuildtype=release -Doptimization=3 -Dalloc_stats=false
ninja -C build

# frag build (fragmentation rows)
meson setup build-frag -Dbenchmark=true -Dbuildtype=release -Doptimization=3 -Dalloc_stats=true
ninja -C build-frag

python3 Benchmark/Scripts/run.py build
# --frag-builddir defaults to <perf-builddir>-frag (i.e. "build-frag" here).
```

Regenerates this file with measurements from the host.

## Environment

| | |
|---|---|
| timestamp | 2026-06-04 08:41:34 UTC |
| git rev   | e55e50d6f84e (master) |
| host CPU  | Intel(R) Core(TM) Ultra 7 165U |
| kernel    | Linux 6.18.32 |
| compiler  | gcc 15.2.0 |
| build (perf) | buildtype=release optimization=3 b_sanitize=[] b_lto=False alloc_stats=False alloc_debug=True |
| build (frag) | buildtype=release optimization=3 b_sanitize=[] b_lto=False alloc_stats=True alloc_debug=True |
| reps      | 10 per row (median reported) |
