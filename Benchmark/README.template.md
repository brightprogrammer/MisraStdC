# MisraStdC Allocator Benchmark

> Generated automatically by `Scripts/run.py`. Do not edit by hand.

Compares MisraStdC's `HeapAllocator` and `SlabAllocator` against four production allocators: glibc, jemalloc, mimalloc, tcmalloc.

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

Same shape as the pair test but writes every byte of the allocation before freeing. Catches page-fault cost that pure alloc/free hides. Time per item, lower is better.

{{TABLE_ALLOC_TOUCH_FREE}}

### Mixed-size Pareto

512-allocation batch with sizes drawn from a Pareto(α=1.16, xm=24) distribution capped at 256 KiB. Time per batch, lower is better.

{{TABLE_MIXED_PARETO}}

### Realloc growth

Geometric realloc ladder from 8 B up to 1 MiB. Time per full ladder, lower is better.

{{TABLE_REALLOC_GROW}}

## Fragmentation

Each allocator's own introspection API reports committed bytes after the workload runs. Lower committed-bytes for the same live-bytes is better.

{{TABLE_FRAG}}

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
| timestamp | {{TIMESTAMP}} |
| git rev   | {{COMMIT}} |
| host CPU  | {{CPU}} |
| kernel    | {{KERNEL}} |
| compiler  | {{COMPILER}} |
| build     | {{BUILD_OPTIONS}} |
| reps      | {{REPS}} per row (median reported) |
