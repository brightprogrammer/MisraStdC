#include <Misra/Std/Allocator/Default.h>
#include <Misra/Std/Zstr.h>
#include <Misra/Std/Container/Float.h>
#include <Misra/Std/Container/Int.h>
#include <Misra/Std/Log.h>

#include "../Util/FloatTestData.h"
#include "../Util/TestRunner.h"

bool test_float_negate_abs(void);
bool test_float_add_small_small(void);
bool test_float_add_very_large_large(void);
bool test_float_add_generic(void);
bool test_float_sub_small_small(void);
bool test_float_sub_very_large_large(void);
bool test_float_sub_generic(void);
bool test_float_mul_small_small(void);
bool test_float_mul_very_large_small(void);
bool test_float_mul_generic(void);
bool test_float_div_small_small(void);
bool test_float_div_very_large_small(void);
bool test_float_div_generic(void);
bool test_float_div_by_zero(void);

// INT64 half-points chosen so that a sum/difference of two parsed exponents
// lands *exactly* on the i64 limit. This makes the overflow guard's `a`
// comparison fire at the boundary only when mutated `>`->`>=` / `<`->`<=`.
//   4611686018427387903 + 4611686018427387904 ==  9223372036854775807 (INT64_MAX)
//  -4611686018427387904 + -4611686018427387904 == -9223372036854775808 (INT64_MIN)
#define EXP_HALF_LO "4611686018427387903"
#define EXP_HALF_HI "4611686018427387904"

