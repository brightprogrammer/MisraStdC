// MisraStdC HeapAllocator backend.
//
// Treats the per-lifetime HeapAllocator as a process-wide heap so the
// column is directly comparable to glibc/jemalloc/mimalloc/tcmalloc.
// One instance, reused across every benchmark iteration; the
// per-bench fixed-size / general-mode hints are no-ops here.
// Workload-matched columns (Slab, Arena, Page, Budget) sit alongside.

#include "Allocator.h"

#include <Misra/Std/Allocator.h>
#include <Misra/Std/Allocator/Heap.h>

static HeapAllocator g_heap;
static bool          g_heap_live = false;

Zstr bench_backend_name(void) {
#ifdef BENCH_BACKEND_NAME
    return BENCH_BACKEND_NAME;
#else
    return "misra-heap";
#endif
}

void bench_init(void) {
    g_heap      = HeapAllocatorInit();
    g_heap_live = true;
}

void bench_teardown(void) {
    if (g_heap_live) {
        HeapAllocatorDeinit(&g_heap);
        g_heap_live = false;
    }
}

void bench_use_fixed_size(size_t slot) {
    (void)slot;
}
void bench_use_general(void) {}
int  bench_can_reset(void) {
    return 0;
}
void bench_reset(void) {}

void *bench_alloc(size_t n) {
    return AllocatorAlloc(&g_heap, (size)n, 0);
}

void *bench_realloc(void *p, size_t n) {
    return AllocatorRealloc(&g_heap, p, (size)n);
}

void bench_free(void *p) {
    AllocatorFree(&g_heap, p);
}

uint64_t bench_live_bytes(void) {
#if FEATURE_ALLOC_STATS
    if (!g_heap_live)
        return 0;
    return (uint64_t)AllocatorBytesInUse(&g_heap);
#else
    return 0;
#endif
}

uint64_t bench_footprint_bytes(void) {
    if (!g_heap_live)
        return 0;
    return (uint64_t)AllocatorFootprintBytes(&g_heap);
}
