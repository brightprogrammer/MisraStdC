#include <Misra/Std/Container/BitVec.h>
#include <Misra/Std/Log.h>
#include <stdbool.h>
#include <stdio.h>
#include <Misra/Types.h>

// Include test utilities
#include "../Util/TestRunner.h"

// Function prototypes
bool test_bitvec_get_set(void);
bool test_bitvec_flip(void);
bool test_bitvec_length_capacity(void);
bool test_bitvec_empty_check(void);
bool test_bitvec_count_operations(void);

// Test basic get/set operations
bool test_bitvec_get_set(void) {
    printf("Testing BitVec get/set operations\n");

    BitVec bitvec = BitVecInit();

    // Reserve some space and resize to have bits to work with
    BitVecResize(&bitvec, 10);

    // Test setting and getting bits
    BitVecSet(&bitvec, 0, true);
    BitVecSet(&bitvec, 5, true);
    BitVecSet(&bitvec, 9, false);

    bool bit0   = BitVecGet(&bitvec, 0);
    bool bit5   = BitVecGet(&bitvec, 5);
    bool bit9   = BitVecGet(&bitvec, 9);
    bool result = (bit0 == true && bit5 == true && bit9 == false);

    // Test getting unset bits (should be false by default)
    bool bit1 = BitVecGet(&bitvec, 1);
    bool bit3 = BitVecGet(&bitvec, 3);
    result    = result && (bit1 == false && bit3 == false);

    BitVecDeinit(&bitvec);
    return result;
}

// Test flip operation
bool test_bitvec_flip(void) {
    printf("Testing BitVec flip operation\n");

    BitVec bitvec = BitVecInit();
    BitVecResize(&bitvec, 5);

    // Set some initial values
    BitVecSet(&bitvec, 0, true);
    BitVecSet(&bitvec, 1, false);

    // Flip them
    BitVecFlip(&bitvec, 0);
    BitVecFlip(&bitvec, 1);

    bool result = (BitVecGet(&bitvec, 0) == false && BitVecGet(&bitvec, 1) == true);

    BitVecDeinit(&bitvec);
    return result;
}

// Test length and capacity operations
bool test_bitvec_length_capacity(void) {
    printf("Testing BitVec length and capacity operations\n");

    BitVec bitvec = BitVecInit();

    // Initially should be empty
    bool result = (BitVecLen(&bitvec) == 0 && BitVecCapacity(&bitvec) == 0);

    // Reserve space
    BitVecReserve(&bitvec, 64);
    result = result && (BitVecCapacity(&bitvec) >= 64);

    // Resize
    BitVecResize(&bitvec, 32);
    result = result && (BitVecLen(&bitvec) == 32);

    BitVecDeinit(&bitvec);
    return result;
}

// Test empty check
bool test_bitvec_empty_check(void) {
    printf("Testing BitVec empty check\n");

    BitVec bitvec = BitVecInit();

    // Should be empty initially
    bool result = BitVecEmpty(&bitvec);

    // After resizing should not be empty
    BitVecResize(&bitvec, 1);
    result = result && !BitVecEmpty(&bitvec);

    // After clearing should be empty again
    BitVecClear(&bitvec);
    result = result && BitVecEmpty(&bitvec);

    BitVecDeinit(&bitvec);
    return result;
}

// Test count operations
bool test_bitvec_count_operations(void) {
    printf("Testing BitVec count operations\n");

    BitVec bitvec = BitVecInit();
    BitVecResize(&bitvec, 10);

    // Set some bits
    BitVecSet(&bitvec, 0, true);
    BitVecSet(&bitvec, 2, true);
    BitVecSet(&bitvec, 4, true);
    // Bits 1, 3, 5, 6, 7, 8, 9 should be false

    bool result = (BitVecCountOnes(&bitvec) == 3 && BitVecCountZeros(&bitvec) == 7);

    BitVecDeinit(&bitvec);
    return result;
}

// Main function that runs all tests
int main(void) {
    printf("[INFO] Starting BitVec.Access tests\n\n");

    // Array of test functions
    TestFunction tests[] = {
        test_bitvec_get_set,
        test_bitvec_flip,
        test_bitvec_length_capacity,
        test_bitvec_empty_check,
        test_bitvec_count_operations
    };

    int total_tests = sizeof(tests) / sizeof(tests[0]);

    // Run all tests using the centralized test driver
    return run_test_suite(tests, total_tests, NULL, 0, "BitVec.Access");
}