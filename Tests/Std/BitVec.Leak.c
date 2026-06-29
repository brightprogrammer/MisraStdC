/// file : tests/std/bitvec.leak.c
/// Leak tests for BitVec: kill a reachable mutation survivor by routing
/// every allocation through an explicit DebugAllocator and asserting the
/// live count is zero after teardown. A dropped internal *Deinit leaves a
/// live allocation, so the count assertion fails -> the mutant is killed.
/// Distinct from existing BitVec.* tests (which use DefaultAllocator and
/// never observe a leak).
#include <Misra.h>
#include <Misra/Std/Allocator/Debug.h>
#include <Misra/Std/Allocator/Default.h>
#include <Misra/Std/Container/BitVec.h>
#include <Misra/Std/Container/Str.h>

#include "../Util/TestRunner.h"

// Lean DebugAllocator config -- leak detection only, no trace capture, to
// keep the leak tests fast.
static DebugAllocatorConfig lean_dbg_cfg(void) {
    return (DebugAllocatorConfig) {.capture_traces = false, .detect_overflow = false, .track_freed_history = false};
}

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

// ---- 1131:5 cxx_remove_void_call ----------------------------------------
// BitVecRotateLeft clones the source into a temp BitVec (line 1119) and
// BitVecDeinit's it at line 1131 on the success path before returning.
// Removing that Deinit leaks the clone's backing on EVERY successful
// rotate-left (mirror of the already-covered RotateRight at 1157, which
// this does NOT cover). Init through a DebugAllocator (the clone inherits
// it), rotate, and require the live count back to zero after teardown.
bool test_rotate_left_frees_temp_clone(void);
bool test_rotate_left_frees_temp_clone(void) {
    WriteFmt("Testing BitVecRotateLeft frees its temp clone (1131:5)\n");

    DebugAllocator dbg  = DebugAllocatorInitWith(lean_dbg_cfg());
    Allocator     *adbg = ALLOCATOR_OF(&dbg);

    BitVec bv = BitVecInit(adbg);

    // length > 1 and positions % length != 0 so the rotate body
    // (clone + copy-back) actually runs.
    BitVecPush(&bv, true);
    BitVecPush(&bv, false);
    BitVecPush(&bv, true);
    BitVecPush(&bv, true);

    BitVecRotateLeft(&bv, 2);

    // Correctness sanity: 1011 rotated left by 2 -> 1110.
    bool ok = (BitVecLen(&bv) == 4);
    ok      = ok && (BitVecGet(&bv, 0) == true);
    ok      = ok && (BitVecGet(&bv, 1) == true);
    ok      = ok && (BitVecGet(&bv, 2) == true);
    ok      = ok && (BitVecGet(&bv, 3) == false);

    BitVecDeinit(&bv);

    // Real code frees the temp clone inside rotate; with the Deinit
    // removed it survives as a live allocation after bv is torn down.
    ok = ok && (DebugAllocatorLiveCount(&dbg) == 0);

    DebugAllocatorDeinit(&dbg);
    return ok;
}

// ---- 1866:5 cxx_remove_void_call ----------------------------------------
// bitvec_regex_match_zstr renders the bitvec into a fresh Str (bv_str) and
// StrDeinit's it at line 1866 before returning. Removing that Deinit leaks
// the rendered string on EVERY call. bv_str borrows bv's allocator, so a
// DebugAllocator-backed bitvec exposes the leak as a non-zero live count.
bool test_regex_match_zstr_frees_rendered_str(void);
bool test_regex_match_zstr_frees_rendered_str(void) {
    WriteFmt("Testing bitvec_regex_match_zstr frees its rendered Str (1866:5)\n");

    DebugAllocator dbg  = DebugAllocatorInitWith(lean_dbg_cfg());
    Allocator     *adbg = ALLOCATOR_OF(&dbg);

    BitVec bv = BitVecInit(adbg);

    // 101010 so the render is a non-empty Str that must be freed.
    BitVecPush(&bv, true);
    BitVecPush(&bv, false);
    BitVecPush(&bv, true);
    BitVecPush(&bv, false);
    BitVecPush(&bv, true);
    BitVecPush(&bv, false);

    bool matched = BitVecRegexMatch(&bv, "101");
    bool ok      = (matched == true);

    BitVecDeinit(&bv);

    // The rendered bv_str borrows bv's DebugAllocator. Real code frees it;
    // the mutant leaks it, so the live count stays non-zero after teardown.
    ok = ok && (DebugAllocatorLiveCount(&dbg) == 0);

    DebugAllocatorDeinit(&dbg);
    return ok;
}

// ---- 1883:5 cxx_remove_void_call ----------------------------------------
// bitvec_regex_match_str: same as the zstr variant but routed through the
// Str-pattern overload. Removing the StrDeinit(&bv_str) at line 1883 leaks
// the rendered Str. The pattern Str lives on a separate allocator; only
// bv's DebugAllocator is leak-tracked.
bool test_regex_match_str_frees_rendered_str(void);
bool test_regex_match_str_frees_rendered_str(void) {
    WriteFmt("Testing bitvec_regex_match_str frees its rendered Str (1883:5)\n");

    DebugAllocator dbg  = DebugAllocatorInitWith(lean_dbg_cfg());
    Allocator     *adbg = ALLOCATOR_OF(&dbg);

    DefaultAllocator palloc = DefaultAllocatorInit();
    Allocator       *pbase  = ALLOCATOR_OF(&palloc);

    BitVec bv = BitVecInit(adbg);
    BitVecPush(&bv, true);
    BitVecPush(&bv, false);
    BitVecPush(&bv, true);
    BitVecPush(&bv, false);
    BitVecPush(&bv, true);
    BitVecPush(&bv, false);

    // Pattern "101" as a Str on a separate (untracked) allocator.
    Str pattern = StrInit(pbase);
    StrPushBackR(&pattern, '1');
    StrPushBackR(&pattern, '0');
    StrPushBackR(&pattern, '1');

    bool matched = bitvec_regex_match_str(&bv, &pattern);
    bool ok      = (matched == true);

    StrDeinit(&pattern);
    BitVecDeinit(&bv);

    // bv_str (the render of bv) borrows bv's DebugAllocator. Real code
    // frees it; the mutant leaks it.
    ok = ok && (DebugAllocatorLiveCount(&dbg) == 0);

    DefaultAllocatorDeinit(&palloc);
    DebugAllocatorDeinit(&dbg);
    return ok;
}

int main(void) {
    TestFunction tests[] = {
        test_rotate_right_frees_temp_clone,
        test_rotate_left_frees_temp_clone,
        test_regex_match_zstr_frees_rendered_str,
        test_regex_match_str_frees_rendered_str,
    };
    TestFunction deadend_tests[] = {0};
    (void)deadend_tests;
    return run_test_suite(tests, (int)(sizeof(tests) / sizeof(tests[0])), NULL, 0, "BitVec.Leak");
}
