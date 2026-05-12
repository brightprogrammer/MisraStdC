#include <Misra/Std/Container/Float.h>
#include <Misra/Std/Container/BitVec.h>
#include <Misra/Std/Log.h>
#include <string.h>

#include "../Util/TestRunner.h"

bool test_float_init(void);
bool test_float_clear(void);
bool test_float_clone(void);
bool test_float_clone_inherits_allocator_config(void);

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

bool test_float_clone_inherits_allocator_config(void) {
    WriteFmt("Testing FloatClone allocator inheritance\n");

    Allocator alloc = HeapAllocator();
    alloc.effort = ALLOCATOR_EFFORT_RETRY_FALLBACK;
    alloc.retry_limit = 6;
    alloc.flags = 0x6D6Du;

    Float original = FloatInit(alloc);
    original.negative = true;
    original.exponent = -3;
    original.significand.bits.allocator.state = (void *)&original;

    BitVecPush(&original.significand.bits, true);
    BitVecPush(&original.significand.bits, false);
    BitVecPush(&original.significand.bits, true);

    Float clone = FloatClone(&original);

    bool result =
        clone.negative == original.negative &&
        clone.exponent == original.exponent &&
        clone.significand.bits.length == original.significand.bits.length &&
        clone.significand.bits.allocator.allocate == original.significand.bits.allocator.allocate &&
        clone.significand.bits.allocator.reallocate == original.significand.bits.allocator.reallocate &&
        clone.significand.bits.allocator.deallocate == original.significand.bits.allocator.deallocate &&
        clone.significand.bits.allocator.state_init == original.significand.bits.allocator.state_init &&
        clone.significand.bits.allocator.state_deinit == original.significand.bits.allocator.state_deinit &&
        clone.significand.bits.allocator.effort == original.significand.bits.allocator.effort &&
        clone.significand.bits.allocator.retry_limit == original.significand.bits.allocator.retry_limit &&
        clone.significand.bits.allocator.flags == original.significand.bits.allocator.flags &&
        clone.significand.bits.allocator.state == NULL &&
        BitVecGet(&clone.significand.bits, 0) == true &&
        BitVecGet(&clone.significand.bits, 1) == false &&
        BitVecGet(&clone.significand.bits, 2) == true;

    FloatDeinit(&original);
    FloatDeinit(&clone);
    return result;
}

int main(void) {
    WriteFmt("[INFO] Starting Float.Type tests\n\n");

    TestFunction tests[] = {
        test_float_init,
        test_float_clear,
        test_float_clone,
        test_float_clone_inherits_allocator_config,
    };

    int total_tests = sizeof(tests) / sizeof(tests[0]);
    return run_test_suite(tests, total_tests, NULL, 0, "Float.Type");
}
