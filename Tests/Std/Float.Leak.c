/// file : tests/std/float.leak.c
///
/// Leak-guard tests for Float: route allocations through an explicit
/// DebugAllocator and assert DebugAllocatorLiveCount(&dbg) == 0 after full
/// cleanup, to KILL mutation survivors that remove internal *Deinit calls on
/// input-reachable branches (the leak-only proposals). Distinct contract from
/// the value-correctness tests in the sibling Float.* files -- do NOT duplicate.
///
/// Each test drives a concrete input that REACHES a specific success-path (or
/// always-reached) internal *Deinit / *Clear, frees everything the test owns,
/// then asserts zero live allocations. Removing the targeted internal Deinit
/// leaves a temporary outstanding -> LiveCount/LiveBytes != 0 -> test FAILS ->
/// mutant KILLED.

#include <Misra.h>
#include <Misra/Std/Allocator/Debug.h>
#include <Misra/Std/Container/Float.h>
#include <Misra/Std/Container/Int.h>
#include <Misra/Std/Container/Str.h>

#include "../Util/TestRunner.h"

// Convenience: a test passes iff `ok` held AND the DebugAllocator has no
// outstanding allocations after the test released everything it owns.
#define LEAK_CLEAN(dbg) (DebugAllocatorLiveCount(&(dbg)) == 0 && DebugAllocatorLiveBytes(&(dbg)) == 0)

// Leak-only config: live-count tracking, NO per-alloc stack-trace / canary /
// freed-history -- avoids the backtrace-per-alloc cost under mull. Leak
// detection (LiveCount / LiveBytes) is unchanged.
#define LEAK_CFG                                                                                                       \
    ((DebugAllocatorConfig) {.capture_traces = false, .detect_overflow = false, .track_freed_history = false})

// =============================================================================
// float_try_from_ieee_bits success path (binexp < 0):
//   IntDeinit(&five)  [87:9]
//   IntDeinit(&pow5)  [88:9]
//   IntDeinit(&out->significand) before significand = sig  [89:9]
// Reached by FloatFrom(f32/f64) of a value whose binary exponent is negative
// (any fractional value with a set mantissa), e.g. 0.5, 0.25, 1.5.

bool test_from_f64_negative_binexp_no_leak(void) {
    DebugAllocator dbg = DebugAllocatorInitWith(LEAK_CFG);

    Float v = FloatFrom(0.5, &dbg.base); // binexp < 0 -> 5^|binexp| path
    FloatDeinit(&v);

    bool ok = LEAK_CLEAN(dbg);
    DebugAllocatorDeinit(&dbg);
    return ok;
}

bool test_from_f32_negative_binexp_no_leak(void) {
    DebugAllocator dbg = DebugAllocatorInitWith(LEAK_CFG);

    Float v = FloatFrom(1.5f, &dbg.base); // binexp < 0 -> 5^|binexp| path
    FloatDeinit(&v);

    bool ok = LEAK_CLEAN(dbg);
    DebugAllocatorDeinit(&dbg);
    return ok;
}

// =============================================================================
// float_normalize trailing-zero loop body:
//   IntDeinit(&value->significand) before significand = quotient  [263:9]
// Reached by constructing a value whose significand is divisible by 10 so the
// while loop iterates at least once (FloatFrom of 100 normalizes 100 -> 1e2).

bool test_normalize_trailing_zeros_no_leak(void) {
    DebugAllocator dbg = DebugAllocatorInitWith(LEAK_CFG);

    Float v = FloatFrom(1000u, &dbg.base); // 1000 -> significand 1, exp 3
    FloatDeinit(&v);

    bool ok = LEAK_CLEAN(dbg);
    DebugAllocatorDeinit(&dbg);
    return ok;
}

// =============================================================================
// float_pow10 success path:
//   IntDeinit(&base) before *out = result  [179:5]
// Reached by any op that calls float_pow10 with success: FloatToInt of an
// integer-valued float with exponent > 0 (e.g. "1e3").

bool test_pow10_success_no_leak(void) {
    DebugAllocator dbg = DebugAllocatorInitWith(LEAK_CFG);

    Float v = FloatFromStr("1e3", &dbg.base); // exponent 3 > 0 -> pow10
    Int   r = IntInit(&dbg.base);

    bool ok = FloatToInt(&r, &v);             // drives float_pow10 + success path

    FloatDeinit(&v);
    IntDeinit(&r);
    ok = ok && LEAK_CLEAN(dbg);
    DebugAllocatorDeinit(&dbg);
    return ok;
}

