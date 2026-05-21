# MisraStdC Allocator Benchmark

> Generated automatically by `Scripts/run.py`. Do not edit by hand.

Compares MisraStdC's `HeapAllocator` and `SlabAllocator` against four production allocators: glibc, jemalloc, mimalloc, tcmalloc.

## Headline

Single alloc/free pair, 16 B:

| backend | time |
|---|---:|
| tcmalloc           | 3.6 ns |
| glibc              | 4.9 ns |
| jemalloc           | 3.6 ns |
| mimalloc           | 7.3 ns |
| misra (Heap only)  | 18.7 ns |
| misra-correct (Slab) | 15.3 ns |

## Timing

### Single alloc/free pair

One `alloc(size)` immediately followed by `free(ptr)`, repeated. Hot reuse — the same slot churns. Time per pair, lower is better.

| benchmark | glibc | jemalloc | mimalloc | tcmalloc | misra | misra-correct | misra-arena | misra-page |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| 16 B | 4.9 | 3.6 | 7.3 | 3.6 | 18.7 | 15.3 | 11.5 | 19.1 |
| 64 B | 4.7 | 3.7 | 7.4 | 3.5 | 19.2 | 15.5 | 11.5 | 18.9 |
| 256 B | 4.6 | 4.1 | 9.2 | 3.5 | 19.2 | 14.9 | 11.6 | 18.9 |
| 1 KiB | 4.6 | 5.8 | 10.9 | 3.5 | 19.9 | 14.9 | 11.6 | 18.8 |
| 4 KiB | 15.8 | 11.0 | 12.2 | 3.7 | 77.6 | 14.9 | 11.5 | 18.7 |
| 16 KiB | 16.0 | 18.7 | 20.6 | 3.7 | 80.9 | 44.1 | 11.5 | 18.9 |
| 64 KiB | 16.1 | 270.8 | 20.5 | 3.8 | 82.6 | 44.8 | 11.7 | 18.8 |

_Values in ns._

### Batch alloc-N-then-free-N

`N` allocs of fixed size, then `N` frees, then repeat. Holds `N` allocations live at peak. Time per batch, lower is better.

| benchmark | glibc | jemalloc | mimalloc | tcmalloc | misra | misra-correct | misra-arena | misra-page |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| 128 × 64 B | 1.0 | 1.0 | 0.7 | 0.5 | 2.7 | 2.1 | n/a | 66.3 |
| 1024 × 64 B | 8.3 | 11.4 | 5.7 | 4.0 | 25.2 | 22.6 | n/a | 3800.4 |
| 8192 × 64 B | 67.3 | 92.4 | 52.0 | 39.9 | 346.1 | 464.3 | n/a | 241868.1 |

_Values in us._

### Alloc + write every byte + free

Same shape as the pair test but writes every byte of the allocation before freeing. Catches page-fault cost that pure alloc/free hides. Time per item, lower is better.

| benchmark | glibc | jemalloc | mimalloc | tcmalloc | misra | misra-correct | misra-arena | misra-page |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| 64 B | 5.0 | 4.7 | 8.5 | 4.5 | 24.7 | 18.0 | 14.1 | 21.2 |
| 4 KiB | 27.7 | 23.1 | 27.6 | 20.3 | 894.7 | 48.1 | 67.2 | 52.5 |
| 64 KiB | 860.8 | 1068.6 | 863.6 | 841.9 | 1124.8 | 880.1 | 1131.1 | 858.9 |

_Values in ns._

### Mixed-size Pareto

512-allocation batch with sizes drawn from a Pareto(α=1.16, xm=24) distribution capped at 256 KiB. Time per batch, lower is better.

| benchmark | glibc | jemalloc | mimalloc | tcmalloc | misra | misra-correct | misra-arena | misra-page |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| Pareto(1.16, 24) | 28.8 | 11.8 | 13.6 | 12.1 | 46.8 | 44.5 | n/a | 858.8 |

_Values in us._

### Realloc growth

Geometric realloc ladder from 8 B up to 1 MiB. Time per full ladder, lower is better.

| benchmark | glibc | jemalloc | mimalloc | tcmalloc | misra | misra-correct | misra-arena | misra-page |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| 8 B → 1 MiB | 5248.8 | 4279.4 | 17154.3 | 15153.7 | 19803.7 | 19334.9 | 138.9 | 52918.8 |

_Values in ns._

### Arena bump + bulk reset

Allocate N small (32 B) objects, then release them all. Arena does this as one O(1) reset; every other backend has to free each pointer individually. Time per batch, lower is better.

| benchmark | glibc | jemalloc | mimalloc | tcmalloc | misra | misra-correct | misra-arena | misra-page |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| 128 × 32 B | 1.0 | 0.5 | 0.7 | 0.5 | 3.1 | 2.6 | 0.8 | 257.1 |
| 1024 × 32 B | 8.5 | 9.8 | 5.0 | 3.9 | 31.0 | 27.6 | 6.4 | 3087.2 |
| 8192 × 32 B | 72.7 | 84.7 | 47.0 | 32.3 | 317.0 | 308.3 | 51.2 | 168137.7 |

_Values in us._

## Fragmentation

Each allocator's own introspection API reports committed bytes after the workload runs. Lower committed-bytes for the same live-bytes is better.

| benchmark | live MB | glibc MB | jemalloc MB | mimalloc MB | tcmalloc MB | misra MB | misra-correct MB | misra-arena MB | misra-page MB |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| Checkerboard (4 K small) | 0.6 | 2.4 | 9.1 | n/a | 7.0 | 0.8 | 0.8 | n/a | 16.0 |
| Checkerboard (16 K small) | 2.5 | 4.1 | 11.9 | n/a | 7.0 | 3.1 | 3.0 | n/a | 64.0 |
| Checkerboard (64 K small) | 10.0 | 15.3 | 21.9 | n/a | 16.0 | 12.2 | 12.2 | n/a | 256.0 |
| Lifetime mix | 4.0 | 5.0 | 20.4 | n/a | 16.0 | 4.3 | 4.2 | n/a | 64.0 |
| Page overhang | 18.2 | 20.5 | 36.0 | n/a | 29.0 | 32.9 | 32.9 | n/a | 256.0 |

## How to read

Two MisraStdC entries:

- **`misra`** — uses `HeapAllocator` for every benchmark. Closest apples-to-apples with the libc-shape allocators.
- **`misra-correct`** — picks the allocator that fits the workload: `SlabAllocator(slot_size)` for fixed-size benches, `HeapAllocator` for mixed-size. What a real MisraStdC user would do.

The gap between the two columns is the cost of wrong-allocator-for-the-job. Where tcmalloc beats `misra-correct` on small AllocFreePair, the gap is MisraStdC's per-call safety checks (double-free, misalignment) that other allocators skip.

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
| timestamp | 2026-05-21 06:17:54 UTC |
| git rev   | a1876289691d (perf/heap-finer-bins) |
| host CPU  | Intel(R) Core(TM) Ultra 7 165U |
| kernel    | Linux 6.18.25 |
| compiler  | gcc 15.2.0 |
| build     | buildtype=release optimization=3 b_lto=False b_sanitize=[] alloc_debug=True heap_validate_full=True |
| reps      | 3 per row (median reported) |
