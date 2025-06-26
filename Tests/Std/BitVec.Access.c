#include <Misra/Std/Container/BitVec.h>
#include <Misra/Std/Log.h>
#include <stdio.h>
#include <Misra/Types.h>

// Include test utilities
#include "../Util/TestRunner.h"

// Function prototypes
bool test_bitvec_get(void);
bool test_bitvec_set(void);
bool test_bitvec_flip(void);
bool test_bitvec_length_capacity(void);
bool test_bitvec_count_operations(void);
bool test_bitvec_get_edge_cases(void);
bool test_bitvec_set_edge_cases(void);
bool test_bitvec_flip_edge_cases(void);
bool test_bitvec_count_edge_cases(void);
bool test_bitvec_access_multiple_operations(void);
bool test_bitvec_access_large_patterns(void);
bool test_bitvec_macro_functions(void);
bool test_bitvec_access_stress_test(void);
bool test_bitvec_bit_patterns_comprehensive(void);
bool test_bitvec_find_functions(void);
bool test_bitvec_predicate_functions(void);
bool test_bitvec_longest_run(void);
bool test_bitvec_find_edge_cases(void);
bool test_bitvec_predicate_edge_cases(void);
bool test_bitvec_longest_run_edge_cases(void);
bool test_bitvec_find_deadend_tests(void);
bool test_bitvec_predicate_deadend_tests(void);
bool test_bitvec_longest_run_deadend_tests(void);

// Test BitVecGet function
bool test_bitvec_get(void) {
    printf("Testing BitVecGet\n");

    BitVec bv = BitVecInit();

    // Add a pattern: 1010
    BitVecPush(&bv, true);
    BitVecPush(&bv, false);
    BitVecPush(&bv, true);
    BitVecPush(&bv, false);

    // Test getting each bit
    bool result = (BitVecGet(&bv, 0) == true);
    result      = result && (BitVecGet(&bv, 1) == false);
    result      = result && (BitVecGet(&bv, 2) == true);
    result      = result && (BitVecGet(&bv, 3) == false);

    // Clean up
    BitVecDeinit(&bv);

    return result;
}

// Test BitVecSet function
bool test_bitvec_set(void) {
    printf("Testing BitVecSet\n");

    BitVec bv = BitVecInit();

    // Initialize with some bits
    BitVecPush(&bv, false);
    BitVecPush(&bv, false);
    BitVecPush(&bv, false);

    // Set specific bits
    BitVecSet(&bv, 0, true);
    BitVecSet(&bv, 2, true);

    // Verify changes
    bool result = (BitVecGet(&bv, 0) == true);
    result      = result && (BitVecGet(&bv, 1) == false);
    result      = result && (BitVecGet(&bv, 2) == true);

    // Test setting back to false
    BitVecSet(&bv, 0, false);
    result = result && (BitVecGet(&bv, 0) == false);

    // Clean up
    BitVecDeinit(&bv);

    return result;
}

// Test BitVecFlip function
bool test_bitvec_flip(void) {
    printf("Testing BitVecFlip\n");

    BitVec bv = BitVecInit();

    // Initialize with pattern: 101
    BitVecPush(&bv, true);
    BitVecPush(&bv, false);
    BitVecPush(&bv, true);

    // Flip each bit
    BitVecFlip(&bv, 0); // 1 -> 0
    BitVecFlip(&bv, 1); // 0 -> 1
    BitVecFlip(&bv, 2); // 1 -> 0

    // Verify flipped values: should now be 010
    bool result = (BitVecGet(&bv, 0) == false);
    result      = result && (BitVecGet(&bv, 1) == true);
    result      = result && (BitVecGet(&bv, 2) == false);

    // Flip back
    BitVecFlip(&bv, 0); // 0 -> 1
    BitVecFlip(&bv, 1); // 1 -> 0
    BitVecFlip(&bv, 2); // 0 -> 1

    // Should be back to original: 101
    result = result && (BitVecGet(&bv, 0) == true);
    result = result && (BitVecGet(&bv, 1) == false);
    result = result && (BitVecGet(&bv, 2) == true);

    // Clean up
    BitVecDeinit(&bv);

    return result;
}

