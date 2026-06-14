/// file : tests/std/vec.mut.c
///
/// Targeted mutation-kill tests for Vec: each test drives an input that makes
/// a specific bucket-A survivor (operator swap / scalar-call replacement / leak)
/// produce an observably-wrong result, so the mutant fails the assertion. These
/// were flagged killable by the original campaign but left as ignores; this file
/// converts them to real kills. Distinct contract from the existing Vec.* tests
/// -- do NOT duplicate them.
///
/// Almost every bucket-A proposal in the audit was ALREADY killed by the
/// pre-existing Vec.* mutation-hardening suites (Vec.Insert / Vec.Remove /
/// Vec.Memory / Vec.Init / Vec.Complex / Vec.Access / Vec.Ops). The single
/// genuinely-uncovered bucket-A survivor is the fast (non-preserve) call in
/// vec_insert_range_l (603:32): on the L-form fast path, cxx_replace_scalar_call
/// forces the insert_range_fast_into_vec() return to the truthy literal 42, so a
/// FAILED insert is reported as success. No existing test exercises a failing
/// fast L-insert, so this is converted to a real kill here.

#include <Misra/Std/Allocator/Default.h>
#include <Misra/Std/Container/Vec.h>
#include <Misra/Std/Log.h>
#include <Misra/Types.h>

#include "../Util/TestRunner.h"

static DefaultAllocator alloc;

// An element wide enough to carry a copy_init hook that can be armed to fail.
typedef struct {
    int value;
} MutElem;

typedef Vec(MutElem) MutElemVec;

// copy_init that fails on the very first call of an insert (armed via g_fail).
static bool g_fail = false;

static bool mut_copy_init(void *dst_, const void *src_, const Allocator *a) {
    (void)a;
    if (g_fail) {
        return false;
    }
    MutElem       *dst = (MutElem *)dst_;
    const MutElem *src = (const MutElem *)src_;
    dst->value         = src->value;
    return true;
}

static void mut_copy_deinit(void *p_, const Allocator *a) {
    (void)a;
    ((MutElem *)p_)->value = 0;
}

// ---- 603:32 cxx_replace_scalar_call -------------------------------------
// vec_insert_range_l, fast (preserve_order == false) path:
//   success = ... : insert_range_fast_into_vec(vec, items, item_size, idx, count);
// The mutant replaces the call's return value with the literal 42 (truthy),
// keeping the side effects. On a copy_init FAILURE the real call returns false,
// so VecInsertRangeFastL must report failure; the mutant reports success (42 ->
// true). Drive a fast L-insert whose first copy_init fails and assert the return
// is false.
bool test_insert_range_fast_l_reports_failure(void);
bool test_insert_range_fast_l_reports_failure(void) {
    WriteFmt("Testing VecInsertRangeFastL reports copy_init failure (603:32)\n");

    MutElemVec vec = VecInitWithDeepCopy(mut_copy_init, mut_copy_deinit, &alloc);

    // Arm copy_init to fail, then attempt a fast L-insert into the empty vec.
    g_fail           = true;
    MutElem items[2] = {{.value = 10}, {.value = 20}};
    bool    ok       = VecInsertRangeFastL(&vec, items, 0, 2);
    g_fail           = false;

    // Real: the insert fails, ok == false, nothing landed in the vec.
    bool result = (ok == false) && (VecLen(&vec) == 0);

    VecDeinit(&vec);
    return result;
}

// ---- 584:10 cxx_init_const ----------------------------------------------
// vec_insert_one_l (the single-element L-insert backing VecInsertL):
//   bool success = preserve_order ? insert_range_into_vec(...) :
//                                   insert_range_fast_into_vec(...);
// col 10 is the initialiser of `success`. The mutant rewrites it to the
// literal 42 (truthy) while keeping the ternary's side effects, so a FAILED
// copy_init insert (real success == false) is reported as success. The
// pre-existing 603:32 test covers the range (Many) backend; this covers the
// single-element backend. Drive a one-element preserve-order L-insert whose
// copy_init fails and assert the return is false with nothing landed.
bool test_insert_one_l_reports_failure(void);
bool test_insert_one_l_reports_failure(void) {
    WriteFmt("Testing VecInsertL reports copy_init failure (584:10)\n");

    MutElemVec vec = VecInitWithDeepCopy(mut_copy_init, mut_copy_deinit, &alloc);

    g_fail       = true;
    MutElem item = {.value = 99};
    bool    ok   = VecInsertL(&vec, item, 0);
    g_fail       = false;

    // Real: the insert fails, ok == false, nothing landed in the vec.
    bool result = (ok == false) && (VecLen(&vec) == 0);

    VecDeinit(&vec);
    return result;
}

// ---- 476:14 cxx_ge_to_gt -------------------------------------------------
// swap_vec, lower bound on the FIRST index:
//   if (idx1 >= vec->length || idx2 >= vec->length)  -- the `idx1 >=` becomes
//   `idx1 >`. The existing Vec.Ops test covers the idx2 comparison (col 37) by
//   passing idx1 == 0 (in range); it never exercises idx1's boundary, so the
//   col-14 mutant survives it. Drive idx1 == length (the out-of-bounds sentinel
//   slot) with idx2 == 0 (in range). Real: idx1 >= length aborts. Mutant:
//   idx1 > length is false and idx2 >= length is false, so the guard passes and
//   it swaps an out-of-bounds slot -- so the abort must come from real code.
bool test_swap_idx1_equal_length_aborts(void);
bool test_swap_idx1_equal_length_aborts(void) {
    WriteFmt("Testing swap rejects idx1 == length (476:14)\n");

    typedef Vec(u32) U32Vec;
    U32Vec vec = VecInit(&alloc);
    for (u32 i = 0; i < 3; i++) {
        VecPushBackR(&vec, i);
    }

    VecSwapItems(&vec, 3, 0); // idx1 == length -> must abort

    // Unreachable on real code.
    VecDeinit(&vec);
    return false;
}

int main(void) {
    alloc = DefaultAllocatorInit();

    TestFunction tests[] = {
        test_insert_range_fast_l_reports_failure,
        test_insert_one_l_reports_failure,
    };

    TestFunction deadend_tests[] = {
        test_swap_idx1_equal_length_aborts,
    };

    int total    = sizeof(tests) / sizeof(tests[0]);
    int deadends = sizeof(deadend_tests) / sizeof(deadend_tests[0]);
    int rc       = run_test_suite(tests, total, deadend_tests, deadends, "Vec.Mut");
    DefaultAllocatorDeinit(&alloc);
    return rc;
}
