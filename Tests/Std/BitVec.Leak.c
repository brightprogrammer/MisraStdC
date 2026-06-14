/// file : tests/std/bitvec.leak.c
/// Leak tests for BitVec: kill a reachable mutation survivor by routing
/// every allocation through an explicit DebugAllocator and asserting the
/// live count is zero after teardown. A dropped internal *Deinit leaves a
/// live allocation, so the count assertion fails -> the mutant is killed.
/// Distinct from existing BitVec.* tests (which use DefaultAllocator and
/// never observe a leak).
#include <Misra.h>
#include <Misra/Std/Allocator/Debug.h>
#include <Misra/Std/Container/BitVec.h>

#include "../Util/TestRunner.h"

// ---- 1157:5 cxx_remove_void_call ----------------------------------------
// BitVecRotateRight clones the source into a temp BitVec (line 1145) and
// BitVecDeinit's it at line 1157 on the success path before returning.
// Removing that Deinit leaks the clone's backing on EVERY successful
// rotate. Init the bitvec with a DebugAllocator (the clone inherits that
// allocator), drive a real rotate, and require live_count back to its
// pre-rotate value after the bitvec itself is freed.
bool test_rotate_right_frees_temp_clone(void);
bool test_rotate_right_frees_temp_clone(void) {
    WriteFmt("Testing BitVecRotateRight frees its temp clone (1157:5)\n");

    DebugAllocator dbg  = DebugAllocatorInit();
    Allocator     *adbg = ALLOCATOR_OF(&dbg);

    BitVec bv = BitVecInit(adbg);

    // 1011 -- length > 1 and positions % length != 0 so the rotate body
    // (clone + copy-back) actually runs.
    BitVecPush(&bv, true);
    BitVecPush(&bv, false);
    BitVecPush(&bv, true);
    BitVecPush(&bv, true);

    BitVecRotateRight(&bv, 1);

    // Correctness sanity: 1011 rotated right by 1 -> 1101.
    bool ok = (BitVecLen(&bv) == 4);
    ok      = ok && (BitVecGet(&bv, 0) == true);
    ok      = ok && (BitVecGet(&bv, 1) == true);
    ok      = ok && (BitVecGet(&bv, 2) == false);
    ok      = ok && (BitVecGet(&bv, 3) == true);

    BitVecDeinit(&bv);

    // Real code frees the temp clone inside rotate; with the Deinit
    // removed it survives as a live allocation after bv is torn down.
    ok = ok && (DebugAllocatorLiveCount(&dbg) == 0);

    DebugAllocatorDeinit(&dbg);
    return ok;
}

int main(void) {
    TestFunction tests[] = {
        test_rotate_right_frees_temp_clone,
    };
    TestFunction deadend_tests[] = {0};
    (void)deadend_tests;
    return run_test_suite(tests, (int)(sizeof(tests) / sizeof(tests[0])), NULL, 0, "BitVec.Leak");
}
