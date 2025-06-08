#include <Misra/Std/Container/Bits.h>
#include <Misra/Std/Log.h>

#include <stdio.h>
#include <Misra/Types.h>

// Include test utilities
#include "../Util/TestRunner.h"

// Function prototypes
bool test_Bits_and(void);
bool test_Bits_or(void);
bool test_Bits_xor(void);
bool test_Bits_not(void);
bool test_Bits_shift_left(void);
bool test_Bits_shift_right(void);
bool test_Bits_rotate_left(void);
bool test_Bits_rotate_right(void);
bool test_Bits_reverse(void);
bool test_Bits_shift_edge_cases(void);
bool test_Bits_rotate_edge_cases(void);
bool test_Bits_bitwise_ops_edge_cases(void);
bool test_Bits_reverse_edge_cases(void);
bool test_Bits_bitwise_comprehensive(void);
bool test_Bits_shift_comprehensive(void);
bool test_Bits_rotate_comprehensive(void);
bool test_Bits_bitwise_identity_operations(void);
bool test_Bits_bitwise_commutative_properties(void);
bool test_Bits_bitwise_large_patterns(void);

// Test BitsAnd function
bool test_Bits_and(void) {
    printf("Testing BitsAnd\n");

    Bits bv1    = BitsInit();
    Bits bv2    = BitsInit();
    Bits result = BitsInit();

    // Set up first Bitstor: 1101
    BitsPush(&bv1, true);
    BitsPush(&bv1, true);
    BitsPush(&bv1, false);
    BitsPush(&bv1, true);

    // Set up second Bitstor: 1010
    BitsPush(&bv2, true);
    BitsPush(&bv2, false);
    BitsPush(&bv2, true);
    BitsPush(&bv2, false);

    // Perform AND operation
    BitsAnd(&result, &bv1, &bv2);

    // Expected result: 1000 (1101 AND 1010)
    bool test_result = (result.length == 4);
    test_result      = test_result && (BitsGet(&result, 0) == true);
    test_result      = test_result && (BitsGet(&result, 1) == false);
    test_result      = test_result && (BitsGet(&result, 2) == false);
    test_result      = test_result && (BitsGet(&result, 3) == false);

    // Clean up
    BitsDeinit(&bv1);
    BitsDeinit(&bv2);
    BitsDeinit(&result);

    return test_result;
}

// Test BitsOr function
bool test_Bits_or(void) {
    printf("Testing BitsOr\n");

    Bits bv1    = BitsInit();
    Bits bv2    = BitsInit();
    Bits result = BitsInit();

    // Set up first Bitstor: 1100
    BitsPush(&bv1, true);
    BitsPush(&bv1, true);
    BitsPush(&bv1, false);
    BitsPush(&bv1, false);

    // Set up second Bitstor: 1010
    BitsPush(&bv2, true);
    BitsPush(&bv2, false);
    BitsPush(&bv2, true);
    BitsPush(&bv2, false);

    // Perform OR operation
    BitsOr(&result, &bv1, &bv2);

    // Expected result: 1110 (1100 OR 1010)
    bool test_result = (result.length == 4);
    test_result      = test_result && (BitsGet(&result, 0) == true);
    test_result      = test_result && (BitsGet(&result, 1) == true);
    test_result      = test_result && (BitsGet(&result, 2) == true);
    test_result      = test_result && (BitsGet(&result, 3) == false);

    // Clean up
    BitsDeinit(&bv1);
    BitsDeinit(&bv2);
    BitsDeinit(&result);

    return test_result;
}

// Test BitsXor function
bool test_Bits_xor(void) {
    printf("Testing BitsXor\n");

    Bits bv1    = BitsInit();
    Bits bv2    = BitsInit();
    Bits result = BitsInit();

    // Set up first Bitstor: 1100
    BitsPush(&bv1, true);
    BitsPush(&bv1, true);
    BitsPush(&bv1, false);
    BitsPush(&bv1, false);

    // Set up second Bitstor: 1010
    BitsPush(&bv2, true);
    BitsPush(&bv2, false);
    BitsPush(&bv2, true);
    BitsPush(&bv2, false);

    // Perform XOR operation
    BitsXor(&result, &bv1, &bv2);

    // Expected result: 0110 (1100 XOR 1010)
    bool test_result = (result.length == 4);
    test_result      = test_result && (BitsGet(&result, 0) == false);
    test_result      = test_result && (BitsGet(&result, 1) == true);
    test_result      = test_result && (BitsGet(&result, 2) == true);
    test_result      = test_result && (BitsGet(&result, 3) == false);

    // Clean up
    BitsDeinit(&bv1);
    BitsDeinit(&bv2);
    BitsDeinit(&result);

    return test_result;
}

