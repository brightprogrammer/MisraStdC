#include <Misra/Std/Container/Bits.h>
#include <Misra/Std/Log.h>
#include <stdio.h>
#include <Misra/Types.h>

// Include test utilities
#include "../Util/TestRunner.h"

// Function prototypes
bool test_Bits_push(void);
bool test_Bits_insert_single(void);
bool test_Bits_insert_range(void);
bool test_Bits_insert_multiple(void);
bool test_Bits_insert_pattern(void);
bool test_Bits_insert_range_edge_cases(void);
bool test_Bits_insert_multiple_edge_cases(void);
bool test_Bits_insert_pattern_edge_cases(void);
bool test_Bits_insert_null_failures(void);
bool test_Bits_insert_invalid_range_failures(void);
bool test_Bits_insert_pattern_null_failures(void);

// Test BitsPush function
bool test_Bits_push(void) {
    printf("Testing BitsPush\n");

    Bits bv = BitsInit();

    // Push some bits
    BitsPush(&bv, true);
    BitsPush(&bv, false);
    BitsPush(&bv, true);
    BitsPush(&bv, true);
    BitsPush(&bv, false);

    // Check length
    bool result = (bv.length == 5);

    // Check each bit
    result = result && (BitsGet(&bv, 0) == true);
    result = result && (BitsGet(&bv, 1) == false);
    result = result && (BitsGet(&bv, 2) == true);
    result = result && (BitsGet(&bv, 3) == true);
    result = result && (BitsGet(&bv, 4) == false);

    // Clean up
    BitsDeinit(&bv);

    return result;
}

// Test BitsInsert single bit function
bool test_Bits_insert_single(void) {
    printf("Testing BitsInsert (single bit)\n");

    Bits bv = BitsInit();

    // Insert at index 0 (empty Bitstor)
    BitsInsert(&bv, 0, true);

    // Check first bit
    bool result = (bv.length == 1 && BitsGet(&bv, 0) == true);

    // Insert at the end
    BitsInsert(&bv, 1, false);

    // Check bits
    result = result && (bv.length == 2);
    result = result && (BitsGet(&bv, 0) == true);
    result = result && (BitsGet(&bv, 1) == false);

    // Insert in the middle
    BitsInsert(&bv, 1, true);

    // Check all bits
    result = result && (bv.length == 3);
    result = result && (BitsGet(&bv, 0) == true);
    result = result && (BitsGet(&bv, 1) == true);
    result = result && (BitsGet(&bv, 2) == false);

    // Clean up
    BitsDeinit(&bv);

    return result;
}

// Test BitsInsertRange function
bool test_Bits_insert_range(void) {
    printf("Testing BitsInsertRange\n");

    Bits bv = BitsInit();

    // Create target Bitstor
    BitsPush(&bv, false);
    BitsPush(&bv, false);

    // Insert range of true bits
    BitsInsertRange(&bv, 1, 3, true);

    // Check result: false, true, true, true, false
    bool result = (bv.length == 5);
    result      = result && (BitsGet(&bv, 0) == false);
    result      = result && (BitsGet(&bv, 1) == true);
    result      = result && (BitsGet(&bv, 2) == true);
    result      = result && (BitsGet(&bv, 3) == true);
    result      = result && (BitsGet(&bv, 4) == false);

    // Clean up
    BitsDeinit(&bv);

    return result;
}

// Test BitsInsertMultiple function
bool test_Bits_insert_multiple(void) {
    printf("Testing BitsInsertMultiple\n");

    Bits bv     = BitsInit();
    Bits source = BitsInit();

    // Start with some bits
    BitsPush(&bv, true);
    BitsPush(&bv, false);

    // Create source Bitstor to insert
    BitsPush(&source, true);
    BitsPush(&source, true);
    BitsPush(&source, true);

    // Insert multiple bits from source
    BitsInsertMultiple(&bv, 1, &source);

    // Check result: true, true, true, true, false
    bool result = (bv.length == 5);
    result      = result && (BitsGet(&bv, 0) == true);
    result      = result && (BitsGet(&bv, 1) == true);
    result      = result && (BitsGet(&bv, 2) == true);
    result      = result && (BitsGet(&bv, 3) == true);
    result      = result && (BitsGet(&bv, 4) == false);

    // Clean up
    BitsDeinit(&bv);
    BitsDeinit(&source);

    return result;
}

// Test BitsInsertPattern function
bool test_Bits_insert_pattern(void) {
    printf("Testing BitsInsertPattern\n");

    Bits bv = BitsInit();

    // Start with some bits
    BitsPush(&bv, false);
    BitsPush(&bv, false);

    // Insert pattern 0x0B (1011 in binary) using 4 bits
    u8 pattern = 0x0B;
    BitsInsertPattern(&bv, 1, pattern, 4);

    // Check result: false, true, false, true, true, false
    // Pattern 1011 gets inserted as individual bits
    bool result = (bv.length == 6);
    result      = result && (BitsGet(&bv, 0) == false); // original
    result      = result && (BitsGet(&bv, 1) == true);  // bit 0 of pattern (LSB)
    result      = result && (BitsGet(&bv, 2) == true);  // bit 1 of pattern
    result      = result && (BitsGet(&bv, 3) == false); // bit 2 of pattern
    result      = result && (BitsGet(&bv, 4) == true);  // bit 3 of pattern (MSB)
    result      = result && (BitsGet(&bv, 5) == false); // original

    // Test with different pattern - 0x05 (0101 in binary) using only 3 bits
    Bits bv2 = BitsInit();
    BitsPush(&bv2, true);

    u8 pattern2 = 0x05;
    BitsInsertPattern(&bv2, 0, pattern2, 3);

    // Check result: true, false, true, true (3 bits: 101)
    result = result && (bv2.length == 4);
    result = result && (BitsGet(&bv2, 0) == true);  // bit 0 of pattern (LSB)
    result = result && (BitsGet(&bv2, 1) == false); // bit 1 of pattern
    result = result && (BitsGet(&bv2, 2) == true);  // bit 2 of pattern
    result = result && (BitsGet(&bv2, 3) == true);  // original

    // Clean up
    BitsDeinit(&bv);
    BitsDeinit(&bv2);

    return result;
}

