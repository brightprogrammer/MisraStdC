#include <Misra/Std/Container/BitVec.h>
#include <Misra/Std/Allocator/Default.h>
#include <Misra/Std/Log.h>

#include <Misra/Types.h>

// Include test utilities
#include "../Util/TestRunner.h"

// Function prototypes
bool test_bitvec_and(void);
bool test_bitvec_or(void);
bool test_bitvec_xor(void);
bool test_bitvec_not(void);
bool test_bitvec_shift_left(void);
bool test_bitvec_shift_right(void);
bool test_bitvec_rotate_left(void);
bool test_bitvec_rotate_right(void);
bool test_bitvec_reverse(void);
bool test_bitvec_shift_edge_cases(void);
bool test_bitvec_rotate_edge_cases(void);
bool test_bitvec_bitwise_ops_edge_cases(void);
bool test_bitvec_reverse_edge_cases(void);
bool test_bitvec_bitwise_comprehensive(void);
bool test_bitvec_shift_comprehensive(void);
bool test_bitvec_rotate_comprehensive(void);
bool test_bitvec_bitwise_identity_operations(void);
bool test_bitvec_bitwise_commutative_properties(void);
bool test_bitvec_bitwise_large_patterns(void);

// Test BitVecAnd function
bool test_bitvec_and(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmtLn("Testing BitVecAnd");

    BitVec bv1    = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVec bv2    = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVec result = BitVecInit(ALLOCATOR_OF(&alloc));

    // Set up first bitvector: 1101
    BitVecPush(&bv1, true);
    BitVecPush(&bv1, true);
    BitVecPush(&bv1, false);
    BitVecPush(&bv1, true);

    // Set up second bitvector: 1010
    BitVecPush(&bv2, true);
    BitVecPush(&bv2, false);
    BitVecPush(&bv2, true);
    BitVecPush(&bv2, false);

    // Perform AND operation
    BitVecAnd(&result, &bv1, &bv2);

    // Expected result: 1000 (1101 AND 1010)
    bool test_result = (BitVecLen(&result) == 4);
    test_result      = test_result && (BitVecGet(&result, 0) == true);
    test_result      = test_result && (BitVecGet(&result, 1) == false);
    test_result      = test_result && (BitVecGet(&result, 2) == false);
    test_result      = test_result && (BitVecGet(&result, 3) == false);

    // Clean up
    BitVecDeinit(&bv1);
    BitVecDeinit(&bv2);
    BitVecDeinit(&result);

    DefaultAllocatorDeinit(&alloc);

    return test_result;
}

// Test BitVecOr function
bool test_bitvec_or(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmtLn("Testing BitVecOr");

    BitVec bv1    = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVec bv2    = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVec result = BitVecInit(ALLOCATOR_OF(&alloc));

    // Set up first bitvector: 1100
    BitVecPush(&bv1, true);
    BitVecPush(&bv1, true);
    BitVecPush(&bv1, false);
    BitVecPush(&bv1, false);

    // Set up second bitvector: 1010
    BitVecPush(&bv2, true);
    BitVecPush(&bv2, false);
    BitVecPush(&bv2, true);
    BitVecPush(&bv2, false);

    // Perform OR operation
    BitVecOr(&result, &bv1, &bv2);

    // Expected result: 1110 (1100 OR 1010)
    bool test_result = (BitVecLen(&result) == 4);
    test_result      = test_result && (BitVecGet(&result, 0) == true);
    test_result      = test_result && (BitVecGet(&result, 1) == true);
    test_result      = test_result && (BitVecGet(&result, 2) == true);
    test_result      = test_result && (BitVecGet(&result, 3) == false);

    // Clean up
    BitVecDeinit(&bv1);
    BitVecDeinit(&bv2);
    BitVecDeinit(&result);

    DefaultAllocatorDeinit(&alloc);

    return test_result;
}

