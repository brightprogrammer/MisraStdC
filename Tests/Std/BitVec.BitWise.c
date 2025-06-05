#include <Misra/Std/Container/BitVec.h>
#include <Misra/Std/Log.h>

#include <stdio.h>
#include <Misra/Types.h>

// Include test utilities
#include "../Util/TestRunner.h"

// Function prototypes
bool test_bitvec_and(void);
bool test_bitvec_or(void);
bool test_bitvec_xor(void);
bool test_bitvec_not(void);
bool test_bitvec_shift_left(void);
bool test_bitvec_shift_right(void);
bool test_bitvec_rotate_left(void);
bool test_bitvec_rotate_right(void);
bool test_bitvec_reverse(void);
bool test_bitvec_shift_edge_cases(void);
bool test_bitvec_rotate_edge_cases(void);
bool test_bitvec_bitwise_ops_edge_cases(void);
bool test_bitvec_reverse_edge_cases(void);
bool test_bitvec_bitwise_null_failures(void);
bool test_bitvec_bitwise_ops_null_failures(void);
bool test_bitvec_reverse_null_failures(void);

// Test BitVecAnd function
bool test_bitvec_and(void) {
    printf("Testing BitVecAnd\n");

    BitVec bv1    = BitVecInit();
    BitVec bv2    = BitVecInit();
    BitVec result = BitVecInit();

    // Set up first bitvector: 1101
    BitVecPush(&bv1, true);
    BitVecPush(&bv1, true);
    BitVecPush(&bv1, false);
    BitVecPush(&bv1, true);

    // Set up second bitvector: 1010
    BitVecPush(&bv2, true);
    BitVecPush(&bv2, false);
    BitVecPush(&bv2, true);
    BitVecPush(&bv2, false);

    // Perform AND operation
    BitVecAnd(&result, &bv1, &bv2);

    // Expected result: 1000 (1101 AND 1010)
    bool test_result = (result.length == 4);
    test_result      = test_result && (BitVecGet(&result, 0) == true);
    test_result      = test_result && (BitVecGet(&result, 1) == false);
    test_result      = test_result && (BitVecGet(&result, 2) == false);
    test_result      = test_result && (BitVecGet(&result, 3) == false);

    // Clean up
    BitVecDeinit(&bv1);
    BitVecDeinit(&bv2);
    BitVecDeinit(&result);

    return test_result;
}

// Test BitVecOr function
bool test_bitvec_or(void) {
    printf("Testing BitVecOr\n");

    BitVec bv1    = BitVecInit();
    BitVec bv2    = BitVecInit();
    BitVec result = BitVecInit();

    // Set up first bitvector: 1100
    BitVecPush(&bv1, true);
    BitVecPush(&bv1, true);
    BitVecPush(&bv1, false);
    BitVecPush(&bv1, false);

    // Set up second bitvector: 1010
    BitVecPush(&bv2, true);
    BitVecPush(&bv2, false);
    BitVecPush(&bv2, true);
    BitVecPush(&bv2, false);

    // Perform OR operation
    BitVecOr(&result, &bv1, &bv2);

    // Expected result: 1110 (1100 OR 1010)
    bool test_result = (result.length == 4);
    test_result      = test_result && (BitVecGet(&result, 0) == true);
    test_result      = test_result && (BitVecGet(&result, 1) == true);
    test_result      = test_result && (BitVecGet(&result, 2) == true);
    test_result      = test_result && (BitVecGet(&result, 3) == false);

    // Clean up
    BitVecDeinit(&bv1);
    BitVecDeinit(&bv2);
    BitVecDeinit(&result);

    return test_result;
}

