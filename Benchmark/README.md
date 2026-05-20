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
| mimalloc           | 7.4 ns |
| misra (Heap only)  | 22.7 ns |
| misra-correct (Slab) | 15.0 ns |

## Timing

### Single alloc/free pair

One `alloc(size)` immediately followed by `free(ptr)`, repeated. Hot reuse — the same slot churns. Time per pair, lower is better.

| benchmark | glibc | jemalloc | mimalloc | tcmalloc | misra | misra-correct | misra-arena | misra-page |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| 16 B | 4.9 | 3.6 | 7.4 | 3.6 | 22.7 | 15.0 | 11.7 | 1620.7 |
| 64 B | 5.1 | 3.7 | 7.5 | 3.6 | 21.3 | 14.9 | 11.6 | 1600.6 |
| 256 B | 5.2 | 4.1 | 9.5 | 3.5 | 21.2 | 15.0 | 11.6 | 1625.0 |
| 1 KiB | 5.1 | 5.8 | 11.0 | 3.5 | 20.7 | 15.0 | 11.6 | 1629.9 |
| 4 KiB | 17.4 | 11.0 | 12.4 | 3.8 | 1641.7 | 15.0 | 11.9 | 1611.2 |
| 16 KiB | 18.0 | 18.6 | 20.8 | 3.8 | 1233.8 | 1635.3 | 11.8 | 1214.2 |
| 64 KiB | 17.5 | 270.5 | 20.5 | 3.8 | 1243.2 | 1659.1 | 11.5 | 1232.6 |

_Values in ns._

### Batch alloc-N-then-free-N

`N` allocs of fixed size, then `N` frees, then repeat. Holds `N` allocations live at peak. Time per batch, lower is better.

| benchmark | glibc | jemalloc | mimalloc | tcmalloc | misra | misra-correct | misra-arena | misra-page |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| 128 × 64 B | 1.2 | 1.0 | 0.7 | 0.5 | 3.0 | 2.0 | n/a | 154.2 |
| 1024 × 64 B | 9.0 | 11.6 | 5.5 | 4.0 | 31.9 | 20.5 | n/a | 1351.1 |
| 8192 × 64 B | 81.3 | 94.2 | 51.2 | 39.9 | 537.7 | 437.0 | n/a | 17811.5 |

_Values in us._

### Alloc + write every byte + free

Same shape as the pair test but writes every byte of the allocation before freeing. Catches page-fault cost that pure alloc/free hides. Time per item, lower is better.

| benchmark | glibc | jemalloc | mimalloc | tcmalloc | misra | misra-correct | misra-arena | misra-page |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| 64 B | 8.2 | 4.8 | 8.5 | 4.4 | 32.2 | 17.7 | 14.7 | 2006.2 |
| 4 KiB | 30.0 | 25.1 | 27.6 | 20.2 | 2177.9 | 48.3 | 67.7 | 2072.5 |
| 64 KiB | 858.1 | 1067.3 | 862.4 | 841.2 | 13995.5 | 14096.2 | 1148.8 | 13946.5 |

_Values in ns._

### Mixed-size Pareto

512-allocation batch with sizes drawn from a Pareto(α=1.16, xm=24) distribution capped at 256 KiB. Time per batch, lower is better.

| benchmark | glibc | jemalloc | mimalloc | tcmalloc | misra | misra-correct | misra-arena | misra-page |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| Pareto(1.16, 24) | 29.0 | 11.9 | 14.0 | 13.0 | 45.2 | 44.8 | n/a | 623.8 |

_Values in us._

### Realloc growth

Geometric realloc ladder from 8 B up to 1 MiB. Time per full ladder, lower is better.

| benchmark | glibc | jemalloc | mimalloc | tcmalloc | misra | misra-correct | misra-arena | misra-page |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| 8 B → 1 MiB | 5735.8 | 4313.3 | 17014.8 | 15119.2 | 261663.3 | 260083.2 | 134.8 | 260357.2 |

_Values in ns._

### Arena bump + bulk reset

Allocate N small (32 B) objects, then release them all. Arena does this as one O(1) reset; every other backend has to free each pointer individually. Time per batch, lower is better.

| benchmark | glibc | jemalloc | mimalloc | tcmalloc | misra | misra-correct | misra-arena | misra-page |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| 128 × 32 B | 1.0 | 0.5 | 0.7 | 0.5 | 4.0 | 3.7 | 0.8 | 152.0 |
| 1024 × 32 B | 8.0 | 9.6 | 5.1 | 3.9 | 40.8 | 39.0 | 6.4 | 1217.0 |
| 8192 × 32 B | 67.0 | 85.4 | 47.7 | 31.8 | 569.5 | 564.7 | 52.6 | 15765.0 |

_Values in us._

## Fragmentation

Each allocator's own introspection API reports committed bytes after the workload runs. Lower committed-bytes for the same live-bytes is better.

| benchmark | live MB | glibc MB | jemalloc MB | mimalloc MB | tcmalloc MB | misra MB | misra-correct MB | misra-arena MB | misra-page MB |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| Checkerboard (4 K small) | 0.6 | 2.2 | 9.2 | n/a | 7.0 | 3.1 | 2.5 | n/a | 16.0 |
| Checkerboard (16 K small) | 2.5 | 4.1 | 11.7 | n/a | 7.0 | 10.1 | 10.0 | n/a | 64.0 |
| Checkerboard (64 K small) | 10.0 | 15.3 | 22.0 | n/a | 16.0 | 40.2 | 40.2 | n/a | 256.0 |
| Lifetime mix | 4.0 | 5.0 | 20.4 | n/a | 16.0 | 45.8 | 45.8 | n/a | 64.0 |
| Page overhang | 18.2 | 20.4 | 36.4 | n/a | 29.0 | 77.0 | 77.0 | n/a | 256.0 |

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
| timestamp | 2026-05-20 23:26:13 UTC |
| git rev   | 85f73f49377d (perf/slab-bitmap-redesign) |
| host CPU  | Intel(R) Core(TM) Ultra 7 165U |
| kernel    | Linux 6.18.25 |
| compiler  | gcc 15.2.0 |
| build     | buildtype=release optimization=3 b_lto=False b_sanitize=[] alloc_debug=True heap_validate_full=True |
| reps      | 3 per row (median reported) |
