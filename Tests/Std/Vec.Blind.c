#include <Misra.h>
#include <Misra/Std/Allocator/Debug.h>
#include <Misra/Std/Allocator/Default.h>
#include <Misra/Std/Container/Vec.h>
#include <Misra/Std/Log.h>
#include "../Util/TestRunner.h"

// ===========================================================================
// Blind-spot mutation-hardening suite for Source/Misra/Std/Container/Vec.c
//
// Targets six surviving mutants not covered by the behavioural tests:
//   205:10 cxx_replace_scalar_call  (clone_vec reserve result)   -> EQUIVALENT
//   261:66 cxx_mul_to_div           (insert failure prezero)      -> EQUIVALENT
//   268:78 cxx_mul_to_div           (insert failure tail-zero)    -> DETECTED
//   344:81 cxx_mul_to_div           (fast-insert failure tail)    -> DETECTED
//   395:22 cxx_sub_to_add           (remove compaction size, t1)  -> DETECTED
//   395:30 cxx_sub_to_add           (remove compaction size, t2)  -> DETECTED
// ===========================================================================

static DefaultAllocator alloc;

// 8-byte element so element stride is unambiguous (aligned_size == 8 under the
// default allocator, alignment 1) and over-moved / un-zeroed bytes are visible.
typedef struct {
    u64 value;
} Elem;

typedef Vec(Elem) ElemVec;

// copy_init fixture: succeeds and copies the value, except on the call whose
// 1-based index equals g_fail_at (0 disables failure).
static size g_init_calls = 0;
static size g_fail_at    = 0;

static bool elem_copy_init(void *dst_, const void *src_, const Allocator *a) {
    (void)a;
    g_init_calls++;
    if (g_fail_at != 0 && g_init_calls == g_fail_at) {
        return false;
    }
    ((Elem *)dst_)->value = ((const Elem *)src_)->value;
    return true;
}

static void elem_copy_deinit(void *p_, const Allocator *a) {
    (void)a;
    ((Elem *)p_)->value = 0;
}

// Prototypes.
bool test_insert_fail_tail_zeroed(void);
bool test_fast_insert_fail_tail_zeroed(void);
bool test_remove_compaction_size_first_term(void);
bool test_remove_compaction_size_second_term(void);

// ---- 268:78 cxx_mul_to_div ------------------------------------------------
// insert_range_into_vec failure cleanup. On a copy_init failure during a middle
// insert (idx < length), the originals are shifted right (pre-loop), then on
// failure shifted back, leaving stale copies of the tail in the top `count`
// slots [length, length+count). Real code zeros that region with
// `count * aligned_size`. The mutant `count / aligned_size` (8 -> 0 bytes for
// count < 8) zeros nothing, so the stale non-zero tail copies survive at
// VecAt(vec, length .. length+count). The vector length is restored, so those
// slots sit one-past-end and are observable.
bool test_insert_fail_tail_zeroed(void) {
    WriteFmt("Testing insert failure zeroes vacated tail slots\n");

    ElemVec vec = VecInitWithDeepCopy(elem_copy_init, elem_copy_deinit, &alloc);

    // Fill 4 live originals (10..13). copy_init must not fail during prefill.
    g_fail_at = 0;
    for (u64 i = 0; i < 4; i++) {
        Elem e = {.value = 10 + i};
        VecPushBackR(&vec, e);
    }

    size length_before = VecLen(&vec); // 4

    // Middle insert at idx 1 of 2 elements; fail on the very first copy_init of
    // this insert so the failure cleanup runs with the originals shifted.
    g_init_calls = 0;
    g_fail_at    = 1;
    Elem add[2]  = {{.value = 1000}, {.value = 2000}};
    bool ok      = VecInsertRangeR(&vec, add, 1, 2);

    bool result = (ok == false) && (VecLen(&vec) == length_before);

    // Originals fully intact after rollback.
    result = result && (VecAt(&vec, 0).value == 10);
    result = result && (VecAt(&vec, 1).value == 11);
    result = result && (VecAt(&vec, 2).value == 12);
    result = result && (VecAt(&vec, 3).value == 13);

    // The two slots one-past-end (the vacated tail) must be zeroed. Under the
    // mutant they retain stale copies of the shifted originals (non-zero).
    result = result && (VecAt(&vec, 4).value == 0);
    result = result && (VecAt(&vec, 5).value == 0);

    g_fail_at = 0; // let VecDeinit run copy_deinit on the live originals
    VecDeinit(&vec);
    return result;
}

