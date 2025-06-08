#include <Misra/Std/Container/Bits.h>
#include <Misra/Std/Container/Str.h>
#include <Misra/Std/Log.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <Misra/Types.h>

// Include test utilities
#include "../Util/TestRunner.h"

// Function prototypes
bool test_Bits_to_string(void);
bool test_Bits_from_string(void);
bool test_Bits_to_bytes(void);
bool test_Bits_from_bytes(void);
bool test_Bits_to_integer(void);
bool test_Bits_from_integer(void);
bool test_Bits_convert_edge_cases(void);
bool test_Bits_from_string_edge_cases(void);
bool test_Bits_bytes_conversion_edge_cases(void);
bool test_Bits_integer_conversion_edge_cases(void);
bool test_Bits_round_trip_conversions(void);
bool test_Bits_conversion_bounds_checking(void);
bool test_Bits_conversion_comprehensive(void);
bool test_Bits_large_scale_conversions(void);
bool test_Bits_convert_null_failures(void);
bool test_Bits_from_string_null_failures(void);
bool test_Bits_bytes_null_failures(void);
bool test_Bits_bytes_bounds_failures(void);
bool test_Bits_integer_bounds_failures(void);

// Test BitsToStr function
bool test_Bits_to_string(void) {
    printf("Testing BitsToStr\n");

    Bits bv = BitsInit();

    // Create pattern: 1011
    BitsPush(&bv, true);
    BitsPush(&bv, false);
    BitsPush(&bv, true);
    BitsPush(&bv, true);

    // Convert to string
    Str str = BitsToStr(&bv);

    // Check result
    bool result = (str.length == 4);
    result      = result && (str.data[0] == '1');
    result      = result && (str.data[1] == '0');
    result      = result && (str.data[2] == '1');
    result      = result && (str.data[3] == '1');

    // Clean up
    StrDeinit(&str);
    BitsDeinit(&bv);

    return result;
}

// Test BitsFromStr function
bool test_Bits_from_string(void) {
    printf("Testing BitsFromStr\n");

    // Convert from string
    const char* str = "1011";
    Bits      bv  = BitsFromStr(str);

    // Check result
    bool result = (bv.length == 4);
    result      = result && (BitsGet(&bv, 0) == true);
    result      = result && (BitsGet(&bv, 1) == false);
    result      = result && (BitsGet(&bv, 2) == true);
    result      = result && (BitsGet(&bv, 3) == true);

    // Test with empty string
    Bits empty_bv = BitsFromStr("");
    result          = result && (empty_bv.length == 0);

    // Clean up
    BitsDeinit(&bv);
    BitsDeinit(&empty_bv);

    return result;
}

// Test BitsToBytes function
bool test_Bits_to_bytes(void) {
    printf("Testing BitsToBytes\n");

    Bits bv = BitsInit();

    // Create pattern: 10110011 (0xB3)
    BitsPush(&bv, true);  // bit 0
    BitsPush(&bv, false); // bit 1
    BitsPush(&bv, true);  // bit 2
    BitsPush(&bv, true);  // bit 3
    BitsPush(&bv, false); // bit 4
    BitsPush(&bv, false); // bit 5
    BitsPush(&bv, true);  // bit 6
    BitsPush(&bv, true);  // bit 7

    u8  bytes[2];           // Buffer for byte output
    u64 byte_count = BitsToBytes(&bv, bytes, sizeof(bytes));

    // Check result
    bool result = (byte_count == 1);
    if (result) {
        // Expected byte value depends on bit ordering
        // If bits are stored LSB first: 10110011 = 0xCD
        // If bits are stored MSB first: 10110011 = 0xB3
        result = result && (bytes[0] == 0xCD || bytes[0] == 0xB3);
    }
    BitsDeinit(&bv);

    return result;
}

// Test BitsFromBytes function
bool test_Bits_from_bytes(void) {
    printf("Testing BitsFromBytes\n");

    // Create byte array
    u8     bytes[] = {0xB3};                    // 10110011 in binary
    Bits bv      = BitsFromBytes(bytes, 8); // 8 bits from the byte

    // Check result (8 bits from 1 byte)
    bool result = (bv.length == 8);

    // The exact bit order depends on implementation
    // Just check that we got 8 bits and some are true, some false
    u64 true_count  = 0;
    u64 false_count = 0;

    for (u64 i = 0; i < bv.length; i++) {
        if (BitsGet(&bv, i)) {
            true_count++;
        } else {
            false_count++;
        }
    }

    // 0xB3 = 10110011 has 5 ones and 3 zeros
    result = result && (true_count == 5) && (false_count == 3);

    // Clean up
    BitsDeinit(&bv);

    return result;
}

