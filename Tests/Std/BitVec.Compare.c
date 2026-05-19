#include <Misra/Std/Container/BitVec.h>
#include <Misra/Std/Allocator/Default.h>
#include <Misra/Std/Log.h>
#include <Misra/Types.h>

// Include test utilities
#include "../Util/TestRunner.h"

// Function prototypes
bool test_bitvec_equals(void);
bool test_bitvec_compare(void);
bool test_bitvec_lex_compare(void);
bool test_bitvec_numerical_compare(void);
bool test_bitvec_weight_compare(void);
bool test_bitvec_signed_compare(void);
bool test_bitvec_is_subset(void);
bool test_bitvec_is_superset(void);
bool test_bitvec_overlaps(void);
bool test_bitvec_disjoint_intersects(void);
bool test_bitvec_equals_range(void);
bool test_bitvec_compare_range(void);
bool test_bitvec_less_than_functions(void);
bool test_bitvec_is_sorted(void);
bool test_bitvec_compare_edge_cases(void);
bool test_bitvec_set_operations_edge_cases(void);
bool test_bitvec_comprehensive_comparison(void);
bool test_bitvec_large_scale_comparison(void);
bool test_bitvec_compare_null_failures(void);
bool test_bitvec_subset_null_failures(void);
bool test_bitvec_range_null_failures(void);
bool test_bitvec_range_bounds_failures(void);
bool test_bitvec_sorted_null_failures(void);

// Test BitVecEquals function
bool test_bitvec_equals(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVecEquals\n");

    BitVec bv1 = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVec bv2 = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVec bv3 = BitVecInit(ALLOCATOR_OF(&alloc));

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

    DefaultAllocatorDeinit(&alloc);

    return result;
}

// Test BitVecCompare function
bool test_bitvec_compare(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVecCompare\n");

    BitVec bv1 = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVec bv2 = BitVecInit(ALLOCATOR_OF(&alloc));

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

    DefaultAllocatorDeinit(&alloc);

    return result;
}

// Test BitVecLexCompare function
bool test_bitvec_lex_compare(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVecLexCompare\n");

    BitVec bv1 = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVec bv2 = BitVecInit(ALLOCATOR_OF(&alloc));

    // Test lexicographic comparison
    BitVecPush(&bv1, true);
    BitVecPush(&bv1, false);
    BitVecPush(&bv1, true); // 101

    BitVecPush(&bv2, true);
    BitVecPush(&bv2, true); // 11 (shorter)

    // Lexicographic comparison considers position-by-position
    int  cmp_result = BitVecCompare(&bv1, &bv2);
    bool result     = (cmp_result != 0); // Should not be equal

    // Test equal bitvectors
    BitVecClear(&bv2);
    BitVecPush(&bv2, true);
    BitVecPush(&bv2, false);
    BitVecPush(&bv2, true); // 101

    result = result && (BitVecCompare(&bv1, &bv2) == 0);

    // Clean up
    BitVecDeinit(&bv1);
    BitVecDeinit(&bv2);

    DefaultAllocatorDeinit(&alloc);

    return result;
}

// Test BitVecNumericalCompare function
bool test_bitvec_numerical_compare(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVecNumericalCompare\n");

    BitVec bv1 = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVec bv2 = BitVecInit(ALLOCATOR_OF(&alloc));

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

    DefaultAllocatorDeinit(&alloc);

    return result;
}

// Test BitVecWeightCompare function
bool test_bitvec_weight_compare(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVecWeightCompare\n");

    BitVec bv1 = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVec bv2 = BitVecInit(ALLOCATOR_OF(&alloc));

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

    DefaultAllocatorDeinit(&alloc);

    return result;
}

// Test BitVecIsSubset function
bool test_bitvec_is_subset(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVecIsSubset\n");

    BitVec subset   = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVec superset = BitVecInit(ALLOCATOR_OF(&alloc));

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

    DefaultAllocatorDeinit(&alloc);

    return result;
}

