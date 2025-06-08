#include <Misra/Std/Container/Bits.h>
#include <Misra/Std/Log.h>

#include <stdio.h>
#include <Misra/Types.h>

// Include test utilities
#include "../Util/TestRunner.h"

// Function prototypes
bool test_Bits_pop(void);
bool test_Bits_remove_single(void);
bool test_Bits_remove_range(void);
bool test_Bits_remove_first(void);
bool test_Bits_remove_last(void);
bool test_Bits_remove_all(void);
bool test_Bits_pop_edge_cases(void);
bool test_Bits_remove_single_edge_cases(void);
bool test_Bits_remove_range_edge_cases(void);
bool test_Bits_remove_first_last_edge_cases(void);
bool test_Bits_remove_all_edge_cases(void);
bool test_Bits_remove_null_failures(void);
bool test_Bits_remove_range_null_failures(void);
bool test_Bits_remove_invalid_range_failures(void);

// Test BitsPop function
bool test_Bits_pop(void) {
    printf("Testing BitsPop\n");

    Bits bv = BitsInit();

    // Add some bits
    BitsPush(&bv, true);
    BitsPush(&bv, false);
    BitsPush(&bv, true);

    // Pop the last bit
    bool popped = BitsPop(&bv);

    // Check result
    bool result = (popped == true) && (bv.length == 2);
    result      = result && (BitsGet(&bv, 0) == true);
    result      = result && (BitsGet(&bv, 1) == false);

    // Pop another bit
    popped = BitsPop(&bv);
    result = result && (popped == false) && (bv.length == 1);
    result = result && (BitsGet(&bv, 0) == true);

    // Pop the last bit
    popped = BitsPop(&bv);
    result = result && (popped == true) && (bv.length == 0);

    // Clean up
    BitsDeinit(&bv);

    return result;
}

// Test BitsRemove single bit function
bool test_Bits_remove_single(void) {
    printf("Testing BitsRemove (single bit)\n");

    Bits bv = BitsInit();

    // Add some bits: true, false, true, false, true
    BitsPush(&bv, true);
    BitsPush(&bv, false);
    BitsPush(&bv, true);
    BitsPush(&bv, false);
    BitsPush(&bv, true);

    // Remove bit at index 2 (middle true)
    bool removed = BitsRemove(&bv, 2);

    // Check result: true, false, false, true
    bool result = (removed == true) && (bv.length == 4);
    result      = result && (BitsGet(&bv, 0) == true);
    result      = result && (BitsGet(&bv, 1) == false);
    result      = result && (BitsGet(&bv, 2) == false);
    result      = result && (BitsGet(&bv, 3) == true);

    // Remove bit at index 0 (first bit)
    removed = BitsRemove(&bv, 0);
    result  = result && (removed == true) && (bv.length == 3);
    result  = result && (BitsGet(&bv, 0) == false);
    result  = result && (BitsGet(&bv, 1) == false);
    result  = result && (BitsGet(&bv, 2) == true);

    // Clean up
    BitsDeinit(&bv);

    return result;
}

// Test BitsRemoveRange function
bool test_Bits_remove_range(void) {
    printf("Testing BitsRemoveRange\n");

    Bits bv = BitsInit();

    // Add some bits: true, false, true, true, false, true
    BitsPush(&bv, true);
    BitsPush(&bv, false);
    BitsPush(&bv, true);
    BitsPush(&bv, true);
    BitsPush(&bv, false);
    BitsPush(&bv, true);

    // Remove range from index 1 to 3 (3 bits)
    BitsRemoveRange(&bv, 1, 3);

    // Check result: true, false, true (removed false, true, true)
    bool result = (bv.length == 3);
    result      = result && (BitsGet(&bv, 0) == true);
    result      = result && (BitsGet(&bv, 1) == false);
    result      = result && (BitsGet(&bv, 2) == true);

    // Clean up
    BitsDeinit(&bv);

    return result;
}

// Test BitsRemoveFirst function
bool test_Bits_remove_first(void) {
    printf("Testing BitsRemoveFirst\n");

    Bits bv = BitsInit();

    // Add some bits: true, false, true, false, true
    BitsPush(&bv, true);
    BitsPush(&bv, false);
    BitsPush(&bv, true);
    BitsPush(&bv, false);
    BitsPush(&bv, true);

    // Remove first occurrence of false
    bool found = BitsRemoveFirst(&bv, false);

    // Check result: true, true, false, true (removed first false at index 1)
    bool result = (found == true) && (bv.length == 4);
    result      = result && (BitsGet(&bv, 0) == true);
    result      = result && (BitsGet(&bv, 1) == true);
    result      = result && (BitsGet(&bv, 2) == false);
    result      = result && (BitsGet(&bv, 3) == true);

    // Try to remove first occurrence of a value that doesn't exist (after removal)
    // Actually, false still exists at index 2, so let's remove all falses first
    BitsRemoveFirst(&bv, false); // Remove the remaining false

    // Now try to remove false from a Bitstor with only trues
    found  = BitsRemoveFirst(&bv, false);
    result = result && (found == false) && (bv.length == 3);

    // Clean up
    BitsDeinit(&bv);

    return result;
}