// Test BitVecXor function
bool test_bitvec_xor(void) {
    printf("Testing BitVecXor\n");

    BitVec bv1    = BitVecInit();
    BitVec bv2    = BitVecInit();
    BitVec result = BitVecInit();

    // Set up first bitvector: 1100
    BitVecPush(&bv1, true);
    BitVecPush(&bv1, true);
    BitVecPush(&bv1, false);
    BitVecPush(&bv1, false);

    // Set up second bitvector: 1010
    BitVecPush(&bv2, true);
    BitVecPush(&bv2, false);
    BitVecPush(&bv2, true);
    BitVecPush(&bv2, false);

    // Perform XOR operation
    BitVecXor(&result, &bv1, &bv2);

    // Expected result: 0110 (1100 XOR 1010)
    bool test_result = (result.length == 4);
    test_result      = test_result && (BitVecGet(&result, 0) == false);
    test_result      = test_result && (BitVecGet(&result, 1) == true);
    test_result      = test_result && (BitVecGet(&result, 2) == true);
    test_result      = test_result && (BitVecGet(&result, 3) == false);

    // Clean up
    BitVecDeinit(&bv1);
    BitVecDeinit(&bv2);
    BitVecDeinit(&result);

    return test_result;
}

// Test BitVecNot function
bool test_bitvec_not(void) {
    printf("Testing BitVecNot\n");

    BitVec bv     = BitVecInit();
    BitVec result = BitVecInit();

    // Set up bitvector: 1010
    BitVecPush(&bv, true);
    BitVecPush(&bv, false);
    BitVecPush(&bv, true);
    BitVecPush(&bv, false);

    // Perform NOT operation
    BitVecNot(&result, &bv);

    // Expected result: 0101 (NOT 1010)
    bool test_result = (result.length == 4);
    test_result      = test_result && (BitVecGet(&result, 0) == false);
    test_result      = test_result && (BitVecGet(&result, 1) == true);
    test_result      = test_result && (BitVecGet(&result, 2) == false);
    test_result      = test_result && (BitVecGet(&result, 3) == true);

    // Clean up
    BitVecDeinit(&bv);
    BitVecDeinit(&result);

    return test_result;
}

// Test BitVecShiftLeft function
bool test_bitvec_shift_left(void) {
    printf("Testing BitVecShiftLeft\n");

    BitVec bv = BitVecInit();

    // Set up bitvector: 1011
    BitVecPush(&bv, true);
    BitVecPush(&bv, false);
    BitVecPush(&bv, true);
    BitVecPush(&bv, true);

    // Shift left by 2 positions
    BitVecShiftLeft(&bv, 2);

    // Expected result: 1100 (1011 << 2, new bits filled with 0)
    bool test_result = (bv.length == 4);
    test_result      = test_result && (BitVecGet(&bv, 0) == false);
    test_result      = test_result && (BitVecGet(&bv, 1) == false);
    test_result      = test_result && (BitVecGet(&bv, 2) == true);
    test_result      = test_result && (BitVecGet(&bv, 3) == false);

    // Clean up
    BitVecDeinit(&bv);

    return test_result;
}

// Test BitVecShiftRight function
bool test_bitvec_shift_right(void) {
    printf("Testing BitVecShiftRight\n");

    BitVec bv = BitVecInit();

    // Set up bitvector: 1011
    BitVecPush(&bv, true);
    BitVecPush(&bv, false);
    BitVecPush(&bv, true);
    BitVecPush(&bv, true);

    // Shift right by 2 positions
    BitVecShiftRight(&bv, 2);

    // Expected result: 0010 (1011 >> 2, new bits filled with 0)
    bool test_result = (bv.length == 4);
    test_result      = test_result && (BitVecGet(&bv, 0) == false);
    test_result      = test_result && (BitVecGet(&bv, 1) == false);
    test_result      = test_result && (BitVecGet(&bv, 2) == true);
    test_result      = test_result && (BitVecGet(&bv, 3) == false);

    // Clean up
    BitVecDeinit(&bv);

    return test_result;
}

// Test BitVecRotateLeft function
bool test_bitvec_rotate_left(void) {
    printf("Testing BitVecRotateLeft\n");

    BitVec bv = BitVecInit();

    // Set up bitvector: 1011
    BitVecPush(&bv, true);
    BitVecPush(&bv, false);
    BitVecPush(&bv, true);
    BitVecPush(&bv, true);

    // Rotate left by 2 positions
    BitVecRotateLeft(&bv, 2);

    // Expected result: 1110 (1011 rotated left by 2)
    bool test_result = (bv.length == 4);
    test_result      = test_result && (BitVecGet(&bv, 0) == true);
    test_result      = test_result && (BitVecGet(&bv, 1) == true);
    test_result      = test_result && (BitVecGet(&bv, 2) == true);
    test_result      = test_result && (BitVecGet(&bv, 3) == false);

    // Clean up
    BitVecDeinit(&bv);

    return test_result;
}