// Test length and capacity operations
bool test_bitvec_length_capacity(void) {
    printf("Testing BitVec length and capacity operations\n");

    BitVec bv = BitVecInit();

    // Initially should be empty
    bool result = (BitVecLen(&bv) == 0) && (BitVecCapacity(&bv) == 0);

    // Add some bits
    BitVecPush(&bv, true);
    BitVecPush(&bv, false);
    BitVecPush(&bv, true);

    result = result && (BitVecLen(&bv) == 3);
    result = result && (BitVecCapacity(&bv) >= 3);

    // Test empty check
    result = result && !BitVecEmpty(&bv);

    // Clear and check empty
    BitVecClear(&bv);
    result = result && BitVecEmpty(&bv);
    result = result && (BitVecLen(&bv) == 0);

    // Clean up
    BitVecDeinit(&bv);

    return result;
}

// Test count operations
bool test_bitvec_count_operations(void) {
    printf("Testing BitVec count operations\n");

    BitVec bv = BitVecInit();

    // Create pattern: 1101000
    BitVecPush(&bv, true);  // 1 one,  0 zeros
    BitVecPush(&bv, true);  // 2 ones, 0 zeros
    BitVecPush(&bv, false); // 2 ones, 1 zero
    BitVecPush(&bv, true);  // 3 ones, 1 zero
    BitVecPush(&bv, false); // 3 ones, 2 zeros
    BitVecPush(&bv, false); // 3 ones, 3 zeros
    BitVecPush(&bv, false); // 3 ones, 4 zeros

    // Test counting
    bool result = (BitVecCountOnes(&bv) == 3);
    result      = result && (BitVecCountZeros(&bv) == 4);

    // Test with all ones
    BitVecClear(&bv);
    BitVecPush(&bv, true);
    BitVecPush(&bv, true);
    BitVecPush(&bv, true);

    result = result && (BitVecCountOnes(&bv) == 3);
    result = result && (BitVecCountZeros(&bv) == 0);

    // Clean up
    BitVecDeinit(&bv);

    return result;
}

// Edge case tests
bool test_bitvec_get_edge_cases(void) {
    printf("Testing BitVecGet edge cases\n");

    BitVec bv     = BitVecInit();
    bool   result = true;

    // Test boundary conditions (no longer test empty bitvec - strict bounds checking now)
    BitVecPush(&bv, true);
    result = result && (BitVecGet(&bv, 0) == true); // Valid index

    // Test with large data set
    BitVecClear(&bv);
    for (int i = 0; i < 1000; i++) {
        BitVecPush(&bv, i % 3 == 0);
    }

    result = result && (BitVecGet(&bv, 0) == true);             // First
    result = result && (BitVecGet(&bv, 999) == (999 % 3 == 0)); // Last

    BitVecDeinit(&bv);
    return result;
}

bool test_bitvec_set_edge_cases(void) {
    printf("Testing BitVecSet edge cases\n");

    BitVec bv     = BitVecInit();
    bool   result = true;

    // Test normal setting
    BitVecPush(&bv, false);
    BitVecSet(&bv, 0, true);
    result = result && (BitVecGet(&bv, 0) == true);

    // Test setting same value multiple times
    BitVecSet(&bv, 0, true);
    BitVecSet(&bv, 0, true);
    result = result && (BitVecGet(&bv, 0) == true);

    BitVecDeinit(&bv);
    return result;
}

bool test_bitvec_flip_edge_cases(void) {
    printf("Testing BitVecFlip edge cases\n");

    BitVec bv     = BitVecInit();
    bool   result = true;

    // Test normal flipping
    BitVecPush(&bv, true);
    BitVecFlip(&bv, 0);
    result = result && (BitVecGet(&bv, 0) == false);

    BitVecFlip(&bv, 0);
    result = result && (BitVecGet(&bv, 0) == true);

    BitVecDeinit(&bv);
    return result;
}

