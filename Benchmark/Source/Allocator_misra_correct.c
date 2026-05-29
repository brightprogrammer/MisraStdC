// MisraStdC backend that picks the right allocator per benchmark.
//
// This is the "right tool" comparison, opposite the one in
// Allocator_misra.c which uses HeapAllocator for everything. The
// bench code calls bench_use_fixed_size(slot) before fixed-size
// workloads (BM_AllocFreePair, BM_BatchAllocFree, BM_AllocTouchFree).
// This backend then destroys whatever it was using and constructs a
// SlabAllocator with the requested slot size -- an intrusive
// free-list allocator that's O(1) on alloc and free and is the
// MisraStdC-shaped equivalent of tcmalloc/jemalloc's per-thread
// fixed-class fastpath.
//
// For mixed-size workloads (BM_ReallocGrow, BM_MixedPareto,
// BM_Frag_*) the bench calls bench_use_general() and we go back to
// HeapAllocator, which is the right allocator for general-purpose,
// process-heap-shaped use.
//
// Comparing this backend against Allocator_misra.c on the same
// suite shows the cost of using the wrong MisraStdC allocator for
// a fixed-size workload -- not a flaw of the library, just a
// mis-tooling.

#include "Allocator.h"

#include <Misra/Std/Allocator.h>
#include <Misra/Std/Allocator/Heap.h>
#include <Misra/Std/Allocator/Page.h>
#include <Misra/Std/Allocator/Slab.h>

// MODE_NONE = pre-init / post-teardown. MODE_HEAP / MODE_SLAB / MODE_PAGE
// choose which of the static allocator instances below is currently
// live. They're unioned by usage, not by struct -- only one is
// initialised at any moment.
typedef enum {
    MODE_NONE = 0,
    MODE_HEAP = 1,
    MODE_SLAB = 2,
    MODE_PAGE = 3,
} bench_mode;

static HeapAllocator g_heap;
static SlabAllocator g_slab;
static PageAllocator g_page;
static bench_mode     g_mode = MODE_NONE;

Zstr bench_backend_name(void) {
#ifdef BENCH_MISRA_VARIANT_NAME
    return BENCH_MISRA_VARIANT_NAME;
#else
    return "misra-correct";
#endif
}

// Free whatever allocator is currently live, leaving g_mode in
// MODE_NONE. Called before every mode-switch and on teardown.
static void tear_current(void) {
    if (g_mode == MODE_HEAP) {
        HeapAllocatorDeinit(&g_heap);
    } else if (g_mode == MODE_SLAB) {
        SlabAllocatorDeinit(&g_slab);
    } else if (g_mode == MODE_PAGE) {
        PageAllocatorDeinit(&g_page);
    }
    g_mode = MODE_NONE;
}

void bench_init(void) {
    g_heap = HeapAllocatorInit();
    g_mode = MODE_HEAP;
}

void bench_teardown(void) {
    tear_current();
}

// SlabAllocator caps at MisraStdC's slab max (currently 4096 B, the
// smallest OS page size we target). Above this, the "right tool" is
// PageAllocator -- the request is already page-aligned in size, so
// one mmap per alloc is the honest cost, no rounding waste, no
// general-heap dispatch noise. The previous fallback to Heap measured
// the wrong-tool case, which is exactly what the `misra` (Heap-only)
// column is for.
#define BENCH_SLAB_MAX_SLOT 4096u

void bench_use_fixed_size(size_t slot) {
    tear_current();
    if (slot > BENCH_SLAB_MAX_SLOT) {
        g_page = PageAllocatorInit();
        g_mode = MODE_PAGE;
        return;
    }
    g_slab = SlabAllocatorInit((size)slot);
    g_mode = MODE_SLAB;
}

void bench_use_general(void) {
    tear_current();
    g_heap = HeapAllocatorInit();
    g_mode = MODE_HEAP;
}

// Slab and Heap both support per-pointer free natively; arena-shaped
// workloads fall back to per-pointer free in the bench harness.
int  bench_can_reset(void) { return 0; }
void bench_reset(void)     {}

void *bench_alloc(size_t n) {
    // _Generic in AllocatorAlloc routes to the typed *_allocator_allocate
    // statically. The g_mode branch is perfectly predicted within a
    // benchmark -- mode is set once before the iteration loop and
    // never changes inside it.
    if (g_mode == MODE_SLAB) {
        return AllocatorAlloc(&g_slab, (size)n, 0);
    }
    if (g_mode == MODE_PAGE) {
        return AllocatorAlloc(&g_page, (size)n, 0);
    }
    return AllocatorAlloc(&g_heap, (size)n, 0);
}

void *bench_realloc(void *p, size_t n) {
    // Realloc is sensible only on the general-purpose Heap path.
    // BM_ReallocGrow is the sole realloc workload and runs in
    // MODE_HEAP, so the slab / page branches should never fire here.
    // Return NULL (caller treats as alloc failure) instead of
    // LOG_FATAL so a routing mismatch doesn't kill interactive runs.
    if (g_mode == MODE_SLAB || g_mode == MODE_PAGE) {
        (void)p;
        (void)n;
        return NULL;
    }
    return AllocatorRealloc(&g_heap, p, (size)n);
}

void bench_free(void *p) {
    if (g_mode == MODE_SLAB) {
        AllocatorFree(&g_slab, p);
    } else if (g_mode == MODE_PAGE) {
        AllocatorFree(&g_page, p);
    } else {
        AllocatorFree(&g_heap, p);
    }
}

uint64_t bench_live_bytes(void) {
#if FEATURE_ALLOC_STATS
    if (g_mode == MODE_SLAB) return (uint64_t)AllocatorBytesInUse(&g_slab);
    if (g_mode == MODE_HEAP) return (uint64_t)AllocatorBytesInUse(&g_heap);
    if (g_mode == MODE_PAGE) return (uint64_t)AllocatorBytesInUse(&g_page);
    return 0;
#else
    return 0;
#endif
}

uint64_t bench_footprint_bytes(void) {
    if (g_mode == MODE_HEAP) return (uint64_t)AllocatorFootprintBytes(&g_heap);
    if (g_mode == MODE_SLAB) return (uint64_t)AllocatorFootprintBytes(&g_slab);
    if (g_mode == MODE_PAGE) return (uint64_t)AllocatorFootprintBytes(&g_page);
    return 0;
}
