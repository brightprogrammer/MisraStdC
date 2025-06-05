#include <Misra/Std/Container/BitVec.h>
#include <Misra/Std/Log.h>

#include <stdio.h>
#include <Misra/Types.h> // For size and other type definitions

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

// Test BitVecGet function
bool test_bitvec_get(void) {
    printf("Testing BitVecGet\n");

    BitVec bv = BitVecInit();

    // Push some bits
    BitVecPush(&bv, true);
    BitVecPush(&bv, false);
    BitVecPush(&bv, true);
    BitVecPush(&bv, false);

    // Test getting bits
    bool result = (BitVecGet(&bv, 0) == true) && (BitVecGet(&bv, 1) == false) && (BitVecGet(&bv, 2) == true) &&
                  (BitVecGet(&bv, 3) == false);

    BitVecDeinit(&bv);
    return result;
}

// Test BitVecSet function
bool test_bitvec_set(void) {
    printf("Testing BitVecSet\n");

    BitVec bv = BitVecInit();

    // Reserve space and set bits
    BitVecResize(&bv, 4);
    BitVecSet(&bv, 0, true);
    BitVecSet(&bv, 1, false);
    BitVecSet(&bv, 2, true);
    BitVecSet(&bv, 3, false);

    // Test getting the set bits
    bool result = (BitVecGet(&bv, 0) == true) && (BitVecGet(&bv, 1) == false) && (BitVecGet(&bv, 2) == true) &&
                  (BitVecGet(&bv, 3) == false);

    BitVecDeinit(&bv);
    return result;
}

// Test BitVecFlip function
bool test_bitvec_flip(void) {
    printf("Testing BitVecFlip\n");

    BitVec bv = BitVecInit();

    // Push some bits
    BitVecPush(&bv, true);
    BitVecPush(&bv, false);
    BitVecPush(&bv, true);
    BitVecPush(&bv, false);

    // Flip some bits
    BitVecFlip(&bv, 0);
    BitVecFlip(&bv, 1);

    // Test the flipped bits
    bool result = (BitVecGet(&bv, 0) == false) && // was true, now false
                  (BitVecGet(&bv, 1) == true) &&  // was false, now true
                  (BitVecGet(&bv, 2) == true) &&  // unchanged
                  (BitVecGet(&bv, 3) == false);   // unchanged

    BitVecDeinit(&bv);
    return result;
}

// Test BitVecLength and BitVecCapacity functions
bool test_bitvec_length_capacity(void) {
    printf("Testing BitVecLength and BitVecCapacity\n");

    BitVec bv = BitVecInit();

    // Initially empty
    bool result = (BitVecLen(&bv) == 0);

    // Push some bits
    BitVecPush(&bv, true);
    BitVecPush(&bv, false);
    BitVecPush(&bv, true);

    result = result && (BitVecLen(&bv) == 3);
    result = result && (BitVecCapacity(&bv) >= 3);

    // Reserve more space
    BitVecReserve(&bv, 100);
    result = result && (BitVecLen(&bv) == 3);
    result = result && (BitVecCapacity(&bv) >= 100);

    BitVecDeinit(&bv);
    return result;
}

// Test BitVecCount functions
bool test_bitvec_count_operations(void) {
    printf("Testing BitVecCount operations\n");

    BitVec bv = BitVecInit();

    // Push a pattern: true, false, true, false, true
    BitVecPush(&bv, true);
    BitVecPush(&bv, false);
    BitVecPush(&bv, true);
    BitVecPush(&bv, false);
    BitVecPush(&bv, true);

    // Count true and false bits
    bool result = (BitVecCountOnes(&bv) == 3) && (BitVecCountZeros(&bv) == 2);

    BitVecDeinit(&bv);
    return result;
}

// Edge case tests for BitVecGet
bool test_bitvec_get_edge_cases(void) {
    printf("Testing BitVecGet edge cases\n");

    BitVec bv     = BitVecInit();
    bool   result = true;

    // Test with single bit
    BitVecPush(&bv, true);
    result = result && (BitVecGet(&bv, 0) == true);

    // Test with larger index
    for (int i = 1; i < 64; i++) {
        BitVecPush(&bv, i % 2 == 0);
    }

    result = result && (BitVecGet(&bv, 63) == false); // 63 % 2 == 1, so i%2==0 is false for i=63

    BitVecDeinit(&bv);
    return result;
}