bool test_bitvec_count_edge_cases(void) {
    printf("Testing BitVec count edge cases\n");

    BitVec bv     = BitVecInit();
    bool   result = true;

    // Test count on empty bitvec
    result = result && (BitVecCountOnes(&bv) == 0);
    result = result && (BitVecCountZeros(&bv) == 0);

    // Test count with single bit
    BitVecPush(&bv, true);
    result = result && (BitVecCountOnes(&bv) == 1);
    result = result && (BitVecCountZeros(&bv) == 0);

    BitVecClear(&bv);
    BitVecPush(&bv, false);
    result = result && (BitVecCountOnes(&bv) == 0);
    result = result && (BitVecCountZeros(&bv) == 1);

    // Test count with large uniform data
    BitVecClear(&bv);
    for (int i = 0; i < 1000; i++) {
        BitVecPush(&bv, true);
    }
    result = result && (BitVecCountOnes(&bv) == 1000);
    result = result && (BitVecCountZeros(&bv) == 0);

    BitVecDeinit(&bv);
    return result;
}

bool test_bitvec_access_multiple_operations(void) {
    printf("Testing BitVec multiple access operations\n");

    BitVec bv     = BitVecInit();
    bool   result = true;

    // Stress test with many operations
    for (int cycle = 0; cycle < 100; cycle++) {
        BitVecPush(&bv, cycle % 2 == 0);

        // Test access during growth
        if (cycle > 0) {
            result = result && (BitVecGet(&bv, 0) == true);
            result = result && (BitVecLen(&bv) == (size)(cycle + 1));
        }

        // Test setting and getting
        BitVecSet(&bv, cycle, cycle % 3 == 0);
        result = result && (BitVecGet(&bv, cycle) == (cycle % 3 == 0));
    }

    BitVecDeinit(&bv);
    return result;
}

// NEW: Test large bit patterns and complex access patterns
bool test_bitvec_access_large_patterns(void) {
    printf("Testing BitVec large pattern access\n");

    BitVec bv     = BitVecInit();
    bool   result = true;

    // Create a large repeating pattern: 10110100 (8-bit pattern)
    u8 pattern = 0b10110100; // 10110100
    for (int repeat = 0; repeat < 500; repeat++) {
        for (int bit = 0; bit < 8; bit++) {
            bool bit_value = (pattern & (1u << bit)) != 0;
            BitVecPush(&bv, bit_value);
        }
    }

    // Verify pattern integrity at various points
    for (int check = 0; check < 50; check++) {
        u64 base_idx = check * 80; // Check every 80 bits
        if (base_idx + 7 < bv.length) {
            for (int bit = 0; bit < 8; bit++) {
                bool expected = (pattern & (1u << bit)) != 0;
                bool actual   = BitVecGet(&bv, base_idx + bit);
                result        = result && (actual == expected);
            }
        }
    }

    // Test random access across the large dataset
    result = result && (BitVecGet(&bv, 0) == false);            // First bit of pattern
    result = result && (BitVecGet(&bv, 2) == true);             // Third bit of pattern
    result = result && (BitVecGet(&bv, bv.length - 1) == true); // Last bit

    BitVecDeinit(&bv);
    return result;
}