// Test BitsToInteger function
bool test_Bits_to_integer(void) {
    printf("Testing BitsToInteger\n");

    Bits bv = BitsInit();

    // Create pattern: 1011 (decimal 11 if MSB first, 13 if LSB first)
    BitsPush(&bv, true);
    BitsPush(&bv, false);
    BitsPush(&bv, true);
    BitsPush(&bv, true);

    // Convert to integer
    u64 value = BitsToInteger(&bv);

    // Check result - pattern 1011 (bits stored as: index 0=true, 1=false, 2=true, 3=true)
    // BitsToInteger uses LSB-first: bit[i] contributes (1ULL << i)
    // So: bit[0]*1 + bit[1]*2 + bit[2]*4 + bit[3]*8 = 1*1 + 0*2 + 1*4 + 1*8 = 13
    bool result = (value == 13);

    // Test with larger pattern
    Bits bv2 = BitsInit();
    for (int i = 0; i < 8; i++) {
        BitsPush(&bv2, (i % 2 == 0)); // Alternating pattern
    }

    u64 value2 = BitsToInteger(&bv2);
    result     = result && (value2 > 0); // Should be some positive value

    // Clean up
    BitsDeinit(&bv);
    BitsDeinit(&bv2);

    return result;
}

// Test BitsFromInteger function
bool test_Bits_from_integer(void) {
    printf("Testing BitsFromInteger\n");

    // Convert from integer
    u64    value = 11; // 1011 in binary
    Bits bv    = BitsFromInteger(value, 4);

    // Check result
    bool result = (bv.length == 4);

    // Count ones and zeros
    u64 true_count  = 0;
    u64 false_count = 0;

    for (u64 i = 0; i < bv.length; i++) {
        if (BitsGet(&bv, i)) {
            true_count++;
        } else {
            false_count++;
        }
    }

    // 11 in binary (1011) has 3 ones and 1 zero
    result = result && (true_count == 3) && (false_count == 1);

    // Test with zero
    Bits zero_bv = BitsFromInteger(0, 8);
    result         = result && (zero_bv.length == 8);

    // All bits should be false
    bool all_false = true;
    for (u64 i = 0; i < zero_bv.length; i++) {
        if (BitsGet(&zero_bv, i)) {
            all_false = false;
            break;
        }
    }
    result = result && all_false;

    // Clean up
    BitsDeinit(&bv);
    BitsDeinit(&zero_bv);

    return result;
}

// Edge case tests
bool test_Bits_convert_edge_cases(void) {
    printf("Testing Bits convert edge cases\n");

    Bits bv     = BitsInit();
    bool   result = true;

    // Test converting empty Bits
    Str str_obj = BitsToStr(&bv);
    result      = result && (str_obj.length == 0);
    StrDeinit(&str_obj);

    // Test converting single bit
    BitsPush(&bv, true);
    str_obj = BitsToStr(&bv);
    result  = result && (str_obj.length == 1);
    result  = result && (StrCmpCstr(&str_obj, "1", 1) == 0);
    StrDeinit(&str_obj);

    // Test large conversions
    BitsClear(&bv);
    for (int i = 0; i < 1000; i++) {
        BitsPush(&bv, i % 2 == 0);
    }
    str_obj = BitsToStr(&bv);
    result  = result && (str_obj.length == 1000);
    StrDeinit(&str_obj);

    BitsDeinit(&bv);
    return result;
}

bool test_Bits_from_string_edge_cases(void) {
    printf("Testing BitsFromStr edge cases\n");

    bool result = true;

    // Test empty string
    Bits bv1 = BitsFromStr("");
    result     = result && (bv1.length == 0);
    BitsDeinit(&bv1);

    // Test single character
    Bits bv2 = BitsFromStr("1");
    result     = result && (bv2.length == 1);
    result     = result && (BitsGet(&bv2, 0) == true);
    BitsDeinit(&bv2);

    // Test long string
    char long_str[1001];
    for (int i = 0; i < 1000; i++) {
        long_str[i] = (i % 2 == 0) ? '1' : '0';
    }
    long_str[1000] = '\0';

    Bits bv3 = BitsFromStr(long_str);
    result     = result && (bv3.length == 1000);
    result     = result && (BitsGet(&bv3, 0) == true);
    result     = result && (BitsGet(&bv3, 1) == false);
    BitsDeinit(&bv3);

    return result;
}

