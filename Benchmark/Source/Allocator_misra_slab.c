// MisraStdC SlabAllocator backend.
//
// Slot size is fixed at init, power of two in [16, 4096]. Workloads
// outside that contract (mixed sizes, super-PAGE_SIZE slots, realloc
// across slot sizes) leave bench_alloc returning NULL, which renders
// n/a in the column.

#include "Allocator.h"

#include <Misra/Std/Allocator.h>
#include <Misra/Std/Allocator/Slab.h>

// MODE_NONE = pre-init / post-teardown. MODE_SLAB = a slot-sized slab
// is live. The slab can't serve mixed-size or super-page-slot workloads;
// in MODE_NONE bench_alloc returns NULL so the row reports n/a (instead
// of silently mis-routing through some other allocator).
typedef enum {
    SLAB_MODE_NONE = 0,
    SLAB_MODE_LIVE = 1,
} bench_slab_mode;

static SlabAllocator   g_slab;
static bench_slab_mode g_mode = SLAB_MODE_NONE;

Zstr bench_backend_name(void) {
#ifdef BENCH_BACKEND_NAME
    return BENCH_BACKEND_NAME;
#else
    return "misra-slab";
#endif
}

// SlabAllocator's slot size is fixed at init -- there is no in-place
// resize across slot sizes. Use a one-slab-per-benchmark scheme:
// bench_use_fixed_size tears the current slab and constructs one sized
// to the requested slot; bench_use_general leaves the slab uninitialised
// so any mixed-size bench reports n/a rather than mis-routing.

static void tear_current(void) {
    if (g_mode == SLAB_MODE_LIVE) {
        SlabAllocatorDeinit(&g_slab);
    }
    g_mode = SLAB_MODE_NONE;
}

void bench_init(void) {
    g_mode = SLAB_MODE_NONE;
}

void bench_teardown(void) {
    tear_current();
}

// Power-of-two slot sizes in [16, 4096] only. Larger requests (16 KiB,
// 64 KiB AllocFreePair / AllocTouchFree, the Pareto tail, ReallocGrow's
// upper steps) exceed PAGE_SIZE on every kernel we target. SLAB_ROUNDUP_POW2
// clamps to 4096 which would silently mis-size the slab, so reject
// out-of-range slot requests up front: bench_alloc returns NULL and the
// runner reports n/a.
#define BENCH_SLAB_MAX_SLOT 4096u
#define BENCH_SLAB_MIN_SLOT 16u

void bench_use_fixed_size(size_t slot) {
    tear_current();
    if (slot < BENCH_SLAB_MIN_SLOT || slot > BENCH_SLAB_MAX_SLOT) {
        return;
    }
    g_slab = SlabAllocatorInit((size)slot);
    g_mode = SLAB_MODE_LIVE;
}

void bench_use_general(void) {
    // Mixed-size workloads have no honest slab analogue: a slab has one
    // slot size for life. Leave g_mode == SLAB_MODE_NONE so bench_alloc
    // returns NULL and the row reports n/a.
    tear_current();
}

int bench_can_reset(void) {
    return 0;
}
void bench_reset(void) {}

void *bench_alloc(size_t n) {
    if (g_mode != SLAB_MODE_LIVE) {
        return NULL;
    }
    return AllocatorAlloc(&g_slab, (size)n, 0);
}

void *bench_realloc(void *p, size_t n) {
    // SlabAllocator's remap fails as soon as the new size exceeds the
    // configured slot. BM_ReallocGrow walks from 8 B to 1 MiB through
    // every size class, so this can't be served honestly. Return NULL
    // (the harness treats this as alloc failure) so the realloc row
    // reads n/a instead of silently testing some other code path.
    (void)p;
    (void)n;
    return NULL;
}

void bench_free(void *p) {
    if (g_mode != SLAB_MODE_LIVE || p == NULL) {
        return;
    }
    AllocatorFree(&g_slab, p);
}

uint64_t bench_live_bytes(void) {
#if FEATURE_ALLOC_STATS
    if (g_mode != SLAB_MODE_LIVE) {
        return 0;
    }
    return (uint64_t)AllocatorBytesInUse(&g_slab);
#else
    return 0;
#endif
}

uint64_t bench_footprint_bytes(void) {
    if (g_mode != SLAB_MODE_LIVE) {
        return 0;
    }
    return (uint64_t)AllocatorFootprintBytes(&g_slab);
}
