#include <Misra/Std/Container/Bits.h>
#include <Misra/Std/Log.h>
#include <stdio.h>
#include <Misra/Types.h>

// Include test utilities
#include "../Util/TestRunner.h"

// Function prototypes
bool test_Bits_get(void);
bool test_Bits_set(void);
bool test_Bits_flip(void);
bool test_Bits_length_capacity(void);
bool test_Bits_count_operations(void);
bool test_Bits_get_edge_cases(void);
bool test_Bits_set_edge_cases(void);
bool test_Bits_flip_edge_cases(void);
bool test_Bits_count_edge_cases(void);
bool test_Bits_access_multiple_operations(void);
bool test_Bits_access_large_patterns(void);
bool test_Bits_macro_functions(void);
bool test_Bits_access_stress_test(void);
bool test_Bits_bit_patterns_comprehensive(void);
bool test_Bits_find_functions(void);
bool test_Bits_predicate_functions(void);
bool test_Bits_longest_run(void);
bool test_Bits_find_edge_cases(void);
bool test_Bits_predicate_edge_cases(void);
bool test_Bits_longest_run_edge_cases(void);
bool test_Bits_find_deadend_tests(void);
bool test_Bits_predicate_deadend_tests(void);
bool test_Bits_longest_run_deadend_tests(void);

// Test BitsGet function
bool test_Bits_get(void) {
    printf("Testing BitsGet\n");

    Bits bv = BitsInit();

    // Add a pattern: 1010
    BitsPush(&bv, true);
    BitsPush(&bv, false);
    BitsPush(&bv, true);
    BitsPush(&bv, false);

    // Test getting each bit
    bool result = (BitsGet(&bv, 0) == true);
    result      = result && (BitsGet(&bv, 1) == false);
    result      = result && (BitsGet(&bv, 2) == true);
    result      = result && (BitsGet(&bv, 3) == false);

    // Clean up
    BitsDeinit(&bv);

    return result;
}

// Test BitsSet function
bool test_Bits_set(void) {
    printf("Testing BitsSet\n");

    Bits bv = BitsInit();

    // Initialize with some bits
    BitsPush(&bv, false);
    BitsPush(&bv, false);
    BitsPush(&bv, false);

    // Set specific bits
    BitsSet(&bv, 0, true);
    BitsSet(&bv, 2, true);

    // Verify changes
    bool result = (BitsGet(&bv, 0) == true);
    result      = result && (BitsGet(&bv, 1) == false);
    result      = result && (BitsGet(&bv, 2) == true);

    // Test setting back to false
    BitsSet(&bv, 0, false);
    result = result && (BitsGet(&bv, 0) == false);

    // Clean up
    BitsDeinit(&bv);

    return result;
}

// Test BitsFlip function
bool test_Bits_flip(void) {
    printf("Testing BitsFlip\n");

    Bits bv = BitsInit();

    // Initialize with pattern: 101
    BitsPush(&bv, true);
    BitsPush(&bv, false);
    BitsPush(&bv, true);

    // Flip each bit
    BitsFlip(&bv, 0); // 1 -> 0
    BitsFlip(&bv, 1); // 0 -> 1
    BitsFlip(&bv, 2); // 1 -> 0

    // Verify flipped values: should now be 010
    bool result = (BitsGet(&bv, 0) == false);
    result      = result && (BitsGet(&bv, 1) == true);
    result      = result && (BitsGet(&bv, 2) == false);

    // Flip back
    BitsFlip(&bv, 0); // 0 -> 1
    BitsFlip(&bv, 1); // 1 -> 0
    BitsFlip(&bv, 2); // 0 -> 1

    // Should be back to original: 101
    result = result && (BitsGet(&bv, 0) == true);
    result = result && (BitsGet(&bv, 1) == false);
    result = result && (BitsGet(&bv, 2) == true);

    // Clean up
    BitsDeinit(&bv);

    return result;
}

