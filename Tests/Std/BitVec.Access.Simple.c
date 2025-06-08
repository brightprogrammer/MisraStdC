#include <Misra/Std/Container/Bits.h>
#include <Misra/Std/Log.h>

#include <stdio.h>
#include <Misra/Types.h> // For size and other type definitions

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

// Test BitsGet function
bool test_Bits_get(void) {
    printf("Testing BitsGet\n");

    Bits bv = BitsInit();

    // Push some bits
    BitsPush(&bv, true);
    BitsPush(&bv, false);
    BitsPush(&bv, true);
    BitsPush(&bv, false);

    // Test getting bits
    bool result = (BitsGet(&bv, 0) == true) && (BitsGet(&bv, 1) == false) && (BitsGet(&bv, 2) == true) &&
                  (BitsGet(&bv, 3) == false);

    BitsDeinit(&bv);
    return result;
}

// Test BitsSet function
bool test_Bits_set(void) {
    printf("Testing BitsSet\n");

    Bits bv = BitsInit();

    // Reserve space and set bits
    BitsResize(&bv, 4);
    BitsSet(&bv, 0, true);
    BitsSet(&bv, 1, false);
    BitsSet(&bv, 2, true);
    BitsSet(&bv, 3, false);

    // Test getting the set bits
    bool result = (BitsGet(&bv, 0) == true) && (BitsGet(&bv, 1) == false) && (BitsGet(&bv, 2) == true) &&
                  (BitsGet(&bv, 3) == false);

    BitsDeinit(&bv);
    return result;
}

// Test BitsFlip function
bool test_Bits_flip(void) {
    printf("Testing BitsFlip\n");

    Bits bv = BitsInit();

    // Push some bits
    BitsPush(&bv, true);
    BitsPush(&bv, false);
    BitsPush(&bv, true);
    BitsPush(&bv, false);

    // Flip some bits
    BitsFlip(&bv, 0);
    BitsFlip(&bv, 1);

    // Test the flipped bits
    bool result = (BitsGet(&bv, 0) == false) && // was true, now false
                  (BitsGet(&bv, 1) == true) &&  // was false, now true
                  (BitsGet(&bv, 2) == true) &&  // unchanged
                  (BitsGet(&bv, 3) == false);   // unchanged

    BitsDeinit(&bv);
    return result;
}

// Test BitsLength and BitsCapacity functions
bool test_Bits_length_capacity(void) {
    printf("Testing BitsLength and BitsCapacity\n");

    Bits bv = BitsInit();

    // Initially empty
    bool result = (BitsLen(&bv) == 0);

    // Push some bits
    BitsPush(&bv, true);
    BitsPush(&bv, false);
    BitsPush(&bv, true);

    result = result && (BitsLen(&bv) == 3);
    result = result && (BitsCapacity(&bv) >= 3);

    // Reserve more space
    BitsReserve(&bv, 100);
    result = result && (BitsLen(&bv) == 3);
    result = result && (BitsCapacity(&bv) >= 100);

    BitsDeinit(&bv);
    return result;
}

// Test BitsCount functions
bool test_Bits_count_operations(void) {
    printf("Testing BitsCount operations\n");

    Bits bv = BitsInit();

    // Push a pattern: true, false, true, false, true
    BitsPush(&bv, true);
    BitsPush(&bv, false);
    BitsPush(&bv, true);
    BitsPush(&bv, false);
    BitsPush(&bv, true);

    // Count true and false bits
    bool result = (BitsCountOnes(&bv) == 3) && (BitsCountZeros(&bv) == 2);

    BitsDeinit(&bv);
    return result;
}

// Edge case tests for BitsGet
bool test_Bits_get_edge_cases(void) {
    printf("Testing BitsGet edge cases\n");

    Bits bv     = BitsInit();
    bool   result = true;

    // Test with single bit
    BitsPush(&bv, true);
    result = result && (BitsGet(&bv, 0) == true);

    // Test with larger index
    for (int i = 1; i < 64; i++) {
        BitsPush(&bv, i % 2 == 0);
    }

    result = result && (BitsGet(&bv, 63) == false); // 63 % 2 == 1, so i%2==0 is false for i=63

    BitsDeinit(&bv);
    return result;
}

