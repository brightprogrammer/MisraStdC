#include <Misra/Std/Container/BitVec.h>
#include <Misra/Std/Allocator/Default.h>
#include <Misra/Std/Log.h>
#include <Misra/Types.h>

// Include test utilities
#include "../../Util/TestRunner.h"

// Function prototypes
bool test_bitvec_push(void);
bool test_bitvec_insert_single(void);
bool test_bitvec_insert_range(void);
bool test_bitvec_insert_multiple(void);
bool test_bitvec_insert_pattern(void);
bool test_bitvec_insert_range_edge_cases(void);
bool test_bitvec_insert_multiple_edge_cases(void);
bool test_bitvec_insert_pattern_edge_cases(void);
bool test_bitvec_insert_null_failures(void);
bool test_bitvec_insert_invalid_range_failures(void);
bool test_bitvec_insert_pattern_null_failures(void);
bool test_insert_range_shifts_existing_bits(void);
bool test_insert_multiple_shifts_existing_bits(void);
bool test_insert_null_aborts(void);
bool test_insert_multiple_null_bv_aborts(void);
bool test_insert_multiple_null_other_aborts(void);

// Test BitVecPush function
bool test_bitvec_push(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVecPush\n");

    BitVec bv = BitVecInit(ALLOCATOR_OF(&alloc));

    // Push some bits
    BitVecPush(&bv, true);
    BitVecPush(&bv, false);
    BitVecPush(&bv, true);
    BitVecPush(&bv, true);
    BitVecPush(&bv, false);

    // Check length
    bool result = (BitVecLen(&bv) == 5);

    // Check each bit
    result = result && (BitVecGet(&bv, 0) == true);
    result = result && (BitVecGet(&bv, 1) == false);
    result = result && (BitVecGet(&bv, 2) == true);
    result = result && (BitVecGet(&bv, 3) == true);
    result = result && (BitVecGet(&bv, 4) == false);

    // Clean up
    BitVecDeinit(&bv);

    DefaultAllocatorDeinit(&alloc);

    return result;
}

// Test BitVecInsert single bit function
bool test_bitvec_insert_single(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVecInsert (single bit)\n");

    BitVec bv = BitVecInit(ALLOCATOR_OF(&alloc));

    // Insert at index 0 (empty bitvector)
    BitVecInsert(&bv, 0, true);

    // Check first bit
    bool result = (BitVecLen(&bv) == 1 && BitVecGet(&bv, 0) == true);

    // Insert at the end
    BitVecInsert(&bv, 1, false);

    // Check bits
    result = result && (BitVecLen(&bv) == 2);
    result = result && (BitVecGet(&bv, 0) == true);
    result = result && (BitVecGet(&bv, 1) == false);

    // Insert in the middle
    BitVecInsert(&bv, 1, true);

    // Check all bits
    result = result && (BitVecLen(&bv) == 3);
    result = result && (BitVecGet(&bv, 0) == true);
    result = result && (BitVecGet(&bv, 1) == true);
    result = result && (BitVecGet(&bv, 2) == false);

    // Clean up
    BitVecDeinit(&bv);

    DefaultAllocatorDeinit(&alloc);

    return result;
}

// Test BitVecInsertRange function
bool test_bitvec_insert_range(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVecInsertRange\n");

    BitVec bv = BitVecInit(ALLOCATOR_OF(&alloc));

    // Create target bitvector
    BitVecPush(&bv, false);
    BitVecPush(&bv, false);

    // Insert range of true bits
    BitVecInsertRange(&bv, 1, 3, true);

    // Check result: false, true, true, true, false
    bool result = (BitVecLen(&bv) == 5);
    result      = result && (BitVecGet(&bv, 0) == false);
    result      = result && (BitVecGet(&bv, 1) == true);
    result      = result && (BitVecGet(&bv, 2) == true);
    result      = result && (BitVecGet(&bv, 3) == true);
    result      = result && (BitVecGet(&bv, 4) == false);

    // Clean up
    BitVecDeinit(&bv);

    DefaultAllocatorDeinit(&alloc);

    return result;
}