// Test length and capacity operations
bool test_Bits_length_capacity(void) {
    printf("Testing Bits length and capacity operations\n");

    Bits bv = BitsInit();

    // Initially should be empty
    bool result = (BitsLen(&bv) == 0) && (BitsCapacity(&bv) == 0);

    // Add some bits
    BitsPush(&bv, true);
    BitsPush(&bv, false);
    BitsPush(&bv, true);

    result = result && (BitsLen(&bv) == 3);
    result = result && (BitsCapacity(&bv) >= 3);

    // Test empty check
    result = result && !BitsEmpty(&bv);

    // Clear and check empty
    BitsClear(&bv);
    result = result && BitsEmpty(&bv);
    result = result && (BitsLen(&bv) == 0);

    // Clean up
    BitsDeinit(&bv);

    return result;
}

// Test count operations
bool test_Bits_count_operations(void) {
    printf("Testing Bits count operations\n");

    Bits bv = BitsInit();

    // Create pattern: 1101000
    BitsPush(&bv, true);  // 1 one,  0 zeros
    BitsPush(&bv, true);  // 2 ones, 0 zeros
    BitsPush(&bv, false); // 2 ones, 1 zero
    BitsPush(&bv, true);  // 3 ones, 1 zero
    BitsPush(&bv, false); // 3 ones, 2 zeros
    BitsPush(&bv, false); // 3 ones, 3 zeros
    BitsPush(&bv, false); // 3 ones, 4 zeros

    // Test counting
    bool result = (BitsCountOnes(&bv) == 3);
    result      = result && (BitsCountZeros(&bv) == 4);

    // Test with all ones
    BitsClear(&bv);
    BitsPush(&bv, true);
    BitsPush(&bv, true);
    BitsPush(&bv, true);

    result = result && (BitsCountOnes(&bv) == 3);
    result = result && (BitsCountZeros(&bv) == 0);

    // Clean up
    BitsDeinit(&bv);

    return result;
}

// Edge case tests
bool test_Bits_get_edge_cases(void) {
    printf("Testing BitsGet edge cases\n");

    Bits bv     = BitsInit();
    bool result = true;

    // Test boundary conditions (no longer test empty Bits - strict bounds checking now)
    BitsPush(&bv, true);
    result = result && (BitsGet(&bv, 0) == true); // Valid index

    // Test with large data set
    BitsClear(&bv);
    for (int i = 0; i < 1000; i++) {
        BitsPush(&bv, i % 3 == 0);
    }

    result = result && (BitsGet(&bv, 0) == true);             // First
    result = result && (BitsGet(&bv, 999) == (999 % 3 == 0)); // Last

    BitsDeinit(&bv);
    return result;
}

bool test_Bits_set_edge_cases(void) {
    printf("Testing BitsSet edge cases\n");

    Bits bv     = BitsInit();
    bool result = true;

    // Test normal setting
    BitsPush(&bv, false);
    BitsSet(&bv, 0, true);
    result = result && (BitsGet(&bv, 0) == true);

    // Test setting same value multiple times
    BitsSet(&bv, 0, true);
    BitsSet(&bv, 0, true);
    result = result && (BitsGet(&bv, 0) == true);

    BitsDeinit(&bv);
    return result;
}

bool test_Bits_flip_edge_cases(void) {
    printf("Testing BitsFlip edge cases\n");

    Bits bv     = BitsInit();
    bool result = true;

    // Test normal flipping
    BitsPush(&bv, true);
    BitsFlip(&bv, 0);
    result = result && (BitsGet(&bv, 0) == false);

    BitsFlip(&bv, 0);
    result = result && (BitsGet(&bv, 0) == true);

    BitsDeinit(&bv);
    return result;
}

bool test_Bits_count_edge_cases(void) {
    printf("Testing Bits count edge cases\n");

    Bits bv     = BitsInit();
    bool result = true;

    // Test count on empty Bits
    result = result && (BitsCountOnes(&bv) == 0);
    result = result && (BitsCountZeros(&bv) == 0);

    // Test count with single bit
    BitsPush(&bv, true);
    result = result && (BitsCountOnes(&bv) == 1);
    result = result && (BitsCountZeros(&bv) == 0);

    BitsClear(&bv);
    BitsPush(&bv, false);
    result = result && (BitsCountOnes(&bv) == 0);
    result = result && (BitsCountZeros(&bv) == 1);

    // Test count with large uniform data
    BitsClear(&bv);
    for (int i = 0; i < 1000; i++) {
        BitsPush(&bv, true);
    }
    result = result && (BitsCountOnes(&bv) == 1000);
    result = result && (BitsCountZeros(&bv) == 0);

    BitsDeinit(&bv);
    return result;
}

