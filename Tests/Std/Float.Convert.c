#include <Misra/Std/Allocator/Default.h>
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

    DefaultAllocator alloc = DefaultAllocatorInit();

    Float value = float_from_u64(42, &alloc.base);
    Str   text  = FloatToStr(&value);

    bool result = ZstrCompare(text.data, "42") == 0;
    result      = result && !FloatIsNegative(&value);

    StrDeinit(&text);
    FloatDeinit(&value);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

bool test_float_from_signed_integer(void) {
    WriteFmt("Testing FloatFrom with signed integer\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Float value = float_from_i64(-42, &alloc.base);
    Str   text  = FloatToStr(&value);

    bool result = ZstrCompare(text.data, "-42") == 0;
    result      = result && FloatIsNegative(&value);

    StrDeinit(&text);
    FloatDeinit(&value);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

bool test_float_from_int_container(void) {
    WriteFmt("Testing FloatFrom with Int container\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int   integer = IntFromStr("12345678901234567890", &alloc.base);
    Float value   = float_from_int(&integer, &alloc.base);
    Str   text    = FloatToStr(&value);

    bool result = ZstrCompare(text.data, "12345678901234567890") == 0;

    IntDeinit(&integer);
    StrDeinit(&text);
    FloatDeinit(&value);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

bool test_float_to_int_exact(void) {
    WriteFmt("Testing FloatToInt exact conversion\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Float value        = FloatFromStr("1234500e-2", &alloc.base);
    Int   result_value = IntInit(&alloc.base);
    Str   text         = StrInit(&alloc.base);

    bool result = FloatToInt(&result_value, &value);
    text        = IntToStr(&result_value);
    result      = result && (ZstrCompare(text.data, "12345") == 0);

    FloatDeinit(&value);
    IntDeinit(&result_value);
    StrDeinit(&text);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

bool test_float_to_int_fractional_failure(void) {
    WriteFmt("Testing FloatToInt fractional failure handling\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Float value        = FloatFromStr("123.45", &alloc.base);
    Int   result_value = IntFrom(99, &alloc.base);

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

    Float value        = FloatFromStr("-42", &alloc.base);
    Int   result_value = IntFrom(99, &alloc.base);

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

    Float value = FloatFromStr("-123.45", &alloc.base);
    Str   text  = FloatToStr(&value);

    bool result = ZstrCompare(text.data, "-123.45") == 0;

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

    alloc.base.effort      = ALLOCATOR_EFFORT_RETRY;
    alloc.base.retry_limit = 5;

    Float value = FloatFromStr("-123.45", &alloc.base);

    ok = FloatTryToStrAlloc(&text, &value, &alloc.base);

    bool result = ok && (ZstrCompare(text.data, "-123.45") == 0) && (text.allocator->effort == alloc.base.effort) &&
                  (text.allocator->retry_limit == alloc.base.retry_limit);

    StrDeinit(&text);
    FloatDeinit(&value);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

bool test_float_very_large_string_round_trip(void) {
    WriteFmt("Testing Float very large string round trip\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Float value = FloatFromStr(FLOAT_TEST_VERY_LARGE_ONES, &alloc.base);
    Str   text  = FloatToStr(&value);

    bool result = ZstrCompare(text.data, FLOAT_TEST_VERY_LARGE_ONES) == 0;

    StrDeinit(&text);
    FloatDeinit(&value);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

bool test_float_scientific_parse(void) {
    WriteFmt("Testing Float scientific parsing\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Float value = FloatFromStr("1.2300e3", &alloc.base);
    Str   text  = FloatToStr(&value);

    bool result = ZstrCompare(text.data, "1230") == 0;

    StrDeinit(&text);
    FloatDeinit(&value);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

bool test_float_from_str_invalid(void) {
    WriteFmt("Testing FloatFromStr invalid format handling\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Float parsed = FloatFromStr("12.3.4", &alloc.base);
    Float value  = FloatInit(&alloc.base);
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

    Float parsed = FloatFromStr(NULL, &alloc.base);
    Float value  = FloatInit(&alloc.base);
    bool  result = !FloatTryFromStr(&value, NULL);

    result = result && FloatIsZero(&parsed);
    result = result && FloatIsZero(&value);

    FloatDeinit(&parsed);
    FloatDeinit(&value);
    DefaultAllocatorDeinit(&alloc);
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
