#include <Misra/Std/Container/BitVec.h>
#include <Misra/Std/Container/Map.h>
#include <Misra/Std/Allocator/Default.h>
#include <Misra/Std/Log.h>
#include <Misra/Types.h>

// Include test utilities
#include "../Util/TestRunner.h"

// Function prototypes
bool test_bitvec_equals(void);
bool test_bitvec_compare(void);
bool test_bitvec_hash_determinism(void);
bool test_bitvec_hash_distinguishes(void);
bool test_bitvec_hash_as_map_key(void);
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
bool test_bitvec_compare_callback(void);
bool test_bitvec_subset_null_failures(void);
bool test_bitvec_range_null_failures(void);
bool test_bitvec_range_bounds_failures(void);
bool test_bitvec_sorted_null_failures(void);
bool test_equals_rejects_bad_second_operand(void);
bool test_equals_range_rejects_bad_second_operand(void);
bool test_subset_skips_high_positions(void);
bool test_subset_bv1_short_no_abort(void);
bool test_subset_bv2_short_no_abort(void);
bool test_disjoint_scans_all_positions(void);
bool test_subset_null_bv2_aborts(void);
bool test_disjoint_null_bv1_aborts(void);
bool test_disjoint_null_bv2_aborts(void);

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

// Two bitvectors built the same way must hash identically.
bool test_bitvec_hash_determinism(void) {
    WriteFmt("Testing bitvec_hash determinism\n");

    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *base  = ALLOCATOR_OF(&alloc);

    BitVec a = BitVecInit(base);
    BitVec b = BitVecInit(base);
    for (int i = 0; i < 11; i++) {
        BitVecPush(&a, (i % 2) == 0);
        BitVecPush(&b, (i % 2) == 0);
    }

    BitVec empty1 = BitVecInit(base);
    BitVec empty2 = BitVecInit(base);

    bool result = (bitvec_hash(&a, 0) == bitvec_hash(&b, 0));
    result      = result && (bitvec_hash(&empty1, 0) == bitvec_hash(&empty2, 0));

    BitVecDeinit(&a);
    BitVecDeinit(&b);
    BitVecDeinit(&empty1);
    BitVecDeinit(&empty2);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Bit pattern AND length both feed the hash, so two BitVecs that
// share a byte prefix but differ in length must still distinguish.
bool test_bitvec_hash_distinguishes(void) {
    WriteFmt("Testing bitvec_hash sensitivity to bits and length\n");

    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *base  = ALLOCATOR_OF(&alloc);

    BitVec a = BitVecInit(base);
    BitVec b = BitVecInit(base);
    BitVec c = BitVecInit(base);

    BitVecPush(&a, true);
    BitVecPush(&a, false);
    BitVecPush(&a, true);

    BitVecPush(&b, true);
    BitVecPush(&b, false);
    BitVecPush(&b, false);

    // c shares the byte-prefix with a but is longer.
    BitVecPush(&c, true);
    BitVecPush(&c, false);
    BitVecPush(&c, true);
    BitVecPush(&c, false);
    BitVecPush(&c, false);

    bool result = (bitvec_hash(&a, 0) != bitvec_hash(&b, 0));
    result      = result && (bitvec_hash(&a, 0) != bitvec_hash(&c, 0));

    BitVecDeinit(&a);
    BitVecDeinit(&b);
    BitVecDeinit(&c);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// End-to-end: BitVec as a Map key with the GenericHash/GenericCompare-
// shaped helpers wired in directly -- no per-callsite cast needed.
bool test_bitvec_hash_as_map_key(void) {
    WriteFmt("Testing bitvec_hash as Map<BitVec, u64> key\n");

    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *base  = ALLOCATOR_OF(&alloc);

    Map(BitVec, u64) counts = MapInit(bitvec_hash, bitvec_compare, &alloc);

    BitVec k1 = BitVecInit(base);
    BitVecPush(&k1, true);
    BitVecPush(&k1, false);
    BitVecPush(&k1, true);

    BitVec k2 = BitVecInit(base);
    BitVecPush(&k2, false);
    BitVecPush(&k2, true);

    MapInsertR(&counts, k1, 1u);
    MapInsertR(&counts, k2, 2u);

    BitVec probe = BitVecInit(base);
    BitVecPush(&probe, true);
    BitVecPush(&probe, false);
    BitVecPush(&probe, true);

    u64 *got = MapGetFirstPtr(&counts, probe);

    BitVec missing = BitVecInit(base);
    BitVecPush(&missing, true);
    BitVecPush(&missing, true);
    u64 *gone = MapGetFirstPtr(&counts, missing);

    bool result = (got != NULL && *got == 1u);
    result      = result && (gone == NULL);
    result      = result && (MapPairCount(&counts) == 2);

    BitVecDeinit(&k1);
    BitVecDeinit(&k2);
    BitVecDeinit(&probe);
    BitVecDeinit(&missing);
    MapDeinit(&counts);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Drop bitvec_compare into a GenericCompare slot and check ordering /
// equality / identity. Mirrors how str_compare is exercised.
bool test_bitvec_compare_callback(void) {
    WriteFmt("Testing bitvec_compare as GenericCompare callback\n");

    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *base  = ALLOCATOR_OF(&alloc);

    BitVec a = BitVecInit(base);
    BitVecPush(&a, false);
    BitVecPush(&a, true);

    BitVec b = BitVecInit(base);
    BitVecPush(&b, false);
    BitVecPush(&b, true);

    BitVec c = BitVecInit(base);
    BitVecPush(&c, true);
    BitVecPush(&c, false);

    GenericCompare cmp = bitvec_compare;

    bool result = (cmp(&a, &b) == 0);
    result      = result && (cmp(&a, &c) < 0);
    result      = result && (cmp(&c, &a) > 0);
    result      = result && (cmp(&a, &a) == 0);

    BitVecDeinit(&a);
    BitVecDeinit(&b);
    BitVecDeinit(&c);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Main function that runs all tests
// Kills BitVec.c:598:5 cxx_remove_void_call -- ValidateBitVec(bv2) in
// BitVecEquals. bv1 is non-empty so a length-mismatch short-circuits to
// `return false` before BitVecEqualsRange would re-validate bv2; only the
// dropped validate keeps real code aborting on the bad-magic bv2.
bool test_equals_rejects_bad_second_operand(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVecEquals rejects bad second operand\n");

    BitVec bv1 = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVec bad = {0};

    BitVecPush(&bv1, true);

    BitVecEquals(&bv1, &bad);

    BitVecDeinit(&bv1);
    DefaultAllocatorDeinit(&alloc);
    return false;
}

// Kills BitVec.c:609:5 cxx_remove_void_call -- ValidateBitVec(bv2) in
// BitVecEqualsRange. len == 0 keeps the range checks and the compare loop from
// re-touching bv2; only the dropped validate keeps real code aborting.
bool test_equals_range_rejects_bad_second_operand(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVecEqualsRange rejects bad second operand\n");

    BitVec bv1 = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVec bad = {0};

    BitVecEqualsRange(&bv1, 0, &bad, 0, 0);

    BitVecDeinit(&bv1);
    DefaultAllocatorDeinit(&alloc);
    return false;
}

// Kills 641:cxx_xor_assign_to_or_assign -- the live-byte mix `hash ^= byte`
// becoming `hash |= byte` is lossy: 0x00 and 0x01 then accumulate to the
// same value. The contract: two same-length bitvecs differing in a single
// bit must hash differently.
static bool test_hash_xor_byte_distinguishes(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *base  = ALLOCATOR_OF(&alloc);

    BitVec a = BitVecInit(base); // byte 0x00
    BitVec b = BitVecInit(base); // byte 0x01

    // a: 8 zero bits -> byte 0x00.
    for (int i = 0; i < 8; i++)
        BitVecPush(&a, false);
    // b: bit 0 set, rest clear -> byte 0x01 (same length 8).
    BitVecPush(&b, true);
    for (int i = 0; i < 7; i++)
        BitVecPush(&b, false);

    bool result = (bitvec_hash(&a, 0) != bitvec_hash(&b, 0));

    BitVecDeinit(&a);
    BitVecDeinit(&b);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Kills 645:cxx_mul_to_div -- the FNV step `hash *= prime` in the length
// tail-mix becoming `hash /= prime` repeatedly divides by a huge constant and
// collapses the accumulator toward zero, so distinct inputs alias. The
// contract: two same-length bitvecs differing in content must hash
// differently.
static bool test_hash_length_mix_mul_distinguishes(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *base  = ALLOCATOR_OF(&alloc);

    BitVec a = BitVecInit(base); // 10101010
    BitVec b = BitVecInit(base); // 01010101

    for (int i = 0; i < 8; i++) {
        BitVecPush(&a, (i % 2) == 0);
        BitVecPush(&b, (i % 2) == 1);
    }

    bool result = (bitvec_hash(&a, 0) != bitvec_hash(&b, 0));

    BitVecDeinit(&a);
    BitVecDeinit(&b);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Kills 640:cxx_post_inc_to_post_dec -- the live-byte loop counter `i++`
// becoming `i--` underflows to a huge value after one pass, so only byte 0 is
// folded into the hash and every later byte is dropped. The contract: two
// 16-bit bitvecs that share their first byte but differ in the second must
// hash differently.
static bool test_hash_all_bytes_folded(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *base  = ALLOCATOR_OF(&alloc);

    BitVec a = BitVecInit(base); // byte0 = 0xFF, byte1 = 0x00
    BitVec b = BitVecInit(base); // byte0 = 0xFF, byte1 = 0xFF

    // a: first 8 bits set, next 8 clear.
    for (int i = 0; i < 8; i++)
        BitVecPush(&a, true);
    for (int i = 0; i < 8; i++)
        BitVecPush(&a, false);
    // b: all 16 bits set.
    for (int i = 0; i < 16; i++)
        BitVecPush(&b, true);

    bool result = (bitvec_hash(&a, 0) != bitvec_hash(&b, 0));

    BitVecDeinit(&a);
    BitVecDeinit(&b);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Kills 635:cxx_remove_void_call -- ValidateBitVec(bv) in bitvec_hash. Real
// code aborts on a NULL bitvec; the mutant skips the guard and dereferences.
static bool test_hash_null_aborts(void) {
    bitvec_hash(NULL, 0); // must abort
    return false;
}

// Kills 656:cxx_remove_void_call -- ValidateBitVec(bv1) in BitVecCompare.
static bool test_compare_null_lhs_aborts(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *base  = ALLOCATOR_OF(&alloc);

    BitVec bv = BitVecInit(base);
    BitVecPush(&bv, true);

    BitVecCompare(NULL, &bv); // must abort

    BitVecDeinit(&bv);
    DefaultAllocatorDeinit(&alloc);
    return false;
}

// Kills 657:cxx_remove_void_call -- ValidateBitVec(bv2) in BitVecCompare.
static bool test_compare_null_rhs_aborts(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *base  = ALLOCATOR_OF(&alloc);

    BitVec bv = BitVecInit(base);
    BitVecPush(&bv, true);

    BitVecCompare(&bv, NULL); // must abort

    BitVecDeinit(&bv);
    DefaultAllocatorDeinit(&alloc);
    return false;
}

// Kills 678:cxx_remove_void_call -- ValidateBitVec(bv1) in BitVecCompareRange.
static bool test_compare_range_null_lhs_aborts(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *base  = ALLOCATOR_OF(&alloc);

    BitVec bv = BitVecInit(base);
    BitVecPush(&bv, true);

    BitVecCompareRange(NULL, 0, &bv, 0, 1); // must abort

    BitVecDeinit(&bv);
    DefaultAllocatorDeinit(&alloc);
    return false;
}

// Kills 679:cxx_remove_void_call -- ValidateBitVec(bv2) in BitVecCompareRange.
static bool test_compare_range_null_rhs_aborts(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *base  = ALLOCATOR_OF(&alloc);

    BitVec bv = BitVecInit(base);
    BitVecPush(&bv, true);

    BitVecCompareRange(&bv, 0, NULL, 0, 1); // must abort

    BitVecDeinit(&bv);
    DefaultAllocatorDeinit(&alloc);
    return false;
}

// Kills 703:cxx_remove_void_call -- ValidateBitVec(bv1) in
// BitVecNumericalCompare.
static bool test_numerical_compare_null_lhs_aborts(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *base  = ALLOCATOR_OF(&alloc);

    BitVec bv = BitVecInit(base);
    BitVecPush(&bv, true);

    BitVecNumericalCompare(NULL, &bv); // must abort

    BitVecDeinit(&bv);
    DefaultAllocatorDeinit(&alloc);
    return false;
}

// BitVecNumericalCompare walks from the true max length down. Replacing the
// loop bound / iterator init with the constant 42 (init_const at 710:9 and
// 712:14) would stop the walk before reaching a differing bit that lives at
// a high index, masking a real ordering. Use a 50-bit operand whose only set
// bit is at index 49.
static bool test_numerical_high_bit_not_truncated(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *base  = ALLOCATOR_OF(&alloc);

    BitVec bv1 = BitVecInit(base);
    BitVec bv2 = BitVecInit(base);

    for (u64 i = 0; i < 50; i++) {
        BitVecPush(&bv1, i == 49);
        BitVecPush(&bv2, false);
    }

    // bv1 has its most-significant magnitude bit set, bv2 is all zero.
    bool result = (BitVecNumericalCompare(&bv1, &bv2) > 0);

    BitVecDeinit(&bv1);
    BitVecDeinit(&bv2);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// BitVecSignedCompare: the both-empty fast path (745). Flipping the first
// "== 0" to "!= 0" (eq_to_ne at 745:21) makes a non-empty bv1 with empty bv2
// take the early "return 0" branch, hiding that a negative value is less than
// zero.
static bool test_signed_neg_vs_empty(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *base  = ALLOCATOR_OF(&alloc);

    BitVec neg   = BitVecInit(base);
    BitVec empty = BitVecInit(base);

    BitVecPush(&neg, true); // single bit set -> MSB set -> negative

    bool result = (BitVecSignedCompare(&neg, &empty) < 0);

    BitVecDeinit(&neg);
    BitVecDeinit(&empty);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// BitVecSignedCompare: flipping the second "== 0" to "!= 0" (eq_to_ne at
// 745:41) makes empty bv1 with non-empty bv2 wrongly take the early return.
// Empty (non-negative zero) must compare greater than a negative.
static bool test_signed_empty_vs_neg(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *base  = ALLOCATOR_OF(&alloc);

    BitVec empty = BitVecInit(base);
    BitVec neg   = BitVecInit(base);

    BitVecPush(&neg, true); // negative

    bool result = (BitVecSignedCompare(&empty, &neg) > 0);

    BitVecDeinit(&empty);
    BitVecDeinit(&neg);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// BitVecSignedCompare sign extraction for bv1 (751). gt_to_le turns
// "length > 0" into "length <= 0", so a non-empty negative bv1 is read as
// non-negative, inverting the ordering against a non-negative bv2.
static bool test_signed_sign1_le_mutant(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *base  = ALLOCATOR_OF(&alloc);

    BitVec neg = BitVecInit(base);
    BitVec pos = BitVecInit(base);

    BitVecPush(&neg, true);  // length 1, MSB set -> negative
    BitVecPush(&pos, false); // length 1, MSB clear -> non-negative (zero)

    // negative < zero
    bool result = (BitVecSignedCompare(&neg, &pos) < 0);

    BitVecDeinit(&neg);
    BitVecDeinit(&pos);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// BitVecSignedCompare sign extraction for bv1 (751). gt_to_ge turns
// "length > 0" into "length >= 0": for an empty bv1 it forces the
// BitVecGet(bv1, length - 1) branch with index UINT64_MAX, which aborts.
// On real code empty (zero) compares greater than a negative without aborting.
static bool test_signed_sign1_ge_no_abort(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *base  = ALLOCATOR_OF(&alloc);

    BitVec empty = BitVecInit(base);
    BitVec neg   = BitVecInit(base);

    BitVecPush(&neg, true); // negative

    bool result = (BitVecSignedCompare(&empty, &neg) > 0);

    BitVecDeinit(&empty);
    BitVecDeinit(&neg);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// BitVecSignedCompare sign extraction for bv2 (752). gt_to_ge turns
// "length > 0" into "length >= 0": for an empty bv2 it forces the
// BitVecGet(bv2, length - 1) branch with index UINT64_MAX, which aborts.
// On real code a negative bv1 compares less than empty (zero) without aborting.
static bool test_signed_sign2_ge_no_abort(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *base  = ALLOCATOR_OF(&alloc);

    BitVec neg   = BitVecInit(base);
    BitVec empty = BitVecInit(base);

    BitVecPush(&neg, true); // negative

    bool result = (BitVecSignedCompare(&neg, &empty) < 0);

    BitVecDeinit(&neg);
    BitVecDeinit(&empty);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// BitVecSignedCompare negation of two negatives (764). assign_const replaces
// "result = -result" with "result = 42", so two equal negatives stop
// comparing equal.
static bool test_signed_two_equal_negatives(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *base  = ALLOCATOR_OF(&alloc);

    BitVec a = BitVecInit(base);
    BitVec b = BitVecInit(base);

    // [1,1]: MSB (index 1) set -> both negative, identical magnitude.
    BitVecPush(&a, true);
    BitVecPush(&a, true);
    BitVecPush(&b, true);
    BitVecPush(&b, true);

    bool result = (BitVecSignedCompare(&a, &b) == 0);

    BitVecDeinit(&a);
    BitVecDeinit(&b);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// BitVecSignedCompare negation of two negatives (764). minus_to_noop turns
// "result = -result" into "result = result", dropping the magnitude->value
// flip. For two negatives the larger magnitude is the smaller value, so the
// sign of the answer must be inverted relative to the unsigned compare.
static bool test_signed_two_negatives_magnitude_flip(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *base  = ALLOCATOR_OF(&alloc);

    BitVec a = BitVecInit(base);
    BitVec b = BitVecInit(base);

    // a = [1,1]: index1 (MSB) set -> negative, magnitude 3.
    BitVecPush(&a, true);
    BitVecPush(&a, true);
    // b = [0,1]: index1 (MSB) set -> negative, magnitude 2.
    BitVecPush(&b, false);
    BitVecPush(&b, true);

    // a has the larger magnitude, so a is the more-negative -> a < b.
    bool result = (BitVecSignedCompare(&a, &b) < 0);

    BitVecDeinit(&a);
    BitVecDeinit(&b);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Deadend: BitVecNumericalCompare validates bv2 (704). Removing that
// validation lets a NULL bv2 reach bv2->length and crash instead of the
// controlled abort.
static bool test_numerical_null_bv2(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *base  = ALLOCATOR_OF(&alloc);

    BitVec bv = BitVecInit(base);
    BitVecPush(&bv, true);

    BitVecNumericalCompare(&bv, NULL); // must abort

    BitVecDeinit(&bv);
    DefaultAllocatorDeinit(&alloc);
    return false;
}

// Deadend: BitVecSignedCompare validates bv1 (742). Removing it lets a NULL
// bv1 reach bv1->length.
static bool test_signed_null_bv1(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *base  = ALLOCATOR_OF(&alloc);

    BitVec bv = BitVecInit(base);
    BitVecPush(&bv, true);

    BitVecSignedCompare(NULL, &bv); // must abort

    BitVecDeinit(&bv);
    DefaultAllocatorDeinit(&alloc);
    return false;
}

// Deadend: BitVecSignedCompare validates bv2 (743). Removing it lets a NULL
// bv2 reach bv2->length.
static bool test_signed_null_bv2(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *base  = ALLOCATOR_OF(&alloc);

    BitVec bv = BitVecInit(base);
    BitVecPush(&bv, true);

    BitVecSignedCompare(&bv, NULL); // must abort

    BitVecDeinit(&bv);
    DefaultAllocatorDeinit(&alloc);
    return false;
}

// Deadend: BitVecIsSubset validates bv1 (771). Removing it lets a NULL bv1
// reach bv1->length.
static bool test_subset_null_bv1(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *base  = ALLOCATOR_OF(&alloc);

    BitVec bv = BitVecInit(base);
    BitVecPush(&bv, true);

    BitVecIsSubset(NULL, &bv); // must abort

    BitVecDeinit(&bv);
    DefaultAllocatorDeinit(&alloc);
    return false;
}

// 777:init_const -- max_len forced to 42 must not stop the subset scan
// short of a high-index disagreement. bv1 has a lone 1 at position 45
// that bv2 lacks, so the relation is false; a scan that quits at 42
// would miss it and wrongly report a subset.
bool test_subset_skips_high_positions(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *base  = ALLOCATOR_OF(&alloc);

    WriteFmt("Testing BitVecIsSubset scans past index 42\n");

    BitVec bv1 = BitVecInit(base);
    BitVec bv2 = BitVecInit(base);

    for (u64 i = 0; i < 50; i++) {
        BitVecPush(&bv1, i == 45);
        BitVecPush(&bv2, false);
    }

    // bv1's 1-bit at 45 has no counterpart in bv2 -> not a subset.
    bool result = (BitVecIsSubset(&bv1, &bv2) == false);

    BitVecDeinit(&bv1);
    BitVecDeinit(&bv2);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// 780:lt_to_le -- the bv1 in-range guard `i < bv1->length` becoming
// `i <= bv1->length` makes a valid subset query call BitVecGet at the
// out-of-range index bv1->length once bv2 is the longer vector, which
// aborts. Real code returns cleanly.
bool test_subset_bv1_short_no_abort(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *base  = ALLOCATOR_OF(&alloc);

    WriteFmt("Testing BitVecIsSubset with shorter bv1 stays in bounds\n");

    BitVec bv1 = BitVecInit(base); // length 2
    BitVec bv2 = BitVecInit(base); // length 5

    BitVecPush(&bv1, true);
    BitVecPush(&bv1, false);

    BitVecPush(&bv2, true);
    BitVecPush(&bv2, false);
    BitVecPush(&bv2, false);
    BitVecPush(&bv2, false);
    BitVecPush(&bv2, false);

    // bv1's only 1 (pos 0) is set in bv2 -> subset is true.
    bool result = (BitVecIsSubset(&bv1, &bv2) == true);

    BitVecDeinit(&bv1);
    BitVecDeinit(&bv2);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// 781:lt_to_le -- the bv2 in-range guard `i < bv2->length` becoming
// `i <= bv2->length` makes a valid subset query call BitVecGet at the
// out-of-range index bv2->length once bv1 is the longer vector, which
// aborts. Real code returns cleanly.
bool test_subset_bv2_short_no_abort(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *base  = ALLOCATOR_OF(&alloc);

    WriteFmt("Testing BitVecIsSubset with shorter bv2 stays in bounds\n");

    BitVec bv1 = BitVecInit(base); // length 5
    BitVec bv2 = BitVecInit(base); // length 2

    BitVecPush(&bv1, true);
    BitVecPush(&bv1, false);
    BitVecPush(&bv1, false);
    BitVecPush(&bv1, false);
    BitVecPush(&bv1, false);

    BitVecPush(&bv2, true);
    BitVecPush(&bv2, false);

    // bv1's only 1 (pos 0) is set in bv2 -> subset is true.
    bool result = (BitVecIsSubset(&bv1, &bv2) == true);

    BitVecDeinit(&bv1);
    BitVecDeinit(&bv2);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// 801:post_inc_to_post_dec -- the disjoint scan counter going from i++
// to i-- collapses the loop to a single iteration (i wraps after pos 0).
// Two vectors that overlap only at position 1 are therefore wrongly
// reported disjoint. Real code finds the overlap and returns false.
bool test_disjoint_scans_all_positions(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *base  = ALLOCATOR_OF(&alloc);

    WriteFmt("Testing BitVecDisjoint scans beyond position 0\n");

    BitVec bv1 = BitVecInit(base);
    BitVec bv2 = BitVecInit(base);

    // Both: 0 1 1 -- no shared 1 at position 0, shared 1s at 1 and 2.
    BitVecPush(&bv1, false);
    BitVecPush(&bv1, true);
    BitVecPush(&bv1, true);

    BitVecPush(&bv2, false);
    BitVecPush(&bv2, true);
    BitVecPush(&bv2, true);

    // They share 1-bits, so they are NOT disjoint.
    bool result = (BitVecDisjoint(&bv1, &bv2) == false);

    BitVecDeinit(&bv1);
    BitVecDeinit(&bv2);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// 772:remove_void_call -- dropping ValidateBitVec(bv2) in BitVecIsSubset
// lets a NULL bv2 slip past the validator (real code aborts cleanly).
bool test_subset_null_bv2_aborts(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *base  = ALLOCATOR_OF(&alloc);

    WriteFmt("Testing BitVecIsSubset rejects NULL bv2\n");

    BitVec bv1 = BitVecInit(base);
    BitVecPush(&bv1, true);

    // Should abort on the NULL second argument.
    BitVecIsSubset(&bv1, NULL);

    BitVecDeinit(&bv1);
    DefaultAllocatorDeinit(&alloc);
    return false;
}

// 796:remove_void_call -- dropping ValidateBitVec(bv1) in BitVecDisjoint
// lets a NULL bv1 slip past the validator (real code aborts cleanly).
bool test_disjoint_null_bv1_aborts(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *base  = ALLOCATOR_OF(&alloc);

    WriteFmt("Testing BitVecDisjoint rejects NULL bv1\n");

    BitVec bv2 = BitVecInit(base);
    BitVecPush(&bv2, true);

    // Should abort on the NULL first argument.
    BitVecDisjoint(NULL, &bv2);

    BitVecDeinit(&bv2);
    DefaultAllocatorDeinit(&alloc);
    return false;
}

// 797:remove_void_call -- dropping ValidateBitVec(bv2) in BitVecDisjoint
// lets a NULL bv2 slip past the validator (real code aborts cleanly).
bool test_disjoint_null_bv2_aborts(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *base  = ALLOCATOR_OF(&alloc);

    WriteFmt("Testing BitVecDisjoint rejects NULL bv2\n");

    BitVec bv1 = BitVecInit(base);
    BitVecPush(&bv1, true);

    // Should abort on the NULL second argument.
    BitVecDisjoint(&bv1, NULL);

    BitVecDeinit(&bv1);
    DefaultAllocatorDeinit(&alloc);
    return false;
}

// 645:28 cxx_rshift_to_lshift -- `bit_count >> (i*8)` -> `<<`. For lengths that
// fit in one byte the high bytes are zero either way, so it only diverges when
// bit_count spans multiple bytes. Use a length of 256 (0x100): real reads
// byte0=0x00, byte1=0x01, ...; the left-shift mutant reads `(256 << (i*8)) &
// 0xFF == 0` for every i, dropping the length signal entirely. So a length-256
// all-zero vec and a length-512 all-zero vec must hash DIFFERENTLY (real); the
// `<<` mutant collapses both to the same value.
static bool test_blind_hash_multibyte_length_shift(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    BitVec a = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVec b = BitVecInit(ALLOCATOR_OF(&alloc));
    for (int i = 0; i < 256; i++) {
        BitVecPush(&a, false);
    }
    for (int i = 0; i < 512; i++) {
        BitVecPush(&b, false);
    }
    // Byte content differs (32 vs 64 zero bytes), so this alone is not a pure
    // length test. Compute the real expected hashes by replay and require the
    // implementation to match them exactly -- the `<<` mutant cannot, since it
    // zeroes every length byte beyond the lowest.
    u64 ha = bitvec_hash(&a, 0);
    u64 hb = bitvec_hash(&b, 0);

    u64 seed  = 1469598103934665603ULL;
    u64 prime = 1099511628211ULL;
    u64 exp_a = seed, exp_b = seed;
    for (int i = 0; i < 32; i++) { // 256 bits -> 32 bytes
        exp_a ^= 0u;
        exp_a *= prime;
    }
    for (u64 i = 0, bc = 256; i < sizeof(bc); i++) {
        exp_a ^= (bc >> (i * 8u)) & 0xFFu;
        exp_a *= prime;
    }
    for (int i = 0; i < 64; i++) { // 512 bits -> 64 bytes
        exp_b ^= 0u;
        exp_b *= prime;
    }
    for (u64 i = 0, bc = 512; i < sizeof(bc); i++) {
        exp_b ^= (bc >> (i * 8u)) & 0xFFu;
        exp_b *= prime;
    }

    bool result = (ha == exp_a) && (hb == exp_b);

    BitVecDeinit(&a);
    BitVecDeinit(&b);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

int main(void) {
    WriteFmt("[INFO] Starting BitVec.Compare tests\n\n");

    // Array of normal test functions
    TestFunction tests[] = {
        test_bitvec_equals,
        test_bitvec_compare,
        test_bitvec_hash_determinism,
        test_bitvec_hash_distinguishes,
        test_bitvec_hash_as_map_key,
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
        test_bitvec_large_scale_comparison,
        test_bitvec_compare_callback,
        test_hash_xor_byte_distinguishes,
        test_hash_length_mix_mul_distinguishes,
        test_hash_all_bytes_folded,
        test_numerical_high_bit_not_truncated,
        test_signed_neg_vs_empty,
        test_signed_empty_vs_neg,
        test_signed_sign1_le_mutant,
        test_signed_sign1_ge_no_abort,
        test_signed_sign2_ge_no_abort,
        test_signed_two_equal_negatives,
        test_signed_two_negatives_magnitude_flip,
        test_subset_skips_high_positions,
        test_subset_bv1_short_no_abort,
        test_subset_bv2_short_no_abort,
        test_disjoint_scans_all_positions,
        test_blind_hash_multibyte_length_shift
    };

    // Array of deadend test functions
    TestFunction deadend_tests[] = {
        test_bitvec_compare_null_failures,
        test_bitvec_subset_null_failures,
        test_bitvec_range_null_failures,
        test_bitvec_range_bounds_failures,
        test_bitvec_sorted_null_failures,
        test_equals_rejects_bad_second_operand,
        test_equals_range_rejects_bad_second_operand,
        test_hash_null_aborts,
        test_compare_null_lhs_aborts,
        test_compare_null_rhs_aborts,
        test_compare_range_null_lhs_aborts,
        test_compare_range_null_rhs_aborts,
        test_numerical_compare_null_lhs_aborts,
        test_numerical_null_bv2,
        test_signed_null_bv1,
        test_signed_null_bv2,
        test_subset_null_bv1,
        test_subset_null_bv2_aborts,
        test_disjoint_null_bv1_aborts,
        test_disjoint_null_bv2_aborts
    };

    int total_tests         = sizeof(tests) / sizeof(tests[0]);
    int total_deadend_tests = sizeof(deadend_tests) / sizeof(deadend_tests[0]);

    // Run all tests using the centralized test driver
    return run_test_suite(tests, total_tests, deadend_tests, total_deadend_tests, "BitVec.Compare");
}