// Edge case tests for BitsSet
bool test_Bits_set_edge_cases(void) {
    printf("Testing BitsSet edge cases\n");

    Bits bv = BitsInit();

    // Set first bit
    BitsResize(&bv, 1);
    BitsSet(&bv, 0, true);
    bool result = (BitsGet(&bv, 0) == true);

    // Set same bit to false
    BitsSet(&bv, 0, false);
    result = result && (BitsGet(&bv, 0) == false);

    BitsDeinit(&bv);
    return result;
}

// Edge case tests for BitsFlip
bool test_Bits_flip_edge_cases(void) {
    printf("Testing BitsFlip edge cases\n");

    Bits bv = BitsInit();

    // Test flipping single bit
    BitsPush(&bv, false);
    BitsFlip(&bv, 0);
    bool result = (BitsGet(&bv, 0) == true);

    // Flip it again
    BitsFlip(&bv, 0);
    result = result && (BitsGet(&bv, 0) == false);

    BitsDeinit(&bv);
    return result;
}

// Edge case tests for BitsCount
bool test_Bits_count_edge_cases(void) {
    printf("Testing BitsCount edge cases\n");

    Bits bv     = BitsInit();
    bool   result = true;

    // Test empty Bitstor
    result = result && (BitsCountOnes(&bv) == 0);
    result = result && (BitsCountZeros(&bv) == 0);

    // Test single bit
    BitsPush(&bv, true);
    result = result && (BitsCountOnes(&bv) == 1);
    result = result && (BitsCountZeros(&bv) == 0);

    // Test all same bits
    BitsClear(&bv);
    for (int i = 0; i < 100; i++) {
        BitsPush(&bv, true);
    }
    result = result && (BitsCountOnes(&bv) == 100);
    result = result && (BitsCountZeros(&bv) == 0);

    BitsDeinit(&bv);
    return result;
}

// Test multiple operations together
bool test_Bits_access_multiple_operations(void) {
    printf("Testing Bits multiple access operations\n");

    Bits bv     = BitsInit();
    bool   result = true;

    // Create pattern using different methods
    BitsPush(&bv, true);
    BitsResize(&bv, 5);
    BitsSet(&bv, 1, false);
    BitsSet(&bv, 2, true);
    BitsSet(&bv, 3, false);
    BitsSet(&bv, 4, true);

    // Verify pattern: T F T F T
    result = result && (BitsGet(&bv, 0) == true);
    result = result && (BitsGet(&bv, 1) == false);
    result = result && (BitsGet(&bv, 2) == true);
    result = result && (BitsGet(&bv, 3) == false);
    result = result && (BitsGet(&bv, 4) == true);

    // Count and verify
    result = result && (BitsCountOnes(&bv) == 3);
    result = result && (BitsCountZeros(&bv) == 2);

    // Flip some bits and verify
    BitsFlip(&bv, 1); // F -> T
    BitsFlip(&bv, 3); // F -> T

    result = result && (BitsCountOnes(&bv) == 5);
    result = result && (BitsCountZeros(&bv) == 0);

    BitsDeinit(&bv);
    return result;
}

// Test with large patterns
bool test_Bits_access_large_patterns(void) {
    printf("Testing Bits access with large patterns\n");

    Bits bv     = BitsInit();
    bool   result = true;

    // Create large alternating pattern
    for (int i = 0; i < 1000; i++) {
        BitsPush(&bv, i % 2 == 0);
    }

    // Verify some positions
    result = result && (BitsGet(&bv, 0) == true);    // 0 % 2 == 0
    result = result && (BitsGet(&bv, 1) == false);   // 1 % 2 != 0
    result = result && (BitsGet(&bv, 500) == true);  // 500 % 2 == 0
    result = result && (BitsGet(&bv, 999) == false); // 999 % 2 != 0

    // Verify counts
    result = result && (BitsCountOnes(&bv) == 500);
    result = result && (BitsCountZeros(&bv) == 500);

    // Flip some bits and verify
    BitsFlip(&bv, 0);   // T -> F
    BitsFlip(&bv, 1);   // F -> T
    BitsFlip(&bv, 500); // T -> F
    BitsFlip(&bv, 999); // F -> T

    result = result && (BitsCountOnes(&bv) == 500);
    result = result && (BitsCountZeros(&bv) == 500);

    BitsDeinit(&bv);
    return result;
}

