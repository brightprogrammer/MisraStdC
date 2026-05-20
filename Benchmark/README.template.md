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

{{TLDR}}

## Timing

### Single alloc/free pair

One `alloc(size)` immediately followed by `free(ptr)`, repeated. Hot reuse — the same slot churns. Time per pair, lower is better.

{{TABLE_ALLOC_FREE_PAIR}}

### Batch alloc-N-then-free-N

`N` allocs of fixed size, then `N` frees, then repeat. Holds `N` allocations live at peak. Time per batch, lower is better.

{{TABLE_BATCH_ALLOC_FREE}}

### Alloc + write every byte + free

Like the pair test but `memset` the whole allocation before freeing. Surfaces first-fault cost on lazy-mmap backends. Time per item, lower is better.

{{TABLE_ALLOC_TOUCH_FREE}}

### Mixed-size Pareto

512-allocation batch with sizes drawn from a Pareto(α=1.16, xm=24) distribution capped at 256 KiB. Time per batch, lower is better.

{{TABLE_MIXED_PARETO}}

### Realloc growth

Geometric realloc ladder from 8 B up to 1 MiB. Time per full ladder, lower is better.

{{TABLE_REALLOC_GROW}}

## Fragmentation

Each allocator's own introspection API reports committed bytes after the workload runs (no `/proc` reads). Lower committed-bytes for the same live-bytes is better.

{{TABLE_FRAG}}

## How to read

The bench has two MisraStdC entries:

- **`misra`** — uses `HeapAllocator` for every benchmark. The general-purpose backend applied to every workload, including fixed-size ones where it's not the right tool. Closest apples-to-apples with the libc-shape allocators that also do general-purpose work.
- **`misra-correct`** — picks the allocator that fits the workload: `SlabAllocator(slot_size)` for fixed-size benches, `HeapAllocator` for mixed-size benches. What an actual MisraStdC user would do.

The gap between the two columns is the cost of wrong-allocator-for-the-job. The gap between `misra-correct` and tcmalloc on small AllocFreePair is the structural cost of MisraStdC's per-call safety checks (magic, double-free, misalignment) that the tcache-style allocators skip on the hot path.

## Environment

| | |
|---|---|
| timestamp | {{TIMESTAMP}} |
| git rev   | {{COMMIT}} |
| host CPU  | {{CPU}} |
| kernel    | {{KERNEL}} |
| compiler  | {{COMPILER}} |
| build     | {{BUILD_OPTIONS}} |
| reps      | {{REPS}} per row (median reported) |
