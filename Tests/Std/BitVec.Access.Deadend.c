#include <Misra/Std/Container/Bits.h>
#include <Misra/Std/Log.h>

#include <stdio.h>
#include <Misra/Types.h> // For size and other type definitions

// Include test utilities
#include "../Util/TestRunner.h"

// Function prototypes for deadend tests
bool test_Bits_find_deadend_tests(void);
bool test_Bits_predicate_deadend_tests(void);
bool test_Bits_longest_run_deadend_tests(void);
bool test_Bits_access_null_failures(void);
bool test_Bits_set_null_failures(void);
bool test_Bits_flip_null_failures(void);
bool test_Bits_get_bounds_failures(void);
bool test_Bits_set_bounds_failures(void);
bool test_Bits_flip_bounds_failures(void);
bool test_Bits_get_large_index_failures(void);
bool test_Bits_set_large_index_failures(void);
bool test_Bits_flip_edge_index_failures(void);
bool test_Bits_count_null_failures(void);
bool test_Bits_get_max_index_failures(void);

// Deadend tests - testing NULL pointers and invalid conditions that should cause fatal errors
bool test_Bits_find_deadend_tests(void) {
    printf("Testing BitsFind deadend scenarios\n");

    // This should cause LOG_FATAL and terminate the program
    BitsFind(NULL, true);

    return true; // Should never reach here
}

bool test_Bits_predicate_deadend_tests(void) {
    printf("Testing Bits predicate deadend scenarios\n");

    // This should cause LOG_FATAL and terminate the program
    BitsAll(NULL, true);

    return true; // Should never reach here
}

bool test_Bits_longest_run_deadend_tests(void) {
    printf("Testing BitsLongestRun deadend scenarios\n");

    // This should cause LOG_FATAL and terminate the program
    BitsLongestRun(NULL, true);

    return true; // Should never reach here
}

// Deadend tests
bool test_Bits_access_null_failures(void) {
    printf("Testing Bits access NULL pointer handling\n");

    // Test NULL Bits pointer - should abort
    BitsGet(NULL, 0);

    return false;
}

bool test_Bits_set_null_failures(void) {
    printf("Testing Bits set NULL pointer handling\n");

    // Test NULL Bits pointer - should abort
    BitsSet(NULL, 0, true);

    return false;
}

bool test_Bits_flip_null_failures(void) {
    printf("Testing Bits flip NULL pointer handling\n");

    // Test NULL Bits pointer - should abort
    BitsFlip(NULL, 0);

    return false;
}

bool test_Bits_get_bounds_failures(void) {
    printf("Testing Bits get bounds checking\n");

    Bits bv = BitsInit();

    // Test get from empty Bits - should abort
    BitsGet(&bv, 0);

    BitsDeinit(&bv);
    return false;
}

bool test_Bits_set_bounds_failures(void) {
    printf("Testing Bits set bounds checking\n");

    Bits bv = BitsInit();

    // Test set on empty Bits - should abort
    BitsSet(&bv, 0, true);

    BitsDeinit(&bv);
    return false;
}

bool test_Bits_flip_bounds_failures(void) {
    printf("Testing Bits flip bounds checking\n");

    Bits bv = BitsInit();

    // Test flip on empty Bits - should abort
    BitsFlip(&bv, 0);

    BitsDeinit(&bv);
    return false;
}

// NEW: More specific bounds checking deadend tests
bool test_Bits_get_large_index_failures(void) {
    printf("Testing Bits get with large out-of-bounds index\n");

    Bits bv = BitsInit();
    BitsPush(&bv, true);
    BitsPush(&bv, false);
    BitsPush(&bv, true);

    // Test with index way beyond length (3) - should abort
    BitsGet(&bv, 1000);

    BitsDeinit(&bv);
    return false;
}

bool test_Bits_set_large_index_failures(void) {
    printf("Testing Bits set with large out-of-bounds index\n");

    Bits bv = BitsInit();
    BitsPush(&bv, true);
    BitsPush(&bv, false);

    // Test with index way beyond length (2) - should abort
    BitsSet(&bv, 500, true);

    BitsDeinit(&bv);
    return false;
}

bool test_Bits_flip_edge_index_failures(void) {
    printf("Testing Bits flip with edge case out-of-bounds index\n");

    Bits bv = BitsInit();
    for (int i = 0; i < 10; i++) {
        BitsPush(&bv, i % 2 == 0);
    }

    // Test with index exactly at length (invalid) - should abort
    BitsFlip(&bv, 10);

    BitsDeinit(&bv);
    return false;
}

bool test_Bits_count_null_failures(void) {
    printf("Testing Bits count operations with NULL pointer\n");

    // Test NULL Bits pointer - should abort
    BitsCountOnes(NULL);

    return false;
}

bool test_Bits_get_max_index_failures(void) {
    printf("Testing Bits get with maximum index value\n");

    Bits bv = BitsInit();
    BitsPush(&bv, true);

    // Test with maximum possible index value - should abort
    BitsGet(&bv, SIZE_MAX);

    BitsDeinit(&bv);
    return false;
}

// Main function that runs all deadend tests
int main(void) {
    printf("[INFO] Starting Bits.Access.Deadend tests\n\n");

    // Deadend tests that would cause program termination
    TestFunction deadend_tests[] = {
        test_Bits_find_deadend_tests,
        test_Bits_predicate_deadend_tests,
        test_Bits_longest_run_deadend_tests,
        test_Bits_access_null_failures,
        test_Bits_set_null_failures,
        test_Bits_flip_null_failures,
        test_Bits_get_bounds_failures,
        test_Bits_set_bounds_failures,
        test_Bits_flip_bounds_failures,
        test_Bits_get_large_index_failures,
        test_Bits_set_large_index_failures,
        test_Bits_flip_edge_index_failures,
        test_Bits_count_null_failures,
        test_Bits_get_max_index_failures
    };

    int total_deadend_tests = sizeof(deadend_tests) / sizeof(deadend_tests[0]);

    // Run all deadend tests using the centralized test driver
    return run_test_suite(NULL, 0, deadend_tests, total_deadend_tests, "Bits.Access.Deadend");
}