// Test BitVecSignedCompare function
bool test_bitvec_signed_compare(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVecSignedCompare\n");

    BitVec bv1 = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVec bv2 = BitVecInit(ALLOCATOR_OF(&alloc));

    // Test positive vs negative (MSB is sign bit)
    // bv1: 011 (positive 3)
    BitVecPush(&bv1, true);
    BitVecPush(&bv1, true);
    BitVecPush(&bv1, false);

    // bv2: 111 (negative, MSB=1)
    BitVecPush(&bv2, true);
    BitVecPush(&bv2, true);
    BitVecPush(&bv2, true);

    // Positive should be greater than negative
    bool result = (BitVecSignedCompare(&bv1, &bv2) > 0);

    // Test two positives
    BitVecClear(&bv2);
    // bv2: 001 (positive 1)
    BitVecPush(&bv2, true);
    BitVecPush(&bv2, false);
    BitVecPush(&bv2, false);

    // 3 > 1
    result = result && (BitVecSignedCompare(&bv1, &bv2) > 0);

    // Test equal signed values
    BitVec bv3 = BitVecClone(&bv1);
    result     = result && (BitVecSignedCompare(&bv1, &bv3) == 0);

    // Clean up
    BitVecDeinit(&bv1);
    BitVecDeinit(&bv2);
    BitVecDeinit(&bv3);

    DefaultAllocatorDeinit(&alloc);

    return result;
}

// Test BitVecIsSuperset function
bool test_bitvec_is_superset(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVecIsSuperset\n");

    BitVec superset = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVec subset   = BitVecInit(ALLOCATOR_OF(&alloc));

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

    // superset should be a superset of subset
    bool result = BitVecIsSuperset(&superset, &subset);

    // Test non-superset case
    BitVecSet(&superset, 2, false); // Change to 1101
    // Now superset (1101) is not a superset of subset (1010)
    result = result && !BitVecIsSuperset(&superset, &subset);

    // Test equal sets (should be superset)
    BitVecClear(&superset);
    BitVecPush(&superset, true);
    BitVecPush(&superset, false);
    BitVecPush(&superset, true);
    BitVecPush(&superset, false);

    result = result && BitVecIsSuperset(&superset, &subset);

    // Clean up
    BitVecDeinit(&superset);
    BitVecDeinit(&subset);

    DefaultAllocatorDeinit(&alloc);

    return result;
}

// Test BitVecOverlaps function
bool test_bitvec_overlaps(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVecOverlaps\n");

    BitVec bv1 = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVec bv2 = BitVecInit(ALLOCATOR_OF(&alloc));

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

    DefaultAllocatorDeinit(&alloc);

    return result;
}

// Test BitVecDisjoint and BitVecIntersects functions
bool test_bitvec_disjoint_intersects(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVecDisjoint and BitVecIntersects\n");

    BitVec bv1 = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVec bv2 = BitVecInit(ALLOCATOR_OF(&alloc));

    // Create disjoint bitvectors
    // bv1: 1010
    BitVecPush(&bv1, true);
    BitVecPush(&bv1, false);
    BitVecPush(&bv1, true);
    BitVecPush(&bv1, false);

    // bv2: 0101 (disjoint with bv1)
    BitVecPush(&bv2, false);
    BitVecPush(&bv2, true);
    BitVecPush(&bv2, false);
    BitVecPush(&bv2, true);

    // Should be disjoint and not intersect
    bool result = BitVecDisjoint(&bv1, &bv2);
    result      = result && !BitVecOverlaps(&bv1, &bv2);

    // Create intersecting bitvectors
    BitVecSet(&bv2, 0, true); // Change bv2 to 1101

    // Should not be disjoint and should intersect
    result = result && !BitVecDisjoint(&bv1, &bv2);
    result = result && BitVecOverlaps(&bv1, &bv2);

    // Test with empty bitvectors
    BitVecClear(&bv1);
    BitVecClear(&bv2);
    result = result && BitVecDisjoint(&bv1, &bv2);
    result = result && !BitVecOverlaps(&bv1, &bv2);

    // Clean up
    BitVecDeinit(&bv1);
    BitVecDeinit(&bv2);

    DefaultAllocatorDeinit(&alloc);

    return result;
}