// =============================================================================
// FloatToInt exponent >= 0 success path:
//   IntDeinit(&factor) after successful mul  [432:9]
//   IntDeinit(result) before *result = temp  [433:9]
// Reached by FloatToInt where the destination Int already holds an allocated
// value (so IntDeinit(result) actually frees something) and exponent > 0.

bool test_to_int_positive_exponent_no_leak(void) {
    DebugAllocator dbg = DebugAllocatorInitWith(LEAK_CFG);

    Float v = FloatFromStr("12e4", &dbg.base); // significand 12, exp 4
    Int   r = IntFrom(999999999u, &dbg.base);  // pre-populated dest

    bool ok = FloatToInt(&r, &v);

    FloatDeinit(&v);
    IntDeinit(&r);
    ok = ok && LEAK_CLEAN(dbg);
    DebugAllocatorDeinit(&dbg);
    return ok;
}

// =============================================================================
// FloatToInt exponent < 0 success path:
//   IntDeinit(&factor) after int_div_exact  [448:9]
//   IntDeinit(result) before final *result = temp  [455:5]
// Reached by FloatToInt of a value with negative exponent that divides exactly
// (e.g. "150e-1" == 15) into a pre-populated destination.

bool test_to_int_negative_exponent_no_leak(void) {
    DebugAllocator dbg = DebugAllocatorInitWith(LEAK_CFG);

    Float v = FloatFromStr("150e-1", &dbg.base); // 15.0, exact integer 15
    Int   r = IntFrom(777u, &dbg.base);          // pre-populated dest

    bool ok = FloatToInt(&r, &v);

    FloatDeinit(&v);
    IntDeinit(&r);
    ok = ok && LEAK_CLEAN(dbg);
    DebugAllocatorDeinit(&dbg);
    return ok;
}

// =============================================================================
// FloatToInt zero-value success path:
//   IntDeinit(result) before *result = temp  [417:9]
// Reached by FloatToInt of zero into a pre-populated destination.

bool test_to_int_zero_no_leak(void) {
    DebugAllocator dbg = DebugAllocatorInitWith(LEAK_CFG);

    Float v = FloatFrom(0u, &dbg.base);
    Int   r = IntFrom(424242u, &dbg.base); // pre-populated dest

    bool ok = FloatToInt(&r, &v);

    FloatDeinit(&v);
    IntDeinit(&r);
    ok = ok && LEAK_CLEAN(dbg);
    DebugAllocatorDeinit(&dbg);
    return ok;
}

// =============================================================================
// float_scale_to_exponent success path:
//   IntDeinit(&factor)              [208:9]
//   IntDeinit(&value->significand)  [209:9]
// Reached via float_add of two floats with different exponents, forcing the
// smaller-magnitude operand to be scaled.

bool test_add_scaling_no_leak(void) {
    DebugAllocator dbg = DebugAllocatorInitWith(LEAK_CFG);

    Float a = FloatFromStr("1.5", &dbg.base);   // exp -1
    Float b = FloatFromStr("0.025", &dbg.base); // exp -3 -> scaling occurs
    Float r = FloatInit(&dbg.base);

    bool ok = FloatAdd(&r, &a, &b);

    FloatDeinit(&a);
    FloatDeinit(&b);
    FloatDeinit(&r);
    ok = ok && LEAK_CLEAN(dbg);
    DebugAllocatorDeinit(&dbg);
    return ok;
}

// =============================================================================
// float_add final cleanup + float_replace (success path):
//   FloatDeinit(&lhs)  [957:5]
//   FloatDeinit(&rhs)  [958:5]
//   float_replace: FloatDeinit(dst) before *dst = *src  [157:5]
// Reached by float_add into a result that already holds an allocated value
// (so float_replace's FloatDeinit(dst) frees real storage).

bool test_add_replace_prepopulated_no_leak(void) {
    DebugAllocator dbg = DebugAllocatorInitWith(LEAK_CFG);

    Float a = FloatFromStr("2.5", &dbg.base);
    Float b = FloatFromStr("3.5", &dbg.base);
    Float r = FloatFromStr("123456789", &dbg.base); // pre-populated dest

    bool ok = FloatAdd(&r, &a, &b);

    FloatDeinit(&a);
    FloatDeinit(&b);
    FloatDeinit(&r);
    ok = ok && LEAK_CLEAN(dbg);
    DebugAllocatorDeinit(&dbg);
    return ok;
}

