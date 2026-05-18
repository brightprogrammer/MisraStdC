#include <Misra.h>
#include <Misra/Std/Allocator/Debug.h>

#include "../Util/TestRunner.h"

// Allocate + free a block — leak count should be 0 at destroy, no
// overflows.
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
    ok = ok && DebugAllocatorOverflows(&dbg) == 0;

    DebugAllocatorDeinit(&dbg);
    return ok;
}

// Write past the end of an allocation; canary check on free should
// bump the overflow counter.
bool test_debug_catches_overflow(void) {
    DebugAllocator dbg  = DebugAllocatorInit();
    Allocator     *adbg = ALLOCATOR_OF(&dbg);

    u8 *buf = (u8 *)AllocatorAlloc(adbg, 16, true);
    // Stomp on the canary by writing the first byte past the user region.
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
    // free everything via the internal heap.
    DebugAllocatorDeinit(&dbg);
    return ok;
}

// Page-backed mode: every alloc consumes a page, every free
// PROT_NONEs the region instead of releasing it. Verifies basic
// alloc + free updates bookkeeping correctly.
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
    ok = ok && ((u64)p & 0xfff) == 0;

    AllocatorFree(adbg, p, 64);
    ok = ok && DebugAllocatorLiveCount(&dbg) == 0;

    DebugAllocatorDeinit(&dbg);
    return ok;
}

// Double-free detection has moved to the underlying HeapAllocator
// (which LOG_FATALs on bit-clear-of-cleared). This is a deadend test:
// the second free crashes the process via the underlying Heap.
bool test_debug_double_free_aborts(void) {
    DebugAllocator dbg  = DebugAllocatorInit();
    Allocator     *adbg = ALLOCATOR_OF(&dbg);

    void *p = AllocatorAlloc(adbg, 32, true);
    AllocatorFree(adbg, p, 32);
    AllocatorFree(adbg, p, 32); // forwarded to Heap -> LOG_FATAL
    return false;               // unreachable
}

int main(void) {
    WriteFmt("[INFO] Starting AllocDebug tests\n\n");

    TestFunction normal[] = {
        test_debug_normal_alloc_free,
        test_debug_catches_overflow,
        test_debug_leak_count,
        test_debug_page_backed_alloc_free,
    };
    TestFunction deadend[] = {
        test_debug_double_free_aborts,
    };

    return run_test_suite(
        normal,
        sizeof(normal) / sizeof(normal[0]),
        deadend,
        sizeof(deadend) / sizeof(deadend[0]),
        "AllocDebug"
    );
}
