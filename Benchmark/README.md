# MisraStdC Allocator Benchmark

> Generated automatically by `Scripts/run.py`. Do not edit by hand.

Compares MisraStdC's `HeapAllocator` and `SlabAllocator` against four production allocators: glibc, jemalloc, mimalloc, tcmalloc.

## Headline

Single alloc/free pair, 16 B:

| backend | time |
|---|---:|
| tcmalloc | 3.7 ns |
| glibc    | 7.7 ns |
| jemalloc | 10.8 ns |
| mimalloc | 3.8 ns |
| misra (Heap, validate-full) | 15.0 ns |
| misra (Heap, validate-fast) | 14.2 ns |
| misra-correct (Slab, validate-full) | 14.2 ns |
| misra-correct (Slab, validate-fast) | 8.7 ns |

## Timing

### Single alloc/free pair

One `alloc(size)` immediately followed by `free(ptr)`, repeated. Hot reuse — the same slot churns. Time per pair, lower is better.

| benchmark | glibc | jemalloc | mimalloc | tcmalloc | misra-full | misra-fast | misra-correct-full | misra-correct-fast | misra-arena | misra-page |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| 16 B | 7.7 | 10.8 | 3.8 | 3.7 | 15.0 | 14.2 | 14.2 | 8.7 | 11.8 | n/a |
| 64 B | 7.5 | 11.1 | 3.8 | 3.8 | 15.5 | 14.8 | 14.1 | 8.7 | 11.7 | n/a |
| 256 B | 7.5 | 11.4 | 16.7 | 3.9 | 15.2 | 14.9 | 14.0 | 8.7 | 11.7 | n/a |
| 1 KiB | 19.7 | 12.4 | 17.4 | 3.8 | 14.6 | 14.6 | 13.9 | 8.6 | 11.7 | n/a |
| 4 KiB | 15.5 | 16.6 | 13.1 | 4.1 | 1185.1 | 1155.4 | 13.8 | 8.6 | 11.6 | 18.5 |
| 16 KiB | 16.1 | 38.7 | 23.1 | 4.0 | 1185.6 | 1162.7 | 19.0 | 19.0 | 11.6 | 18.4 |
| 64 KiB | 17.5 | 531.4 | 22.1 | 4.2 | 1193.6 | 1164.4 | 19.0 | 18.9 | 11.7 | 18.3 |

_Values in ns._

### Batch alloc-N-then-free-N

`N` allocs of fixed size, then `N` frees, then repeat. Holds `N` allocations live at peak. Time per batch, lower is better.

| benchmark | glibc | jemalloc | mimalloc | tcmalloc | misra-full | misra-fast | misra-correct-full | misra-correct-fast | misra-arena | misra-page |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| 128 × 64 B | 1.4 | 1.3 | 0.5 | 0.5 | 1.6 | 1.6 | 1.8 | 1.2 | n/a | n/a |
| 1024 × 64 B | 11.2 | 24.9 | 4.2 | 4.3 | 13.0 | 12.8 | 18.8 | 15.4 | n/a | n/a |
| 8192 × 64 B | 87.9 | 109.1 | 37.9 | 43.6 | 104.9 | 101.2 | 401.1 | 375.0 | n/a | n/a |

_Values in us._

### Alloc + write every byte + free

Same shape as the pair test but writes every byte of the allocation before freeing. Catches page-fault cost that pure alloc/free hides. Time per item, lower is better.

| benchmark | glibc | jemalloc | mimalloc | tcmalloc | misra-full | misra-fast | misra-correct-full | misra-correct-fast | misra-arena | misra-page |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| 64 B | 8.1 | 5.6 | 3.5 | 4.6 | 15.5 | 15.2 | 16.7 | 11.0 | 13.8 | n/a |
| 4 KiB | 41.4 | 29.6 | 27.7 | 21.8 | 2389.9 | 2375.9 | 33.8 | 28.3 | 54.3 | 38.4 |
| 64 KiB | 1290.2 | 1285.7 | 1001.9 | 977.8 | 13254.3 | 13046.3 | 848.2 | 849.2 | 915.5 | 848.8 |

