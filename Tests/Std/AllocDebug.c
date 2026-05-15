#include <Misra.h>
#include <Misra/Std/Allocator/Debug.h>

#include <stdint.h>

#include "../Util/TestRunner.h"

// Allocate + free a block — leak count should be 0 at destroy, no
// overflows, no double-frees.
bool test_debug_normal_alloc_free(void) {
    DebugAllocator dbg  = DebugAllocatorInit();
    Allocator     *adbg = ALLOCATOR_OF(&dbg);

    void *p1 = AllocatorAlloc(adbg, 64, true);
    void *p2 = AllocatorAlloc(adbg, 128, true);

    bool ok = p1 && p2;
    ok      = ok && DebugAllocatorLiveCount(&dbg) == 2;
    ok      = ok && DebugAllocatorLiveBytes(&dbg) == (64 + 128);

    AllocatorFree(adbg, p1, 64);
    ok = ok && DebugAllocatorLiveCount(&dbg) == 1;
    ok = ok && DebugAllocatorLiveBytes(&dbg) == 128;

    AllocatorFree(adbg, p2, 128);
    ok = ok && DebugAllocatorLiveCount(&dbg) == 0;
    ok = ok && DebugAllocatorLiveBytes(&dbg) == 0;
    ok = ok && DebugAllocatorDoubleFrees(&dbg) == 0;
    ok = ok && DebugAllocatorOverflows(&dbg) == 0;

    DebugAllocatorDeinit(&dbg);
    return ok;
}

// Free the same pointer twice; second free should bump the
// double-free counter without crashing.
bool test_debug_catches_double_free(void) {
    DebugAllocator dbg  = DebugAllocatorInit();
    Allocator     *adbg = ALLOCATOR_OF(&dbg);

    void *p = AllocatorAlloc(adbg, 32, true);
    AllocatorFree(adbg, p, 32);
    AllocatorFree(adbg, p, 32); // expected to be caught

    bool ok = DebugAllocatorDoubleFrees(&dbg) == 1;

    DebugAllocatorDeinit(&dbg);
    return ok;
}

// Write past the end of an allocation; canary check on free should
// bump the overflow counter.
bool test_debug_catches_overflow(void) {
    DebugAllocator dbg  = DebugAllocatorInit();
    Allocator     *adbg = ALLOCATOR_OF(&dbg);

    u8 *buf = (u8 *)AllocatorAlloc(adbg, 16, true);
    // Stomp on the canary by writing the first byte past the user
    // region.
    buf[16] = 0x55;
    AllocatorFree(adbg, buf, 16);

    bool ok = DebugAllocatorOverflows(&dbg) == 1;

    DebugAllocatorDeinit(&dbg);
    return ok;
}

// Allocate without freeing; live count should reflect the leak when
// we query before destroy.
bool test_debug_leak_count(void) {
    DebugAllocator dbg  = DebugAllocatorInit();
    Allocator     *adbg = ALLOCATOR_OF(&dbg);

    (void)AllocatorAlloc(adbg, 32, true);
    (void)AllocatorAlloc(adbg, 48, true);

    bool ok = DebugAllocatorLiveCount(&dbg) == 2;
    ok      = ok && DebugAllocatorLiveBytes(&dbg) == (32 + 48);

    // DebugAllocatorDeinit will print the leaks (LOG_ERROR) and
    // free everything via the internal heap. The leaked records are
    // surfaced before the tracking map gets torn down.
    DebugAllocatorDeinit(&dbg);
    return ok;
}

// Page-backed mode: every alloc consumes a page, every free
// PROT_NONEs the region instead of releasing it. Verifies basic
// alloc + free still update bookkeeping correctly. Double-free
// detection still works (looked up via map BEFORE dereferencing the
// protected memory).
bool test_debug_page_backed_alloc_free(void) {
    DebugAllocatorConfig cfg = DEBUG_ALLOCATOR_DEFAULTS;
    cfg.force_page_backing   = true;
    DebugAllocator dbg       = DebugAllocatorInitWith(cfg);
    Allocator     *adbg      = ALLOCATOR_OF(&dbg);

    void *p  = AllocatorAlloc(adbg, 64, true);
    bool  ok = p != NULL;
    ok       = ok && DebugAllocatorLiveCount(&dbg) == 1;
    ok       = ok && DebugAllocatorLiveBytes(&dbg) == 64;

    // Page-backed allocations should be page-aligned.
    ok = ok && ((uintptr_t)p & 0xfff) == 0;

    AllocatorFree(adbg, p, 64);
    ok = ok && DebugAllocatorLiveCount(&dbg) == 0;
    ok = ok && DebugAllocatorDoubleFrees(&dbg) == 0;

    // Second free of the same pointer is caught (via the freed map),
    // never dereferences the PROT_NONE'd memory.
    AllocatorFree(adbg, p, 64);
    ok = ok && DebugAllocatorDoubleFrees(&dbg) == 1;

    DebugAllocatorDeinit(&dbg);
    return ok;
}

int main(void) {
    WriteFmt("[INFO] Starting AllocDebug tests\n\n");

    TestFunction tests[] = {
        test_debug_normal_alloc_free,
        test_debug_catches_double_free,
        test_debug_catches_overflow,
        test_debug_leak_count,
        test_debug_page_backed_alloc_free,
    };

    return run_test_suite(tests, sizeof(tests) / sizeof(tests[0]), NULL, 0, "AllocDebug");
}
