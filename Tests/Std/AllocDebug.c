#include <Misra.h>
#include <Misra/Std/Allocator/Debug.h>
#include <Misra/Std/Allocator/Heap.h>

#include "../Util/TestRunner.h"

// Allocate + free a block — leak count should be 0 at destroy, no
// overflows, no double-frees.
bool test_debug_normal_alloc_free(void) {
    HeapAllocator backing = HeapAllocatorInit();
    HeapAllocator meta    = HeapAllocatorInit();

    DebugAllocator *dbg = DebugAllocatorCreate(ALLOCATOR_OF(&backing), ALLOCATOR_OF(&meta));
    if (!dbg) {
        return false;
    }

    Allocator *adbg = ALLOCATOR_OF(dbg);
    void      *p1   = AllocatorAlloc(adbg, 64, true);
    void      *p2   = AllocatorAlloc(adbg, 128, true);

    bool ok = p1 && p2;
    ok      = ok && DebugAllocatorLiveCount(dbg) == 2;
    ok      = ok && DebugAllocatorLiveBytes(dbg) == (64 + 128);

    AllocatorFree(adbg, p1, 64);
    ok = ok && DebugAllocatorLiveCount(dbg) == 1;
    ok = ok && DebugAllocatorLiveBytes(dbg) == 128;

    AllocatorFree(adbg, p2, 128);
    ok = ok && DebugAllocatorLiveCount(dbg) == 0;
    ok = ok && DebugAllocatorLiveBytes(dbg) == 0;
    ok = ok && DebugAllocatorDoubleFrees(dbg) == 0;
    ok = ok && DebugAllocatorOverflows(dbg) == 0;

    DebugAllocatorDestroy(dbg, ALLOCATOR_OF(&meta));
    HeapAllocatorDeinit(&meta);
    HeapAllocatorDeinit(&backing);
    return ok;
}

// Free the same pointer twice; second free should bump the
// double-free counter without crashing.
bool test_debug_catches_double_free(void) {
    HeapAllocator backing = HeapAllocatorInit();
    HeapAllocator meta    = HeapAllocatorInit();

    DebugAllocator *dbg  = DebugAllocatorCreate(ALLOCATOR_OF(&backing), ALLOCATOR_OF(&meta));
    Allocator      *adbg = ALLOCATOR_OF(dbg);

    void *p = AllocatorAlloc(adbg, 32, true);
    AllocatorFree(adbg, p, 32);
    AllocatorFree(adbg, p, 32); // expected to be caught

    bool ok = DebugAllocatorDoubleFrees(dbg) == 1;

    DebugAllocatorDestroy(dbg, ALLOCATOR_OF(&meta));
    HeapAllocatorDeinit(&meta);
    HeapAllocatorDeinit(&backing);
    return ok;
}

// Write past the end of an allocation; canary check on free should
// bump the overflow counter.
bool test_debug_catches_overflow(void) {
    HeapAllocator backing = HeapAllocatorInit();
    HeapAllocator meta    = HeapAllocatorInit();

    DebugAllocator *dbg  = DebugAllocatorCreate(ALLOCATOR_OF(&backing), ALLOCATOR_OF(&meta));
    Allocator      *adbg = ALLOCATOR_OF(dbg);

    u8 *buf = (u8 *)AllocatorAlloc(adbg, 16, true);
    // Stomp on the canary by writing the first byte past the user
    // region.
    buf[16] = 0x55;
    AllocatorFree(adbg, buf, 16);

    bool ok = DebugAllocatorOverflows(dbg) == 1;

    DebugAllocatorDestroy(dbg, ALLOCATOR_OF(&meta));
    HeapAllocatorDeinit(&meta);
    HeapAllocatorDeinit(&backing);
    return ok;
}

// Allocate without freeing; live count should reflect the leak when
// we query before destroy.
bool test_debug_leak_count(void) {
    HeapAllocator backing = HeapAllocatorInit();
    HeapAllocator meta    = HeapAllocatorInit();

    DebugAllocator *dbg  = DebugAllocatorCreate(ALLOCATOR_OF(&backing), ALLOCATOR_OF(&meta));
    Allocator      *adbg = ALLOCATOR_OF(dbg);

    (void)AllocatorAlloc(adbg, 32, true);
    (void)AllocatorAlloc(adbg, 48, true);

    bool ok = DebugAllocatorLiveCount(dbg) == 2;
    ok      = ok && DebugAllocatorLiveBytes(dbg) == (32 + 48);

    // DebugAllocatorDestroy will free the underlying buffers via the
    // parent and log the two leaks — but it tears down the tracking
    // map. Capture the count first.
    DebugAllocatorDestroy(dbg, ALLOCATOR_OF(&meta));

    // backing is HeapAllocator — it does not detect the leak itself;
    // libc malloc just retains the blocks until process exit. That is
    // fine: the DebugAllocator surfaced the issue before we lost the
    // information.
    HeapAllocatorDeinit(&meta);
    HeapAllocatorDeinit(&backing);
    return ok;
}

int main(void) {
    WriteFmt("[INFO] Starting AllocDebug tests\n\n");

    TestFunction tests[] = {
        test_debug_normal_alloc_free,
        test_debug_catches_double_free,
        test_debug_catches_overflow,
        test_debug_leak_count,
    };

    return run_test_suite(tests, sizeof(tests) / sizeof(tests[0]), NULL, 0, "AllocDebug");
}
