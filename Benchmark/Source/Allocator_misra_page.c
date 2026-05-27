// MisraStdC PageAllocator backend.
//
// PageAllocator is the foundation everything else builds on: one mmap
// per allocation, rounded up to a page. Surfaces the cost of going
// straight to the kernel without any user-space bin / cache layer.
//
// Use cases: when you need page-aligned regions (shared memory, mmap'd
// files, JIT codegen), or as the page source for Heap/Slab/Arena/etc.
// Not a general-purpose allocator -- requests below page size waste
// the rest of the page.

#include "Allocator.h"

#include <Misra/Std/Allocator.h>
#include <Misra/Std/Allocator/Page.h>

static PageAllocator g_page;
static bool          g_page_live = false;

Zstr bench_backend_name(void) {
#ifdef BENCH_MISRA_VARIANT_NAME
    return BENCH_MISRA_VARIANT_NAME;
#else
    return "misra-page";
#endif
}

void bench_init(void) {
    g_page      = PageAllocatorInit();
    g_page_live = true;
}

void bench_teardown(void) {
    if (g_page_live) {
        PageAllocatorDeinit(&g_page);
        g_page_live = false;
    }
}

// Tear down + reinit the PageAllocator between benches. The retain-on-
// free policy means free_entries[] grows monotonically across bench
// runs unless we reset between them; without this, a bench that holds
// thousands of pages live (BatchAllocFree/8192) pollutes the
// free_entries[] sorted-by-size table for every subsequent bench,
// inflating the next bench's per-call cost with O(N) memmove. The
// Allocator.h doc on bench_use_fixed_size/general specifies exactly
// this teardown-and-reinit behaviour.
static void page_reset(void) {
    if (g_page_live) {
        PageAllocatorDeinit(&g_page);
    }
    g_page      = PageAllocatorInit();
    g_page_live = true;
}

void bench_use_fixed_size(size_t slot) { (void)slot; page_reset(); }
void bench_use_general(void)           { page_reset(); }
int  bench_can_reset(void)             { return 0; }
void bench_reset(void)                 {}

void *bench_alloc(size_t n) {
    return AllocatorAlloc(&g_page, (size)n, 0);
}

void *bench_realloc(void *p, size_t n) {
    return AllocatorRealloc(&g_page, p, (size)n);
}

void bench_free(void *p) {
    AllocatorFree(&g_page, p);
}

uint64_t bench_live_bytes(void) {
#if FEATURE_ALLOC_STATS
    AllocatorStats s = AllocatorGetStats(ALLOCATOR_OF(&g_page));
    return (uint64_t)s.bytes_in_use;
#else
    return 0;
#endif
}

uint64_t bench_footprint_bytes(void) {
    if (!g_page_live) return 0;
    uint64_t total = 0;
    for (u32 i = 0; i < g_page.len; i++) {
        total += (uint64_t)g_page.entries[i].bytes;
    }
    return total;
}
