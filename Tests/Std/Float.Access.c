#include <Misra/Std/Container/Float.h>
#include <Misra/Std/Log.h>

#include "../Util/TestRunner.h"

bool test_float_is_zero(void);
bool test_float_is_negative(void);
bool test_float_exponent(void);

bool test_float_is_zero(void) {
    WriteFmt("Testing FloatIsZero\n");

    Float zero  = FloatInit();
    Float value = FloatFromStr("0.001");

    bool result = FloatIsZero(&zero);
    result      = result && !FloatIsZero(&value);

    FloatDeinit(&zero);
    FloatDeinit(&value);
    return result;
}

bool test_float_is_negative(void) {
    WriteFmt("Testing FloatIsNegative\n");

    Float neg  = FloatFromStr("-42");
    Float pos  = FloatFromStr("42");
    Float zero = FloatFromStr("-0.0");

    bool result = FloatIsNegative(&neg);
    result      = result && !FloatIsNegative(&pos);
    result      = result && !FloatIsNegative(&zero);

    FloatDeinit(&neg);
    FloatDeinit(&pos);
    FloatDeinit(&zero);
    return result;
}

bool test_float_exponent(void) {
    WriteFmt("Testing FloatExponent\n");

    Float value = FloatFromStr("12.34");

    bool result = FloatExponent(&value) == -2;

    FloatDeinit(&value);
    return result;
}

int main(void) {
    WriteFmt("[INFO] Starting Float.Access tests\n\n");

    TestFunction tests[] = {
        test_float_is_zero,
        test_float_is_negative,
        test_float_exponent,
    };

    int total_tests = sizeof(tests) / sizeof(tests[0]);
    return run_test_suite(tests, total_tests, NULL, 0, "Float.Access");
}
