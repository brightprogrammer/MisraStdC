#include <Misra/Std/Allocator/Default.h>
#include <Misra/Std/Container/Float.h>
#include <Misra/Std/Container/Int.h>
#include <Misra/Std/Container/Map.h>
#include <Misra/Std/Log.h>

#include "../Util/FloatTestData.h"
#include "../Util/TestRunner.h"

bool test_float_compare_small_small(void);
bool test_float_compare_very_large_large(void);
bool test_float_compare_very_large_small(void);
bool test_float_compare_wrappers(void);
bool test_float_compare_generic(void);
bool test_float_hash_determinism(void);
bool test_float_hash_distinguishes(void);
bool test_float_hash_as_map_key(void);

bool test_float_compare_small_small(void) {
    WriteFmt("Testing FloatCompare with small floats\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Float a = FloatFromStr("1.23", &alloc.base);
    Float b = FloatFromStr("123e-2", &alloc.base);
    Float c = FloatFromStr("-1.23", &alloc.base);

    bool result = FloatCompare(&a, &b) == 0;
    result      = result && FloatEQ(&a, &b);
    result      = result && (FloatCompare(&c, &a) < 0);
    result      = result && (FloatCompare(&a, &c) > 0);

    FloatDeinit(&a);
    FloatDeinit(&b);
    FloatDeinit(&c);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

bool test_float_compare_very_large_large(void) {
    WriteFmt("Testing FloatCompare with very large floats\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Float a = FloatFromStr(FLOAT_TEST_VERY_LARGE_ONES, &alloc.base);
    Float b = FloatFromStr(FLOAT_TEST_VERY_LARGE_TWOS, &alloc.base);
    Float c = FloatFromStr(FLOAT_TEST_VERY_LARGE_ONES, &alloc.base);

    bool result = FloatLT(&a, &b);
    result      = result && FloatGT(&b, &a);
    result      = result && (FloatCompare(&a, &c) == 0);
    result      = result && FloatEQ(&a, &c);

    FloatDeinit(&a);
    FloatDeinit(&b);
    FloatDeinit(&c);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

bool test_float_compare_very_large_small(void) {
    WriteFmt("Testing FloatCompare with very large and small floats\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Float large          = FloatFromStr(FLOAT_TEST_VERY_LARGE_ONES, &alloc.base);
    Float negative_large = FloatFromStr("-" FLOAT_TEST_VERY_LARGE_ONES, &alloc.base);
    Float small          = FloatFromStr("2.5", &alloc.base);

    bool result = FloatGT(&large, &small);
    result      = result && FloatLT(&negative_large, &small);
    result      = result && FloatNE(&large, &small);

    FloatDeinit(&large);
    FloatDeinit(&negative_large);
    FloatDeinit(&small);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

bool test_float_compare_wrappers(void) {
    WriteFmt("Testing Float compare macros\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Float a        = FloatFromStr("-2", &alloc.base);
    Float b        = FloatFromStr("0.5", &alloc.base);
    Float expected = FloatFromStr("5e-1", &alloc.base);

    bool result = FloatLT(&a, &b);
    result      = result && FloatLE(&a, &b);
    result      = result && FloatGT(&b, &a);
    result      = result && FloatGE(&b, &a);
    result      = result && FloatNE(&a, &b);
    result      = result && FloatEQ(&b, &expected);

    FloatDeinit(&a);
    FloatDeinit(&b);
    FloatDeinit(&expected);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

bool test_float_compare_generic(void) {
    WriteFmt("Testing FloatCompare generic dispatch\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Float value = FloatFromStr("12.5", &alloc.base);
    Float same  = FloatFromStr("12.5", &alloc.base);
    Int   whole = IntFrom(12, &alloc.base);
    Int   next  = IntFrom(13, &alloc.base);

    bool result = (FloatCompare(&value, &same) == 0);
    result      = result && (FloatCompare(&value, &whole) > 0);
    result      = result && (FloatCompare(&value, &next) < 0);
    result      = result && (FloatCompare(&value, 12) > 0);
    result      = result && (FloatCompare(&value, -1) > 0);
    result      = result && (FloatCompare(&value, 12.5f) == 0);
    result      = result && (FloatCompare(&value, 12.5) == 0);
    result      = result && FloatEQ(&value, &same);
    result      = result && FloatEQ(&value, 12.5);
    result      = result && FloatGE(&value, 12.5f);
    result      = result && FloatGT(&value, &whole);
    result      = result && FloatLE(&value, 13);
    result      = result && FloatNE(&value, 12);

    FloatDeinit(&value);
    FloatDeinit(&same);
    IntDeinit(&whole);
    IntDeinit(&next);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Two construction paths for the same value must hash to the same bucket.
bool test_float_hash_determinism(void) {
    WriteFmt("Testing float_hash determinism\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Float a     = FloatFromStr("1.23", &alloc.base);
    Float b     = FloatFromStr("123e-2", &alloc.base);
    Float zero1 = FloatFromStr("0", &alloc.base);
    Float zero2 = FloatFromStr("0", &alloc.base);

    bool result = (float_hash(&a, 0) == float_hash(&b, 0));
    result      = result && (float_hash(&zero1, 0) == float_hash(&zero2, 0));

    FloatDeinit(&a);
    FloatDeinit(&b);
    FloatDeinit(&zero1);
    FloatDeinit(&zero2);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Sign, exponent, and magnitude must each pull the hash apart, so
// +1.5e3 / -1.5e3 / 1.5e2 all land in distinct buckets.
bool test_float_hash_distinguishes(void) {
    WriteFmt("Testing float_hash sensitivity to sign / exponent / magnitude\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Float pos   = FloatFromStr("1.5e3", &alloc.base);
    Float neg   = FloatFromStr("-1.5e3", &alloc.base);
    Float small = FloatFromStr("1.5e2", &alloc.base);
    Float zero  = FloatFromStr("0", &alloc.base);
    Float one   = FloatFromStr("1", &alloc.base);

    u64 h_pos   = float_hash(&pos, 0);
    u64 h_neg   = float_hash(&neg, 0);
    u64 h_small = float_hash(&small, 0);
    u64 h_zero  = float_hash(&zero, 0);
    u64 h_one   = float_hash(&one, 0);

    bool result = (h_pos != h_neg);             // sign matters
    result      = result && (h_pos != h_small); // exponent matters
    result      = result && (h_neg != h_small);
    result      = result && (h_zero != h_one);
    result      = result && (h_zero != h_pos);

    FloatDeinit(&pos);
    FloatDeinit(&neg);
    FloatDeinit(&small);
    FloatDeinit(&zero);
    FloatDeinit(&one);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// End-to-end: plug float_hash + float_compare into a Map and verify
// the GenericHash / GenericCompare-shaped helpers wire in directly.
bool test_float_hash_as_map_key(void) {
    WriteFmt("Testing float_hash as Map<Float, u64> key\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Map(Float, u64) counts = MapInit(float_hash, float_compare, &alloc);

    Float k1 = FloatFromStr("3.14", &alloc.base);
    Float k2 = FloatFromStr("2.71", &alloc.base);
    MapInsertR(&counts, k1, 1u);
    MapInsertR(&counts, k2, 2u);

    Float probe   = FloatFromStr("314e-2", &alloc.base); // same value as k1
    u64  *got     = MapGetFirstPtr(&counts, probe);
    Float missing = FloatFromStr("9.99", &alloc.base);
    u64  *gone    = MapGetFirstPtr(&counts, missing);

    bool result = (got != NULL && *got == 1u);
    result      = result && (gone == NULL);
    result      = result && (MapPairCount(&counts) == 2);

    FloatDeinit(&k1);
    FloatDeinit(&k2);
    FloatDeinit(&probe);
    FloatDeinit(&missing);
    MapDeinit(&counts);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Kills float_compare_u64 return survivor (Float.c:803): comparing a Float
// smaller than the u64 operand must return a negative ordering. Replacing the
// forwarded return with a truthy literal (42) yields a positive result instead.
static bool test_m14_compare_u64_less_than(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    Float small = FloatFrom((u64)5, &alloc.base);
    int   cmp   = FloatCompare(&small, (u64)100);
    bool  ok    = cmp < 0;

    FloatDeinit(&small);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// Equality control for float_compare_u64: equal magnitudes compare 0 (a
// truthy-literal replacement would report 42 here too).
static bool test_m14_compare_u64_equal(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    Float value = FloatFrom((u64)42, &alloc.base);
    int   cmp   = FloatCompare(&value, (u64)42);
    bool  ok    = cmp == 0;

    FloatDeinit(&value);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// PROBE for equivalent mutants 762:16 / 812:16 / 837:16:
//   `*error = false;` -> `*error = 42;` in the with-error compare wrappers.
// On every return path the out-param `*error` is overwritten after that store
// (success: inner float_compare_with_error re-sets it to false; conversion
// failure: it is set to true). So the store is dead and no observation of
// *error can distinguish the mutant. This probe pins the OBSERVABLE contract
// (error stays false on a successful equal compare) so it keeps passing under
// the emulated mutant -- demonstrating equivalence, not a kill.
static bool test_ff_probe_compare_error_paths(void) {
    WriteFmt("Probe: with-error compare keeps error=false on success\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Float v   = FloatFromStr("3", &alloc.base);
    Int   i3  = IntFrom(3, &alloc.base);
    bool  err = true;
    bool  ok  = true;

    int c1 = float_compare_int_with_error(&v, &i3, &err);
    ok     = ok && (c1 == 0) && (err == false);

    err    = true;
    int c2 = float_compare_i64_with_error(&v, (i64)3, &err);
    ok     = ok && (c2 == 0) && (err == false);

    err    = true;
    int c3 = float_compare_f32_with_error(&v, 3.0f, &err);
    ok     = ok && (c3 == 0) && (err == false);

    IntDeinit(&i3);
    FloatDeinit(&v);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// float_try_abs_compare, FloatIsZero(lhs) short-circuit (L225 col 9):
// comparing a nonzero magnitude against zero. float_compare_with_error has
// already ruled out the both-zero case, so abs_compare sees a nonzero lhs
// and a zero rhs. The cxx_replace_scalar_call mutation turns FloatIsZero(lhs)
// into 42 (truthy); combined with FloatIsZero(rhs)==true it enters the
// both-zero branch and returns 0 instead of the correct +1.
static bool test_m5_compare_nonzero_vs_zero(void) {
    WriteFmt("Testing FloatCompare(5.0, 0.0) is strictly greater\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Float a = FloatFrom(5.0, &alloc.base);
    Float b = FloatFrom(0.0, &alloc.base);

    int  cmp    = FloatCompare(&a, &b);
    bool result = (cmp > 0);

    FloatDeinit(&a);
    FloatDeinit(&b);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// float_try_abs_compare, FloatIsZero(rhs) short-circuit (L225 col 29):
// mirror of the above with a zero lhs and a nonzero rhs. The mutation turns
// FloatIsZero(rhs) into 42; combined with FloatIsZero(lhs)==true it enters
// the both-zero branch and returns 0 instead of the correct -1.
static bool test_m5_compare_zero_vs_nonzero(void) {
    WriteFmt("Testing FloatCompare(0.0, 5.0) is strictly less\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Float a = FloatFrom(0.0, &alloc.base);
    Float b = FloatFrom(5.0, &alloc.base);

    int  cmp    = FloatCompare(&a, &b);
    bool result = (cmp < 0);

    FloatDeinit(&a);
    FloatDeinit(&b);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Kills float_compare_u64_with_error line 797: the comparison result is
// assigned to `cmp` and returned verbatim. A value-42 (cxx_assign_const) or
// scalar-call-to-42 replacement makes the function return 42 instead of the
// real ordering. Asserting the exact ordering value (+1) catches it.
static bool test_m6_compare_u64_greater(void) {
    WriteFmt("Testing FloatCompare(Float, u64) returns +1 when greater\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Float lhs = FloatFromStr("5", &alloc.base);
    int   cmp = FloatCompare(&lhs, (u64)3);

    bool result = (cmp == 1);

    FloatDeinit(&lhs);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Equality path: real code returns 0. A 42 return would fail this exact check.
static bool test_m6_compare_u64_equal(void) {
    WriteFmt("Testing FloatCompare(Float, u64) returns 0 when equal\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Float lhs = FloatFromStr("5", &alloc.base);
    int   cmp = FloatCompare(&lhs, (u64)5);

    bool result = (cmp == 0);

    FloatDeinit(&lhs);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Less-than path: real code returns -1. The mutated return of 42 fails this.
static bool test_m6_compare_u64_less(void) {
    WriteFmt("Testing FloatCompare(Float, u64) returns -1 when less\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Float lhs = FloatFromStr("5", &alloc.base);
    int   cmp = FloatCompare(&lhs, (u64)9);

    bool result = (cmp == -1);

    FloatDeinit(&lhs);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Exercise the explicit-error overload on the success path. The error flag must
// be cleared (false) and the ordering exact. Reinforces the 797 assignment kill
// while also pinning the error contract on the success path.
static bool test_m6_compare_u64_no_error_flag(void) {
    WriteFmt("Testing FloatCompare(Float, u64, &error) clears error on success\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Float lhs   = FloatFromStr("7", &alloc.base);
    bool  error = true;
    int   cmp   = FloatCompare(&lhs, (u64)2, &error);

    bool result = (cmp == 1) && (error == false);

    FloatDeinit(&lhs);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// L707:16 cxx_assign_const: `*error = false;` -> `*error = 42`.
// A valid comparison must leave *error cleared.
static bool test_m7_compare_error_cleared_on_success(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    Float a   = FloatFromStr("1.5", &alloc.base);
    Float b   = FloatFromStr("2.5", &alloc.base);
    bool  err = true;

    int  cmp = FloatCompare(&a, &b, &err);
    bool ok  = (cmp == -1) && (err == false);

    FloatDeinit(&a);
    FloatDeinit(&b);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// L713:9 cxx_replace_scalar_call: `FloatIsZero(lhs)` -> 42 (truthy).
// lhs is NON-zero, rhs IS zero. Real: false&&true -> skip, real ordering 1.
// Mutant: 42&&true -> returns 0. Assert the real positive ordering.
static bool test_m7_compare_nonzero_lhs_zero_rhs(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    Float a = FloatFromStr("3", &alloc.base); // non-zero
    Float b = FloatFromStr("0", &alloc.base); // zero

    int  cmp = FloatCompare(&a, &b);
    bool ok  = (cmp == 1);

    FloatDeinit(&a);
    FloatDeinit(&b);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// L713:29 cxx_replace_scalar_call: `FloatIsZero(rhs)` -> 42 (truthy).
// lhs IS zero, rhs is NON-zero. Real: true&&false -> skip, ordering -1.
// Mutant: true&&42 -> returns 0. Assert the real negative ordering.
static bool test_m7_compare_zero_lhs_nonzero_rhs(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    Float a = FloatFromStr("0", &alloc.base); // zero
    Float b = FloatFromStr("4", &alloc.base); // non-zero

    int  cmp = FloatCompare(&a, &b);
    bool ok  = (cmp == -1);

    FloatDeinit(&a);
    FloatDeinit(&b);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// L726:35 cxx_minus_to_noop: `return FloatIsNegative(lhs) ? -cmp : cmp;`
// the `-cmp` becomes `cmp`. Two negatives where lhs is more negative:
// |lhs|>|rhs| -> abs cmp = 1; real negates -> -1; mutant keeps 1.
static bool test_m7_compare_two_negatives_sign_flip(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    Float a = FloatFromStr("-5", &alloc.base);
    Float b = FloatFromStr("-2", &alloc.base);

    int  cmp = FloatCompare(&a, &b); // -5 < -2 -> -1
    bool ok  = (cmp == -1);

    FloatDeinit(&a);
    FloatDeinit(&b);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// Companion: two negatives where lhs is the larger (less negative) one,
// pins the magnitude path so the negate must yield +1 for -2 vs -5.
static bool test_m7_compare_two_negatives_other_order(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    Float a = FloatFromStr("-2", &alloc.base);
    Float b = FloatFromStr("-5", &alloc.base);

    int  cmp = FloatCompare(&a, &b); // -2 > -5 -> 1
    bool ok  = (cmp == 1);

    FloatDeinit(&a);
    FloatDeinit(&b);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

//
// float_compare_int_with_error clears *error to false on a successful
// comparison (line 762 `*error = false`). If that assignment is mutated
// away (to a truthy value) the flag would read true after a successful
// compare. We exercise three orderings to also pin the returned cmp.
//
static bool test_m8_compare_int_error_cleared(void) {
    WriteFmt("Testing float_compare_int_with_error clears error and orders correctly\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Float lhs  = FloatFromStr("7", &alloc.base);
    Int   less = IntFrom(3, &alloc.base);
    Int   same = IntFrom(7, &alloc.base);
    Int   more = IntFrom(9, &alloc.base);

    bool error = true;
    int  cmp   = FloatCompare(&lhs, &less, &error);
    bool ok    = (cmp > 0) && (error == false);

    error = true;
    cmp   = FloatCompare(&lhs, &same, &error);
    ok    = ok && (cmp == 0) && (error == false);

    error = true;
    cmp   = FloatCompare(&lhs, &more, &error);
    ok    = ok && (cmp < 0) && (error == false);

    FloatDeinit(&lhs);
    IntDeinit(&less);
    IntDeinit(&same);
    IntDeinit(&more);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

//
// float_compare_i64_with_error clears *error to false on a successful
// comparison (line 812 `*error = false`). Same shape against signed
// scalar operands so the i64 generic slot is selected.
//
static bool test_m8_compare_i64_error_cleared(void) {
    WriteFmt("Testing float_compare_i64_with_error clears error and orders correctly\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Float lhs = FloatFromStr("-4", &alloc.base);

    bool error = true;
    int  cmp   = FloatCompare(&lhs, (signed long long)-10, &error);
    bool ok    = (cmp > 0) && (error == false);

    error = true;
    cmp   = FloatCompare(&lhs, (signed long long)-4, &error);
    ok    = ok && (cmp == 0) && (error == false);

    error = true;
    cmp   = FloatCompare(&lhs, (signed long long)5, &error);
    ok    = ok && (cmp < 0) && (error == false);

    FloatDeinit(&lhs);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

//
// float_compare_f32_with_error clears *error to false on a successful
// comparison (line 837 `*error = false`). Use exactly representable
// binary fractions so the conversion is exact.
//
static bool test_m8_compare_f32_error_cleared(void) {
    WriteFmt("Testing float_compare_f32_with_error clears error and orders correctly\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Float lhs = FloatFromStr("0.5", &alloc.base);

    bool error = true;
    int  cmp   = FloatCompare(&lhs, 0.25f, &error);
    bool ok    = (cmp > 0) && (error == false);

    error = true;
    cmp   = FloatCompare(&lhs, 0.5f, &error);
    ok    = ok && (cmp == 0) && (error == false);

    error = true;
    cmp   = FloatCompare(&lhs, 0.75f, &error);
    ok    = ok && (cmp < 0) && (error == false);

    FloatDeinit(&lhs);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

int main(void) {
    WriteFmt("[INFO] Starting Float.Compare tests\n\n");

    TestFunction tests[] = {
        test_float_compare_small_small,
        test_float_compare_very_large_large,
        test_float_compare_very_large_small,
        test_float_compare_wrappers,
        test_float_compare_generic,
        test_float_hash_determinism,
        test_float_hash_distinguishes,
        test_float_hash_as_map_key,
        test_m14_compare_u64_less_than,
        test_m14_compare_u64_equal,
        test_ff_probe_compare_error_paths,
        test_m5_compare_nonzero_vs_zero,
        test_m5_compare_zero_vs_nonzero,
        test_m6_compare_u64_greater,
        test_m6_compare_u64_equal,
        test_m6_compare_u64_less,
        test_m6_compare_u64_no_error_flag,
        test_m7_compare_error_cleared_on_success,
        test_m7_compare_nonzero_lhs_zero_rhs,
        test_m7_compare_zero_lhs_nonzero_rhs,
        test_m7_compare_two_negatives_sign_flip,
        test_m7_compare_two_negatives_other_order,
        test_m8_compare_int_error_cleared,
        test_m8_compare_i64_error_cleared,
        test_m8_compare_f32_error_cleared,
    };

    int total_tests = sizeof(tests) / sizeof(tests[0]);
    return run_test_suite(tests, total_tests, NULL, 0, "Float.Compare");
}
