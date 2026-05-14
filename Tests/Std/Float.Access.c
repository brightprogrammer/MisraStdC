#include <Misra/Std/Allocator/Default.h>
#include <Misra/Std/Container/Float.h>
#include <Misra/Std/Log.h>

#include "../Util/TestRunner.h"

bool test_float_is_zero(void);
bool test_float_is_negative(void);
bool test_float_exponent(void);

bool test_float_is_zero(void) {
    WriteFmt("Testing FloatIsZero\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Float zero  = FloatInit(&alloc.base);
    Float value = FloatFromStr("0.001", &alloc.base);

    bool result = FloatIsZero(&zero);
    result      = result && !FloatIsZero(&value);

    FloatDeinit(&zero);
    FloatDeinit(&value);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

bool test_float_is_negative(void) {
    WriteFmt("Testing FloatIsNegative\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Float neg  = FloatFromStr("-42", &alloc.base);
    Float pos  = FloatFromStr("42", &alloc.base);
    Float zero = FloatFromStr("-0.0", &alloc.base);

    bool result = FloatIsNegative(&neg);
    result      = result && !FloatIsNegative(&pos);
    result      = result && !FloatIsNegative(&zero);

    FloatDeinit(&neg);
    FloatDeinit(&pos);
    FloatDeinit(&zero);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

bool test_float_exponent(void) {
    WriteFmt("Testing FloatExponent\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Float value = FloatFromStr("12.34", &alloc.base);

    bool result = FloatExponent(&value) == -2;

    FloatDeinit(&value);
    DefaultAllocatorDeinit(&alloc);
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