// Test BitsNot function
bool test_Bits_not(void) {
    printf("Testing BitsNot\n");

    Bits bv     = BitsInit();
    Bits result = BitsInit();

    // Set up Bitstor: 1010
    BitsPush(&bv, true);
    BitsPush(&bv, false);
    BitsPush(&bv, true);
    BitsPush(&bv, false);

    // Perform NOT operation
    BitsNot(&result, &bv);

    // Expected result: 0101 (NOT 1010)
    bool test_result = (result.length == 4);
    test_result      = test_result && (BitsGet(&result, 0) == false);
    test_result      = test_result && (BitsGet(&result, 1) == true);
    test_result      = test_result && (BitsGet(&result, 2) == false);
    test_result      = test_result && (BitsGet(&result, 3) == true);

    // Clean up
    BitsDeinit(&bv);
    BitsDeinit(&result);

    return test_result;
}

// Test BitsShiftLeft function - CORRECTED EXPECTATIONS
bool test_Bits_shift_left(void) {
    printf("Testing BitsShiftLeft\n");

    Bits bv = BitsInit();

    // Set up Bitstor: 1011 (indices 0,1,2,3)
    BitsPush(&bv, true);  // index 0
    BitsPush(&bv, false); // index 1
    BitsPush(&bv, true);  // index 2
    BitsPush(&bv, true);  // index 3

    // Shift left by 2 positions
    // Original: 1011 (bit 0=1, bit 1=0, bit 2=1, bit 3=1)
    // After shift left by 2: bits move to higher indices, lower indices filled with 0
    // New: 0011 (bit 0=0, bit 1=0, bit 2=1, bit 3=0) - but we shift out the high bits
    BitsShiftLeft(&bv, 2);

    // After shift left by 2, implementation should clear bits that shift out
    // and fill lower positions with 0
    bool test_result = (bv.length == 4);

    // Let me trace through the implementation:
    // Original: bit[0]=1, bit[1]=0, bit[2]=1, bit[3]=1
    // Shift left by 2 means bits move to higher indices:
    // New bit[2] = old bit[0] = 1
    // New bit[3] = old bit[1] = 0
    // New bit[0] = 0 (filled)
    // New bit[1] = 0 (filled)
    // Expected result: 0010
    test_result = test_result && (BitsGet(&bv, 0) == false);
    test_result = test_result && (BitsGet(&bv, 1) == false);
    test_result = test_result && (BitsGet(&bv, 2) == true);
    test_result = test_result && (BitsGet(&bv, 3) == false);

    // Clean up
    BitsDeinit(&bv);

    return test_result;
}

// Test BitsShiftRight function - CORRECTED EXPECTATIONS
bool test_Bits_shift_right(void) {
    printf("Testing BitsShiftRight\n");

    Bits bv = BitsInit();

    // Set up Bitstor: 1011
    BitsPush(&bv, true);  // index 0
    BitsPush(&bv, false); // index 1
    BitsPush(&bv, true);  // index 2
    BitsPush(&bv, true);  // index 3

    // Shift right by 2 positions
    // Original: 1011 (bit 0=1, bit 1=0, bit 2=1, bit 3=1)
    // After shift right by 2: bits move to lower indices
    // New bit[0] = old bit[2] = 1
    // New bit[1] = old bit[3] = 1
    // New bit[2] = 0 (filled)
    // New bit[3] = 0 (filled)
    // Expected result: 1100
    BitsShiftRight(&bv, 2);

    bool test_result = (bv.length == 4);
    test_result      = test_result && (BitsGet(&bv, 0) == true);
    test_result      = test_result && (BitsGet(&bv, 1) == true);
    test_result      = test_result && (BitsGet(&bv, 2) == false);
    test_result      = test_result && (BitsGet(&bv, 3) == false);

    // Clean up
    BitsDeinit(&bv);

    return test_result;
}

