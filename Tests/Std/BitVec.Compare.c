#include <Misra/Std/Container/BitVec.h>
#include <Misra/Std/Log.h>
#include <stdio.h>
#include <Misra/Types.h>

// Include test utilities
#include "../Util/TestRunner.h"

// Function prototypes
bool test_bitvec_equals(void);
bool test_bitvec_compare(void);
bool test_bitvec_lex_compare(void);
bool test_bitvec_numerical_compare(void);
bool test_bitvec_weight_compare(void);
bool test_bitvec_is_subset(void);
bool test_bitvec_overlaps(void);
bool test_bitvec_compare_edge_cases(void);
bool test_bitvec_set_operations_edge_cases(void);
bool test_bitvec_compare_null_failures(void);
bool test_bitvec_subset_null_failures(void);

// Test BitVecEquals function
bool test_bitvec_equals(void) {
    printf("Testing BitVecEquals\n");

    BitVec bv1 = BitVecInit();
    BitVec bv2 = BitVecInit();
    BitVec bv3 = BitVecInit();

    // Test equal empty bitvectors
    bool result = BitVecEquals(&bv1, &bv2);

    // Add same pattern to both
    BitVecPush(&bv1, true);
    BitVecPush(&bv1, false);
    BitVecPush(&bv1, true);

    BitVecPush(&bv2, true);
    BitVecPush(&bv2, false);
    BitVecPush(&bv2, true);

    // Should be equal
    result = result && BitVecEquals(&bv1, &bv2);

    // Add different pattern to third
    BitVecPush(&bv3, true);
    BitVecPush(&bv3, false);
    BitVecPush(&bv3, false); // Different bit

    // Should not be equal
    result = result && !BitVecEquals(&bv1, &bv3);

    // Test different lengths
    BitVecPush(&bv3, true);
    result = result && !BitVecEquals(&bv1, &bv3);

    // Clean up
    BitVecDeinit(&bv1);
    BitVecDeinit(&bv2);
    BitVecDeinit(&bv3);

    return result;
}

// Test BitVecCompare function
bool test_bitvec_compare(void) {
    printf("Testing BitVecCompare\n");

    BitVec bv1 = BitVecInit();
    BitVec bv2 = BitVecInit();

    // Test equal bitvectors
    BitVecPush(&bv1, true);
    BitVecPush(&bv1, false);

    BitVecPush(&bv2, true);
    BitVecPush(&bv2, false);

    bool result = (BitVecCompare(&bv1, &bv2) == 0);

    // Test first greater than second
    BitVecClear(&bv1);
    BitVecClear(&bv2);

    BitVecPush(&bv1, true);
    BitVecPush(&bv1, true);  // 11

    BitVecPush(&bv2, true);
    BitVecPush(&bv2, false); // 10

    result = result && (BitVecCompare(&bv1, &bv2) > 0);
    result = result && (BitVecCompare(&bv2, &bv1) < 0);

    // Clean up
    BitVecDeinit(&bv1);
    BitVecDeinit(&bv2);

    return result;
}

// Test BitVecLexCompare function
bool test_bitvec_lex_compare(void) {
    printf("Testing BitVecLexCompare\n");

    BitVec bv1 = BitVecInit();
    BitVec bv2 = BitVecInit();

    // Test lexicographic comparison
    BitVecPush(&bv1, true);
    BitVecPush(&bv1, false);
    BitVecPush(&bv1, true); // 101

    BitVecPush(&bv2, true);
    BitVecPush(&bv2, true); // 11 (shorter)

    // Lexicographic comparison considers position-by-position
    int  cmp_result = BitVecLexCompare(&bv1, &bv2);
    bool result     = (cmp_result != 0); // Should not be equal

    // Test equal bitvectors
    BitVecClear(&bv2);
    BitVecPush(&bv2, true);
    BitVecPush(&bv2, false);
    BitVecPush(&bv2, true); // 101

    result = result && (BitVecLexCompare(&bv1, &bv2) == 0);

    // Clean up
    BitVecDeinit(&bv1);
    BitVecDeinit(&bv2);

    return result;
}

