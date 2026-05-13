#include <Misra/Std/Container/Float.h>
#include <Misra/Std/Container/Int.h>
#include <Misra/Std/Log.h>
#include <string.h>

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

bool test_float_from_unsigned_integer(void) {
    WriteFmt("Testing FloatFrom with unsigned integer\n");

    Float value = FloatFrom(42);
    Str   text  = FloatToStr(&value);

    bool result = ZstrCompare(text.data, "42") == 0;
    result      = result && !FloatIsNegative(&value);

    StrDeinit(&text);
    FloatDeinit(&value);
    return result;
}

bool test_float_from_signed_integer(void) {
    WriteFmt("Testing FloatFrom with signed integer\n");

    Float value = FloatFrom(-42);
    Str   text  = FloatToStr(&value);

    bool result = ZstrCompare(text.data, "-42") == 0;
    result      = result && FloatIsNegative(&value);

    StrDeinit(&text);
    FloatDeinit(&value);
    return result;
}

bool test_float_from_int_container(void) {
    WriteFmt("Testing FloatFrom with Int container\n");

    Int   integer = IntFromStr("12345678901234567890");
    Float value   = FloatFrom(&integer);
    Str   text    = FloatToStr(&value);

    bool result = ZstrCompare(text.data, "12345678901234567890") == 0;

    IntDeinit(&integer);
    StrDeinit(&text);
    FloatDeinit(&value);
    return result;
}

bool test_float_to_int_exact(void) {
    WriteFmt("Testing FloatToInt exact conversion\n");

    Float value        = FloatFromStr("1234500e-2");
    Int   result_value = IntInit();
    Str   text         = StrInit();

    bool result = FloatToInt(&result_value, &value);
    text        = IntToStr(&result_value);
    result      = result && (ZstrCompare(text.data, "12345") == 0);

    FloatDeinit(&value);
    IntDeinit(&result_value);
    StrDeinit(&text);
    return result;
}

bool test_float_to_int_fractional_failure(void) {
    WriteFmt("Testing FloatToInt fractional failure handling\n");

    Float value        = FloatFromStr("123.45");
    Int   result_value = IntFrom(99);

    bool result = !FloatToInt(&result_value, &value);
    result      = result && IntEQ(&result_value, 99);

    FloatDeinit(&value);
    IntDeinit(&result_value);
    return result;
}

bool test_float_to_int_negative_failure(void) {
    WriteFmt("Testing FloatToInt negative failure handling\n");

    Float value        = FloatFromStr("-42");
    Int   result_value = IntFrom(99);

    bool result = !FloatToInt(&result_value, &value);
    result      = result && IntEQ(&result_value, 99);

    FloatDeinit(&value);
    IntDeinit(&result_value);
    return result;
}

bool test_float_string_round_trip(void) {
    WriteFmt("Testing Float string round trip\n");

    Float value = FloatFromStr("-123.45");
    Str   text  = FloatToStr(&value);

    bool result = ZstrCompare(text.data, "-123.45") == 0;

    StrDeinit(&text);
    FloatDeinit(&value);
    return result;
}

bool test_float_try_to_str_allocator_inheritance(void) {
    WriteFmt("Testing FloatTryToStr allocator behavior\n");

    Float     value = FloatFromStr("-123.45");
    Str       text;
    Allocator alloc = DefaultAllocator();
    bool      ok;

    alloc.effort      = ALLOCATOR_EFFORT_RETRY;
    alloc.retry_limit = 5;

    ok = FloatTryToStrAlloc(&text, &value, alloc);

    bool result = ok && (ZstrCompare(text.data, "-123.45") == 0) && (text.allocator.effort == alloc.effort) &&
                  (text.allocator.retry_limit == alloc.retry_limit);

    StrDeinit(&text);
    FloatDeinit(&value);
    return result;
}

bool test_float_very_large_string_round_trip(void) {
    WriteFmt("Testing Float very large string round trip\n");

    Float value = FloatFromStr(FLOAT_TEST_VERY_LARGE_ONES);
    Str   text  = FloatToStr(&value);

    bool result = ZstrCompare(text.data, FLOAT_TEST_VERY_LARGE_ONES) == 0;

    StrDeinit(&text);
    FloatDeinit(&value);
    return result;
}

bool test_float_scientific_parse(void) {
    WriteFmt("Testing Float scientific parsing\n");

    Float value = FloatFromStr("1.2300e3");
    Str   text  = FloatToStr(&value);

    bool result = ZstrCompare(text.data, "1230") == 0;

    StrDeinit(&text);
    FloatDeinit(&value);
    return result;
}

bool test_float_from_str_invalid(void) {
    WriteFmt("Testing FloatFromStr invalid format handling\n");

    Float parsed = FloatFromStr("12.3.4");
    Float value  = FloatInit();
    bool  result = !FloatTryFromStr(&value, "12.3.4");

    result = result && FloatIsZero(&parsed);
    result = result && FloatIsZero(&value);

    FloatDeinit(&parsed);
    FloatDeinit(&value);
    return result;
}

bool test_float_from_str_null(void) {
    WriteFmt("Testing FloatFromStr NULL handling\n");

    Float parsed = FloatFromStr(NULL);
    Float value  = FloatInit();
    bool  result = !FloatTryFromStr(&value, NULL);

    result = result && FloatIsZero(&parsed);
    result = result && FloatIsZero(&value);

    FloatDeinit(&parsed);
    FloatDeinit(&value);
    return result;
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
        test_float_from_str_null,
    };

    TestFunction deadend_tests[1] = {0};

    int total_tests         = sizeof(tests) / sizeof(tests[0]);
    int total_deadend_tests = 0;

    return run_test_suite(tests, total_tests, deadend_tests, total_deadend_tests, "Float.Convert");
}
