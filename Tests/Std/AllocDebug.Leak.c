/// file : tests/std/allocdebug.leak.c
///
/// Leak-guard tests for the DebugAllocator implementation itself.
///
/// The usual leak-kill idiom -- route a module's allocations through an
/// explicit DebugAllocator and assert DebugAllocatorLiveCount(&dbg) == 0 after
/// cleanup -- does not apply here, because the module under test IS the
/// DebugAllocator. Its internal teardown frees DebugAllocator-private storage
/// (the live Map, the freed Vec) through its embedded `meta` HeapAllocator, and
/// the embedded `meta` / `heap` HeapAllocators themselves through
/// `HeapAllocatorDeinit`. None of those backing allocators can be swapped for an
/// observer (they are owned inline, by value, with no config knob), and
/// `DebugAllocatorDeinit` ends with an unconditional `MemSet(self, 0, ...)` that
/// zeroes every embedded allocator's bookkeeping AFTER the teardown calls run --
/// so a removed internal *Deinit leaves no in-process accounting to read back.
///
/// Every leak-flavoured survivor in this source is therefore bucket-B
/// (non-observable); they are recorded in Conventions/mull-ignores.toml rather
/// than killed by a test here. See the per-id rationale in that ledger. This
/// file stays an empty skeleton on purpose: any test added here would assert
/// nothing the removed line changes, over-fitting the suite.

#include <Misra.h>
#include <Misra/Std/Allocator/Debug.h>

#include "../Util/TestRunner.h"

int main(void) {
    TestFunction tests[]         = {0};
    TestFunction deadend_tests[] = {0};
    (void)tests;
    (void)deadend_tests;
    return run_test_suite(tests, 0, deadend_tests, 0, "AllocDebug.Leak");
}