// Test BitVecEqualsRange function
bool test_bitvec_equals_range(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVecEqualsRange\n");

    BitVec bv1 = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVec bv2 = BitVecInit(ALLOCATOR_OF(&alloc));

    // Create test patterns
    // bv1: 11100111
    for (int i = 0; i < 8; i++) {
        BitVecPush(&bv1, (i >= 3 && i <= 5) ? false : true);
    }

    // bv2: 00100100
    for (int i = 0; i < 8; i++) {
        BitVecPush(&bv2, (i == 2 || i == 5) ? true : false);
    }

    // Test unequal ranges
    // bv1[3:5] = 000, bv2[1:3] = 010, should be false
    bool result = !BitVecEqualsRange(&bv1, 3, &bv2, 1, 3);

    // Test equal ranges that actually match
    result = result && BitVecEqualsRange(&bv1, 0, &bv1, 0, 8); // Self-equality

    // Test equal ranges with same pattern
    // bv1[3:5] = 000, bv2[3:5] = 001, should be false
    result = result && !BitVecEqualsRange(&bv1, 3, &bv2, 3, 3);

    // Test actually equal ranges: bv1[3:4] = 00, bv2[0:1] = 00
    result = result && BitVecEqualsRange(&bv1, 3, &bv2, 0, 2);

    // Test boundary conditions
    result = result && BitVecEqualsRange(&bv1, 0, &bv2, 0, 0); // Zero length (should be true)

    // Clean up
    BitVecDeinit(&bv1);
    BitVecDeinit(&bv2);

    DefaultAllocatorDeinit(&alloc);

    return result;
}

// Test BitVecCompareRange function
bool test_bitvec_compare_range(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVecCompareRange\n");

    BitVec bv1 = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVec bv2 = BitVecInit(ALLOCATOR_OF(&alloc));

    // Create test patterns
    // bv1: 11010110
    BitVecPush(&bv1, true);
    BitVecPush(&bv1, true);
    BitVecPush(&bv1, false);
    BitVecPush(&bv1, true);
    BitVecPush(&bv1, false);
    BitVecPush(&bv1, true);
    BitVecPush(&bv1, true);
    BitVecPush(&bv1, false);

    // bv2: 01101011
    BitVecPush(&bv2, false);
    BitVecPush(&bv2, true);
    BitVecPush(&bv2, true);
    BitVecPush(&bv2, false);
    BitVecPush(&bv2, true);
    BitVecPush(&bv2, false);
    BitVecPush(&bv2, true);
    BitVecPush(&bv2, true);

    // Test range comparisons
    int  cmp_result = BitVecCompareRange(&bv1, 2, &bv2, 2, 3); // Compare 3-bit ranges
    bool result     = (cmp_result != 0);                       // Should not be equal

    // Test equal ranges
    cmp_result = BitVecCompareRange(&bv1, 0, &bv1, 0, 8); // Self-comparison
    result     = result && (cmp_result == 0);

    // Test zero-length ranges
    cmp_result = BitVecCompareRange(&bv1, 0, &bv2, 0, 0);
    result     = result && (cmp_result == 0); // Zero-length ranges are equal

    // Clean up
    BitVecDeinit(&bv1);
    BitVecDeinit(&bv2);

    DefaultAllocatorDeinit(&alloc);

    return result;
}

