// MisraStdC ArenaAllocator backend.
//
// Arena does bump-pointer allocation and gives back NO per-allocation
// free. Releasing memory is either Reset (zero the bump pointer, keep
// the backing) or Deinit (release the backing). This backend exposes
// that via bench_can_reset() / bench_reset(): arena-shaped workloads
// like BM_ArenaBumpReset use the bulk reset; pair-style workloads
// (alloc + free per iter) leak in this backend until the next
// bench_use_general() call tears the arena down.
//
// Comparing this backend's BM_ArenaBumpReset against the libc and
// HeapAllocator backends shows the cost differential of "1 reset"
// vs "N individual frees" for the same bump-N workload.

#include "Allocator.h"

#include <Misra/Std/Allocator.h>
#include <Misra/Std/Allocator/Arena.h>
#include <Misra/Std/Allocator/Page.h>

static ArenaAllocator g_arena;
static int            g_arena_live = 0;

const char *bench_backend_name(void) {
#ifdef BENCH_MISRA_VARIANT_NAME
    return BENCH_MISRA_VARIANT_NAME;
#else
    return "misra-arena";
#endif
}

void bench_init(void) {
    g_arena      = ArenaAllocatorInit();
    g_arena_live = 1;
}

void bench_teardown(void) {
    if (g_arena_live) {
        ArenaAllocatorDeinit(&g_arena);
        g_arena_live = 0;
    }
}

// Arena has one mode -- bump. The hints come in between benchmarks;
// reset the arena on each so a bench that doesn't call bench_reset()
// (pair-style workloads where bench_free is a no-op by arena design)
// at least starts each new bench with a fresh arena, bounding the
// per-bench memory growth to that single bench's iteration count.
void bench_use_fixed_size(size_t slot) {
    (void)slot;
    if (g_arena_live) ArenaAllocatorReset(&g_arena);
}
void bench_use_general(void) {
    if (g_arena_live) ArenaAllocatorReset(&g_arena);
}

int bench_can_reset(void) { return 1; }
void bench_reset(void) {
    // ArenaAllocatorReset rewinds the bump pointer and frees every
    // page-backed slab the arena owns except the most recent one.
    // O(pages-1) cost, independent of allocation count.
    ArenaAllocatorReset(&g_arena);
}

void *bench_alloc(size_t n) {
    return AllocatorAlloc(&g_arena, (size)n, 0);
}

void *bench_realloc(void *p, size_t n) {
    return AllocatorRealloc(&g_arena, p, (size)n);
}

void bench_free(void *p) {
    // Arena's deallocate is LIFO-rewinding: when `ptr` is the
    // most-recently-bumped allocation (`arena->last_ptr == ptr`),
    // it rewinds the bump cursor and frees the allocation for real.
    // For any other ptr the call is a no-op (arena holds onto it
    // until Reset/Deinit). This lets pair-style benches
    // (alloc-then-free-immediately) run with zero net memory
    // growth and gives BatchAllocFree honest numbers when the bench
    // frees in reverse order.
    AllocatorFree(&g_arena, p);
}

uint64_t bench_live_bytes(void) {
#if FEATURE_ALLOC_STATS
    AllocatorStats s = AllocatorGetStats(ALLOCATOR_OF(&g_arena));
    return (uint64_t)s.bytes_in_use;
#else
    return 0;
#endif
}

uint64_t bench_footprint_bytes(void) {
    if (!g_arena_live) return 0;
    const PageAllocator *p = &g_arena.page;
    uint64_t total = 0;
    for (u32 i = 0; i < p->len; i++) {
        total += (uint64_t)p->entries[i].bytes;
    }
    return total;
}
