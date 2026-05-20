# MisraStdC Allocator Benchmark

> Generated automatically by `Scripts/run.py`. Do not edit by hand.

Compares MisraStdC's `HeapAllocator` and `SlabAllocator` against four production allocators: glibc, jemalloc, mimalloc, tcmalloc.

## Headline

Single alloc/free pair, 16 B:

| backend | time |
|---|---:|
| tcmalloc           | 3.6 ns |
| glibc              | 4.6 ns |
| jemalloc           | 3.5 ns |
| mimalloc           | 7.4 ns |
| misra (Heap only)  | 22.7 ns |
| misra-correct (Slab) | 15.3 ns |

## Timing

### Single alloc/free pair

One `alloc(size)` immediately followed by `free(ptr)`, repeated. Hot reuse — the same slot churns. Time per pair, lower is better.

| benchmark | glibc | jemalloc | mimalloc | tcmalloc | misra | misra-correct | misra-arena | misra-page |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| 16 B | 4.6 | 3.5 | 7.4 | 3.6 | 22.7 | 15.3 | 11.5 | 1610.5 |
| 64 B | 4.6 | 3.7 | 7.5 | 3.6 | 20.8 | 15.0 | 11.5 | 1614.0 |
| 256 B | 5.0 | 4.1 | 9.4 | 3.6 | 20.6 | 15.0 | 11.6 | 1608.0 |
| 1 KiB | 4.9 | 5.8 | 11.0 | 3.6 | 20.0 | 15.0 | 11.8 | 1641.1 |
| 4 KiB | 16.8 | 11.0 | 12.5 | 3.8 | 1636.8 | 15.1 | 11.8 | 1636.1 |
| 16 KiB | 16.1 | 18.9 | 20.9 | 3.8 | 1250.4 | 1608.0 | 11.9 | 1203.0 |
| 64 KiB | 16.1 | 270.8 | 21.0 | 3.9 | 1248.9 | 1621.2 | 11.9 | 1198.1 |

_Values in ns._

### Batch alloc-N-then-free-N

`N` allocs of fixed size, then `N` frees, then repeat. Holds `N` allocations live at peak. Time per batch, lower is better.

| benchmark | glibc | jemalloc | mimalloc | tcmalloc | misra | misra-correct | misra-arena | misra-page |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| 128 × 64 B | 1.0 | 1.1 | 0.7 | 0.5 | 3.0 | 2.0 | n/a | 153.9 |
| 1024 × 64 B | 8.5 | 11.7 | 6.0 | 4.1 | 32.1 | 20.5 | n/a | 1358.0 |
| 8192 × 64 B | 66.9 | 95.0 | 51.4 | 40.3 | 531.6 | 437.9 | n/a | 17750.9 |

_Values in us._

### Alloc + write every byte + free

Same shape as the pair test but writes every byte of the allocation before freeing. Catches page-fault cost that pure alloc/free hides. Time per item, lower is better.

| benchmark | glibc | jemalloc | mimalloc | tcmalloc | misra | misra-correct | misra-arena | misra-page |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| 64 B | 4.9 | 4.7 | 8.5 | 4.5 | 33.0 | 18.2 | 14.6 | 2026.2 |
| 4 KiB | 30.0 | 25.1 | 27.7 | 20.2 | 2197.8 | 48.5 | 67.8 | 2094.7 |
| 64 KiB | 856.5 | 1060.5 | 866.9 | 841.7 | 13994.6 | 14209.5 | 1136.3 | 13955.8 |

_Values in ns._

### Mixed-size Pareto

512-allocation batch with sizes drawn from a Pareto(α=1.16, xm=24) distribution capped at 256 KiB. Time per batch, lower is better.

| benchmark | glibc | jemalloc | mimalloc | tcmalloc | misra | misra-correct | misra-arena | misra-page |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| Pareto(1.16, 24) | 28.8 | 13.1 | 14.0 | 12.2 | 45.8 | 44.1 | n/a | 629.8 |

_Values in us._

### Realloc growth

Geometric realloc ladder from 8 B up to 1 MiB. Time per full ladder, lower is better.

| benchmark | glibc | jemalloc | mimalloc | tcmalloc | misra | misra-correct | misra-arena | misra-page |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| 8 B → 1 MiB | 4771.7 | 11580.5 | 16933.1 | 15135.0 | 268603.8 | 258694.1 | 139.9 | 258397.1 |

_Values in ns._

### Arena bump + bulk reset

Allocate N small (32 B) objects, then release them all. Arena does this as one O(1) reset; every other backend has to free each pointer individually. Time per batch, lower is better.

| benchmark | glibc | jemalloc | mimalloc | tcmalloc | misra | misra-correct | misra-arena | misra-page |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| 128 × 32 B | 1.0 | 0.5 | 0.7 | 0.5 | 4.2 | 3.7 | 0.8 | 154.0 |
| 1024 × 32 B | 8.4 | 9.8 | 5.3 | 4.0 | 41.9 | 39.6 | 6.5 | 1222.4 |
| 8192 × 32 B | 68.5 | 86.2 | 47.2 | 31.8 | 574.3 | 562.4 | 52.4 | 15842.2 |

_Values in us._

## Fragmentation

Each allocator's own introspection API reports committed bytes after the workload runs. Lower committed-bytes for the same live-bytes is better.

| benchmark | live MB | glibc MB | jemalloc MB | mimalloc MB | tcmalloc MB | misra MB | misra-correct MB | misra-arena MB | misra-page MB |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| Checkerboard (4 K small) | 0.6 | 1.3 | 9.1 | 0.0 | 7.0 | 3.1 | 2.5 | n/a | 16.0 |
| Checkerboard (16 K small) | 2.5 | 4.1 | 11.7 | 0.0 | 7.0 | 10.1 | 10.0 | n/a | 64.0 |
| Checkerboard (64 K small) | 10.0 | 15.3 | 21.9 | 0.0 | 16.0 | 40.2 | 40.2 | n/a | 256.0 |
| Lifetime mix | 4.0 | 5.0 | 20.1 | 0.0 | 16.0 | 45.8 | 45.8 | n/a | 64.0 |
| Page overhang | 18.2 | 20.5 | 34.0 | 0.0 | 29.0 | 77.0 | 77.0 | n/a | 256.0 |

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
| timestamp | 2026-05-20 23:09:43 UTC |
| git rev   | 4ed3d620fd4f (perf/slab-bitmap-redesign) |
| host CPU  | Intel(R) Core(TM) Ultra 7 165U |
| kernel    | Linux 6.18.25 |
| compiler  | gcc 15.2.0 |
| build     | buildtype=release optimization=3 b_lto=False b_sanitize=[] alloc_debug=True heap_validate_full=True |
| reps      | 3 per row (median reported) |
