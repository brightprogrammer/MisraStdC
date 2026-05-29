#include <Misra/Std/Allocator/Default.h>
#include <Misra/Std/Zstr.h>
#include <Misra/Std/Container/Float.h>
#include <Misra/Std/Log.h>

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
    result      = result && (ZstrCompare(StrBegin(&text), "-12.5") == 0);
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
    // intentional bypass: no public setter on `Allocator` for effort /
    // retry_limit -- pre-seeded directly so the inheritance path below
    // can be observed end-to-end.
    alloc.base.effort      = ALLOCATOR_EFFORT_RETRY_FALLBACK;
    alloc.base.retry_limit = 6;

    // "-0.005" normalises to (negative=true, significand=5, exponent=-3),
    // a non-trivial three-bit magnitude that exercises the clone-vs-original
    // equality check below.
    Float original = FloatFromStr("-0.005", &alloc.base);

    Float clone = FloatClone(&original);

    bool result = FloatEQ(&clone, &original) && FloatAllocator(&clone) == FloatAllocator(&original) &&
                  FloatAllocator(&clone)->allocate == FloatAllocator(&original)->allocate &&
                  FloatAllocator(&clone)->remap == FloatAllocator(&original)->remap &&
                  FloatAllocator(&clone)->deallocate == FloatAllocator(&original)->deallocate &&
                  FloatAllocator(&clone)->effort == FloatAllocator(&original)->effort &&
                  FloatAllocator(&clone)->retry_limit == FloatAllocator(&original)->retry_limit;

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
