#include <Misra/Std/Container/Bits.h>
#include <Misra/Std/Log.h>
#include <stdio.h>
#include <Misra/Types.h>

// Include test utilities
#include "../Util/TestRunner.h"

// Function prototypes
bool test_Bits_equals(void);
bool test_Bits_compare(void);
bool test_Bits_lex_compare(void);
bool test_Bits_numerical_compare(void);
bool test_Bits_weight_compare(void);
bool test_Bits_signed_compare(void);
bool test_Bits_is_subset(void);
bool test_Bits_is_superset(void);
bool test_Bits_overlaps(void);
bool test_Bits_disjoint_intersects(void);
bool test_Bits_equals_range(void);
bool test_Bits_compare_range(void);
bool test_Bits_less_than_functions(void);
bool test_Bits_is_sorted(void);
bool test_Bits_compare_edge_cases(void);
bool test_Bits_set_operations_edge_cases(void);
bool test_Bits_comprehensive_comparison(void);
bool test_Bits_large_scale_comparison(void);
bool test_Bits_compare_null_failures(void);
bool test_Bits_subset_null_failures(void);
bool test_Bits_range_null_failures(void);
bool test_Bits_range_bounds_failures(void);
bool test_Bits_sorted_null_failures(void);

// Test BitsEquals function
bool test_Bits_equals(void) {
    printf("Testing BitsEquals\n");

    Bits bv1 = BitsInit();
    Bits bv2 = BitsInit();
    Bits bv3 = BitsInit();

    // Test equal empty Bitstors
    bool result = BitsEquals(&bv1, &bv2);

    // Add same pattern to both
    BitsPush(&bv1, true);
    BitsPush(&bv1, false);
    BitsPush(&bv1, true);

    BitsPush(&bv2, true);
    BitsPush(&bv2, false);
    BitsPush(&bv2, true);

    // Should be equal
    result = result && BitsEquals(&bv1, &bv2);

    // Add different pattern to third
    BitsPush(&bv3, true);
    BitsPush(&bv3, false);
    BitsPush(&bv3, false); // Different bit

    // Should not be equal
    result = result && !BitsEquals(&bv1, &bv3);

    // Test different lengths
    BitsPush(&bv3, true);
    result = result && !BitsEquals(&bv1, &bv3);

    // Clean up
    BitsDeinit(&bv1);
    BitsDeinit(&bv2);
    BitsDeinit(&bv3);

    return result;
}

// Test BitsCompare function
bool test_Bits_compare(void) {
    printf("Testing BitsCompare\n");

    Bits bv1 = BitsInit();
    Bits bv2 = BitsInit();

    // Test equal Bitstors
    BitsPush(&bv1, true);
    BitsPush(&bv1, false);

    BitsPush(&bv2, true);
    BitsPush(&bv2, false);

    bool result = (BitsCompare(&bv1, &bv2) == 0);

    // Test first greater than second
    BitsClear(&bv1);
    BitsClear(&bv2);

    BitsPush(&bv1, true);
    BitsPush(&bv1, true);  // 11

    BitsPush(&bv2, true);
    BitsPush(&bv2, false); // 10

    result = result && (BitsCompare(&bv1, &bv2) > 0);
    result = result && (BitsCompare(&bv2, &bv1) < 0);

    // Clean up
    BitsDeinit(&bv1);
    BitsDeinit(&bv2);

    return result;
}

// Test BitsLexCompare function
bool test_Bits_lex_compare(void) {
    printf("Testing BitsLexCompare\n");

    Bits bv1 = BitsInit();
    Bits bv2 = BitsInit();

    // Test lexicographic comparison
    BitsPush(&bv1, true);
    BitsPush(&bv1, false);
    BitsPush(&bv1, true); // 101

    BitsPush(&bv2, true);
    BitsPush(&bv2, true); // 11 (shorter)

    // Lexicographic comparison considers position-by-position
    int  cmp_result = BitsLexCompare(&bv1, &bv2);
    bool result     = (cmp_result != 0); // Should not be equal

    // Test equal Bitstors
    BitsClear(&bv2);
    BitsPush(&bv2, true);
    BitsPush(&bv2, false);
    BitsPush(&bv2, true); // 101

    result = result && (BitsLexCompare(&bv1, &bv2) == 0);

    // Clean up
    BitsDeinit(&bv1);
    BitsDeinit(&bv2);

    return result;
}