// Edge case tests
bool test_Bits_insert_range_edge_cases(void) {
    printf("Testing BitsInsertRange edge cases\n");

    Bits bv     = BitsInit();
    bool result = true;

    // Test inserting 0 bits (should be no-op)
    BitsInsertRange(&bv, 0, 0, true);
    result = result && (bv.length == 0);

    // Test inserting at end
    BitsPush(&bv, true);
    BitsInsertRange(&bv, 1, 2, false);
    result = result && (bv.length == 3);
    result = result && (BitsGet(&bv, 1) == false);
    result = result && (BitsGet(&bv, 2) == false);

    // Test large range insertion
    BitsClear(&bv);
    BitsInsertRange(&bv, 0, 1000, true);
    result = result && (bv.length == 1000);
    result = result && (BitsGet(&bv, 0) == true);
    result = result && (BitsGet(&bv, 999) == true);

    BitsDeinit(&bv);
    return result;
}

bool test_Bits_insert_multiple_edge_cases(void) {
    printf("Testing BitsInsertMultiple edge cases\n");

    Bits bv     = BitsInit();
    Bits empty  = BitsInit();
    Bits source = BitsInit();
    bool result = true;

    // Test inserting empty Bits
    BitsInsertMultiple(&bv, 0, &empty);
    result = result && (bv.length == 0);

    // Test inserting single bit Bits
    BitsPush(&source, true);
    BitsInsertMultiple(&bv, 0, &source);
    result = result && (bv.length == 1) && (BitsGet(&bv, 0) == true);

    // Test inserting large Bits
    BitsClear(&source);
    for (int i = 0; i < 500; i++) {
        BitsPush(&source, false);
    }
    BitsInsertMultiple(&bv, 1, &source);
    result = result && (bv.length == 501);
    result = result && (BitsGet(&bv, 1) == false);
    result = result && (BitsGet(&bv, 500) == false);

    BitsDeinit(&bv);
    BitsDeinit(&empty);
    BitsDeinit(&source);
    return result;
}

bool test_Bits_insert_pattern_edge_cases(void) {
    printf("Testing BitsInsertPattern edge cases\n");

    Bits bv     = BitsInit();
    bool result = true;

    // Test inserting empty pattern (should be no-op)
    BitsInsertPattern(&bv, 0, 0x00, 0);
    result = result && (bv.length == 0);

    // Test inserting single bit pattern
    BitsInsertPattern(&bv, 0, 0x01, 1); // 1 bit pattern
    result = result && (bv.length == 1);

    // Test inserting 8-bit pattern
    BitsClear(&bv);
    BitsInsertPattern(&bv, 0, 0xAA, 8);            // 10101010 pattern
    result = result && (bv.length == 8);
    result = result && (BitsGet(&bv, 0) == false); // First bit of 0xAA
    result = result && (BitsGet(&bv, 1) == true);  // Second bit

    BitsDeinit(&bv);
    return result;
}

// Deadend tests
bool test_Bits_insert_null_failures(void) {
    printf("Testing Bits insert NULL pointer handling\n");

    // Test NULL Bits pointer - should abort
    BitsInsertRange(NULL, 0, 1, true);

    return false;
}

bool test_Bits_insert_invalid_range_failures(void) {
    printf("Testing Bits insert invalid range handling\n");

    Bits bv = BitsInit();

    // Test inserting beyond capacity limit - should abort
    BitsInsertRange(&bv, SIZE_MAX, 1, true);

    BitsDeinit(&bv);
    return false;
}

bool test_Bits_insert_pattern_null_failures(void) {
    printf("Testing Bits insert pattern NULL handling\n");

    // Test NULL Bits - should abort
    BitsInsertPattern(NULL, 0, 0xFF, 8);

    return false;
}

// Main function that runs all tests
int main(void) {
    printf("[INFO] Starting Bits.Insert tests\n\n");

    // Array of normal test functions
    TestFunction tests[] = {
        test_Bits_insert_range,
        test_Bits_insert_multiple,
        test_Bits_insert_pattern,
        test_Bits_insert_range_edge_cases,
        test_Bits_insert_multiple_edge_cases,
        test_Bits_insert_pattern_edge_cases
    };

    // Array of deadend test functions
    TestFunction deadend_tests[] = {
        test_Bits_insert_null_failures,
        test_Bits_insert_invalid_range_failures,
        test_Bits_insert_pattern_null_failures
    };

    int total_tests         = sizeof(tests) / sizeof(tests[0]);
    int total_deadend_tests = sizeof(deadend_tests) / sizeof(deadend_tests[0]);

    // Run all tests using the centralized test driver
    return run_test_suite(tests, total_tests, deadend_tests, total_deadend_tests, "Bits.Insert");
}