_Values in ns._

### Mixed-size Pareto

512-allocation batch with sizes drawn from a Pareto(α=1.16, xm=24) distribution capped at 256 KiB. Time per batch, lower is better.

| benchmark | glibc | jemalloc | mimalloc | tcmalloc | misra-full | misra-fast | misra-correct-full | misra-correct-fast | misra-arena | misra-page |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| Pareto(1.16, 24) | 30.3 | 15.1 | 12.8 | 12.6 | 32.8 | 32.1 | 32.9 | 33.8 | n/a | n/a |

_Values in us._

### Realloc growth

Geometric realloc ladder from 8 B up to 1 MiB. Time per full ladder, lower is better.

| benchmark | glibc | jemalloc | mimalloc | tcmalloc | misra-full | misra-fast | misra-correct-full | misra-correct-fast | misra-arena | misra-page |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| 8 B → 1 MiB | 5740.1 | 5188.4 | 22553.0 | 18382.7 | 17088.7 | 17618.5 | 16174.7 | 16072.4 | 126.0 | n/a |

_Values in ns._

### Arena bump + bulk reset

Allocate N small (32 B) objects, then release them all. Arena does this as one O(1) reset; every other backend has to free each pointer individually. Time per batch, lower is better.

| benchmark | glibc | jemalloc | mimalloc | tcmalloc | misra-full | misra-fast | misra-correct-full | misra-correct-fast | misra-arena | misra-page |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| 128 × 32 B | n/a | n/a | n/a | n/a | n/a | n/a | n/a | n/a | 0.8 | n/a |
| 1024 × 32 B | n/a | n/a | n/a | n/a | n/a | n/a | n/a | n/a | 6.2 | n/a |
| 8192 × 32 B | n/a | n/a | n/a | n/a | n/a | n/a | n/a | n/a | 49.4 | n/a |

_Values in us._

## Fragmentation

Each allocator's own introspection API reports committed bytes after the workload runs. Lower committed-bytes for the same live-bytes is better.

| benchmark | live MB | glibc MB | jemalloc MB | mimalloc MB | tcmalloc MB | misra-full MB | misra-fast MB | misra-correct-full MB | misra-correct-fast MB | misra-arena MB | misra-page MB |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| Checkerboard (4 K small) | 0.6 | 2.1 | 9.1 | n/a | 7.0 | 0.8 | 0.8 | 0.8 | 0.8 | n/a | n/a |
| Checkerboard (16 K small) | 2.5 | 4.1 | 11.9 | n/a | 7.0 | 3.2 | 3.2 | 3.1 | 3.1 | n/a | n/a |
| Checkerboard (64 K small) | 10.0 | 15.3 | 21.8 | n/a | 16.0 | 12.6 | 12.6 | 12.5 | 12.5 | n/a | n/a |
| Lifetime mix | 4.0 | 5.1 | 16.7 | n/a | 16.0 | 12.6 | 12.6 | 12.5 | 12.5 | n/a | n/a |
| Page overhang | 18.2 | 20.4 | 32.3 | n/a | 29.0 | 34.3 | 34.3 | 34.2 | 34.2 | n/a | n/a |

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
| timestamp | 2026-05-29 07:31:28 UTC |
| git rev   | ac3e23856006 (r16-base) |
| host CPU  | Intel(R) Core(TM) Ultra 7 165U |
| kernel    | Linux 6.18.28 |
| compiler  | gcc 14.3.0 |
| build     | validate-full: buildtype=release optimization=3 b_lto=False b_sanitize=[] alloc_debug=True heap_validate_full=True  |  validate-fast: buildtype=release optimization=3 b_sanitize=[] b_lto=False alloc_debug=True heap_validate_full=False |
| reps      | 10 per row (median reported) |