// NEW: Test macro functions comprehensively
bool test_bitvec_macro_functions(void) {
    printf("Testing BitVec macro functions\n");

    BitVec bv     = BitVecInit();
    bool   result = true;

    // Test all macros on empty bitvector
    result = result && (BitVecLen(&bv) == 0);
    result = result && (BitVecCapacity(&bv) == 0);
    result = result && BitVecEmpty(&bv);
    result = result && (BitVecByteSize(&bv) == 0);

    // Add bits and test macros again
    for (int i = 0; i < 65; i++) { // Test across byte boundaries
        BitVecPush(&bv, i % 2 == 0);
    }

    result = result && (BitVecLen(&bv) == 65);
    result = result && (BitVecCapacity(&bv) >= 65);
    result = result && !BitVecEmpty(&bv);
    result = result && (BitVecByteSize(&bv) >= 9); // At least 9 bytes for 65 bits

    // Test with exactly power-of-2 sizes
    BitVecClear(&bv);
    for (int i = 0; i < 64; i++) { // Exactly 64 bits
        BitVecPush(&bv, true);
    }

    result = result && (BitVecLen(&bv) == 64);
    result = result && (BitVecByteSize(&bv) >= 8); // At least 8 bytes for 64 bits

    BitVecDeinit(&bv);
    return result;
}

// NEW: Stress test for access operations
bool test_bitvec_access_stress_test(void) {
    printf("Testing BitVec access stress test\n");

    BitVec bv     = BitVecInit();
    bool   result = true;

    // Create a large bitvector with known pattern
    const u64 size = 10000;
    for (u64 i = 0; i < size; i++) {
        // Pattern: alternating every 7 bits
        BitVecPush(&bv, (i / 7) % 2 == 0);
    }

    // Stress test random access patterns
    for (int test = 0; test < 1000; test++) {
        u64  idx      = (test * 7 + test * 3) % size; // Pseudo-random indices
        bool expected = (idx / 7) % 2 == 0;
        bool actual   = BitVecGet(&bv, idx);
        result        = result && (actual == expected);
    }

    // Stress test setting and getting at boundaries
    u64 boundaries[]   = {0, 1, 7, 8, 63, 64, 127, 128, 255, 256, 511, 512, 1023, 1024, size - 1};
    int boundary_count = sizeof(boundaries) / sizeof(boundaries[0]);

    for (int i = 0; i < boundary_count; i++) {
        if (boundaries[i] < size) {
            bool original = BitVecGet(&bv, boundaries[i]);
            BitVecFlip(&bv, boundaries[i]);
            result = result && (BitVecGet(&bv, boundaries[i]) == !original);
            BitVecFlip(&bv, boundaries[i]); // Flip back
            result = result && (BitVecGet(&bv, boundaries[i]) == original);
        }
    }

    BitVecDeinit(&bv);
    return result;
}

// NEW: Comprehensive bit pattern testing
bool test_bitvec_bit_patterns_comprehensive(void) {
    printf("Testing BitVec comprehensive bit patterns\n");

    BitVec bv     = BitVecInit();
    bool   result = true;

    // Test all-zeros pattern
    for (int i = 0; i < 100; i++) {
        BitVecPush(&bv, false);
    }

    result = result && (BitVecCountOnes(&bv) == 0);
    result = result && (BitVecCountZeros(&bv) == 100);

    // Verify all bits are false
    for (int i = 0; i < 100; i++) {
        result = result && (BitVecGet(&bv, i) == false);
    }

    // Change to all-ones pattern
    for (int i = 0; i < 100; i++) {
        BitVecSet(&bv, i, true);
    }

    result = result && (BitVecCountOnes(&bv) == 100);
    result = result && (BitVecCountZeros(&bv) == 0);

    // Test checkerboard pattern
    BitVecClear(&bv);
    for (int i = 0; i < 100; i++) {
        BitVecPush(&bv, i % 2 == 0);
    }

    result = result && (BitVecCountOnes(&bv) == 50);
    result = result && (BitVecCountZeros(&bv) == 50);

    // Verify checkerboard pattern
    for (int i = 0; i < 100; i++) {
        bool expected = (i % 2 == 0);
        result        = result && (BitVecGet(&bv, i) == expected);
    }

    // Test Fibonacci-like pattern (each bit is XOR of previous two)
    BitVecClear(&bv);
    BitVecPush(&bv, true); // F(0) = 1
    BitVecPush(&bv, true); // F(1) = 1
    for (int i = 2; i < 50; i++) {
        bool prev1 = BitVecGet(&bv, i - 1);
        bool prev2 = BitVecGet(&bv, i - 2);
        BitVecPush(&bv, prev1 != prev2); // XOR
    }

    // Verify first few Fibonacci bits
    result = result && (BitVecGet(&bv, 0) == true);  // F(0) = 1
    result = result && (BitVecGet(&bv, 1) == true);  // F(1) = 1
    result = result && (BitVecGet(&bv, 2) == false); // F(2) = 1 XOR 1 = 0
    result = result && (BitVecGet(&bv, 3) == true);  // F(3) = 1 XOR 0 = 1
    result = result && (BitVecGet(&bv, 4) == true);  // F(4) = 0 XOR 1 = 1

    BitVecDeinit(&bv);
    return result;
}