// Test BitsNumericalCompare function
bool test_Bits_numerical_compare(void) {
    printf("Testing BitsNumericalCompare\n");

    Bits bv1 = BitsInit();
    Bits bv2 = BitsInit();

    // Create Bitstors representing different numbers
    // bv1: 101 (binary) = 5 (decimal)
    BitsPush(&bv1, true);
    BitsPush(&bv1, false);
    BitsPush(&bv1, true);

    // bv2: 11 (binary) = 3 (decimal)
    BitsPush(&bv2, true);
    BitsPush(&bv2, true);

    // Numerical comparison should compare the integer values
    int  cmp_result = BitsNumericalCompare(&bv1, &bv2);
    bool result     = (cmp_result > 0); // 5 > 3

    // Test equal values
    BitsClear(&bv2);
    BitsPush(&bv2, true);
    BitsPush(&bv2, false);
    BitsPush(&bv2, true); // Also 101

    result = result && (BitsNumericalCompare(&bv1, &bv2) == 0);

    // Clean up
    BitsDeinit(&bv1);
    BitsDeinit(&bv2);

    return result;
}

// Test BitsWeightCompare function
bool test_Bits_weight_compare(void) {
    printf("Testing BitsWeightCompare\n");

    Bits bv1 = BitsInit();
    Bits bv2 = BitsInit();

    // bv1: 111 (3 ones)
    BitsPush(&bv1, true);
    BitsPush(&bv1, true);
    BitsPush(&bv1, true);

    // bv2: 101 (2 ones)
    BitsPush(&bv2, true);
    BitsPush(&bv2, false);
    BitsPush(&bv2, true);

    // Weight comparison should compare number of set bits
    int  cmp_result = BitsWeightCompare(&bv1, &bv2);
    bool result     = (cmp_result > 0); // 3 ones > 2 ones

    // Test equal weights
    BitsClear(&bv2);
    BitsPush(&bv2, true);
    BitsPush(&bv2, true);
    BitsPush(&bv2, true); // Also 3 ones

    result = result && (BitsWeightCompare(&bv1, &bv2) == 0);

    // Clean up
    BitsDeinit(&bv1);
    BitsDeinit(&bv2);

    return result;
}

// Test BitsIsSubset function
bool test_Bits_is_subset(void) {
    printf("Testing BitsIsSubset\n");

    Bits subset   = BitsInit();
    Bits superset = BitsInit();

    // Create superset: 1111
    BitsPush(&superset, true);
    BitsPush(&superset, true);
    BitsPush(&superset, true);
    BitsPush(&superset, true);

    // Create subset: 1010
    BitsPush(&subset, true);
    BitsPush(&subset, false);
    BitsPush(&subset, true);
    BitsPush(&subset, false);

    // subset should be a subset of superset (all 1s in subset are also 1s in superset)
    bool result = BitsIsSubset(&subset, &superset);

    // Test non-subset case
    BitsSet(&superset, 2, false); // Change superset to 1101
    // Now subset (1010) is not a subset of superset (1101) because subset has 1 at position 2
    result = result && !BitsIsSubset(&subset, &superset);

    // Test equal sets (should be subset)
    BitsClear(&superset);
    BitsPush(&superset, true);
    BitsPush(&superset, false);
    BitsPush(&superset, true);
    BitsPush(&superset, false);

    result = result && BitsIsSubset(&subset, &superset);

    // Clean up
    BitsDeinit(&subset);
    BitsDeinit(&superset);

    return result;
}