// Test macro functions
bool test_Bits_macro_functions(void) {
    printf("Testing Bits macro functions\n");

    Bits bv     = BitsInit();
    bool   result = true;

    // Test Bits_GET, Bits_SET, Bits_FLIP if they exist
    BitsPush(&bv, true);
    BitsPush(&bv, false);

    // Test length and capacity macros if they exist
    result = result && (BitsLen(&bv) == 2);
    result = result && (BitsCapacity(&bv) >= 2);

    // Test some bit operations
    BitsSet(&bv, 0, false);
    BitsSet(&bv, 1, true);

    result = result && (BitsGet(&bv, 0) == false);
    result = result && (BitsGet(&bv, 1) == true);

    // Test count operations
    result = result && (BitsCountOnes(&bv) == 1);
    result = result && (BitsCountZeros(&bv) == 1);

    BitsDeinit(&bv);
    return result;
}

// Stress test for access operations
bool test_Bits_access_stress_test(void) {
    printf("Testing Bits access stress test\n");

    Bits bv     = BitsInit();
    bool   result = true;

    // Create large Bitstor
    const int size = 10000;
    BitsReserve(&bv, size);

    // Set alternating pattern
    for (int i = 0; i < size; i++) {
        BitsResize(&bv, i + 1);
        BitsSet(&bv, i, i % 3 == 0); // Every third bit is true
    }

    // Verify pattern
    for (int i = 0; i < size && result; i++) {
        result = result && (BitsGet(&bv, i) == (i % 3 == 0));
    }

    // Count and verify
    int expected_true_count = 0;
    for (int i = 0; i < size; i++) {
        if (i % 3 == 0)
            expected_true_count++;
    }

    result = result && (BitsCountOnes(&bv) == expected_true_count);
    result = result && (BitsCountZeros(&bv) == (size - expected_true_count));

    // Flip every 7th bit
    for (int i = 0; i < size; i += 7) {
        BitsFlip(&bv, i);
    }

    // Verify flipped bits
    for (int i = 0; i < size && result; i++) {
        bool expected = (i % 3 == 0);
        if (i % 7 == 0) {
            expected = !expected; // Flipped
        }
        result = result && (BitsGet(&bv, i) == expected);
    }

    BitsDeinit(&bv);
    return result;
}

// Comprehensive bit pattern testing
bool test_Bits_bit_patterns_comprehensive(void) {
    printf("Testing Bits comprehensive bit patterns\n");

    Bits bv     = BitsInit();
    bool   result = true;

    // Test all zeros
    for (int i = 0; i < 64; i++) {
        BitsPush(&bv, false);
    }
    result = result && (BitsCountZeros(&bv) == 64);
    result = result && (BitsCountOnes(&bv) == 0);

    // Test all ones
    BitsClear(&bv);
    for (int i = 0; i < 64; i++) {
        BitsPush(&bv, true);
    }
    result = result && (BitsCountOnes(&bv) == 64);
    result = result && (BitsCountZeros(&bv) == 0);

    // Test checkerboard pattern
    BitsClear(&bv);
    for (int i = 0; i < 64; i++) {
        BitsPush(&bv, i % 2 == 0);
    }
    result = result && (BitsCountOnes(&bv) == 32);
    result = result && (BitsCountZeros(&bv) == 32);

    // Test sparse pattern (every 8th bit)
    BitsClear(&bv);
    for (int i = 0; i < 64; i++) {
        BitsPush(&bv, i % 8 == 0);
    }
    result = result && (BitsCountOnes(&bv) == 8);
    result = result && (BitsCountZeros(&bv) == 56);

    // Test random-like pattern (using simple algorithm)
    BitsClear(&bv);
    for (int i = 0; i < 100; i++) {
        BitsPush(&bv, (i * 17 + 3) % 7 < 3); // Pseudo-random pattern
    }

    // Verify we can access all bits
    for (int i = 0; i < 100 && result; i++) {
        bool expected = (i * 17 + 3) % 7 < 3;
        result        = result && (BitsGet(&bv, i) == expected);
    }

    BitsDeinit(&bv);
    return result;
}