// Deadend tests
bool test_bitvec_access_null_failures(void) {
    printf("Testing BitVec access NULL pointer handling\n");

    // Test NULL bitvec pointer - should abort
    BitVecGet(NULL, 0);

    return false;
}

bool test_bitvec_set_null_failures(void) {
    printf("Testing BitVec set NULL pointer handling\n");

    // Test NULL bitvec pointer - should abort
    BitVecSet(NULL, 0, true);

    return false;
}

bool test_bitvec_flip_null_failures(void) {
    printf("Testing BitVec flip NULL pointer handling\n");

    // Test NULL bitvec pointer - should abort
    BitVecFlip(NULL, 0);

    return false;
}

bool test_bitvec_get_bounds_failures(void) {
    printf("Testing BitVec get bounds checking\n");

    BitVec bv = BitVecInit();

    // Test get from empty bitvec - should abort
    BitVecGet(&bv, 0);

    BitVecDeinit(&bv);
    return false;
}

bool test_bitvec_set_bounds_failures(void) {
    printf("Testing BitVec set bounds checking\n");

    BitVec bv = BitVecInit();

    // Test set on empty bitvec - should abort
    BitVecSet(&bv, 0, true);

    BitVecDeinit(&bv);
    return false;
}

bool test_bitvec_flip_bounds_failures(void) {
    printf("Testing BitVec flip bounds checking\n");

    BitVec bv = BitVecInit();

    // Test flip on empty bitvec - should abort
    BitVecFlip(&bv, 0);

    BitVecDeinit(&bv);
    return false;
}

// NEW: More specific bounds checking deadend tests
bool test_bitvec_get_large_index_failures(void) {
    printf("Testing BitVec get with large out-of-bounds index\n");

    BitVec bv = BitVecInit();
    BitVecPush(&bv, true);
    BitVecPush(&bv, false);
    BitVecPush(&bv, true);

    // Test with index way beyond length (3) - should abort
    BitVecGet(&bv, 1000);

    BitVecDeinit(&bv);
    return false;
}

bool test_bitvec_set_large_index_failures(void) {
    printf("Testing BitVec set with large out-of-bounds index\n");

    BitVec bv = BitVecInit();
    BitVecPush(&bv, true);
    BitVecPush(&bv, false);

    // Test with index way beyond length (2) - should abort
    BitVecSet(&bv, 500, true);

    BitVecDeinit(&bv);
    return false;
}

bool test_bitvec_flip_edge_index_failures(void) {
    printf("Testing BitVec flip with edge case out-of-bounds index\n");

    BitVec bv = BitVecInit();
    for (int i = 0; i < 10; i++) {
        BitVecPush(&bv, i % 2 == 0);
    }

    // Test with index exactly at length (invalid) - should abort
    BitVecFlip(&bv, 10);

    BitVecDeinit(&bv);
    return false;
}

bool test_bitvec_count_null_failures(void) {
    printf("Testing BitVec count operations with NULL pointer\n");

    // Test NULL bitvec pointer - should abort
    BitVecCountOnes(NULL);

    return false;
}

bool test_bitvec_get_max_index_failures(void) {
    printf("Testing BitVec get with maximum index value\n");

    BitVec bv = BitVecInit();
    BitVecPush(&bv, true);

    // Test with maximum possible index value - should abort
    BitVecGet(&bv, SIZE_MAX);

    BitVecDeinit(&bv);
    return false;
}

