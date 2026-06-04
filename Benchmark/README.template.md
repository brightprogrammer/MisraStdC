# MisraStdC Allocator Benchmark

> Generated automatically by `Scripts/run.py`. Do not edit by hand.

One column per allocator. The libc-shape columns (`glibc`, `jemalloc`, `mimalloc`, `tcmalloc`) sit next to one column for each MisraStdC allocator type (`misra-heap`, `misra-slab`, `misra-arena`, `misra-page`, `misra-budget`). Rows that fall outside an allocator's contract -- mixed sizes for a fixed-slot allocator, sub-page requests for `PageAllocator`, many-live-allocs for `ArenaAllocator`, and so on -- read `n/a`. The benchmark does not decide which allocator is the "right tool" for a workload; the reader does.

`DebugAllocator` is intentionally not benchmarked. It's a diagnostic wrapper (leak / canary / trace tracking) for tests and fuzz, not a production allocator -- the column would measure diagnostic overhead, not allocator design.

## Headline

{{TLDR}}

## Timing

### Single alloc/free pair

One `alloc(size)` immediately followed by `free(ptr)`, repeated. Hot reuse — the same slot churns. Time per pair, lower is better.

{{TABLE_ALLOC_FREE_PAIR}}

### Batch alloc-N-then-free-N

`N` allocs of fixed size, then `N` frees, then repeat. Holds `N` allocations live at peak. Time per batch, lower is better.

{{TABLE_BATCH_ALLOC_FREE}}

### Alloc + write every byte + free

Same shape as the pair test but writes every byte of the allocation before freeing. Catches page-fault cost that pure alloc/free hides. Time per item, lower is better.

{{TABLE_ALLOC_TOUCH_FREE}}

### Mixed-size Pareto

512-allocation batch with sizes drawn from a Pareto(α=1.16, xm=24) distribution capped at 256 KiB. Time per batch, lower is better.

{{TABLE_MIXED_PARETO}}

### Realloc growth

Geometric realloc ladder from 8 B up to 1 MiB. Time per full ladder, lower is better.

{{TABLE_REALLOC_GROW}}

### Arena bump + bulk reset

Allocate N small (32 B) objects, then release them all. Arena does this as one O(1) reset; every other backend has to free each pointer individually. Time per batch, lower is better.

{{TABLE_ARENA_BUMP_RESET}}

## Fragmentation

Each cell shows `live / footprint` for one backend, both numbers as the backend's OWN introspection API reports them. `live` is what's currently outstanding to the user; `footprint` is what the allocator pulled from the OS. Lower footprint at the same live = less fragmentation. `n/a` means the backend's stats API didn't expose the number -- the harness does not synthesize a value to fill the cell.

{{TABLE_FRAG}}

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
| timestamp | {{TIMESTAMP}} |
| git rev   | {{COMMIT}} |
| host CPU  | {{CPU}} |
| kernel    | {{KERNEL}} |
| compiler  | {{COMPILER}} |
| build (perf) | {{BUILD_OPTIONS}} |
| build (frag) | {{BUILD_OPTIONS_FRAG}} |
| reps      | {{REPS}} per row (median reported) |