// Test BitVecXor function
bool test_bitvec_xor(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmtLn("Testing BitVecXor");

    BitVec bv1    = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVec bv2    = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVec result = BitVecInit(ALLOCATOR_OF(&alloc));

    // Set up first bitvector: 1100
    BitVecPush(&bv1, true);
    BitVecPush(&bv1, true);
    BitVecPush(&bv1, false);
    BitVecPush(&bv1, false);

    // Set up second bitvector: 1010
    BitVecPush(&bv2, true);
    BitVecPush(&bv2, false);
    BitVecPush(&bv2, true);
    BitVecPush(&bv2, false);

    // Perform XOR operation
    BitVecXor(&result, &bv1, &bv2);

    // Expected result: 0110 (1100 XOR 1010)
    bool test_result = (BitVecLen(&result) == 4);
    test_result      = test_result && (BitVecGet(&result, 0) == false);
    test_result      = test_result && (BitVecGet(&result, 1) == true);
    test_result      = test_result && (BitVecGet(&result, 2) == true);
    test_result      = test_result && (BitVecGet(&result, 3) == false);

    // Clean up
    BitVecDeinit(&bv1);
    BitVecDeinit(&bv2);
    BitVecDeinit(&result);

    DefaultAllocatorDeinit(&alloc);

    return test_result;
}

// Test BitVecNot function
bool test_bitvec_not(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmtLn("Testing BitVecNot");

    BitVec bv     = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVec result = BitVecInit(ALLOCATOR_OF(&alloc));

    // Set up bitvector: 1010
    BitVecPush(&bv, true);
    BitVecPush(&bv, false);
    BitVecPush(&bv, true);
    BitVecPush(&bv, false);

    // Perform NOT operation
    BitVecNot(&result, &bv);

    // Expected result: 0101 (NOT 1010)
    bool test_result = (BitVecLen(&result) == 4);
    test_result      = test_result && (BitVecGet(&result, 0) == false);
    test_result      = test_result && (BitVecGet(&result, 1) == true);
    test_result      = test_result && (BitVecGet(&result, 2) == false);
    test_result      = test_result && (BitVecGet(&result, 3) == true);

    // Clean up
    BitVecDeinit(&bv);
    BitVecDeinit(&result);

    DefaultAllocatorDeinit(&alloc);

    return test_result;
}

// Test BitVecShiftLeft function - CORRECTED EXPECTATIONS
bool test_bitvec_shift_left(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmtLn("Testing BitVecShiftLeft");

    BitVec bv = BitVecInit(ALLOCATOR_OF(&alloc));

    // Set up bitvector: 1011 (indices 0,1,2,3)
    BitVecPush(&bv, true);  // index 0
    BitVecPush(&bv, false); // index 1
    BitVecPush(&bv, true);  // index 2
    BitVecPush(&bv, true);  // index 3

    // Shift left by 2 positions
    // Original: 1011 (bit 0=1, bit 1=0, bit 2=1, bit 3=1)
    // After shift left by 2: bits move to higher indices, lower indices filled with 0
    // New: 0011 (bit 0=0, bit 1=0, bit 2=1, bit 3=0) - but we shift out the high bits
    BitVecShiftLeft(&bv, 2);

    // After shift left by 2, implementation should clear bits that shift out
    // and fill lower positions with 0
    bool test_result = (BitVecLen(&bv) == 4);

    // Let me trace through the implementation:
    // Original: bit[0]=1, bit[1]=0, bit[2]=1, bit[3]=1
    // Shift left by 2 means bits move to higher indices:
    // New bit[2] = old bit[0] = 1
    // New bit[3] = old bit[1] = 0
    // New bit[0] = 0 (filled)
    // New bit[1] = 0 (filled)
    // Expected result: 0010
    test_result = test_result && (BitVecGet(&bv, 0) == false);
    test_result = test_result && (BitVecGet(&bv, 1) == false);
    test_result = test_result && (BitVecGet(&bv, 2) == true);
    test_result = test_result && (BitVecGet(&bv, 3) == false);

    // Clean up
    BitVecDeinit(&bv);

    DefaultAllocatorDeinit(&alloc);

    return test_result;
}

// Test BitVecShiftRight function - CORRECTED EXPECTATIONS
bool test_bitvec_shift_right(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmtLn("Testing BitVecShiftRight");

    BitVec bv = BitVecInit(ALLOCATOR_OF(&alloc));

    // Set up bitvector: 1011
    BitVecPush(&bv, true);  // index 0
    BitVecPush(&bv, false); // index 1
    BitVecPush(&bv, true);  // index 2
    BitVecPush(&bv, true);  // index 3

    // Shift right by 2 positions
    // Original: 1011 (bit 0=1, bit 1=0, bit 2=1, bit 3=1)
    // After shift right by 2: bits move to lower indices
    // New bit[0] = old bit[2] = 1
    // New bit[1] = old bit[3] = 1
    // New bit[2] = 0 (filled)
    // New bit[3] = 0 (filled)
    // Expected result: 1100
    BitVecShiftRight(&bv, 2);

    bool test_result = (BitVecLen(&bv) == 4);
    test_result      = test_result && (BitVecGet(&bv, 0) == true);
    test_result      = test_result && (BitVecGet(&bv, 1) == true);
    test_result      = test_result && (BitVecGet(&bv, 2) == false);
    test_result      = test_result && (BitVecGet(&bv, 3) == false);

    // Clean up
    BitVecDeinit(&bv);

    DefaultAllocatorDeinit(&alloc);

    return test_result;
}

