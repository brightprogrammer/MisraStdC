#include <Misra/Std/Container/Float.h>
#include <Misra/Std/Log.h>
#include <string.h>

#include "../Util/TestRunner.h"

bool test_float_init(void);
bool test_float_clear(void);
bool test_float_clone(void);

bool test_float_init(void) {
    WriteFmt("Testing FloatInit\n");

    Float value = FloatInit();

    bool result = FloatIsZero(&value);
    result      = result && !FloatIsNegative(&value);
    result      = result && (FloatExponent(&value) == 0);

    FloatDeinit(&value);
    return result;
}

bool test_float_clear(void) {
    WriteFmt("Testing FloatClear\n");

    Float value = FloatFromStr("-123.45");

    FloatClear(&value);

    bool result = FloatIsZero(&value);
    result      = result && !FloatIsNegative(&value);
    result      = result && (FloatExponent(&value) == 0);

    FloatDeinit(&value);
    return result;
}

bool test_float_clone(void) {
    WriteFmt("Testing FloatClone\n");

    Float original = FloatFromStr("-12.5");
    Float clone    = FloatClone(&original);
    Float expected = FloatFromStr("-12.5");
    Str   text     = FloatToStr(&clone);

    FloatAbs(&original);

    bool result = FloatEQ(&clone, &expected);
    result      = result && (strcmp(text.data, "-12.5") == 0);
    result      = result && !FloatEQ(&clone, &original);

    StrDeinit(&text);
    FloatDeinit(&original);
    FloatDeinit(&clone);
    FloatDeinit(&expected);
    return result;
}

int main(void) {
    WriteFmt("[INFO] Starting Float.Type tests\n\n");

    TestFunction tests[] = {
        test_float_init,
        test_float_clear,
        test_float_clone,
    };

    int total_tests = sizeof(tests) / sizeof(tests[0]);
    return run_test_suite(tests, total_tests, NULL, 0, "Float.Type");
}
