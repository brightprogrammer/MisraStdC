# MisraStdC Allocator Benchmark

> Generated automatically by `Scripts/run.py`. Do not edit by hand.

Compares MisraStdC's `HeapAllocator` and `SlabAllocator` against four production allocators: glibc, jemalloc, mimalloc, tcmalloc.

## Headline

Single alloc/free pair, 16 B:

| backend | time |
|---|---:|
| tcmalloc           | 3.4 ns |
| glibc              | 4.6 ns |
| jemalloc           | 3.6 ns |
| mimalloc           | 7.2 ns |
| misra (Heap only)  | 17.6 ns |
| misra-correct (Slab) | 14.3 ns |

## Timing

### Single alloc/free pair

One `alloc(size)` immediately followed by `free(ptr)`, repeated. Hot reuse — the same slot churns. Time per pair, lower is better.

| benchmark | glibc | jemalloc | mimalloc | tcmalloc | misra | misra-correct | misra-arena | misra-page |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| 16 B | 4.6 | 3.6 | 7.2 | 3.4 | 17.6 | 14.3 | 11.4 | n/a |
| 64 B | 4.6 | 3.8 | 7.2 | 3.4 | 18.3 | 14.3 | 11.3 | n/a |
| 256 B | 4.7 | 4.2 | 9.1 | 3.5 | 18.3 | 14.3 | 11.4 | n/a |
| 1 KiB | 4.8 | 5.9 | 10.4 | 8.6 | 18.9 | 14.3 | 11.4 | n/a |
| 4 KiB | 16.2 | 10.8 | 11.8 | 3.6 | 74.6 | 14.3 | 11.4 | 18.6 |
| 16 KiB | 15.3 | 18.4 | 20.2 | 3.6 | 77.5 | 42.7 | 11.4 | 18.4 |
| 64 KiB | 15.4 | 265.4 | 20.1 | 3.7 | 77.3 | 42.7 | 11.1 | 18.2 |

_Values in ns._

### Batch alloc-N-then-free-N

`N` allocs of fixed size, then `N` frees, then repeat. Holds `N` allocations live at peak. Time per batch, lower is better.

| benchmark | glibc | jemalloc | mimalloc | tcmalloc | misra | misra-correct | misra-arena | misra-page |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| 128 × 64 B | 1.0 | 1.0 | 0.7 | 0.5 | 2.5 | 2.0 | n/a | n/a |
| 1024 × 64 B | 8.0 | 11.0 | 5.4 | 3.8 | 23.9 | 21.2 | n/a | n/a |
| 8192 × 64 B | 63.9 | 90.0 | 50.3 | 39.1 | 335.0 | 443.8 | n/a | n/a |

_Values in us._

### Alloc + write every byte + free

Same shape as the pair test but writes every byte of the allocation before freeing. Catches page-fault cost that pure alloc/free hides. Time per item, lower is better.

| benchmark | glibc | jemalloc | mimalloc | tcmalloc | misra | misra-correct | misra-arena | misra-page |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| 64 B | 4.8 | 4.6 | 8.1 | 4.4 | 24.3 | 16.9 | 13.9 | n/a |
| 4 KiB | 37.3 | 24.3 | 26.7 | 19.8 | 857.5 | 46.6 | 65.8 | 50.5 |
| 64 KiB | 856.2 | 1055.4 | 862.7 | 841.8 | 1110.0 | 861.9 | 1135.9 | 850.6 |

_Values in ns._

### Mixed-size Pareto

512-allocation batch with sizes drawn from a Pareto(α=1.16, xm=24) distribution capped at 256 KiB. Time per batch, lower is better.

| benchmark | glibc | jemalloc | mimalloc | tcmalloc | misra | misra-correct | misra-arena | misra-page |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| Pareto(1.16, 24) | 28.2 | 11.4 | 13.4 | 11.8 | 45.3 | 42.8 | n/a | n/a |

_Values in us._

### Realloc growth

Geometric realloc ladder from 8 B up to 1 MiB. Time per full ladder, lower is better.

| benchmark | glibc | jemalloc | mimalloc | tcmalloc | misra | misra-correct | misra-arena | misra-page |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| 8 B → 1 MiB | 3316.4 | 3757.2 | 17018.6 | 15114.2 | 19284.8 | 18675.9 | 130.3 | n/a |

_Values in ns._

### Arena bump + bulk reset

Allocate N small (32 B) objects, then release them all. Arena does this as one O(1) reset; every other backend has to free each pointer individually. Time per batch, lower is better.

| benchmark | glibc | jemalloc | mimalloc | tcmalloc | misra | misra-correct | misra-arena | misra-page |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| 128 × 32 B | 0.9 | 0.5 | 0.6 | 0.5 | 2.4 | 2.6 | 0.8 | n/a |
| 1024 × 32 B | 7.9 | 9.3 | 4.9 | 3.8 | 28.6 | 26.6 | 6.1 | n/a |
| 8192 × 32 B | 65.1 | 81.6 | 45.3 | 31.0 | 302.3 | 293.9 | 49.5 | n/a |

_Values in us._

## Fragmentation

Each allocator's own introspection API reports committed bytes after the workload runs. Lower committed-bytes for the same live-bytes is better.

| benchmark | live MB | glibc MB | jemalloc MB | mimalloc MB | tcmalloc MB | misra MB | misra-correct MB | misra-arena MB | misra-page MB |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| Checkerboard (4 K small) | 0.6 | 2.2 | 9.2 | n/a | 7.0 | 0.8 | 0.8 | n/a | n/a |
| Checkerboard (16 K small) | 2.5 | 4.1 | 11.8 | n/a | 7.0 | 3.1 | 3.0 | n/a | n/a |
| Checkerboard (64 K small) | 10.0 | 15.3 | 21.9 | n/a | 16.0 | 12.2 | 12.2 | n/a | n/a |
| Lifetime mix | 4.0 | 5.0 | 19.9 | n/a | 16.0 | 4.3 | 4.3 | n/a | n/a |
| Page overhang | 18.2 | 20.5 | 38.7 | n/a | 29.0 | 32.9 | 32.9 | n/a | n/a |

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
| timestamp | 2026-05-21 08:22:57 UTC |
| git rev   | c7d2ee76300e (master) |
| host CPU  | Intel(R) Core(TM) Ultra 7 165U |
| kernel    | Linux 6.18.25 |
| compiler  | gcc 15.2.0 |
| build     | buildtype=release optimization=3 b_lto=False b_sanitize=[] alloc_debug=True heap_validate_full=True |
| reps      | 3 per row (median reported) |