// Test BitVecInsertMultiple function
bool test_bitvec_insert_multiple(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVecInsertMultiple\n");

    BitVec bv     = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVec source = BitVecInit(ALLOCATOR_OF(&alloc));

    // Start with some bits
    BitVecPush(&bv, true);
    BitVecPush(&bv, false);

    // Create source bitvector to insert
    BitVecPush(&source, true);
    BitVecPush(&source, true);
    BitVecPush(&source, true);

    // Insert multiple bits from source
    BitVecInsertMultiple(&bv, 1, &source);

    // Check result: true, true, true, true, false
    bool result = (BitVecLen(&bv) == 5);
    result      = result && (BitVecGet(&bv, 0) == true);
    result      = result && (BitVecGet(&bv, 1) == true);
    result      = result && (BitVecGet(&bv, 2) == true);
    result      = result && (BitVecGet(&bv, 3) == true);
    result      = result && (BitVecGet(&bv, 4) == false);

    // Clean up
    BitVecDeinit(&bv);
    BitVecDeinit(&source);

    DefaultAllocatorDeinit(&alloc);

    return result;
}

// Test BitVecInsertPattern function
bool test_bitvec_insert_pattern(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVecInsertPattern\n");

    BitVec bv = BitVecInit(ALLOCATOR_OF(&alloc));

    // Start with some bits
    BitVecPush(&bv, false);
    BitVecPush(&bv, false);

    // Insert pattern 0x0B (1011 in binary) using 4 bits
    u8 pattern = 0x0B;
    BitVecInsertPattern(&bv, 1, pattern, 4);

    // Check result: false, true, false, true, true, false
    // Pattern 1011 gets inserted as individual bits
    bool result = (BitVecLen(&bv) == 6);
    result      = result && (BitVecGet(&bv, 0) == false); // original
    result      = result && (BitVecGet(&bv, 1) == true);  // bit 0 of pattern (LSB)
    result      = result && (BitVecGet(&bv, 2) == true);  // bit 1 of pattern
    result      = result && (BitVecGet(&bv, 3) == false); // bit 2 of pattern
    result      = result && (BitVecGet(&bv, 4) == true);  // bit 3 of pattern (MSB)
    result      = result && (BitVecGet(&bv, 5) == false); // original

    // Test with different pattern - 0x05 (0101 in binary) using only 3 bits
    BitVec bv2 = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVecPush(&bv2, true);

    u8 pattern2 = 0x05;
    BitVecInsertPattern(&bv2, 0, pattern2, 3);

    // Check result: true, false, true, true (3 bits: 101)
    result = result && (BitVecLen(&bv2) == 4);
    result = result && (BitVecGet(&bv2, 0) == true);  // bit 0 of pattern (LSB)
    result = result && (BitVecGet(&bv2, 1) == false); // bit 1 of pattern
    result = result && (BitVecGet(&bv2, 2) == true);  // bit 2 of pattern
    result = result && (BitVecGet(&bv2, 3) == true);  // original

    // Clean up
    BitVecDeinit(&bv);
    BitVecDeinit(&bv2);

    DefaultAllocatorDeinit(&alloc);

    return result;
}