// Test BitVecFind and BitVecFindLast functions
bool test_bitvec_find_functions(void) {
    printf("Testing BitVecFind and BitVecFindLast functions\n");

    BitVec bv     = BitVecInit();
    bool   result = true;

    // Create pattern: 10110100
    BitVecPush(&bv, true);  // 0
    BitVecPush(&bv, false); // 1
    BitVecPush(&bv, true);  // 2
    BitVecPush(&bv, true);  // 3
    BitVecPush(&bv, false); // 4
    BitVecPush(&bv, true);  // 5
    BitVecPush(&bv, false); // 6
    BitVecPush(&bv, false); // 7

    // Test finding first true (should be at index 0)
    result = result && (BitVecFind(&bv, true) == 0);

    // Test finding first false (should be at index 1)
    result = result && (BitVecFind(&bv, false) == 1);

    // Test finding last true (should be at index 5)
    result = result && (BitVecFindLast(&bv, true) == 5);

    // Test finding last false (should be at index 7)
    result = result && (BitVecFindLast(&bv, false) == 7);

    // Test with all same values
    BitVecClear(&bv);
    BitVecPush(&bv, true);
    BitVecPush(&bv, true);
    BitVecPush(&bv, true);

    result = result && (BitVecFind(&bv, true) == 0);
    result = result && (BitVecFindLast(&bv, true) == 2);
    result = result && (BitVecFind(&bv, false) == SIZE_MAX);
    result = result && (BitVecFindLast(&bv, false) == SIZE_MAX);

    BitVecDeinit(&bv);
    return result;
}

// Test BitVecAll, BitVecAny, BitVecNone functions
bool test_bitvec_predicate_functions(void) {
    printf("Testing BitVecAll, BitVecAny, BitVecNone functions\n");

    BitVec bv     = BitVecInit();
    bool   result = true;

    // Test with all true bits
    BitVecPush(&bv, true);
    BitVecPush(&bv, true);
    BitVecPush(&bv, true);

    result = result && BitVecAll(&bv, true);
    result = result && !BitVecAll(&bv, false);
    result = result && BitVecAny(&bv, true);
    result = result && !BitVecAny(&bv, false);
    result = result && !BitVecNone(&bv, true);
    result = result && BitVecNone(&bv, false);

    // Test with all false bits
    BitVecClear(&bv);
    BitVecPush(&bv, false);
    BitVecPush(&bv, false);
    BitVecPush(&bv, false);

    result = result && !BitVecAll(&bv, true);
    result = result && BitVecAll(&bv, false);
    result = result && !BitVecAny(&bv, true);
    result = result && BitVecAny(&bv, false);
    result = result && BitVecNone(&bv, true);
    result = result && !BitVecNone(&bv, false);

    // Test with mixed bits
    BitVecClear(&bv);
    BitVecPush(&bv, true);
    BitVecPush(&bv, false);
    BitVecPush(&bv, true);

    result = result && !BitVecAll(&bv, true);
    result = result && !BitVecAll(&bv, false);
    result = result && BitVecAny(&bv, true);
    result = result && BitVecAny(&bv, false);
    result = result && !BitVecNone(&bv, true);
    result = result && !BitVecNone(&bv, false);

    BitVecDeinit(&bv);
    return result;
}

