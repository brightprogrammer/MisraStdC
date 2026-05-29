# MisraStdC Allocator Benchmark

> Generated automatically by `Scripts/run.py`. Do not edit by hand.

Compares MisraStdC's `HeapAllocator` and `SlabAllocator` against four production allocators: glibc, jemalloc, mimalloc, tcmalloc.

## Headline

Single alloc/free pair, 16 B:

| backend | time |
|---|---:|
| tcmalloc | 3.4 ns |
| glibc    | 7.2 ns |
| jemalloc | 3.4 ns |
| mimalloc | 2.9 ns |
| misra (Heap, validate-full) | 13.4 ns |
| misra (Heap, validate-fast) | 13.0 ns |
| misra-correct (Slab, validate-full) | 12.8 ns |
| misra-correct (Slab, validate-fast) | 8.8 ns |

## Timing

### Single alloc/free pair

One `alloc(size)` immediately followed by `free(ptr)`, repeated. Hot reuse — the same slot churns. Time per pair, lower is better.

| benchmark | glibc | jemalloc | mimalloc | tcmalloc | misra-full | misra-fast | misra-correct-full | misra-correct-fast | misra-arena | misra-page |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| 16 B | 7.2 | 3.4 | 2.9 | 3.4 | 13.4 | 13.0 | 12.8 | 8.8 | 9.9 | n/a |
| 64 B | 7.0 | 3.5 | 3.0 | 3.3 | 13.5 | 13.3 | 12.7 | 8.7 | 9.9 | n/a |
| 256 B | 7.2 | 3.9 | 13.5 | 3.3 | 13.7 | 13.3 | 12.7 | 8.7 | 9.9 | n/a |
| 1 KiB | 7.2 | 5.2 | 14.8 | 3.3 | 13.2 | 13.2 | 12.7 | 8.7 | 9.9 | n/a |
| 4 KiB | 22.9 | 9.7 | 10.6 | 3.3 | 1514.5 | 1517.9 | 12.7 | 8.7 | 9.9 | 16.6 |
| 16 KiB | 23.4 | 18.7 | 19.9 | 3.4 | 1159.8 | 1152.9 | 16.9 | 16.9 | 9.9 | 16.7 |
| 64 KiB | 25.6 | 270.3 | 20.0 | 3.4 | 1529.7 | 1531.6 | 16.9 | 16.9 | 9.9 | 16.8 |

_Values in ns._

### Batch alloc-N-then-free-N

`N` allocs of fixed size, then `N` frees, then repeat. Holds `N` allocations live at peak. Time per batch, lower is better.

| benchmark | glibc | jemalloc | mimalloc | tcmalloc | misra-full | misra-fast | misra-correct-full | misra-correct-fast | misra-arena | misra-page |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| 128 × 64 B | 1.2 | 1.0 | 0.5 | 0.5 | 1.5 | 1.4 | 1.7 | 1.0 | n/a | n/a |
| 1024 × 64 B | 9.6 | 10.9 | 4.0 | 4.3 | 11.8 | 11.7 | 17.5 | 12.2 | n/a | n/a |
| 8192 × 64 B | 77.9 | 88.9 | 34.3 | 39.1 | 94.7 | 94.6 | 390.2 | 358.8 | n/a | n/a |

_Values in us._

### Alloc + write every byte + free

Same shape as the pair test but writes every byte of the allocation before freeing. Catches page-fault cost that pure alloc/free hides. Time per item, lower is better.

| benchmark | glibc | jemalloc | mimalloc | tcmalloc | misra-full | misra-fast | misra-correct-full | misra-correct-fast | misra-arena | misra-page |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| 64 B | 7.0 | 4.3 | 3.3 | 4.1 | 14.4 | 14.3 | 14.4 | 9.9 | 10.7 | n/a |
| 4 KiB | 33.0 | 23.1 | 24.1 | 18.8 | 2348.6 | 2364.9 | 30.9 | 25.0 | 56.3 | 33.7 |
| 64 KiB | 864.7 | 1063.5 | 865.2 | 839.8 | 13202.5 | 13183.3 | 847.7 | 847.2 | 949.9 | 847.0 |

_Values in ns._

### Mixed-size Pareto

512-allocation batch with sizes drawn from a Pareto(α=1.16, xm=24) distribution capped at 256 KiB. Time per batch, lower is better.

| benchmark | glibc | jemalloc | mimalloc | tcmalloc | misra-full | misra-fast | misra-correct-full | misra-correct-fast | misra-arena | misra-page |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| Pareto(1.16, 24) | 28.6 | 10.9 | 11.3 | 11.1 | 32.5 | 32.6 | 32.2 | 31.8 | n/a | n/a |

_Values in us._

### Realloc growth

Geometric realloc ladder from 8 B up to 1 MiB. Time per full ladder, lower is better.

