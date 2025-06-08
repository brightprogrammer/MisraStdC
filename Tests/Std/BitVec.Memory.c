#include <Misra/Std/Container/Bits.h>
#include <Misra/Std/Log.h>
#include <stdio.h>
#include <Misra/Types.h>

// Include test utilities
#include "../Util/TestRunner.h"

// Function prototypes
bool test_Bits_shrink_to_fit(void);
bool test_Bits_set_capacity(void);
bool test_Bits_swap(void);
bool test_Bits_clone(void);
bool test_Bits_shrink_to_fit_edge_cases(void);
bool test_Bits_set_capacity_edge_cases(void);
bool test_Bits_swap_edge_cases(void);
bool test_Bits_clone_edge_cases(void);
bool test_Bits_memory_stress_test(void);
bool test_Bits_memory_null_failures(void);
bool test_Bits_swap_null_failures(void);
bool test_Bits_clone_null_failures(void);

// Test BitsShrinkToFit function
bool test_Bits_shrink_to_fit(void) {
    printf("Testing BitsShrinkToFit\n");

    Bits bv = BitsInit();

    // Add some bits
    BitsPush(&bv, true);
    BitsPush(&bv, false);
    BitsPush(&bv, true);

    // Force capacity to be larger than needed by reserving space
    BitsReserve(&bv, 100);

    // Check that capacity is larger than length
    u64  initial_capacity = bv.capacity;
    bool result           = (initial_capacity >= 100) && (bv.length == 3);

    // Shrink to fit
    BitsTryReduceSpace(&bv);

    // Check that capacity is now closer to length
    result = result && (bv.capacity < initial_capacity);
    result = result && (bv.capacity >= bv.length);

    // Check that data is still intact
    result = result && (bv.length == 3);
    result = result && (BitsGet(&bv, 0) == true);
    result = result && (BitsGet(&bv, 1) == false);
    result = result && (BitsGet(&bv, 2) == true);

    // Clean up
    BitsDeinit(&bv);

    return result;
}

// Test BitsClone function
bool test_Bits_clone(void) {
    printf("Testing BitsClone\n");

    Bits original = BitsInit();

    // Set up original Bitstor
    BitsPush(&original, true);
    BitsPush(&original, false);
    BitsPush(&original, true);
    BitsPush(&original, false);

    // Clone the Bitstor
    Bits clone = BitsClone(&original);

    // Check that clone has same content as original
    bool result = (clone.length == original.length);

    for (u64 i = 0; i < original.length; i++) {
        result = result && (BitsGet(&clone, i) == BitsGet(&original, i));
    }

    // Verify they are independent by modifying original
    BitsPush(&original, true);

    // Clone should remain unchanged
    result = result && (clone.length == 4) && (original.length == 5);
    result = result && (BitsGet(&clone, 0) == true);
    result = result && (BitsGet(&clone, 1) == false);
    result = result && (BitsGet(&clone, 2) == true);
    result = result && (BitsGet(&clone, 3) == false);

    // Modify clone to verify independence
    BitsSet(&clone, 0, false);

    // Original should remain unchanged at index 0
    result = result && (BitsGet(&original, 0) == true);
    result = result && (BitsGet(&clone, 0) == false);

    // Clean up
    BitsDeinit(&original);
    BitsDeinit(&clone);

    return result;
}

// Edge case tests
bool test_Bits_shrink_to_fit_edge_cases(void) {
    printf("Testing BitsShrinkToFit edge cases\n");

    Bits bv     = BitsInit();
    bool   result = true;

    // Test shrink on empty Bits
    BitsTryReduceSpace(&bv);
    result = result && (bv.length == 0) && (bv.capacity >= 0);

    // Test shrink on single element
    BitsPush(&bv, true);
    BitsTryReduceSpace(&bv);
    result = result && (bv.length == 1) && (bv.capacity >= 1);
    result = result && (BitsGet(&bv, 0) == true);

    // Test multiple shrinks (should be safe)
    BitsTryReduceSpace(&bv);
    BitsTryReduceSpace(&bv);
    result = result && (bv.length == 1);

    // Test shrink after reserve and clear
    BitsReserve(&bv, 1000);
    BitsClear(&bv);
    BitsTryReduceSpace(&bv);
    result = result && (bv.length == 0);

    BitsDeinit(&bv);
    return result;
}