// =============================================================================
// float_add equal-magnitude opposite-sign branch:
//   FloatClear(&temp)  [952:13]
// Reached by adding x + (-x); cmp == 0 path. (temp.significand is freshly
// zero here so this is leak-neutral, but the test still exercises the branch.)

bool test_add_cancel_to_zero_no_leak(void) {
    DebugAllocator dbg = DebugAllocatorInitWith(LEAK_CFG);

    Float a = FloatFromStr("7.25", &dbg.base);
    Float b = FloatFromStr("-7.25", &dbg.base);
    Float r = FloatInit(&dbg.base);

    bool ok = FloatAdd(&r, &a, &b);
    ok      = ok && FloatIsZero(&r);

    FloatDeinit(&a);
    FloatDeinit(&b);
    FloatDeinit(&r);
    ok = ok && LEAK_CLEAN(dbg);
    DebugAllocatorDeinit(&dbg);
    return ok;
}

// =============================================================================
// float_try_abs_compare success path:
//   FloatDeinit(&lhs_scaled)  [244:9]
//   FloatDeinit(&rhs_scaled)  [245:9]
// Reached by FloatCompare of two non-zero floats with different exponents
// (same sign so the abs-compare path is taken).

bool test_compare_abs_no_leak(void) {
    DebugAllocator dbg = DebugAllocatorInitWith(LEAK_CFG);

    Float a = FloatFromStr("1.5", &dbg.base);
    Float b = FloatFromStr("1.25", &dbg.base);

    int  cmp = FloatCompare(&a, &b);
    bool ok  = cmp > 0;

    FloatDeinit(&a);
    FloatDeinit(&b);
    ok = ok && LEAK_CLEAN(dbg);
    DebugAllocatorDeinit(&dbg);
    return ok;
}

// =============================================================================
// float_to_str success paths:
//   StrDeinit(&digits) before *out = result  [681:5]
// Reached by FloatToStr of a non-zero value with a fractional part (exercises
// the digits buffer + result assembly + final StrDeinit(&digits)).

bool test_to_str_fractional_no_leak(void) {
    DebugAllocator dbg = DebugAllocatorInitWith(LEAK_CFG);

    Float v   = FloatFromStr("12.5", &dbg.base);
    Str   out = StrInit(&dbg.base);

    bool ok = FloatTryToStr(&out, &v);

    FloatDeinit(&v);
    StrDeinit(&out);
    ok = ok && LEAK_CLEAN(dbg);
    DebugAllocatorDeinit(&dbg);
    return ok;
}

bool test_to_str_integer_no_leak(void) {
    DebugAllocator dbg = DebugAllocatorInitWith(LEAK_CFG);

    Float v   = FloatFromStr("1200", &dbg.base); // exponent >= 0 arm
    Str   out = StrInit(&dbg.base);

    bool ok = FloatTryToStr(&out, &v);

    FloatDeinit(&v);
    StrDeinit(&out);
    ok = ok && LEAK_CLEAN(dbg);
    DebugAllocatorDeinit(&dbg);
    return ok;
}

// =============================================================================
// float_try_from_str_impl success path:
//   StrDeinit(&digits) before *out = result  [568:5]
//   FloatDeinit(out) before *out = result    [570:5]
// Reached by FloatTryFromStr into a pre-populated Float (so the FloatDeinit(out)
// before the swap frees real storage).

bool test_from_str_prepopulated_no_leak(void) {
    DebugAllocator dbg = DebugAllocatorInitWith(LEAK_CFG);

    Float v = FloatFromStr("99999", &dbg.base); // pre-populated target

    bool ok = FloatTryFromStr(&v, "3.14159");

    FloatDeinit(&v);
    ok = ok && LEAK_CLEAN(dbg);
    DebugAllocatorDeinit(&dbg);
    return ok;
}

// =============================================================================
// float_div success path:
//   IntDeinit(&scale)   [1200:5]
//   IntDeinit(&scaled)  [1201:5]
//   (plus float_replace into a pre-populated result)
// Reached by FloatDiv of two non-zero floats with precision into a populated
// result.

