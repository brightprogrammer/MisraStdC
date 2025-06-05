#include <Misra/Std/Container/BitVec.h>
#include <Misra/Std/Log.h>
#include <stdbool.h>
#include <stdio.h>
#include <Misra/Types.h>

// Include test utilities
#include "../Util/TestRunner.h"

// Function prototypes
bool test_bitvec_type_basic(void);
bool test_bitvec_validate(void);

// Test basic BitVec type functionality
bool test_bitvec_type_basic(void) {
    printf("Testing basic BitVec type functionality\n");

    // Create a bitvector
    BitVec bitvec = BitVecInit();

    // Check initial state
    bool result = (bitvec.length == 0 && bitvec.capacity == 0 && bitvec.data == NULL && bitvec.byte_size == 0);

    // Clean up
    BitVecDeinit(&bitvec);

    return result;
}

// Test ValidateBitVec macro
bool test_bitvec_validate(void) {
    printf("Testing ValidateBitVec macro\n");

    // Create a valid bitvector
    BitVec bitvec = BitVecInit();

    // This should not abort
    ValidateBitVec(&bitvec);

    // Clean up
    BitVecDeinit(&bitvec);

    // Note: We can't easily test the negative case (invalid bitvector)
    // as it would abort the program

    return true;
}

// Main function that runs all tests
int main(void) {
    printf("[INFO] Starting BitVec.Type tests\n\n");

    // Array of test functions
    TestFunction tests[] = {test_bitvec_type_basic, test_bitvec_validate};

    int total_tests = sizeof(tests) / sizeof(tests[0]);

    // Run all tests using the centralized test driver
    return run_test_suite(tests, total_tests, NULL, 0, "BitVec.Type");
}