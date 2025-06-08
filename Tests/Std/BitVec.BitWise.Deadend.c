#include <Misra/Std/Container/Bits.h>
#include <Misra/Std/Log.h>

#include <stdio.h>
#include <Misra/Types.h>

// Include test utilities
#include "../Util/TestRunner.h"

// Function prototypes for deadend tests
bool test_Bits_bitwise_null_failures(void);
bool test_Bits_bitwise_ops_null_failures(void);
bool test_Bits_reverse_null_failures(void);
bool test_Bits_shift_ops_null_failures(void);
bool test_Bits_rotate_ops_null_failures(void);
bool test_Bits_and_result_null_failures(void);
bool test_Bits_or_operand_null_failures(void);
bool test_Bits_xor_second_operand_null_failures(void);
bool test_Bits_not_null_failures(void);

// Deadend tests
bool test_Bits_bitwise_null_failures(void) {
    printf("Testing Bits bitwise NULL pointer handling\n");

    // Test NULL Bits pointer - should abort
    BitsShiftLeft(NULL, 1);

    return false;
}

bool test_Bits_bitwise_ops_null_failures(void) {
    printf("Testing Bits bitwise operations NULL handling\n");

    Bits bv  = BitsInit();
    Bits bv2 = BitsInit();

    // Test NULL pointer - should abort
    BitsAnd(NULL, &bv, &bv2);

    BitsDeinit(&bv);
    BitsDeinit(&bv2);
    return false;
}

bool test_Bits_reverse_null_failures(void) {
    printf("Testing Bits reverse NULL handling\n");

    // Test NULL pointer - should abort
    BitsReverse(NULL);

    return false;
}

// NEW: Additional deadend tests
bool test_Bits_shift_ops_null_failures(void) {
    printf("Testing Bits shift operations NULL handling\n");

    // Test NULL pointer for shift right - should abort
    BitsShiftRight(NULL, 5);

    return false;
}

bool test_Bits_rotate_ops_null_failures(void) {
    printf("Testing Bits rotate operations NULL handling\n");

    // Test NULL pointer for rotate - should abort
    BitsRotateLeft(NULL, 3);

    return false;
}

bool test_Bits_and_result_null_failures(void) {
    printf("Testing Bits AND with NULL result handling\n");

    Bits bv1 = BitsInit();
    Bits bv2 = BitsInit();
    BitsPush(&bv1, true);
    BitsPush(&bv2, false);

    // Test NULL result pointer - should abort
    BitsAnd(NULL, &bv1, &bv2);

    BitsDeinit(&bv1);
    BitsDeinit(&bv2);
    return false;
}

bool test_Bits_or_operand_null_failures(void) {
    printf("Testing Bits OR with NULL operand handling\n");

    Bits result = BitsInit();
    Bits bv1    = BitsInit();

    // Test NULL operand - should abort
    BitsOr(&result, &bv1, NULL);

    BitsDeinit(&result);
    BitsDeinit(&bv1);
    return false;
}

bool test_Bits_xor_second_operand_null_failures(void) {
    printf("Testing Bits XOR with NULL second operand handling\n");

    Bits result = BitsInit();
    Bits bv1    = BitsInit();

    // Test NULL second operand - should abort
    BitsXor(&result, NULL, &bv1);

    BitsDeinit(&result);
    BitsDeinit(&bv1);
    return false;
}

bool test_Bits_not_null_failures(void) {
    printf("Testing Bits NOT with NULL handling\n");

    Bits result = BitsInit();

    // Test NULL operand - should abort
    BitsNot(&result, NULL);

    BitsDeinit(&result);
    return false;
}

// Main function that runs all deadend tests
int main(void) {
    printf("[INFO] Starting Bits.BitWise.Deadend tests\n\n");

    // Array of deadend test functions
    TestFunction deadend_tests[] = {
        test_Bits_bitwise_null_failures,
        test_Bits_bitwise_ops_null_failures,
        test_Bits_reverse_null_failures,
        test_Bits_shift_ops_null_failures,
        test_Bits_rotate_ops_null_failures,
        test_Bits_and_result_null_failures,
        test_Bits_or_operand_null_failures,
        test_Bits_xor_second_operand_null_failures,
        test_Bits_not_null_failures
    };

    int total_deadend_tests = sizeof(deadend_tests) / sizeof(deadend_tests[0]);

    // Run all deadend tests using the centralized test driver
    return run_test_suite(NULL, 0, deadend_tests, total_deadend_tests, "Bits.BitWise.Deadend");
}
