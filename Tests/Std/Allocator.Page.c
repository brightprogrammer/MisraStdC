/// file      : tests/std/allocator.page.c
/// Smoke tests for the page-backed allocator.

#include <Misra/Std/Allocator.h>
#include <Misra/Std/Allocator/Page.h>
#include <Misra/Std/Container/Vec.h>
#include <Misra/Std/Io.h>
#include <Misra/Std/Log.h>
#include <Misra/Std/Memory.h>

#include "../Util/TestRunner.h"

static bool test_page_size_query(void) {
    size ps = PageAllocatorPageSize(NULL);
    bool ok = (ps >= 4096) && ((ps & (ps - 1)) == 0);
    if (!ok) {
        WriteFmt("page size invalid: {}\n", ps);
    }
    return ok;
}

static bool test_basic_alloc_and_free(void) {
    PageAllocator alloc      = PageAllocatorInit();
    Allocator    *alloc_base = ALLOCATOR_OF(&alloc);
    void         *ptr        = AllocatorAlloc(alloc_base, 128, true);
    bool          ok         = (ptr != NULL);

    if (ptr) {
        ((char *)ptr)[0]   = 'x';
        ((char *)ptr)[127] = 'y';
        ok                 = ok && (((char *)ptr)[0] == 'x') && (((char *)ptr)[127] == 'y');
        AllocatorFree(&alloc, ptr);
    }

    PageAllocatorDeinit(&alloc);
    return ok;
}

static bool test_realloc_grow_then_shrink(void) {
    PageAllocator alloc      = PageAllocatorInit();
    Allocator    *alloc_base = ALLOCATOR_OF(&alloc);
    size          page       = PageAllocatorPageSize(&alloc);
    u8           *ptr        = (u8 *)AllocatorAlloc(alloc_base, 64, true);
    bool          ok         = (ptr != NULL);

    if (ptr) {
        ptr[0]    = 'a';
        ptr[63]   = 'z';
        u8 *grown = (u8 *)AllocatorRealloc(alloc_base, ptr, page * 2);
        ok        = ok && (grown != NULL);
        ok        = ok && (grown[0] == 'a') && (grown[63] == 'z');
        if (grown) {
            grown[page * 2 - 1] = 'q';
            u8 *shrunk          = (u8 *)AllocatorRealloc(alloc_base, grown, 32);
            ok                  = ok && (shrunk != NULL) && (shrunk[0] == 'a');
            if (shrunk) {
                AllocatorFree(&alloc, shrunk);
            }
        }
    }

    PageAllocatorDeinit(&alloc);
    return ok;
}

static bool test_vec_with_page_allocator(void) {
    // PageAllocator backs Vec via the generic Allocator dispatch.
    // Exercises the internal descriptor table across multiple grows.
    PageAllocator alloc = PageAllocatorInit();
    typedef Vec(int) IntVec;
    IntVec v  = VecInit(&alloc);
    bool   ok = true;

    for (int i = 0; i < 1024; i++) {
        if (!VecPushBackR(&v, i)) {
            ok = false;
            break;
        }
    }

    ok = ok && (VecLen(&v) == 1024) && (VecAt(&v, 0) == 0) && (VecAt(&v, 1023) == 1023);
    VecDeinit(&v);
    PageAllocatorDeinit(&alloc);
    return ok;
}

static bool test_page_aligned_allocator(void) {
    // PageAllocatorInitAligned is best-effort beyond the page size: requests
    // smaller than a page get rounded up to the page boundary. We only
    // assert page-alignment here since that is what mmap guarantees
    // portably.
    PageAllocator alloc      = PageAllocatorInitAligned(64);
    Allocator    *alloc_base = ALLOCATOR_OF(&alloc);
    size          page       = PageAllocatorPageSize(&alloc);
    void         *ptr        = AllocatorAlloc(alloc_base, 1024, true);
    bool          ok         = (ptr != NULL) && (((u64)ptr & (page - 1u)) == 0);

    if (ptr) {
        AllocatorFree(&alloc, ptr);
    }

    PageAllocatorDeinit(&alloc);
    return ok;
}