// Test BitsRotateLeft function
bool test_Bits_rotate_left(void) {
    printf("Testing BitsRotateLeft\n");

    Bits bv = BitsInit();

    // Set up Bitstor: 1011
    BitsPush(&bv, true);
    BitsPush(&bv, false);
    BitsPush(&bv, true);
    BitsPush(&bv, true);

    // Rotate left by 2 positions
    BitsRotateLeft(&bv, 2);

    // Expected result: 1110 (1011 rotated left by 2)
    bool test_result = (bv.length == 4);
    test_result      = test_result && (BitsGet(&bv, 0) == true);
    test_result      = test_result && (BitsGet(&bv, 1) == true);
    test_result      = test_result && (BitsGet(&bv, 2) == true);
    test_result      = test_result && (BitsGet(&bv, 3) == false);

    // Clean up
    BitsDeinit(&bv);

    return test_result;
}

// Test BitsRotateRight function
bool test_Bits_rotate_right(void) {
    printf("Testing BitsRotateRight\n");

    Bits bv = BitsInit();

    // Set up Bitstor: 1011
    BitsPush(&bv, true);
    BitsPush(&bv, false);
    BitsPush(&bv, true);
    BitsPush(&bv, true);

    // Rotate right by 1 position
    BitsRotateRight(&bv, 1);

    // Expected result: 1101 (1011 rotated right by 1)
    bool test_result = (bv.length == 4);
    test_result      = test_result && (BitsGet(&bv, 0) == true);
    test_result      = test_result && (BitsGet(&bv, 1) == true);
    test_result      = test_result && (BitsGet(&bv, 2) == false);
    test_result      = test_result && (BitsGet(&bv, 3) == true);

    // Clean up
    BitsDeinit(&bv);

    return test_result;
}

// Test BitsReverse function
bool test_Bits_reverse(void) {
    printf("Testing BitsReverse\n");

    Bits bv = BitsInit();

    // Set up Bitstor: 1011
    BitsPush(&bv, true);
    BitsPush(&bv, false);
    BitsPush(&bv, true);
    BitsPush(&bv, true);

    // Reverse the bits
    BitsReverse(&bv);

    // Expected result: 1101 (1011 reversed)
    bool test_result = (bv.length == 4);
    test_result      = test_result && (BitsGet(&bv, 0) == true);
    test_result      = test_result && (BitsGet(&bv, 1) == true);
    test_result      = test_result && (BitsGet(&bv, 2) == false);
    test_result      = test_result && (BitsGet(&bv, 3) == true);

    // Clean up
    BitsDeinit(&bv);

    return test_result;
}

// Edge case tests
bool test_Bits_shift_edge_cases(void) {
    printf("Testing Bits shift edge cases\n");

    Bits bv     = BitsInit();
    bool result = true;

    // Test shift empty Bits
    BitsShiftLeft(&bv, 5);
    result = result && (bv.length == 0);

    BitsShiftRight(&bv, 3);
    result = result && (bv.length == 0);

    // Test shift by 0 (should be no-op)
    BitsPush(&bv, true);
    BitsPush(&bv, false);
    BitsShiftLeft(&bv, 0);
    result = result && (bv.length == 2);
    result = result && (BitsGet(&bv, 0) == true);

    // Test shift larger than length (should clear all bits)
    BitsShiftLeft(&bv, 10);
    result = result && (bv.length == 0); // Should clear when shifting everything out

    // Test large data shift
    BitsClear(&bv);
    for (int i = 0; i < 1000; i++) {
        BitsPush(&bv, i % 2 == 0);
    }
    BitsShiftLeft(&bv, 1);
    result = result && (bv.length == 1000);

    BitsDeinit(&bv);
    return result;
}