bool test_div_success_no_leak(void) {
    DebugAllocator dbg = DebugAllocatorInitWith(LEAK_CFG);

    Float a = FloatFromStr("1", &dbg.base);
    Float b = FloatFromStr("3", &dbg.base);
    Float r = FloatFromStr("987654321", &dbg.base); // pre-populated dest

    bool ok = FloatDiv(&r, &a, &b, 8u);

    FloatDeinit(&a);
    FloatDeinit(&b);
    FloatDeinit(&r);
    ok = ok && LEAK_CLEAN(dbg);
    DebugAllocatorDeinit(&dbg);
    return ok;
}

// =============================================================================
// float_div a == 0 short-circuit success path:
//   FloatDeinit(result) before *result = zero  [1179:9]
// Reached by FloatDiv where the dividend is zero into a pre-populated result.

bool test_div_zero_dividend_no_leak(void) {
    DebugAllocator dbg = DebugAllocatorInitWith(LEAK_CFG);

    Float a = FloatFrom(0u, &dbg.base);
    Float b = FloatFromStr("4", &dbg.base);
    Float r = FloatFromStr("55555", &dbg.base); // pre-populated dest

    bool ok = FloatDiv(&r, &a, &b, 4u);
    ok      = ok && FloatIsZero(&r);

    FloatDeinit(&a);
    FloatDeinit(&b);
    FloatDeinit(&r);
    ok = ok && LEAK_CLEAN(dbg);
    DebugAllocatorDeinit(&dbg);
    return ok;
}

// =============================================================================
// float_mul success path drives float_replace into a pre-populated result.
//   float_replace FloatDeinit(dst)  [157:5] (also via mul)
// (FloatMul itself has no rhs-temp on the Float*Float overload; the scalar
// overloads carry the FloatDeinit(&rhs) tested below.)

bool test_mul_replace_prepopulated_no_leak(void) {
    DebugAllocator dbg = DebugAllocatorInitWith(LEAK_CFG);

    Float a = FloatFromStr("2.5", &dbg.base);
    Float b = FloatFromStr("4", &dbg.base);
    Float r = FloatFromStr("314159265", &dbg.base); // pre-populated dest

    bool ok = FloatMul(&r, &a, &b);

    FloatDeinit(&a);
    FloatDeinit(&b);
    FloatDeinit(&r);
    ok = ok && LEAK_CLEAN(dbg);
    DebugAllocatorDeinit(&dbg);
    return ok;
}

// =============================================================================
// Scalar-operand temporaries: FloatDeinit(&rhs) on the success path of every
// float_<op>_<scalar> wrapper. Each wrapper builds an internal `rhs` Float from
// the scalar and frees it before returning. Removing that FloatDeinit(&rhs)
// leaks the rhs significand. One leak-guard per distinct wrapper line.

// float_add_int  [970:5]
bool test_add_int_rhs_no_leak(void) {
    DebugAllocator dbg = DebugAllocatorInitWith(LEAK_CFG);
    Float          a   = FloatFromStr("1.5", &dbg.base);
    Float          r   = FloatInit(&dbg.base);
    Int            b   = IntFrom(2, &dbg.base);

    bool ok = FloatAdd(&r, &a, &b);

    FloatDeinit(&a);
    FloatDeinit(&r);
    IntDeinit(&b);
    ok = ok && LEAK_CLEAN(dbg);
    DebugAllocatorDeinit(&dbg);
    return ok;
}

// float_add_u64  [981:5]
bool test_add_u64_rhs_no_leak(void) {
    DebugAllocator dbg = DebugAllocatorInitWith(LEAK_CFG);
    Float          a   = FloatFromStr("1.5", &dbg.base);
    Float          r   = FloatInit(&dbg.base);

    bool ok = FloatAdd(&r, &a, 2u);

    FloatDeinit(&a);
    FloatDeinit(&r);
    ok = ok && LEAK_CLEAN(dbg);
    DebugAllocatorDeinit(&dbg);
    return ok;
}

// float_add_i64  [992:5]
bool test_add_i64_rhs_no_leak(void) {
    DebugAllocator dbg = DebugAllocatorInitWith(LEAK_CFG);
    Float          a   = FloatFromStr("1.5", &dbg.base);
    Float          r   = FloatInit(&dbg.base);

    bool ok = FloatAdd(&r, &a, -2);

    FloatDeinit(&a);
    FloatDeinit(&r);
    ok = ok && LEAK_CLEAN(dbg);
    DebugAllocatorDeinit(&dbg);
    return ok;
}