// Test BitVecRotateLeft function
bool test_bitvec_rotate_left(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmtLn("Testing BitVecRotateLeft");

    BitVec bv = BitVecInit(ALLOCATOR_OF(&alloc));

    // Set up bitvector: 1011
    BitVecPush(&bv, true);
    BitVecPush(&bv, false);
    BitVecPush(&bv, true);
    BitVecPush(&bv, true);

    // Rotate left by 2 positions
    BitVecRotateLeft(&bv, 2);

    // Expected result: 1110 (1011 rotated left by 2)
    bool test_result = (BitVecLen(&bv) == 4);
    test_result      = test_result && (BitVecGet(&bv, 0) == true);
    test_result      = test_result && (BitVecGet(&bv, 1) == true);
    test_result      = test_result && (BitVecGet(&bv, 2) == true);
    test_result      = test_result && (BitVecGet(&bv, 3) == false);

    // Clean up
    BitVecDeinit(&bv);

    DefaultAllocatorDeinit(&alloc);

    return test_result;
}

// Test BitVecRotateRight function
bool test_bitvec_rotate_right(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmtLn("Testing BitVecRotateRight");

    BitVec bv = BitVecInit(ALLOCATOR_OF(&alloc));

    // Set up bitvector: 1011
    BitVecPush(&bv, true);
    BitVecPush(&bv, false);
    BitVecPush(&bv, true);
    BitVecPush(&bv, true);

    // Rotate right by 1 position
    BitVecRotateRight(&bv, 1);

    // Expected result: 1101 (1011 rotated right by 1)
    bool test_result = (BitVecLen(&bv) == 4);
    test_result      = test_result && (BitVecGet(&bv, 0) == true);
    test_result      = test_result && (BitVecGet(&bv, 1) == true);
    test_result      = test_result && (BitVecGet(&bv, 2) == false);
    test_result      = test_result && (BitVecGet(&bv, 3) == true);

    // Clean up
    BitVecDeinit(&bv);

    DefaultAllocatorDeinit(&alloc);

    return test_result;
}

// Test BitVecReverse function
bool test_bitvec_reverse(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmtLn("Testing BitVecReverse");

    BitVec bv = BitVecInit(ALLOCATOR_OF(&alloc));

    // Set up bitvector: 1011
    BitVecPush(&bv, true);
    BitVecPush(&bv, false);
    BitVecPush(&bv, true);
    BitVecPush(&bv, true);

    // Reverse the bits
    BitVecReverse(&bv);

    // Expected result: 1101 (1011 reversed)
    bool test_result = (BitVecLen(&bv) == 4);
    test_result      = test_result && (BitVecGet(&bv, 0) == true);
    test_result      = test_result && (BitVecGet(&bv, 1) == true);
    test_result      = test_result && (BitVecGet(&bv, 2) == false);
    test_result      = test_result && (BitVecGet(&bv, 3) == true);

    // Clean up
    BitVecDeinit(&bv);

    DefaultAllocatorDeinit(&alloc);

    return test_result;
}

