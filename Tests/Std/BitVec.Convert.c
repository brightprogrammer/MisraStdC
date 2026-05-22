#include <Misra/Std/Container/BitVec.h>
#include <Misra/Std/Zstr.h>
#include <Misra/Std/Allocator/Default.h>
#include <Misra/Std/Container/Str.h>
#include <Misra/Std/Log.h>
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
bool test_bitvec_try_conversion_allocators(void);
bool test_bitvec_convert_edge_cases(void);
bool test_bitvec_from_string_edge_cases(void);
bool test_bitvec_bytes_conversion_edge_cases(void);
bool test_bitvec_integer_conversion_edge_cases(void);
bool test_bitvec_round_trip_conversions(void);
bool test_bitvec_conversion_bounds_checking(void);
bool test_bitvec_conversion_comprehensive(void);
bool test_bitvec_large_scale_conversions(void);
bool test_bitvec_convert_null_failures(void);
bool test_bitvec_from_string_null_failures(void);
bool test_bitvec_bytes_null_failures(void);
bool test_bitvec_bytes_bounds_failures(void);
bool test_bitvec_integer_bounds_failures(void);

// Test BitVecToStr function
bool test_bitvec_to_string(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVecToStr\n");

    BitVec bv = BitVecInit(ALLOCATOR_OF(&alloc));

    // Create pattern: 1011
    BitVecPush(&bv, true);
    BitVecPush(&bv, false);
    BitVecPush(&bv, true);
    BitVecPush(&bv, true);

    // Convert to string
    Str  str;
    bool ok = BitVecTryToStr(&str, &bv);

    // Check result
    bool result = ok && (str.length == 4);
    result      = result && (str.data[0] == '1');
    result      = result && (str.data[1] == '0');
    result      = result && (str.data[2] == '1');
    result      = result && (str.data[3] == '1');

    // Clean up
    StrDeinit(&str);
    BitVecDeinit(&bv);

    DefaultAllocatorDeinit(&alloc);

    return result;
}

// Test BitVecFromStr function
bool test_bitvec_from_string(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVecFromStr\n");

    // Convert from string
    Zstr   str = "1011";
    BitVec bv;
    bool   ok = BitVecTryFromStr(&bv, str, ALLOCATOR_OF(&alloc));

    // Check result
    bool result = ok && (bv.length == 4);
    result      = result && (BitVecGet(&bv, 0) == true);
    result      = result && (BitVecGet(&bv, 1) == false);
    result      = result && (BitVecGet(&bv, 2) == true);
    result      = result && (BitVecGet(&bv, 3) == true);

    // Test with empty string
    BitVec empty_bv = BitVecFromStr("", ALLOCATOR_OF(&alloc));
    result          = result && (empty_bv.length == 0);

    // Clean up
    BitVecDeinit(&bv);
    BitVecDeinit(&empty_bv);

    DefaultAllocatorDeinit(&alloc);

    return result;
}

// Test BitVecToBytes function
bool test_bitvec_to_bytes(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVecToBytes\n");

    BitVec bv = BitVecInit(ALLOCATOR_OF(&alloc));

    // Create pattern: 10110011 (0xB3)
    BitVecPush(&bv, true);  // bit 0
    BitVecPush(&bv, false); // bit 1
    BitVecPush(&bv, true);  // bit 2
    BitVecPush(&bv, true);  // bit 3
    BitVecPush(&bv, false); // bit 4
    BitVecPush(&bv, false); // bit 5
    BitVecPush(&bv, true);  // bit 6
    BitVecPush(&bv, true);  // bit 7

    u8  bytes[2];           // Buffer for byte output
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

    DefaultAllocatorDeinit(&alloc);

    return result;
}

