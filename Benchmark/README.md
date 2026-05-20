# MisraStdC Allocator Benchmark

> Generated automatically by `Scripts/run.py`. Do not edit by hand — change `Benchmark/README.template.md` instead.

Compares MisraStdC's `HeapAllocator` and `SlabAllocator` against four production general-purpose allocators: glibc, jemalloc, mimalloc, tcmalloc. Each backend runs the same harness; only the malloc-shaped entry point differs.

## How to run

```sh
meson setup build -Dbenchmark=true -Dbuildtype=release -Doptimization=3
ninja  -C build
python3 Benchmark/Scripts/run.py build
```

Refreshes this README from `Benchmark/README.template.md` with measurements taken on the host running the script.

## Headline

Single alloc/free pair, 16 B:

| backend | time |
|---|---:|
| tcmalloc           | 3.6 ns |
| glibc              | 5.0 ns |
| jemalloc           | 3.7 ns |
| mimalloc           | 7.3 ns |
| misra (Heap only)  | 22.3 ns |
| misra-correct (Slab) | 14.9 ns |

## Timing

### Single alloc/free pair

One `alloc(size)` immediately followed by `free(ptr)`, repeated. Hot reuse — the same slot churns. Time per pair, lower is better.

| benchmark | glibc | jemalloc | mimalloc | tcmalloc | misra | misra-correct |
|---|---:|---:|---:|---:|---:|---:|
| 16 B | 5.0 | 3.7 | 7.3 | 3.6 | 22.3 | 14.9 |
| 64 B | 5.0 | 3.8 | 7.3 | 3.5 | 20.8 | 15.0 |
| 256 B | 5.0 | 4.2 | 8.9 | 3.5 | 20.3 | 15.0 |
| 1 KiB | 5.1 | 5.8 | 10.4 | 3.5 | 20.2 | 15.0 |
| 4 KiB | 17.2 | 11.0 | 12.0 | 3.7 | 1639.4 | 15.0 |
| 16 KiB | 18.4 | 18.3 | 20.6 | 3.7 | 1249.9 | 1615.0 |
| 64 KiB | 17.2 | 269.4 | 20.6 | 3.7 | 1244.5 | 1623.4 |

_Values in ns._

### Batch alloc-N-then-free-N

`N` allocs of fixed size, then `N` frees, then repeat. Holds `N` allocations live at peak. Time per batch, lower is better.

| benchmark | glibc | jemalloc | mimalloc | tcmalloc | misra | misra-correct |
|---|---:|---:|---:|---:|---:|---:|
| 128 × 64 B | 1.1 | 0.5 | 0.7 | 0.5 | 3.0 | 2.0 |
| 1024 × 64 B | 9.3 | 11.5 | 5.5 | 4.0 | 32.4 | 20.6 |
| 8192 × 64 B | 67.4 | 96.4 | 51.1 | 33.0 | 523.2 | 434.2 |

_Values in us._

### Alloc + write every byte + free

Like the pair test but `memset` the whole allocation before freeing. Surfaces first-fault cost on lazy-mmap backends. Time per item, lower is better.

| benchmark | glibc | jemalloc | mimalloc | tcmalloc | misra | misra-correct |
|---|---:|---:|---:|---:|---:|---:|
| 64 B | 5.0 | 4.9 | 8.3 | 4.4 | 31.1 | 17.8 |
| 4 KiB | 27.3 | 24.3 | 27.0 | 19.9 | 2237.0 | 48.1 |
| 64 KiB | 846.7 | 1067.6 | 864.1 | 842.7 | 13858.3 | 14299.6 |

_Values in ns._

### Mixed-size Pareto

512-allocation batch with sizes drawn from a Pareto(α=1.16, xm=24) distribution capped at 256 KiB. Time per batch, lower is better.

| benchmark | glibc | jemalloc | mimalloc | tcmalloc | misra | misra-correct |
|---|---:|---:|---:|---:|---:|---:|
| Pareto(1.16, 24) | 28.3 | 11.9 | 13.6 | 11.8 | 44.0 | 43.7 |

_Values in us._

### Realloc growth

Geometric realloc ladder from 8 B up to 1 MiB. Time per full ladder, lower is better.

| benchmark | glibc | jemalloc | mimalloc | tcmalloc | misra | misra-correct |
|---|---:|---:|---:|---:|---:|---:|
| 8 B → 1 MiB | 244.7 | 3852.9 | 16937.1 | 15254.2 | 255749.4 | 254675.3 |

_Values in ns._

## Fragmentation

Each allocator's own introspection API reports committed bytes after the workload runs (no `/proc` reads). Lower committed-bytes for the same live-bytes is better.

| benchmark | live MB | glibc MB | jemalloc MB | mimalloc MB | tcmalloc MB | misra MB | misra-correct MB |
|---|---:|---:|---:|---:|---:|---:|---:|
| Checkerboard (4 K small) | 0.6 | 1.7 | 9.2 | 0.0 | 7.0 | 3.1 | 2.5 |
| Checkerboard (16 K small) | 2.5 | 4.1 | 11.7 | 0.0 | 7.0 | 10.1 | 10.0 |
| Checkerboard (64 K small) | 10.0 | 15.3 | 21.9 | 0.0 | 16.0 | 40.2 | 40.2 |
| Lifetime mix | 4.0 | 5.1 | 16.8 | 0.0 | 16.0 | 45.8 | 45.8 |
| Page overhang | 18.2 | 20.5 | 32.3 | 0.0 | 29.0 | 77.0 | 77.0 |

## How to read

The bench has two MisraStdC entries:

- **`misra`** — uses `HeapAllocator` for every benchmark. The general-purpose backend applied to every workload, including fixed-size ones where it's not the right tool. Closest apples-to-apples with the libc-shape allocators that also do general-purpose work.
- **`misra-correct`** — picks the allocator that fits the workload: `SlabAllocator(slot_size)` for fixed-size benches, `HeapAllocator` for mixed-size benches. What an actual MisraStdC user would do.

The gap between the two columns is the cost of wrong-allocator-for-the-job. The gap between `misra-correct` and tcmalloc on small AllocFreePair is the structural cost of MisraStdC's per-call safety checks (magic, double-free, misalignment) that the tcache-style allocators skip on the hot path.

## Environment

| | |
|---|---|
| timestamp | 2026-05-20 21:38:23 UTC |
| git rev   | f5c400c5ff24 (perf/slab-bitmap-redesign) |
| host CPU  | Intel(R) Core(TM) Ultra 7 165U |
| kernel    | Linux 6.18.25 |
| compiler  | gcc 15.2.0 |
| build     | buildtype=release optimization=3 b_lto=False b_sanitize=[] |
| reps      | 5 per row (median reported) |