// Test BitVecNumericalCompare function
bool test_bitvec_numerical_compare(void) {
    printf("Testing BitVecNumericalCompare\n");

    BitVec bv1 = BitVecInit();
    BitVec bv2 = BitVecInit();

    // Create bitvectors representing different numbers
    // bv1: 101 (binary) = 5 (decimal)
    BitVecPush(&bv1, true);
    BitVecPush(&bv1, false);
    BitVecPush(&bv1, true);

    // bv2: 11 (binary) = 3 (decimal)
    BitVecPush(&bv2, true);
    BitVecPush(&bv2, true);

    // Numerical comparison should compare the integer values
    int  cmp_result = BitVecNumericalCompare(&bv1, &bv2);
    bool result     = (cmp_result > 0); // 5 > 3

    // Test equal values
    BitVecClear(&bv2);
    BitVecPush(&bv2, true);
    BitVecPush(&bv2, false);
    BitVecPush(&bv2, true); // Also 101

    result = result && (BitVecNumericalCompare(&bv1, &bv2) == 0);

    // Clean up
    BitVecDeinit(&bv1);
    BitVecDeinit(&bv2);

    return result;
}

// Test BitVecWeightCompare function
bool test_bitvec_weight_compare(void) {
    printf("Testing BitVecWeightCompare\n");

    BitVec bv1 = BitVecInit();
    BitVec bv2 = BitVecInit();

    // bv1: 111 (3 ones)
    BitVecPush(&bv1, true);
    BitVecPush(&bv1, true);
    BitVecPush(&bv1, true);

    // bv2: 101 (2 ones)
    BitVecPush(&bv2, true);
    BitVecPush(&bv2, false);
    BitVecPush(&bv2, true);

    // Weight comparison should compare number of set bits
    int  cmp_result = BitVecWeightCompare(&bv1, &bv2);
    bool result     = (cmp_result > 0); // 3 ones > 2 ones

    // Test equal weights
    BitVecClear(&bv2);
    BitVecPush(&bv2, true);
    BitVecPush(&bv2, true);
    BitVecPush(&bv2, true); // Also 3 ones

    result = result && (BitVecWeightCompare(&bv1, &bv2) == 0);

    // Clean up
    BitVecDeinit(&bv1);
    BitVecDeinit(&bv2);

    return result;
}

// Test BitVecIsSubset function
bool test_bitvec_is_subset(void) {
    printf("Testing BitVecIsSubset\n");

    BitVec subset   = BitVecInit();
    BitVec superset = BitVecInit();

    // Create superset: 1111
    BitVecPush(&superset, true);
    BitVecPush(&superset, true);
    BitVecPush(&superset, true);
    BitVecPush(&superset, true);

    // Create subset: 1010
    BitVecPush(&subset, true);
    BitVecPush(&subset, false);
    BitVecPush(&subset, true);
    BitVecPush(&subset, false);

    // subset should be a subset of superset (all 1s in subset are also 1s in superset)
    bool result = BitVecIsSubset(&subset, &superset);

    // Test non-subset case
    BitVecSet(&superset, 2, false); // Change superset to 1101
    // Now subset (1010) is not a subset of superset (1101) because subset has 1 at position 2
    result = result && !BitVecIsSubset(&subset, &superset);

    // Test equal sets (should be subset)
    BitVecClear(&superset);
    BitVecPush(&superset, true);
    BitVecPush(&superset, false);
    BitVecPush(&superset, true);
    BitVecPush(&superset, false);

    result = result && BitVecIsSubset(&subset, &superset);

    // Clean up
    BitVecDeinit(&subset);
    BitVecDeinit(&superset);

    return result;
}

// Test BitVecOverlaps function
bool test_bitvec_overlaps(void) {
    printf("Testing BitVecOverlaps\n");

    BitVec bv1 = BitVecInit();
    BitVec bv2 = BitVecInit();

    // Create overlapping bitvectors
    // bv1: 1010
    BitVecPush(&bv1, true);
    BitVecPush(&bv1, false);
    BitVecPush(&bv1, true);
    BitVecPush(&bv1, false);

    // bv2: 1100
    BitVecPush(&bv2, true);
    BitVecPush(&bv2, true);
    BitVecPush(&bv2, false);
    BitVecPush(&bv2, false);

    // They overlap at position 0 (both have 1)
    bool result = BitVecOverlaps(&bv1, &bv2);

    // Test non-overlapping bitvectors
    BitVecClear(&bv2);
    // bv2: 0101 (complement of bv1)
    BitVecPush(&bv2, false);
    BitVecPush(&bv2, true);
    BitVecPush(&bv2, false);
    BitVecPush(&bv2, true);

    // They should not overlap (no position where both have 1)
    result = result && !BitVecOverlaps(&bv1, &bv2);

    // Clean up
    BitVecDeinit(&bv1);
    BitVecDeinit(&bv2);

    return result;
}