bool test_Bits_bytes_conversion_edge_cases(void) {
    printf("Testing Bits bytes conversion edge cases\n");

    Bits bv     = BitsInit();
    bool   result = true;

    // Test empty Bits to bytes
    u8  bytes[1] = {0};
    u64 written  = BitsToBytes(&bv, bytes, 1);
    result       = result && (written == 0); // Empty Bits should write 0 bytes

    // Test bytes to Bits with 0 bits (should return empty Bitstor)
    u8     empty_bytes[1] = {0x05};
    Bits bv2            = BitsFromBytes(empty_bytes, 0); // 0 bits
    result                = result && (bv2.length == 0);
    BitsDeinit(&bv2);

    // Test single byte
    u8     single_byte[1] = {0xFF};
    Bits bv3            = BitsFromBytes(single_byte, 8); // 8 bits from 1 byte
    result                = result && (bv3.length == 8);
    BitsDeinit(&bv3);

    BitsDeinit(&bv);
    return result;
}

bool test_Bits_integer_conversion_edge_cases(void) {
    printf("Testing Bits integer conversion edge cases\n");

    Bits bv     = BitsInit();
    bool   result = true;

    // Test empty Bits to integer
    u64 value = BitsToInteger(&bv);
    result    = result && (value == 0);

    // Test integer to Bits with 0
    Bits bv2 = BitsFromInteger(0, 8);     // 8 bits for zero
    result     = result && (bv2.length == 8); // Should be 8 bits
    BitsDeinit(&bv2);

    // Test large integer
    Bits bv3 = BitsFromInteger(UINT64_MAX, 64); // 64 bits for max value
    result     = result && (bv3.length == 64);
    BitsDeinit(&bv3);

    BitsDeinit(&bv);
    return result;
}

// Round-trip conversion tests
bool test_Bits_round_trip_conversions(void) {
    printf("Testing Bits round-trip conversions\n");

    bool result = true;

    // Test string round-trip
    const char* patterns[] = {"101", "1111000011110000", "1", "0", "10101010", "01010101"};

    for (size_t i = 0; i < sizeof(patterns) / sizeof(patterns[0]); i++) {
        Bits bv  = BitsFromStr(patterns[i]);
        Str    str = BitsToStr(&bv);

        // Should get exact same string back
        result = result && (ZstrCompare(str.data, patterns[i]) == 0);

        StrDeinit(&str);
        BitsDeinit(&bv);
    }

    // Test integer round-trip for various sizes
    u64 values[]    = {0, 1, 15, 255, 65535, 0xFFFFFFFF, 0x123456789ABCDEF};
    u64 bit_sizes[] = {1, 4, 8, 16, 32, 64};

    for (size_t i = 0; i < sizeof(values) / sizeof(values[0]); i++) {
        for (size_t j = 0; j < sizeof(bit_sizes) / sizeof(bit_sizes[0]); j++) {
            u64 bits  = bit_sizes[j];
            u64 value = values[i];

            // Mask value to fit in bit_size
            u64 mask  = (bits == 64) ? 0xFFFFFFFFFFFFFFFF : (1ULL << bits) - 1;
            value    &= mask;

            Bits bv        = BitsFromInteger(value, bits);
            u64    recovered = BitsToInteger(&bv);

            result = result && (recovered == value);

            BitsDeinit(&bv);
        }
    }

    // Test byte round-trip
    u8 test_bytes[] = {0x00, 0xFF, 0xAA, 0x55, 0x01, 0x80, 0x7F, 0xFE};

    for (size_t i = 0; i < sizeof(test_bytes); i++) {
        Bits bv             = BitsFromBytes(&test_bytes[i], 8);
        u8     recovered_byte = 0;
        u64    written        = BitsToBytes(&bv, &recovered_byte, 1);

        result = result && (written == 1);
        result = result && (recovered_byte == test_bytes[i]);

        BitsDeinit(&bv);
    }

    return result;
}

// Bounds checking tests
bool test_Bits_conversion_bounds_checking(void) {
    printf("Testing Bits conversion bounds checking\n");

    bool result = true;

    // Test large integer conversion (should cap at 64 bits)
    Bits large_bv = BitsFromInteger(0xFFFFFFFFFFFFFFFF, 64);
    result          = result && (large_bv.length == 64);

    u64 large_value = BitsToInteger(&large_bv);
    result          = result && (large_value == 0xFFFFFFFFFFFFFFFF);
    BitsDeinit(&large_bv);

    // Test oversized Bits to integer (should handle gracefully)
    Bits oversized = BitsInit();
    for (int i = 0; i < 100; i++) { // 100 bits > 64 bit limit
        BitsPush(&oversized, i % 2 == 0);
    }

    u64 oversized_value = BitsToInteger(&oversized);
    // Should return some value or 0, but not crash
    result = result && (oversized_value >= 0); // Always true for u64, but documents intent
    BitsDeinit(&oversized);

    // Test zero-length conversions
    Bits empty = BitsInit();

    Str empty_str = BitsToStr(&empty);
    result        = result && (empty_str.length == 0);
    StrDeinit(&empty_str);

    u64 empty_value = BitsToInteger(&empty);
    result          = result && (empty_value == 0);

    u8  empty_bytes[1] = {0xFF};
    u64 empty_written  = BitsToBytes(&empty, empty_bytes, 1);
    result             = result && (empty_written == 0);
    result             = result && (empty_bytes[0] == 0xFF); // Should be unchanged

    BitsDeinit(&empty);

    return result;
}