// Test BitsRemoveLast function
bool test_Bits_remove_last(void) {
    printf("Testing BitsRemoveLast\n");

    Bits bv = BitsInit();

    // Add some bits: true, false, true, false, true
    BitsPush(&bv, true);
    BitsPush(&bv, false);
    BitsPush(&bv, true);
    BitsPush(&bv, false);
    BitsPush(&bv, true);

    // Remove last occurrence of false
    bool found = BitsRemoveLast(&bv, false);

    // Check result: true, false, true, true (removed last false at index 3)
    bool result = (found == true) && (bv.length == 4);
    result      = result && (BitsGet(&bv, 0) == true);
    result      = result && (BitsGet(&bv, 1) == false);
    result      = result && (BitsGet(&bv, 2) == true);
    result      = result && (BitsGet(&bv, 3) == true);

    // Remove last occurrence of true
    found = BitsRemoveLast(&bv, true);

    // Check result: true, false, true (removed last true at index 3)
    result = result && (found == true) && (bv.length == 3);
    result = result && (BitsGet(&bv, 0) == true);
    result = result && (BitsGet(&bv, 1) == false);
    result = result && (BitsGet(&bv, 2) == true);

    // Clean up
    BitsDeinit(&bv);

    return result;
}

// Test BitsRemoveAll function
bool test_Bits_remove_all(void) {
    printf("Testing BitsRemoveAll\n");

    Bits bv = BitsInit();

    // Add some bits: true, false, true, false, true, false
    BitsPush(&bv, true);
    BitsPush(&bv, false);
    BitsPush(&bv, true);
    BitsPush(&bv, false);
    BitsPush(&bv, true);
    BitsPush(&bv, false);

    // Remove all false bits
    u64 removed_count = BitsRemoveAll(&bv, false);

    // Check result: true, true, true (all false bits removed)
    bool result = (removed_count == 3) && (bv.length == 3);
    result      = result && (BitsGet(&bv, 0) == true);
    result      = result && (BitsGet(&bv, 1) == true);
    result      = result && (BitsGet(&bv, 2) == true);

    // Try to remove all false bits again (should return 0)
    removed_count = BitsRemoveAll(&bv, false);
    result        = result && (removed_count == 0) && (bv.length == 3);

    // Remove all true bits
    removed_count = BitsRemoveAll(&bv, true);
    result        = result && (removed_count == 3) && (bv.length == 0);

    // Clean up
    BitsDeinit(&bv);

    return result;
}

// Edge case tests
bool test_Bits_pop_edge_cases(void) {
    printf("Testing BitsPop edge cases\n");

    Bits bv     = BitsInit();
    bool result = true;

    // Test pop single element
    BitsPush(&bv, true);
    bool popped = BitsPop(&bv);
    result      = result && (popped == true) && (bv.length == 0);

    // Test multiple pops in sequence
    for (int i = 0; i < 100; i++) {
        BitsPush(&bv, i % 2 == 0);
    }
    for (int i = 99; i >= 0; i--) {
        popped = BitsPop(&bv);
        result = result && (popped == (i % 2 == 0));
        result = result && (bv.length == (size)i);
    }

    BitsDeinit(&bv);
    return result;
}

bool test_Bits_remove_single_edge_cases(void) {
    printf("Testing BitsRemove edge cases\n");

    Bits bv     = BitsInit();
    bool result = true;

    // Test remove last element
    BitsPush(&bv, true);
    bool removed = BitsRemove(&bv, 0);
    result       = result && (removed == true) && (bv.length == 0);

    // Test remove from large Bits
    for (int i = 0; i < 1000; i++) {
        BitsPush(&bv, i % 3 == 0);
    }

    // Remove middle element
    removed = BitsRemove(&bv, 500);
    result  = result && (removed == (500 % 3 == 0)); // Should return the value of the removed bit
    result  = result && (bv.length == 999);

    BitsDeinit(&bv);
    return result;
}

bool test_Bits_remove_range_edge_cases(void) {
    printf("Testing BitsRemoveRange edge cases\n");

    Bits bv     = BitsInit();
    bool result = true;

    // Test remove 0 elements (should be no-op)
    BitsPush(&bv, true);
    BitsRemoveRange(&bv, 0, 0);
    result = result && (bv.length == 1);

    // Test remove entire Bits
    BitsClear(&bv);
    for (int i = 0; i < 10; i++) {
        BitsPush(&bv, i % 2 == 0);
    }
    BitsRemoveRange(&bv, 0, 10);
    result = result && (bv.length == 0);

    // Test remove partial range
    for (int i = 0; i < 10; i++) {
        BitsPush(&bv, i % 2 == 0);
    }
    BitsRemoveRange(&bv, 1, 5);          // Remove 5 elements starting at index 1
    result = result && (bv.length == 5); // Should have 5 elements left

    BitsDeinit(&bv);
    return result;
}

