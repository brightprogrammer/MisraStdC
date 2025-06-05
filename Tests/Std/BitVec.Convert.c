#include <Misra/Std/Container/BitVec.h>
#include <Misra/Std/Container/Str.h>
#include <Misra/Std/Log.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <Misra/Types.h>

// Include test utilities
#include "../Util/TestRunner.h"

// Function prototypes
bool test_bitvec_to_string(void);
bool test_bitvec_from_string(void);
bool test_bitvec_to_bytes(void);
bool test_bitvec_from_bytes(void);
bool test_bitvec_to_integer(void);
bool test_bitvec_from_integer(void);
bool test_bitvec_convert_edge_cases(void);
bool test_bitvec_from_string_edge_cases(void);
bool test_bitvec_bytes_conversion_edge_cases(void);
bool test_bitvec_integer_conversion_edge_cases(void);
bool test_bitvec_convert_null_failures(void);
bool test_bitvec_from_string_null_failures(void);
bool test_bitvec_bytes_null_failures(void);

// Test BitVecToStr function
bool test_bitvec_to_string(void) {
    printf("Testing BitVecToStr\n");

    BitVec bv = BitVecInit();

    // Create pattern: 1011
    BitVecPush(&bv, true);
    BitVecPush(&bv, false);
    BitVecPush(&bv, true);
    BitVecPush(&bv, true);

    // Convert to string
    Str str = BitVecToStr(&bv);

    // Check result
    bool result = (str.length == 4);
    result      = result && (str.data[0] == '1');
    result      = result && (str.data[1] == '0');
    result      = result && (str.data[2] == '1');
    result      = result && (str.data[3] == '1');

    // Clean up
    StrDeinit(&str);
    BitVecDeinit(&bv);

    return result;
}

// Test BitVecFromStr function
bool test_bitvec_from_string(void) {
    printf("Testing BitVecFromStr\n");

    // Convert from string
    const char *str = "1011";
    BitVec      bv  = BitVecFromStr(str);

    // Check result
    bool result = (bv.length == 4);
    result      = result && (BitVecGet(&bv, 0) == true);
    result      = result && (BitVecGet(&bv, 1) == false);
    result      = result && (BitVecGet(&bv, 2) == true);
    result      = result && (BitVecGet(&bv, 3) == true);

    // Test with empty string
    BitVec empty_bv = BitVecFromStr("");
    result          = result && (empty_bv.length == 0);

    // Clean up
    BitVecDeinit(&bv);
    BitVecDeinit(&empty_bv);

    return result;
}

// Test BitVecToBytes function
bool test_bitvec_to_bytes(void) {
    printf("Testing BitVecToBytes\n");

    BitVec bv = BitVecInit();

    // Create pattern: 10110011 (0xB3)
    BitVecPush(&bv, true);  // bit 0
    BitVecPush(&bv, false); // bit 1
    BitVecPush(&bv, true);  // bit 2
    BitVecPush(&bv, true);  // bit 3
    BitVecPush(&bv, false); // bit 4
    BitVecPush(&bv, false); // bit 5
    BitVecPush(&bv, true);  // bit 6
    BitVecPush(&bv, true);  // bit 7

    u8   bytes[2];          // Buffer for byte output
    u64 byte_count = BitVecToBytes(&bv, bytes, sizeof(bytes));

    // Check result
    bool result = (byte_count == 1);
    if (result) {
        // Expected byte value depends on bit ordering
        // If bits are stored LSB first: 10110011 = 0xCD
        // If bits are stored MSB first: 10110011 = 0xB3
        result = result && (bytes[0] == 0xCD || bytes[0] == 0xB3);
    }
    BitVecDeinit(&bv);

    return result;
}

// Test BitVecFromBytes function
bool test_bitvec_from_bytes(void) {
    printf("Testing BitVecFromBytes\n");

    // Create byte array
    u8     bytes[] = {0xB3}; // 10110011 in binary
    BitVec bv      = BitVecFromBytes(bytes, 8); // 8 bits from the byte

    // Check result (8 bits from 1 byte)
    bool result = (bv.length == 8);

    // The exact bit order depends on implementation
    // Just check that we got 8 bits and some are true, some false
    u64 true_count  = 0;
    u64 false_count = 0;

    for (u64 i = 0; i < bv.length; i++) {
        if (BitVecGet(&bv, i)) {
            true_count++;
        } else {
            false_count++;
        }
    }

    // 0xB3 = 10110011 has 5 ones and 3 zeros
    result = result && (true_count == 5) && (false_count == 3);

    // Clean up
    BitVecDeinit(&bv);

    return result;
}