// Edge case tests
bool test_bitvec_shift_edge_cases(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmtLn("Testing BitVec shift edge cases");

    BitVec bv     = BitVecInit(ALLOCATOR_OF(&alloc));
    bool   result = true;

    // Test shift empty bitvec
    BitVecShiftLeft(&bv, 5);
    result = result && (BitVecLen(&bv) == 0);

    BitVecShiftRight(&bv, 3);
    result = result && (BitVecLen(&bv) == 0);

    // Test shift by 0 (should be no-op)
    BitVecPush(&bv, true);
    BitVecPush(&bv, false);
    BitVecShiftLeft(&bv, 0);
    result = result && (BitVecLen(&bv) == 2);
    result = result && (BitVecGet(&bv, 0) == true);

    // Test shift larger than length (should clear all bits)
    BitVecShiftLeft(&bv, 10);
    result = result && (BitVecLen(&bv) == 0); // Should clear when shifting everything out

    // Test large data shift
    BitVecClear(&bv);
    for (int i = 0; i < 1000; i++) {
        BitVecPush(&bv, i % 2 == 0);
    }
    BitVecShiftLeft(&bv, 1);
    result = result && (BitVecLen(&bv) == 1000);

    BitVecDeinit(&bv);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

bool test_bitvec_rotate_edge_cases(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmtLn("Testing BitVec rotate edge cases");

    BitVec bv     = BitVecInit(ALLOCATOR_OF(&alloc));
    bool   result = true;

    // Test rotate empty bitvec
    BitVecRotateLeft(&bv, 5);
    result = result && (BitVecLen(&bv) == 0);

    // Test rotate by 0
    BitVecPush(&bv, true);
    BitVecRotateRight(&bv, 0);
    result = result && (BitVecGet(&bv, 0) == true);

    // Test rotate by length (should be no-op)
    BitVecPush(&bv, false);
    BitVecRotateLeft(&bv, 2);
    result = result && (BitVecLen(&bv) == 2);

    // Test large rotate amount
    BitVecRotateRight(&bv, 1000);
    result = result && (BitVecLen(&bv) == 2);

    BitVecDeinit(&bv);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

bool test_bitvec_bitwise_ops_edge_cases(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmtLn("Testing BitVec bitwise operations edge cases");

    BitVec bv1    = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVec bv2    = BitVecInit(ALLOCATOR_OF(&alloc));
    bool   result = true;

    // Test operations on empty bitvecs
    BitVec result_bv = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVecAnd(&result_bv, &bv1, &bv2);
    result = result && (BitVecLen(&result_bv) == 0);

    BitVecOr(&result_bv, &bv1, &bv2);
    result = result && (BitVecLen(&result_bv) == 0);

    // Test operations with different lengths
    BitVecPush(&bv1, true);
    BitVecPush(&bv1, false);
    BitVecPush(&bv2, false);

    BitVecAnd(&result_bv, &bv1, &bv2);
    result = result && (BitVecLen(&result_bv) >= 1); // Should handle gracefully

    // Test NOT on various sizes
    BitVecClear(&bv1);
    BitVecNot(&result_bv, &bv1);
    result = result && (BitVecLen(&result_bv) == 0);

    BitVecPush(&bv1, true);
    BitVecNot(&result_bv, &bv1);
    result = result && (BitVecGet(&result_bv, 0) == false);

    BitVecDeinit(&result_bv);

    BitVecDeinit(&bv1);
    BitVecDeinit(&bv2);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

bool test_bitvec_reverse_edge_cases(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmtLn("Testing BitVecReverse edge cases");

    BitVec bv     = BitVecInit(ALLOCATOR_OF(&alloc));
    bool   result = true;

    // Test reverse empty bitvec
    BitVecReverse(&bv);
    result = result && (BitVecLen(&bv) == 0);

    // Test reverse single bit
    BitVecPush(&bv, true);
    BitVecReverse(&bv);
    result = result && (BitVecLen(&bv) == 1);
    result = result && (BitVecGet(&bv, 0) == true);

    // Test reverse even length
    BitVecClear(&bv);
    BitVecPush(&bv, true);
    BitVecPush(&bv, false);
    BitVecReverse(&bv);
    result = result && (BitVecGet(&bv, 0) == false);
    result = result && (BitVecGet(&bv, 1) == true);

    // Test double reverse (should restore original)
    BitVecReverse(&bv);
    result = result && (BitVecGet(&bv, 0) == true);
    result = result && (BitVecGet(&bv, 1) == false);

    BitVecDeinit(&bv);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// NEW: Comprehensive bitwise operations testing
bool test_bitvec_bitwise_comprehensive(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmtLn("Testing BitVec comprehensive bitwise operations");

    BitVec bv1         = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVec bv2         = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVec result      = BitVecInit(ALLOCATOR_OF(&alloc));
    bool   test_result = true;

    // Test with different length operands
    // bv1: 11010110 (8 bits)
    // bv2: 1011     (4 bits)
    for (int i = 0; i < 8; i++) {
        BitVecPush(&bv1, (0b11010110 >> i) & 1);
    }
    for (int i = 0; i < 4; i++) {
        BitVecPush(&bv2, (0b1011 >> i) & 1);
    }

    // Test AND with different lengths (result should be min length)
    BitVecAnd(&result, &bv1, &bv2);
    test_result = test_result && (BitVecLen(&result) == 4);

    // Test OR with different lengths (result should be max length)
    BitVecOr(&result, &bv1, &bv2);
    test_result = test_result && (BitVecLen(&result) == 8);

    // Test XOR with different lengths
    BitVecXor(&result, &bv1, &bv2);
    test_result = test_result && (BitVecLen(&result) == 8);

    // Test with single bit operands
    BitVecClear(&bv1);
    BitVecClear(&bv2);
    BitVecPush(&bv1, true);
    BitVecPush(&bv2, false);

    BitVecAnd(&result, &bv1, &bv2);
    test_result = test_result && (BitVecLen(&result) == 1);
    test_result = test_result && (BitVecGet(&result, 0) == false);

    BitVecOr(&result, &bv1, &bv2);
    test_result = test_result && (BitVecGet(&result, 0) == true);

    // Test NOT on large bitvector
    BitVecClear(&bv1);
    for (int i = 0; i < 100; i++) {
        BitVecPush(&bv1, i % 3 == 0);
    }

    BitVecNot(&result, &bv1);
    test_result = test_result && (BitVecLen(&result) == 100);

    // Verify NOT correctness
    for (int i = 0; i < 100; i++) {
        bool original = BitVecGet(&bv1, i);
        bool inverted = BitVecGet(&result, i);
        test_result   = test_result && (original != inverted);
    }

    BitVecDeinit(&bv1);
    BitVecDeinit(&bv2);
    BitVecDeinit(&result);
    DefaultAllocatorDeinit(&alloc);
    return test_result;
}

// NEW: Comprehensive shift testing
bool test_bitvec_shift_comprehensive(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmtLn("Testing BitVec comprehensive shift operations");

    BitVec bv     = BitVecInit(ALLOCATOR_OF(&alloc));
    bool   result = true;

    // Test shift with known pattern that will show data loss
    // Pattern: 10101010 10101011 (16 bits) - asymmetric to detect shifts
    for (int i = 0; i < 15; i++) {
        BitVecPush(&bv, i % 2 == 0);
    }
    BitVecPush(&bv, true); // Make the last bit different to break symmetry

    // Test various shift amounts
    BitVec original = BitVecClone(&bv);

    // Shift left by 1, then right by 1 - should NOT restore original (data loss)
    BitVecShiftLeft(&bv, 1);
    BitVecShiftRight(&bv, 1);

    // Should be different from original (lost MSB, gained LSB zero)
    bool changed = false;
    for (int i = 0; i < (int)BitVecLen(&original); i++) {
        if (BitVecGet(&bv, i) != BitVecGet(&original, i)) {
            changed = true;
            break;
        }
    }
    result = result && changed;

    // Test shifting by exactly length (should clear everything)
    BitVecClear(&bv);
    for (int i = 0; i < 8; i++) {
        BitVecPush(&bv, true);
    }

    BitVecShiftLeft(&bv, 8);
    result = result && (BitVecLen(&bv) == 0);

    // Test shifting by more than length
    BitVecClear(&bv);
    for (int i = 0; i < 5; i++) {
        BitVecPush(&bv, true);
    }

    BitVecShiftRight(&bv, 10);
    result = result && (BitVecLen(&bv) == 0);

    // Test boundary conditions - shift by length-1
    BitVecClear(&bv);
    BitVecPush(&bv, true);
    BitVecPush(&bv, true);
    BitVecPush(&bv, false);

    BitVecShiftLeft(&bv, 2);
    result = result && (BitVecLen(&bv) == 3);
    result = result && (BitVecGet(&bv, 0) == false); // filled with 0
    result = result && (BitVecGet(&bv, 1) == false); // filled with 0
    result = result && (BitVecGet(&bv, 2) == true);  // original bit 0

    BitVecDeinit(&bv);
    BitVecDeinit(&original);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// NEW: Comprehensive rotate testing
bool test_bitvec_rotate_comprehensive(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmtLn("Testing BitVec comprehensive rotate operations");

    BitVec bv     = BitVecInit(ALLOCATOR_OF(&alloc));
    bool   result = true;

    // Test that rotate left by n, then rotate right by n restores original
    // Pattern: 10110100
    for (int i = 0; i < 8; i++) {
        BitVecPush(&bv, (0b10110100 >> i) & 1);
    }

    BitVec original = BitVecClone(&bv);

    // Rotate left by 3, then right by 3
    BitVecRotateLeft(&bv, 3);
    BitVecRotateRight(&bv, 3);

    // Should restore original
    bool restored = true;
    for (int i = 0; i < 8; i++) {
        if (BitVecGet(&bv, i) != BitVecGet(&original, i)) {
            restored = false;
            break;
        }
    }
    result = result && restored;

    // Test rotate by multiple of length (should be no-op)
    BitVecRotateLeft(&bv, 16); // 8 * 2

    bool unchanged = true;
    for (int i = 0; i < 8; i++) {
        if (BitVecGet(&bv, i) != BitVecGet(&original, i)) {
            unchanged = false;
            break;
        }
    }
    result = result && unchanged;

    // Test rotate with odd length
    BitVecClear(&bv);
    BitVecPush(&bv, true);
    BitVecPush(&bv, false);
    BitVecPush(&bv, true);
    BitVecPush(&bv, false);
    BitVecPush(&bv, true); // 5 bits: 10101

    BitVecRotateLeft(&bv, 2);
    // 10101 -> 10110 (rotated left by 2)
    result = result && (BitVecGet(&bv, 0) == true);
    result = result && (BitVecGet(&bv, 1) == false);
    result = result && (BitVecGet(&bv, 2) == true);
    result = result && (BitVecGet(&bv, 3) == true);
    result = result && (BitVecGet(&bv, 4) == false);

    BitVecDeinit(&bv);
    BitVecDeinit(&original);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// NEW: Identity operations testing
bool test_bitvec_bitwise_identity_operations(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmtLn("Testing BitVec bitwise identity operations");

    BitVec bv1         = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVec bv2         = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVec result      = BitVecInit(ALLOCATOR_OF(&alloc));
    bool   test_result = true;

    // Create test pattern
    for (int i = 0; i < 16; i++) {
        BitVecPush(&bv1, i % 3 == 0);
    }

    // Test A AND A = A
    BitVecAnd(&result, &bv1, &bv1);
    bool and_identity = true;
    for (int i = 0; i < 16; i++) {
        if (BitVecGet(&result, i) != BitVecGet(&bv1, i)) {
            and_identity = false;
            break;
        }
    }
    test_result = test_result && and_identity;

    // Test A OR A = A
    BitVecOr(&result, &bv1, &bv1);
    bool or_identity = true;
    for (int i = 0; i < 16; i++) {
        if (BitVecGet(&result, i) != BitVecGet(&bv1, i)) {
            or_identity = false;
            break;
        }
    }
    test_result = test_result && or_identity;

    // Test A XOR A = 0
    BitVecXor(&result, &bv1, &bv1);
    test_result = test_result && (BitVecLen(&result) == 16);
    for (int i = 0; i < 16; i++) {
        test_result = test_result && (BitVecGet(&result, i) == false);
    }

    // Test NOT(NOT(A)) = A
    BitVecNot(&result, &bv1);    // result = NOT(A)
    BitVecNot(&result, &result); // result = NOT(NOT(A))

    bool double_not = true;
    for (int i = 0; i < 16; i++) {
        if (BitVecGet(&result, i) != BitVecGet(&bv1, i)) {
            double_not = false;
            break;
        }
    }
    test_result = test_result && double_not;

    // Test A AND 0 = 0
    BitVecClear(&bv2);
    for (int i = 0; i < 16; i++) {
        BitVecPush(&bv2, false);
    }

    BitVecAnd(&result, &bv1, &bv2);
    bool and_zero = true;
    for (int i = 0; i < 16; i++) {
        if (BitVecGet(&result, i) != false) {
            and_zero = false;
            break;
        }
    }
    test_result = test_result && and_zero;

    // Test A OR 1 = 1
    BitVecClear(&bv2);
    for (int i = 0; i < 16; i++) {
        BitVecPush(&bv2, true);
    }

    BitVecOr(&result, &bv1, &bv2);
    bool or_ones = true;
    for (int i = 0; i < 16; i++) {
        if (BitVecGet(&result, i) != true) {
            or_ones = false;
            break;
        }
    }
    test_result = test_result && or_ones;

    BitVecDeinit(&bv1);
    BitVecDeinit(&bv2);
    BitVecDeinit(&result);
    DefaultAllocatorDeinit(&alloc);
    return test_result;
}

// NEW: Commutative properties testing
bool test_bitvec_bitwise_commutative_properties(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmtLn("Testing BitVec bitwise commutative properties");

    BitVec bv1         = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVec bv2         = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVec result1     = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVec result2     = BitVecInit(ALLOCATOR_OF(&alloc));
    bool   test_result = true;

    // Create different patterns
    for (int i = 0; i < 12; i++) {
        BitVecPush(&bv1, i % 2 == 0);
        BitVecPush(&bv2, i % 3 == 0);
    }

    // Test A AND B = B AND A
    BitVecAnd(&result1, &bv1, &bv2);
    BitVecAnd(&result2, &bv2, &bv1);

    bool and_commutative = true;
    for (int i = 0; i < 12; i++) {
        if (BitVecGet(&result1, i) != BitVecGet(&result2, i)) {
            and_commutative = false;
            break;
        }
    }
    test_result = test_result && and_commutative;

    // Test A OR B = B OR A
    BitVecOr(&result1, &bv1, &bv2);
    BitVecOr(&result2, &bv2, &bv1);

    bool or_commutative = true;
    for (int i = 0; i < 12; i++) {
        if (BitVecGet(&result1, i) != BitVecGet(&result2, i)) {
            or_commutative = false;
            break;
        }
    }
    test_result = test_result && or_commutative;

    // Test A XOR B = B XOR A
    BitVecXor(&result1, &bv1, &bv2);
    BitVecXor(&result2, &bv2, &bv1);

    bool xor_commutative = true;
    for (int i = 0; i < 12; i++) {
        if (BitVecGet(&result1, i) != BitVecGet(&result2, i)) {
            xor_commutative = false;
            break;
        }
    }
    test_result = test_result && xor_commutative;

    BitVecDeinit(&bv1);
    BitVecDeinit(&bv2);
    BitVecDeinit(&result1);
    BitVecDeinit(&result2);
    DefaultAllocatorDeinit(&alloc);
    return test_result;
}

// NEW: Large pattern testing
bool test_bitvec_bitwise_large_patterns(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmtLn("Testing BitVec bitwise operations with large patterns");

    BitVec bv1         = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVec bv2         = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVec result      = BitVecInit(ALLOCATOR_OF(&alloc));
    bool   test_result = true;

    // Create large bitvectors (1000 bits each)
    for (int i = 0; i < 1000; i++) {
        BitVecPush(&bv1, i % 7 == 0);  // Pattern every 7 bits
        BitVecPush(&bv2, i % 11 == 0); // Pattern every 11 bits
    }

    // Test AND on large data
    BitVecAnd(&result, &bv1, &bv2);
    test_result = test_result && (BitVecLen(&result) == 1000);

    // Verify result integrity - spot check a few positions
    for (int i = 0; i < 1000; i += 77) { // Check every 77th bit
        bool expected = (i % 7 == 0) && (i % 11 == 0);
        bool actual   = BitVecGet(&result, i);
        test_result   = test_result && (expected == actual);
    }

    // Test XOR on large data
    BitVecXor(&result, &bv1, &bv2);
    test_result = test_result && (BitVecLen(&result) == 1000);

    // Test NOT on large data
    BitVecNot(&result, &bv1);
    test_result = test_result && (BitVecLen(&result) == 1000);

    // Verify NOT correctness on sample
    for (int i = 0; i < 1000; i += 123) {
        bool original = BitVecGet(&bv1, i);
        bool inverted = BitVecGet(&result, i);
        test_result   = test_result && (original != inverted);
    }

    // Test shift on large data
    BitVecClear(&result);
    for (int i = 0; i < 1000; i++) {
        BitVecPush(&result, i % 2 == 0);
    }

    BitVecShiftLeft(&result, 100);
    test_result = test_result && (BitVecLen(&result) == 1000);

    // First 100 bits should be 0
    for (int i = 0; i < 100; i++) {
        test_result = test_result && (BitVecGet(&result, i) == false);
    }

    BitVecDeinit(&bv1);
    BitVecDeinit(&bv2);
    BitVecDeinit(&result);
    DefaultAllocatorDeinit(&alloc);
    return test_result;
}

// Deadend tests
bool test_bitvec_bitwise_null_failures(void) {
    WriteFmtLn("Testing BitVec bitwise NULL pointer handling");

    // Test NULL bitvec pointer - should abort
    BitVecShiftLeft(NULL, 1);

    return false;
}

bool test_bitvec_bitwise_ops_null_failures(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmtLn("Testing BitVec bitwise operations NULL handling");

    BitVec bv  = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVec bv2 = BitVecInit(ALLOCATOR_OF(&alloc));

    // Test NULL pointer - should abort
    BitVecAnd(NULL, &bv, &bv2);

    BitVecDeinit(&bv);
    BitVecDeinit(&bv2);
    DefaultAllocatorDeinit(&alloc);
    return false;
}

bool test_bitvec_reverse_null_failures(void) {
    WriteFmtLn("Testing BitVec reverse NULL handling");

    // Test NULL pointer - should abort
    BitVecReverse(NULL);

    return false;
}

// NEW: Additional deadend tests
bool test_bitvec_shift_ops_null_failures(void) {
    WriteFmtLn("Testing BitVec shift operations NULL handling");

    // Test NULL pointer for shift right - should abort
    BitVecShiftRight(NULL, 5);

    return false;
}

bool test_bitvec_rotate_ops_null_failures(void) {
    WriteFmtLn("Testing BitVec rotate operations NULL handling");

    // Test NULL pointer for rotate - should abort
    BitVecRotateLeft(NULL, 3);

    return false;
}

bool test_bitvec_and_result_null_failures(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmtLn("Testing BitVec AND with NULL result handling");

    BitVec bv1 = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVec bv2 = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVecPush(&bv1, true);
    BitVecPush(&bv2, false);

    // Test NULL result pointer - should abort
    BitVecAnd(NULL, &bv1, &bv2);

    BitVecDeinit(&bv1);
    BitVecDeinit(&bv2);
    DefaultAllocatorDeinit(&alloc);
    return false;
}

bool test_bitvec_or_operand_null_failures(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmtLn("Testing BitVec OR with NULL operand handling");

    BitVec result = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVec bv1    = BitVecInit(ALLOCATOR_OF(&alloc));

    // Test NULL operand - should abort
    BitVecOr(&result, &bv1, NULL);

    BitVecDeinit(&result);
    BitVecDeinit(&bv1);
    DefaultAllocatorDeinit(&alloc);
    return false;
}

bool test_bitvec_xor_second_operand_null_failures(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmtLn("Testing BitVec XOR with NULL second operand handling");

    BitVec result = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVec bv1    = BitVecInit(ALLOCATOR_OF(&alloc));

    // Test NULL second operand - should abort
    BitVecXor(&result, NULL, &bv1);

    BitVecDeinit(&result);
    BitVecDeinit(&bv1);
    DefaultAllocatorDeinit(&alloc);
    return false;
}

bool test_bitvec_not_null_failures(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmtLn("Testing BitVec NOT with NULL handling");

    BitVec result = BitVecInit(ALLOCATOR_OF(&alloc));

    // Test NULL operand - should abort
    BitVecNot(&result, NULL);

    BitVecDeinit(&result);
    DefaultAllocatorDeinit(&alloc);
    return false;
}

// Main function that runs all tests
int main(void) {
    WriteFmtLn("[INFO] Starting BitVec.BitWise tests");

    // Array of normal test functions
    TestFunction tests[] = {
        test_bitvec_shift_left,
        test_bitvec_shift_right,
        test_bitvec_rotate_left,
        test_bitvec_rotate_right,
        test_bitvec_and,
        test_bitvec_or,
        test_bitvec_xor,
        test_bitvec_not,
        test_bitvec_reverse,
        test_bitvec_shift_edge_cases,
        test_bitvec_rotate_edge_cases,
        test_bitvec_bitwise_ops_edge_cases,
        test_bitvec_reverse_edge_cases,
        test_bitvec_bitwise_comprehensive,
        test_bitvec_shift_comprehensive,
        test_bitvec_rotate_comprehensive,
        test_bitvec_bitwise_identity_operations,
        test_bitvec_bitwise_commutative_properties,
        test_bitvec_bitwise_large_patterns
    };

    // Array of deadend test functions
    TestFunction deadend_tests[] = {
        test_bitvec_bitwise_null_failures,
        test_bitvec_bitwise_ops_null_failures,
        test_bitvec_reverse_null_failures,
        test_bitvec_shift_ops_null_failures,
        test_bitvec_rotate_ops_null_failures,
        test_bitvec_and_result_null_failures,
        test_bitvec_or_operand_null_failures,
        test_bitvec_xor_second_operand_null_failures,
        test_bitvec_not_null_failures
    };

    int total_tests         = sizeof(tests) / sizeof(tests[0]);
    int total_deadend_tests = sizeof(deadend_tests) / sizeof(deadend_tests[0]);

    // Run all tests using the centralized test driver
    return run_test_suite(tests, total_tests, deadend_tests, total_deadend_tests, "BitVec.BitWise");
}
