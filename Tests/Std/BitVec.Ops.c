#include <Misra/Std/Container/BitVec.h>
#include <Misra/Std/Log.h>
#include <stdbool.h>
#include <stdio.h>
#include <Misra/Types.h>

// Include test utilities
#include "../Util/TestRunner.h"

// Function prototypes
bool test_bitvec_push_pop(void);
bool test_bitvec_insert_remove(void);
bool test_bitvec_bitwise_operations(void);

// Test push/pop operations
bool test_bitvec_push_pop(void) {
    printf("Testing BitVec push/pop operations\n");

    BitVec bitvec = BitVecInit();

    // Push some bits
    BitVecPush(&bitvec, true);
    BitVecPush(&bitvec, false);
    BitVecPush(&bitvec, true);

    bool result = (BitVecLen(&bitvec) == 3);

    // Check values
    result = result && (BitVecGet(&bitvec, 0) == true);
    result = result && (BitVecGet(&bitvec, 1) == false);
    result = result && (BitVecGet(&bitvec, 2) == true);

    // Pop bits
    bool popped1 = BitVecPop(&bitvec);
    bool popped2 = BitVecPop(&bitvec);

    result = result && (popped1 == true);
    result = result && (popped2 == false);
    result = result && (BitVecLen(&bitvec) == 1);
    result = result && (BitVecGet(&bitvec, 0) == true);

    BitVecDeinit(&bitvec);
    return result;
}

// Test insert/remove operations
bool test_bitvec_insert_remove(void) {
    printf("Testing BitVec insert/remove operations\n");

    BitVec bitvec = BitVecInit();

    // Start with some bits
    BitVecPush(&bitvec, true);
    BitVecPush(&bitvec, false);
    BitVecPush(&bitvec, true);
    // bitvec: [1, 0, 1]

    // Insert at beginning
    BitVecInsert(&bitvec, 0, false);
    // bitvec: [0, 1, 0, 1]

    bool result = (BitVecLen(&bitvec) == 4);
    result      = result && (BitVecGet(&bitvec, 0) == false);
    result      = result && (BitVecGet(&bitvec, 1) == true);
    result      = result && (BitVecGet(&bitvec, 2) == false);
    result      = result && (BitVecGet(&bitvec, 3) == true);

    // Insert in middle
    BitVecInsert(&bitvec, 2, true);
    // bitvec: [0, 1, 1, 0, 1]

    result = result && (BitVecLen(&bitvec) == 5);
    result = result && (BitVecGet(&bitvec, 2) == true);

    // Remove from beginning
    BitVecRemove(&bitvec, 0);
    // bitvec: [1, 1, 0, 1]

    result = result && (BitVecLen(&bitvec) == 4);
    result = result && (BitVecGet(&bitvec, 0) == true);

    BitVecDeinit(&bitvec);
    return result;
}

// Test bitwise operations
bool test_bitvec_bitwise_operations(void) {
    printf("Testing BitVec bitwise operations\n");

    BitVec a      = BitVecInit();
    BitVec b      = BitVecInit();
    BitVec result = BitVecInit();

    // Setup test vectors
    // a: [1, 0, 1, 0]
    BitVecPush(&a, true);
    BitVecPush(&a, false);
    BitVecPush(&a, true);
    BitVecPush(&a, false);

    // b: [1, 1, 0, 0]
    BitVecPush(&b, true);
    BitVecPush(&b, true);
    BitVecPush(&b, false);
    BitVecPush(&b, false);

    // Test AND operation
    BitVecAnd(&result, &a, &b);
    // result should be: [1, 0, 0, 0]
    bool test_result = (BitVecLen(&result) == 4);
    test_result      = test_result && (BitVecGet(&result, 0) == true);
    test_result      = test_result && (BitVecGet(&result, 1) == false);
    test_result      = test_result && (BitVecGet(&result, 2) == false);
    test_result      = test_result && (BitVecGet(&result, 3) == false);

    // Test OR operation
    BitVecOr(&result, &a, &b);
    // result should be: [1, 1, 1, 0]
    test_result = test_result && (BitVecGet(&result, 0) == true);
    test_result = test_result && (BitVecGet(&result, 1) == true);
    test_result = test_result && (BitVecGet(&result, 2) == true);
    test_result = test_result && (BitVecGet(&result, 3) == false);

    // Test XOR operation
    BitVecXor(&result, &a, &b);
    // result should be: [0, 1, 1, 0]
    test_result = test_result && (BitVecGet(&result, 0) == false);
    test_result = test_result && (BitVecGet(&result, 1) == true);
    test_result = test_result && (BitVecGet(&result, 2) == true);
    test_result = test_result && (BitVecGet(&result, 3) == false);

    // Test NOT operation
    BitVecNot(&result, &a);
    // result should be: [0, 1, 0, 1]
    test_result = test_result && (BitVecGet(&result, 0) == false);
    test_result = test_result && (BitVecGet(&result, 1) == true);
    test_result = test_result && (BitVecGet(&result, 2) == false);
    test_result = test_result && (BitVecGet(&result, 3) == true);

    BitVecDeinit(&a);
    BitVecDeinit(&b);
    BitVecDeinit(&result);
    return test_result;
}

// Main function that runs all tests
int main(void) {
    printf("[INFO] Starting BitVec.Ops tests\n\n");

    // Array of test functions
    TestFunction tests[] = {test_bitvec_push_pop, test_bitvec_insert_remove, test_bitvec_bitwise_operations};

    int total_tests = sizeof(tests) / sizeof(tests[0]);

    // Run all tests using the centralized test driver
    return run_test_suite(tests, total_tests, NULL, 0, "BitVec.Ops");
}