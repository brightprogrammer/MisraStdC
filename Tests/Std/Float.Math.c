#include <Misra/Std/Container/Float.h>
#include <Misra/Std/Container/Int.h>
#include <Misra/Std/Log.h>
#include <string.h>

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

bool test_float_negate_abs(void) {
    WriteFmt("Testing FloatNegate and FloatAbs\n");

    Float value = FloatFromStr("12.5");
    Str   text  = StrInit();

    FloatNegate(&value);
    text = FloatToStr(&value);

    bool result = strcmp(text.data, "-12.5") == 0;

    StrDeinit(&text);
    FloatAbs(&value);
    text   = FloatToStr(&value);
    result = result && (strcmp(text.data, "12.5") == 0);

    StrDeinit(&text);
    FloatDeinit(&value);
    return result;
}

bool test_float_add_small_small(void) {
    WriteFmt("Testing FloatAdd with small floats\n");

    Float a            = FloatFromStr("1.2");
    Float b            = FloatFromStr("0.03");
    Float result_value = FloatInit();
    Str   text         = StrInit();

    FloatAdd(&result_value, &a, &b);
    text = FloatToStr(&result_value);

    bool result = strcmp(text.data, "1.23") == 0;

    StrDeinit(&text);
    FloatDeinit(&a);
    FloatDeinit(&b);
    FloatDeinit(&result_value);
    return result;
}

bool test_float_add_very_large_large(void) {
    WriteFmt("Testing FloatAdd with very large floats\n");

    Float a            = FloatFromStr(FLOAT_TEST_VERY_LARGE_ONES);
    Float b            = FloatFromStr(FLOAT_TEST_VERY_LARGE_TWOS);
    Float result_value = FloatInit();
    Str   text         = StrInit();

    FloatAdd(&result_value, &a, &b);
    text = FloatToStr(&result_value);

    bool result = strcmp(text.data, FLOAT_TEST_VERY_LARGE_THREES) == 0;

    StrDeinit(&text);
    FloatDeinit(&a);
    FloatDeinit(&b);
    FloatDeinit(&result_value);
    return result;
}

bool test_float_add_generic(void) {
    WriteFmt("Testing FloatAdd generic dispatch\n");

    Float a            = FloatFromStr("1.25");
    Float b            = FloatFromStr("0.75");
    Int   whole        = IntFromU64(2);
    Float result_value = FloatInit();
    Str   text         = StrInit();

    FloatAdd(&result_value, &a, b);
    text = FloatToStr(&result_value);
    bool result = strcmp(text.data, "2") == 0;

    StrDeinit(&text);
    FloatAdd(&result_value, &a, whole);
    text   = FloatToStr(&result_value);
    result = result && (strcmp(text.data, "3.25") == 0);

    StrDeinit(&text);
    FloatAdd(&result_value, &a, 0.75);
    text   = FloatToStr(&result_value);
    result = result && (strcmp(text.data, "2") == 0);

    FloatDeinit(&a);
    FloatDeinit(&b);
    IntDeinit(&whole);
    FloatDeinit(&result_value);
    StrDeinit(&text);
    return result;
}

bool test_float_sub_small_small(void) {
    WriteFmt("Testing FloatSub with small floats\n");

    Float a            = FloatFromStr("1.5");
    Float b            = FloatFromStr("2");
    Float result_value = FloatInit();
    Str   text         = StrInit();

    FloatSub(&result_value, &a, &b);
    text = FloatToStr(&result_value);

    bool result = strcmp(text.data, "-0.5") == 0;

    StrDeinit(&text);
    FloatDeinit(&a);
    FloatDeinit(&b);
    FloatDeinit(&result_value);
    return result;
}

bool test_float_sub_very_large_large(void) {
    WriteFmt("Testing FloatSub with very large floats\n");

    Float a            = FloatFromStr(FLOAT_TEST_VERY_LARGE_THREES);
    Float b            = FloatFromStr(FLOAT_TEST_VERY_LARGE_ONES);
    Float result_value = FloatInit();
    Str   text         = StrInit();

    FloatSub(&result_value, &a, &b);
    text = FloatToStr(&result_value);

    bool result = strcmp(text.data, FLOAT_TEST_VERY_LARGE_TWOS) == 0;

    StrDeinit(&text);
    FloatDeinit(&a);
    FloatDeinit(&b);
    FloatDeinit(&result_value);
    return result;
}

bool test_float_sub_generic(void) {
    WriteFmt("Testing FloatSub generic dispatch\n");

    Float a            = FloatFromStr("5.5");
    Float b            = FloatFromStr("0.5");
    Int   whole        = IntFromU64(2);
    Float result_value = FloatInit();
    Str   text         = StrInit();

    FloatSub(&result_value, &a, b);
    text = FloatToStr(&result_value);
    bool result = strcmp(text.data, "5") == 0;

    StrDeinit(&text);
    FloatSub(&result_value, &a, whole);
    text   = FloatToStr(&result_value);
    result = result && (strcmp(text.data, "3.5") == 0);

    StrDeinit(&text);
    FloatSub(&result_value, &a, -2);
    text   = FloatToStr(&result_value);
    result = result && (strcmp(text.data, "7.5") == 0);

    FloatDeinit(&a);
    FloatDeinit(&b);
    IntDeinit(&whole);
    FloatDeinit(&result_value);
    StrDeinit(&text);
    return result;
}

