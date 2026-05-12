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

bool test_int_clone_inherits_allocator_config(void) {
    WriteFmt("Testing IntClone allocator inheritance\n");

    Allocator alloc = HeapAllocator();
    alloc.effort = ALLOCATOR_EFFORT_RETRY_FALLBACK;
    alloc.retry_limit = 5;
    alloc.flags = 0x4B4Bu;

    Int original = IntInit(alloc);
    original.bits.allocator.state = (void *)&original;

    BitVecPush(&original.bits, true);
    BitVecPush(&original.bits, false);
    BitVecPush(&original.bits, true);

    Int clone = IntClone(&original);

    bool result =
        clone.bits.length == original.bits.length &&
        clone.bits.allocator.allocate == original.bits.allocator.allocate &&
        clone.bits.allocator.reallocate == original.bits.allocator.reallocate &&
        clone.bits.allocator.deallocate == original.bits.allocator.deallocate &&
        clone.bits.allocator.state_init == original.bits.allocator.state_init &&
        clone.bits.allocator.state_deinit == original.bits.allocator.state_deinit &&
        clone.bits.allocator.effort == original.bits.allocator.effort &&
        clone.bits.allocator.retry_limit == original.bits.allocator.retry_limit &&
        clone.bits.allocator.flags == original.bits.allocator.flags &&
        clone.bits.allocator.state == NULL &&
        BitVecGet(&clone.bits, 0) == true &&
        BitVecGet(&clone.bits, 1) == false &&
        BitVecGet(&clone.bits, 2) == true;

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
        test_int_clone_inherits_allocator_config,
    };

    int total_tests = sizeof(tests) / sizeof(tests[0]);
    return run_test_suite(tests, total_tests, NULL, 0, "Int.Type");
}