bool test_Bits_access_multiple_operations(void) {
    printf("Testing Bits multiple access operations\n");

    Bits bv     = BitsInit();
    bool result = true;

    // Stress test with many operations
    for (int cycle = 0; cycle < 100; cycle++) {
        BitsPush(&bv, cycle % 2 == 0);

        // Test access during growth
        if (cycle > 0) {
            result = result && (BitsGet(&bv, 0) == true);
            result = result && (BitsLen(&bv) == (size)(cycle + 1));
        }

        // Test setting and getting
        BitsSet(&bv, cycle, cycle % 3 == 0);
        result = result && (BitsGet(&bv, cycle) == (cycle % 3 == 0));
    }

    BitsDeinit(&bv);
    return result;
}

// NEW: Test large bit patterns and complex access patterns
bool test_Bits_access_large_patterns(void) {
    printf("Testing Bits large pattern access\n");

    Bits bv     = BitsInit();
    bool result = true;

    // Create a large repeating pattern: 10110100 (8-bit pattern)
    u8 pattern = 0b10110100; // 10110100
    for (int repeat = 0; repeat < 500; repeat++) {
        for (int bit = 0; bit < 8; bit++) {
            bool bit_value = (pattern & (1u << bit)) != 0;
            BitsPush(&bv, bit_value);
        }
    }

    // Verify pattern integrity at various points
    for (int check = 0; check < 50; check++) {
        u64 base_idx = check * 80; // Check every 80 bits
        if (base_idx + 7 < bv.length) {
            for (int bit = 0; bit < 8; bit++) {
                bool expected = (pattern & (1u << bit)) != 0;
                bool actual   = BitsGet(&bv, base_idx + bit);
                result        = result && (actual == expected);
            }
        }
    }

    // Test random access across the large dataset
    result = result && (BitsGet(&bv, 0) == false);            // First bit of pattern
    result = result && (BitsGet(&bv, 2) == true);             // Third bit of pattern
    result = result && (BitsGet(&bv, bv.length - 1) == true); // Last bit

    BitsDeinit(&bv);
    return result;
}

// NEW: Test macro functions comprehensively
bool test_Bits_macro_functions(void) {
    printf("Testing Bits macro functions\n");

    Bits bv     = BitsInit();
    bool result = true;

    // Test all macros on empty Bitstor
    result = result && (BitsLen(&bv) == 0);
    result = result && (BitsCapacity(&bv) == 0);
    result = result && BitsEmpty(&bv);
    result = result && (BitsByteSize(&bv) == 0);

    // Add bits and test macros again
    for (int i = 0; i < 65; i++) { // Test across byte boundaries
        BitsPush(&bv, i % 2 == 0);
    }

    result = result && (BitsLen(&bv) == 65);
    result = result && (BitsCapacity(&bv) >= 65);
    result = result && !BitsEmpty(&bv);
    result = result && (BitsByteSize(&bv) >= 9); // At least 9 bytes for 65 bits

    // Test with exactly power-of-2 sizes
    BitsClear(&bv);
    for (int i = 0; i < 64; i++) { // Exactly 64 bits
        BitsPush(&bv, true);
    }

    result = result && (BitsLen(&bv) == 64);
    result = result && (BitsByteSize(&bv) >= 8); // At least 8 bytes for 64 bits

    BitsDeinit(&bv);
    return result;
}

// NEW: Stress test for access operations
bool test_Bits_access_stress_test(void) {
    printf("Testing Bits access stress test\n");

    Bits bv     = BitsInit();
    bool result = true;

    // Create a large Bitstor with known pattern
    const u64 size = 10000;
    for (u64 i = 0; i < size; i++) {
        // Pattern: alternating every 7 bits
        BitsPush(&bv, (i / 7) % 2 == 0);
    }

    // Stress test random access patterns
    for (int test = 0; test < 1000; test++) {
        u64  idx      = (test * 7 + test * 3) % size; // Pseudo-random indices
        bool expected = (idx / 7) % 2 == 0;
        bool actual   = BitsGet(&bv, idx);
        result        = result && (actual == expected);
    }

    // Stress test setting and getting at boundaries
    u64 boundaries[]   = {0, 1, 7, 8, 63, 64, 127, 128, 255, 256, 511, 512, 1023, 1024, size - 1};
    int boundary_count = sizeof(boundaries) / sizeof(boundaries[0]);

    for (int i = 0; i < boundary_count; i++) {
        if (boundaries[i] < size) {
            bool original = BitsGet(&bv, boundaries[i]);
            BitsFlip(&bv, boundaries[i]);
            result = result && (BitsGet(&bv, boundaries[i]) == !original);
            BitsFlip(&bv, boundaries[i]); // Flip back
            result = result && (BitsGet(&bv, boundaries[i]) == original);
        }
    }

    BitsDeinit(&bv);
    return result;
}

