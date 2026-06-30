#include <Misra/Std/Allocator/Default.h>
#include <Misra/Std/Zstr.h>
#include <Misra/Std/Container/Float.h>
#include <Misra/Std/Container/Int.h>
#include <Misra/Std/Log.h>

#include "../../Util/TestRunner.h"

bool test_float_init(void);
bool test_float_clear(void);
bool test_float_clone(void);
bool test_float_clone_inherits_allocator_config(void);

bool test_float_init(void) {
    WriteFmt("Testing FloatInit\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Float value = FloatInit(&alloc.base);

    bool result = FloatIsZero(&value);
    result      = result && !FloatIsNegative(&value);
    result      = result && (FloatExponent(&value) == 0);

    FloatDeinit(&value);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

bool test_float_clear(void) {
    WriteFmt("Testing FloatClear\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Float value = FloatFromStr("-123.45", &alloc.base);

    FloatClear(&value);

    bool result = FloatIsZero(&value);
    result      = result && !FloatIsNegative(&value);
    result      = result && (FloatExponent(&value) == 0);

    FloatDeinit(&value);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

bool test_float_clone(void) {
    WriteFmt("Testing FloatClone\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Float original = FloatFromStr("-12.5", &alloc.base);
    Float clone    = FloatClone(&original);
    Float expected = FloatFromStr("-12.5", &alloc.base);
    Str   text     = FloatToStr(&clone);

    FloatAbs(&original);

    bool result = FloatEQ(&clone, &expected);
    result      = result && (ZstrCompare(StrBegin(&text), "-12.5") == 0);
    result      = result && !FloatEQ(&clone, &original);

    StrDeinit(&text);
    FloatDeinit(&original);
    FloatDeinit(&clone);
    FloatDeinit(&expected);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

bool test_float_clone_inherits_allocator_config(void) {
    WriteFmt("Testing FloatClone allocator inheritance\n");

    DefaultAllocator alloc = DefaultAllocatorInit();
    // intentional bypass: no public setter on `Allocator` for effort /
    // retry_limit -- pre-seeded directly so the inheritance path below
    // can be observed end-to-end.
    alloc.base.effort      = ALLOCATOR_EFFORT_RETRY_FALLBACK;
    alloc.base.retry_limit = 6;

    // "-0.005" normalises to (negative=true, significand=5, exponent=-3),
    // a non-trivial three-bit magnitude that exercises the clone-vs-original
    // equality check below.
    Float original = FloatFromStr("-0.005", &alloc.base);

    Float clone = FloatClone(&original);

    bool result = FloatEQ(&clone, &original) && FloatAllocator(&clone) == FloatAllocator(&original) &&
                  FloatAllocator(&clone)->allocate == FloatAllocator(&original)->allocate &&
                  FloatAllocator(&clone)->remap == FloatAllocator(&original)->remap &&
                  FloatAllocator(&clone)->deallocate == FloatAllocator(&original)->deallocate &&
                  FloatAllocator(&clone)->effort == FloatAllocator(&original)->effort &&
                  FloatAllocator(&clone)->retry_limit == FloatAllocator(&original)->retry_limit;

    FloatDeinit(&original);
    FloatDeinit(&clone);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// float_normalize zero branch: `value->exponent = 0` (line 255). Multiplying a
// zero by a value with a non-zero exponent (3e5) yields a pre-normalize zero
// significand carrying exponent 5; normalize must reset the exponent to 0.
// The assign-const mutant sets it to 42 -> FloatExponent != 0 -> killed.
static bool test_m10_normalize_zero_exponent(void) {
    WriteFmt("Testing float_normalize canonicalises zero exponent\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Float zero = FloatFromStr("0", &alloc.base);
    Float big  = FloatFromStr("3e5", &alloc.base);
    Float r    = FloatInit(&alloc.base);

    bool ok = FloatMul(&r, &zero, &big);
    ok      = ok && FloatIsZero(&r);
    ok      = ok && (FloatExponent(&r) == 0);

    FloatDeinit(&zero);
    FloatDeinit(&big);
    FloatDeinit(&r);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// Mutant 254:25 cxx_assign_const in float_normalize:
//   `value->negative = false;` (canonical-zero branch) -> `value->negative = 42;`
//
// A Float that reaches float_normalize with a zero significand must come out
// as the canonical positive zero. FloatIsNegative() and FloatToStr() both mask
// the sign of a zero, but float_hash() reads ->negative directly (sign byte),
// so a "negative zero" hashes differently from a canonical zero.
//
// FloatFromStr("0") routes through float_normalize's zero branch (the mutated
// store). FloatInit() builds a canonical zero WITHOUT touching float_normalize
// (its negative=false comes from the struct initializer). On real code both
// are the canonical positive zero and hash identically. Under the mutant the
// "0" string path carries negative=42 (truthy) into the sign byte, so its hash
// diverges from the FloatInit zero -> test fails -> mutant killed.
bool test_ff_254_normalized_zero_sign_is_positive(void) {
    WriteFmt("Testing float_normalize canonical-zero sign (mutant 254)\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Float normalized_zero = FloatFromStr("0", &alloc.base);
    Float canonical_zero  = FloatInit(&alloc.base);

    // Sanity: both are zero.
    bool result = FloatIsZero(&normalized_zero) && FloatIsZero(&canonical_zero);

    // The normalized zero must hash identically to the canonical positive zero.
    // Real code: equal. Mutant (negative=42): sign byte differs -> not equal.
    u64 h_norm = float_hash(&normalized_zero, 0);
    u64 h_can  = float_hash(&canonical_zero, 0);
    result     = result && (h_norm == h_can);

    FloatDeinit(&normalized_zero);
    FloatDeinit(&canonical_zero);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// PROBE for equivalent mutant 412:9:
//   `IntDeinit(&temp);` removed in the FloatIsNegative early-return branch.
// `temp` is a freshly IntInit'd Int with no heap yet; dropping its deinit is a
// (potential) leak only, and the test allocator does not abort on leak. The
// observable contract -- FloatToInt returns false for a negative input -- is
// unchanged. This probe keeps passing under the emulated removal.
bool test_ff_probe_to_int_negative(void) {
    WriteFmt("Probe: FloatToInt returns false for negative input\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Float neg = FloatFromStr("-5", &alloc.base);
    Int   out = IntInit(&alloc.base);

    bool ok = (FloatToInt(&out, &neg) == false);

    IntDeinit(&out);
    FloatDeinit(&neg);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// Build an UN-normalized but structurally valid Float directly from struct
// fields. ValidateFloat only checks the significand is a valid Int, so a
// significand with trailing zeros and a negative exponent is a legitimate
// caller-observable input (e.g. 50 x 10^-1 == 5). This is the only way to
// reach the exact-division success path of FloatToInt's negative-exponent
// branch, since FloatFromStr/arith always normalize trailing zeros away.

// Kills line 439 init_const (places = (u64)(-value->exponent) -> 42):
// real places == 1 so factor == 10 and 50/10 == 5 divides exactly -> true;
// mutant places == 42 so factor == 10^42 and 50/10^42 is not exact ->
// int_div_exact fails and FloatToInt returns false. Result and return value
// are both caller-observable, so the mutation is distinguished.
bool test_m2_to_int_neg_exponent_exact(void) {
    WriteFmt("Testing FloatToInt on un-normalized 50e-1 (== 5)\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Float value = {
        .negative    = false,
        .significand = IntFromStr("50", &alloc.base),
        .exponent    = -1,
    };
    Int result = IntInit(&alloc.base);

    bool ok = FloatToInt(&result, &value);

    Str  text  = IntToStr(&result);
    bool right = ok && (ZstrCompare(StrBegin(&text), "5") == 0);

    StrDeinit(&text);
    IntDeinit(&result);
    IntDeinit(&value.significand);
    DefaultAllocatorDeinit(&alloc);
    return right;
}

// Exercises the non-negative-exponent branch: 12 x 10^1 == 120.
bool test_m2_to_int_integer_branch(void) {
    WriteFmt("Testing FloatToInt on 120 (exponent branch)\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Float value  = FloatFromStr("120", &alloc.base);
    Int   result = IntInit(&alloc.base);

    bool ok = FloatToInt(&result, &value);

    Str  text  = IntToStr(&result);
    bool right = ok && (ZstrCompare(StrBegin(&text), "120") == 0);

    StrDeinit(&text);
    IntDeinit(&result);
    FloatDeinit(&value);
    DefaultAllocatorDeinit(&alloc);
    return right;
}

// Exercises the zero branch: result must be 0 and the call must succeed.
bool test_m2_to_int_zero(void) {
    WriteFmt("Testing FloatToInt on 0\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Float value  = FloatFromStr("0", &alloc.base);
    Int   result = IntFromStr("999", &alloc.base);

    bool ok    = FloatToInt(&result, &value);
    bool right = ok && IntIsZero(&result);

    IntDeinit(&result);
    FloatDeinit(&value);
    DefaultAllocatorDeinit(&alloc);
    return right;
}

// Exercises the negative branch: FloatToInt rejects negative inputs.
bool test_m2_to_int_negative_rejected(void) {
    WriteFmt("Testing FloatToInt rejects -7\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Float value  = FloatFromStr("-7", &alloc.base);
    Int   result = IntInit(&alloc.base);

    bool ok    = FloatToInt(&result, &value);
    bool right = (ok == false);

    IntDeinit(&result);
    FloatDeinit(&value);
    DefaultAllocatorDeinit(&alloc);
    return right;
}

// Kills line 511 cxx_eq_to_ne on the second comparison: `ch == 'E'` mutated to
// `ch != 'E'`. With an uppercase 'E' exponent marker the real code enters the
// exponent branch; the mutant fails to recognise 'E' and rejects the string.
bool test_m3_uppercase_exponent_marker(void) {
    WriteFmt("Testing FloatTryFromStr handles uppercase 'E' exponent\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Float f    = FloatInit(&alloc.base);
    bool  ok   = FloatTryFromStr(&f, "1E2");
    Str   text = StrInit(&alloc.base);

    text = FloatToStr(&f);

    bool result = ok && (ZstrCompare(StrBegin(&text), "100") == 0);

    StrDeinit(&text);
    FloatDeinit(&f);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Kills line 560 cxx_lt_to_le: `explicit_exp < INT64_MIN + fractional` mutated
// to `<=`. With no fractional digits and an exponent of exactly INT64_MIN the
// real guard is false (the value is representable) so the parse succeeds; the
// mutant's `<=` makes the boundary value fail. We pin both success and the
// exact resulting exponent.
bool test_m3_min_exponent_boundary(void) {
    WriteFmt("Testing FloatTryFromStr accepts the INT64_MIN exponent boundary\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Float     f      = FloatInit(&alloc.base);
    bool      ok     = FloatTryFromStr(&f, "1e-9223372036854775808");
    const i64 minexp = (i64)(-9223372036854775807LL - 1);

    bool result = ok && (FloatExponent(&f) == minexp);

    FloatDeinit(&f);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// float_try_from_ieee_bits, binexp > 0 branch (L70/L74): an integer-valued
// double with a positive binary exponent must yield decimal exponent 0 and
// round-trip exactly. 2^60 = 1152921504606846976 is exactly representable;
// its significand (2^60) is not divisible by 10, so normalize leaves the
// exponent at 0. The cxx_assign_const mutation `out->exponent = 0` -> 42
// would make the value 2^60 * 10^42, breaking both the exponent and the
// printed string.
bool test_m5_from_double_large_integer_exponent_zero(void) {
    WriteFmt("Testing FloatFrom(double) of a large integer keeps exponent 0\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    double two_pow_60 = 1152921504606846976.0; // 2^60, exact in f64
    Float  value      = FloatFrom(two_pow_60, &alloc.base);
    Str    text       = FloatToStr(&value);

    bool result = (FloatExponent(&value) == 0);
    result      = result && (ZstrCompare(StrBegin(&text), "1152921504606846976") == 0);

    StrDeinit(&text);
    FloatDeinit(&value);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Wide exponent gap forces a real multi-place rescale: 1e2 + 1e-1 = 100.1.
static bool test_m7_scale_add_exponent_alignment(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    Float a = FloatFromStr("100", &alloc.base); // exp 2
    Float b = FloatFromStr("0.1", &alloc.base); // exp -1
    Float r = FloatInit(&alloc.base);
    Str   t = StrInit(&alloc.base);

    FloatAdd(&r, &a, &b);
    t       = FloatToStr(&r);
    bool ok = ZstrCompare(StrBegin(&t), "100.1") == 0;

    StrDeinit(&t);
    FloatDeinit(&a);
    FloatDeinit(&b);
    FloatDeinit(&r);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// Adding zero with a different exponent must return the other operand exactly
// (exercises the FloatIsZero scale branch without disturbing magnitude).
static bool test_m7_scale_add_zero_keeps_value(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    Float a = FloatFromStr("12.5", &alloc.base); // exp -1
    Float z = FloatFromStr("0", &alloc.base);    // exp 0, zero
    Float r = FloatInit(&alloc.base);
    Str   t = StrInit(&alloc.base);

    FloatAdd(&r, &a, &z);
    t       = FloatToStr(&r);
    bool ok = ZstrCompare(StrBegin(&t), "12.5") == 0;

    StrDeinit(&t);
    FloatDeinit(&a);
    FloatDeinit(&z);
    FloatDeinit(&r);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

int main(void) {
    WriteFmt("[INFO] Starting Float.Type tests\n\n");

    TestFunction tests[] = {
        test_float_init,
        test_float_clear,
        test_float_clone,
        test_float_clone_inherits_allocator_config,
        test_m10_normalize_zero_exponent,
        test_ff_254_normalized_zero_sign_is_positive,
        test_ff_probe_to_int_negative,
        test_m2_to_int_neg_exponent_exact,
        test_m2_to_int_integer_branch,
        test_m2_to_int_zero,
        test_m2_to_int_negative_rejected,
        test_m3_uppercase_exponent_marker,
        test_m3_min_exponent_boundary,
        test_m5_from_double_large_integer_exponent_zero,
        test_m7_scale_add_exponent_alignment,
        test_m7_scale_add_zero_keeps_value,
    };

    int total_tests = sizeof(tests) / sizeof(tests[0]);
    return run_test_suite(tests, total_tests, NULL, 0, "Float.Type");
}