// Edge case tests for BitVecSet
bool test_bitvec_set_edge_cases(void) {
    printf("Testing BitVecSet edge cases\n");

    BitVec bv = BitVecInit();

    // Set first bit
    BitVecResize(&bv, 1);
    BitVecSet(&bv, 0, true);
    bool result = (BitVecGet(&bv, 0) == true);

    // Set same bit to false
    BitVecSet(&bv, 0, false);
    result = result && (BitVecGet(&bv, 0) == false);

    BitVecDeinit(&bv);
    return result;
}

// Edge case tests for BitVecFlip
bool test_bitvec_flip_edge_cases(void) {
    printf("Testing BitVecFlip edge cases\n");

    BitVec bv = BitVecInit();

    // Test flipping single bit
    BitVecPush(&bv, false);
    BitVecFlip(&bv, 0);
    bool result = (BitVecGet(&bv, 0) == true);

    // Flip it again
    BitVecFlip(&bv, 0);
    result = result && (BitVecGet(&bv, 0) == false);

    BitVecDeinit(&bv);
    return result;
}

// Edge case tests for BitVecCount
bool test_bitvec_count_edge_cases(void) {
    printf("Testing BitVecCount edge cases\n");

    BitVec bv     = BitVecInit();
    bool   result = true;

    // Test empty bitvector
    result = result && (BitVecCountOnes(&bv) == 0);
    result = result && (BitVecCountZeros(&bv) == 0);

    // Test single bit
    BitVecPush(&bv, true);
    result = result && (BitVecCountOnes(&bv) == 1);
    result = result && (BitVecCountZeros(&bv) == 0);

    // Test all same bits
    BitVecClear(&bv);
    for (int i = 0; i < 100; i++) {
        BitVecPush(&bv, true);
    }
    result = result && (BitVecCountOnes(&bv) == 100);
    result = result && (BitVecCountZeros(&bv) == 0);

    BitVecDeinit(&bv);
    return result;
}

// Test multiple operations together
bool test_bitvec_access_multiple_operations(void) {
    printf("Testing BitVec multiple access operations\n");

    BitVec bv     = BitVecInit();
    bool   result = true;

    // Create pattern using different methods
    BitVecPush(&bv, true);
    BitVecResize(&bv, 5);
    BitVecSet(&bv, 1, false);
    BitVecSet(&bv, 2, true);
    BitVecSet(&bv, 3, false);
    BitVecSet(&bv, 4, true);

    // Verify pattern: T F T F T
    result = result && (BitVecGet(&bv, 0) == true);
    result = result && (BitVecGet(&bv, 1) == false);
    result = result && (BitVecGet(&bv, 2) == true);
    result = result && (BitVecGet(&bv, 3) == false);
    result = result && (BitVecGet(&bv, 4) == true);

    // Count and verify
    result = result && (BitVecCountOnes(&bv) == 3);
    result = result && (BitVecCountZeros(&bv) == 2);

    // Flip some bits and verify
    BitVecFlip(&bv, 1); // F -> T
    BitVecFlip(&bv, 3); // F -> T

    result = result && (BitVecCountOnes(&bv) == 5);
    result = result && (BitVecCountZeros(&bv) == 0);

    BitVecDeinit(&bv);
    return result;
}

// Test with large patterns
bool test_bitvec_access_large_patterns(void) {
    printf("Testing BitVec access with large patterns\n");

    BitVec bv     = BitVecInit();
    bool   result = true;

    // Create large alternating pattern
    for (int i = 0; i < 1000; i++) {
        BitVecPush(&bv, i % 2 == 0);
    }

    // Verify some positions
    result = result && (BitVecGet(&bv, 0) == true);    // 0 % 2 == 0
    result = result && (BitVecGet(&bv, 1) == false);   // 1 % 2 != 0
    result = result && (BitVecGet(&bv, 500) == true);  // 500 % 2 == 0
    result = result && (BitVecGet(&bv, 999) == false); // 999 % 2 != 0

    // Verify counts
    result = result && (BitVecCountOnes(&bv) == 500);
    result = result && (BitVecCountZeros(&bv) == 500);

    // Flip some bits and verify
    BitVecFlip(&bv, 0);   // T -> F
    BitVecFlip(&bv, 1);   // F -> T
    BitVecFlip(&bv, 500); // T -> F
    BitVecFlip(&bv, 999); // F -> T

    result = result && (BitVecCountOnes(&bv) == 500);
    result = result && (BitVecCountZeros(&bv) == 500);

    BitVecDeinit(&bv);
    return result;
}