// Test BitVecLongestRun function
bool test_bitvec_longest_run(void) {
    printf("Testing BitVecLongestRun function\n");

    BitVec bv     = BitVecInit();
    bool   result = true;

    // Test pattern: 11100110011111
    BitVecPush(&bv, true);  // Run of 3 trues
    BitVecPush(&bv, true);
    BitVecPush(&bv, true);
    BitVecPush(&bv, false); // 1 false
    BitVecPush(&bv, false); // Run of 2 falses
    BitVecPush(&bv, true);  // 1 true
    BitVecPush(&bv, true);  // Run of 2 trues
    BitVecPush(&bv, false); // 1 false
    BitVecPush(&bv, false); // Run of 2 falses
    BitVecPush(&bv, true);  // Run of 5 trues
    BitVecPush(&bv, true);
    BitVecPush(&bv, true);
    BitVecPush(&bv, true);
    BitVecPush(&bv, true);

    // Longest run of trues should be 5
    result = result && (BitVecLongestRun(&bv, true) == 5);

    // Longest run of falses should be 2
    result = result && (BitVecLongestRun(&bv, false) == 2);

    // Test with all same values
    BitVecClear(&bv);
    for (int i = 0; i < 10; i++) {
        BitVecPush(&bv, true);
    }
    result = result && (BitVecLongestRun(&bv, true) == 10);
    result = result && (BitVecLongestRun(&bv, false) == 0);

    // Test alternating pattern
    BitVecClear(&bv);
    for (int i = 0; i < 10; i++) {
        BitVecPush(&bv, i % 2 == 0);
    }
    result = result && (BitVecLongestRun(&bv, true) == 1);
    result = result && (BitVecLongestRun(&bv, false) == 1);

    BitVecDeinit(&bv);
    return result;
}

// Edge case tests for Find functions
bool test_bitvec_find_edge_cases(void) {
    printf("Testing BitVecFind edge cases\n");

    BitVec bv     = BitVecInit();
    bool   result = true;

    // Test empty bitvector
    result = result && (BitVecFind(&bv, true) == SIZE_MAX);
    result = result && (BitVecFind(&bv, false) == SIZE_MAX);
    result = result && (BitVecFindLast(&bv, true) == SIZE_MAX);
    result = result && (BitVecFindLast(&bv, false) == SIZE_MAX);

    // Test single element
    BitVecPush(&bv, true);
    result = result && (BitVecFind(&bv, true) == 0);
    result = result && (BitVecFindLast(&bv, true) == 0);
    result = result && (BitVecFind(&bv, false) == SIZE_MAX);
    result = result && (BitVecFindLast(&bv, false) == SIZE_MAX);

    // Test with large bitvector
    BitVecClear(&bv);
    for (int i = 0; i < 1000; i++) {
        BitVecPush(&bv, i == 500 || i == 999); // Only indices 500 and 999 are true
    }
    result = result && (BitVecFind(&bv, true) == 500);
    result = result && (BitVecFindLast(&bv, true) == 999);

    BitVecDeinit(&bv);
    return result;
}

// Edge case tests for predicate functions
bool test_bitvec_predicate_edge_cases(void) {
    printf("Testing BitVec predicate edge cases\n");

    BitVec bv     = BitVecInit();
    bool   result = true;

    // Test empty bitvector - all predicates should return true for empty set
    result = result && BitVecAll(&bv, true);
    result = result && BitVecAll(&bv, false);
    result = result && !BitVecAny(&bv, true);
    result = result && !BitVecAny(&bv, false);
    result = result && BitVecNone(&bv, true);
    result = result && BitVecNone(&bv, false);

    // Test single element bitvector
    BitVecPush(&bv, true);
    result = result && BitVecAll(&bv, true);
    result = result && !BitVecAll(&bv, false);
    result = result && BitVecAny(&bv, true);
    result = result && !BitVecAny(&bv, false);

    // Test large bitvector with specific patterns
    BitVecClear(&bv);
    for (int i = 0; i < 1000; i++) {
        BitVecPush(&bv, true); // All true
    }
    result = result && BitVecAll(&bv, true);
    result = result && !BitVecAll(&bv, false);

    // Change one bit to false
    BitVecSet(&bv, 500, false);
    result = result && !BitVecAll(&bv, true);
    result = result && BitVecAny(&bv, true);
    result = result && BitVecAny(&bv, false);

    BitVecDeinit(&bv);
    return result;
}