// Test BitsFind functions (Find, FindLast)
bool test_Bits_find_functions(void) {
    printf("Testing BitsFind functions\n");

    Bits bv     = BitsInit();
    bool   result = true;

    // Create pattern: F T F T F T F T
    for (int i = 0; i < 8; i++) {
        BitsPush(&bv, i % 2 == 1);
    }

    // Test BitsFind
    result = result && (BitsFind(&bv, true) == 1);  // First true at index 1
    result = result && (BitsFind(&bv, false) == 0); // First false at index 0

    // Test BitsFindLast
    result = result && (BitsFindLast(&bv, true) == 7);  // Last true at index 7
    result = result && (BitsFindLast(&bv, false) == 6); // Last false at index 6

    // Test with all same values
    BitsClear(&bv);
    for (int i = 0; i < 5; i++) {
        BitsPush(&bv, true);
    }
    result = result && (BitsFind(&bv, true) == 0);
    result = result && (BitsFindLast(&bv, true) == 4);
    result = result && (BitsFind(&bv, false) == SIZE_MAX);
    result = result && (BitsFindLast(&bv, false) == SIZE_MAX);

    BitsDeinit(&bv);
    return result;
}

// Test Bits predicate functions (All, Any, None)
bool test_Bits_predicate_functions(void) {
    printf("Testing Bits predicate functions\n");

    Bits bv     = BitsInit();
    bool   result = true;

    // Test with all true
    for (int i = 0; i < 5; i++) {
        BitsPush(&bv, true);
    }
    result = result && BitsAll(&bv, true);
    result = result && !BitsAll(&bv, false);
    result = result && BitsAny(&bv, true);
    result = result && !BitsAny(&bv, false);
    result = result && !BitsNone(&bv, true);
    result = result && BitsNone(&bv, false);

    // Test with all false
    BitsClear(&bv);
    for (int i = 0; i < 5; i++) {
        BitsPush(&bv, false);
    }
    result = result && !BitsAll(&bv, true);
    result = result && BitsAll(&bv, false);
    result = result && !BitsAny(&bv, true);
    result = result && BitsAny(&bv, false);
    result = result && BitsNone(&bv, true);
    result = result && !BitsNone(&bv, false);

    // Test with mixed values
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
    printf("Testing BitsLongestRun\n");

    Bits bv     = BitsInit();
    bool   result = true;

    // Test pattern: T T T F F T T F F F F
    bool pattern[] = {true, true, true, false, false, true, true, false, false, false, false};
    for (int i = 0; i < 11; i++) {
        BitsPush(&bv, pattern[i]);
    }

    // Longest run of true should be 3, longest run of false should be 4
    result = result && (BitsLongestRun(&bv, true) == 3);
    result = result && (BitsLongestRun(&bv, false) == 4);

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
    bool   result = true;

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
    bool   result = true;

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
    bool   result = true;

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

// Main function that runs all tests
int main(void) {
    printf("[INFO] Starting Bits.Access.Simple tests\n\n");

    // Array of test functions
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
        test_Bits_find_functions,
        test_Bits_predicate_functions,
        test_Bits_longest_run,
        test_Bits_find_edge_cases,
        test_Bits_predicate_edge_cases,
        test_Bits_longest_run_edge_cases
    };

    int total_tests = sizeof(tests) / sizeof(tests[0]);

    // Run all tests using the centralized test driver
    return run_test_suite(tests, total_tests, NULL, 0, "Bits.Access.Simple");
}