bool test_Bits_clone_edge_cases(void) {
    printf("Testing BitsClone edge cases\n");

    Bits bv     = BitsInit();
    bool   result = true;

    // Test clone empty Bits
    Bits clone1 = BitsClone(&bv);
    result        = result && (clone1.length == 0);
    BitsDeinit(&clone1);

    // Test clone single element
    BitsPush(&bv, true);
    Bits clone2 = BitsClone(&bv);
    result        = result && (clone2.length == 1);
    result        = result && (BitsGet(&clone2, 0) == true);
    BitsDeinit(&clone2);

    // Test clone large data
    BitsClear(&bv);
    for (int i = 0; i < 1000; i++) {
        BitsPush(&bv, i % 2 == 0);
    }

    Bits clone3 = BitsClone(&bv);
    result        = result && (clone3.length == 1000);

    // Verify all bits match
    for (u64 i = 0; i < 1000; i++) {
        result = result && (BitsGet(&clone3, i) == BitsGet(&bv, i));
    }

    // Test independence - modify original
    BitsSet(&bv, 0, !BitsGet(&bv, 0));
    result = result && (BitsGet(&clone3, 0) != BitsGet(&bv, 0));

    BitsDeinit(&clone3);
    BitsDeinit(&bv);
    return result;
}

bool test_Bits_memory_stress_test(void) {
    printf("Testing Bits memory stress test\n");

    bool result = true;

    // Test multiple clone/swap/reu64 cycles
    for (int cycle = 0; cycle < 10; cycle++) {
        Bits bv1 = BitsInit();
        Bits bv2 = BitsInit();

        // Add random-sized data
        for (int i = 0; i < cycle * 10; i++) {
            BitsPush(&bv1, i % 2 == 0);
            BitsPush(&bv2, i % 3 == 0);
        }

        Bits clone = BitsClone(&bv1);

        // Reu64 operations
        BitsResize(&bv1, cycle * 20);
        BitsTryReduceSpace(&bv2);

        // Verify data integrity
        result = result && (clone.length == cycle * 10);
        if (cycle > 0) {
            result = result && (BitsGet(&clone, 0) == true); // 0 % 2 == 0
        }

        BitsDeinit(&bv1);
        BitsDeinit(&bv2);
        BitsDeinit(&clone);
    }

    return result;
}

// Deadend tests
bool test_Bits_memory_null_failures(void) {
    printf("Testing Bits memory NULL pointer handling\n");

    // Test NULL Bits pointer - should abort
    BitsTryReduceSpace(NULL);

    return false;
}

bool test_Bits_clone_null_failures(void) {
    printf("Testing Bits clone NULL handling\n");

    // Test NULL pointer - should abort
    BitsClone(NULL);

    return false;
}

// Main function that runs all tests
int main(void) {
    printf("[INFO] Starting Bits.Memory tests\n\n");

    // Array of normal test functions
    TestFunction tests[] = {
        test_Bits_shrink_to_fit,
        test_Bits_clone,
        test_Bits_shrink_to_fit_edge_cases,
        test_Bits_clone_edge_cases,
        test_Bits_memory_stress_test
    };

    // Array of deadend test functions
    TestFunction deadend_tests[] = {
        test_Bits_memory_null_failures,
        test_Bits_clone_null_failures
    };

    int total_tests         = sizeof(tests) / sizeof(tests[0]);
    int total_deadend_tests = sizeof(deadend_tests) / sizeof(deadend_tests[0]);

    // Run all tests using the centralized test driver
    return run_test_suite(tests, total_tests, deadend_tests, total_deadend_tests, "Bits.Memory");
}

