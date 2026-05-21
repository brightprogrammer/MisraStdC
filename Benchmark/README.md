# MisraStdC Allocator Benchmark

> Generated automatically by `Scripts/run.py`. Do not edit by hand.

Compares MisraStdC's `HeapAllocator` and `SlabAllocator` against four production allocators: glibc, jemalloc, mimalloc, tcmalloc.

## Headline

Single alloc/free pair, 16 B:

| backend | time |
|---|---:|
| tcmalloc           | 3.6 ns |
| glibc              | 4.4 ns |
| jemalloc           | 3.5 ns |
| mimalloc           | 7.3 ns |
| misra (Heap only)  | 22.5 ns |
| misra-correct (Slab) | 15.0 ns |

## Timing

### Single alloc/free pair

One `alloc(size)` immediately followed by `free(ptr)`, repeated. Hot reuse — the same slot churns. Time per pair, lower is better.

| benchmark | glibc | jemalloc | mimalloc | tcmalloc | misra | misra-correct | misra-arena | misra-page |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| 16 B | 4.4 | 3.5 | 7.3 | 3.6 | 22.5 | 15.0 | 11.9 | 19.4 |
| 64 B | 4.4 | 3.6 | 7.3 | 3.6 | 20.5 | 14.9 | 11.9 | 19.3 |
| 256 B | 4.4 | 4.0 | 9.3 | 3.6 | 20.7 | 15.0 | 12.1 | 19.6 |
| 1 KiB | 4.5 | 5.7 | 9.1 | 3.5 | 19.9 | 15.0 | 12.1 | 19.6 |
| 4 KiB | 15.0 | 10.9 | 12.2 | 3.7 | 82.8 | 15.0 | 11.8 | 19.4 |
| 16 KiB | 15.0 | 19.8 | 20.4 | 3.8 | 85.9 | 47.4 | 11.8 | 19.4 |
| 64 KiB | 15.3 | 268.2 | 20.5 | 3.8 | 87.2 | 47.5 | 11.8 | 19.4 |

_Values in ns._

### Batch alloc-N-then-free-N

`N` allocs of fixed size, then `N` frees, then repeat. Holds `N` allocations live at peak. Time per batch, lower is better.

| benchmark | glibc | jemalloc | mimalloc | tcmalloc | misra | misra-correct | misra-arena | misra-page |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| 128 × 64 B | 1.0 | 0.5 | 0.7 | 0.5 | 3.0 | 2.0 | n/a | 65.8 |
| 1024 × 64 B | 8.2 | 11.1 | 5.6 | 3.9 | 32.1 | 23.9 | n/a | 3808.3 |
| 8192 × 64 B | 64.6 | 90.4 | 51.7 | 39.7 | 531.8 | 459.0 | n/a | 242193.1 |

_Values in us._

### Alloc + write every byte + free

Same shape as the pair test but writes every byte of the allocation before freeing. Catches page-fault cost that pure alloc/free hides. Time per item, lower is better.

| benchmark | glibc | jemalloc | mimalloc | tcmalloc | misra | misra-correct | misra-arena | misra-page |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| 64 B | 4.8 | 4.8 | 8.4 | 4.5 | 32.4 | 18.0 | 14.5 | 22.4 |
| 4 KiB | 36.4 | 24.2 | 27.3 | 20.3 | 1498.1 | 47.6 | 67.7 | 52.3 |
| 64 KiB | 857.0 | 1069.8 | 865.0 | 842.0 | 1132.9 | 879.3 | 1132.6 | 854.4 |

_Values in ns._

### Mixed-size Pareto

512-allocation batch with sizes drawn from a Pareto(α=1.16, xm=24) distribution capped at 256 KiB. Time per batch, lower is better.

| benchmark | glibc | jemalloc | mimalloc | tcmalloc | misra | misra-correct | misra-arena | misra-page |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| Pareto(1.16, 24) | 28.0 | 11.7 | 13.6 | 12.0 | 43.9 | 40.4 | n/a | 858.5 |

_Values in us._

### Realloc growth

Geometric realloc ladder from 8 B up to 1 MiB. Time per full ladder, lower is better.

| benchmark | glibc | jemalloc | mimalloc | tcmalloc | misra | misra-correct | misra-arena | misra-page |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| 8 B → 1 MiB | 2854.5 | 3999.8 | 16775.1 | 15227.7 | 25874.5 | 19272.4 | 136.9 | 51584.2 |

_Values in ns._

### Arena bump + bulk reset

Allocate N small (32 B) objects, then release them all. Arena does this as one O(1) reset; every other backend has to free each pointer individually. Time per batch, lower is better.

| benchmark | glibc | jemalloc | mimalloc | tcmalloc | misra | misra-correct | misra-arena | misra-page |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| 128 × 32 B | 1.0 | 0.5 | 0.7 | 0.5 | 4.2 | 3.6 | 0.8 | 252.3 |
| 1024 × 32 B | 7.9 | 9.8 | 5.0 | 3.9 | 40.6 | 37.4 | 6.6 | 3070.4 |
| 8192 × 32 B | 65.5 | 85.0 | 47.7 | 32.1 | 570.7 | 557.2 | 52.7 | 184187.3 |

_Values in us._

## Fragmentation

Each allocator's own introspection API reports committed bytes after the workload runs. Lower committed-bytes for the same live-bytes is better.

| benchmark | live MB | glibc MB | jemalloc MB | mimalloc MB | tcmalloc MB | misra MB | misra-correct MB | misra-arena MB | misra-page MB |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| Checkerboard (4 K small) | 0.6 | 2.2 | 9.2 | n/a | 7.0 | 3.1 | 2.5 | n/a | 16.0 |
| Checkerboard (16 K small) | 2.5 | 4.1 | 11.8 | n/a | 7.0 | 10.1 | 10.0 | n/a | 64.0 |
| Checkerboard (64 K small) | 10.0 | 15.3 | 21.9 | n/a | 16.0 | 40.2 | 40.2 | n/a | 256.0 |
| Lifetime mix | 3.9 | 5.0 | 20.3 | n/a | 16.0 | 45.8 | 45.8 | n/a | 64.0 |
| Page overhang | 18.2 | 20.4 | 35.5 | n/a | 29.0 | 77.0 | 77.0 | n/a | 256.0 |

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
| timestamp | 2026-05-21 03:09:31 UTC |
| git rev   | c855e0e44324 (perf/page-retain-on-free) |
| host CPU  | Intel(R) Core(TM) Ultra 7 165U |
| kernel    | Linux 6.18.25 |
| compiler  | gcc 15.2.0 |
| build     | buildtype=release optimization=3 b_lto=False b_sanitize=[] alloc_debug=True heap_validate_full=True |
| reps      | 5 per row (median reported) |