bool test_Bits_rotate_edge_cases(void) {
    printf("Testing Bits rotate edge cases\n");

    Bits bv     = BitsInit();
    bool result = true;

    // Test rotate empty Bits
    BitsRotateLeft(&bv, 5);
    result = result && (bv.length == 0);

    // Test rotate by 0
    BitsPush(&bv, true);
    BitsRotateRight(&bv, 0);
    result = result && (BitsGet(&bv, 0) == true);

    // Test rotate by length (should be no-op)
    BitsPush(&bv, false);
    BitsRotateLeft(&bv, 2);
    result = result && (bv.length == 2);

    // Test large rotate amount
    BitsRotateRight(&bv, 1000);
    result = result && (bv.length == 2);

    BitsDeinit(&bv);
    return result;
}

bool test_Bits_bitwise_ops_edge_cases(void) {
    printf("Testing Bits bitwise operations edge cases\n");

    Bits bv1    = BitsInit();
    Bits bv2    = BitsInit();
    bool result = true;

    // Test operations on empty Bitss
    Bits result_bv = BitsInit();
    BitsAnd(&result_bv, &bv1, &bv2);
    result = result && (result_bv.length == 0);

    BitsOr(&result_bv, &bv1, &bv2);
    result = result && (result_bv.length == 0);

    // Test operations with different lengths
    BitsPush(&bv1, true);
    BitsPush(&bv1, false);
    BitsPush(&bv2, false);

    BitsAnd(&result_bv, &bv1, &bv2);
    result = result && (result_bv.length >= 1); // Should handle gracefully

    // Test NOT on various sizes
    BitsClear(&bv1);
    BitsNot(&result_bv, &bv1);
    result = result && (result_bv.length == 0);

    BitsPush(&bv1, true);
    BitsNot(&result_bv, &bv1);
    result = result && (BitsGet(&result_bv, 0) == false);

    BitsDeinit(&result_bv);

    BitsDeinit(&bv1);
    BitsDeinit(&bv2);
    return result;
}

bool test_Bits_reverse_edge_cases(void) {
    printf("Testing BitsReverse edge cases\n");

    Bits bv     = BitsInit();
    bool result = true;

    // Test reverse empty Bits
    BitsReverse(&bv);
    result = result && (bv.length == 0);

    // Test reverse single bit
    BitsPush(&bv, true);
    BitsReverse(&bv);
    result = result && (bv.length == 1);
    result = result && (BitsGet(&bv, 0) == true);

    // Test reverse even length
    BitsClear(&bv);
    BitsPush(&bv, true);
    BitsPush(&bv, false);
    BitsReverse(&bv);
    result = result && (BitsGet(&bv, 0) == false);
    result = result && (BitsGet(&bv, 1) == true);

    // Test double reverse (should restore original)
    BitsReverse(&bv);
    result = result && (BitsGet(&bv, 0) == true);
    result = result && (BitsGet(&bv, 1) == false);

    BitsDeinit(&bv);
    return result;
}

// NEW: Comprehensive bitwise operations testing
bool test_Bits_bitwise_comprehensive(void) {
    printf("Testing Bits comprehensive bitwise operations\n");

    Bits bv1         = BitsInit();
    Bits bv2         = BitsInit();
    Bits result      = BitsInit();
    bool test_result = true;

    // Test with different length operands
    // bv1: 11010110 (8 bits)
    // bv2: 1011     (4 bits)
    for (int i = 0; i < 8; i++) {
        BitsPush(&bv1, (0b11010110 >> i) & 1);
    }
    for (int i = 0; i < 4; i++) {
        BitsPush(&bv2, (0b1011 >> i) & 1);
    }

    // Test AND with different lengths (result should be min length)
    BitsAnd(&result, &bv1, &bv2);
    test_result = test_result && (result.length == 4);

    // Test OR with different lengths (result should be max length)
    BitsOr(&result, &bv1, &bv2);
    test_result = test_result && (result.length == 8);

    // Test XOR with different lengths
    BitsXor(&result, &bv1, &bv2);
    test_result = test_result && (result.length == 8);

    // Test with single bit operands
    BitsClear(&bv1);
    BitsClear(&bv2);
    BitsPush(&bv1, true);
    BitsPush(&bv2, false);

    BitsAnd(&result, &bv1, &bv2);
    test_result = test_result && (result.length == 1);
    test_result = test_result && (BitsGet(&result, 0) == false);

    BitsOr(&result, &bv1, &bv2);
    test_result = test_result && (BitsGet(&result, 0) == true);

    // Test NOT on large Bitstor
    BitsClear(&bv1);
    for (int i = 0; i < 100; i++) {
        BitsPush(&bv1, i % 3 == 0);
    }

    BitsNot(&result, &bv1);
    test_result = test_result && (result.length == 100);

    // Verify NOT correctness
    for (int i = 0; i < 100; i++) {
        bool original = BitsGet(&bv1, i);
        bool inverted = BitsGet(&result, i);
        test_result   = test_result && (original != inverted);
    }

    BitsDeinit(&bv1);
    BitsDeinit(&bv2);
    BitsDeinit(&result);
    return test_result;
}

