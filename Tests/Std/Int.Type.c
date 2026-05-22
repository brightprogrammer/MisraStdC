#include <Misra/Std/Allocator/Default.h>
#include <Misra/Std/Container/Int.h>
#include <Misra/Std/Container/BitVec.h>
#include <Misra/Std/Log.h>
#include <Misra/Types.h>

#include "../Util/TestRunner.h"

bool test_int_init(void);
bool test_int_clear(void);
bool test_int_clone(void);
bool test_int_clone_inherits_allocator_config(void);

bool test_int_init(void) {
    WriteFmt("Testing IntInit\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int value = IntInit(&alloc.base);

    bool result = IntIsZero(&value);
    result      = result && (IntBitLength(&value) == 0);

    IntDeinit(&value);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

bool test_int_clear(void) {
    WriteFmt("Testing IntClear\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int value = IntFromBinary("101101", &alloc.base);

    IntClear(&value);

    bool result = IntIsZero(&value);
    result      = result && (IntBitLength(&value) == 0);

    IntDeinit(&value);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

bool test_int_clone(void) {
    WriteFmt("Testing IntClone\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int original = IntFromBinary("1011", &alloc.base);
    Int clone    = IntClone(&original);

    bool result = IntEQ(&clone, &original);
    result      = result && (IntToU64(&clone) == 11);

    IntShiftLeft(&original, 1);

    result = result && !IntEQ(&clone, &original);
    result = result && (IntToU64(&clone) == 11);
    result = result && (IntToU64(&original) == 22);

    IntDeinit(&original);
    IntDeinit(&clone);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

bool test_int_clone_inherits_allocator_config(void) {
    WriteFmt("Testing IntClone allocator inheritance\n");

    DefaultAllocator alloc = DefaultAllocatorInit();
    alloc.base.effort      = ALLOCATOR_EFFORT_RETRY_FALLBACK;
    alloc.base.retry_limit = 5;

    Int original = IntInit(&alloc);

    BitVecPush(&original.bits, true);
    BitVecPush(&original.bits, false);
    BitVecPush(&original.bits, true);

    Int clone = IntClone(&original);

    bool result =
        BitVecLen(&clone.bits) == BitVecLen(&original.bits) && clone.bits.allocator == original.bits.allocator &&
        clone.bits.allocator->allocate == original.bits.allocator->allocate &&
        clone.bits.allocator->remap == original.bits.allocator->remap &&
        clone.bits.allocator->deallocate == original.bits.allocator->deallocate &&
        clone.bits.allocator->effort == original.bits.allocator->effort &&
        clone.bits.allocator->retry_limit == original.bits.allocator->retry_limit &&
        BitVecGet(&clone.bits, 0) == true && BitVecGet(&clone.bits, 1) == false && BitVecGet(&clone.bits, 2) == true;

    IntDeinit(&original);
    IntDeinit(&clone);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

int main(void) {
    WriteFmt("[INFO] Starting Int.Type tests\n\n");

    TestFunction tests[] = {
        test_int_init,
        test_int_clear,
        test_int_clone,
        test_int_clone_inherits_allocator_config,
    };

    int total_tests = sizeof(tests) / sizeof(tests[0]);
    return run_test_suite(tests, total_tests, NULL, 0, "Int.Type");
}
