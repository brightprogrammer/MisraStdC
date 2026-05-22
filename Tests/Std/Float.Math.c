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
    };

    TestFunction deadend_tests[1] = {0};

    int total_tests         = sizeof(tests) / sizeof(tests[0]);
    int total_deadend_tests = 0;

    return run_test_suite(tests, total_tests, deadend_tests, total_deadend_tests, "Float.Math");
}