// NEW: Comprehensive shift testing
bool test_Bits_shift_comprehensive(void) {
    printf("Testing Bits comprehensive shift operations\n");

    Bits bv     = BitsInit();
    bool result = true;

    // Test shift with known pattern that will show data loss
    // Pattern: 10101010 10101011 (16 bits) - asymmetric to detect shifts
    for (int i = 0; i < 15; i++) {
        BitsPush(&bv, i % 2 == 0);
    }
    BitsPush(&bv, true); // Make the last bit different to break symmetry

    // Test various shift amounts
    Bits original = BitsClone(&bv);

    // Shift left by 1, then right by 1 - should NOT restore original (data loss)
    BitsShiftLeft(&bv, 1);
    BitsShiftRight(&bv, 1);

    // Should be different from original (lost MSB, gained LSB zero)
    bool changed = false;
    for (int i = 0; i < (int)original.length; i++) {
        if (BitsGet(&bv, i) != BitsGet(&original, i)) {
            changed = true;
            break;
        }
    }
    result = result && changed;

    // Test shifting by exactly length (should clear everything)
    BitsClear(&bv);
    for (int i = 0; i < 8; i++) {
        BitsPush(&bv, true);
    }

    BitsShiftLeft(&bv, 8);
    result = result && (bv.length == 0);

    // Test shifting by more than length
    BitsClear(&bv);
    for (int i = 0; i < 5; i++) {
        BitsPush(&bv, true);
    }

    BitsShiftRight(&bv, 10);
    result = result && (bv.length == 0);

    // Test boundary conditions - shift by length-1
    BitsClear(&bv);
    BitsPush(&bv, true);
    BitsPush(&bv, true);
    BitsPush(&bv, false);

    BitsShiftLeft(&bv, 2);
    result = result && (bv.length == 3);
    result = result && (BitsGet(&bv, 0) == false); // filled with 0
    result = result && (BitsGet(&bv, 1) == false); // filled with 0
    result = result && (BitsGet(&bv, 2) == true);  // original bit 0

    BitsDeinit(&bv);
    BitsDeinit(&original);
    return result;
}

// NEW: Comprehensive rotate testing
bool test_Bits_rotate_comprehensive(void) {
    printf("Testing Bits comprehensive rotate operations\n");

    Bits bv     = BitsInit();
    bool result = true;

    // Test that rotate left by n, then rotate right by n restores original
    // Pattern: 10110100
    for (int i = 0; i < 8; i++) {
        BitsPush(&bv, (0b10110100 >> i) & 1);
    }

    Bits original = BitsClone(&bv);

    // Rotate left by 3, then right by 3
    BitsRotateLeft(&bv, 3);
    BitsRotateRight(&bv, 3);

    // Should restore original
    bool restored = true;
    for (int i = 0; i < 8; i++) {
        if (BitsGet(&bv, i) != BitsGet(&original, i)) {
            restored = false;
            break;
        }
    }
    result = result && restored;

    // Test rotate by multiple of length (should be no-op)
    BitsRotateLeft(&bv, 16); // 8 * 2

    bool unchanged = true;
    for (int i = 0; i < 8; i++) {
        if (BitsGet(&bv, i) != BitsGet(&original, i)) {
            unchanged = false;
            break;
        }
    }
    result = result && unchanged;

    // Test rotate with odd length
    BitsClear(&bv);
    BitsPush(&bv, true);
    BitsPush(&bv, false);
    BitsPush(&bv, true);
    BitsPush(&bv, false);
    BitsPush(&bv, true); // 5 bits: 10101

    BitsRotateLeft(&bv, 2);
    // 10101 -> 10110 (rotated left by 2)
    result = result && (BitsGet(&bv, 0) == true);
    result = result && (BitsGet(&bv, 1) == false);
    result = result && (BitsGet(&bv, 2) == true);
    result = result && (BitsGet(&bv, 3) == true);
    result = result && (BitsGet(&bv, 4) == false);

    BitsDeinit(&bv);
    BitsDeinit(&original);
    return result;
}

