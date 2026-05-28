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
#include <Misra/Std/Allocator/Slab.h>

// MODE_NONE = pre-init / post-teardown. MODE_HEAP / MODE_SLAB choose
// which of the two static allocator instances below is currently
// live. The two are unioned by usage, not by struct -- only one is
// initialised at any moment. We pay the (~150 + ~80 B) BSS cost to
// avoid a heap alloc on bench_init.
typedef enum {
    MODE_NONE = 0,
    MODE_HEAP = 1,
    MODE_SLAB = 2,
} bench_mode;

static HeapAllocator g_heap;
static SlabAllocator g_slab;
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

// Threshold above which the slab's slot-size design (one slab = one OS
// page, slot_size capped at the page) can't fit a single slot. Above
// this, the "correct" backend falls back to HeapAllocator so the bench
// still measures a real allocation rather than NULL returns.
//
// Matches MisraStdC's slab max (currently 4096 B, the smallest OS page
// size we target). If the slab grows multi-page support later, raise
// this threshold to match.
#define BENCH_SLAB_MAX_SLOT 4096u

void bench_use_fixed_size(size_t slot) {
    tear_current();
    if (slot > BENCH_SLAB_MAX_SLOT) {
        // Slab can't hold a slot this big; fall back to Heap and let
        // it route through XL (mmap-per-alloc) the way the upstream
        // "wrong tool" backend does. The bench's
        // wrong-vs-right-allocator comparison still works for sizes
        // within the slab's range; for larger sizes both backends
        // converge on the same Heap path.
        g_heap = HeapAllocatorInit();
        g_mode = MODE_HEAP;
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
    // _Generic in AllocatorAlloc routes to slab_allocator_allocate or
    // heap_allocator_allocate based on the static type at each call
    // site. The g_mode branch is perfectly predicted within a
    // benchmark because the mode is set once before the iteration
    // loop and never changes inside it.
    if (g_mode == MODE_SLAB) {
        return AllocatorAlloc(&g_slab, (size)n, 0);
    }
    return AllocatorAlloc(&g_heap, (size)n, 0);
}

void *bench_realloc(void *p, size_t n) {
    // SlabAllocator slot size is fixed at init; realloc is not a
    // sensible operation in slab mode. The bench's only realloc
    // workload (BM_ReallocGrow) runs in MODE_HEAP, so this branch
    // should never fire in slab mode. Return NULL (caller treats as
    // alloc failure -- the bench then reads the size-class check
    // and moves on); we avoid LOG_FATAL here because aborting the
    // whole bench process on a routing mismatch would make the
    // failure mode hostile to interactive debugging.
    if (g_mode == MODE_SLAB) {
        (void)p;
        (void)n;
        return NULL;
    }
    return AllocatorRealloc(&g_heap, p, (size)n);
}

void bench_free(void *p) {
    if (g_mode == MODE_SLAB) {
        AllocatorFree(&g_slab, p);
    } else {
        AllocatorFree(&g_heap, p);
    }
}

uint64_t bench_live_bytes(void) {
#if FEATURE_ALLOC_STATS
    // AllocatorGetStats takes a base `Allocator *`, not a typed
    // pointer -- both HeapAllocator and SlabAllocator embed
    // `Allocator base` as their first field, so the cast is layout-safe.
    AllocatorStats s;
    if (g_mode == MODE_SLAB) {
        s = AllocatorGetStats(ALLOCATOR_OF(&g_slab));
    } else if (g_mode == MODE_HEAP) {
        s = AllocatorGetStats(ALLOCATOR_OF(&g_heap));
    } else {
        return 0;
    }
    return (uint64_t)s.bytes_in_use;
#else
    return 0;
#endif
}

uint64_t bench_footprint_bytes(void) {
    // HeapAllocator and SlabAllocator no longer embed a PageAllocator;
    // each talks to the kernel directly. Footprint measurement via the
    // PageAllocator entries[] vector is no longer available here.
    (void)g_mode;
    return 0;
}
