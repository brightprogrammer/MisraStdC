#include <Misra/Std/Container/Int.h>
#include <Misra/Std/Log.h>
#include <Misra/Types.h>

#include "../Util/TestRunner.h"

bool test_int_init(void);
bool test_int_clear(void);
bool test_int_clone(void);

bool test_int_init(void) {
    WriteFmt("Testing IntInit\n");

    Int value = IntInit();

    bool result = IntIsZero(&value);
    result      = result && (IntBitLength(&value) == 0);

    IntDeinit(&value);
    return result;
}

bool test_int_clear(void) {
    WriteFmt("Testing IntClear\n");

    Int value = IntFromBinary("101101");

    IntClear(&value);

    bool result = IntIsZero(&value);
    result      = result && (IntBitLength(&value) == 0);

    IntDeinit(&value);
    return result;
}

bool test_int_clone(void) {
    WriteFmt("Testing IntClone\n");

    Int original = IntFromBinary("1011");
    Int clone    = IntClone(&original);

    bool result = IntEQ(&clone, &original);
    result      = result && (IntToU64(&clone) == 11);

    IntShiftLeft(&original, 1);

    result = result && !IntEQ(&clone, &original);
    result = result && (IntToU64(&clone) == 11);
    result = result && (IntToU64(&original) == 22);

    IntDeinit(&original);
    IntDeinit(&clone);
    return result;
}

int main(void) {
    WriteFmt("[INFO] Starting Int.Type tests\n\n");

    TestFunction tests[] = {
        test_int_init,
        test_int_clear,
        test_int_clone,
    };

    int total_tests = sizeof(tests) / sizeof(tests[0]);
    return run_test_suite(tests, total_tests, NULL, 0, "Int.Type");
}
