// Uniform allocator interface used by the benchmark harness.
//
// One backend per binary, selected by -DBENCH_BACKEND_<NAME> at the meson
// layer. The benchmark code never sees backend-specific types; it calls
// bench_alloc / bench_free / bench_realloc and queries bench_live_bytes /
// bench_peak_bytes for fragmentation accounting.
//
// Why one backend per binary, not one binary with all backends:
//   * jemalloc / mimalloc / tcmalloc each interpose `malloc` / `free` at
//     link time. Linking all three into one process is undefined.
//   * MisraStdC's HeapAllocator does NOT interpose malloc; it has its own
//     entry points. So it shares the "libc-shape" binary with whichever
//     allocator the linker pulled in, but the bench code routes calls
//     through the wrapper functions below regardless.
//
// Why singleton globals in every `Allocator_misra_*.c`: the misra
// project's library convention is init-by-value with no globals (see
// `CODING-CONVENTIONS.md`). The bench harness deliberately violates
// that because Google Benchmark's `BENCHMARK(BM_*)` signature offers
// no user-data slot to thread an allocator through; the body's only
// reachable state is process-global. Each backend file therefore
// holds one `static <X>Allocator g_<x>` plus a liveness flag, and
// `bench_init` / `bench_teardown` book-end the process lifetime.
// This shape is a harness contract and should NOT be copied into
// library code or new callers; production code passes the allocator
// in explicitly per the conventions.

#ifndef BENCH_ALLOCATOR_H
#define BENCH_ALLOCATOR_H

#include <stddef.h>
#include <stdint.h>

#include <Misra/Std/Zstr.h>

#ifdef __cplusplus
extern "C" {
#endif

    // Backend name used in benchmark output (e.g. "glibc", "jemalloc", "misra").
    Zstr bench_backend_name(void);

    // One-time init / teardown. For libc-shape backends these are no-ops; for
    // MisraStdC they construct/destruct the HeapAllocator.
    void bench_init(void);
    void bench_teardown(void);

    // Per-benchmark hint: which workload shape the next BM_* will drive.
    // Backends whose allocator has one mode (libc-shape, HeapAllocator,
    // ArenaAllocator, DebugAllocator wrapper) treat these as no-ops.
    // Backends with a per-workload setup (SlabAllocator, BudgetAllocator,
    // PageAllocator) honour the hint by tearing down and reconstructing
    // the allocator with the requested slot size; outstanding allocations
    // from before the call are NOT preserved. The asymmetry does not
    // favour the misra backends -- a fresh slab/heap faults its first
    // page on iteration 1, and gbench's auto-scaled iteration count
    // absorbs that cold-start blip in the median.
    //
    // Call bench_use_fixed_size before the first bench_alloc of a
    // fixed-size benchmark; call bench_use_general before any benchmark
    // that uses multiple sizes or realloc.
    void bench_use_fixed_size(size_t slot);
    void bench_use_general(void);

    // Bulk-reset support. Some allocators (ArenaAllocator) can release
    // every outstanding allocation in O(1) via a single reset; others
    // have to free each one individually. Arena-shaped workloads
    // (BM_ArenaBumpReset) branch on `bench_can_reset()`:
    //   true  -> the harness calls bench_reset() once after the alloc loop.
    //   false -> the harness frees each tracked pointer in a loop.
    // Same workload, same accounting; the cost differential is the story.
    int  bench_can_reset(void);
    void bench_reset(void);

    void *bench_alloc(size_t n);
    void *bench_realloc(void *p, size_t n);
    void  bench_free(void *p);

    // Currently-live bytes, as reported by the allocator's own accounting.
    //
    //   * MisraStdC: AllocatorStats.bytes_in_use, exact.
    //   * jemalloc:  je_mallctl("stats.allocated", ...).
    //   * mimalloc:  mi_stats_get / mi_process_info.
    //   * tcmalloc:  MallocExtension::GetNumericProperty("generic.current_allocated_bytes").
    //   * glibc:     mallinfo2().uordblks.
    //
    // Returns 0 when the backend has no stats API (kept so the bench code
    // can call uniformly).
    uint64_t bench_live_bytes(void);

    // Allocator-committed footprint, in bytes. Each backend reports it
    // through its OWN introspection API, no /proc/self/statm reads -- so
    // the number reflects exactly what the heap pulled from the OS, with
    // zero process-noise from gbench, our std::vectors, libstdc++, etc.
    //
    //   * glibc:     mallinfo2().arena + .hblkhd
    //   * jemalloc:  mallctl("stats.mapped")
    //   * mimalloc:  mi_process_info(&current_commit, ...)
    //   * tcmalloc:  MallocExtension_GetNumericProperty("generic.heap_size")
    //   * misra:     AllocatorFootprintBytes(a) -- direct read of
    //                base.footprint_bytes, which every typed allocator's
    //                os_page_map/unmap pair maintains.
    //
    // (footprint - live) / footprint is the fragmentation ratio.
    uint64_t bench_footprint_bytes(void);

#ifdef __cplusplus
}
#endif

#endif // BENCH_ALLOCATOR_H