static bool test_footprint_and_entry_count(void) {
    // PageAllocatorEntryCount tracks the live `entries[]` only; retained
    // pages on free_entries[] don't count. PageAllocatorFootprintBytes
    // covers both, plus the descriptor tables. NULL gives 0 for both.
    bool ok = (PageAllocatorFootprintBytes(NULL) == 0);
    if (!ok) { WriteFmt("NULL footprint != 0\n"); return false; }

    PageAllocator alloc      = PageAllocatorInit();
    Allocator    *alloc_base = ALLOCATOR_OF(&alloc);
    size          page       = PageAllocatorPageSize(&alloc);

    if (PageAllocatorEntryCount(&alloc) != 0) { WriteFmt("init EntryCount != 0\n"); ok = false; }
    if (PageAllocatorFootprintBytes(&alloc) != 0) { WriteFmt("init Footprint != 0\n"); ok = false; }

    void *p1 = AllocatorAlloc(alloc_base, page, true);
    void *p2 = AllocatorAlloc(alloc_base, page * 2, true);
    if (!p1 || !p2) { WriteFmt("alloc failed\n"); ok = false; }
    if (PageAllocatorEntryCount(&alloc) != 2) {
        WriteFmt("after 2 allocs EntryCount={} want 2\n", (u64)PageAllocatorEntryCount(&alloc));
        ok = false;
    }
    // Live footprint is at least p1 + p2; descriptor table adds more.
    size foot_with_two = PageAllocatorFootprintBytes(&alloc);
    if (foot_with_two < page + page * 2) {
        WriteFmt("foot_with_two={} want >= {}\n", (u64)foot_with_two, (u64)(page + page * 2));
        ok = false;
    }

    if (p1) {
        AllocatorFree(&alloc, p1);
    }
    // EntryCount drops to 1 (p1 moved to free_entries[]).
    if (PageAllocatorEntryCount(&alloc) != 1) {
        WriteFmt("after free p1 EntryCount={} want 1\n", (u64)PageAllocatorEntryCount(&alloc));
        ok = false;
    }
    // Footprint includes descriptor table grow on free side, so it
    // can be >= the live-only baseline. Must NOT shrink.
    size foot_after_free_p1 = PageAllocatorFootprintBytes(&alloc);
    if (foot_after_free_p1 < foot_with_two) {
        WriteFmt("foot shrank after retention: {} < {}\n", (u64)foot_after_free_p1, (u64)foot_with_two);
        ok = false;
    }

    if (p2) {
        AllocatorFree(&alloc, p2);
    }
    if (PageAllocatorEntryCount(&alloc) != 0) {
        WriteFmt("after free p2 EntryCount={} want 0\n", (u64)PageAllocatorEntryCount(&alloc));
        ok = false;
    }
    if (PageAllocatorFootprintBytes(&alloc) < foot_with_two) {
        WriteFmt("foot shrank after all-free\n");
        ok = false;
    }

    PageAllocatorDeinit(&alloc);
    // Post-deinit the struct is zeroed: both accessors return 0.
    if (PageAllocatorEntryCount(&alloc) != 0) { WriteFmt("post-deinit EntryCount != 0\n"); ok = false; }
    if (PageAllocatorFootprintBytes(&alloc) != 0) { WriteFmt("post-deinit Footprint != 0\n"); ok = false; }
    return ok;
}

int main(void) {
    TestFunction tests[] = {
        test_page_size_query,
        test_basic_alloc_and_free,
        test_realloc_grow_then_shrink,
        test_vec_with_page_allocator,
        test_page_aligned_allocator,
        test_footprint_and_entry_count,
    };
    return run_test_suite(tests, (int)(sizeof(tests) / sizeof(tests[0])), NULL, 0, "Allocator.Page");
}
