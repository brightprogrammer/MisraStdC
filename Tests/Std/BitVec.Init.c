#include <Misra/Std/Container/Bits.h>
#include <Misra/Std/Log.h>
#include <stdio.h>
#include <Misra/Types.h>

// Include test utilities
#include "../Util/TestRunner.h"

// Function prototypes
bool test_Bits_init(void);
bool test_Bits_deinit(void);
bool test_Bits_reserve(void);
bool test_Bits_clear(void);
bool test_Bits_resize(void);
bool test_Bits_init_edge_cases(void);
bool test_Bits_deinit_edge_cases(void);
bool test_Bits_reserve_edge_cases(void);
bool test_Bits_clear_edge_cases(void);
bool test_Bits_resize_edge_cases(void);
bool test_Bits_multiple_cycles(void);

// Test BitsInit function
bool test_Bits_init(void) {
    printf("Testing BitsInit\n");

    // Test basic initialization
    Bits bv = BitsInit();

    // Check initial state
    bool result = (bv.length == 0);
    result      = result && (bv.capacity == 0);
    result      = result && (bv.data == NULL);
    result      = result && (bv.byte_size == 0);

    // Clean up
    BitsDeinit(&bv);

    return result;
}

// Test BitsDeinit function
bool test_Bits_deinit(void) {
    printf("Testing BitsDeinit\n");

    Bits bv = BitsInit();

    // Add some data to make sure deinitialization works with allocated memory
    BitsPush(&bv, true);
    BitsPush(&bv, false);
    BitsPush(&bv, true);

    // Check that data was allocated
    bool result = (bv.length == 3) && (bv.data != NULL);

    // Deinitialize
    BitsDeinit(&bv);

    // After deinitialization, the Bitstor should be in a safe state
    // Note: We can't easily test that memory was freed without causing issues,
    // but we can check that the structure is reset to safe values
    result = result && (bv.length == 0);
    result = result && (bv.capacity == 0);
    result = result && (bv.data == NULL);
    result = result && (bv.byte_size == 0);

    return result;
}

// Test BitsReserve function
bool test_Bits_reserve(void) {
    printf("Testing BitsReserve\n");

    Bits bv = BitsInit();

    // Reserve space for 50 bits
    BitsReserve(&bv, 50);

    // Check that capacity was increased
    bool result = (bv.capacity >= 50);
    result      = result && (bv.length == 0);  // Length should still be 0
    result      = result && (bv.data != NULL); // Memory should be allocated

    // Add some bits to make sure the reserved space works
    for (int i = 0; i < 10; i++) {
        BitsPush(&bv, (i % 2 == 0));
    }

    result = result && (bv.length == 10);
    result = result && (bv.capacity >= 50); // Should still have the reserved capacity

    // Test reserving less than current capacity (should be no-op)
    u64 original_capacity = bv.capacity;
    BitsReserve(&bv, 25);
    result = result && (bv.capacity == original_capacity);

    // Clean up
    BitsDeinit(&bv);

    return result;
}

// Test BitsClear function
bool test_Bits_clear(void) {
    printf("Testing BitsClear\n");

    Bits bv = BitsInit();

    // Add some data
    BitsPush(&bv, true);
    BitsPush(&bv, false);
    BitsPush(&bv, true);
    BitsPush(&bv, false);

    // Check initial state
    bool result            = (bv.length == 4) && (bv.data != NULL);
    u64  original_capacity = bv.capacity;

    // Clear the Bitstor
    BitsClear(&bv);

    // Check that length is 0 but capacity and memory allocation remain
    result = result && (bv.length == 0);
    result = result && (bv.capacity == original_capacity);
    result = result && (bv.data != NULL); // Memory should still be allocated

    // Test that we can still add data after clearing
    BitsPush(&bv, true);
    result = result && (bv.length == 1);
    result = result && (BitsGet(&bv, 0) == true);

    // Clean up
    BitsDeinit(&bv);

    return result;
}

// Test BitsResize function
bool test_Bits_resize(void) {
    printf("Testing BitsResize\n");

    Bits bv = BitsInit();

    // Add some initial data
    BitsPush(&bv, true);
    BitsPush(&bv, false);
    BitsPush(&bv, true);

    // Test resizing to larger size
    BitsResize(&bv, 6);

    // Check that length was increased and new bits have the default value
    bool result = (bv.length == 6);
    result      = result && (BitsGet(&bv, 0) == true);  // Original data
    result      = result && (BitsGet(&bv, 1) == false); // Original data
    result      = result && (BitsGet(&bv, 2) == true);  // Original data
    result      = result && (BitsGet(&bv, 3) == false); // New data (likely default to false)
    result      = result && (BitsGet(&bv, 4) == false); // New data (likely default to false)
    result      = result && (BitsGet(&bv, 5) == false); // New data (likely default to false)

    // Test resizing to smaller size
    BitsResize(&bv, 2);

    // Check that length was decreased and data was truncated
    result = result && (bv.length == 2);
    result = result && (BitsGet(&bv, 0) == true);  // Original data preserved
    result = result && (BitsGet(&bv, 1) == false); // Original data preserved

    // Test resizing to same size (should be no-op)
    BitsResize(&bv, 2);
    result = result && (bv.length == 2);

    // Clean up
    BitsDeinit(&bv);

    return result;
}

// Edge case tests - boundary conditions and unusual but valid inputs
bool test_Bits_init_edge_cases(void) {
    printf("Testing BitsInit edge cases\n");

    // Test multiple initializations
    Bits bv1 = BitsInit();
    Bits bv2 = BitsInit();
    Bits bv3 = BitsInit();

    bool result = (bv1.length == 0) && (bv2.length == 0) && (bv3.length == 0);
    result      = result && (bv1.data == NULL) && (bv2.data == NULL) && (bv3.data == NULL);

    // Clean up all
    BitsDeinit(&bv1);
    BitsDeinit(&bv2);
    BitsDeinit(&bv3);

    return result;
}

