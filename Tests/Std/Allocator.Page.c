/// file      : Tests/Std/Allocator.Page.c
/// Smoke tests for the page-backed allocator.

#include <stdint.h>

#include <Misra/Std/Allocator.h>
#include <Misra/Std/Container/Vec.h>
#include <Misra/Std/Io.h>
#include <Misra/Std/Log.h>
#include <Misra/Std/Memory.h>

#include "../Util/TestRunner.h"

static bool test_page_size_query(void) {
    size ps = PageAllocatorPageSize();
    bool ok = (ps >= 4096) && ((ps & (ps - 1)) == 0);
    if (!ok) {
        WriteFmt("page size invalid: {}\n", ps);
    }
    return ok;
}

static bool test_basic_alloc_and_free(void) {
    Allocator alloc = PageAllocator();
    void     *ptr   = AllocatorAlloc(&alloc, 128, true);
    bool      ok    = (ptr != NULL);

    if (ptr) {
        ((char *)ptr)[0]   = 'x';
        ((char *)ptr)[127] = 'y';
        ok                 = ok && (((char *)ptr)[0] == 'x') && (((char *)ptr)[127] == 'y');
        AllocatorFree(&alloc, ptr, 128);
    }

    AllocatorUnbind(&alloc);
    return ok;
}

static bool test_realloc_grow_then_shrink(void) {
    Allocator alloc = PageAllocator();
    size      page  = PageAllocatorPageSize();
    char     *ptr   = (char *)AllocatorAlloc(&alloc, 64, true);
    bool      ok    = (ptr != NULL);

    if (ptr) {
        ptr[0]      = 'a';
        ptr[63]     = 'z';
        char *grown = (char *)AllocatorRealloc(&alloc, ptr, 64, page * 2);
        ok          = ok && (grown != NULL);
        ok          = ok && (grown[0] == 'a') && (grown[63] == 'z');
        if (grown) {
            grown[page * 2 - 1] = 'q';
            char *shrunk        = (char *)AllocatorRealloc(&alloc, grown, page * 2, 32);
            ok                  = ok && (shrunk != NULL) && (shrunk[0] == 'a');
            if (shrunk) {
                AllocatorFree(&alloc, shrunk, 32);
            }
        }
    }

    AllocatorUnbind(&alloc);
    return ok;
}

static bool test_vec_with_page_allocator(void) {
    typedef Vec(int) IntVec;
    IntVec v  = VecInit(PageAllocator());
    bool   ok = true;

    for (int i = 0; i < 1024; i++) {
        if (!VecPushBackR(&v, i)) {
            ok = false;
            break;
        }
    }

    ok = ok && (VecLen(&v) == 1024) && (VecAt(&v, 0) == 0) && (VecAt(&v, 1023) == 1023);
    VecDeinit(&v);
    return ok;
}

static bool test_page_aligned_allocator(void) {
    // PageAllocatorAligned is best-effort beyond the page size: requests
    // smaller than a page get rounded up to the page boundary. We only
    // assert page-alignment here since that is what mmap guarantees
    // portably.
    Allocator alloc = PageAllocatorAligned(64);
    size      page  = PageAllocatorPageSize();
    void     *ptr   = AllocatorAlloc(&alloc, 1024, true);
    bool      ok    = (ptr != NULL) && (((uintptr_t)ptr & (page - 1u)) == 0);

    if (ptr) {
        AllocatorFree(&alloc, ptr, 1024);
    }

    AllocatorUnbind(&alloc);
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