// ---- 344:81 cxx_mul_to_div ------------------------------------------------
// insert_range_fast_into_vec failure cleanup. A front/middle insert parks the
// displaced originals at the very end [length+count-displaced, length+count).
// On failure the parked block is moved back to [idx, idx+displaced), leaving
// stale copies in [length, length+count). Real code zeros that region with
// `aligned_size * count`; the mutant `aligned_size / count` (8/count -> 0 for
// count >= 2) leaves the stale parked copies. Observable one-past-end.
bool test_fast_insert_fail_tail_zeroed(void) {
    WriteFmt("Testing fast-insert failure zeroes parked tail slots\n");

    ElemVec vec = VecInitWithDeepCopy(elem_copy_init, elem_copy_deinit, &alloc);

    g_fail_at = 0;
    for (u64 i = 0; i < 4; i++) {
        Elem e = {.value = 100 + i}; // 100..103
        VecPushBackR(&vec, e);
    }

    size length_before = VecLen(&vec); // 4

    // Front insert at idx 0 of 2 elements -> displaced = min(length-idx,count)
    // = min(4,2) = 2, so the displacement MemMove runs. Fail on first copy.
    g_init_calls = 0;
    g_fail_at    = 1;
    Elem add[2]  = {{.value = 7000}, {.value = 8000}};
    bool ok      = VecInsertRangeFastR(&vec, add, 0, 2);

    bool result = (ok == false) && (VecLen(&vec) == length_before);

    // Originals must all still be present (order may differ for the fast path,
    // but a front-insert with displaced==count restores them in place).
    bool found100 = false, found101 = false, found102 = false, found103 = false;
    for (size i = 0; i < VecLen(&vec); i++) {
        u64 v    = VecAt(&vec, i).value;
        found100 = found100 || (v == 100);
        found101 = found101 || (v == 101);
        found102 = found102 || (v == 102);
        found103 = found103 || (v == 103);
    }
    result = result && found100 && found101 && found102 && found103;

    // The two parked slots one-past-end must be zeroed; the mutant leaves
    // stale displaced copies there.
    result = result && (VecAt(&vec, 4).value == 0);
    result = result && (VecAt(&vec, 5).value == 0);

    g_fail_at = 0;
    VecDeinit(&vec);
    return result;
}

// ---- 395:22 cxx_sub_to_add (length - start -> length + start) -------------
// remove_range_vec compacts the surviving tail with a MemMove of
// `(vec->length - start - count) * aligned_size` bytes. The element count is a
// pure size argument: every element inside the new logical length is correct
// regardless of an over-large move (it merely copies extra), and the
// always-zeroed capacity tail makes any in-bounds surplus read back as zero, so
// the corruption is NOT observable as logical data. The only reachable effect
// of `vec->length + start` is a gross over-read/over-write: the move balloons to
// `length + start - count` elements. Sizing the buffer to exactly `length + 1`
// (via VecTryReduceSpace) and removing the final element (start == length-1,
// count == 1 -> real move size 0) makes the mutant relocate ~2*length elements
// from a `length+1`-element allocation, running far past the buffer and
// trapping -- the same overrun-fault detection the behavioural displacement
// over-copy test relies on. Real code moves 0 bytes and returns cleanly.
bool test_remove_compaction_size_first_term(void) {
    WriteFmt("Testing remove compaction size (length - start term)\n");

    typedef Vec(u64) U64Vec;
    U64Vec vec = VecInit(&alloc);

    // Large, tightly-allocated buffer: capacity == length so the only slack is
    // the single sentinel slot. A few-element overrun would hide in heap slack;
    // a ~2*length-element overrun runs far past the allocation and faults.
    const size N = 100000;
    for (u64 i = 0; i < N; i++) {
        VecPushBackR(&vec, i + 1); // all non-zero
    }
    VecTryReduceSpace(&vec);       // capacity := length, buffer := length + 1

    size old_len = VecLen(&vec);

    // Remove the final element. Real move size = length - (length-1) - 1 = 0.
    // Mutant size = length + (length-1) - 1 = 2*length - 2 elements -> overrun.
    VecDeleteRange(&vec, old_len - 1, 1);

    bool result = (VecLen(&vec) == old_len - 1);
    // Surviving prefix is intact (the real path moved nothing).
    result = result && (VecAt(&vec, 0) == 1);
    result = result && (VecAt(&vec, old_len - 2) == (u64)(old_len - 1));

    VecDeinit(&vec);
    return result;
}

// ---- 395:30 cxx_sub_to_add ((..) - count -> (..) + count) -----------------
// Same MemMove element count; flipping the second `- count` to `+ count` blows
// the move up to `length - start + count` elements. As above the in-bounds
// surplus is unobservable (zeroed capacity tail), so detection is via the gross
// over-read/over-write. Removing `length - 1` elements from the front
// (start == 0, count == length-1 -> real move size 1) makes the mutant relocate
// `length - 0 + (length-1)` = 2*length - 1 elements from a `length+1`-element
// buffer -> far overrun and trap. Real code moves exactly the one surviving
// tail element and compacts correctly.
bool test_remove_compaction_size_second_term(void) {
    WriteFmt("Testing remove compaction size (- count term)\n");

    typedef Vec(u64) U64Vec;
    U64Vec vec = VecInit(&alloc);

    const size N = 100000;
    for (u64 i = 0; i < N; i++) {
        VecPushBackR(&vec, i + 1); // all non-zero
    }
    VecTryReduceSpace(&vec);       // capacity := length, buffer := length + 1

    size old_len = VecLen(&vec);

    // Remove all but the last element from the front. Real move size =
    // length - 0 - (length-1) = 1. Mutant size = length - 0 + (length-1) =
    // 2*length - 1 elements -> overrun far past the buffer.
    VecDeleteRange(&vec, 0, old_len - 1);

    bool result = (VecLen(&vec) == 1);
    // The single survivor is the original last element.
    result = result && (VecAt(&vec, 0) == (u64)old_len);

    VecDeinit(&vec);
    return result;
}

int main(void) {
    alloc = DefaultAllocatorInit();

    TestFunction tests[] = {
        test_insert_fail_tail_zeroed,
        test_fast_insert_fail_tail_zeroed,
        test_remove_compaction_size_first_term,
        test_remove_compaction_size_second_term,
    };
    int rc = run_test_suite(tests, sizeof(tests) / sizeof(tests[0]), NULL, 0, "Vec.Blind");
    DefaultAllocatorDeinit(&alloc);
    return rc;
}