// Test BitsSignedCompare function
bool test_Bits_signed_compare(void) {
    printf("Testing BitsSignedCompare\n");

    Bits bv1 = BitsInit();
    Bits bv2 = BitsInit();

    // Test positive vs negative (MSB is sign bit)
    // bv1: 011 (positive 3)
    BitsPush(&bv1, true);
    BitsPush(&bv1, true);
    BitsPush(&bv1, false);

    // bv2: 111 (negative, MSB=1)
    BitsPush(&bv2, true);
    BitsPush(&bv2, true);
    BitsPush(&bv2, true);

    // Positive should be greater than negative
    bool result = (BitsSignedCompare(&bv1, &bv2) > 0);

    // Test two positives
    BitsClear(&bv2);
    // bv2: 001 (positive 1)
    BitsPush(&bv2, true);
    BitsPush(&bv2, false);
    BitsPush(&bv2, false);

    // 3 > 1
    result = result && (BitsSignedCompare(&bv1, &bv2) > 0);

    // Test equal signed values
    Bits bv3 = BitsClone(&bv1);
    result   = result && (BitsSignedCompare(&bv1, &bv3) == 0);

    // Clean up
    BitsDeinit(&bv1);
    BitsDeinit(&bv2);
    BitsDeinit(&bv3);

    return result;
}

// Test BitsIsSuperset function
bool test_Bits_is_superset(void) {
    printf("Testing BitsIsSuperset\n");

    Bits superset = BitsInit();
    Bits subset   = BitsInit();

    // Create superset: 1111
    BitsPush(&superset, true);
    BitsPush(&superset, true);
    BitsPush(&superset, true);
    BitsPush(&superset, true);

    // Create subset: 1010
    BitsPush(&subset, true);
    BitsPush(&subset, false);
    BitsPush(&subset, true);
    BitsPush(&subset, false);

    // superset should be a superset of subset
    bool result = BitsIsSuperset(&superset, &subset);

    // Test non-superset case
    BitsSet(&superset, 2, false); // Change to 1101
    // Now superset (1101) is not a superset of subset (1010)
    result = result && !BitsIsSuperset(&superset, &subset);

    // Test equal sets (should be superset)
    BitsClear(&superset);
    BitsPush(&superset, true);
    BitsPush(&superset, false);
    BitsPush(&superset, true);
    BitsPush(&superset, false);

    result = result && BitsIsSuperset(&superset, &subset);

    // Clean up
    BitsDeinit(&superset);
    BitsDeinit(&subset);

    return result;
}

// Test BitsOverlaps function
bool test_Bits_overlaps(void) {
    printf("Testing BitsOverlaps\n");

    Bits bv1 = BitsInit();
    Bits bv2 = BitsInit();

    // Create overlapping Bitstors
    // bv1: 1010
    BitsPush(&bv1, true);
    BitsPush(&bv1, false);
    BitsPush(&bv1, true);
    BitsPush(&bv1, false);

    // bv2: 1100
    BitsPush(&bv2, true);
    BitsPush(&bv2, true);
    BitsPush(&bv2, false);
    BitsPush(&bv2, false);

    // They overlap at position 0 (both have 1)
    bool result = BitsOverlaps(&bv1, &bv2);

    // Test non-overlapping Bitstors
    BitsClear(&bv2);
    // bv2: 0101 (complement of bv1)
    BitsPush(&bv2, false);
    BitsPush(&bv2, true);
    BitsPush(&bv2, false);
    BitsPush(&bv2, true);

    // They should not overlap (no position where both have 1)
    result = result && !BitsOverlaps(&bv1, &bv2);

    // Clean up
    BitsDeinit(&bv1);
    BitsDeinit(&bv2);

    return result;
}

// Test BitsDisjoint and BitsIntersects functions
bool test_Bits_disjoint_intersects(void) {
    printf("Testing BitsDisjoint and BitsIntersects\n");

    Bits bv1 = BitsInit();
    Bits bv2 = BitsInit();

    // Create disjoint Bitstors
    // bv1: 1010
    BitsPush(&bv1, true);
    BitsPush(&bv1, false);
    BitsPush(&bv1, true);
    BitsPush(&bv1, false);

    // bv2: 0101 (disjoint with bv1)
    BitsPush(&bv2, false);
    BitsPush(&bv2, true);
    BitsPush(&bv2, false);
    BitsPush(&bv2, true);

    // Should be disjoint and not intersect
    bool result = BitsDisjoint(&bv1, &bv2);
    result      = result && !BitsIntersects(&bv1, &bv2);

    // Create intersecting Bitstors
    BitsSet(&bv2, 0, true); // Change bv2 to 1101

    // Should not be disjoint and should intersect
    result = result && !BitsDisjoint(&bv1, &bv2);
    result = result && BitsIntersects(&bv1, &bv2);

    // Test with empty Bitstors
    BitsClear(&bv1);
    BitsClear(&bv2);
    result = result && BitsDisjoint(&bv1, &bv2);
    result = result && !BitsIntersects(&bv1, &bv2);

    // Clean up
    BitsDeinit(&bv1);
    BitsDeinit(&bv2);

    return result;
}