| benchmark | glibc | jemalloc | mimalloc | tcmalloc | misra-full | misra-fast | misra-correct-full | misra-correct-fast | misra-arena | misra-page |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| 8 B → 1 MiB | 8265.9 | 4054.7 | 18185.7 | 15349.6 | 15787.3 | 16312.1 | 16030.5 | 16180.3 | 88.3 | n/a |

_Values in ns._

### Arena bump + bulk reset

Allocate N small (32 B) objects, then release them all. Arena does this as one O(1) reset; every other backend has to free each pointer individually. Time per batch, lower is better.

| benchmark | glibc | jemalloc | mimalloc | tcmalloc | misra-full | misra-fast | misra-correct-full | misra-correct-fast | misra-arena | misra-page |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| 128 × 32 B | n/a | n/a | n/a | n/a | n/a | n/a | n/a | n/a | 0.6 | n/a |
| 1024 × 32 B | n/a | n/a | n/a | n/a | n/a | n/a | n/a | n/a | 4.9 | n/a |
| 8192 × 32 B | n/a | n/a | n/a | n/a | n/a | n/a | n/a | n/a | 39.5 | n/a |

_Values in us._

## Fragmentation

Each allocator's own introspection API reports committed bytes after the workload runs. Lower committed-bytes for the same live-bytes is better.

| benchmark | live MB | glibc MB | jemalloc MB | mimalloc MB | tcmalloc MB | misra-full MB | misra-fast MB | misra-correct-full MB | misra-correct-fast MB | misra-arena MB | misra-page MB |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| Checkerboard (4 K small) | 0.6 | 1.1 | 9.2 | n/a | 2.0 | 0.6 | 0.6 | n/a | n/a | n/a | n/a |
| Checkerboard (16 K small) | 2.5 | 3.9 | 11.8 | n/a | 4.0 | 2.5 | 2.5 | n/a | n/a | n/a | n/a |
| Checkerboard (64 K small) | 10.0 | 15.1 | 21.8 | n/a | 14.0 | 10.0 | 10.0 | n/a | n/a | n/a | n/a |
| Lifetime mix | 4.0 | 4.9 | 20.2 | n/a | 14.0 | 4.0 | 4.0 | n/a | n/a | n/a | n/a |
| Page overhang | 18.2 | 20.3 | 32.4 | n/a | 27.0 | 18.2 | 18.2 | n/a | n/a | n/a | n/a |

## How to read

MisraStdC's typed-allocator family shows up as several columns:

- **`misra` / `misra-correct`** — `HeapAllocator` everywhere vs the right tool per workload (`SlabAllocator(slot_size)` for fixed-size benches, `HeapAllocator` for mixed). The latter is what a real MisraStdC caller would do.
- **`*-full` vs `*-fast`** — same source, different `heap_validate_full` setting. `-full` does per-dispatch cross-class checks plus volatile descriptor probes (project default; catches torn pointers / freed metadata). `-fast` is the magic-only check (catches uninitialised / post-deinit / type-confusion only). The gap between them is MisraStdC's per-call safety tax — turn it off in shipping release builds for tighter dispatch.
- **`misra-arena` / `misra-page`** — specialised backends (bump+bulk-reset, mmap-per-alloc). `n/a` rows mean the workload doesn't fit the backend's contract (e.g. arena can't handle many-live-allocs; page wastes whole pages on sub-page requests).

Where tcmalloc beats `misra-correct-fast` on small AllocFreePair, the gap is MisraStdC's structural safety (double-free, misalignment, magic-tag dispatch) that production allocators skip even in their fast paths.

## Reproduce

Single-tier run (validate-full only):

```sh
meson setup build -Dbenchmark=true -Dbuildtype=release -Doptimization=3
ninja  -C build
python3 Benchmark/Scripts/run.py build
```

Two-tier run that shows the safety overhead side-by-side (matches the numbers above):

```sh
meson setup build-full -Dbenchmark=true -Dbuildtype=release -Doptimization=3 \
    -Dalloc_debug=false -Dheap_validate_full=true
meson setup build-fast -Dbenchmark=true -Dbuildtype=release -Doptimization=3 \
    -Dalloc_debug=false -Dheap_validate_full=false
ninja -C build-full
ninja -C build-fast
python3 Benchmark/Scripts/run.py build-full --validate-fast build-fast
```

Regenerates this file with measurements from the host.

## Environment

| | |
|---|---|
| timestamp | 2026-05-29 02:14:47 UTC |
| git rev   | af143d4679e6 (master) |
| host CPU  | Intel(R) Core(TM) Ultra 7 165U |
| kernel    | Linux 6.18.28 |
| compiler  | gcc 14.3.0 |
| build     | validate-full: buildtype=release optimization=3 b_lto=True b_sanitize=[] alloc_debug=False heap_validate_full=True  |  validate-fast: buildtype=release optimization=3 b_lto=True b_sanitize=[] alloc_debug=False heap_validate_full=False |
| reps      | 10 per row (median reported) |