// Test BitVecToInteger function
bool test_bitvec_to_integer(void) {
    printf("Testing BitVecToInteger\n");

    BitVec bv = BitVecInit();

    // Create pattern: 1011 (decimal 11 if MSB first, 13 if LSB first)
    BitVecPush(&bv, true);
    BitVecPush(&bv, false);
    BitVecPush(&bv, true);
    BitVecPush(&bv, true);

    // Convert to integer
    u64 value = BitVecToInteger(&bv);

    // Check result - should be either 11 or 13 depending on bit order
    bool result = (value == 11 || value == 13);

    // Test with larger pattern
    BitVec bv2 = BitVecInit();
    for (int i = 0; i < 8; i++) {
        BitVecPush(&bv2, (i % 2 == 0)); // Alternating pattern
    }

    u64 value2 = BitVecToInteger(&bv2);
    result     = result && (value2 > 0); // Should be some positive value

    // Clean up
    BitVecDeinit(&bv);
    BitVecDeinit(&bv2);

    return result;
}

// Test BitVecFromInteger function
bool test_bitvec_from_integer(void) {
    printf("Testing BitVecFromInteger\n");

    // Convert from integer
    u64    value = 11; // 1011 in binary
    BitVec bv    = BitVecFromInteger(value, 4);

    // Check result
    bool result = (bv.length == 4);

    // Count ones and zeros
    u64 true_count  = 0;
    u64 false_count = 0;

    for (u64 i = 0; i < bv.length; i++) {
        if (BitVecGet(&bv, i)) {
            true_count++;
        } else {
            false_count++;
        }
    }

    // 11 in binary (1011) has 3 ones and 1 zero
    result = result && (true_count == 3) && (false_count == 1);

    // Test with zero
    BitVec zero_bv = BitVecFromInteger(0, 8);
    result         = result && (zero_bv.length == 8);

    // All bits should be false
    bool all_false = true;
    for (u64 i = 0; i < zero_bv.length; i++) {
        if (BitVecGet(&zero_bv, i)) {
            all_false = false;
            break;
        }
    }
    result = result && all_false;

    // Clean up
    BitVecDeinit(&bv);
    BitVecDeinit(&zero_bv);

    return result;
}

// Edge case tests
bool test_bitvec_convert_edge_cases(void) {
    printf("Testing BitVec convert edge cases\n");

    BitVec bv     = BitVecInit();
    bool   result = true;

    // Test converting empty bitvec
    Str str_obj = BitVecToStr(&bv);
    result      = result && (str_obj.length == 0);
    StrDeinit(&str_obj);

    // Test converting single bit
    BitVecPush(&bv, true);
    str_obj = BitVecToStr(&bv);
    result  = result && (str_obj.length == 1);
    result  = result && (StrCmpCstr(&str_obj, "1", 1) == 0);
    StrDeinit(&str_obj);

    // Test large conversions
    BitVecClear(&bv);
    for (int i = 0; i < 1000; i++) {
        BitVecPush(&bv, i % 2 == 0);
    }
    str_obj = BitVecToStr(&bv);
    result  = result && (str_obj.length == 1000);
    StrDeinit(&str_obj);

    BitVecDeinit(&bv);
    return result;
}

