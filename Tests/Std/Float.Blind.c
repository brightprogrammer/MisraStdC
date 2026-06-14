#include <Misra.h>
#include <Misra/Std/Allocator/Debug.h>
#include <Misra/Std/Allocator/Default.h>
#include <Misra/Std/Zstr.h>
#include <Misra/Std/Container/Float.h>
#include <Misra/Std/Container/Int.h>
#include <Misra/Std/Log.h>

#include "../Util/TestRunner.h"

// ---------------------------------------------------------------------------
// Float blind-survivor kills. These target the value/branch mutants that the
// behavioural Float.{Type,Access,Compare,Convert,Math} suites leave alive.
// Every kill drives an input whose observable result differs between the real
// code and the mutant; the equivalent survivors are documented in the worker
// report (alloc-failure-only Deinit branches, dead stores, bool-normalised
// assignments, and min-selection ties).
// ---------------------------------------------------------------------------

// Kills 93:23 cxx_assign_const. In float_try_from_ieee_bits the final `else`
// (binexp == 0) sets `out->exponent = 0`. The IEEE value 2^52 = 4503599627370496
// decodes to mantissa = 2^52, e = 1075, binexp = 1075 - 1023 - 52 = 0, so it
// lands in exactly that branch. Real exponent is 0 (the integer 2^52, whose
// trailing digit 6 means normalize does not move the exponent). The mutant sets
// exponent = 42, i.e. 2^52 * 10^42 -- a wildly different value. Comparing the
// constructed Float against the Int 2^52 distinguishes them.
static bool test_blind_93_ieee_binexp_zero_exponent(void) {
    WriteFmt("Testing float_try_from_ieee_bits binexp==0 exponent (2^52)\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Float value = FloatFrom(4503599627370496.0, &alloc.base); // 2^52, binexp == 0
    Int   whole = IntFromStr("4503599627370496", &alloc.base);

    bool result = (FloatExponent(&value) == 0);
    result      = result && (FloatCompare(&value, &whole) == 0);

    IntDeinit(&whole);
    FloatDeinit(&value);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Companion render check for 93:23: the same 2^52 value prints as its exact
// integer form. Under the exponent=42 mutant it would gain 42 trailing zeros.
static bool test_blind_93_ieee_binexp_zero_render(void) {
    WriteFmt("Testing float_try_from_ieee_bits binexp==0 render (2^52)\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Float value = FloatFrom(4503599627370496.0, &alloc.base);
    Str   text  = FloatToStr(&value);

    bool result = (ZstrCompare(StrBegin(&text), "4503599627370496") == 0) && (StrLen(&text) == 16);

    StrDeinit(&text);
    FloatDeinit(&value);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Kills 746:14 cxx_xor_assign_to_or_assign. float_hash mixes each exponent byte
// with `hash ^= byte`. Swapping `^=` to `|=` only ever sets bits, so two values
// whose exponents differ but whose mixed states OR to the same word collide.
// "1" (significand 1, exponent 0) and "1e2" (significand 1, exponent 2) hash
// apart under the real xor mixing but collide under the `|=` mutant.
static bool test_blind_746_hash_exponent_xor(void) {
    WriteFmt("Testing float_hash exponent xor mixing (1 vs 1e2)\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Float a = FloatFromStr("1", &alloc.base);   // significand 1, exponent 0
    Float b = FloatFromStr("1e2", &alloc.base); // significand 1, exponent 2

    // Construction guard: identical significand, exponents differ only in low
    // byte (0 vs 2) -- the only hash input that changes is an exponent byte.
    bool built_ok = (FloatExponent(&a) == 0) && (FloatExponent(&b) == 2);
    bool distinct = float_hash(&a, 0) != float_hash(&b, 0);

    FloatDeinit(&a);
    FloatDeinit(&b);
    DefaultAllocatorDeinit(&alloc);
    return built_ok && distinct;
}

// Kills 750:10 cxx_xor_assign_to_or_assign. The sign byte is folded with
// `hash ^= sign`. Under `|=` a positive value (sign 0) and its negation
// (sign 1) collapse to the same hash whenever the pre-sign hash already has its
// low bit set. "3.14" and "-3.14" hash apart under the real xor but collide
// under the `|=` mutant.
static bool test_blind_750_hash_sign_xor(void) {
    WriteFmt("Testing float_hash sign xor mixing (3.14 vs -3.14)\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Float pos = FloatFromStr("3.14", &alloc.base);
    Float neg = FloatFromStr("-3.14", &alloc.base);

    bool distinct = float_hash(&pos, 0) != float_hash(&neg, 0);

    FloatDeinit(&pos);
    FloatDeinit(&neg);
    DefaultAllocatorDeinit(&alloc);
    return distinct;
}

// Second sign-xor pair to keep the 750 kill robust: integers 2 and -2 also
// collide under the `|=` mutant but differ under real.
static bool test_blind_750_hash_sign_xor_int(void) {
    WriteFmt("Testing float_hash sign xor mixing (2 vs -2)\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Float pos = FloatFromStr("2", &alloc.base);
    Float neg = FloatFromStr("-2", &alloc.base);

    bool distinct = float_hash(&pos, 0) != float_hash(&neg, 0);

    FloatDeinit(&pos);
    FloatDeinit(&neg);
    DefaultAllocatorDeinit(&alloc);
    return distinct;
}

// Kills 745:23 cxx_lt_to_le. The exponent-mixing loop runs `for (i = 0;
// i < sizeof(exp); i++)`. Swapping `<` to `<=` runs one extra iteration that
// re-mixes a byte and perturbs every hash, so no input is invariant. We pin the
// absolute hash of "1" to the real constant; the mutant produces a different
// value. The constant was computed by linking the unmutated float_hash against
// the built library and reading the result.
static bool test_blind_745_hash_loop_bound(void) {
    WriteFmt("Testing float_hash exponent loop bound (hash of 1)\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Float one = FloatFromStr("1", &alloc.base);

    // Real float_hash(1) == 8811625884996682770; the i<=sizeof mutant yields
    // 12078381013977967254 (one extra exponent-byte mixing round).
    bool ok = float_hash(&one, 0) == 8811625884996682770ULL;

    FloatDeinit(&one);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// Companion absolute-hash pin for 745:23 on a different value ("0") to keep the
// kill from depending on a single magic constant.
static bool test_blind_745_hash_loop_bound_zero(void) {
    WriteFmt("Testing float_hash exponent loop bound (hash of 0)\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Float zero = FloatFromStr("0", &alloc.base);

    // Real float_hash(0) == 16357866887873635513; the i<=sizeof mutant yields
    // 10587487812035896923.
    bool ok = float_hash(&zero, 0) == 16357866887873635513ULL;

    FloatDeinit(&zero);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

int main(void) {
    WriteFmt("[INFO] Starting Float.Blind tests\n\n");

    TestFunction tests[] = {
        test_blind_93_ieee_binexp_zero_exponent,
        test_blind_93_ieee_binexp_zero_render,
        test_blind_746_hash_exponent_xor,
        test_blind_750_hash_sign_xor,
        test_blind_750_hash_sign_xor_int,
        test_blind_745_hash_loop_bound,
        test_blind_745_hash_loop_bound_zero,
    };

    TestFunction deadend_tests[1] = {0};

    int total_tests         = sizeof(tests) / sizeof(tests[0]);
    int total_deadend_tests = 0;

    return run_test_suite(tests, total_tests, deadend_tests, total_deadend_tests, "Float.Blind");
}
