/// file      : Tests/Std/Allocator.Page.c
/// Smoke tests for the page-backed allocator.

#include <stdint.h>

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
        AllocatorFree(&alloc.base, ptr);
    }

    PageAllocatorDeinit(&alloc);
    return ok;
}

static bool test_realloc_grow_then_shrink(void) {
    PageAllocator alloc      = PageAllocatorInit();
    Allocator    *alloc_base = ALLOCATOR_OF(&alloc);
    size          page       = PageAllocatorPageSize(&alloc);
    char         *ptr        = (char *)AllocatorAlloc(alloc_base, 64, true);
    bool          ok         = (ptr != NULL);

    if (ptr) {
        ptr[0]      = 'a';
        ptr[63]     = 'z';
        char *grown = (char *)AllocatorRealloc(alloc_base, ptr, page * 2);
        ok          = ok && (grown != NULL);
        ok          = ok && (grown[0] == 'a') && (grown[63] == 'z');
        if (grown) {
            grown[page * 2 - 1] = 'q';
            char *shrunk        = (char *)AllocatorRealloc(alloc_base, grown, 32);
            ok                  = ok && (shrunk != NULL) && (shrunk[0] == 'a');
            if (shrunk) {
                AllocatorFree(&alloc.base, shrunk);
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
    bool          ok         = (ptr != NULL) && (((uintptr_t)ptr & (page - 1u)) == 0);

    if (ptr) {
        AllocatorFree(&alloc.base, ptr);
    }

    PageAllocatorDeinit(&alloc);
    return ok;
}

int main(void) {
    TestFunction tests[] = {
        test_page_size_query,
        test_basic_alloc_and_free,
        test_realloc_grow_then_shrink,
        test_vec_with_page_allocator,
        test_page_aligned_allocator,
    };
    return run_test_suite(tests, (int)(sizeof(tests) / sizeof(tests[0])), NULL, 0, "Allocator.Page");
}