// NEW: Comprehensive bit pattern testing
bool test_Bits_bit_patterns_comprehensive(void) {
    printf("Testing Bits comprehensive bit patterns\n");

    Bits bv     = BitsInit();
    bool result = true;

    // Test all-zeros pattern
    for (int i = 0; i < 100; i++) {
        BitsPush(&bv, false);
    }

    result = result && (BitsCountOnes(&bv) == 0);
    result = result && (BitsCountZeros(&bv) == 100);

    // Verify all bits are false
    for (int i = 0; i < 100; i++) {
        result = result && (BitsGet(&bv, i) == false);
    }

    // Change to all-ones pattern
    for (int i = 0; i < 100; i++) {
        BitsSet(&bv, i, true);
    }

    result = result && (BitsCountOnes(&bv) == 100);
    result = result && (BitsCountZeros(&bv) == 0);

    // Test checkerboard pattern
    BitsClear(&bv);
    for (int i = 0; i < 100; i++) {
        BitsPush(&bv, i % 2 == 0);
    }

    result = result && (BitsCountOnes(&bv) == 50);
    result = result && (BitsCountZeros(&bv) == 50);

    // Verify checkerboard pattern
    for (int i = 0; i < 100; i++) {
        bool expected = (i % 2 == 0);
        result        = result && (BitsGet(&bv, i) == expected);
    }

    // Test Fibonacci-like pattern (each bit is XOR of previous two)
    BitsClear(&bv);
    BitsPush(&bv, true); // F(0) = 1
    BitsPush(&bv, true); // F(1) = 1
    for (int i = 2; i < 50; i++) {
        bool prev1 = BitsGet(&bv, i - 1);
        bool prev2 = BitsGet(&bv, i - 2);
        BitsPush(&bv, prev1 != prev2); // XOR
    }

    // Verify first few Fibonacci bits
    result = result && (BitsGet(&bv, 0) == true);  // F(0) = 1
    result = result && (BitsGet(&bv, 1) == true);  // F(1) = 1
    result = result && (BitsGet(&bv, 2) == false); // F(2) = 1 XOR 1 = 0
    result = result && (BitsGet(&bv, 3) == true);  // F(3) = 1 XOR 0 = 1
    result = result && (BitsGet(&bv, 4) == true);  // F(4) = 0 XOR 1 = 1

    BitsDeinit(&bv);
    return result;
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

// Test BitsFind and BitsFindLast functions
bool test_Bits_find_functions(void) {
    printf("Testing BitsFind and BitsFindLast functions\n");

    Bits bv     = BitsInit();
    bool result = true;

    // Create pattern: 10110100
    BitsPush(&bv, true);  // 0
    BitsPush(&bv, false); // 1
    BitsPush(&bv, true);  // 2
    BitsPush(&bv, true);  // 3
    BitsPush(&bv, false); // 4
    BitsPush(&bv, true);  // 5
    BitsPush(&bv, false); // 6
    BitsPush(&bv, false); // 7

    // Test finding first true (should be at index 0)
    result = result && (BitsFind(&bv, true) == 0);

    // Test finding first false (should be at index 1)
    result = result && (BitsFind(&bv, false) == 1);

    // Test finding last true (should be at index 5)
    result = result && (BitsFindLast(&bv, true) == 5);

    // Test finding last false (should be at index 7)
    result = result && (BitsFindLast(&bv, false) == 7);

    // Test with all same values
    BitsClear(&bv);
    BitsPush(&bv, true);
    BitsPush(&bv, true);
    BitsPush(&bv, true);

    result = result && (BitsFind(&bv, true) == 0);
    result = result && (BitsFindLast(&bv, true) == 2);
    result = result && (BitsFind(&bv, false) == SIZE_MAX);
    result = result && (BitsFindLast(&bv, false) == SIZE_MAX);

    BitsDeinit(&bv);
    return result;
}

// Test BitsAll, BitsAny, BitsNone functions
bool test_Bits_predicate_functions(void) {
    printf("Testing BitsAll, BitsAny, BitsNone functions\n");

    Bits bv     = BitsInit();
    bool result = true;

    // Test with all true bits
    BitsPush(&bv, true);
    BitsPush(&bv, true);
    BitsPush(&bv, true);

    result = result && BitsAll(&bv, true);
    result = result && !BitsAll(&bv, false);
    result = result && BitsAny(&bv, true);
    result = result && !BitsAny(&bv, false);
    result = result && !BitsNone(&bv, true);
    result = result && BitsNone(&bv, false);

    // Test with all false bits
    BitsClear(&bv);
    BitsPush(&bv, false);
    BitsPush(&bv, false);
    BitsPush(&bv, false);

    result = result && !BitsAll(&bv, true);
    result = result && BitsAll(&bv, false);
    result = result && !BitsAny(&bv, true);
    result = result && BitsAny(&bv, false);
    result = result && BitsNone(&bv, true);
    result = result && !BitsNone(&bv, false);

    // Test with mixed bits
    BitsClear(&bv);
    BitsPush(&bv, true);
    BitsPush(&bv, false);
    BitsPush(&bv, true);

    result = result && !BitsAll(&bv, true);
    result = result && !BitsAll(&bv, false);
    result = result && BitsAny(&bv, true);
    result = result && BitsAny(&bv, false);
    result = result && !BitsNone(&bv, true);
    result = result && !BitsNone(&bv, false);

    BitsDeinit(&bv);
    return result;
}

// Test BitsLongestRun function
bool test_Bits_longest_run(void) {
    printf("Testing BitsLongestRun function\n");

    Bits bv     = BitsInit();
    bool result = true;

    // Test pattern: 11100110011111
    BitsPush(&bv, true);  // Run of 3 trues
    BitsPush(&bv, true);
    BitsPush(&bv, true);
    BitsPush(&bv, false); // 1 false
    BitsPush(&bv, false); // Run of 2 falses
    BitsPush(&bv, true);  // 1 true
    BitsPush(&bv, true);  // Run of 2 trues
    BitsPush(&bv, false); // 1 false
    BitsPush(&bv, false); // Run of 2 falses
    BitsPush(&bv, true);  // Run of 5 trues
    BitsPush(&bv, true);
    BitsPush(&bv, true);
    BitsPush(&bv, true);
    BitsPush(&bv, true);

    // Longest run of trues should be 5
    result = result && (BitsLongestRun(&bv, true) == 5);

    // Longest run of falses should be 2
    result = result && (BitsLongestRun(&bv, false) == 2);

    // Test with all same values
    BitsClear(&bv);
    for (int i = 0; i < 10; i++) {
        BitsPush(&bv, true);
    }
    result = result && (BitsLongestRun(&bv, true) == 10);
    result = result && (BitsLongestRun(&bv, false) == 0);

    // Test alternating pattern
    BitsClear(&bv);
    for (int i = 0; i < 10; i++) {
        BitsPush(&bv, i % 2 == 0);
    }
    result = result && (BitsLongestRun(&bv, true) == 1);
    result = result && (BitsLongestRun(&bv, false) == 1);

    BitsDeinit(&bv);
    return result;
}

// Edge case tests for Find functions
bool test_Bits_find_edge_cases(void) {
    printf("Testing BitsFind edge cases\n");

    Bits bv     = BitsInit();
    bool result = true;

    // Test empty Bitstor
    result = result && (BitsFind(&bv, true) == SIZE_MAX);
    result = result && (BitsFind(&bv, false) == SIZE_MAX);
    result = result && (BitsFindLast(&bv, true) == SIZE_MAX);
    result = result && (BitsFindLast(&bv, false) == SIZE_MAX);

    // Test single element
    BitsPush(&bv, true);
    result = result && (BitsFind(&bv, true) == 0);
    result = result && (BitsFindLast(&bv, true) == 0);
    result = result && (BitsFind(&bv, false) == SIZE_MAX);
    result = result && (BitsFindLast(&bv, false) == SIZE_MAX);

    // Test with large Bitstor
    BitsClear(&bv);
    for (int i = 0; i < 1000; i++) {
        BitsPush(&bv, i == 500 || i == 999); // Only indices 500 and 999 are true
    }
    result = result && (BitsFind(&bv, true) == 500);
    result = result && (BitsFindLast(&bv, true) == 999);

    BitsDeinit(&bv);
    return result;
}

// Edge case tests for predicate functions
bool test_Bits_predicate_edge_cases(void) {
    printf("Testing Bits predicate edge cases\n");

    Bits bv     = BitsInit();
    bool result = true;

    // Test empty Bitstor - all predicates should return true for empty set
    result = result && BitsAll(&bv, true);
    result = result && BitsAll(&bv, false);
    result = result && !BitsAny(&bv, true);
    result = result && !BitsAny(&bv, false);
    result = result && BitsNone(&bv, true);
    result = result && BitsNone(&bv, false);

    // Test single element Bitstor
    BitsPush(&bv, true);
    result = result && BitsAll(&bv, true);
    result = result && !BitsAll(&bv, false);
    result = result && BitsAny(&bv, true);
    result = result && !BitsAny(&bv, false);

    // Test large Bitstor with specific patterns
    BitsClear(&bv);
    for (int i = 0; i < 1000; i++) {
        BitsPush(&bv, true); // All true
    }
    result = result && BitsAll(&bv, true);
    result = result && !BitsAll(&bv, false);

    // Change one bit to false
    BitsSet(&bv, 500, false);
    result = result && !BitsAll(&bv, true);
    result = result && BitsAny(&bv, true);
    result = result && BitsAny(&bv, false);

    BitsDeinit(&bv);
    return result;
}

// Edge case tests for LongestRun function
bool test_Bits_longest_run_edge_cases(void) {
    printf("Testing BitsLongestRun edge cases\n");

    Bits bv     = BitsInit();
    bool result = true;

    // Test empty Bitstor
    result = result && (BitsLongestRun(&bv, true) == 0);
    result = result && (BitsLongestRun(&bv, false) == 0);

    // Test single element
    BitsPush(&bv, true);
    result = result && (BitsLongestRun(&bv, true) == 1);
    result = result && (BitsLongestRun(&bv, false) == 0);

    // Test large runs
    BitsClear(&bv);
    for (int i = 0; i < 10000; i++) {
        BitsPush(&bv, true);
    }
    result = result && (BitsLongestRun(&bv, true) == 10000);
    result = result && (BitsLongestRun(&bv, false) == 0);

    // Test with one interruption in the middle
    BitsSet(&bv, 5000, false);
    result = result && (BitsLongestRun(&bv, true) == 5000);
    result = result && (BitsLongestRun(&bv, false) == 1);

    BitsDeinit(&bv);
    return result;
}

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

// Main function that runs all tests
int main(void) {
    printf("[INFO] Starting Bits.Access tests\n\n");

    // Array of test functions (adding new tests to existing ones)
    TestFunction tests[] = {
        test_Bits_get,
        test_Bits_set,
        test_Bits_flip,
        test_Bits_length_capacity,
        test_Bits_count_operations,
        test_Bits_get_edge_cases,
        test_Bits_set_edge_cases,
        test_Bits_flip_edge_cases,
        test_Bits_count_edge_cases,
        test_Bits_access_multiple_operations,
        test_Bits_access_large_patterns,
        test_Bits_macro_functions,
        test_Bits_access_stress_test,
        test_Bits_bit_patterns_comprehensive,
        // New comprehensive tests for missing functions
        test_Bits_find_functions,
        test_Bits_predicate_functions,
        test_Bits_longest_run,
        test_Bits_find_edge_cases,
        test_Bits_predicate_edge_cases,
        test_Bits_longest_run_edge_cases
    };

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

    int total_tests         = sizeof(tests) / sizeof(tests[0]);
    int total_deadend_tests = sizeof(deadend_tests) / sizeof(deadend_tests[0]);

    // Run all tests using the centralized test driver
    return run_test_suite(tests, total_tests, deadend_tests, total_deadend_tests, "Bits.Access");
}