// Test BitsEqualsRange function
bool test_Bits_equals_range(void) {
    printf("Testing BitsEqualsRange\n");

    Bits bv1 = BitsInit();
    Bits bv2 = BitsInit();

    // Create test patterns
    // bv1: 11100111
    for (int i = 0; i < 8; i++) {
        BitsPush(&bv1, (i >= 3 && i <= 5) ? false : true);
    }

    // bv2: 00100100
    for (int i = 0; i < 8; i++) {
        BitsPush(&bv2, (i == 2 || i == 5) ? true : false);
    }

    // Test unequal ranges
    // bv1[3:5] = 000, bv2[1:3] = 010, should be false
    bool result = !BitsEqualsRange(&bv1, 3, &bv2, 1, 3);

    // Test equal ranges that actually match
    result = result && BitsEqualsRange(&bv1, 0, &bv1, 0, 8); // Self-equality

    // Test equal ranges with same pattern
    // bv1[3:5] = 000, bv2[3:5] = 001, should be false
    result = result && !BitsEqualsRange(&bv1, 3, &bv2, 3, 3);

    // Test actually equal ranges: bv1[3:4] = 00, bv2[0:1] = 00
    result = result && BitsEqualsRange(&bv1, 3, &bv2, 0, 2);

    // Test boundary conditions
    result = result && BitsEqualsRange(&bv1, 0, &bv2, 0, 0); // Zero length (should be true)

    // Clean up
    BitsDeinit(&bv1);
    BitsDeinit(&bv2);

    return result;
}

// Test BitsCompareRange function
bool test_Bits_compare_range(void) {
    printf("Testing BitsCompareRange\n");

    Bits bv1 = BitsInit();
    Bits bv2 = BitsInit();

    // Create test patterns
    // bv1: 11010110
    BitsPush(&bv1, true);
    BitsPush(&bv1, true);
    BitsPush(&bv1, false);
    BitsPush(&bv1, true);
    BitsPush(&bv1, false);
    BitsPush(&bv1, true);
    BitsPush(&bv1, true);
    BitsPush(&bv1, false);

    // bv2: 01101011
    BitsPush(&bv2, false);
    BitsPush(&bv2, true);
    BitsPush(&bv2, true);
    BitsPush(&bv2, false);
    BitsPush(&bv2, true);
    BitsPush(&bv2, false);
    BitsPush(&bv2, true);
    BitsPush(&bv2, true);

    // Test range comparisons
    int  cmp_result = BitsCompareRange(&bv1, 2, &bv2, 2, 3); // Compare 3-bit ranges
    bool result     = (cmp_result != 0);                     // Should not be equal

    // Test equal ranges
    cmp_result = BitsCompareRange(&bv1, 0, &bv1, 0, 8); // Self-comparison
    result     = result && (cmp_result == 0);

    // Test zero-length ranges
    cmp_result = BitsCompareRange(&bv1, 0, &bv2, 0, 0);
    result     = result && (cmp_result == 0); // Zero-length ranges are equal

    // Clean up
    BitsDeinit(&bv1);
    BitsDeinit(&bv2);

    return result;
}