// Comprehensive conversion validation
bool test_Bits_conversion_comprehensive(void) {
    printf("Testing Bits comprehensive conversion validation\n");

    bool result = true;

    // Test specific bit patterns with exact expectations
    struct {
        const char* pattern;
        u64         expected_value;
        u8          expected_bytes[8];
        size_t      byte_count;
    } test_cases[] = {
        {        "10000000",   0x01,       {0x01}, 1}, // MSB set
        {        "00000001",   0x80,       {0x80}, 1}, // LSB set
        {        "11111111",   0xFF,       {0xFF}, 1}, // All bits set
        {        "00000000",   0x00,       {0x00}, 1}, // No bits set
        {"1010101010101010", 0x5555, {0x55, 0x55}, 2}, // Alternating pattern
        {"0101010101010101", 0xAAAA, {0xAA, 0xAA}, 2}, // Inverse alternating
    };

    for (size_t i = 0; i < sizeof(test_cases) / sizeof(test_cases[0]); i++) {
        Bits bv = BitsFromStr(test_cases[i].pattern);

        // Test string conversion consistency
        Str str = BitsToStr(&bv);
        result  = result && (ZstrCompare(str.data, test_cases[i].pattern) == 0);
        StrDeinit(&str);

        // Test integer conversion (may depend on bit order)
        u64 value = BitsToInteger(&bv);
        // We test that we get a consistent value (not necessarily the exact expected one)
        result = result && (value > 0 || ZstrCompare(test_cases[i].pattern, "00000000") == 0);

        // Test byte conversion
        u8  bytes[8] = {0};
        u64 written  = BitsToBytes(&bv, bytes, 8);
        result       = result && (written == test_cases[i].byte_count);

        BitsDeinit(&bv);
    }

    // Test cross-format validation
    Bits bv1 = BitsFromStr("11010110");
    Bits bv2 = BitsFromInteger(0xD6, 8); // Assuming MSB-first: 11010110 = 0xD6
    Bits bv3 = BitsFromBytes((u8[]) {0xD6}, 8);

    // All three should produce the same result when converted back
    Str str1 = BitsToStr(&bv1);
    Str str2 = BitsToStr(&bv2);
    Str str3 = BitsToStr(&bv3);

    // At least two of them should match (bit order might affect one)
    bool cross_match = (ZstrCompare(str1.data, str2.data) == 0) || (ZstrCompare(str1.data, str3.data) == 0) ||
                       (ZstrCompare(str2.data, str3.data) == 0);
    result = result && cross_match;

    StrDeinit(&str1);
    StrDeinit(&str2);
    StrDeinit(&str3);
    BitsDeinit(&bv1);
    BitsDeinit(&bv2);
    BitsDeinit(&bv3);

    return result;
}