// Test BitVecRotateRight function
bool test_bitvec_rotate_right(void) {
    printf("Testing BitVecRotateRight\n");

    BitVec bv = BitVecInit();

    // Set up bitvector: 1011
    BitVecPush(&bv, true);
    BitVecPush(&bv, false);
    BitVecPush(&bv, true);
    BitVecPush(&bv, true);

    // Rotate right by 1 position
    BitVecRotateRight(&bv, 1);

    // Expected result: 1101 (1011 rotated right by 1)
    bool test_result = (bv.length == 4);
    test_result      = test_result && (BitVecGet(&bv, 0) == true);
    test_result      = test_result && (BitVecGet(&bv, 1) == true);
    test_result      = test_result && (BitVecGet(&bv, 2) == false);
    test_result      = test_result && (BitVecGet(&bv, 3) == true);

    // Clean up
    BitVecDeinit(&bv);

    return test_result;
}

// Test BitVecReverse function
bool test_bitvec_reverse(void) {
    printf("Testing BitVecReverse\n");

    BitVec bv = BitVecInit();

    // Set up bitvector: 1011
    BitVecPush(&bv, true);
    BitVecPush(&bv, false);
    BitVecPush(&bv, true);
    BitVecPush(&bv, true);

    // Reverse the bits
    BitVecReverse(&bv);

    // Expected result: 1101 (1011 reversed)
    bool test_result = (bv.length == 4);
    test_result      = test_result && (BitVecGet(&bv, 0) == true);
    test_result      = test_result && (BitVecGet(&bv, 1) == true);
    test_result      = test_result && (BitVecGet(&bv, 2) == false);
    test_result      = test_result && (BitVecGet(&bv, 3) == true);

    // Clean up
    BitVecDeinit(&bv);

    return test_result;
}

// Edge case tests
bool test_bitvec_shift_edge_cases(void) {
    printf("Testing BitVec shift edge cases\n");

    BitVec bv     = BitVecInit();
    bool   result = true;

    // Test shift empty bitvec
    BitVecShiftLeft(&bv, 5);
    result = result && (bv.length == 0);

    BitVecShiftRight(&bv, 3);
    result = result && (bv.length == 0);

    // Test shift by 0 (should be no-op)
    BitVecPush(&bv, true);
    BitVecPush(&bv, false);
    BitVecShiftLeft(&bv, 0);
    result = result && (bv.length == 2);
    result = result && (BitVecGet(&bv, 0) == true);

    // Test shift larger than length (should clear all bits)
    BitVecShiftLeft(&bv, 10);
    result = result && (bv.length == 0); // Should clear when shifting everything out

    // Test large data shift
    BitVecClear(&bv);
    for (int i = 0; i < 1000; i++) {
        BitVecPush(&bv, i % 2 == 0);
    }
    BitVecShiftLeft(&bv, 1);
    result = result && (bv.length == 1000);

    BitVecDeinit(&bv);
    return result;
}

bool test_bitvec_rotate_edge_cases(void) {
    printf("Testing BitVec rotate edge cases\n");

    BitVec bv     = BitVecInit();
    bool   result = true;

    // Test rotate empty bitvec
    BitVecRotateLeft(&bv, 5);
    result = result && (bv.length == 0);

    // Test rotate by 0
    BitVecPush(&bv, true);
    BitVecRotateRight(&bv, 0);
    result = result && (BitVecGet(&bv, 0) == true);

    // Test rotate by length (should be no-op)
    BitVecPush(&bv, false);
    BitVecRotateLeft(&bv, 2);
    result = result && (bv.length == 2);

    // Test large rotate amount
    BitVecRotateRight(&bv, 1000);
    result = result && (bv.length == 2);

    BitVecDeinit(&bv);
    return result;
}