// Test BitsIsLexicographicallyLess and BitsIsNumericallyLess
bool test_Bits_less_than_functions(void) {
    printf("Testing BitsIsLexicographicallyLess and BitsIsNumericallyLess\n");

    Bits bv1 = BitsInit();
    Bits bv2 = BitsInit();

    // Test lexicographic comparison
    // bv1: 10 (shorter)
    BitsPush(&bv1, true);
    BitsPush(&bv1, false);

    // bv2: 101 (longer)
    BitsPush(&bv2, true);
    BitsPush(&bv2, false);
    BitsPush(&bv2, true);

    // Lexicographically, shorter comes first
    bool result = BitsIsLexicographicallyLess(&bv1, &bv2);

    // Numerically, 10 (2) < 101 (5)
    result = result && BitsIsNumericallyLess(&bv1, &bv2);

    // Test equal cases
    Bits bv3 = BitsClone(&bv1);
    result   = result && !BitsIsLexicographicallyLess(&bv1, &bv3);
    result   = result && !BitsIsNumericallyLess(&bv1, &bv3);

    // Test reverse comparison
    result = result && !BitsIsLexicographicallyLess(&bv2, &bv1);
    result = result && !BitsIsNumericallyLess(&bv2, &bv1);

    // Clean up
    BitsDeinit(&bv1);
    BitsDeinit(&bv2);
    BitsDeinit(&bv3);

    return result;
}

// Test BitsIsSorted function
bool test_Bits_is_sorted(void) {
    printf("Testing BitsIsSorted\n");

    Bits bv = BitsInit();

    // Test empty Bitstor (should be sorted)
    bool result = BitsIsSorted(&bv);

    // Test sorted pattern: 0001111
    BitsPush(&bv, false);
    BitsPush(&bv, false);
    BitsPush(&bv, false);
    BitsPush(&bv, true);
    BitsPush(&bv, true);
    BitsPush(&bv, true);
    BitsPush(&bv, true);

    result = result && BitsIsSorted(&bv);

    // Test unsorted pattern (add 0 after 1s)
    BitsPush(&bv, false);
    result = result && !BitsIsSorted(&bv);

    // Test all zeros
    BitsClear(&bv);
    for (int i = 0; i < 5; i++) {
        BitsPush(&bv, false);
    }
    result = result && BitsIsSorted(&bv);

    // Test all ones
    BitsClear(&bv);
    for (int i = 0; i < 5; i++) {
        BitsPush(&bv, true);
    }
    result = result && BitsIsSorted(&bv);

    // Clean up
    BitsDeinit(&bv);

    return result;
}

// Edge case tests
bool test_Bits_compare_edge_cases(void) {
    printf("Testing Bits compare edge cases\n");

    Bits bv1    = BitsInit();
    Bits bv2    = BitsInit();
    bool result = true;

    // Test compare empty Bitss
    result = result && BitsEquals(&bv1, &bv2);
    result = result && (BitsCompare(&bv1, &bv2) == 0);

    // Test compare empty vs non-empty
    BitsPush(&bv1, true);
    result = result && !BitsEquals(&bv1, &bv2);
    result = result && (BitsCompare(&bv1, &bv2) != 0);

    // Test large identical Bitss
    BitsClear(&bv1);
    BitsClear(&bv2);
    for (int i = 0; i < 1000; i++) {
        bool bit = i % 3 == 0;
        BitsPush(&bv1, bit);
        BitsPush(&bv2, bit);
    }
    result = result && BitsEquals(&bv1, &bv2);

    // Test subset operations on empty sets
    BitsClear(&bv1);
    BitsClear(&bv2);
    result = result && BitsIsSubset(&bv1, &bv2); // Empty is subset of empty

    BitsDeinit(&bv1);
    BitsDeinit(&bv2);
    return result;
}

bool test_Bits_set_operations_edge_cases(void) {
    printf("Testing Bits set operations edge cases\n");

    Bits bv1    = BitsInit();
    Bits bv2    = BitsInit();
    bool result = true;

    // Test with empty sets
    result = result && !BitsOverlaps(&bv1, &bv2);
    result = result && BitsDisjoint(&bv1, &bv2);

    // Test single bit sets
    BitsPush(&bv1, true);
    BitsPush(&bv2, false);
    result = result && !BitsIntersects(&bv1, &bv2);

    // Test large sets
    BitsClear(&bv1);
    BitsClear(&bv2);
    for (int i = 0; i < 1000; i++) {
        BitsPush(&bv1, i % 2 == 0);
        BitsPush(&bv2, i % 3 == 0);
    }
    // Should have some overlap since both contain position 0 (true)
    result = result && BitsIntersects(&bv1, &bv2);

    BitsDeinit(&bv1);
    BitsDeinit(&bv2);
    return result;
}

