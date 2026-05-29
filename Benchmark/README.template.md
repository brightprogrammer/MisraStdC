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

### Arena bump + bulk reset

Allocate N small (32 B) objects, then release them all. Arena does this as one O(1) reset; every other backend has to free each pointer individually. Time per batch, lower is better.

{{TABLE_ARENA_BUMP_RESET}}

## Fragmentation

Each allocator's own introspection API reports committed bytes after the workload runs. Lower committed-bytes for the same live-bytes is better.

{{TABLE_FRAG}}

## How to read

MisraStdC's typed-allocator family shows up as several columns:

- **`misra` / `misra-correct`** — `HeapAllocator` everywhere vs the right tool per workload (`SlabAllocator(slot_size)` for fixed-size benches, `HeapAllocator` for mixed). The latter is what a real MisraStdC caller would do.
- **`*-full` vs `*-fast`** — same source, different `heap_validate_full` setting. `-full` does per-dispatch cross-class checks plus volatile descriptor probes (project default; catches torn pointers / freed metadata). `-fast` is the magic-only check (catches uninitialised / post-deinit / type-confusion only). The gap between them is MisraStdC's per-call safety tax — turn it off in shipping release builds for tighter dispatch.
- **`misra-arena` / `misra-page`** — specialised backends (bump+bulk-reset, mmap-per-alloc). `n/a` rows mean the workload doesn't fit the backend's contract (e.g. arena can't handle many-live-allocs; page wastes whole pages on sub-page requests).

Where tcmalloc beats `misra-correct-fast` on small AllocFreePair, the gap is MisraStdC's structural safety (double-free, misalignment, magic-tag dispatch) that production allocators skip even in their fast paths.

## Reproduce

Single-tier run (validate-full only):

```sh
meson setup build -Dbenchmark=true -Dbuildtype=release -Doptimization=3
ninja  -C build
python3 Benchmark/Scripts/run.py build
```

Two-tier run that shows the safety overhead side-by-side (matches the numbers above):

```sh
meson setup build-full -Dbenchmark=true -Dbuildtype=release -Doptimization=3 \
    -Dalloc_debug=false -Dheap_validate_full=true
meson setup build-fast -Dbenchmark=true -Dbuildtype=release -Doptimization=3 \
    -Dalloc_debug=false -Dheap_validate_full=false
ninja -C build-full
ninja -C build-fast
python3 Benchmark/Scripts/run.py build-full --validate-fast build-fast
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