bool test_float_negate_abs(void) {
    WriteFmt("Testing FloatNegate and FloatAbs\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Float value = FloatFromStr("12.5", &alloc.base);
    Str   text  = StrInit(&alloc.base);

    FloatNegate(&value);
    text = FloatToStr(&value);

    bool result = ZstrCompare(StrBegin(&text), "-12.5") == 0;

    StrDeinit(&text);
    FloatAbs(&value);
    text   = FloatToStr(&value);
    result = result && (ZstrCompare(StrBegin(&text), "12.5") == 0);

    StrDeinit(&text);
    FloatDeinit(&value);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

bool test_float_add_small_small(void) {
    WriteFmt("Testing FloatAdd with small floats\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Float a            = FloatFromStr("1.2", &alloc.base);
    Float b            = FloatFromStr("0.03", &alloc.base);
    Float result_value = FloatInit(&alloc.base);
    Str   text         = StrInit(&alloc.base);

    FloatAdd(&result_value, &a, &b);
    text = FloatToStr(&result_value);

    bool result = ZstrCompare(StrBegin(&text), "1.23") == 0;

    StrDeinit(&text);
    FloatDeinit(&a);
    FloatDeinit(&b);
    FloatDeinit(&result_value);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

bool test_float_add_very_large_large(void) {
    WriteFmt("Testing FloatAdd with very large floats\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Float a            = FloatFromStr(FLOAT_TEST_VERY_LARGE_ONES, &alloc.base);
    Float b            = FloatFromStr(FLOAT_TEST_VERY_LARGE_TWOS, &alloc.base);
    Float result_value = FloatInit(&alloc.base);
    Str   text         = StrInit(&alloc.base);

    FloatAdd(&result_value, &a, &b);
    text = FloatToStr(&result_value);

    bool result = ZstrCompare(StrBegin(&text), FLOAT_TEST_VERY_LARGE_THREES) == 0;

    StrDeinit(&text);
    FloatDeinit(&a);
    FloatDeinit(&b);
    FloatDeinit(&result_value);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

bool test_float_add_generic(void) {
    WriteFmt("Testing FloatAdd generic dispatch\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Float a            = FloatFromStr("1.25", &alloc.base);
    Float b            = FloatFromStr("0.75", &alloc.base);
    Int   whole        = IntFrom(2, &alloc.base);
    Float result_value = FloatInit(&alloc.base);
    Str   text         = StrInit(&alloc.base);

    FloatAdd(&result_value, &a, &b);
    text        = FloatToStr(&result_value);
    bool result = ZstrCompare(StrBegin(&text), "2") == 0;

    StrDeinit(&text);
    FloatAdd(&result_value, &a, &whole);
    text   = FloatToStr(&result_value);
    result = result && (ZstrCompare(StrBegin(&text), "3.25") == 0);

    StrDeinit(&text);
    FloatAdd(&result_value, &a, 2u);
    text   = FloatToStr(&result_value);
    result = result && (ZstrCompare(StrBegin(&text), "3.25") == 0);

    StrDeinit(&text);
    FloatAdd(&result_value, &a, -1);
    text   = FloatToStr(&result_value);
    result = result && (ZstrCompare(StrBegin(&text), "0.25") == 0);

    StrDeinit(&text);
    FloatAdd(&result_value, &a, 0.75f);
    text   = FloatToStr(&result_value);
    result = result && (ZstrCompare(StrBegin(&text), "2") == 0);

    StrDeinit(&text);
    FloatAdd(&result_value, &a, 0.75);
    text   = FloatToStr(&result_value);
    result = result && (ZstrCompare(StrBegin(&text), "2") == 0);

    FloatDeinit(&a);
    FloatDeinit(&b);
    IntDeinit(&whole);
    FloatDeinit(&result_value);
    StrDeinit(&text);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

bool test_float_sub_small_small(void) {
    WriteFmt("Testing FloatSub with small floats\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Float a            = FloatFromStr("1.5", &alloc.base);
    Float b            = FloatFromStr("2", &alloc.base);
    Float result_value = FloatInit(&alloc.base);
    Str   text         = StrInit(&alloc.base);

    FloatSub(&result_value, &a, &b);
    text = FloatToStr(&result_value);

    bool result = ZstrCompare(StrBegin(&text), "-0.5") == 0;

    StrDeinit(&text);
    FloatDeinit(&a);
    FloatDeinit(&b);
    FloatDeinit(&result_value);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

bool test_float_sub_very_large_large(void) {
    WriteFmt("Testing FloatSub with very large floats\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Float a            = FloatFromStr(FLOAT_TEST_VERY_LARGE_THREES, &alloc.base);
    Float b            = FloatFromStr(FLOAT_TEST_VERY_LARGE_ONES, &alloc.base);
    Float result_value = FloatInit(&alloc.base);
    Str   text         = StrInit(&alloc.base);

    FloatSub(&result_value, &a, &b);
    text = FloatToStr(&result_value);

    bool result = ZstrCompare(StrBegin(&text), FLOAT_TEST_VERY_LARGE_TWOS) == 0;

    StrDeinit(&text);
    FloatDeinit(&a);
    FloatDeinit(&b);
    FloatDeinit(&result_value);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

bool test_float_sub_generic(void) {
    WriteFmt("Testing FloatSub generic dispatch\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Float a            = FloatFromStr("5.5", &alloc.base);
    Float b            = FloatFromStr("0.5", &alloc.base);
    Int   whole        = IntFrom(2, &alloc.base);
    Float result_value = FloatInit(&alloc.base);
    Str   text         = StrInit(&alloc.base);

    FloatSub(&result_value, &a, &b);
    text        = FloatToStr(&result_value);
    bool result = ZstrCompare(StrBegin(&text), "5") == 0;

    StrDeinit(&text);
    FloatSub(&result_value, &a, &whole);
    text   = FloatToStr(&result_value);
    result = result && (ZstrCompare(StrBegin(&text), "3.5") == 0);

    StrDeinit(&text);
    FloatSub(&result_value, &a, 2u);
    text   = FloatToStr(&result_value);
    result = result && (ZstrCompare(StrBegin(&text), "3.5") == 0);

    StrDeinit(&text);
    FloatSub(&result_value, &a, -2);
    text   = FloatToStr(&result_value);
    result = result && (ZstrCompare(StrBegin(&text), "7.5") == 0);

    StrDeinit(&text);
    FloatSub(&result_value, &a, 0.5f);
    text   = FloatToStr(&result_value);
    result = result && (ZstrCompare(StrBegin(&text), "5") == 0);

    FloatDeinit(&a);
    FloatDeinit(&b);
    IntDeinit(&whole);
    FloatDeinit(&result_value);
    StrDeinit(&text);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

bool test_float_mul_small_small(void) {
    WriteFmt("Testing FloatMul with small floats\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Float a            = FloatFromStr("12.5", &alloc.base);
    Float b            = FloatFromStr("-0.2", &alloc.base);
    Float result_value = FloatInit(&alloc.base);
    Str   text         = StrInit(&alloc.base);

    FloatMul(&result_value, &a, &b);
    text = FloatToStr(&result_value);

    bool result = ZstrCompare(StrBegin(&text), "-2.5") == 0;

    StrDeinit(&text);
    FloatDeinit(&a);
    FloatDeinit(&b);
    FloatDeinit(&result_value);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

bool test_float_mul_very_large_small(void) {
    WriteFmt("Testing FloatMul with very large and small floats\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Float a            = FloatFromStr(FLOAT_TEST_VERY_LARGE_ONES, &alloc.base);
    Float b            = FloatFromStr("2", &alloc.base);
    Float result_value = FloatInit(&alloc.base);
    Str   text         = StrInit(&alloc.base);

    FloatMul(&result_value, &a, &b);
    text = FloatToStr(&result_value);

    bool result = ZstrCompare(StrBegin(&text), FLOAT_TEST_VERY_LARGE_TWOS) == 0;

    StrDeinit(&text);
    FloatDeinit(&a);
    FloatDeinit(&b);
    FloatDeinit(&result_value);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

bool test_float_mul_generic(void) {
    WriteFmt("Testing FloatMul generic dispatch\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Float a            = FloatFromStr("1.5", &alloc.base);
    Float b            = FloatFromStr("2", &alloc.base);
    Int   whole        = IntFrom(2, &alloc.base);
    Float result_value = FloatInit(&alloc.base);
    Str   text         = StrInit(&alloc.base);

    FloatMul(&result_value, &a, &b);
    text        = FloatToStr(&result_value);
    bool result = ZstrCompare(StrBegin(&text), "3") == 0;

    StrDeinit(&text);
    FloatMul(&result_value, &a, &whole);
    text   = FloatToStr(&result_value);
    result = result && (ZstrCompare(StrBegin(&text), "3") == 0);

    StrDeinit(&text);
    FloatMul(&result_value, &a, 2u);
    text   = FloatToStr(&result_value);
    result = result && (ZstrCompare(StrBegin(&text), "3") == 0);

    StrDeinit(&text);
    FloatMul(&result_value, &a, -2);
    text   = FloatToStr(&result_value);
    result = result && (ZstrCompare(StrBegin(&text), "-3") == 0);

    StrDeinit(&text);
    FloatMul(&result_value, &a, 0.5f);
    text   = FloatToStr(&result_value);
    result = result && (ZstrCompare(StrBegin(&text), "0.75") == 0);

    FloatDeinit(&a);
    FloatDeinit(&b);
    IntDeinit(&whole);
    FloatDeinit(&result_value);
    StrDeinit(&text);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

bool test_float_div_small_small(void) {
    WriteFmt("Testing FloatDiv with small floats\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Float a            = FloatFromStr("1", &alloc.base);
    Float b            = FloatFromStr("8", &alloc.base);
    Float result_value = FloatInit(&alloc.base);
    Str   text         = StrInit(&alloc.base);

    FloatDiv(&result_value, &a, &b, 3);
    text = FloatToStr(&result_value);

    bool result = ZstrCompare(StrBegin(&text), "0.125") == 0;

    StrDeinit(&text);
    FloatDeinit(&a);
    FloatDeinit(&b);
    FloatDeinit(&result_value);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

bool test_float_div_very_large_small(void) {
    WriteFmt("Testing FloatDiv with very large and small floats\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Float a            = FloatFromStr(FLOAT_TEST_VERY_LARGE_TWOS, &alloc.base);
    Float b            = FloatFromStr("2", &alloc.base);
    Float result_value = FloatInit(&alloc.base);
    Str   text         = StrInit(&alloc.base);

    FloatDiv(&result_value, &a, &b, 0);
    text = FloatToStr(&result_value);

    bool result = ZstrCompare(StrBegin(&text), FLOAT_TEST_VERY_LARGE_ONES) == 0;

    StrDeinit(&text);
    FloatDeinit(&a);
    FloatDeinit(&b);
    FloatDeinit(&result_value);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

bool test_float_div_generic(void) {
    WriteFmt("Testing FloatDiv generic dispatch\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Float a            = FloatFromStr("7.5", &alloc.base);
    Float b            = FloatFromStr("2.5", &alloc.base);
    Int   whole        = IntFrom(3, &alloc.base);
    Float result_value = FloatInit(&alloc.base);
    Str   text         = StrInit(&alloc.base);

    FloatDiv(&result_value, &a, &b, 1);
    text        = FloatToStr(&result_value);
    bool result = ZstrCompare(StrBegin(&text), "3") == 0;

    StrDeinit(&text);
    FloatDiv(&result_value, &a, &whole, 1);
    text   = FloatToStr(&result_value);
    result = result && (ZstrCompare(StrBegin(&text), "2.5") == 0);

    StrDeinit(&text);
    FloatDiv(&result_value, &a, 3u, 1);
    text   = FloatToStr(&result_value);
    result = result && (ZstrCompare(StrBegin(&text), "2.5") == 0);

    StrDeinit(&text);
    FloatDiv(&result_value, &a, -3, 1);
    text   = FloatToStr(&result_value);
    result = result && (ZstrCompare(StrBegin(&text), "-2.5") == 0);

    StrDeinit(&text);
    FloatDiv(&result_value, &a, 0.5f, 1);
    text   = FloatToStr(&result_value);
    result = result && (ZstrCompare(StrBegin(&text), "15") == 0);

    StrDeinit(&text);
    FloatDiv(&result_value, &a, 0.5, 1);
    text   = FloatToStr(&result_value);
    result = result && (ZstrCompare(StrBegin(&text), "15") == 0);

    FloatDeinit(&a);
    FloatDeinit(&b);
    IntDeinit(&whole);
    FloatDeinit(&result_value);
    StrDeinit(&text);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

bool test_float_div_by_zero(void) {
    WriteFmt("Testing FloatDiv divide-by-zero handling\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Float a = FloatFromStr("1", &alloc.base);
    Float b = FloatInit(&alloc.base);
    Float r = FloatInit(&alloc.base);
    bool  ok;

    ok = !FloatDiv(&r, &a, &b, 4);
    ok = ok && FloatIsZero(&r);

    FloatDeinit(&a);
    FloatDeinit(&b);
    FloatDeinit(&r);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// Kills line 950 cxx_assign_const: in float_add the (cmp < 0) branch sets
// temp.negative = rhs.negative. The mutant forces it to true. Here lhs is a
// negative value of SMALLER magnitude than the positive rhs, so the true
// result is positive (rhs sign = false). The result is non-zero, so
// float_normalize does NOT collapse the sign, making the mutation observable.
// Real: -2 + 5 = 3. Mutant: -3.
bool test_m1_add_neg_lhs_larger_pos_rhs(void) {
    WriteFmt("Testing FloatAdd sign: -2 + 5 = 3\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Float a            = FloatFromStr("-2", &alloc.base);
    Float b            = FloatFromStr("5", &alloc.base);
    Float result_value = FloatInit(&alloc.base);
    Str   text         = StrInit(&alloc.base);

    FloatAdd(&result_value, &a, &b);
    text = FloatToStr(&result_value);

    bool result = ZstrCompare(StrBegin(&text), "3") == 0;

    StrDeinit(&text);
    FloatDeinit(&a);
    FloatDeinit(&b);
    FloatDeinit(&result_value);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Same lever from the other operand ordering: lhs positive of smaller
// magnitude than negative rhs. Here cmp(|lhs|, |rhs|) < 0 so the result sign
// must be rhs.negative == true. Real: 2 + (-5) = -5+2 = -3. The mutant's forced
// true happens to match here, so this guards the genuine sign value rather than
// the mutant; kept for behavioral coverage of the cmp<0 branch.
bool test_m1_add_pos_lhs_larger_neg_rhs(void) {
    WriteFmt("Testing FloatAdd sign: 2 + (-5) = -3\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Float a            = FloatFromStr("2", &alloc.base);
    Float b            = FloatFromStr("-5", &alloc.base);
    Float result_value = FloatInit(&alloc.base);
    Str   text         = StrInit(&alloc.base);

    FloatAdd(&result_value, &a, &b);
    text = FloatToStr(&result_value);

    bool result = ZstrCompare(StrBegin(&text), "-3") == 0;

    StrDeinit(&text);
    FloatDeinit(&a);
    FloatDeinit(&b);
    FloatDeinit(&result_value);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Decimal-fractional variant of the cmp<0 positive-result kill, exercising the
// exponent-alignment path as well: -0.25 + 1.00 = 0.75 (positive). Mutant would
// yield -0.75.
bool test_m1_add_sign_from_larger_magnitude(void) {
    WriteFmt("Testing FloatAdd sign w/ fractions: -0.25 + 1 = 0.75\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Float a            = FloatFromStr("-0.25", &alloc.base);
    Float b            = FloatFromStr("1.00", &alloc.base);
    Float result_value = FloatInit(&alloc.base);
    Str   text         = StrInit(&alloc.base);

    FloatAdd(&result_value, &a, &b);
    text = FloatToStr(&result_value);

    bool result = ZstrCompare(StrBegin(&text), "0.75") == 0;

    StrDeinit(&text);
    FloatDeinit(&a);
    FloatDeinit(&b);
    FloatDeinit(&result_value);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// float_add_i64_checked: `a > INT64_MAX - b` (line 28 col 21). At the exact
// boundary the real code does NOT overflow (sum == INT64_MAX is representable),
// so FloatMul succeeds and the result exponent equals INT64_MAX. Under the
// gt_to_ge mutant the guard fires LOG_FATAL and the process aborts -> killed.
static bool test_m10_mul_exp_max_boundary(void) {
    WriteFmt("Testing exponent-sum at INT64_MAX boundary (FloatMul)\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Float a = FloatFromStr("3e" EXP_HALF_LO, &alloc.base);
    Float b = FloatFromStr("3e" EXP_HALF_HI, &alloc.base);
    Float r = FloatInit(&alloc.base);

    bool ok = FloatMul(&r, &a, &b);
    ok      = ok && (FloatExponent(&r) == INT64_MAX);

    FloatDeinit(&a);
    FloatDeinit(&b);
    FloatDeinit(&r);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// float_add_i64_checked: `a < INT64_MIN - b` (line 28 col 53). Sum of two
// negative exponents lands exactly on INT64_MIN; real code accepts it, the
// lt_to_le mutant aborts -> killed.
static bool test_m10_mul_exp_min_boundary(void) {
    WriteFmt("Testing exponent-sum at INT64_MIN boundary (FloatMul)\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Float a = FloatFromStr("3e-" EXP_HALF_HI, &alloc.base);
    Float b = FloatFromStr("3e-" EXP_HALF_HI, &alloc.base);
    Float r = FloatInit(&alloc.base);

    bool ok = FloatMul(&r, &a, &b);
    ok      = ok && (FloatExponent(&r) == INT64_MIN);

    FloatDeinit(&a);
    FloatDeinit(&b);
    FloatDeinit(&r);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// float_sub_i64_checked inner call in float_div: `a < INT64_MIN + b`
// (line 36 col 21). a->exp - b->exp == INT64_MIN exactly. Significand 3/3 == 1,
// so normalize does not perturb the exponent. Real returns the boundary
// exponent; the lt_to_le mutant aborts -> killed.
static bool test_m10_div_exp_min_boundary(void) {
    WriteFmt("Testing exponent-diff at INT64_MIN boundary (FloatDiv)\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Float a = FloatFromStr("3e-" EXP_HALF_HI, &alloc.base);
    Float b = FloatFromStr("3e" EXP_HALF_HI, &alloc.base);
    Float r = FloatInit(&alloc.base);

    bool ok = FloatDiv(&r, &a, &b, 0);
    ok      = ok && (FloatExponent(&r) == INT64_MIN);

    FloatDeinit(&a);
    FloatDeinit(&b);
    FloatDeinit(&r);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// float_sub_i64_checked inner call in float_div: `a > INT64_MAX + b`
// (line 36 col 53). a->exp - b->exp == INT64_MAX exactly. Real returns the
// boundary exponent; the gt_to_ge mutant aborts -> killed.
static bool test_m10_div_exp_max_boundary(void) {
    WriteFmt("Testing exponent-diff at INT64_MAX boundary (FloatDiv)\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Float a = FloatFromStr("3e" EXP_HALF_LO, &alloc.base);
    Float b = FloatFromStr("3e-" EXP_HALF_HI, &alloc.base);
    Float r = FloatInit(&alloc.base);

    bool ok = FloatDiv(&r, &a, &b, 0);
    ok      = ok && (FloatExponent(&r) == INT64_MAX);

    FloatDeinit(&a);
    FloatDeinit(&b);
    FloatDeinit(&r);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// float_div_f64: `ok = float_div(...)` (line 1263 cols 8 & 10). Dividing by a
// 0.0 double routes through float_div_f64, whose float_div sees a zero divisor
// and returns false. Both the assign-const (ok = 42) and replace-scalar-call
// (call -> 42) mutants force ok truthy -> FloatDiv returns true -> killed.
static bool test_m10_div_f64_by_zero_returns_false(void) {
    WriteFmt("Testing float_div_f64 propagates divide-by-zero failure\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Float a = FloatFromStr("1", &alloc.base);
    Float r = FloatInit(&alloc.base);

    bool ok = !FloatDiv(&r, &a, 0.0, 4);

    FloatDeinit(&a);
    FloatDeinit(&r);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// Kills 109:24 cxx_rshift_to_lshift in float_try_from_f32_value:
//   `(bits >> 31) & 1u` computes the IEEE sign bit. Swapping to
//   `(bits << 31) & 1u` always yields bit 0 of `bits` shifted left,
//   whose low bit is 0, so a negative input would lose its sign.
bool test_m11_f32_sign(void) {
    WriteFmt("Testing float_try_from_f32_value sign bit\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Float neg = FloatFrom(-1.0f, &alloc.base);
    Float pos = FloatFrom(1.0f, &alloc.base);

    bool result = FloatIsNegative(&neg) && !FloatIsNegative(&pos);

    FloatDeinit(&neg);
    FloatDeinit(&pos);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Kills 120:18 cxx_assign_const in float_try_from_f32_value:
//   denormal branch sets `binexp = -126 - 23` (= -149). Replacing it
//   with `42` makes the result `mantissa << 42` (a huge positive value)
//   instead of a tiny subnormal. The smallest positive subnormal is
//   well below 1, but the mutant lands far above it.
bool test_m11_f32_denormal_exp(void) {
    WriteFmt("Testing float_try_from_f32_value denormal exponent\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    // 0x1p-149f: e == 0, m == 1 -> smallest positive f32 subnormal.
    Float tiny = FloatFrom(0x1p-149f, &alloc.base);

    bool result = FloatLT(&tiny, 1.0f) && FloatGT(&tiny, 0.0f);

    FloatDeinit(&tiny);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Kills 119:18 cxx_assign_const in float_try_from_f32_value:
//   denormal branch sets `mantissa = (u64)m`. Replacing it with `42`
//   collapses every subnormal (same exponent) to one value, so two
//   distinct subnormals (m=1 vs m=3) would compare equal.
bool test_m11_f32_denormal_mantissa(void) {
    WriteFmt("Testing float_try_from_f32_value denormal mantissa\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Float a = FloatFrom(0x1p-149f, &alloc.base); // m == 1
    Float b = FloatFrom(0x3p-149f, &alloc.base); // m == 3

    bool result = FloatNE(&a, &b);

    FloatDeinit(&a);
    FloatDeinit(&b);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Kills 138:24 cxx_rshift_to_lshift in float_try_from_f64_value:
//   `(bits >> 63) & 1ull` computes the IEEE sign bit; `<<` loses it.
bool test_m11_f64_sign(void) {
    WriteFmt("Testing float_try_from_f64_value sign bit\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Float neg = FloatFrom(-1.0, &alloc.base);
    Float pos = FloatFrom(1.0, &alloc.base);

    bool result = FloatIsNegative(&neg) && !FloatIsNegative(&pos);

    FloatDeinit(&neg);
    FloatDeinit(&pos);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Kills 148:18 cxx_assign_const in float_try_from_f64_value:
//   denormal branch sets `binexp = -1022 - 52` (= -1074); `42` would
//   produce `mantissa << 42` instead of a tiny subnormal.
bool test_m11_f64_denormal_exp(void) {
    WriteFmt("Testing float_try_from_f64_value denormal exponent\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    // 0x1p-1074: e == 0, m == 1 -> smallest positive f64 subnormal.
    Float tiny = FloatFrom(0x1p-1074, &alloc.base);

    bool result = FloatLT(&tiny, 1.0) && FloatGT(&tiny, 0.0);

    FloatDeinit(&tiny);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Kills 147:18 cxx_assign_const in float_try_from_f64_value:
//   denormal branch sets `mantissa = m`; `42` collapses distinct
//   subnormals to one value.
bool test_m11_f64_denormal_mantissa(void) {
    WriteFmt("Testing float_try_from_f64_value denormal mantissa\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Float a = FloatFrom(0x1p-1074, &alloc.base); // m == 1
    Float b = FloatFrom(0x3p-1074, &alloc.base); // m == 3

    bool result = FloatNE(&a, &b);

    FloatDeinit(&a);
    FloatDeinit(&b);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

bool test_m12_div_int_by_zero_returns_false(void) {
    WriteFmt("Testing FloatDiv(Int 0) returns false\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Float a      = FloatFromStr("6", &alloc.base);
    Float result = FloatInit(&alloc.base);
    Int   zero   = IntFrom(0u, &alloc.base);

    // Real float_div rejects division by zero -> false.
    bool ok = FloatDiv(&result, &a, &zero, 4);

    bool pass = (ok == false);

    IntDeinit(&zero);
    FloatDeinit(&a);
    FloatDeinit(&result);
    DefaultAllocatorDeinit(&alloc);
    return pass;
}

bool test_m12_div_i64_by_zero_returns_false(void) {
    WriteFmt("Testing FloatDiv(i64 0) returns false\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Float a      = FloatFromStr("6", &alloc.base);
    Float result = FloatInit(&alloc.base);

    bool ok = FloatDiv(&result, &a, (signed long long)0, 4);

    bool pass = (ok == false);

    FloatDeinit(&a);
    FloatDeinit(&result);
    DefaultAllocatorDeinit(&alloc);
    return pass;
}

bool test_m12_div_f32_by_zero_returns_false(void) {
    WriteFmt("Testing FloatDiv(f32 0) returns false\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Float a      = FloatFromStr("6", &alloc.base);
    Float result = FloatInit(&alloc.base);

    bool ok = FloatDiv(&result, &a, 0.0f, 4);

    bool pass = (ok == false);

    FloatDeinit(&a);
    FloatDeinit(&result);
    DefaultAllocatorDeinit(&alloc);
    return pass;
}

// Kills line 479 cxx_assign_const: `negative = text[pos] == '-'` mutated to
// `negative = 42` would force every explicitly-signed literal negative. A
// leading '+' must yield a positive value.
bool test_m3_leading_plus_is_positive(void) {
    WriteFmt("Testing FloatTryFromStr keeps leading-'+' positive\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Float f    = FloatInit(&alloc.base);
    bool  ok   = FloatTryFromStr(&f, "+5");
    Str   text = StrInit(&alloc.base);

    text = FloatToStr(&f);

    bool result = ok && (ZstrCompare(StrBegin(&text), "5") == 0);

    StrDeinit(&text);
    FloatDeinit(&f);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Kills line 486 cxx_le_to_lt: `ch <= '9'` mutated to `ch < '9'` rejects the
// digit '9' as an invalid character. Parsing a string containing '9' must
// succeed and produce the right value.
bool test_m3_digit_nine_accepted(void) {
    WriteFmt("Testing FloatTryFromStr accepts the digit '9'\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Float f    = FloatInit(&alloc.base);
    bool  ok   = FloatTryFromStr(&f, "19");
    Str   text = StrInit(&alloc.base);

    text = FloatToStr(&f);

    bool result = ok && (ZstrCompare(StrBegin(&text), "19") == 0);

    StrDeinit(&text);
    FloatDeinit(&f);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

bool test_m4_div_sanity(void) {
    WriteFmt("Testing float_div sanity (6/3=2, exact)\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Float a        = FloatFromStr("6", &alloc.base);
    Float b        = FloatFromStr("3", &alloc.base);
    Float quotient = FloatInit(&alloc.base);
    Str   text     = StrInit(&alloc.base);

    bool ok     = FloatDiv(&quotient, &a, &b, 8);
    text        = FloatToStr(&quotient);
    bool result = ok && (ZstrCompare(StrBegin(&text), "2") == 0);

    StrDeinit(&text);
    FloatDeinit(&a);
    FloatDeinit(&b);
    FloatDeinit(&quotient);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

///
/// float_div_u64 must propagate the false result of float_div when the
/// divisor is zero. Mutations that force ok=true (cxx_assign_const) or
/// replace the float_div return value with a truthy literal
/// (cxx_replace_scalar_call) at Float.c:1227 make this return true instead.
///
bool test_m9_div_u64_by_zero_returns_false(void) {
    WriteFmt("Testing FloatDiv(u64) divide-by-zero returns false\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Float a = FloatFromStr("7.5", &alloc.base);
    Float r = FloatInit(&alloc.base);

    bool ok = (FloatDiv(&r, &a, 0u, 4) == false);

    FloatDeinit(&a);
    FloatDeinit(&r);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

///
/// A zero-divisor float_div_u64 must not write a quotient into result.
/// Reinforces the false-propagation kill: result stays at its
/// pre-call zero value when the operation reports failure.
///
bool test_m9_div_u64_by_zero_leaves_result_unchanged(void) {
    WriteFmt("Testing FloatDiv(u64) divide-by-zero keeps result zero\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Float a = FloatFromStr("100", &alloc.base);
    Float r = FloatInit(&alloc.base);

    bool ok = !FloatDiv(&r, &a, 0u, 4);
    ok      = ok && FloatIsZero(&r);

    FloatDeinit(&a);
    FloatDeinit(&r);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

///
/// Exact division through the u64 dispatch: 7.5 / 3 = 2.5. Guards the
/// success path of float_div_u64 (returns true with the correct quotient).
///
bool test_m9_div_u64_exact(void) {
    WriteFmt("Testing FloatDiv(u64) exact quotient\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Float a = FloatFromStr("7.5", &alloc.base);
    Float r = FloatInit(&alloc.base);
    Str   text;

    bool ok = FloatDiv(&r, &a, 3u, 1);
    text    = FloatToStr(&r);
    ok      = ok && (ZstrCompare(StrBegin(&text), "2.5") == 0);

    StrDeinit(&text);
    FloatDeinit(&a);
    FloatDeinit(&r);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

int main(void) {
    WriteFmt("[INFO] Starting Float.Math tests\n\n");

    TestFunction tests[] = {
        test_float_negate_abs,
        test_float_add_small_small,
        test_float_add_very_large_large,
        test_float_add_generic,
        test_float_sub_small_small,
        test_float_sub_very_large_large,
        test_float_sub_generic,
        test_float_mul_small_small,
        test_float_mul_very_large_small,
        test_float_mul_generic,
        test_float_div_small_small,
        test_float_div_very_large_small,
        test_float_div_generic,
        test_float_div_by_zero,
        test_m1_add_neg_lhs_larger_pos_rhs,
        test_m1_add_pos_lhs_larger_neg_rhs,
        test_m1_add_sign_from_larger_magnitude,
        test_m10_mul_exp_max_boundary,
        test_m10_mul_exp_min_boundary,
        test_m10_div_exp_min_boundary,
        test_m10_div_exp_max_boundary,
        test_m10_div_f64_by_zero_returns_false,
        test_m11_f32_sign,
        test_m11_f32_denormal_exp,
        test_m11_f32_denormal_mantissa,
        test_m11_f64_sign,
        test_m11_f64_denormal_exp,
        test_m11_f64_denormal_mantissa,
        test_m12_div_int_by_zero_returns_false,
        test_m12_div_i64_by_zero_returns_false,
        test_m12_div_f32_by_zero_returns_false,
        test_m3_leading_plus_is_positive,
        test_m3_digit_nine_accepted,
        test_m4_div_sanity,
        test_m9_div_u64_by_zero_returns_false,
        test_m9_div_u64_by_zero_leaves_result_unchanged,
        test_m9_div_u64_exact,
    };

    TestFunction deadend_tests[1] = {0};

    int total_tests         = sizeof(tests) / sizeof(tests[0]);
    int total_deadend_tests = 0;

    return run_test_suite(tests, total_tests, deadend_tests, total_deadend_tests, "Float.Math");
}
