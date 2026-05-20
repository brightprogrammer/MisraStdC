# MisraStdC Allocator Benchmark

> Generated automatically by `Scripts/run.py`. Do not edit by hand.

Compares MisraStdC's `HeapAllocator` and `SlabAllocator` against four production allocators: glibc, jemalloc, mimalloc, tcmalloc.

## Headline

Single alloc/free pair, 16 B:

| backend | time |
|---|---:|
| tcmalloc           | 3.5 ns |
| glibc              | 4.5 ns |
| jemalloc           | 3.5 ns |
| mimalloc           | 7.2 ns |
| misra (Heap only)  | 21.5 ns |
| misra-correct (Slab) | 14.7 ns |

## Timing

### Single alloc/free pair

One `alloc(size)` immediately followed by `free(ptr)`, repeated. Hot reuse — the same slot churns. Time per pair, lower is better.

| benchmark | glibc | jemalloc | mimalloc | tcmalloc | misra | misra-correct |
|---|---:|---:|---:|---:|---:|---:|
| 16 B | 4.5 | 3.5 | 7.2 | 3.5 | 21.5 | 14.7 |
| 64 B | 4.4 | 3.6 | 7.3 | 3.5 | 20.5 | 15.1 |
| 256 B | 4.4 | 4.1 | 8.8 | 3.5 | 20.0 | 15.0 |
| 1 KiB | 4.5 | 5.7 | 10.6 | 3.5 | 19.4 | 14.8 |
| 4 KiB | 15.0 | 10.9 | 12.0 | 3.7 | 1594.2 | 14.8 |
| 16 KiB | 15.2 | 18.2 | 20.2 | 3.7 | 1247.0 | 1617.1 |
| 64 KiB | 15.2 | 267.2 | 20.2 | 3.8 | 1270.0 | 1613.0 |

_Values in ns._

### Batch alloc-N-then-free-N

`N` allocs of fixed size, then `N` frees, then repeat. Holds `N` allocations live at peak. Time per batch, lower is better.

| benchmark | glibc | jemalloc | mimalloc | tcmalloc | misra | misra-correct |
|---|---:|---:|---:|---:|---:|---:|
| 128 × 64 B | 1.5 | 0.5 | 0.7 | 0.5 | 2.9 | 2.0 |
| 1024 × 64 B | 13.0 | 11.3 | 5.5 | 4.0 | 31.8 | 20.0 |
| 8192 × 64 B | 104.7 | 94.8 | 52.2 | 33.3 | 521.3 | 427.7 |

_Values in us._

### Alloc + write every byte + free

Same shape as the pair test but writes every byte of the allocation before freeing. Catches page-fault cost that pure alloc/free hides. Time per item, lower is better.

| benchmark | glibc | jemalloc | mimalloc | tcmalloc | misra | misra-correct |
|---|---:|---:|---:|---:|---:|---:|
| 64 B | 4.7 | 4.6 | 8.2 | 4.4 | 30.9 | 17.4 |
| 4 KiB | 36.3 | 24.0 | 27.1 | 19.9 | 2150.3 | 47.9 |
| 64 KiB | 857.8 | 1056.6 | 863.4 | 849.6 | 13581.5 | 13862.8 |

_Values in ns._

### Mixed-size Pareto

512-allocation batch with sizes drawn from a Pareto(α=1.16, xm=24) distribution capped at 256 KiB. Time per batch, lower is better.

| benchmark | glibc | jemalloc | mimalloc | tcmalloc | misra | misra-correct |
|---|---:|---:|---:|---:|---:|---:|
| Pareto(1.16, 24) | 27.9 | 11.8 | 13.3 | 12.1 | 44.5 | 42.9 |

_Values in us._

### Realloc growth

Geometric realloc ladder from 8 B up to 1 MiB. Time per full ladder, lower is better.

| benchmark | glibc | jemalloc | mimalloc | tcmalloc | misra | misra-correct |
|---|---:|---:|---:|---:|---:|---:|
| 8 B → 1 MiB | 244.0 | 6863.0 | 16964.1 | 15276.7 | 252861.3 | 253408.3 |

_Values in ns._

## Fragmentation

Each allocator's own introspection API reports committed bytes after the workload runs. Lower committed-bytes for the same live-bytes is better.

| benchmark | live MB | glibc MB | jemalloc MB | mimalloc MB | tcmalloc MB | misra MB | misra-correct MB |
|---|---:|---:|---:|---:|---:|---:|---:|
| Checkerboard (4 K small) | 0.6 | 1.7 | 9.2 | 0.0 | 7.0 | 3.1 | 2.5 |
| Checkerboard (16 K small) | 2.5 | 4.1 | 11.9 | 0.0 | 7.0 | 10.1 | 10.0 |
| Checkerboard (64 K small) | 10.0 | 15.3 | 21.8 | 0.0 | 16.0 | 40.2 | 40.2 |
| Lifetime mix | 3.9 | 5.1 | 20.1 | 0.0 | 16.0 | 45.8 | 45.8 |
| Page overhang | 18.2 | 20.4 | 34.6 | 0.0 | 29.0 | 77.0 | 77.0 |

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
| timestamp | 2026-05-20 21:53:53 UTC |
| git rev   | b73f7477eaf8 (perf/slab-bitmap-redesign) |
| host CPU  | Intel(R) Core(TM) Ultra 7 165U |
| kernel    | Linux 6.18.25 |
| compiler  | gcc 15.2.0 |
| build     | buildtype=release optimization=3 b_lto=False b_sanitize=[] alloc_debug=True heap_validate_full=True |
| reps      | 5 per row (median reported) |
