# MisraStdC Allocator Benchmark

> Generated automatically by `Scripts/run.py`. Do not edit by hand.

Compares MisraStdC's `HeapAllocator` and `SlabAllocator` against four production allocators: glibc, jemalloc, mimalloc, tcmalloc.

## Headline

Single alloc/free pair, 16 B:

| backend | time |
|---|---:|
| tcmalloc           | 3.5 ns |
| glibc              | 5.1 ns |
| jemalloc           | 3.6 ns |
| mimalloc           | 7.2 ns |
| misra (Heap only)  | 22.2 ns |
| misra-correct (Slab) | 15.2 ns |

## Timing

### Single alloc/free pair

One `alloc(size)` immediately followed by `free(ptr)`, repeated. Hot reuse — the same slot churns. Time per pair, lower is better.

| benchmark | glibc | jemalloc | mimalloc | tcmalloc | misra | misra-correct | misra-arena | misra-page |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| 16 B | 5.1 | 3.6 | 7.2 | 3.5 | 22.2 | 15.2 | 12.0 | 19.4 |
| 64 B | 5.1 | 3.7 | 7.3 | 3.5 | 21.2 | 15.1 | 11.8 | 19.1 |
| 256 B | 5.1 | 4.0 | 9.3 | 3.6 | 20.4 | 15.2 | 12.1 | 19.3 |
| 1 KiB | 5.2 | 5.8 | 8.9 | 3.5 | 19.9 | 15.2 | 12.0 | 19.4 |
| 4 KiB | 18.1 | 10.6 | 12.1 | 3.8 | 82.7 | 15.3 | 11.9 | 19.4 |
| 16 KiB | 17.7 | 20.0 | 20.4 | 3.8 | 85.4 | 48.2 | 12.0 | 19.3 |
| 64 KiB | 17.8 | 271.1 | 20.5 | 3.8 | 85.4 | 48.0 | 12.0 | 19.5 |

_Values in ns._

### Batch alloc-N-then-free-N

`N` allocs of fixed size, then `N` frees, then repeat. Holds `N` allocations live at peak. Time per batch, lower is better.

| benchmark | glibc | jemalloc | mimalloc | tcmalloc | misra | misra-correct | misra-arena | misra-page |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| 128 × 64 B | 1.1 | 1.0 | 0.7 | 0.5 | 3.3 | 2.1 | n/a | 66.3 |
| 1024 × 64 B | 8.6 | 11.1 | 5.6 | 4.0 | 35.0 | 23.0 | n/a | 3782.5 |
| 8192 × 64 B | 67.4 | 91.2 | 51.7 | 40.1 | 678.1 | 465.3 | n/a | 240310.9 |

_Values in us._

### Alloc + write every byte + free

Same shape as the pair test but writes every byte of the allocation before freeing. Catches page-fault cost that pure alloc/free hides. Time per item, lower is better.

| benchmark | glibc | jemalloc | mimalloc | tcmalloc | misra | misra-correct | misra-arena | misra-page |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| 64 B | 4.9 | 4.5 | 8.4 | 4.4 | 24.6 | 18.0 | 14.4 | 21.9 |
| 4 KiB | 41.6 | 24.2 | 27.5 | 19.9 | 1341.6 | 47.9 | 67.4 | 51.8 |
| 64 KiB | 857.6 | 1063.3 | 863.4 | 842.2 | 1133.0 | 874.0 | 1133.9 | 857.2 |

_Values in ns._

### Mixed-size Pareto

512-allocation batch with sizes drawn from a Pareto(α=1.16, xm=24) distribution capped at 256 KiB. Time per batch, lower is better.

| benchmark | glibc | jemalloc | mimalloc | tcmalloc | misra | misra-correct | misra-arena | misra-page |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| Pareto(1.16, 24) | 29.0 | 12.5 | 14.1 | 12.0 | 69.2 | 51.5 | n/a | 855.9 |

_Values in us._

### Realloc growth

Geometric realloc ladder from 8 B up to 1 MiB. Time per full ladder, lower is better.

| benchmark | glibc | jemalloc | mimalloc | tcmalloc | misra | misra-correct | misra-arena | misra-page |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| 8 B → 1 MiB | 6588.3 | 4036.7 | 16776.8 | 15284.3 | 20289.2 | 19342.5 | 136.5 | 51900.4 |

_Values in ns._

### Arena bump + bulk reset

Allocate N small (32 B) objects, then release them all. Arena does this as one O(1) reset; every other backend has to free each pointer individually. Time per batch, lower is better.

| benchmark | glibc | jemalloc | mimalloc | tcmalloc | misra | misra-correct | misra-arena | misra-page |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| 128 × 32 B | 1.0 | 0.5 | 0.7 | 0.5 | 7.2 | 4.7 | 0.8 | 255.1 |
| 1024 × 32 B | 8.2 | 9.3 | 5.1 | 3.9 | 71.4 | 46.5 | 6.4 | 3059.2 |
| 8192 × 32 B | 70.5 | 83.4 | 47.4 | 31.8 | 756.3 | 750.4 | 51.7 | 182694.7 |

_Values in us._

## Fragmentation

Each allocator's own introspection API reports committed bytes after the workload runs. Lower committed-bytes for the same live-bytes is better.

| benchmark | live MB | glibc MB | jemalloc MB | mimalloc MB | tcmalloc MB | misra MB | misra-correct MB | misra-arena MB | misra-page MB |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| Checkerboard (4 K small) | 0.6 | 2.1 | 9.2 | n/a | 7.0 | 2.5 | 2.5 | n/a | 16.0 |
| Checkerboard (16 K small) | 2.5 | 4.1 | 11.8 | n/a | 7.0 | 10.1 | 10.0 | n/a | 64.0 |
| Checkerboard (64 K small) | 10.0 | 15.3 | 22.0 | n/a | 16.0 | 40.2 | 40.2 | n/a | 256.0 |
| Lifetime mix | 4.0 | 5.0 | 20.4 | n/a | 16.0 | 8.6 | 8.6 | n/a | 64.0 |
| Page overhang | 18.2 | 20.4 | 34.8 | n/a | 29.0 | 47.3 | 47.3 | n/a | 256.0 |

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
| timestamp | 2026-05-21 04:56:08 UTC |
| git rev   | 47d93e7e63b9 (perf/heap-reclaim-empty-pages) |
| host CPU  | Intel(R) Core(TM) Ultra 7 165U |
| kernel    | Linux 6.18.25 |
| compiler  | gcc 15.2.0 |
| build     | buildtype=release optimization=3 b_lto=False b_sanitize=[] alloc_debug=True heap_validate_full=True |
| reps      | 5 per row (median reported) |