bool test_bitvec_bitwise_ops_edge_cases(void) {
    printf("Testing BitVec bitwise operations edge cases\n");

    BitVec bv1    = BitVecInit();
    BitVec bv2    = BitVecInit();
    bool   result = true;

    // Test operations on empty bitvecs
    BitVec result_bv = BitVecInit();
    BitVecAnd(&result_bv, &bv1, &bv2);
    result = result && (result_bv.length == 0);

    BitVecOr(&result_bv, &bv1, &bv2);
    result = result && (result_bv.length == 0);

    // Test operations with different lengths
    BitVecPush(&bv1, true);
    BitVecPush(&bv1, false);
    BitVecPush(&bv2, false);

    BitVecAnd(&result_bv, &bv1, &bv2);
    result = result && (result_bv.length >= 1); // Should handle gracefully

    // Test NOT on various sizes
    BitVecClear(&bv1);
    BitVecNot(&result_bv, &bv1);
    result = result && (result_bv.length == 0);

    BitVecPush(&bv1, true);
    BitVecNot(&result_bv, &bv1);
    result = result && (BitVecGet(&result_bv, 0) == false);

    BitVecDeinit(&result_bv);

    BitVecDeinit(&bv1);
    BitVecDeinit(&bv2);
    return result;
}

bool test_bitvec_reverse_edge_cases(void) {
    printf("Testing BitVecReverse edge cases\n");

    BitVec bv     = BitVecInit();
    bool   result = true;

    // Test reverse empty bitvec
    BitVecReverse(&bv);
    result = result && (bv.length == 0);

    // Test reverse single bit
    BitVecPush(&bv, true);
    BitVecReverse(&bv);
    result = result && (bv.length == 1);
    result = result && (BitVecGet(&bv, 0) == true);

    // Test reverse even length
    BitVecClear(&bv);
    BitVecPush(&bv, true);
    BitVecPush(&bv, false);
    BitVecReverse(&bv);
    result = result && (BitVecGet(&bv, 0) == false);
    result = result && (BitVecGet(&bv, 1) == true);

    // Test double reverse (should restore original)
    BitVecReverse(&bv);
    result = result && (BitVecGet(&bv, 0) == true);
    result = result && (BitVecGet(&bv, 1) == false);

    BitVecDeinit(&bv);
    return result;
}

// Deadend tests
bool test_bitvec_bitwise_null_failures(void) {
    printf("Testing BitVec bitwise NULL pointer handling\n");

    // Test NULL bitvec pointer - should abort
    BitVecShiftLeft(NULL, 1);

    return false;
}

bool test_bitvec_bitwise_ops_null_failures(void) {
    printf("Testing BitVec bitwise operations NULL handling\n");

    BitVec bv  = BitVecInit();
    BitVec bv2 = BitVecInit();

    // Test NULL pointer - should abort
    BitVecAnd(NULL, &bv, &bv2);

    BitVecDeinit(&bv);
    BitVecDeinit(&bv2);
    return false;
}

bool test_bitvec_reverse_null_failures(void) {
    printf("Testing BitVec reverse NULL handling\n");

    // Test NULL pointer - should abort
    BitVecReverse(NULL);

    return false;
}

// Main function that runs all tests
int main(void) {
    printf("[INFO] Starting BitVec.BitWise tests\n\n");

    // Array of normal test functions
    TestFunction tests[] = {
        test_bitvec_shift_left,
        test_bitvec_shift_right,
        test_bitvec_rotate_left,
        test_bitvec_rotate_right,
        test_bitvec_and,
        test_bitvec_or,
        test_bitvec_xor,
        test_bitvec_not,
        test_bitvec_reverse,
        test_bitvec_shift_edge_cases,
        test_bitvec_rotate_edge_cases,
        test_bitvec_bitwise_ops_edge_cases,
        test_bitvec_reverse_edge_cases
    };

    // Array of deadend test functions
    TestFunction deadend_tests[] = {
        test_bitvec_bitwise_null_failures,
        test_bitvec_bitwise_ops_null_failures,
        test_bitvec_reverse_null_failures
    };

    int total_tests         = sizeof(tests) / sizeof(tests[0]);
    int total_deadend_tests = sizeof(deadend_tests) / sizeof(deadend_tests[0]);

    // Run all tests using the centralized test driver
    return run_test_suite(tests, total_tests, deadend_tests, total_deadend_tests, "BitVec.BitWise");
}