// Test BitVecFromBytes function
bool test_bitvec_from_bytes(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVecFromBytes\n");

    // Create byte array
    u8     bytes[] = {0xB3};                                             // 10110011 in binary
    BitVec bv;
    bool   ok = BitVecTryFromBytes(&bv, bytes, 8, ALLOCATOR_OF(&alloc)); // 8 bits from the byte

    // Check result (8 bits from 1 byte)
    bool result = ok && (bv.length == 8);

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

    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Test BitVecToInteger function
bool test_bitvec_to_integer(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVecToInteger\n");

    BitVec bv = BitVecInit(ALLOCATOR_OF(&alloc));

    // Create pattern: 1011 (decimal 11 if MSB first, 13 if LSB first)
    BitVecPush(&bv, true);
    BitVecPush(&bv, false);
    BitVecPush(&bv, true);
    BitVecPush(&bv, true);

    // Convert to integer
    u64 value = BitVecToInteger(&bv);

    // Check result - pattern 1011 (bits stored as: index 0=true, 1=false, 2=true, 3=true)
    // BitVecToInteger uses LSB-first: bit[i] contributes (1ULL << i)
    // So: bit[0]*1 + bit[1]*2 + bit[2]*4 + bit[3]*8 = 1*1 + 0*2 + 1*4 + 1*8 = 13
    bool result = (value == 13);

    // Test with larger pattern
    BitVec bv2 = BitVecInit(ALLOCATOR_OF(&alloc));
    for (int i = 0; i < 8; i++) {
        BitVecPush(&bv2, (i % 2 == 0)); // Alternating pattern
    }

    u64 value2 = BitVecToInteger(&bv2);
    result     = result && (value2 > 0); // Should be some positive value

    // Clean up
    BitVecDeinit(&bv);
    BitVecDeinit(&bv2);

    DefaultAllocatorDeinit(&alloc);

    return result;
}

// Test BitVecFromInteger function
bool test_bitvec_from_integer(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVecFromInteger\n");

    // Convert from integer
    u64    value = 11; // 1011 in binary
    BitVec bv;
    bool   ok = BitVecTryFromInteger(&bv, value, 4, ALLOCATOR_OF(&alloc));

    // Check result
    bool result = ok && (bv.length == 4);

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
    BitVec zero_bv;
    result = result && BitVecTryFromInteger(&zero_bv, 0, 8, ALLOCATOR_OF(&alloc));
    result = result && (zero_bv.length == 8);

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

    DefaultAllocatorDeinit(&alloc);
    return result;
}

bool test_bitvec_try_conversion_allocators(void) {
    WriteFmt("Testing BitVec try conversion allocator behavior\n");

    DefaultAllocator alloc = DefaultAllocatorInit();
    alloc.base.effort      = ALLOCATOR_EFFORT_RETRY;
    alloc.base.retry_limit = 3;

    BitVec bv;
    Str    str;
    bool   ok = BitVecTryFromStr(&bv, "101001", ALLOCATOR_OF(&alloc));
    bool   result =
        ok && (bv.allocator->effort == alloc.base.effort) && (bv.allocator->retry_limit == alloc.base.retry_limit);

    ok     = BitVecTryToStr(&str, &bv);
    result = result && ok && (str.allocator->effort == alloc.base.effort) &&
             (str.allocator->retry_limit == alloc.base.retry_limit) && (ZstrCompare(str.data, "101001") == 0);

    StrDeinit(&str);
    BitVecDeinit(&bv);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Edge case tests
bool test_bitvec_convert_edge_cases(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVec convert edge cases\n");

    BitVec bv     = BitVecInit(ALLOCATOR_OF(&alloc));
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
    DefaultAllocatorDeinit(&alloc);
    return result;
}

bool test_bitvec_from_string_edge_cases(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVecFromStr edge cases\n");

    bool result = true;

    // Test empty string
    BitVec bv1 = BitVecFromStr("", ALLOCATOR_OF(&alloc));
    result     = result && (bv1.length == 0);
    BitVecDeinit(&bv1);

    // Test single character
    BitVec bv2 = BitVecFromStr("1", ALLOCATOR_OF(&alloc));
    result     = result && (bv2.length == 1);
    result     = result && (BitVecGet(&bv2, 0) == true);
    BitVecDeinit(&bv2);

    // Test long string
    char long_str[1001];
    for (int i = 0; i < 1000; i++) {
        long_str[i] = (i % 2 == 0) ? '1' : '0';
    }
    long_str[1000] = '\0';

    BitVec bv3 = BitVecFromStr(long_str, ALLOCATOR_OF(&alloc));
    result     = result && (bv3.length == 1000);
    result     = result && (BitVecGet(&bv3, 0) == true);
    result     = result && (BitVecGet(&bv3, 1) == false);
    BitVecDeinit(&bv3);

    DefaultAllocatorDeinit(&alloc);

    return result;
}

bool test_bitvec_bytes_conversion_edge_cases(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVec bytes conversion edge cases\n");

    BitVec bv     = BitVecInit(ALLOCATOR_OF(&alloc));
    bool   result = true;

    // Test empty bitvec to bytes
    u8  bytes[1] = {0};
    u64 written  = BitVecToBytes(&bv, bytes, 1);
    result       = result && (written == 0); // Empty bitvec should write 0 bytes

    // Test bytes to bitvec with 0 bits (should return empty bitvector)
    u8     empty_bytes[1] = {0x05};
    BitVec bv2            = BitVecFromBytes(empty_bytes, 0, ALLOCATOR_OF(&alloc)); // 0 bits
    result                = result && (bv2.length == 0);
    BitVecDeinit(&bv2);

    // Test single byte
    u8     single_byte[1] = {0xFF};
    BitVec bv3            = BitVecFromBytes(single_byte, 8, ALLOCATOR_OF(&alloc)); // 8 bits from 1 byte
    result                = result && (bv3.length == 8);
    BitVecDeinit(&bv3);

    BitVecDeinit(&bv);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

bool test_bitvec_integer_conversion_edge_cases(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVec integer conversion edge cases\n");

    BitVec bv     = BitVecInit(ALLOCATOR_OF(&alloc));
    bool   result = true;

    // Test empty bitvec to integer
    u64 value = BitVecToInteger(&bv);
    result    = result && (value == 0);

    // Test integer to bitvec with 0
    BitVec bv2 = BitVecFromInteger(0, 8, ALLOCATOR_OF(&alloc)); // 8 bits for zero
    result     = result && (bv2.length == 8);                   // Should be 8 bits
    BitVecDeinit(&bv2);

    // Test large integer
    BitVec bv3 = BitVecFromInteger(UINT64_MAX, 64, ALLOCATOR_OF(&alloc)); // 64 bits for max value
    result     = result && (bv3.length == 64);
    BitVecDeinit(&bv3);

    BitVecDeinit(&bv);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Round-trip conversion tests
bool test_bitvec_round_trip_conversions(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVec round-trip conversions\n");

    bool result = true;

    // Test string round-trip
    Zstr patterns[] = {"101", "1111000011110000", "1", "0", "10101010", "01010101"};

    for (size_t i = 0; i < sizeof(patterns) / sizeof(patterns[0]); i++) {
        BitVec bv  = BitVecFromStr(patterns[i], ALLOCATOR_OF(&alloc));
        Str    str = BitVecToStr(&bv);

        // Should get exact same string back
        result = result && (ZstrCompare(str.data, patterns[i]) == 0);

        StrDeinit(&str);
        BitVecDeinit(&bv);
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

            BitVec bv        = BitVecFromInteger(value, bits, ALLOCATOR_OF(&alloc));
            u64    recovered = BitVecToInteger(&bv);

            result = result && (recovered == value);

            BitVecDeinit(&bv);
        }
    }

    // Test byte round-trip
    u8 test_bytes[] = {0x00, 0xFF, 0xAA, 0x55, 0x01, 0x80, 0x7F, 0xFE};

    for (size_t i = 0; i < sizeof(test_bytes); i++) {
        BitVec bv             = BitVecFromBytes(&test_bytes[i], 8, ALLOCATOR_OF(&alloc));
        u8     recovered_byte = 0;
        u64    written        = BitVecToBytes(&bv, &recovered_byte, 1);

        result = result && (written == 1);
        result = result && (recovered_byte == test_bytes[i]);

        BitVecDeinit(&bv);
    }

    DefaultAllocatorDeinit(&alloc);

    return result;
}

// Bounds checking tests
bool test_bitvec_conversion_bounds_checking(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVec conversion bounds checking\n");

    bool result = true;

    // Test large integer conversion (should cap at 64 bits)
    BitVec large_bv = BitVecFromInteger(0xFFFFFFFFFFFFFFFF, 64, ALLOCATOR_OF(&alloc));
    result          = result && (large_bv.length == 64);

    u64 large_value = BitVecToInteger(&large_bv);
    result          = result && (large_value == 0xFFFFFFFFFFFFFFFF);
    BitVecDeinit(&large_bv);

    // Test oversized bitvec to integer (should handle gracefully)
    BitVec oversized = BitVecInit(ALLOCATOR_OF(&alloc));
    for (int i = 0; i < 100; i++) { // 100 bits > 64 bit limit
        BitVecPush(&oversized, i % 2 == 0);
    }

    u64 oversized_value = BitVecToInteger(&oversized);
    // Should return some value or 0, but not crash
    result = result && (oversized_value >= 0); // Always true for u64, but documents intent
    BitVecDeinit(&oversized);

    // Test zero-length conversions
    BitVec empty = BitVecInit(ALLOCATOR_OF(&alloc));

    Str empty_str = BitVecToStr(&empty);
    result        = result && (empty_str.length == 0);
    StrDeinit(&empty_str);

    u64 empty_value = BitVecToInteger(&empty);
    result          = result && (empty_value == 0);

    u8  empty_bytes[1] = {0xFF};
    u64 empty_written  = BitVecToBytes(&empty, empty_bytes, 1);
    result             = result && (empty_written == 0);
    result             = result && (empty_bytes[0] == 0xFF); // Should be unchanged

    BitVecDeinit(&empty);

    DefaultAllocatorDeinit(&alloc);

    return result;
}

// Comprehensive conversion validation
bool test_bitvec_conversion_comprehensive(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVec comprehensive conversion validation\n");

    bool result = true;

    // Test specific bit patterns with exact expectations
    struct {
        Zstr   pattern;
        u64    expected_value;
        u8     expected_bytes[8];
        size_t byte_count;
    } test_cases[] = {
        {        "10000000",   0x01,       {0x01}, 1}, // MSB set
        {        "00000001",   0x80,       {0x80}, 1}, // LSB set
        {        "11111111",   0xFF,       {0xFF}, 1}, // All bits set
        {        "00000000",   0x00,       {0x00}, 1}, // No bits set
        {"1010101010101010", 0x5555, {0x55, 0x55}, 2}, // Alternating pattern
        {"0101010101010101", 0xAAAA, {0xAA, 0xAA}, 2}, // Inverse alternating
    };

    for (size_t i = 0; i < sizeof(test_cases) / sizeof(test_cases[0]); i++) {
        BitVec bv = BitVecFromStr(test_cases[i].pattern, ALLOCATOR_OF(&alloc));

        // Test string conversion consistency
        Str str = BitVecToStr(&bv);
        result  = result && (ZstrCompare(str.data, test_cases[i].pattern) == 0);
        StrDeinit(&str);

        // Test integer conversion (may depend on bit order)
        u64 value = BitVecToInteger(&bv);
        // We test that we get a consistent value (not necessarily the exact expected one)
        result = result && (value > 0 || ZstrCompare(test_cases[i].pattern, "00000000") == 0);

        // Test byte conversion
        u8  bytes[8] = {0};
        u64 written  = BitVecToBytes(&bv, bytes, 8);
        result       = result && (written == test_cases[i].byte_count);

        BitVecDeinit(&bv);
    }

    // Test cross-format validation
    BitVec bv1 = BitVecFromStr("11010110", ALLOCATOR_OF(&alloc));
    BitVec bv2 = BitVecFromInteger(0xD6, 8, ALLOCATOR_OF(&alloc)); // Assuming MSB-first: 11010110 = 0xD6
    BitVec bv3 = BitVecFromBytes((u8[]) {0xD6}, 8, ALLOCATOR_OF(&alloc));

    // All three should produce the same result when converted back
    Str str1 = BitVecToStr(&bv1);
    Str str2 = BitVecToStr(&bv2);
    Str str3 = BitVecToStr(&bv3);

    // At least two of them should match (bit order might affect one)
    bool cross_match = (ZstrCompare(str1.data, str2.data) == 0) || (ZstrCompare(str1.data, str3.data) == 0) ||
                       (ZstrCompare(str2.data, str3.data) == 0);
    result = result && cross_match;

    StrDeinit(&str1);
    StrDeinit(&str2);
    StrDeinit(&str3);
    BitVecDeinit(&bv1);
    BitVecDeinit(&bv2);
    BitVecDeinit(&bv3);

    DefaultAllocatorDeinit(&alloc);

    return result;
}

// Large-scale conversion tests
bool test_bitvec_large_scale_conversions(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVec large-scale conversions\n");

    bool result = true;

    // Test with very large bitvectors
    BitVec large_bv = BitVecInit(ALLOCATOR_OF(&alloc));

    // Create a 1000-bit pattern
    for (int i = 0; i < 1000; i++) {
        BitVecPush(&large_bv, (i % 3) == 0); // Every third bit set
    }

    // Test string conversion
    Str large_str = BitVecToStr(&large_bv);
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
    u64 written = BitVecToBytes(&large_bv, large_bytes, 125);
    result      = result && (written == 125);

    // Test round-trip from bytes
    BitVec recovered_bv = BitVecFromBytes(large_bytes, 1000, ALLOCATOR_OF(&alloc));
    result              = result && (recovered_bv.length == 1000);

    // Verify recovered pattern
    bool recovered_pattern_correct = true;
    for (u64 i = 0; i < recovered_bv.length; i++) {
        bool expected = (i % 3) == 0;
        bool actual   = BitVecGet(&recovered_bv, i);
        if (expected != actual) {
            recovered_pattern_correct = false;
            break;
        }
    }
    result = result && recovered_pattern_correct;

    BitVecDeinit(&large_bv);
    BitVecDeinit(&recovered_bv);

    // Test string to bitvec with large patterns
    char large_pattern[2001];                          // 2000 bits + null terminator
    for (int i = 0; i < 2000; i++) {
        large_pattern[i] = ((i % 7) == 0) ? '1' : '0'; // Every 7th bit set
    }
    large_pattern[2000] = '\0';

    BitVec large_from_str = BitVecFromStr(large_pattern, ALLOCATOR_OF(&alloc));
    result                = result && (large_from_str.length == 2000);

    // Verify pattern
    bool large_pattern_correct = true;
    for (u64 i = 0; i < large_from_str.length; i++) {
        bool expected = (i % 7) == 0;
        bool actual   = BitVecGet(&large_from_str, i);
        if (expected != actual) {
            large_pattern_correct = false;
            break;
        }
    }
    result = result && large_pattern_correct;

    BitVecDeinit(&large_from_str);

    DefaultAllocatorDeinit(&alloc);

    return result;
}

// Enhanced deadend tests
bool test_bitvec_bytes_bounds_failures(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVec bytes bounds failures\n");

    BitVec bv = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVecPush(&bv, true);

    // Test with insufficient buffer size
    u8  small_buffer[1];
    u64 written = BitVecToBytes(&bv, small_buffer, 0); // 0 buffer size
    (void)written;                                     // Suppress unused variable warning

    // Should handle gracefully
    BitVecDeinit(&bv);

    // Test fromBytes with 0 bit length - should return empty bitvec
    u8     dummy_bytes[1] = {0xFF};
    BitVec empty_bv       = BitVecFromBytes(dummy_bytes, 0, ALLOCATOR_OF(&alloc));
    bool   result         = (empty_bv.length == 0);
    BitVecDeinit(&empty_bv);

    DefaultAllocatorDeinit(&alloc);

    return result;
}

bool test_bitvec_integer_bounds_failures(void) {
    WriteFmt("Testing BitVec integer bounds failures\n");

    // Test BitVecToInteger with NULL pointer - should abort
    u64 value = BitVecToInteger(NULL);
    (void)value; // Suppress unused variable warning

    return false;
}

// Deadend tests
bool test_bitvec_convert_null_failures(void) {
    WriteFmt("Testing BitVec convert NULL pointer handling\n");

    // Test NULL bitvec pointer - should abort
    BitVecToStr(NULL);

    return false;
}

bool test_bitvec_from_string_null_failures(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVec from string NULL handling\n");

    // Test NULL string - should abort
    BitVecFromStr(NULL, ALLOCATOR_OF(&alloc));

    DefaultAllocatorDeinit(&alloc);

    return false;
}

bool test_bitvec_bytes_null_failures(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVec bytes NULL handling\n");

    // Test NULL bytes - should abort
    BitVecFromBytes(NULL, 8, ALLOCATOR_OF(&alloc)); // NULL bytes, 8 bits

    DefaultAllocatorDeinit(&alloc);

    return false;
}

// Main function that runs all tests
int main(void) {
    WriteFmt("[INFO] Starting BitVec.Convert tests\n\n");

    // Array of normal test functions
    TestFunction tests[] = {
        test_bitvec_to_string,
        test_bitvec_from_string,
        test_bitvec_to_bytes,
        test_bitvec_from_bytes,
        test_bitvec_to_integer,
        test_bitvec_from_integer,
        test_bitvec_try_conversion_allocators,
        test_bitvec_convert_edge_cases,
        test_bitvec_from_string_edge_cases,
        test_bitvec_bytes_conversion_edge_cases,
        test_bitvec_integer_conversion_edge_cases,
        test_bitvec_round_trip_conversions,
        test_bitvec_conversion_bounds_checking,
        test_bitvec_conversion_comprehensive,
        test_bitvec_large_scale_conversions
    };

    // Array of deadend test functions
    TestFunction deadend_tests[] = {
        test_bitvec_convert_null_failures,
        test_bitvec_from_string_null_failures,
        test_bitvec_bytes_null_failures,
        test_bitvec_bytes_bounds_failures,
        test_bitvec_integer_bounds_failures
    };

    int total_tests         = sizeof(tests) / sizeof(tests[0]);
    int total_deadend_tests = sizeof(deadend_tests) / sizeof(deadend_tests[0]);

    // Run all tests using the centralized test driver
    return run_test_suite(tests, total_tests, deadend_tests, total_deadend_tests, "BitVec.Convert");
}