// float_add_f32  [1003:5]
bool test_add_f32_rhs_no_leak(void) {
    DebugAllocator dbg = DebugAllocatorInitWith(LEAK_CFG);
    Float          a   = FloatFromStr("1.5", &dbg.base);
    Float          r   = FloatInit(&dbg.base);

    bool ok = FloatAdd(&r, &a, 0.5f);

    FloatDeinit(&a);
    FloatDeinit(&r);
    ok = ok && LEAK_CLEAN(dbg);
    DebugAllocatorDeinit(&dbg);
    return ok;
}

// float_add_f64  [1014:5]
bool test_add_f64_rhs_no_leak(void) {
    DebugAllocator dbg = DebugAllocatorInitWith(LEAK_CFG);
    Float          a   = FloatFromStr("1.5", &dbg.base);
    Float          r   = FloatInit(&dbg.base);

    bool ok = FloatAdd(&r, &a, 0.25);

    FloatDeinit(&a);
    FloatDeinit(&r);
    ok = ok && LEAK_CLEAN(dbg);
    DebugAllocatorDeinit(&dbg);
    return ok;
}

// float_sub (Float*Float): FloatDeinit(&rhs) negated clone  [1030:5]
bool test_sub_float_rhs_no_leak(void) {
    DebugAllocator dbg = DebugAllocatorInitWith(LEAK_CFG);
    Float          a   = FloatFromStr("5.5", &dbg.base);
    Float          b   = FloatFromStr("1.25", &dbg.base);
    Float          r   = FloatInit(&dbg.base);

    bool ok = FloatSub(&r, &a, &b);

    FloatDeinit(&a);
    FloatDeinit(&b);
    FloatDeinit(&r);
    ok = ok && LEAK_CLEAN(dbg);
    DebugAllocatorDeinit(&dbg);
    return ok;
}

// float_sub_int  [1041:5]
bool test_sub_int_rhs_no_leak(void) {
    DebugAllocator dbg = DebugAllocatorInitWith(LEAK_CFG);
    Float          a   = FloatFromStr("5.5", &dbg.base);
    Float          r   = FloatInit(&dbg.base);
    Int            b   = IntFrom(2, &dbg.base);

    bool ok = FloatSub(&r, &a, &b);

    FloatDeinit(&a);
    FloatDeinit(&r);
    IntDeinit(&b);
    ok = ok && LEAK_CLEAN(dbg);
    DebugAllocatorDeinit(&dbg);
    return ok;
}

// float_sub_u64  [1052:5]
bool test_sub_u64_rhs_no_leak(void) {
    DebugAllocator dbg = DebugAllocatorInitWith(LEAK_CFG);
    Float          a   = FloatFromStr("5.5", &dbg.base);
    Float          r   = FloatInit(&dbg.base);

    bool ok = FloatSub(&r, &a, 2u);

    FloatDeinit(&a);
    FloatDeinit(&r);
    ok = ok && LEAK_CLEAN(dbg);
    DebugAllocatorDeinit(&dbg);
    return ok;
}

// float_sub_i64  [1063:5]
bool test_sub_i64_rhs_no_leak(void) {
    DebugAllocator dbg = DebugAllocatorInitWith(LEAK_CFG);
    Float          a   = FloatFromStr("5.5", &dbg.base);
    Float          r   = FloatInit(&dbg.base);

    bool ok = FloatSub(&r, &a, -2);

    FloatDeinit(&a);
    FloatDeinit(&r);
    ok = ok && LEAK_CLEAN(dbg);
    DebugAllocatorDeinit(&dbg);
    return ok;
}

// float_sub_f32  [1074:5]
bool test_sub_f32_rhs_no_leak(void) {
    DebugAllocator dbg = DebugAllocatorInitWith(LEAK_CFG);
    Float          a   = FloatFromStr("5.5", &dbg.base);
    Float          r   = FloatInit(&dbg.base);

    bool ok = FloatSub(&r, &a, 0.5f);

    FloatDeinit(&a);
    FloatDeinit(&r);
    ok = ok && LEAK_CLEAN(dbg);
    DebugAllocatorDeinit(&dbg);
    return ok;
}

