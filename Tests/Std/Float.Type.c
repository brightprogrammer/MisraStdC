#include <Misra/Std/Allocator/Default.h>
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

    DefaultAllocator alloc = DefaultAllocatorInit();

    Float value = FloatInit(&alloc.base);

    bool result = FloatIsZero(&value);
    result      = result && !FloatIsNegative(&value);
    result      = result && (FloatExponent(&value) == 0);

    FloatDeinit(&value);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

bool test_float_clear(void) {
    WriteFmt("Testing FloatClear\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Float value = FloatFromStr("-123.45", &alloc.base);

    FloatClear(&value);

    bool result = FloatIsZero(&value);
    result      = result && !FloatIsNegative(&value);
    result      = result && (FloatExponent(&value) == 0);

    FloatDeinit(&value);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

bool test_float_clone(void) {
    WriteFmt("Testing FloatClone\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Float original = FloatFromStr("-12.5", &alloc.base);
    Float clone    = FloatClone(&original);
    Float expected = FloatFromStr("-12.5", &alloc.base);
    Str   text     = FloatToStr(&clone);

    FloatAbs(&original);

    bool result = FloatEQ(&clone, &expected);
    result      = result && (ZstrCompare(text.data, "-12.5") == 0);
    result      = result && !FloatEQ(&clone, &original);

    StrDeinit(&text);
    FloatDeinit(&original);
    FloatDeinit(&clone);
    FloatDeinit(&expected);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

bool test_float_clone_inherits_allocator_config(void) {
    WriteFmt("Testing FloatClone allocator inheritance\n");

    DefaultAllocator alloc = DefaultAllocatorInit();
    alloc.base.effort      = ALLOCATOR_EFFORT_RETRY_FALLBACK;
    alloc.base.retry_limit = 6;

    Float original    = FloatInit(&alloc.base);
    original.negative = true;
    original.exponent = -3;

    BitVecPush(&original.significand.bits, true);
    BitVecPush(&original.significand.bits, false);
    BitVecPush(&original.significand.bits, true);

    Float clone = FloatClone(&original);

    bool result = clone.negative == original.negative && clone.exponent == original.exponent &&
                  clone.significand.bits.length == original.significand.bits.length &&
                  clone.significand.bits.allocator == original.significand.bits.allocator &&
                  clone.significand.bits.allocator->allocate == original.significand.bits.allocator->allocate &&
                  clone.significand.bits.allocator->reallocate == original.significand.bits.allocator->reallocate &&
                  clone.significand.bits.allocator->deallocate == original.significand.bits.allocator->deallocate &&
                  clone.significand.bits.allocator->effort == original.significand.bits.allocator->effort &&
                  clone.significand.bits.allocator->retry_limit == original.significand.bits.allocator->retry_limit &&
                  BitVecGet(&clone.significand.bits, 0) == true && BitVecGet(&clone.significand.bits, 1) == false &&
                  BitVecGet(&clone.significand.bits, 2) == true;

    FloatDeinit(&original);
    FloatDeinit(&clone);
    DefaultAllocatorDeinit(&alloc);
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
