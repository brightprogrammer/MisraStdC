/// file : tests/std/bitvec.mut.c
/// Targeted mutation-kill tests for BitVec: each test drives an input that makes a
/// specific surviving mutant produce an observably-wrong result. Distinct from the
/// existing BitVec.* tests -- do NOT duplicate.
#include <Misra.h>
#include <Misra/Std/Allocator/Debug.h>
#include <Misra/Std/Container/BitVec.h>
#include <Misra/Std/Container/Str.h>

#include "../Util/TestRunner.h"

// Lean DebugAllocator config -- leak detection only, no trace capture, to
// keep the leak tests fast.
static DebugAllocatorConfig lean_dbg_cfg(void) {
    return (DebugAllocatorConfig) {.capture_traces = false, .detect_overflow = false, .track_freed_history = false};
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

// ---- 1943:23 cxx_and_to_or ----------------------------------------------
// ValidateBitVec memoizes: with MAGIC_VALIDATED_BIT clear it trusts the
// prior structural check and returns early (line 1943-1945). The mutant
// `!(magic | VALIDATED_BIT)` is always false, so it ALWAYS re-runs the
// structural validator. Hand-build a bitvec whose validated bit is CLEAR
// but whose byte_size is too small for its capacity: the real code skips
// the structural check and returns normally; the mutant runs it and aborts
// at the byte_size check. A normal (non-aborting) test therefore passes on
// real code and dies (longjmp) under the mutant.
bool test_validate_memoization_skips_structural(void);
bool test_validate_memoization_skips_structural(void) {
    WriteFmt("Testing ValidateBitVec honours the validated-bit memoization (1943:23)\n");

    BitVec bv = {0};
    // Structurally inconsistent: capacity claims 400 bits but byte_size 1
    // only backs 8 bits. length 0 + data NULL keeps the earlier structural
    // checks (length>capacity, length>0&&!data, data[0] read) inert, so the
    // ONLY check that would fire is the byte_size-vs-capacity one.
    bv.length    = 0;
    bv.capacity  = 400;
    bv.data      = NULL;
    bv.byte_size = 1;
    bv.allocator = NULL;
    // Valid magic with the validated bit CLEAR -> real code returns early.
    bv.__magic = BITVEC_MAGIC;

    // Real: validated bit clear -> early return, no structural check, no abort.
    // Mutant: always runs structural -> 1*8 < 400 -> LOG_FATAL.
    ValidateBitVec(&bv);

    return true; // reached only when no abort occurred (real code)
}

// ---- 1926:22 cxx_gt_to_le (DEADEND) -------------------------------------
// validate_bitvec_structural guards the byte_size check with `capacity > 0`.
// The mutant `capacity <= 0` (i.e. == 0) disables the check for every real
// bitvec (capacity > 0), so a too-small byte_size is no longer caught.
// Hand-build an invalid bitvec WITH the validated bit set so the structural
// validator actually runs: real code aborts at the byte_size check; the
// mutant skips it and returns. Deadend => expects the abort.
bool test_structural_byte_size_check_aborts(void);
bool test_structural_byte_size_check_aborts(void) {
    WriteFmt("Testing validate_bitvec_structural catches an undersized byte_size (1926:22)\n");

    BitVec bv    = {0};
    bv.length    = 0;
    bv.capacity  = 400;
    bv.data      = NULL;
    bv.byte_size = 1; // 1*8 == 8 < 400 -> structurally invalid
    bv.allocator = NULL;
    // Validated bit SET so validate_bitvec_structural runs.
    bv.__magic = BITVEC_MAGIC | MAGIC_VALIDATED_BIT;

    // Real code aborts here; the mutant returns normally.
    ValidateBitVec(&bv);

    return false; // should never reach here on real code
}

int main(void) {
    TestFunction tests[] = {
        test_rotate_left_frees_temp_clone,
        test_regex_match_zstr_frees_rendered_str,
        test_regex_match_str_frees_rendered_str,
        test_validate_memoization_skips_structural,
    };
    TestFunction deadend_tests[] = {
        test_structural_byte_size_check_aborts,
    };
    return run_test_suite(
        tests,
        (int)(sizeof(tests) / sizeof(tests[0])),
        deadend_tests,
        (int)(sizeof(deadend_tests) / sizeof(deadend_tests[0])),
        "BitVec.Mut"
    );
}