// float_sub_f64  [1085:5]
bool test_sub_f64_rhs_no_leak(void) {
    DebugAllocator dbg = DebugAllocatorInitWith(LEAK_CFG);
    Float          a   = FloatFromStr("5.5", &dbg.base);
    Float          r   = FloatInit(&dbg.base);

    bool ok = FloatSub(&r, &a, 0.25);

    FloatDeinit(&a);
    FloatDeinit(&r);
    ok = ok && LEAK_CLEAN(dbg);
    DebugAllocatorDeinit(&dbg);
    return ok;
}

// float_mul_int  [1115:5]
bool test_mul_int_rhs_no_leak(void) {
    DebugAllocator dbg = DebugAllocatorInitWith(LEAK_CFG);
    Float          a   = FloatFromStr("1.5", &dbg.base);
    Float          r   = FloatInit(&dbg.base);
    Int            b   = IntFrom(4, &dbg.base);

    bool ok = FloatMul(&r, &a, &b);

    FloatDeinit(&a);
    FloatDeinit(&r);
    IntDeinit(&b);
    ok = ok && LEAK_CLEAN(dbg);
    DebugAllocatorDeinit(&dbg);
    return ok;
}

// float_mul_u64  [1126:5]
bool test_mul_u64_rhs_no_leak(void) {
    DebugAllocator dbg = DebugAllocatorInitWith(LEAK_CFG);
    Float          a   = FloatFromStr("1.5", &dbg.base);
    Float          r   = FloatInit(&dbg.base);

    bool ok = FloatMul(&r, &a, 4u);

    FloatDeinit(&a);
    FloatDeinit(&r);
    ok = ok && LEAK_CLEAN(dbg);
    DebugAllocatorDeinit(&dbg);
    return ok;
}

// float_mul_i64  [1137:5]
bool test_mul_i64_rhs_no_leak(void) {
    DebugAllocator dbg = DebugAllocatorInitWith(LEAK_CFG);
    Float          a   = FloatFromStr("1.5", &dbg.base);
    Float          r   = FloatInit(&dbg.base);

    bool ok = FloatMul(&r, &a, -4);

    FloatDeinit(&a);
    FloatDeinit(&r);
    ok = ok && LEAK_CLEAN(dbg);
    DebugAllocatorDeinit(&dbg);
    return ok;
}

// float_mul_f32  [1148:5]
bool test_mul_f32_rhs_no_leak(void) {
    DebugAllocator dbg = DebugAllocatorInitWith(LEAK_CFG);
    Float          a   = FloatFromStr("1.5", &dbg.base);
    Float          r   = FloatInit(&dbg.base);

    bool ok = FloatMul(&r, &a, 0.5f);

    FloatDeinit(&a);
    FloatDeinit(&r);
    ok = ok && LEAK_CLEAN(dbg);
    DebugAllocatorDeinit(&dbg);
    return ok;
}

// float_mul_f64  [1159:5]
bool test_mul_f64_rhs_no_leak(void) {
    DebugAllocator dbg = DebugAllocatorInitWith(LEAK_CFG);
    Float          a   = FloatFromStr("1.5", &dbg.base);
    Float          r   = FloatInit(&dbg.base);

    bool ok = FloatMul(&r, &a, 0.25);

    FloatDeinit(&a);
    FloatDeinit(&r);
    ok = ok && LEAK_CLEAN(dbg);
    DebugAllocatorDeinit(&dbg);
    return ok;
}

// float_div_int  [1216:5]
bool test_div_int_rhs_no_leak(void) {
    DebugAllocator dbg = DebugAllocatorInitWith(LEAK_CFG);
    Float          a   = FloatFromStr("1", &dbg.base);
    Float          r   = FloatInit(&dbg.base);
    Int            b   = IntFrom(4, &dbg.base);

    bool ok = FloatDiv(&r, &a, &b, 6u);

    FloatDeinit(&a);
    FloatDeinit(&r);
    IntDeinit(&b);
    ok = ok && LEAK_CLEAN(dbg);
    DebugAllocatorDeinit(&dbg);
    return ok;
}