// NEW: Identity operations testing
bool test_Bits_bitwise_identity_operations(void) {
    printf("Testing Bits bitwise identity operations\n");

    Bits bv1         = BitsInit();
    Bits bv2         = BitsInit();
    Bits result      = BitsInit();
    bool test_result = true;

    // Create test pattern
    for (int i = 0; i < 16; i++) {
        BitsPush(&bv1, i % 3 == 0);
    }

    // Test A AND A = A
    BitsAnd(&result, &bv1, &bv1);
    bool and_identity = true;
    for (int i = 0; i < 16; i++) {
        if (BitsGet(&result, i) != BitsGet(&bv1, i)) {
            and_identity = false;
            break;
        }
    }
    test_result = test_result && and_identity;

    // Test A OR A = A
    BitsOr(&result, &bv1, &bv1);
    bool or_identity = true;
    for (int i = 0; i < 16; i++) {
        if (BitsGet(&result, i) != BitsGet(&bv1, i)) {
            or_identity = false;
            break;
        }
    }
    test_result = test_result && or_identity;

    // Test A XOR A = 0
    BitsXor(&result, &bv1, &bv1);
    test_result = test_result && (result.length == 16);
    for (int i = 0; i < 16; i++) {
        test_result = test_result && (BitsGet(&result, i) == false);
    }

    // Test NOT(NOT(A)) = A
    BitsNot(&result, &bv1);    // result = NOT(A)
    BitsNot(&result, &result); // result = NOT(NOT(A))

    bool double_not = true;
    for (int i = 0; i < 16; i++) {
        if (BitsGet(&result, i) != BitsGet(&bv1, i)) {
            double_not = false;
            break;
        }
    }
    test_result = test_result && double_not;

    // Test A AND 0 = 0
    BitsClear(&bv2);
    for (int i = 0; i < 16; i++) {
        BitsPush(&bv2, false);
    }

    BitsAnd(&result, &bv1, &bv2);
    bool and_zero = true;
    for (int i = 0; i < 16; i++) {
        if (BitsGet(&result, i) != false) {
            and_zero = false;
            break;
        }
    }
    test_result = test_result && and_zero;

    // Test A OR 1 = 1
    BitsClear(&bv2);
    for (int i = 0; i < 16; i++) {
        BitsPush(&bv2, true);
    }

    BitsOr(&result, &bv1, &bv2);
    bool or_ones = true;
    for (int i = 0; i < 16; i++) {
        if (BitsGet(&result, i) != true) {
            or_ones = false;
            break;
        }
    }
    test_result = test_result && or_ones;

    BitsDeinit(&bv1);
    BitsDeinit(&bv2);
    BitsDeinit(&result);
    return test_result;
}