// Edge case tests for LongestRun function
bool test_bitvec_longest_run_edge_cases(void) {
    printf("Testing BitVecLongestRun edge cases\n");

    BitVec bv     = BitVecInit();
    bool   result = true;

    // Test empty bitvector
    result = result && (BitVecLongestRun(&bv, true) == 0);
    result = result && (BitVecLongestRun(&bv, false) == 0);

    // Test single element
    BitVecPush(&bv, true);
    result = result && (BitVecLongestRun(&bv, true) == 1);
    result = result && (BitVecLongestRun(&bv, false) == 0);

    // Test large runs
    BitVecClear(&bv);
    for (int i = 0; i < 10000; i++) {
        BitVecPush(&bv, true);
    }
    result = result && (BitVecLongestRun(&bv, true) == 10000);
    result = result && (BitVecLongestRun(&bv, false) == 0);

    // Test with one interruption in the middle
    BitVecSet(&bv, 5000, false);
    result = result && (BitVecLongestRun(&bv, true) == 5000);
    result = result && (BitVecLongestRun(&bv, false) == 1);

    BitVecDeinit(&bv);
    return result;
}

// Deadend tests - testing NULL pointers and invalid conditions that should cause fatal errors
bool test_bitvec_find_deadend_tests(void) {
    printf("Testing BitVecFind deadend scenarios\n");

    // This should cause LOG_FATAL and terminate the program
    BitVecFind(NULL, true);

    return true; // Should never reach here
}

bool test_bitvec_predicate_deadend_tests(void) {
    printf("Testing BitVec predicate deadend scenarios\n");

    // This should cause LOG_FATAL and terminate the program
    BitVecAll(NULL, true);

    return true; // Should never reach here
}

bool test_bitvec_longest_run_deadend_tests(void) {
    printf("Testing BitVecLongestRun deadend scenarios\n");

    // This should cause LOG_FATAL and terminate the program
    BitVecLongestRun(NULL, true);

    return true; // Should never reach here
}

// Main function that runs all tests
int main(void) {
    printf("[INFO] Starting BitVec.Access tests\n\n");

    // Array of test functions (adding new tests to existing ones)
    TestFunction tests[] = {
        test_bitvec_get,
        test_bitvec_set,
        test_bitvec_flip,
        test_bitvec_length_capacity,
        test_bitvec_count_operations,
        test_bitvec_get_edge_cases,
        test_bitvec_set_edge_cases,
        test_bitvec_flip_edge_cases,
        test_bitvec_count_edge_cases,
        test_bitvec_access_multiple_operations,
        test_bitvec_access_large_patterns,
        test_bitvec_macro_functions,
        test_bitvec_access_stress_test,
        test_bitvec_bit_patterns_comprehensive,
        // New comprehensive tests for missing functions
        test_bitvec_find_functions,
        test_bitvec_predicate_functions,
        test_bitvec_longest_run,
        test_bitvec_find_edge_cases,
        test_bitvec_predicate_edge_cases,
        test_bitvec_longest_run_edge_cases
    };

    // Deadend tests that would cause program termination
    TestFunction deadend_tests[] = {
        test_bitvec_find_deadend_tests,
        test_bitvec_predicate_deadend_tests,
        test_bitvec_longest_run_deadend_tests,
        test_bitvec_access_null_failures,
        test_bitvec_set_null_failures,
        test_bitvec_flip_null_failures,
        test_bitvec_get_bounds_failures,
        test_bitvec_set_bounds_failures,
        test_bitvec_flip_bounds_failures,
        test_bitvec_get_large_index_failures,
        test_bitvec_set_large_index_failures,
        test_bitvec_flip_edge_index_failures,
        test_bitvec_count_null_failures,
        test_bitvec_get_max_index_failures
    };

    int total_tests         = sizeof(tests) / sizeof(tests[0]);
    int total_deadend_tests = sizeof(deadend_tests) / sizeof(deadend_tests[0]);

    // Run all tests using the centralized test driver
    return run_test_suite(tests, total_tests, deadend_tests, total_deadend_tests, "BitVec.Access");
}