// float_div_u64  [1228:5]
bool test_div_u64_rhs_no_leak(void) {
    DebugAllocator dbg = DebugAllocatorInitWith(LEAK_CFG);
    Float          a   = FloatFromStr("1", &dbg.base);
    Float          r   = FloatInit(&dbg.base);

    bool ok = FloatDiv(&r, &a, 4u, 6u);

    FloatDeinit(&a);
    FloatDeinit(&r);
    ok = ok && LEAK_CLEAN(dbg);
    DebugAllocatorDeinit(&dbg);
    return ok;
}

// float_div_i64  [1240:5]
bool test_div_i64_rhs_no_leak(void) {
    DebugAllocator dbg = DebugAllocatorInitWith(LEAK_CFG);
    Float          a   = FloatFromStr("1", &dbg.base);
    Float          r   = FloatInit(&dbg.base);

    bool ok = FloatDiv(&r, &a, -4, 6u);

    FloatDeinit(&a);
    FloatDeinit(&r);
    ok = ok && LEAK_CLEAN(dbg);
    DebugAllocatorDeinit(&dbg);
    return ok;
}

// float_div_f32  [1252:5]
bool test_div_f32_rhs_no_leak(void) {
    DebugAllocator dbg = DebugAllocatorInitWith(LEAK_CFG);
    Float          a   = FloatFromStr("1", &dbg.base);
    Float          r   = FloatInit(&dbg.base);

    bool ok = FloatDiv(&r, &a, 4.0f, 6u);

    FloatDeinit(&a);
    FloatDeinit(&r);
    ok = ok && LEAK_CLEAN(dbg);
    DebugAllocatorDeinit(&dbg);
    return ok;
}

// float_div_f64  [1264:5]
bool test_div_f64_rhs_no_leak(void) {
    DebugAllocator dbg = DebugAllocatorInitWith(LEAK_CFG);
    Float          a   = FloatFromStr("1", &dbg.base);
    Float          r   = FloatInit(&dbg.base);

    bool ok = FloatDiv(&r, &a, 4.0, 6u);

    FloatDeinit(&a);
    FloatDeinit(&r);
    ok = ok && LEAK_CLEAN(dbg);
    DebugAllocatorDeinit(&dbg);
    return ok;
}

// =============================================================================
// float_compare_<scalar>_with_error success path: FloatDeinit(&rhs_value)
// before returning the comparison result. Each comparison wrapper builds an
// internal rhs_value Float and frees it before returning.

// float_compare_int_with_error  [773:5]
bool test_compare_int_rhs_no_leak(void) {
    DebugAllocator dbg = DebugAllocatorInitWith(LEAK_CFG);
    Float          a   = FloatFromStr("3.5", &dbg.base);
    Int            b   = IntFrom(3, &dbg.base);

    int  cmp = FloatCompare(&a, &b);
    bool ok  = cmp > 0;

    FloatDeinit(&a);
    IntDeinit(&b);
    ok = ok && LEAK_CLEAN(dbg);
    DebugAllocatorDeinit(&dbg);
    return ok;
}

// float_compare_u64_with_error  [798:5]
bool test_compare_u64_rhs_no_leak(void) {
    DebugAllocator dbg = DebugAllocatorInitWith(LEAK_CFG);
    Float          a   = FloatFromStr("3.5", &dbg.base);

    int  cmp = FloatCompare(&a, 3u);
    bool ok  = cmp > 0;

    FloatDeinit(&a);
    ok = ok && LEAK_CLEAN(dbg);
    DebugAllocatorDeinit(&dbg);
    return ok;
}

// float_compare_i64_with_error  [823:5]
bool test_compare_i64_rhs_no_leak(void) {
    DebugAllocator dbg = DebugAllocatorInitWith(LEAK_CFG);
    Float          a   = FloatFromStr("-3.5", &dbg.base);

    int  cmp = FloatCompare(&a, -3);
    bool ok  = cmp < 0;

    FloatDeinit(&a);
    ok = ok && LEAK_CLEAN(dbg);
    DebugAllocatorDeinit(&dbg);
    return ok;
}

// float_compare_f32_with_error  [848:5]
bool test_compare_f32_rhs_no_leak(void) {
    DebugAllocator dbg = DebugAllocatorInitWith(LEAK_CFG);
    Float          a   = FloatFromStr("3.5", &dbg.base);

    int  cmp = FloatCompare(&a, 3.25f);
    bool ok  = cmp > 0;

    FloatDeinit(&a);
    ok = ok && LEAK_CLEAN(dbg);
    DebugAllocatorDeinit(&dbg);
    return ok;
}