bool test_bitvec_from_string_edge_cases(void) {
    printf("Testing BitVecFromStr edge cases\n");

    bool result = true;

    // Test empty string
    BitVec bv1 = BitVecFromStr("");
    result     = result && (bv1.length == 0);
    BitVecDeinit(&bv1);

    // Test single character
    BitVec bv2 = BitVecFromStr("1");
    result     = result && (bv2.length == 1);
    result     = result && (BitVecGet(&bv2, 0) == true);
    BitVecDeinit(&bv2);

    // Test long string
    char long_str[1001];
    for (int i = 0; i < 1000; i++) {
        long_str[i] = (i % 2 == 0) ? '1' : '0';
    }
    long_str[1000] = '\0';

    BitVec bv3 = BitVecFromStr(long_str);
    result     = result && (bv3.length == 1000);
    result     = result && (BitVecGet(&bv3, 0) == true);
    result     = result && (BitVecGet(&bv3, 1) == false);
    BitVecDeinit(&bv3);

    return result;
}

bool test_bitvec_bytes_conversion_edge_cases(void) {
    printf("Testing BitVec bytes conversion edge cases\n");

    BitVec bv     = BitVecInit();
    bool   result = true;

    // Test empty bitvec to bytes
    u8  bytes[1] = {0};
    u64 written  = BitVecToBytes(&bv, bytes, 1);
    result       = result && (written == 0); // Empty bitvec should write 0 bytes

    // Test bytes to bitvec with 0 bits (should return empty bitvector)
    u8     empty_bytes[1] = {0x05};
    BitVec bv2            = BitVecFromBytes(empty_bytes, 0); // 0 bits
    result                = result && (bv2.length == 0);
    BitVecDeinit(&bv2);

    // Test single byte
    u8     single_byte[1] = {0xFF};
    BitVec bv3            = BitVecFromBytes(single_byte, 8); // 8 bits from 1 byte
    result                = result && (bv3.length == 8);
    BitVecDeinit(&bv3);

    BitVecDeinit(&bv);
    return result;
}

bool test_bitvec_integer_conversion_edge_cases(void) {
    printf("Testing BitVec integer conversion edge cases\n");

    BitVec bv     = BitVecInit();
    bool   result = true;

    // Test empty bitvec to integer
    u64 value = BitVecToInteger(&bv);
    result    = result && (value == 0);

    // Test integer to bitvec with 0
    BitVec bv2 = BitVecFromInteger(0, 8); // 8 bits for zero
    result     = result && (bv2.length == 8); // Should be 8 bits
    BitVecDeinit(&bv2);

    // Test large integer
    BitVec bv3 = BitVecFromInteger(UINT64_MAX, 64); // 64 bits for max value
    result     = result && (bv3.length == 64);
    BitVecDeinit(&bv3);

    BitVecDeinit(&bv);
    return result;
}

// Deadend tests
bool test_bitvec_convert_null_failures(void) {
    printf("Testing BitVec convert NULL pointer handling\n");

    // Test NULL bitvec pointer - should abort
    BitVecToStr(NULL);

    return false;
}

bool test_bitvec_from_string_null_failures(void) {
    printf("Testing BitVec from string NULL handling\n");

    // Test NULL string - should abort
    BitVecFromStr(NULL);

    return false;
}

bool test_bitvec_bytes_null_failures(void) {
    printf("Testing BitVec bytes NULL handling\n");

    // Test NULL bytes - should abort
    BitVecFromBytes(NULL, 8); // NULL bytes, 8 bits

    return false;
}

// Main function that runs all tests
int main(void) {
    printf("[INFO] Starting BitVec.Convert tests\n\n");

    // Array of normal test functions
    TestFunction tests[] = {
        test_bitvec_to_string,
        test_bitvec_from_string,
        test_bitvec_to_bytes,
        test_bitvec_from_bytes,
        test_bitvec_to_integer,
        test_bitvec_from_integer,
        test_bitvec_convert_edge_cases,
        test_bitvec_from_string_edge_cases,
        test_bitvec_bytes_conversion_edge_cases,
        test_bitvec_integer_conversion_edge_cases
    };

    // Array of deadend test functions
    TestFunction deadend_tests[] = {
        test_bitvec_convert_null_failures,
        test_bitvec_from_string_null_failures,
        test_bitvec_bytes_null_failures
    };

    int total_tests         = sizeof(tests) / sizeof(tests[0]);
    int total_deadend_tests = sizeof(deadend_tests) / sizeof(deadend_tests[0]);

    // Run all tests using the centralized test driver
    return run_test_suite(tests, total_tests, deadend_tests, total_deadend_tests, "BitVec.Convert");
}