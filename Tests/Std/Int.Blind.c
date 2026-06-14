/// file      : tests/std/int.blind.c
///
/// Mutation-hardening ("blind") investigation for
/// Source/Misra/Std/Container/Int.c.
///
/// Every surviving mutant supplied for this module was analysed and verified
/// (by rebuilding Int.c with that exact mutation applied and re-running an
/// exhaustive probe) to be EQUIVALENT: it cannot change any observable result
/// on any reachable input. The reasons fall into three groups:
///
///   1. Deleted int_normalize(...) calls. `Int` never exposes raw bit-vector
///      length; every observable query (IntBitLength, IntByteLength, IntCompare,
///      hashing, byte export) recomputes the significant-bit count dynamically
///      via int_significant_bits, so trailing non-significant zero bits left
///      behind by a skipped normalize are invisible.
///        Lines: 359, 526, 1170, 1417, 1418.
///
///   2. Deleted IntDeinit/sint_deinit on fault-only or unreachable cleanup
///      paths. These run only after an internal allocation failure (no
///      fault-injection harness exists), or guard a branch that no valid input
///      reaches (e.g. the mid==1 reset in IntRootRem, the no-solution exits in
///      IntModSqrt after the Jacobi guard already returned, the candidate<=2
///      block in IntNextPrime which is dead once value>=2). With no reachable
///      success-path execution, nothing leaks.
///        Lines: 63, 197, 208, 575, 1110, 1232, 1378, 1413, 1819, 2063, 2317,
///               2358, 2620, 2734, 2775, 2898, 2899, 2902, 2903, 2915.
///
///   3. Deleted deinit/replace whose target is always an empty Int (a freshly
///      IntInit'd placeholder holds zero live allocations), or a write that is
///      never read again. Removing them frees/changes nothing observable.
///        Lines: 134, 151, 184, 772, 1012, 2085.
///
///   Plus 1322 (cxx_gt_to_ge in int_pow_u64): turning `if (exponent > 0)` into
///   `>=` only performs one extra, discarded squaring once the exponent has
///   already reached 0; the controlling `while (exponent > 0)` still terminates
///   and the accumulator is unchanged.
///
/// The tests below are correctness-regression assertions over the functions
/// that host these mutants. They all pass on the unmodified code and pin the
/// observable contracts (values + allocator balance) that the analysis relies
/// on; they intentionally do not claim to kill the equivalent mutants.

#include <Misra.h>
#include <Misra/Std/Allocator/Debug.h>
#include <Misra/Std/Zstr.h>
#include <Misra/Std/Container/Int.h>
#include <Misra/Std/Container/Str.h>
#include <Misra/Std/Log.h>
#include <Misra/Types.h>

#include "../Util/TestRunner.h"

static DebugAllocatorConfig blind_cfg(void) {
    return (DebugAllocatorConfig) {.capture_traces = false, .detect_overflow = false, .track_freed_history = false};
}

// Modular inverse computes the right value and balances all allocations.
bool test_blind_mod_inv_correct(void) {
    DebugAllocator dbg   = DebugAllocatorInitWith(blind_cfg());
    Allocator     *alloc = ALLOCATOR_OF(&dbg);

    Int value   = IntFrom(7u, alloc);
    Int modulus = IntFrom(40u, alloc); // 7 * 23 = 161 = 1 (mod 40)
    Int inverse = IntFrom(0u, alloc);

    bool ok = IntModInv(&inverse, &value, &modulus);
    ok      = ok && IntCompare(&inverse, 23u) == 0;

    IntDeinit(&value);
    IntDeinit(&modulus);
    IntDeinit(&inverse);

    ok = ok && DebugAllocatorLiveCount(&dbg) == 0;
    DebugAllocatorDeinit(&dbg);
    return ok;
}

// Tonelli-Shanks square root (modulus % 4 == 1) round-trips and balances.
bool test_blind_mod_sqrt_tonelli(void) {
    DebugAllocator dbg   = DebugAllocatorInitWith(blind_cfg());
    Allocator     *alloc = ALLOCATOR_OF(&dbg);

    Int value   = IntFrom(9u, alloc);
    Int modulus = IntFrom(41u, alloc); // 41-1 = 8*5 -> several Tonelli rounds
    Int root    = IntFrom(0u, alloc);
    Int check   = IntFrom(0u, alloc);

    bool ok = IntModSqrt(&root, &value, &modulus);
    ok      = ok && IntSquareMod(&check, &root, &modulus);
    ok      = ok && IntCompare(&check, 9u) == 0;

    IntDeinit(&value);
    IntDeinit(&modulus);
    IntDeinit(&root);
    IntDeinit(&check);

    ok = ok && DebugAllocatorLiveCount(&dbg) == 0;
    DebugAllocatorDeinit(&dbg);
    return ok;
}