// Comprehensive comparison testing with cross-validation
bool test_Bits_comprehensive_comparison(void) {
    printf("Testing Bits comprehensive comparison operations\n");

    Bits bv1    = BitsInit();
    Bits bv2    = BitsInit();
    bool result = true;

    // Test comparison consistency across all comparison types
    // bv1: 1010110 (decimal 86 when read as LSB-first)
    int pattern1[] = {1, 0, 1, 0, 1, 1, 0};
    for (int i = 0; i < 7; i++) {
        BitsPush(&bv1, pattern1[i]);
    }

    // bv2: 1100101 (decimal 89 when read as LSB-first)
    int pattern2[] = {1, 1, 0, 0, 1, 0, 1};
    for (int i = 0; i < 7; i++) {
        BitsPush(&bv2, pattern2[i]);
    }

    // Cross-validate different comparison methods
    // Numerical: 86 < 89, so bv1 < bv2
    result = result && (BitsNumericalCompare(&bv1, &bv2) < 0);
    result = result && BitsIsNumericallyLess(&bv1, &bv2);

    // Weight: bv1 has 4 ones, bv2 has 4 ones, so equal weight
    result = result && (BitsWeightCompare(&bv1, &bv2) == 0);

    // Lexicographic comparison
    int  lex_cmp  = BitsLexCompare(&bv1, &bv2);
    bool lex_less = BitsIsLexicographicallyLess(&bv1, &bv2);
    result        = result && ((lex_cmp < 0) == lex_less);

    // Test transitivity: if A < B and B < C, then A < C
    Bits bv3 = BitsInit();
    // bv3: larger than bv2
    for (int i = 0; i < 8; i++) {
        BitsPush(&bv3, true);
    }

    if (BitsNumericalCompare(&bv1, &bv2) < 0 && BitsNumericalCompare(&bv2, &bv3) < 0) {
        result = result && (BitsNumericalCompare(&bv1, &bv3) < 0);
    }

    // Test subset/superset consistency
    Bits subset   = BitsInit();
    Bits superset = BitsInit();

    // Create actual subset/superset relationship
    BitsPush(&subset, true);
    BitsPush(&subset, false);
    BitsPush(&subset, true);

    BitsPush(&superset, true);
    BitsPush(&superset, true);
    BitsPush(&superset, true);

    result = result && BitsIsSubset(&subset, &superset);
    result = result && BitsIsSuperset(&superset, &subset);

    // Clean up
    BitsDeinit(&bv1);
    BitsDeinit(&bv2);
    BitsDeinit(&bv3);
    BitsDeinit(&subset);
    BitsDeinit(&superset);

    return result;
}

// Large-scale testing with stress patterns
bool test_Bits_large_scale_comparison(void) {
    printf("Testing Bits large-scale comparison operations\n");

    Bits large1 = BitsInit();
    Bits large2 = BitsInit();
    bool result = true;

    // Create large Bitstors (2000 bits each)
    for (int i = 0; i < 2000; i++) {
        // Pattern 1: Fibonacci-like XOR pattern
        bool bit1 = (i % 3 == 0) ^ (i % 5 == 0);
        BitsPush(&large1, bit1);

        // Pattern 2: Prime-like pattern
        bool bit2 = (i % 7 == 0) || (i % 11 == 0);
        BitsPush(&large2, bit2);
    }

    // Test large-scale comparison performance and correctness
    result = result && (BitsEquals(&large1, &large1)); // Self-equality
    result = result && !BitsEquals(&large1, &large2);  // Different patterns

    // Test range operations on large vectors
    result = result && BitsEqualsRange(&large1, 100, &large1, 100, 500); // Self-range equality

    // Test set operations on large vectors
    bool overlaps = BitsOverlaps(&large1, &large2);
    bool disjoint = BitsDisjoint(&large1, &large2);
    result        = result && (overlaps != disjoint); // Should be opposite

    // Verify signed vs unsigned comparison differences
    Bits pos = BitsInit();
    Bits neg = BitsInit();

    // Positive number (MSB = 0): 01111111
    for (int i = 0; i < 7; i++) {
        BitsPush(&pos, true);
    }
    BitsPush(&pos, false);

    // Negative number (MSB = 1): 10000001
    BitsPush(&neg, true);
    for (int i = 1; i < 7; i++) {
        BitsPush(&neg, false);
    }
    BitsPush(&neg, true);

    // Unsigned: 01111111 (127) < 10000001 (129)
    result = result && (BitsNumericalCompare(&pos, &neg) < 0);

    // Signed: 01111111 (+127) > 10000001 (-127)
    result = result && (BitsSignedCompare(&pos, &neg) > 0);

    // Clean up
    BitsDeinit(&large1);
    BitsDeinit(&large2);
    BitsDeinit(&pos);
    BitsDeinit(&neg);

    return result;
}