// Test BitVecIsLexicographicallyLess and BitVecIsNumericallyLess
bool test_bitvec_less_than_functions(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVecIsLexicographicallyLess and BitVecIsNumericallyLess\n");

    BitVec bv1 = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVec bv2 = BitVecInit(ALLOCATOR_OF(&alloc));

    // Test lexicographic comparison
    // bv1: 10 (shorter)
    BitVecPush(&bv1, true);
    BitVecPush(&bv1, false);

    // bv2: 101 (longer)
    BitVecPush(&bv2, true);
    BitVecPush(&bv2, false);
    BitVecPush(&bv2, true);

    // Lexicographically, shorter comes first
    bool result = (BitVecCompare(&bv1, &bv2) < 0);

    // Numerically, 10 (2) < 101 (5)
    result = result && (BitVecNumericalCompare(&bv1, &bv2) < 0);

    // Test equal cases
    BitVec bv3 = BitVecClone(&bv1);
    result     = result && !(BitVecCompare(&bv1, &bv3) < 0);
    result     = result && !(BitVecNumericalCompare(&bv1, &bv3) < 0);

    // Test reverse comparison
    result = result && !(BitVecCompare(&bv2, &bv1) < 0);
    result = result && !(BitVecNumericalCompare(&bv2, &bv1) < 0);

    // Clean up
    BitVecDeinit(&bv1);
    BitVecDeinit(&bv2);
    BitVecDeinit(&bv3);

    DefaultAllocatorDeinit(&alloc);

    return result;
}

// Test BitVecIsSorted function
bool test_bitvec_is_sorted(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVecIsSorted\n");

    BitVec bv = BitVecInit(ALLOCATOR_OF(&alloc));

    // Test empty bitvector (should be sorted)
    bool result = BitVecIsSorted(&bv);

    // Test sorted pattern: 0001111
    BitVecPush(&bv, false);
    BitVecPush(&bv, false);
    BitVecPush(&bv, false);
    BitVecPush(&bv, true);
    BitVecPush(&bv, true);
    BitVecPush(&bv, true);
    BitVecPush(&bv, true);

    result = result && BitVecIsSorted(&bv);

    // Test unsorted pattern (add 0 after 1s)
    BitVecPush(&bv, false);
    result = result && !BitVecIsSorted(&bv);

    // Test all zeros
    BitVecClear(&bv);
    for (int i = 0; i < 5; i++) {
        BitVecPush(&bv, false);
    }
    result = result && BitVecIsSorted(&bv);

    // Test all ones
    BitVecClear(&bv);
    for (int i = 0; i < 5; i++) {
        BitVecPush(&bv, true);
    }
    result = result && BitVecIsSorted(&bv);

    // Clean up
    BitVecDeinit(&bv);

    DefaultAllocatorDeinit(&alloc);

    return result;
}

// Edge case tests
bool test_bitvec_compare_edge_cases(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVec compare edge cases\n");

    BitVec bv1    = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVec bv2    = BitVecInit(ALLOCATOR_OF(&alloc));
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
    DefaultAllocatorDeinit(&alloc);
    return result;
}

