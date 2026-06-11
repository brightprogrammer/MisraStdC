#include <Misra/Std/Allocator/Default.h>
#include <Misra/Std/Zstr.h>
#include <Misra/Std/Container/Float.h>
#include <Misra/Std/Container/Int.h>
#include <Misra/Std/Log.h>

#include "../Util/FloatTestData.h"
#include "../Util/TestRunner.h"

bool test_float_from_unsigned_integer(void);
bool test_float_from_signed_integer(void);
bool test_float_from_int_container(void);
bool test_float_to_int_exact(void);
bool test_float_to_int_fractional_failure(void);
bool test_float_to_int_negative_failure(void);
bool test_float_string_round_trip(void);
bool test_float_try_to_str_allocator_inheritance(void);
bool test_float_very_large_string_round_trip(void);
bool test_float_scientific_parse(void);
bool test_float_from_str_invalid(void);
bool test_float_from_str_null(void);
bool test_float_try_from_str_null(void);

bool test_float_from_unsigned_integer(void) {
    WriteFmt("Testing FloatFrom with unsigned integer\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Float value = float_from_u64(42, ALLOCATOR_OF(&alloc));
    Str   text  = FloatToStr(&value);

    bool result = ZstrCompare(StrBegin(&text), "42") == 0;
    result      = result && !FloatIsNegative(&value);

    StrDeinit(&text);
    FloatDeinit(&value);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

bool test_float_from_signed_integer(void) {
    WriteFmt("Testing FloatFrom with signed integer\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Float value = float_from_i64(-42, ALLOCATOR_OF(&alloc));
    Str   text  = FloatToStr(&value);

    bool result = ZstrCompare(StrBegin(&text), "-42") == 0;
    result      = result && FloatIsNegative(&value);

    StrDeinit(&text);
    FloatDeinit(&value);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

bool test_float_from_int_container(void) {
    WriteFmt("Testing FloatFrom with Int container\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int   integer = IntFromStr("12345678901234567890", ALLOCATOR_OF(&alloc));
    Float value   = float_from_int(&integer, ALLOCATOR_OF(&alloc));
    Str   text    = FloatToStr(&value);

    bool result = ZstrCompare(StrBegin(&text), "12345678901234567890") == 0;

    IntDeinit(&integer);
    StrDeinit(&text);
    FloatDeinit(&value);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

bool test_float_to_int_exact(void) {
    WriteFmt("Testing FloatToInt exact conversion\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Float value        = FloatFromStr("1234500e-2", ALLOCATOR_OF(&alloc));
    Int   result_value = IntInit(ALLOCATOR_OF(&alloc));
    Str   text         = StrInit(ALLOCATOR_OF(&alloc));

    bool result = FloatToInt(&result_value, &value);
    text        = IntToStr(&result_value);
    result      = result && (ZstrCompare(StrBegin(&text), "12345") == 0);

    FloatDeinit(&value);
    IntDeinit(&result_value);
    StrDeinit(&text);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

bool test_float_to_int_fractional_failure(void) {
    WriteFmt("Testing FloatToInt fractional failure handling\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Float value        = FloatFromStr("123.45", ALLOCATOR_OF(&alloc));
    Int   result_value = IntFrom(99, ALLOCATOR_OF(&alloc));

    bool result = !FloatToInt(&result_value, &value);
    result      = result && IntEQ(&result_value, 99);

    FloatDeinit(&value);
    IntDeinit(&result_value);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

bool test_float_to_int_negative_failure(void) {
    WriteFmt("Testing FloatToInt negative failure handling\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Float value        = FloatFromStr("-42", ALLOCATOR_OF(&alloc));
    Int   result_value = IntFrom(99, ALLOCATOR_OF(&alloc));

    bool result = !FloatToInt(&result_value, &value);
    result      = result && IntEQ(&result_value, 99);

    FloatDeinit(&value);
    IntDeinit(&result_value);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

bool test_float_string_round_trip(void) {
    WriteFmt("Testing Float string round trip\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Float value = FloatFromStr("-123.45", ALLOCATOR_OF(&alloc));
    Str   text  = FloatToStr(&value);

    bool result = ZstrCompare(StrBegin(&text), "-123.45") == 0;

    StrDeinit(&text);
    FloatDeinit(&value);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

bool test_float_try_to_str_allocator_inheritance(void) {
    WriteFmt("Testing FloatTryToStr allocator behavior\n");

    DefaultAllocator alloc = DefaultAllocatorInit();
    Str              text;
    bool             ok;

    // intentional bypass: no public setter on `Allocator` for effort /
    // retry_limit -- pre-seeded directly so the inheritance path below
    // can be observed end-to-end.
    alloc.base.effort      = ALLOCATOR_EFFORT_RETRY;
    alloc.base.retry_limit = 5;

    Float value = FloatFromStr("-123.45", ALLOCATOR_OF(&alloc));

    ok = float_try_to_str(&text, &value, ALLOCATOR_OF(&alloc));

    bool result = ok && (ZstrCompare(StrBegin(&text), "-123.45") == 0) &&
                  (StrAllocator(&text)->effort == alloc.base.effort) &&
                  (StrAllocator(&text)->retry_limit == alloc.base.retry_limit);

    StrDeinit(&text);
    FloatDeinit(&value);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

bool test_float_very_large_string_round_trip(void) {
    WriteFmt("Testing Float very large string round trip\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Float value = FloatFromStr(FLOAT_TEST_VERY_LARGE_ONES, ALLOCATOR_OF(&alloc));
    Str   text  = FloatToStr(&value);

    bool result = ZstrCompare(StrBegin(&text), FLOAT_TEST_VERY_LARGE_ONES) == 0;

    StrDeinit(&text);
    FloatDeinit(&value);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

bool test_float_scientific_parse(void) {
    WriteFmt("Testing Float scientific parsing\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Float value = FloatFromStr("1.2300e3", ALLOCATOR_OF(&alloc));
    Str   text  = FloatToStr(&value);

    bool result = ZstrCompare(StrBegin(&text), "1230") == 0;

    StrDeinit(&text);
    FloatDeinit(&value);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

bool test_float_from_str_invalid(void) {
    WriteFmt("Testing FloatFromStr invalid format handling\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Float parsed = FloatFromStr("12.3.4", ALLOCATOR_OF(&alloc));
    Float value  = FloatInit(ALLOCATOR_OF(&alloc));
    bool  result = !FloatTryFromStr(&value, "12.3.4");

    result = result && FloatIsZero(&parsed);
    result = result && FloatIsZero(&value);

    FloatDeinit(&parsed);
    FloatDeinit(&value);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

bool test_float_from_str_null(void) {
    WriteFmt("Testing FloatFromStr NULL handling\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    FloatFromStr((Zstr)NULL, ALLOCATOR_OF(&alloc));
    DefaultAllocatorDeinit(&alloc);
    return false;
}

bool test_float_try_from_str_null(void) {
    WriteFmt("Testing FloatTryFromStr NULL handling\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Float value = FloatInit(ALLOCATOR_OF(&alloc));
    FloatTryFromStr(&value, (Zstr)NULL);
    FloatDeinit(&value);
    DefaultAllocatorDeinit(&alloc);
    return false;
}

// float_normalize zero branch: `value->negative = false` (line 254).
// 0 * (negative) produces a pre-normalize zero whose sign flag is true;
// normalize must clear it so the canonical zero hashes identically to a
// freshly parsed "0". float_hash mixes the sign byte, so the assign-const
// mutant (negative -> 42, truthy) flips that byte -> hashes differ -> killed.
static bool test_m10_normalize_zero_sign_hash(void) {
    WriteFmt("Testing float_normalize canonicalises zero sign\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Float zero    = FloatFromStr("0", &alloc.base);
    Float neg     = FloatFromStr("-7.5", &alloc.base);
    Float product = FloatInit(&alloc.base);
    Float canon   = FloatFromStr("0", &alloc.base);

    bool ok = FloatMul(&product, &zero, &neg);
    ok      = ok && FloatIsZero(&product);

    u64 h_product = float_hash(&product, 0);
    u64 h_canon   = float_hash(&canon, 0);
    ok            = ok && (h_product == h_canon);

    FloatDeinit(&zero);
    FloatDeinit(&neg);
    FloatDeinit(&product);
    FloatDeinit(&canon);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// Kills line 387: removing float_normalize(&result) in float_from_int.
// float_from_int(100) keeps significand=100, exponent=0 without normalize.
// With normalize the trailing zeros are trimmed: significand=1, exponent=2.
// The rendered string is identical either way, but FloatExponent exposes the
// raw exponent, so a normalized result reports exponent 2 and the
// un-normalized (mutant) result reports exponent 0.
static bool test_m13_from_int_normalizes_exponent(void) {
    WriteFmt("Testing float_from_int trims trailing zeros (exponent)\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int   whole = IntFrom(100, &alloc.base);
    Float value = FloatFrom(&whole, &alloc.base);

    // 100 == 1 * 10^2 after normalization.
    bool result = FloatExponent(&value) == 2;

    IntDeinit(&whole);
    FloatDeinit(&value);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Reinforces line 387 / 383 / 352: the normalized value must still equal the
// source integer (renders as "100"), guarding against a normalization that
// changes the value rather than just the representation.
static bool test_m13_from_int_value_preserved(void) {
    WriteFmt("Testing float_from_int preserves numeric value\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int   whole = IntFrom(100, &alloc.base);
    Float value = FloatFrom(&whole, &alloc.base);
    Str   text  = FloatToStr(&value);

    bool result = ZstrCompare(StrBegin(&text), "100") == 0;
    result      = result && (FloatCompare(&value, &whole) == 0);

    StrDeinit(&text);
    IntDeinit(&whole);
    FloatDeinit(&value);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// A larger trailing-zero integer: 2000 normalizes to significand=2,
// exponent=3. FloatExponent must report 3 (mutant: 0). Negate afterward to
// confirm the normalized representation still renders correctly.
static bool test_m13_from_int_negative_value(void) {
    WriteFmt("Testing float_from_int on multi-zero value then negate\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int   whole = IntFrom(2000, &alloc.base);
    Float value = FloatFrom(&whole, &alloc.base);

    bool result = FloatExponent(&value) == 3;

    FloatNegate(&value);
    Str text = FloatToStr(&value);

    result = result && FloatIsNegative(&value);
    result = result && (ZstrCompare(StrBegin(&text), "-2000") == 0);
    result = result && (FloatExponent(&value) == 3);

    StrDeinit(&text);
    IntDeinit(&whole);
    FloatDeinit(&value);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Kills float_from_u64 normalize survivor (Float.c:368): float_from_u64 must
// trim trailing zeros, rescaling 100 -> significand 1 with exponent 2. Without
// the float_normalize(&result) call the exponent stays 0.
static bool test_m14_from_u64_normalizes_exponent(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    Float value = FloatFrom((u64)100, &alloc.base);
    bool  ok    = FloatExponent(&value) == 2;

    FloatDeinit(&value);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// Same lever via the printed form, to pin the value identity (not just exp).
static bool test_m14_from_u64_value_roundtrip(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    Float value = FloatFrom((u64)100, &alloc.base);
    Str   text  = FloatToStr(&value);
    bool  ok    = ZstrCompare(StrBegin(&text), "100") == 0;

    StrDeinit(&text);
    FloatDeinit(&value);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// Kills float_from_i64 normalize survivor (Float.c:376): float_from_i64 of -200
// must rescale to significand 2, exponent 2 (printed "-200"). Without
// float_normalize the exponent stays 0.
static bool test_m14_from_i64_normalizes_exponent(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    Float value = FloatFrom((i64)-200, &alloc.base);
    bool  ok    = FloatExponent(&value) == 2;

    FloatDeinit(&value);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// Kills float_try_from_str_str return survivor (Float.c:591): parsing a
// malformed Str must return false. Replacing the return with a truthy literal
// makes it report success on garbage.
static bool test_m14_try_from_str_str_rejects_invalid(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    Str   text  = StrInitFromZstr("not-a-number", &alloc.base);
    Float value = FloatInit(&alloc.base);
    bool  ok    = (FloatTryFromStr(&value, &text) == false);

    FloatDeinit(&value);
    StrDeinit(&text);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// Positive control for the same return path: a valid Str must parse true and
// yield the expected value, so the survivor can't be killed by always-false.
static bool test_m14_try_from_str_str_accepts_valid(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    Str   text  = StrInitFromZstr("3.14", &alloc.base);
    Float value = FloatInit(&alloc.base);
    bool  ok    = FloatTryFromStr(&value, &text);

    Str printed = FloatToStr(&value);
    ok          = ok && (ZstrCompare(StrBegin(&printed), "3.14") == 0);

    StrDeinit(&printed);
    FloatDeinit(&value);
    StrDeinit(&text);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// Kills float_try_to_str line 659 (`i < StrLen(&digits)` -> `<=`). For "3.14"
// the significand digits are "314" and exponent is -2, so split == 1: the
// integer part is one digit and the fractional loop walks indices [1, len).
// Swapping `<` to `<=` walks one index past the end, appending an extra
// character. Asserting both the exact text AND the exact length (4) detects the
// trailing garbage even if it stringifies as a terminator.
static bool test_m6_to_str_neg_exp_split(void) {
    WriteFmt("Testing FloatToStr fractional loop bound for 3.14\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Float value = FloatFromStr("3.14", &alloc.base);
    Str   text  = FloatToStr(&value);

    bool result = (ZstrCompare(StrBegin(&text), "3.14") == 0) && (StrLen(&text) == 4);

    StrDeinit(&text);
    FloatDeinit(&value);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Kills float_try_to_str line 669 trio (init i=0 -> 42; `-split` -> `split`;
// `i++` -> `i--`). For "0.001" the significand digit is "1", exponent -3, so
// split == 1 + (-3) == -2 and the leading-zero loop must emit `-split` == 2
// zeros, yielding "0.001" (length 5). Under init-to-42 or minus-to-noop the
// loop body never runs -> "0.1"; under post-inc-to-post-dec the loop never
// terminates. Asserting exact text and length 5 catches all three.
static bool test_m6_to_str_leading_zeros(void) {
    WriteFmt("Testing FloatToStr leading-zero loop for 0.001\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Float value = FloatFromStr("0.001", &alloc.base);
    Str   text  = FloatToStr(&value);

    bool result = (ZstrCompare(StrBegin(&text), "0.001") == 0) && (StrLen(&text) == 5);

    StrDeinit(&text);
    FloatDeinit(&value);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Companion coverage for the non-negative-exponent branch: "100" must print its
// trailing zeros exactly. Guards the positive-exponent path of float_try_to_str.
static bool test_m6_to_str_positive_exp(void) {
    WriteFmt("Testing FloatToStr positive-exponent trailing zeros for 100\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    // 1e2 keeps significand 1 with exponent 2, exercising the >= 0 zero loop.
    Float value = FloatFromStr("1e2", &alloc.base);
    Str   text  = FloatToStr(&value);

    bool result = (ZstrCompare(StrBegin(&text), "100") == 0) && (StrLen(&text) == 3);

    StrDeinit(&text);
    FloatDeinit(&value);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Equal values must hash equal (sanity guard the mutations must not break).
static bool test_m7_hash_equal_values_equal(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    Float a = FloatFromStr("3.14", &alloc.base);
    Float b = FloatFromStr("3.14", &alloc.base);

    bool ok = float_hash(&a, 0) == float_hash(&b, 0);

    FloatDeinit(&a);
    FloatDeinit(&b);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// L745:39 i++ -> i-- (loop runs once, only byte0),
// L746:27 >> -> << (high-byte shifts out to 0),
// L746:33 *  -> /  (i/8 == 0 for all i<8, only byte0).
// Two values share significand=1 and sign, differing ONLY in a HIGH exponent
// byte (256 = 0x100, byte1 set, byte0 == 0 like the exp-0 value). Real code
// mixes byte1 and hashes them apart; each mutation collapses to byte0 only,
// which is identical for both -> mutant hashes them equal.
static bool test_m7_hash_high_exponent_byte(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    Float a = FloatFromStr("1", &alloc.base);     // significand 1, exp 0
    Float b = FloatFromStr("1e256", &alloc.base); // significand 1, exp 256

    // Construction guards: same magnitude-1 significand, exponents differ only
    // in a high byte (0 vs 256), so the only hash input that changes is an
    // exponent byte above byte0.
    bool built_ok = (FloatExponent(&a) == 0) && (FloatExponent(&b) == 256);

    bool distinct = float_hash(&a, 0) != float_hash(&b, 0);

    FloatDeinit(&a);
    FloatDeinit(&b);
    DefaultAllocatorDeinit(&alloc);
    return built_ok && distinct;
}

// Companion low-byte guard: exponents differing in byte0 (exp 0 vs exp 1)
// must also hash apart, pinning the per-byte exponent mixing generally.
static bool test_m7_hash_low_exponent_byte(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    Float a = FloatFromStr("1", &alloc.base);   // exp 0
    Float b = FloatFromStr("1e1", &alloc.base); // exp 1

    bool built_ok = (FloatExponent(&a) == 0) && (FloatExponent(&b) == 1);
    bool distinct = float_hash(&a, 0) != float_hash(&b, 0);

    FloatDeinit(&a);
    FloatDeinit(&b);
    DefaultAllocatorDeinit(&alloc);
    return built_ok && distinct;
}

// float_try_from_str_impl `bool saw_digit = false` (line 465): a digitless
// string must be rejected by the `!saw_digit` guard; the init-const mutation
// (saw_digit -> truthy) would accept it.
static bool test_fg_465_digitless_dot_rejected(void) {
    WriteFmt("Testing float_try_from_str rejects digitless \".\"\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Float value = FloatInit(&alloc.base);
    bool  ok    = (FloatTryFromStr(&value, ".") == false);

    FloatDeinit(&value);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

static bool test_fg_465_digitless_sign_dot_rejected(void) {
    WriteFmt("Testing float_try_from_str rejects digitless \"+.\"\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Float value = FloatInit(&alloc.base);
    bool  ok    = (FloatTryFromStr(&value, "+.") == false);

    FloatDeinit(&value);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

static bool test_fg_465_with_digit_accepted(void) {
    WriteFmt("Testing float_try_from_str still accepts \"1.\"\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Float value = FloatInit(&alloc.base);
    bool  ok    = FloatTryFromStr(&value, "1.");

    Str text = FloatToStr(&value);
    ok       = ok && (ZstrCompare(StrBegin(&text), "1") == 0);

    StrDeinit(&text);
    FloatDeinit(&value);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

int main(void) {
    WriteFmt("[INFO] Starting Float.Convert tests\n\n");

    TestFunction tests[] = {
        test_float_from_unsigned_integer,
        test_float_from_signed_integer,
        test_float_from_int_container,
        test_float_to_int_exact,
        test_float_to_int_fractional_failure,
        test_float_to_int_negative_failure,
        test_float_string_round_trip,
        test_float_try_to_str_allocator_inheritance,
        test_float_very_large_string_round_trip,
        test_float_scientific_parse,
        test_float_from_str_invalid,
        test_m10_normalize_zero_sign_hash,
        test_m13_from_int_normalizes_exponent,
        test_m13_from_int_value_preserved,
        test_m13_from_int_negative_value,
        test_m14_from_u64_normalizes_exponent,
        test_m14_from_u64_value_roundtrip,
        test_m14_from_i64_normalizes_exponent,
        test_m14_try_from_str_str_rejects_invalid,
        test_m14_try_from_str_str_accepts_valid,
        test_m6_to_str_neg_exp_split,
        test_m6_to_str_leading_zeros,
        test_m6_to_str_positive_exp,
        test_m7_hash_equal_values_equal,
        test_m7_hash_high_exponent_byte,
        test_m7_hash_low_exponent_byte,
        test_fg_465_digitless_dot_rejected,
        test_fg_465_digitless_sign_dot_rejected,
        test_fg_465_with_digit_accepted,
    };

    // NULL-input: strict contract = LOG_FATAL; deadend driver catches.
    TestFunction deadend_tests[] = {
        test_float_from_str_null,
        test_float_try_from_str_null,
    };

    int total_tests         = sizeof(tests) / sizeof(tests[0]);
    int total_deadend_tests = sizeof(deadend_tests) / sizeof(deadend_tests[0]);

    return run_test_suite(tests, total_tests, deadend_tests, total_deadend_tests, "Float.Convert");
}