// Edge case tests
bool test_bitvec_compare_edge_cases(void) {
    printf("Testing BitVec compare edge cases\n");

    BitVec bv1    = BitVecInit();
    BitVec bv2    = BitVecInit();
    bool   result = true;

    // Test compare empty bitvecs
    result = result && BitVecEquals(&bv1, &bv2);
    result = result && (BitVecCompare(&bv1, &bv2) == 0);

    // Test compare empty vs non-empty
    BitVecPush(&bv1, true);
    result = result && !BitVecEquals(&bv1, &bv2);
    result = result && (BitVecCompare(&bv1, &bv2) != 0);

    // Test large identical bitvecs
    BitVecClear(&bv1);
    BitVecClear(&bv2);
    for (int i = 0; i < 1000; i++) {
        bool bit = i % 3 == 0;
        BitVecPush(&bv1, bit);
        BitVecPush(&bv2, bit);
    }
    result = result && BitVecEquals(&bv1, &bv2);

    // Test subset operations on empty sets
    BitVecClear(&bv1);
    BitVecClear(&bv2);
    result = result && BitVecIsSubset(&bv1, &bv2); // Empty is subset of empty

    BitVecDeinit(&bv1);
    BitVecDeinit(&bv2);
    return result;
}

bool test_bitvec_set_operations_edge_cases(void) {
    printf("Testing BitVec set operations edge cases\n");

    BitVec bv1    = BitVecInit();
    BitVec bv2    = BitVecInit();
    bool   result = true;

    // Test with empty sets
    result = result && !BitVecOverlaps(&bv1, &bv2);
    result = result && BitVecDisjoint(&bv1, &bv2);

    // Test single bit sets
    BitVecPush(&bv1, true);
    BitVecPush(&bv2, false);
    result = result && !BitVecIntersects(&bv1, &bv2);

    // Test large sets
    BitVecClear(&bv1);
    BitVecClear(&bv2);
    for (int i = 0; i < 1000; i++) {
        BitVecPush(&bv1, i % 2 == 0);
        BitVecPush(&bv2, i % 3 == 0);
    }
    // Should have some overlap since both contain position 0 (true)
    result = result && BitVecIntersects(&bv1, &bv2);

    BitVecDeinit(&bv1);
    BitVecDeinit(&bv2);
    return result;
}

// Deadend tests
bool test_bitvec_compare_null_failures(void) {
    printf("Testing BitVec compare NULL pointer handling\n");

    BitVec bv = BitVecInit();

    // Test NULL pointer - should abort
    BitVecEquals(NULL, &bv);

    BitVecDeinit(&bv);
    return false;
}

bool test_bitvec_subset_null_failures(void) {
    printf("Testing BitVec subset NULL handling\n");

    // Test NULL pointer - should abort
    BitVecIsSubset(NULL, NULL);

    return false;
}

// Main function that runs all tests
int main(void) {
    printf("[INFO] Starting BitVec.Compare tests\n\n");

    // Array of normal test functions
    TestFunction tests[] = {
        test_bitvec_equals,
        test_bitvec_compare,
        test_bitvec_lex_compare,
        test_bitvec_numerical_compare,
        test_bitvec_weight_compare,
        test_bitvec_is_subset,
        test_bitvec_overlaps,
        test_bitvec_compare_edge_cases,
        test_bitvec_set_operations_edge_cases
    };

    // Array of deadend test functions
    TestFunction deadend_tests[] = {test_bitvec_compare_null_failures, test_bitvec_subset_null_failures};

    int total_tests         = sizeof(tests) / sizeof(tests[0]);
    int total_deadend_tests = sizeof(deadend_tests) / sizeof(deadend_tests[0]);

    // Run all tests using the centralized test driver
    return run_test_suite(tests, total_tests, deadend_tests, total_deadend_tests, "BitVec.Compare");
}