bool test_bitvec_set_operations_edge_cases(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVec set operations edge cases\n");

    BitVec bv1    = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVec bv2    = BitVecInit(ALLOCATOR_OF(&alloc));
    bool   result = true;

    // Test with empty sets
    result = result && !BitVecOverlaps(&bv1, &bv2);
    result = result && BitVecDisjoint(&bv1, &bv2);

    // Test single bit sets
    BitVecPush(&bv1, true);
    BitVecPush(&bv2, false);
    result = result && !BitVecOverlaps(&bv1, &bv2);

    // Test large sets
    BitVecClear(&bv1);
    BitVecClear(&bv2);
    for (int i = 0; i < 1000; i++) {
        BitVecPush(&bv1, i % 2 == 0);
        BitVecPush(&bv2, i % 3 == 0);
    }
    // Should have some overlap since both contain position 0 (true)
    result = result && BitVecOverlaps(&bv1, &bv2);

    BitVecDeinit(&bv1);
    BitVecDeinit(&bv2);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Comprehensive comparison testing with cross-validation
bool test_bitvec_comprehensive_comparison(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVec comprehensive comparison operations\n");

    BitVec bv1    = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVec bv2    = BitVecInit(ALLOCATOR_OF(&alloc));
    bool   result = true;

    // Test comparison consistency across all comparison types
    // bv1: 1010110 (decimal 86 when read as LSB-first)
    int pattern1[] = {1, 0, 1, 0, 1, 1, 0};
    for (int i = 0; i < 7; i++) {
        BitVecPush(&bv1, pattern1[i]);
    }

    // bv2: 1100101 (decimal 89 when read as LSB-first)
    int pattern2[] = {1, 1, 0, 0, 1, 0, 1};
    for (int i = 0; i < 7; i++) {
        BitVecPush(&bv2, pattern2[i]);
    }

    // Cross-validate different comparison methods
    // Numerical: 86 < 89, so bv1 < bv2
    result = result && (BitVecNumericalCompare(&bv1, &bv2) < 0);
    result = result && (BitVecCompare(&bv1, &bv2) < 0);

    // Weight: bv1 has 4 ones, bv2 has 4 ones, so equal weight
    result = result && (BitVecWeightCompare(&bv1, &bv2) == 0);

    // Lexicographic comparison
    int  lex_cmp  = BitVecCompare(&bv1, &bv2);
    bool lex_less = (BitVecCompare(&bv1, &bv2) < 0);
    result        = result && ((lex_cmp < 0) == lex_less);

    // Test transitivity: if A < B and B < C, then A < C
    BitVec bv3 = BitVecInit(ALLOCATOR_OF(&alloc));
    // bv3: larger than bv2
    for (int i = 0; i < 8; i++) {
        BitVecPush(&bv3, true);
    }

    if (BitVecNumericalCompare(&bv1, &bv2) < 0 && BitVecNumericalCompare(&bv2, &bv3) < 0) {
        result = result && (BitVecNumericalCompare(&bv1, &bv3) < 0);
    }

    // Test subset/superset consistency
    BitVec subset   = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVec superset = BitVecInit(ALLOCATOR_OF(&alloc));

    // Create actual subset/superset relationship
    BitVecPush(&subset, true);
    BitVecPush(&subset, false);
    BitVecPush(&subset, true);

    BitVecPush(&superset, true);
    BitVecPush(&superset, true);
    BitVecPush(&superset, true);

    result = result && BitVecIsSubset(&subset, &superset);
    result = result && BitVecIsSuperset(&superset, &subset);

    // Clean up
    BitVecDeinit(&bv1);
    BitVecDeinit(&bv2);
    BitVecDeinit(&bv3);
    BitVecDeinit(&subset);
    BitVecDeinit(&superset);

    DefaultAllocatorDeinit(&alloc);

    return result;
}

// Large-scale testing with stress patterns
bool test_bitvec_large_scale_comparison(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVec large-scale comparison operations\n");

    BitVec large1 = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVec large2 = BitVecInit(ALLOCATOR_OF(&alloc));
    bool   result = true;

    // Create large bitvectors (2000 bits each)
    for (int i = 0; i < 2000; i++) {
        // Pattern 1: Fibonacci-like XOR pattern
        bool bit1 = (i % 3 == 0) ^ (i % 5 == 0);
        BitVecPush(&large1, bit1);

        // Pattern 2: Prime-like pattern
        bool bit2 = (i % 7 == 0) || (i % 11 == 0);
        BitVecPush(&large2, bit2);
    }

    // Test large-scale comparison performance and correctness
    result = result && (BitVecEquals(&large1, &large1)); // Self-equality
    result = result && !BitVecEquals(&large1, &large2);  // Different patterns

    // Test range operations on large vectors
    result = result && BitVecEqualsRange(&large1, 100, &large1, 100, 500); // Self-range equality

    // Test set operations on large vectors
    bool overlaps = BitVecOverlaps(&large1, &large2);
    bool disjoint = BitVecDisjoint(&large1, &large2);
    result        = result && (overlaps != disjoint); // Should be opposite

    // Verify signed vs unsigned comparison differences
    BitVec pos = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVec neg = BitVecInit(ALLOCATOR_OF(&alloc));

    // Positive number (MSB = 0): 01111111
    for (int i = 0; i < 7; i++) {
        BitVecPush(&pos, true);
    }
    BitVecPush(&pos, false);

    // Negative number (MSB = 1): 10000001
    BitVecPush(&neg, true);
    for (int i = 1; i < 7; i++) {
        BitVecPush(&neg, false);
    }
    BitVecPush(&neg, true);

    // Unsigned: 01111111 (127) < 10000001 (129)
    result = result && (BitVecNumericalCompare(&pos, &neg) < 0);

    // Signed: 01111111 (+127) > 10000001 (-127)
    result = result && (BitVecSignedCompare(&pos, &neg) > 0);

    // Clean up
    BitVecDeinit(&large1);
    BitVecDeinit(&large2);
    BitVecDeinit(&pos);
    BitVecDeinit(&neg);

    DefaultAllocatorDeinit(&alloc);

    return result;
}

// Deadend tests
bool test_bitvec_compare_null_failures(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVec compare NULL pointer handling\n");

    BitVec bv = BitVecInit(ALLOCATOR_OF(&alloc));

    // Test NULL pointer - should abort
    BitVecEquals(NULL, &bv);

    BitVecDeinit(&bv);
    DefaultAllocatorDeinit(&alloc);
    return false;
}

bool test_bitvec_subset_null_failures(void) {
    WriteFmt("Testing BitVec subset NULL handling\n");

    // Test NULL pointer - should abort
    BitVecIsSubset(NULL, NULL);

    return false;
}

bool test_bitvec_range_null_failures(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVec range operations NULL handling\n");

    BitVec bv = BitVecInit(ALLOCATOR_OF(&alloc));

    // Test NULL pointer in range operations - should abort
    BitVecEqualsRange(NULL, 0, &bv, 0, 1);

    BitVecDeinit(&bv);
    DefaultAllocatorDeinit(&alloc);
    return false;
}

bool test_bitvec_range_bounds_failures(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVec range operations bounds checking\n");

    BitVec bv1 = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVec bv2 = BitVecInit(ALLOCATOR_OF(&alloc));

    // Create small bitvectors
    for (int i = 0; i < 3; i++) {
        BitVecPush(&bv1, i % 2 == 0);
        BitVecPush(&bv2, i % 2 == 1);
    }

    // Test out-of-bounds range - should abort
    BitVecEqualsRange(&bv1, 0, &bv2, 0, 5); // Range exceeds length

    BitVecDeinit(&bv1);
    BitVecDeinit(&bv2);
    DefaultAllocatorDeinit(&alloc);
    return false;
}

bool test_bitvec_sorted_null_failures(void) {
    WriteFmt("Testing BitVec sorted operations NULL handling\n");

    // Test NULL pointer - should abort
    BitVecIsSorted(NULL);

    return false;
}

// Main function that runs all tests
int main(void) {
    WriteFmt("[INFO] Starting BitVec.Compare tests\n\n");

    // Array of normal test functions
    TestFunction tests[] = {
        test_bitvec_equals,
        test_bitvec_compare,
        test_bitvec_lex_compare,
        test_bitvec_numerical_compare,
        test_bitvec_weight_compare,
        test_bitvec_signed_compare,
        test_bitvec_is_subset,
        test_bitvec_is_superset,
        test_bitvec_overlaps,
        test_bitvec_disjoint_intersects,
        test_bitvec_equals_range,
        test_bitvec_compare_range,
        test_bitvec_less_than_functions,
        test_bitvec_is_sorted,
        test_bitvec_compare_edge_cases,
        test_bitvec_set_operations_edge_cases,
        test_bitvec_comprehensive_comparison,
        test_bitvec_large_scale_comparison
    };

    // Array of deadend test functions
    TestFunction deadend_tests[] = {
        test_bitvec_compare_null_failures,
        test_bitvec_subset_null_failures,
        test_bitvec_range_null_failures,
        test_bitvec_range_bounds_failures,
        test_bitvec_sorted_null_failures
    };

    int total_tests         = sizeof(tests) / sizeof(tests[0]);
    int total_deadend_tests = sizeof(deadend_tests) / sizeof(deadend_tests[0]);

    // Run all tests using the centralized test driver
    return run_test_suite(tests, total_tests, deadend_tests, total_deadend_tests, "BitVec.Compare");
}