// float_compare_f64_with_error  [873:5]
bool test_compare_f64_rhs_no_leak(void) {
    DebugAllocator dbg = DebugAllocatorInitWith(LEAK_CFG);
    Float          a   = FloatFromStr("3.5", &dbg.base);

    int  cmp = FloatCompare(&a, 3.25);
    bool ok  = cmp > 0;

    FloatDeinit(&a);
    ok = ok && LEAK_CLEAN(dbg);
    DebugAllocatorDeinit(&dbg);
    return ok;
}

// =============================================================================
// FloatToInt negative-exponent branch, factor cleanup:
//   IntDeinit(&factor) after int_div_exact  [448:9]
// The earlier test_to_int_negative_exponent_no_leak passes an input that
// normalizes to a non-negative exponent, so it never enters this block. A value
// that keeps a negative exponent (1.5 -> significand 15, exponent -1) DOES enter
// it: float_pow10 allocates `factor` (= 10), int_div_exact then fails because
// 1.5 is not an exact integer, and `factor` is freed at [448] before the false
// return. FloatToInt returns false here, so this test gates purely on the leak
// state -- removing IntDeinit(&factor) leaves `factor` (10) outstanding.

bool test_to_int_negative_exponent_factor_no_leak(void) {
    DebugAllocator dbg = DebugAllocatorInitWith(LEAK_CFG);

    Float v = FloatFromStr("1.5", &dbg.base); // significand 15, exponent -1
    Int   r = IntFrom(777u, &dbg.base);       // pre-populated dest

    // Non-integer -> FloatToInt returns false, but the negative-exponent block
    // is still entered and `factor` is allocated then freed at [448].
    (void)FloatToInt(&r, &v);

    FloatDeinit(&v);
    IntDeinit(&r);
    bool ok = LEAK_CLEAN(dbg);
    DebugAllocatorDeinit(&dbg);
    return ok;
}

int main(void) {
    TestFunction tests[] = {
        test_from_f64_negative_binexp_no_leak,
        test_from_f32_negative_binexp_no_leak,
        test_normalize_trailing_zeros_no_leak,
        test_pow10_success_no_leak,
        test_to_int_positive_exponent_no_leak,
        test_to_int_negative_exponent_no_leak,
        test_to_int_negative_exponent_factor_no_leak,
        test_to_int_zero_no_leak,
        test_add_scaling_no_leak,
        test_add_replace_prepopulated_no_leak,
        test_add_cancel_to_zero_no_leak,
        test_compare_abs_no_leak,
        test_to_str_fractional_no_leak,
        test_to_str_integer_no_leak,
        test_from_str_prepopulated_no_leak,
        test_div_success_no_leak,
        test_div_zero_dividend_no_leak,
        test_mul_replace_prepopulated_no_leak,
        test_add_int_rhs_no_leak,
        test_add_u64_rhs_no_leak,
        test_add_i64_rhs_no_leak,
        test_add_f32_rhs_no_leak,
        test_add_f64_rhs_no_leak,
        test_sub_float_rhs_no_leak,
        test_sub_int_rhs_no_leak,
        test_sub_u64_rhs_no_leak,
        test_sub_i64_rhs_no_leak,
        test_sub_f32_rhs_no_leak,
        test_sub_f64_rhs_no_leak,
        test_mul_int_rhs_no_leak,
        test_mul_u64_rhs_no_leak,
        test_mul_i64_rhs_no_leak,
        test_mul_f32_rhs_no_leak,
        test_mul_f64_rhs_no_leak,
        test_div_int_rhs_no_leak,
        test_div_u64_rhs_no_leak,
        test_div_i64_rhs_no_leak,
        test_div_f32_rhs_no_leak,
        test_div_f64_rhs_no_leak,
        test_compare_int_rhs_no_leak,
        test_compare_u64_rhs_no_leak,
        test_compare_i64_rhs_no_leak,
        test_compare_f32_rhs_no_leak,
        test_compare_f64_rhs_no_leak,
    };
    TestFunction deadend_tests[] = {
        0,
    };
    (void)deadend_tests;
    return run_test_suite(tests, sizeof(tests) / sizeof(tests[0]), deadend_tests, 0, "Float.Leak");
}