// Deadend tests
bool test_Bits_compare_null_failures(void) {
    printf("Testing Bits compare NULL pointer handling\n");

    Bits bv = BitsInit();

    // Test NULL pointer - should abort
    BitsEquals(NULL, &bv);

    BitsDeinit(&bv);
    return false;
}

bool test_Bits_subset_null_failures(void) {
    printf("Testing Bits subset NULL handling\n");

    // Test NULL pointer - should abort
    BitsIsSubset(NULL, NULL);

    return false;
}

bool test_Bits_range_null_failures(void) {
    printf("Testing Bits range operations NULL handling\n");

    Bits bv = BitsInit();

    // Test NULL pointer in range operations - should abort
    BitsEqualsRange(NULL, 0, &bv, 0, 1);

    BitsDeinit(&bv);
    return false;
}

bool test_Bits_range_bounds_failures(void) {
    printf("Testing Bits range operations bounds checking\n");

    Bits bv1 = BitsInit();
    Bits bv2 = BitsInit();

    // Create small Bitstors
    for (int i = 0; i < 3; i++) {
        BitsPush(&bv1, i % 2 == 0);
        BitsPush(&bv2, i % 2 == 1);
    }

    // Test out-of-bounds range - should abort
    BitsEqualsRange(&bv1, 0, &bv2, 0, 5); // Range exceeds length

    BitsDeinit(&bv1);
    BitsDeinit(&bv2);
    return false;
}

bool test_Bits_sorted_null_failures(void) {
    printf("Testing Bits sorted operations NULL handling\n");

    // Test NULL pointer - should abort
    BitsIsSorted(NULL);

    return false;
}

// Main function that runs all tests
int main(void) {
    printf("[INFO] Starting Bits.Compare tests\n\n");

    // Array of normal test functions
    TestFunction tests[] = {
        test_Bits_equals,
        test_Bits_compare,
        test_Bits_lex_compare,
        test_Bits_numerical_compare,
        test_Bits_weight_compare,
        test_Bits_signed_compare,
        test_Bits_is_subset,
        test_Bits_is_superset,
        test_Bits_overlaps,
        test_Bits_disjoint_intersects,
        test_Bits_equals_range,
        test_Bits_compare_range,
        test_Bits_less_than_functions,
        test_Bits_is_sorted,
        test_Bits_compare_edge_cases,
        test_Bits_set_operations_edge_cases,
        test_Bits_comprehensive_comparison,
        test_Bits_large_scale_comparison
    };

    // Array of deadend test functions
    TestFunction deadend_tests[] = {
        test_Bits_compare_null_failures,
        test_Bits_subset_null_failures,
        test_Bits_range_null_failures,
        test_Bits_range_bounds_failures,
        test_Bits_sorted_null_failures
    };

    int total_tests         = sizeof(tests) / sizeof(tests[0]);
    int total_deadend_tests = sizeof(deadend_tests) / sizeof(deadend_tests[0]);

    // Run all tests using the centralized test driver
    return run_test_suite(tests, total_tests, deadend_tests, total_deadend_tests, "Bits.Compare");
}
