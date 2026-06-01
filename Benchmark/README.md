# MisraStdC Allocator Benchmark

> Generated automatically by `Scripts/run.py`. Do not edit by hand.

Compares MisraStdC's `HeapAllocator` and `SlabAllocator` against four production allocators: glibc, jemalloc, mimalloc, tcmalloc.

## Headline

Single alloc/free pair, 16 B:

| backend | time |
|---|---:|
| tcmalloc | 3.4 ns |
| glibc    | 7.3 ns |
| jemalloc | 3.5 ns |
| mimalloc | 3.0 ns |
| misra (Heap) | 14.8 ns |
| misra-correct (Slab) | 14.0 ns |

## Timing

### Single alloc/free pair

One `alloc(size)` immediately followed by `free(ptr)`, repeated. Hot reuse — the same slot churns. Time per pair, lower is better.

| benchmark | glibc | jemalloc | mimalloc | tcmalloc | misra | misra-correct | misra-arena | misra-page |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| 16 B | 7.3 | 3.5 | 3.0 | 3.4 | 14.8 | 14.0 | 12.0 | n/a |
| 64 B | 7.2 | 3.6 | 3.2 | 3.5 | 15.0 | 14.0 | 12.2 | n/a |
| 256 B | 7.2 | 3.9 | 13.7 | 3.4 | 15.0 | 13.9 | 12.4 | n/a |
| 1 KiB | 19.3 | 5.5 | 15.1 | 3.4 | 15.0 | 13.9 | 12.2 | n/a |
| 4 KiB | 15.1 | 10.4 | 10.6 | 3.6 | 19.1 | 13.9 | 12.2 | 19.8 |
| 16 KiB | 16.2 | 18.8 | 19.6 | 3.6 | 19.2 | 19.3 | 12.3 | 19.7 |
| 64 KiB | 17.2 | 267.0 | 19.6 | 3.6 | 19.2 | 19.2 | 12.3 | 20.0 |

_Values in ns._

### Batch alloc-N-then-free-N

`N` allocs of fixed size, then `N` frees, then repeat. Holds `N` allocations live at peak. Time per batch, lower is better.

| benchmark | glibc | jemalloc | mimalloc | tcmalloc | misra | misra-correct | misra-arena | misra-page |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| 128 × 64 B | 1.4 | 0.5 | 0.4 | 0.5 | 1.6 | 1.9 | n/a | n/a |
| 1024 × 64 B | 10.9 | 10.8 | 3.6 | 3.9 | 12.4 | 18.9 | n/a | n/a |
| 8192 × 64 B | 87.3 | 88.7 | 33.2 | 38.9 | 99.6 | 399.5 | n/a | n/a |

_Values in us._

### Alloc + write every byte + free

Same shape as the pair test but writes every byte of the allocation before freeing. Catches page-fault cost that pure alloc/free hides. Time per item, lower is better.

| benchmark | glibc | jemalloc | mimalloc | tcmalloc | misra | misra-correct | misra-arena | misra-page |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| 64 B | 7.8 | 4.4 | 3.2 | 4.4 | 15.0 | 16.6 | 14.7 | n/a |
| 4 KiB | 32.1 | 23.3 | 24.0 | 18.9 | 38.1 | 33.6 | 55.3 | 38.7 |
| 64 KiB | 955.9 | 1061.8 | 863.4 | 840.3 | 852.3 | 850.4 | 916.3 | 851.1 |

_Values in ns._

### Mixed-size Pareto

512-allocation batch with sizes drawn from a Pareto(α=1.16, xm=24) distribution capped at 256 KiB. Time per batch, lower is better.

| benchmark | glibc | jemalloc | mimalloc | tcmalloc | misra | misra-correct | misra-arena | misra-page |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| Pareto(1.16, 24) | 30.5 | 11.0 | 10.8 | 11.4 | 24.9 | 25.3 | n/a | n/a |

_Values in us._

### Realloc growth

Geometric realloc ladder from 8 B up to 1 MiB. Time per full ladder, lower is better.

| benchmark | glibc | jemalloc | mimalloc | tcmalloc | misra | misra-correct | misra-arena | misra-page |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| 8 B → 1 MiB | 2851.4 | 3883.1 | 18015.4 | 15131.8 | 16677.8 | 15854.8 | 135.0 | n/a |

_Values in ns._

### Arena bump + bulk reset

Allocate N small (32 B) objects, then release them all. Arena does this as one O(1) reset; every other backend has to free each pointer individually. Time per batch, lower is better.

| benchmark | glibc | jemalloc | mimalloc | tcmalloc | misra | misra-correct | misra-arena | misra-page |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| 128 × 32 B | n/a | n/a | n/a | n/a | n/a | n/a | 0.8 | n/a |
| 1024 × 32 B | n/a | n/a | n/a | n/a | n/a | n/a | 6.5 | n/a |
| 8192 × 32 B | n/a | n/a | n/a | n/a | n/a | n/a | 52.4 | n/a |

_Values in us._

## Fragmentation

Each allocator's own introspection API reports committed bytes after the workload runs. Lower committed-bytes for the same live-bytes is better.

| benchmark | live MB | glibc MB | jemalloc MB | mimalloc MB | tcmalloc MB | misra MB | misra-correct MB | misra-arena MB | misra-page MB |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| Checkerboard (4 K small) | 0.6 | 2.2 | 9.1 | n/a | 7.0 | 0.8 | 0.8 | n/a | n/a |
| Checkerboard (16 K small) | 2.5 | 4.1 | 11.7 | n/a | 7.0 | 3.2 | 3.1 | n/a | n/a |
| Checkerboard (64 K small) | 10.0 | 15.3 | 22.0 | n/a | 16.0 | 12.5 | 12.5 | n/a | n/a |
| Lifetime mix | 4.0 | 5.0 | 20.1 | n/a | 16.0 | 4.6 | 4.7 | n/a | n/a |
| Page overhang | 18.2 | 20.5 | 32.8 | n/a | 29.0 | 38.4 | 38.4 | n/a | n/a |

## How to read

MisraStdC's typed-allocator family shows up as several columns:

- **`misra` / `misra-correct`** — `HeapAllocator` everywhere vs the right tool per workload (`SlabAllocator(slot_size)` for fixed-size benches, `HeapAllocator` for mixed). The latter is what a real MisraStdC caller would do.
- **`misra-arena` / `misra-page`** — specialised backends (bump+bulk-reset, mmap-per-alloc). `n/a` rows mean the workload doesn't fit the backend's contract (e.g. arena can't handle many-live-allocs; page wastes whole pages on sub-page requests).

Where tcmalloc beats `misra-correct` on small AllocFreePair, the gap is MisraStdC's structural safety (double-free, misalignment, magic-tag dispatch, cross-class invariants) that production allocators skip even in their fast paths.

## Reproduce

```sh
meson setup build -Dbenchmark=true -Dbuildtype=release -Doptimization=3
ninja  -C build
python3 Benchmark/Scripts/run.py build
```

Regenerates this file with measurements from the host.

## Environment

| | |
|---|---|
| timestamp | 2026-05-29 12:59:55 UTC |
| git rev   | e837a85ac468 (feat/heap-class-shrink-xl-retention) |
| host CPU  | Intel(R) Core(TM) Ultra 7 165U |
| kernel    | Linux 6.18.28 |
| compiler  | gcc 14.3.0 |
| build     | buildtype=release optimization=3 b_lto=False b_sanitize=[] alloc_debug=True |
| reps      | 10 per row (median reported) |
