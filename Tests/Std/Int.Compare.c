#include <Misra/Std/Container/Int.h>
#include <Misra/Std/Log.h>
#include <Misra/Types.h>

#include "../Util/TestRunner.h"

bool test_int_compare(void);
bool test_int_compare_wrappers(void);
bool test_int_compare_u64(void);
bool test_int_compare_generic(void);

bool test_int_compare(void) {
    WriteFmt("Testing IntCompare\n");

    Int a = IntFromU64(41);
    Int b = IntFromU64(42);
    Int c = IntFromBinary("000101010");

    bool result = IntCompare(&a, &b) < 0;
    result      = result && (IntCompare(&b, &a) > 0);
    result      = result && (IntCompare(&b, &c) == 0);

    IntDeinit(&a);
    IntDeinit(&b);
    IntDeinit(&c);
    return result;
}

bool test_int_compare_wrappers(void) {
    WriteFmt("Testing Int compare wrappers\n");

    Int a = IntFromU64(41);
    Int b = IntFromU64(42);
    Int c = IntFromBinary("000101010");

    bool result = IntLT(&a, &b);
    result      = result && IntLE(&a, &b);
    result      = result && IntGT(&b, &a);
    result      = result && IntGE(&b, &a);
    result      = result && IntEQ(&b, &c);
    result      = result && IntGE(&b, &c);
    result      = result && IntLE(&b, &c);
    result      = result && IntNE(&a, &b);

    IntDeinit(&a);
    IntDeinit(&b);
    IntDeinit(&c);
    return result;
}

bool test_int_compare_u64(void) {
    WriteFmt("Testing IntCompareU64\n");

    Int small = IntFromU64(1234);
    Int big   = IntFromU64(1);

    IntShiftLeft(&big, 80);

    bool result = IntCompareU64(&small, 2000) < 0;
    result      = result && (IntCompareU64(&small, 1234) == 0);
    result      = result && (IntCompareU64(&small, 10) > 0);
    result      = result && (IntCompareU64(&big, UINT64_MAX) > 0);

    IntDeinit(&small);
    IntDeinit(&big);
    return result;
}

bool test_int_compare_generic(void) {
    WriteFmt("Testing Int generic compare macros\n");

    Int value = IntFromU64(42);
    Int same  = IntFromBinary("00101010");

    bool result = IntEQ(&value, &same);
    result      = result && IntEQ(&value, same);
    result      = result && IntEQ(&value, 42);
    result      = result && IntLE(&value, 42);
    result      = result && IntGT(&value, -1);
    result      = result && IntLT(&value, 100ULL);
    result      = result && IntNE(&value, 43);

    IntDeinit(&value);
    IntDeinit(&same);
    return result;
}

int main(void) {
    WriteFmt("[INFO] Starting Int.Compare tests\n\n");

    TestFunction tests[] = {
        test_int_compare,
        test_int_compare_wrappers,
        test_int_compare_u64,
        test_int_compare_generic,
    };

    int total_tests = sizeof(tests) / sizeof(tests[0]);
    return run_test_suite(tests, total_tests, NULL, 0, "Int.Compare");
}