// Test macro functions
bool test_bitvec_macro_functions(void) {
    printf("Testing BitVec macro functions\n");

    BitVec bv     = BitVecInit();
    bool   result = true;

    // Test BITVEC_GET, BITVEC_SET, BITVEC_FLIP if they exist
    BitVecPush(&bv, true);
    BitVecPush(&bv, false);

    // Test length and capacity macros if they exist
    result = result && (BitVecLen(&bv) == 2);
    result = result && (BitVecCapacity(&bv) >= 2);

    // Test some bit operations
    BitVecSet(&bv, 0, false);
    BitVecSet(&bv, 1, true);

    result = result && (BitVecGet(&bv, 0) == false);
    result = result && (BitVecGet(&bv, 1) == true);

    // Test count operations
    result = result && (BitVecCountOnes(&bv) == 1);
    result = result && (BitVecCountZeros(&bv) == 1);

    BitVecDeinit(&bv);
    return result;
}

// Stress test for access operations
bool test_bitvec_access_stress_test(void) {
    printf("Testing BitVec access stress test\n");

    BitVec bv     = BitVecInit();
    bool   result = true;

    // Create large bitvector
    const int size = 10000;
    BitVecReserve(&bv, size);

    // Set alternating pattern
    for (int i = 0; i < size; i++) {
        BitVecResize(&bv, i + 1);
        BitVecSet(&bv, i, i % 3 == 0); // Every third bit is true
    }

    // Verify pattern
    for (int i = 0; i < size && result; i++) {
        result = result && (BitVecGet(&bv, i) == (i % 3 == 0));
    }

    // Count and verify
    int expected_true_count = 0;
    for (int i = 0; i < size; i++) {
        if (i % 3 == 0)
            expected_true_count++;
    }

    result = result && (BitVecCountOnes(&bv) == expected_true_count);
    result = result && (BitVecCountZeros(&bv) == (size - expected_true_count));

    // Flip every 7th bit
    for (int i = 0; i < size; i += 7) {
        BitVecFlip(&bv, i);
    }

    // Verify flipped bits
    for (int i = 0; i < size && result; i++) {
        bool expected = (i % 3 == 0);
        if (i % 7 == 0) {
            expected = !expected; // Flipped
        }
        result = result && (BitVecGet(&bv, i) == expected);
    }

    BitVecDeinit(&bv);
    return result;
}

// Comprehensive bit pattern testing
bool test_bitvec_bit_patterns_comprehensive(void) {
    printf("Testing BitVec comprehensive bit patterns\n");

    BitVec bv     = BitVecInit();
    bool   result = true;

    // Test all zeros
    for (int i = 0; i < 64; i++) {
        BitVecPush(&bv, false);
    }
    result = result && (BitVecCountZeros(&bv) == 64);
    result = result && (BitVecCountOnes(&bv) == 0);

    // Test all ones
    BitVecClear(&bv);
    for (int i = 0; i < 64; i++) {
        BitVecPush(&bv, true);
    }
    result = result && (BitVecCountOnes(&bv) == 64);
    result = result && (BitVecCountZeros(&bv) == 0);

    // Test checkerboard pattern
    BitVecClear(&bv);
    for (int i = 0; i < 64; i++) {
        BitVecPush(&bv, i % 2 == 0);
    }
    result = result && (BitVecCountOnes(&bv) == 32);
    result = result && (BitVecCountZeros(&bv) == 32);

    // Test sparse pattern (every 8th bit)
    BitVecClear(&bv);
    for (int i = 0; i < 64; i++) {
        BitVecPush(&bv, i % 8 == 0);
    }
    result = result && (BitVecCountOnes(&bv) == 8);
    result = result && (BitVecCountZeros(&bv) == 56);

    // Test random-like pattern (using simple algorithm)
    BitVecClear(&bv);
    for (int i = 0; i < 100; i++) {
        BitVecPush(&bv, (i * 17 + 3) % 7 < 3); // Pseudo-random pattern
    }

    // Verify we can access all bits
    for (int i = 0; i < 100 && result; i++) {
        bool expected = (i * 17 + 3) % 7 < 3;
        result        = result && (BitVecGet(&bv, i) == expected);
    }

    BitVecDeinit(&bv);
    return result;
}

