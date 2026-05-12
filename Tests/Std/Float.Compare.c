#include <Misra/Std/Container/Float.h>
#include <Misra/Std/Container/Int.h>
#include <Misra/Std/Log.h>

#include "../Util/FloatTestData.h"
#include "../Util/TestRunner.h"

bool test_float_compare_small_small(void);
bool test_float_compare_very_large_large(void);
bool test_float_compare_very_large_small(void);
bool test_float_compare_wrappers(void);
bool test_float_compare_generic(void);

bool test_float_compare_small_small(void) {
    WriteFmt("Testing FloatCompare with small floats\n");

    Float a = FloatFromStr("1.23");
    Float b = FloatFromStr("123e-2");
    Float c = FloatFromStr("-1.23");

    bool result = FloatCompare(&a, &b) == 0;
    result      = result && FloatEQ(&a, &b);
    result      = result && (FloatCompare(&c, &a) < 0);
    result      = result && (FloatCompare(&a, &c) > 0);

    FloatDeinit(&a);
    FloatDeinit(&b);
    FloatDeinit(&c);
    return result;
}

bool test_float_compare_very_large_large(void) {
    WriteFmt("Testing FloatCompare with very large floats\n");

    Float a = FloatFromStr(FLOAT_TEST_VERY_LARGE_ONES);
    Float b = FloatFromStr(FLOAT_TEST_VERY_LARGE_TWOS);
    Float c = FloatFromStr(FLOAT_TEST_VERY_LARGE_ONES);

    bool result = FloatLT(&a, &b);
    result      = result && FloatGT(&b, &a);
    result      = result && (FloatCompare(&a, &c) == 0);
    result      = result && FloatEQ(&a, &c);

    FloatDeinit(&a);
    FloatDeinit(&b);
    FloatDeinit(&c);
    return result;
}

bool test_float_compare_very_large_small(void) {
    WriteFmt("Testing FloatCompare with very large and small floats\n");

    Float large          = FloatFromStr(FLOAT_TEST_VERY_LARGE_ONES);
    Float negative_large = FloatFromStr("-" FLOAT_TEST_VERY_LARGE_ONES);
    Float small          = FloatFromStr("2.5");

    bool result = FloatGT(&large, &small);
    result      = result && FloatLT(&negative_large, &small);
    result      = result && FloatNE(&large, &small);

    FloatDeinit(&large);
    FloatDeinit(&negative_large);
    FloatDeinit(&small);
    return result;
}

bool test_float_compare_wrappers(void) {
    WriteFmt("Testing Float compare macros\n");

    Float a        = FloatFromStr("-2");
    Float b        = FloatFromStr("0.5");
    Float expected = FloatFromStr("5e-1");

    bool result = FloatLT(&a, &b);
    result      = result && FloatLE(&a, &b);
    result      = result && FloatGT(&b, &a);
    result      = result && FloatGE(&b, &a);
    result      = result && FloatNE(&a, &b);
    result      = result && FloatEQ(&b, &expected);

    FloatDeinit(&a);
    FloatDeinit(&b);
    FloatDeinit(&expected);
    return result;
}

bool test_float_compare_generic(void) {
    WriteFmt("Testing FloatCompare generic dispatch\n");

    Float value = FloatFromStr("12.5");
    Float same  = FloatFromStr("12.5");
    Int   whole = IntFrom(12);
    Int   next  = IntFrom(13);

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
    return result;
}

int main(void) {
    WriteFmt("[INFO] Starting Float.Compare tests\n\n");

    TestFunction tests[] = {
        test_float_compare_small_small,
        test_float_compare_very_large_large,
        test_float_compare_very_large_small,
        test_float_compare_wrappers,
        test_float_compare_generic,
    };

    int total_tests = sizeof(tests) / sizeof(tests[0]);
    return run_test_suite(tests, total_tests, NULL, 0, "Float.Compare");
}
