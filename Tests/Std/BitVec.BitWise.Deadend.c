#include <Misra/Std/Container/BitVec.h>
#include <Misra/Std/Allocator/Default.h>
#include <Misra/Std/Log.h>

#include <stdio.h>
#include <Misra/Types.h>

// Include test utilities
#include "../Util/TestRunner.h"

// Function prototypes for deadend tests
bool test_bitvec_bitwise_null_failures(void);
bool test_bitvec_bitwise_ops_null_failures(void);
bool test_bitvec_reverse_null_failures(void);
bool test_bitvec_shift_ops_null_failures(void);
bool test_bitvec_rotate_ops_null_failures(void);
bool test_bitvec_and_result_null_failures(void);
bool test_bitvec_or_operand_null_failures(void);
bool test_bitvec_xor_second_operand_null_failures(void);
bool test_bitvec_not_null_failures(void);

// Deadend tests
bool test_bitvec_bitwise_null_failures(void) {
    WriteFmt("Testing BitVec bitwise NULL pointer handling\n");

    // Test NULL bitvec pointer - should abort
    BitVecShiftLeft(NULL, 1);

    return false;
}

bool test_bitvec_bitwise_ops_null_failures(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVec bitwise operations NULL handling\n");

    BitVec bv  = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVec bv2 = BitVecInit(ALLOCATOR_OF(&alloc));

    // Test NULL pointer - should abort
    BitVecAnd(NULL, &bv, &bv2);

    BitVecDeinit(&bv);
    BitVecDeinit(&bv2);
    DefaultAllocatorDeinit(&alloc);
    return false;
}

bool test_bitvec_reverse_null_failures(void) {
    WriteFmt("Testing BitVec reverse NULL handling\n");

    // Test NULL pointer - should abort
    BitVecReverse(NULL);

    return false;
}

// NEW: Additional deadend tests
bool test_bitvec_shift_ops_null_failures(void) {
    WriteFmt("Testing BitVec shift operations NULL handling\n");

    // Test NULL pointer for shift right - should abort
    BitVecShiftRight(NULL, 5);

    return false;
}

bool test_bitvec_rotate_ops_null_failures(void) {
    WriteFmt("Testing BitVec rotate operations NULL handling\n");

    // Test NULL pointer for rotate - should abort
    BitVecRotateLeft(NULL, 3);

    return false;
}

bool test_bitvec_and_result_null_failures(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVec AND with NULL result handling\n");

    BitVec bv1 = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVec bv2 = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVecPush(&bv1, true);
    BitVecPush(&bv2, false);

    // Test NULL result pointer - should abort
    BitVecAnd(NULL, &bv1, &bv2);

    BitVecDeinit(&bv1);
    BitVecDeinit(&bv2);
    DefaultAllocatorDeinit(&alloc);
    return false;
}

bool test_bitvec_or_operand_null_failures(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVec OR with NULL operand handling\n");

    BitVec result = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVec bv1    = BitVecInit(ALLOCATOR_OF(&alloc));

    // Test NULL operand - should abort
    BitVecOr(&result, &bv1, NULL);

    BitVecDeinit(&result);
    BitVecDeinit(&bv1);
    DefaultAllocatorDeinit(&alloc);
    return false;
}

bool test_bitvec_xor_second_operand_null_failures(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVec XOR with NULL second operand handling\n");

    BitVec result = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVec bv1    = BitVecInit(ALLOCATOR_OF(&alloc));

    // Test NULL second operand - should abort
    BitVecXor(&result, NULL, &bv1);

    BitVecDeinit(&result);
    BitVecDeinit(&bv1);
    DefaultAllocatorDeinit(&alloc);
    return false;
}

bool test_bitvec_not_null_failures(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVec NOT with NULL handling\n");

    BitVec result = BitVecInit(ALLOCATOR_OF(&alloc));

    // Test NULL operand - should abort
    BitVecNot(&result, NULL);

    BitVecDeinit(&result);
    DefaultAllocatorDeinit(&alloc);
    return false;
}

// Main function that runs all deadend tests
int main(void) {
    WriteFmt("[INFO] Starting BitVec.BitWise.Deadend tests\n\n");

    // Array of deadend test functions
    TestFunction deadend_tests[] = {
        test_bitvec_bitwise_null_failures,
        test_bitvec_bitwise_ops_null_failures,
        test_bitvec_reverse_null_failures,
        test_bitvec_shift_ops_null_failures,
        test_bitvec_rotate_ops_null_failures,
        test_bitvec_and_result_null_failures,
        test_bitvec_or_operand_null_failures,
        test_bitvec_xor_second_operand_null_failures,
        test_bitvec_not_null_failures
    };

    int total_deadend_tests = sizeof(deadend_tests) / sizeof(deadend_tests[0]);

    // Run all deadend tests using the centralized test driver
    return run_test_suite(NULL, 0, deadend_tests, total_deadend_tests, "BitVec.BitWise.Deadend");
}