// Test BitVecFind functions (Find, FindLast)
bool test_bitvec_find_functions(void) {
    printf("Testing BitVecFind functions\n");

    BitVec bv     = BitVecInit();
    bool   result = true;

    // Create pattern: F T F T F T F T
    for (int i = 0; i < 8; i++) {
        BitVecPush(&bv, i % 2 == 1);
    }

    // Test BitVecFind
    result = result && (BitVecFind(&bv, true) == 1);  // First true at index 1
    result = result && (BitVecFind(&bv, false) == 0); // First false at index 0

    // Test BitVecFindLast
    result = result && (BitVecFindLast(&bv, true) == 7);  // Last true at index 7
    result = result && (BitVecFindLast(&bv, false) == 6); // Last false at index 6

    // Test with all same values
    BitVecClear(&bv);
    for (int i = 0; i < 5; i++) {
        BitVecPush(&bv, true);
    }
    result = result && (BitVecFind(&bv, true) == 0);
    result = result && (BitVecFindLast(&bv, true) == 4);
    result = result && (BitVecFind(&bv, false) == SIZE_MAX);
    result = result && (BitVecFindLast(&bv, false) == SIZE_MAX);

    BitVecDeinit(&bv);
    return result;
}

// Test BitVec predicate functions (All, Any, None)
bool test_bitvec_predicate_functions(void) {
    printf("Testing BitVec predicate functions\n");

    BitVec bv     = BitVecInit();
    bool   result = true;

    // Test with all true
    for (int i = 0; i < 5; i++) {
        BitVecPush(&bv, true);
    }
    result = result && BitVecAll(&bv, true);
    result = result && !BitVecAll(&bv, false);
    result = result && BitVecAny(&bv, true);
    result = result && !BitVecAny(&bv, false);
    result = result && !BitVecNone(&bv, true);
    result = result && BitVecNone(&bv, false);

    // Test with all false
    BitVecClear(&bv);
    for (int i = 0; i < 5; i++) {
        BitVecPush(&bv, false);
    }
    result = result && !BitVecAll(&bv, true);
    result = result && BitVecAll(&bv, false);
    result = result && !BitVecAny(&bv, true);
    result = result && BitVecAny(&bv, false);
    result = result && BitVecNone(&bv, true);
    result = result && !BitVecNone(&bv, false);

    // Test with mixed values
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
    printf("Testing BitVecLongestRun\n");

    BitVec bv     = BitVecInit();
    bool   result = true;

    // Test pattern: T T T F F T T F F F F
    bool pattern[] = {true, true, true, false, false, true, true, false, false, false, false};
    for (int i = 0; i < 11; i++) {
        BitVecPush(&bv, pattern[i]);
    }

    // Longest run of true should be 3, longest run of false should be 4
    result = result && (BitVecLongestRun(&bv, true) == 3);
    result = result && (BitVecLongestRun(&bv, false) == 4);

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

// Main function that runs all tests
int main(void) {
    printf("[INFO] Starting BitVec.Access.Simple tests\n\n");

    // Array of test functions
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
        test_bitvec_find_functions,
        test_bitvec_predicate_functions,
        test_bitvec_longest_run,
        test_bitvec_find_edge_cases,
        test_bitvec_predicate_edge_cases,
        test_bitvec_longest_run_edge_cases
    };

    int total_tests = sizeof(tests) / sizeof(tests[0]);

    // Run all tests using the centralized test driver
    return run_test_suite(tests, total_tests, NULL, 0, "BitVec.Access.Simple");
}