// Edge case tests
bool test_bitvec_insert_range_edge_cases(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVecInsertRange edge cases\n");

    BitVec bv     = BitVecInit(ALLOCATOR_OF(&alloc));
    bool   result = true;

    // Test inserting 0 bits (should be no-op)
    BitVecInsertRange(&bv, 0, 0, true);
    result = result && (BitVecLen(&bv) == 0);

    // Test inserting at end
    BitVecPush(&bv, true);
    BitVecInsertRange(&bv, 1, 2, false);
    result = result && (BitVecLen(&bv) == 3);
    result = result && (BitVecGet(&bv, 1) == false);
    result = result && (BitVecGet(&bv, 2) == false);

    // Test large range insertion
    BitVecClear(&bv);
    BitVecInsertRange(&bv, 0, 1000, true);
    result = result && (BitVecLen(&bv) == 1000);
    result = result && (BitVecGet(&bv, 0) == true);
    result = result && (BitVecGet(&bv, 999) == true);

    BitVecDeinit(&bv);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

bool test_bitvec_insert_multiple_edge_cases(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVecInsertMultiple edge cases\n");

    BitVec bv     = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVec empty  = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVec source = BitVecInit(ALLOCATOR_OF(&alloc));
    bool   result = true;

    // Test inserting empty bitvec
    BitVecInsertMultiple(&bv, 0, &empty);
    result = result && (BitVecLen(&bv) == 0);

    // Test inserting single bit bitvec
    BitVecPush(&source, true);
    BitVecInsertMultiple(&bv, 0, &source);
    result = result && (BitVecLen(&bv) == 1) && (BitVecGet(&bv, 0) == true);

    // Test inserting large bitvec
    BitVecClear(&source);
    for (int i = 0; i < 500; i++) {
        BitVecPush(&source, false);
    }
    BitVecInsertMultiple(&bv, 1, &source);
    result = result && (BitVecLen(&bv) == 501);
    result = result && (BitVecGet(&bv, 1) == false);
    result = result && (BitVecGet(&bv, 500) == false);

    BitVecDeinit(&bv);
    BitVecDeinit(&empty);
    BitVecDeinit(&source);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

bool test_bitvec_insert_pattern_edge_cases(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVecInsertPattern edge cases\n");

    BitVec bv     = BitVecInit(ALLOCATOR_OF(&alloc));
    bool   result = true;

    // Test inserting empty pattern (should be no-op)
    BitVecInsertPattern(&bv, 0, 0x00, 0);
    result = result && (BitVecLen(&bv) == 0);

    // Test inserting single bit pattern
    BitVecInsertPattern(&bv, 0, 0x01, 1); // 1 bit pattern
    result = result && (BitVecLen(&bv) == 1);

    // Test inserting 8-bit pattern
    BitVecClear(&bv);
    BitVecInsertPattern(&bv, 0, 0xAA, 8);            // 10101010 pattern
    result = result && (BitVecLen(&bv) == 8);
    result = result && (BitVecGet(&bv, 0) == false); // First bit of 0xAA
    result = result && (BitVecGet(&bv, 1) == true);  // Second bit

    BitVecDeinit(&bv);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Deadend tests
bool test_bitvec_insert_null_failures(void) {
    WriteFmt("Testing BitVec insert NULL pointer handling\n");

    // Test NULL bitvec pointer - should abort
    BitVecInsertRange(NULL, 0, 1, true);

    return false;
}

bool test_bitvec_insert_invalid_range_failures(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVec insert invalid range handling\n");

    BitVec bv = BitVecInit(ALLOCATOR_OF(&alloc));

    // Test inserting beyond capacity limit - should abort
    BitVecInsertRange(&bv, SIZE_MAX, 1, true);

    BitVecDeinit(&bv);
    DefaultAllocatorDeinit(&alloc);
    return false;
}

bool test_bitvec_insert_pattern_null_failures(void) {
    WriteFmt("Testing BitVec insert pattern NULL handling\n");

    // Test NULL bitvec - should abort
    BitVecInsertPattern(NULL, 0, 0xFF, 8);

    return false;
}

// 354:9 remove_void_call -- BitVecInsertRange must shift the existing tail bits
// up by `count` positions. If the shift Set is dropped, the bits after the
// inserted region keep stale values.
bool test_insert_range_shifts_existing_bits(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVecInsertRange shifts existing tail bits\n");

    BitVec bv = BitVecInit(ALLOCATOR_OF(&alloc));

    // Original: [1, 0, 1, 1]
    BitVecPush(&bv, true);
    BitVecPush(&bv, false);
    BitVecPush(&bv, true);
    BitVecPush(&bv, true);

    // Insert 2 false bits at index 1 -> [1, 0, 0, 0, 1, 1]
    BitVecInsertRange(&bv, 1, 2, false);

    bool result = (BitVecLen(&bv) == 6);
    result      = result && (BitVecGet(&bv, 0) == true);
    result      = result && (BitVecGet(&bv, 1) == false); // inserted
    result      = result && (BitVecGet(&bv, 2) == false); // inserted
    result      = result && (BitVecGet(&bv, 3) == false); // shifted orig[1]
    result      = result && (BitVecGet(&bv, 4) == true);  // shifted orig[2]
    result      = result && (BitVecGet(&bv, 5) == true);  // shifted orig[3]

    BitVecDeinit(&bv);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// 382:9 remove_void_call -- BitVecInsertMultiple must shift the existing tail
// bits up by other->length. Dropping the shift Set leaves the tail stale.
bool test_insert_multiple_shifts_existing_bits(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVecInsertMultiple shifts existing tail bits\n");

    BitVec bv    = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVec other = BitVecInit(ALLOCATOR_OF(&alloc));

    // bv: [1, 0, 1]
    BitVecPush(&bv, true);
    BitVecPush(&bv, false);
    BitVecPush(&bv, true);

    // other: [1, 1]
    BitVecPush(&other, true);
    BitVecPush(&other, true);

    // Insert other at index 1 -> [1, 1, 1, 0, 1]
    BitVecInsertMultiple(&bv, 1, &other);

    bool result = (BitVecLen(&bv) == 5);
    result      = result && (BitVecGet(&bv, 0) == true);  // orig[0]
    result      = result && (BitVecGet(&bv, 1) == true);  // other[0]
    result      = result && (BitVecGet(&bv, 2) == true);  // other[1]
    result      = result && (BitVecGet(&bv, 3) == false); // shifted orig[1]
    result      = result && (BitVecGet(&bv, 4) == true);  // shifted orig[2]

    BitVecDeinit(&bv);
    BitVecDeinit(&other);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// 320:5 remove_void_call -- BitVecInsert must validate its bitvec argument.
// With validation removed, a NULL bitvec is accepted instead of aborting.
bool test_insert_null_aborts(void) {
    WriteFmt("Testing BitVecInsert NULL validation\n");

    BitVecInsert(NULL, 0, true);

    // If we reach here, validation did not abort.
    return false;
}

// 365:5 remove_void_call -- BitVecInsertMultiple must validate `bv`.
bool test_insert_multiple_null_bv_aborts(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVecInsertMultiple NULL bv validation\n");

    BitVec other = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVecPush(&other, true);

    BitVecInsertMultiple(NULL, 0, &other);

    // If we reach here, validation did not abort.
    BitVecDeinit(&other);
    DefaultAllocatorDeinit(&alloc);
    return false;
}

// 366:5 remove_void_call -- BitVecInsertMultiple must validate `other`.
bool test_insert_multiple_null_other_aborts(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVecInsertMultiple NULL other validation\n");

    BitVec bv = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVecPush(&bv, true);

    BitVecInsertMultiple(&bv, 0, NULL);

    // If we reach here, validation did not abort.
    BitVecDeinit(&bv);
    DefaultAllocatorDeinit(&alloc);
    return false;
}

// Main function that runs all tests
int main(void) {
    WriteFmt("[INFO] Starting BitVec.Insert tests\n\n");

    // Array of normal test functions
    TestFunction tests[] = {
        test_bitvec_push,
        test_bitvec_insert_single,
        test_bitvec_insert_range,
        test_bitvec_insert_multiple,
        test_bitvec_insert_pattern,
        test_bitvec_insert_range_edge_cases,
        test_bitvec_insert_multiple_edge_cases,
        test_bitvec_insert_pattern_edge_cases,
        test_insert_range_shifts_existing_bits,
        test_insert_multiple_shifts_existing_bits
    };

    // Array of deadend test functions
    TestFunction deadend_tests[] = {
        test_bitvec_insert_null_failures,
        test_bitvec_insert_invalid_range_failures,
        test_bitvec_insert_pattern_null_failures,
        test_insert_null_aborts,
        test_insert_multiple_null_bv_aborts,
        test_insert_multiple_null_other_aborts
    };

    int total_tests         = sizeof(tests) / sizeof(tests[0]);
    int total_deadend_tests = sizeof(deadend_tests) / sizeof(deadend_tests[0]);

    // Run all tests using the centralized test driver
    return run_test_suite(tests, total_tests, deadend_tests, total_deadend_tests, "BitVec.Insert");
}