bool test_float_mul_small_small(void) {
    WriteFmt("Testing FloatMul with small floats\n");

    Float a            = FloatFromStr("12.5");
    Float b            = FloatFromStr("-0.2");
    Float result_value = FloatInit();
    Str   text         = StrInit();

    FloatMul(&result_value, &a, &b);
    text = FloatToStr(&result_value);

    bool result = strcmp(text.data, "-2.5") == 0;

    StrDeinit(&text);
    FloatDeinit(&a);
    FloatDeinit(&b);
    FloatDeinit(&result_value);
    return result;
}

bool test_float_mul_very_large_small(void) {
    WriteFmt("Testing FloatMul with very large and small floats\n");

    Float a            = FloatFromStr(FLOAT_TEST_VERY_LARGE_ONES);
    Float b            = FloatFromStr("2");
    Float result_value = FloatInit();
    Str   text         = StrInit();

    FloatMul(&result_value, &a, &b);
    text = FloatToStr(&result_value);

    bool result = strcmp(text.data, FLOAT_TEST_VERY_LARGE_TWOS) == 0;

    StrDeinit(&text);
    FloatDeinit(&a);
    FloatDeinit(&b);
    FloatDeinit(&result_value);
    return result;
}

bool test_float_mul_generic(void) {
    WriteFmt("Testing FloatMul generic dispatch\n");

    Float a            = FloatFromStr("1.5");
    Float b            = FloatFromStr("2");
    Int   whole        = IntFromU64(2);
    Float result_value = FloatInit();
    Str   text         = StrInit();

    FloatMul(&result_value, &a, b);
    text = FloatToStr(&result_value);
    bool result = strcmp(text.data, "3") == 0;

    StrDeinit(&text);
    FloatMul(&result_value, &a, whole);
    text   = FloatToStr(&result_value);
    result = result && (strcmp(text.data, "3") == 0);

    StrDeinit(&text);
    FloatMul(&result_value, &a, -2);
    text   = FloatToStr(&result_value);
    result = result && (strcmp(text.data, "-3") == 0);

    FloatDeinit(&a);
    FloatDeinit(&b);
    IntDeinit(&whole);
    FloatDeinit(&result_value);
    StrDeinit(&text);
    return result;
}

bool test_float_div_small_small(void) {
    WriteFmt("Testing FloatDiv with small floats\n");

    Float a            = FloatFromStr("1");
    Float b            = FloatFromStr("8");
    Float result_value = FloatInit();
    Str   text         = StrInit();

    FloatDiv(&result_value, &a, &b, 3);
    text = FloatToStr(&result_value);

    bool result = strcmp(text.data, "0.125") == 0;

    StrDeinit(&text);
    FloatDeinit(&a);
    FloatDeinit(&b);
    FloatDeinit(&result_value);
    return result;
}

bool test_float_div_very_large_small(void) {
    WriteFmt("Testing FloatDiv with very large and small floats\n");

    Float a            = FloatFromStr(FLOAT_TEST_VERY_LARGE_TWOS);
    Float b            = FloatFromStr("2");
    Float result_value = FloatInit();
    Str   text         = StrInit();

    FloatDiv(&result_value, &a, &b, 0);
    text = FloatToStr(&result_value);

    bool result = strcmp(text.data, FLOAT_TEST_VERY_LARGE_ONES) == 0;

    StrDeinit(&text);
    FloatDeinit(&a);
    FloatDeinit(&b);
    FloatDeinit(&result_value);
    return result;
}

bool test_float_div_generic(void) {
    WriteFmt("Testing FloatDiv generic dispatch\n");

    Float a            = FloatFromStr("7.5");
    Float b            = FloatFromStr("2.5");
    Int   whole        = IntFromU64(3);
    Float result_value = FloatInit();
    Str   text         = StrInit();

    FloatDiv(&result_value, &a, b, 1);
    text = FloatToStr(&result_value);
    bool result = strcmp(text.data, "3") == 0;

    StrDeinit(&text);
    FloatDiv(&result_value, &a, whole, 1);
    text   = FloatToStr(&result_value);
    result = result && (strcmp(text.data, "2.5") == 0);

    StrDeinit(&text);
    FloatDiv(&result_value, &a, 0.5, 1);
    text   = FloatToStr(&result_value);
    result = result && (strcmp(text.data, "15") == 0);

    FloatDeinit(&a);
    FloatDeinit(&b);
    IntDeinit(&whole);
    FloatDeinit(&result_value);
    StrDeinit(&text);
    return result;
}

bool test_float_div_by_zero(void) {
    WriteFmt("Testing FloatDiv divide-by-zero handling\n");

    Float a = FloatFromStr("1");
    Float b = FloatInit();
    Float r = FloatInit();

    FloatDiv(&r, &a, &b, 4);
    return false;
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
    };

    TestFunction deadend_tests[] = {
        test_float_div_by_zero,
    };

    int total_tests         = sizeof(tests) / sizeof(tests[0]);
    int total_deadend_tests = sizeof(deadend_tests) / sizeof(deadend_tests[0]);

    return run_test_suite(tests, total_tests, deadend_tests, total_deadend_tests, "Float.Math");
}