bool test_Bits_reserve_edge_cases(void) {
    printf("Testing BitsReserve edge cases\n");

    Bits bv     = BitsInit();
    bool   result = true;

    // Test reserving 0 (should be safe no-op)
    BitsReserve(&bv, 0);
    result = result && (bv.capacity == 0) && (bv.data == NULL);

    // Test reserving 1 bit (minimum meaningful size)
    BitsReserve(&bv, 1);
    result = result && (bv.capacity >= 1) && (bv.data != NULL);

    // Test very large but reasonable reservation
    BitsReserve(&bv, 10000);
    result = result && (bv.capacity >= 10000);

    // Test reserving same u64 repeatedly (should be no-op)
    u64 cap_before = bv.capacity;
    BitsReserve(&bv, bv.capacity);
    BitsReserve(&bv, bv.capacity);
    result = result && (bv.capacity == cap_before);

    BitsDeinit(&bv);
    return result;
}

bool test_Bits_resize_edge_cases(void) {
    printf("Testing BitsResize edge cases\n");

    Bits bv     = BitsInit();
    bool   result = true;

    // Test resize to 0 (should clear but keep memory)
    BitsPush(&bv, true);
    BitsPush(&bv, false);
    BitsResize(&bv, 0);
    result = result && (bv.length == 0);

    // Test Resize from 0 to non-zero
    BitsResize(&bv, 5);
    result = result && (bv.length == 5);
    // New bits should be false
    for (u64 i = 0; i < 5; i++) {
        result = result && (BitsGet(&bv, i) == false);
    }

    // Test Resize to same size
    BitsResize(&bv, 5);
    result = result && (bv.length == 5);

    // Test large resize
    BitsResize(&bv, 1000);
    result = result && (bv.length == 1000);

    // Test shrinking from large size
    BitsResize(&bv, 10);
    result = result && (bv.length == 10);

    BitsDeinit(&bv);
    return result;
}

bool test_Bits_clear_edge_cases(void) {
    printf("Testing BitsClear edge cases\n");

    Bits bv     = BitsInit();
    bool   result = true;

    // Test clear on empty Bits
    BitsClear(&bv);
    result = result && (bv.length == 0);

    // Test clear after single bit
    BitsPush(&bv, true);
    BitsClear(&bv);
    result = result && (bv.length == 0);

    // Test multiple clears
    BitsClear(&bv);
    BitsClear(&bv);
    result = result && (bv.length == 0);

    // Test clear after large data
    for (int i = 0; i < 1000; i++) {
        BitsPush(&bv, i % 2);
    }
    BitsClear(&bv);
    result = result && (bv.length == 0);

    BitsDeinit(&bv);
    return result;
}

bool test_Bits_multiple_cycles(void) {
    printf("Testing Bits multiple init/deinit cycles\n");

    bool result = true;

    // Test multiple init/deinit cycles
    for (int cycle = 0; cycle < 100; cycle++) {
        Bits bv = BitsInit();

        // Add some data
        for (int i = 0; i < cycle % 10; i++) {
            BitsPush(&bv, i % 2);
        }

        result = result && (bv.length == (size)(cycle % 10));
        BitsDeinit(&bv);
    }

    return result;
}

// Deadend tests - verify expected failures occur gracefully
bool test_Bits_null_pointer_failures(void) {
    printf("Testing Bits NULL pointer handling\n");

    // Test NULL pointer passed to functions that should validate
    // These should trigger aborts if validation is working

    // Test NULL Bits pointer - should abort
    BitsDeinit(NULL);

    // If we reach here, validation didn't work as expected
    return false;
}

bool test_Bits_invalid_operations(void) {
    printf("Testing Bits invalid operations\n");

    // Test operation that should trigger validation failure
    // Try to reserve an impossibly large amount that should fail
    Bits bv = BitsInit();

    // This should trigger an abort if validation is working
    BitsReserve(&bv, SIZE_MAX);

    // If we reach here, validation didn't work as expected
    BitsDeinit(&bv);
    return false;
}

bool test_Bits_set_operations_failures(void) {
    printf("Testing Bits set operations on invalid indices\n");

    // Test operation that should trigger validation failure
    // Try to Resize to impossibly large size
    Bits bv = BitsInit();

    // This should trigger an abort if validation is working
    BitsResize(&bv, SIZE_MAX);

    // If we reach here, validation didn't work as expected
    BitsDeinit(&bv);
    return false;
}

// Main function that runs all tests
int main(void) {
    printf("[INFO] Starting Bits.Init tests\n\n");

    // Array of normal test functions
    TestFunction tests[] = {
        test_Bits_init,
        test_Bits_deinit,
        test_Bits_reserve,
        test_Bits_clear,
        test_Bits_resize,
        test_Bits_init_edge_cases,
        test_Bits_reserve_edge_cases,
        test_Bits_resize_edge_cases,
        test_Bits_clear_edge_cases,
        test_Bits_multiple_cycles
    };

    // Array of deadend test functions (expected failure scenarios)
    TestFunction deadend_tests[] = {
        test_Bits_null_pointer_failures,
        test_Bits_invalid_operations,
        test_Bits_set_operations_failures
    };

    int total_tests         = sizeof(tests) / sizeof(tests[0]);
    int total_deadend_tests = sizeof(deadend_tests) / sizeof(deadend_tests[0]);

    // Run all tests using the centralized test driver
    return run_test_suite(tests, total_tests, deadend_tests, total_deadend_tests, "Bits.Init");
}