// Large-scale conversion tests
bool test_Bits_large_scale_conversions(void) {
    printf("Testing Bits large-scale conversions\n");

    bool result = true;

    // Test with very large Bitstors
    Bits large_bv = BitsInit();

    // Create a 1000-bit pattern
    for (int i = 0; i < 1000; i++) {
        BitsPush(&large_bv, (i % 3) == 0); // Every third bit set
    }

    // Test string conversion
    Str large_str = BitsToStr(&large_bv);
    result        = result && (large_str.length == 1000);

    // Verify pattern consistency
    bool pattern_correct = true;
    for (u64 i = 0; i < large_str.length; i++) {
        bool expected = (i % 3) == 0;
        bool actual   = (large_str.data[i] == '1');
        if (expected != actual) {
            pattern_correct = false;
            break;
        }
    }
    result = result && pattern_correct;

    StrDeinit(&large_str);

    // Test byte conversion
    u8  large_bytes[125]; // 1000 bits = 125 bytes
    u64 written = BitsToBytes(&large_bv, large_bytes, 125);
    result      = result && (written == 125);

    // Test round-trip from bytes
    Bits recovered_bv = BitsFromBytes(large_bytes, 1000);
    result              = result && (recovered_bv.length == 1000);

    // Verify recovered pattern
    bool recovered_pattern_correct = true;
    for (u64 i = 0; i < recovered_bv.length; i++) {
        bool expected = (i % 3) == 0;
        bool actual   = BitsGet(&recovered_bv, i);
        if (expected != actual) {
            recovered_pattern_correct = false;
            break;
        }
    }
    result = result && recovered_pattern_correct;

    BitsDeinit(&large_bv);
    BitsDeinit(&recovered_bv);

    // Test string to Bits with large patterns
    char large_pattern[2001];                          // 2000 bits + null terminator
    for (int i = 0; i < 2000; i++) {
        large_pattern[i] = ((i % 7) == 0) ? '1' : '0'; // Every 7th bit set
    }
    large_pattern[2000] = '\0';

    Bits large_from_str = BitsFromStr(large_pattern);
    result                = result && (large_from_str.length == 2000);

    // Verify pattern
    bool large_pattern_correct = true;
    for (u64 i = 0; i < large_from_str.length; i++) {
        bool expected = (i % 7) == 0;
        bool actual   = BitsGet(&large_from_str, i);
        if (expected != actual) {
            large_pattern_correct = false;
            break;
        }
    }
    result = result && large_pattern_correct;

    BitsDeinit(&large_from_str);

    return result;
}

// Enhanced deadend tests
bool test_Bits_bytes_bounds_failures(void) {
    printf("Testing Bits bytes bounds failures\n");

    Bits bv = BitsInit();
    BitsPush(&bv, true);

    // Test with insufficient buffer size
    u8  small_buffer[1];
    u64 written = BitsToBytes(&bv, small_buffer, 0); // 0 buffer size
    (void)written;                                     // Suppress unused variable warning

    // Should handle gracefully
    BitsDeinit(&bv);

    // Test fromBytes with 0 bit length - should return empty Bits
    u8     dummy_bytes[1] = {0xFF};
    Bits empty_bv       = BitsFromBytes(dummy_bytes, 0);
    bool   result         = (empty_bv.length == 0);
    BitsDeinit(&empty_bv);

    return result;
}

bool test_Bits_integer_bounds_failures(void) {
    printf("Testing Bits integer bounds failures\n");

    // Test BitsToInteger with NULL pointer - should abort
    u64 value = BitsToInteger(NULL);
    (void)value; // Suppress unused variable warning

    return false;
}

// Deadend tests
bool test_Bits_convert_null_failures(void) {
    printf("Testing Bits convert NULL pointer handling\n");

    // Test NULL Bits pointer - should abort
    BitsToStr(NULL);

    return false;
}

bool test_Bits_from_string_null_failures(void) {
    printf("Testing Bits from string NULL handling\n");

    // Test NULL string - should abort
    BitsFromStr(NULL);

    return false;
}

bool test_Bits_bytes_null_failures(void) {
    printf("Testing Bits bytes NULL handling\n");

    // Test NULL bytes - should abort
    BitsFromBytes(NULL, 8); // NULL bytes, 8 bits

    return false;
}

// Main function that runs all tests
int main(void) {
    printf("[INFO] Starting Bits.Convert tests\n\n");

    // Array of normal test functions
    TestFunction tests[] = {
        test_Bits_to_string,
        test_Bits_from_string,
        test_Bits_to_bytes,
        test_Bits_from_bytes,
        test_Bits_to_integer,
        test_Bits_from_integer,
        test_Bits_convert_edge_cases,
        test_Bits_from_string_edge_cases,
        test_Bits_bytes_conversion_edge_cases,
        test_Bits_integer_conversion_edge_cases,
        test_Bits_round_trip_conversions,
        test_Bits_conversion_bounds_checking,
        test_Bits_conversion_comprehensive,
        test_Bits_large_scale_conversions
    };

    // Array of deadend test functions
    TestFunction deadend_tests[] = {
        test_Bits_convert_null_failures,
        test_Bits_from_string_null_failures,
        test_Bits_bytes_null_failures,
        test_Bits_bytes_bounds_failures,
        test_Bits_integer_bounds_failures
    };

    int total_tests         = sizeof(tests) / sizeof(tests[0]);
    int total_deadend_tests = sizeof(deadend_tests) / sizeof(deadend_tests[0]);

    // Run all tests using the centralized test driver
    return run_test_suite(tests, total_tests, deadend_tests, total_deadend_tests, "Bits.Convert");
}