bool test_Bits_remove_first_last_edge_cases(void) {
    printf("Testing BitsRemoveFirst/Last edge cases\n");

    Bits bv     = BitsInit();
    bool result = true;

    // Test remove from empty Bits
    bool found = BitsRemoveFirst(&bv, true);
    result     = result && (found == false) && (bv.length == 0);

    found  = BitsRemoveLast(&bv, false);
    result = result && (found == false) && (bv.length == 0);

    // Test remove when value doesn't exist
    BitsPush(&bv, true);
    BitsPush(&bv, true);
    found  = BitsRemoveFirst(&bv, false);
    result = result && (found == false) && (bv.length == 2);

    // Test remove single occurrence
    BitsClear(&bv);
    BitsPush(&bv, false);
    found  = BitsRemoveFirst(&bv, false);
    result = result && (found == true) && (bv.length == 0);

    // Test remove from large uniform data
    for (int i = 0; i < 1000; i++) {
        BitsPush(&bv, true);
    }
    found  = BitsRemoveFirst(&bv, true);
    result = result && (found == true) && (bv.length == 999);

    BitsDeinit(&bv);
    return result;
}

bool test_Bits_remove_all_edge_cases(void) {
    printf("Testing BitsRemoveAll edge cases\n");

    Bits bv     = BitsInit();
    bool result = true;

    // Test remove all from empty Bits
    u64 count = BitsRemoveAll(&bv, true);
    result    = result && (count == 0) && (bv.length == 0);

    // Test remove all when value doesn't exist
    BitsPush(&bv, true);
    BitsPush(&bv, true);
    count  = BitsRemoveAll(&bv, false);
    result = result && (count == 0) && (bv.length == 2);

    // Test remove all of uniform data
    BitsClear(&bv);
    for (int i = 0; i < 100; i++) {
        BitsPush(&bv, true);
    }
    count  = BitsRemoveAll(&bv, true);
    result = result && (count == 100) && (bv.length == 0);

    // Test remove all mixed data
    for (int i = 0; i < 1000; i++) {
        BitsPush(&bv, i % 2 == 0);
    }
    count  = BitsRemoveAll(&bv, false); // Remove odds
    result = result && (count == 500) && (bv.length == 500);

    BitsDeinit(&bv);
    return result;
}

// Deadend tests
bool test_Bits_remove_null_failures(void) {
    printf("Testing Bits remove NULL pointer handling\n");

    // Test NULL Bits pointer - should abort
    BitsPop(NULL);

    return false;
}

bool test_Bits_remove_range_null_failures(void) {
    printf("Testing Bits remove range NULL handling\n");

    // Test NULL Bits pointer - should abort
    BitsRemoveRange(NULL, 0, 1);

    return false;
}

bool test_Bits_remove_invalid_range_failures(void) {
    printf("Testing Bits remove invalid range handling\n");

    Bits bv = BitsInit();

    // Test removing beyond capacity limit - should abort
    BitsRemoveRange(&bv, SIZE_MAX, 1);

    BitsDeinit(&bv);
    return false;
}

bool test_Bits_pop_bounds_failures(void) {
    printf("Testing Bits pop bounds checking\n");

    Bits bv = BitsInit();

    // Test pop from empty Bits - should abort
    BitsPop(&bv);

    BitsDeinit(&bv);
    return false;
}

bool test_Bits_remove_bounds_failures(void) {
    printf("Testing Bits remove bounds checking\n");

    Bits bv = BitsInit();

    // Test remove from empty Bits - should abort
    BitsRemove(&bv, 0);

    BitsDeinit(&bv);
    return false;
}

bool test_Bits_remove_range_bounds_failures(void) {
    printf("Testing Bits remove range bounds checking\n");

    Bits bv = BitsInit();

    // Test remove range from empty Bits - should abort
    BitsRemoveRange(&bv, 0, 1);

    BitsDeinit(&bv);
    return false;
}

// Main function that runs all tests
int main(void) {
    printf("[INFO] Starting Bits.Remove tests\n\n");

    // Array of normal test functions
    TestFunction tests[] = {
        test_Bits_pop,
        test_Bits_remove_single,
        test_Bits_remove_range,
        test_Bits_remove_first,
        test_Bits_remove_last,
        test_Bits_remove_all,
        test_Bits_pop_edge_cases,
        test_Bits_remove_single_edge_cases,
        test_Bits_remove_range_edge_cases,
        test_Bits_remove_first_last_edge_cases,
        test_Bits_remove_all_edge_cases
    };

    // Array of deadend test functions
    TestFunction deadend_tests[] = {
        test_Bits_remove_null_failures,
        test_Bits_remove_range_null_failures,
        test_Bits_remove_invalid_range_failures,
        test_Bits_pop_bounds_failures,
        test_Bits_remove_bounds_failures,
        test_Bits_remove_range_bounds_failures
    };

    int total_tests         = sizeof(tests) / sizeof(tests[0]);
    int total_deadend_tests = sizeof(deadend_tests) / sizeof(deadend_tests[0]);

    // Run all tests using the centralized test driver
    return run_test_suite(tests, total_tests, deadend_tests, total_deadend_tests, "Bits.Remove");
}