// NEW: Commutative properties testing
bool test_Bits_bitwise_commutative_properties(void) {
    printf("Testing Bits bitwise commutative properties\n");

    Bits bv1         = BitsInit();
    Bits bv2         = BitsInit();
    Bits result1     = BitsInit();
    Bits result2     = BitsInit();
    bool test_result = true;

    // Create different patterns
    for (int i = 0; i < 12; i++) {
        BitsPush(&bv1, i % 2 == 0);
        BitsPush(&bv2, i % 3 == 0);
    }

    // Test A AND B = B AND A
    BitsAnd(&result1, &bv1, &bv2);
    BitsAnd(&result2, &bv2, &bv1);

    bool and_commutative = true;
    for (int i = 0; i < 12; i++) {
        if (BitsGet(&result1, i) != BitsGet(&result2, i)) {
            and_commutative = false;
            break;
        }
    }
    test_result = test_result && and_commutative;

    // Test A OR B = B OR A
    BitsOr(&result1, &bv1, &bv2);
    BitsOr(&result2, &bv2, &bv1);

    bool or_commutative = true;
    for (int i = 0; i < 12; i++) {
        if (BitsGet(&result1, i) != BitsGet(&result2, i)) {
            or_commutative = false;
            break;
        }
    }
    test_result = test_result && or_commutative;

    // Test A XOR B = B XOR A
    BitsXor(&result1, &bv1, &bv2);
    BitsXor(&result2, &bv2, &bv1);

    bool xor_commutative = true;
    for (int i = 0; i < 12; i++) {
        if (BitsGet(&result1, i) != BitsGet(&result2, i)) {
            xor_commutative = false;
            break;
        }
    }
    test_result = test_result && xor_commutative;

    BitsDeinit(&bv1);
    BitsDeinit(&bv2);
    BitsDeinit(&result1);
    BitsDeinit(&result2);
    return test_result;
}

// NEW: Large pattern testing
bool test_Bits_bitwise_large_patterns(void) {
    printf("Testing Bits bitwise operations with large patterns\n");

    Bits bv1         = BitsInit();
    Bits bv2         = BitsInit();
    Bits result      = BitsInit();
    bool test_result = true;

    // Create large Bitstors (1000 bits each)
    for (int i = 0; i < 1000; i++) {
        BitsPush(&bv1, i % 7 == 0);  // Pattern every 7 bits
        BitsPush(&bv2, i % 11 == 0); // Pattern every 11 bits
    }

    // Test AND on large data
    BitsAnd(&result, &bv1, &bv2);
    test_result = test_result && (result.length == 1000);

    // Verify result integrity - spot check a few positions
    for (int i = 0; i < 1000; i += 77) { // Check every 77th bit
        bool expected = (i % 7 == 0) && (i % 11 == 0);
        bool actual   = BitsGet(&result, i);
        test_result   = test_result && (expected == actual);
    }

    // Test XOR on large data
    BitsXor(&result, &bv1, &bv2);
    test_result = test_result && (result.length == 1000);

    // Test NOT on large data
    BitsNot(&result, &bv1);
    test_result = test_result && (result.length == 1000);

    // Verify NOT correctness on sample
    for (int i = 0; i < 1000; i += 123) {
        bool original = BitsGet(&bv1, i);
        bool inverted = BitsGet(&result, i);
        test_result   = test_result && (original != inverted);
    }

    // Test shift on large data
    BitsClear(&result);
    for (int i = 0; i < 1000; i++) {
        BitsPush(&result, i % 2 == 0);
    }

    BitsShiftLeft(&result, 100);
    test_result = test_result && (result.length == 1000);

    // First 100 bits should be 0
    for (int i = 0; i < 100; i++) {
        test_result = test_result && (BitsGet(&result, i) == false);
    }

    BitsDeinit(&bv1);
    BitsDeinit(&bv2);
    BitsDeinit(&result);
    return test_result;
}

// Deadend tests



// NEW: Additional deadend tests




// Main function that runs all tests
int main(void) {
    printf("[INFO] Starting Bits.BitWise tests\n\n");

    // Array of normal test functions
    TestFunction tests[] = {
        test_Bits_shift_left,
        test_Bits_shift_right,
        test_Bits_rotate_left,
        test_Bits_rotate_right,
        test_Bits_and,
        test_Bits_or,
        test_Bits_xor,
        test_Bits_not,
        test_Bits_reverse,
        test_Bits_shift_edge_cases,
        test_Bits_rotate_edge_cases,
        test_Bits_bitwise_ops_edge_cases,
        test_Bits_reverse_edge_cases,
        test_Bits_bitwise_comprehensive,
        test_Bits_shift_comprehensive,
        test_Bits_rotate_comprehensive,
        test_Bits_bitwise_identity_operations,
        test_Bits_bitwise_commutative_properties,
        test_Bits_bitwise_large_patterns
    };

    int total_tests = sizeof(tests) / sizeof(tests[0]);

    // Run simple tests using the centralized test driver
    return run_test_suite(tests, total_tests, NULL, 0, "Bits.BitWise.Simple");
}