// Square root mod p with p % 4 == 3 (the fast-exponent branch).
bool test_blind_mod_sqrt_fast_branch(void) {
    DebugAllocator dbg   = DebugAllocatorInitWith(blind_cfg());
    Allocator     *alloc = ALLOCATOR_OF(&dbg);

    Int value   = IntFrom(2u, alloc);
    Int modulus = IntFrom(7u, alloc); // 7 % 4 == 3
    Int root    = IntFrom(0u, alloc);
    Int check   = IntFrom(0u, alloc);

    bool ok = IntModSqrt(&root, &value, &modulus);
    ok      = ok && IntSquareMod(&check, &root, &modulus);
    ok      = ok && IntCompare(&check, 2u) == 0;

    IntDeinit(&value);
    IntDeinit(&modulus);
    IntDeinit(&root);
    IntDeinit(&check);

    ok = ok && DebugAllocatorLiveCount(&dbg) == 0;
    DebugAllocatorDeinit(&dbg);
    return ok;
}

// Modular subtraction across its three internal branches.
bool test_blind_mod_sub_branches(void) {
    DebugAllocator dbg   = DebugAllocatorInitWith(blind_cfg());
    Allocator     *alloc = ALLOCATOR_OF(&dbg);

    Int m = IntFrom(7u, alloc);
    Int r = IntFrom(0u, alloc);

    // ar >= br branch: 5 - 2 = 3 (mod 7)
    Int  a1 = IntFrom(5u, alloc), b1 = IntFrom(2u, alloc);
    bool ok = IntModSub(&r, &a1, &b1, &m) && IntCompare(&r, 3u) == 0;
    // ar < br branch: 2 - 5 = -3 = 4 (mod 7)
    Int a2 = IntFrom(2u, alloc), b2 = IntFrom(5u, alloc);
    ok = ok && IntModSub(&r, &a2, &b2, &m) && IntCompare(&r, 4u) == 0;
    // equal branch: 5 - 12 = 0 (mod 7)
    Int a3 = IntFrom(5u, alloc), b3 = IntFrom(12u, alloc);
    ok = ok && IntModSub(&r, &a3, &b3, &m) && IntIsZero(&r);

    IntDeinit(&a1);
    IntDeinit(&b1);
    IntDeinit(&a2);
    IntDeinit(&b2);
    IntDeinit(&a3);
    IntDeinit(&b3);
    IntDeinit(&m);
    IntDeinit(&r);

    ok = ok && DebugAllocatorLiveCount(&dbg) == 0;
    DebugAllocatorDeinit(&dbg);
    return ok;
}

// IntNextPrime returns the next prime and balances allocations.
bool test_blind_next_prime(void) {
    DebugAllocator dbg   = DebugAllocatorInitWith(blind_cfg());
    Allocator     *alloc = ALLOCATOR_OF(&dbg);

    Int value = IntFrom(100u, alloc);
    Int next  = IntFrom(0u, alloc);

    bool ok = IntNextPrime(&next, &value);
    ok      = ok && IntCompare(&next, 101u) == 0;

    IntDeinit(&value);
    IntDeinit(&next);

    ok = ok && DebugAllocatorLiveCount(&dbg) == 0;
    DebugAllocatorDeinit(&dbg);
    return ok;
}

// Carmichael number 561 is composite; Miller-Rabin must not be fooled.
bool test_blind_carmichael_is_composite(void) {
    DebugAllocator dbg   = DebugAllocatorInitWith(blind_cfg());
    Allocator     *alloc = ALLOCATOR_OF(&dbg);

    Int  value = IntFrom(561u, alloc);
    bool error = false;
    bool prime = IntIsProbablePrime(&value, &error);
    bool ok    = (error == false) && (prime == false);

    IntDeinit(&value);

    ok = ok && DebugAllocatorLiveCount(&dbg) == 0;
    DebugAllocatorDeinit(&dbg);
    return ok;
}

// Big base-16 stringification round-trips and balances allocations (exercises
// the to_str_radix division loop that hosts several deleted-deinit mutants).
bool test_blind_to_str_radix_big(void) {
    DebugAllocator dbg   = DebugAllocatorInitWith(blind_cfg());
    Allocator     *alloc = ALLOCATOR_OF(&dbg);

    Int value  = IntFromStr("123456789012345678901234567890", alloc);
    Str hex    = IntToStrRadix(&value, 16, false, alloc);
    Int parsed = IntFromStrRadix(&hex, 16, alloc);

    bool ok = IntCompare(&value, &parsed) == 0;

    StrDeinit(&hex);
    IntDeinit(&value);
    IntDeinit(&parsed);

    ok = ok && DebugAllocatorLiveCount(&dbg) == 0;
    DebugAllocatorDeinit(&dbg);
    return ok;
}

int main(void) {
    TestFunction tests[] = {
        test_blind_mod_inv_correct,
        test_blind_mod_sqrt_tonelli,
        test_blind_mod_sqrt_fast_branch,
        test_blind_mod_sub_branches,
        test_blind_next_prime,
        test_blind_carmichael_is_composite,
        test_blind_to_str_radix_big,
    };
    TestFunction deadend_tests[] = {0};
    (void)deadend_tests;

    return run_test_suite(tests, sizeof(tests) / sizeof(tests[0]), deadend_tests, 0, "Int.Blind");
}
