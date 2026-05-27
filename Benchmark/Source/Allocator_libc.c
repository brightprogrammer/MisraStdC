// libc-shape backend: glibc / jemalloc / mimalloc / tcmalloc.
//
// alloc/free/realloc are identical across the four; they all interpose
// `malloc`/`free`/`realloc`. Only the footprint query differs --
// each library has its own stats API and we dispatch on
// BENCH_BACKEND_<NAME> set by meson.

#include "Allocator.h"

#include <stdlib.h>
#include <string.h>

#if BENCH_BACKEND_GLIBC
#  include <malloc.h>
#elif BENCH_BACKEND_JEMALLOC
#  include <jemalloc/jemalloc.h>
#elif BENCH_BACKEND_MIMALLOC
#  include <mimalloc.h>
#elif BENCH_BACKEND_TCMALLOC
#  include <gperftools/malloc_extension_c.h>
#endif

Zstr bench_backend_name(void) {
    return BENCH_BACKEND_NAME;
}

void bench_init(void)     {}
void bench_teardown(void) {}

// libc-shape backends don't have a per-size specialised allocator
// (malloc handles any size from a unified heap), so the size-class
// hints are no-ops. The hints exist so the bench code can write one
// suite that runs unchanged across every backend.
void bench_use_fixed_size(size_t slot) { (void)slot; }
void bench_use_general(void)           {}

// Libc has no bulk-reset; arena-shaped workloads fall back to
// per-pointer free in the harness.
int  bench_can_reset(void) { return 0; }
void bench_reset(void)     {}

void *bench_alloc(size_t n)              { return malloc(n);     }
void *bench_realloc(void *p, size_t n)   { return realloc(p, n); }
void  bench_free(void *p)                { free(p);              }

uint64_t bench_live_bytes(void) {
    // Each backend exposes "currently-allocated bytes" through its
    // own API; we read the same shape as bench_footprint_bytes does
    // for that backend. Reported alongside as a sanity check
    // against the harness's external sum_live() bookkeeping.
#if BENCH_BACKEND_GLIBC
    struct mallinfo2 mi = mallinfo2();
    return (uint64_t)mi.uordblks + (uint64_t)mi.hblkhd;
#elif BENCH_BACKEND_JEMALLOC
    uint64_t epoch = 1;
    mallctl("epoch", NULL, NULL, &epoch, sizeof(epoch));
    size_t allocated = 0;
    size_t sz = sizeof(allocated);
    if (mallctl("stats.allocated", &allocated, &sz, NULL, 0) != 0) return 0;
    return (uint64_t)allocated;
#elif BENCH_BACKEND_MIMALLOC
    // mimalloc doesn't split "live" out cleanly; the `committed`
    // counter is the closest, and that's what bench_footprint_bytes
    // returns. Report 0 here so the bench falls back to external
    // tracking for the live column.
    return 0;
#elif BENCH_BACKEND_TCMALLOC
    size_t v = 0;
    if (!MallocExtension_GetNumericProperty(
            "generic.current_allocated_bytes", &v)) return 0;
    return (uint64_t)v;
#else
    return 0;
#endif
}

uint64_t bench_footprint_bytes(void) {
#if BENCH_BACKEND_GLIBC
    // mallinfo2().arena = total sbrk'd bytes, .hblkhd = total
    // mmap'd bytes. Sum is everything glibc has pulled from the
    // kernel for this process via the allocator.
    struct mallinfo2 mi = mallinfo2();
    return (uint64_t)mi.arena + (uint64_t)mi.hblkhd;
#elif BENCH_BACKEND_JEMALLOC
    // jemalloc requires an epoch refresh before stats.* reads
    // reflect current state. stats.mapped = total bytes mapped
    // from the OS (anywhere -- chunks, base allocator, etc.).
    uint64_t epoch = 1;
    mallctl("epoch", NULL, NULL, &epoch, sizeof(epoch));
    size_t mapped = 0;
    size_t sz = sizeof(mapped);
    if (mallctl("stats.mapped", &mapped, &sz, NULL, 0) != 0) return 0;
    return (uint64_t)mapped;
#elif BENCH_BACKEND_MIMALLOC
    // mimalloc 3.3.0's public footprint introspection is unreliable:
    //   - mi_process_info(...&current_commit...): MI_STAT-gated, returns
    //     SIZE_MAX-ish garbage in the default release build.
    //   - mi_stats_get(&stats): header (mimalloc-stats.h:147) declares
    //     `bool mi_stats_get(mi_stats_t*)` but the source upstream took
    //     a two-arg form. Calling per the header returns stats with
    //     .committed.current = 0 even after enabling MI_STAT=1 in the
    //     build override; calling per the source ABI mis-passes args
    //     and segfaults in mi_subproc_stats_get.
    // Return 0 so the script reports n/a for mimalloc footprint. The
    // four other backends introspect cleanly.
    return 0;
#elif BENCH_BACKEND_TCMALLOC
    // generic.heap_size = bytes managed by tcmalloc (allocated + free
    // pages still owned by the allocator). Matches "footprint".
    size_t v = 0;
    if (!MallocExtension_GetNumericProperty("generic.heap_size", &v)) return 0;
    return (uint64_t)v;
#else
    return 0;
#endif
}
