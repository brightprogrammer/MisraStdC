#include <Misra/Std/Container/Int.h>
#include <Misra/Std/Log.h>
#include <Misra/Std/Memory.h>
#include <Misra/Types.h>

#include "../Util/TestRunner.h"

bool test_int_from_unsigned_integer(void);
bool test_int_bytes_le_round_trip(void);
bool test_int_bytes_be_round_trip(void);
bool test_int_binary_round_trip(void);
bool test_int_decimal_round_trip(void);
bool test_int_radix_round_trip(void);
bool test_int_upper_hex_radix(void);
bool test_int_try_to_str_allocator_inheritance(void);
bool test_int_compare_ignores_leading_zeros(void);
bool test_int_zero_binary(void);
bool test_int_binary_prefix_and_separators(void);
bool test_int_octal_round_trip(void);
bool test_int_hex_round_trip(void);
bool test_int_from_binary_invalid_digit(void);
bool test_int_from_decimal_invalid_digit(void);
bool test_int_from_hex_invalid_digit(void);
bool test_int_from_radix_invalid_digit(void);
bool test_int_from_radix_invalid_radix(void);
bool test_int_to_u64_overflow(void);
bool test_int_to_str_radix_invalid_radix(void);
bool test_int_from_binary_null(void);
bool test_int_from_decimal_null(void);
bool test_int_from_radix_null(void);
bool test_int_from_octal_null(void);
bool test_int_from_hex_null(void);
bool test_int_from_bytes_le_null(void);
bool test_int_to_bytes_le_null(void);
bool test_int_to_bytes_be_zero_max_len(void);

bool test_int_from_unsigned_integer(void) {
    WriteFmt("Testing IntFrom with unsigned integer\n");

    Int value = IntFrom(13);
    Str text  = IntToBinary(&value);

    bool result = IntBitLength(&value) == 4;
    result      = result && (IntToU64(&value) == 13);
    result      = result && (ZstrCompare(text.data, "1101") == 0);

    StrDeinit(&text);
    IntDeinit(&value);
    return result;
}

bool test_int_bytes_le_round_trip(void) {
    WriteFmt("Testing Int little-endian byte conversion\n");

    u8  bytes[] = {0x34, 0x12, 0xEF, 0xCD};
    u8  out[4]  = {0};
    Int value   = IntFromBytesLE(bytes, sizeof(bytes));
    u64 written = IntToBytesLE(&value, out, sizeof(out));
    Str text    = IntToHexStr(&value);

    bool result = written == 4;
    result      = result && (MemCompare(out, bytes, sizeof(bytes)) == 0);
    result      = result && (ZstrCompare(text.data, "cdef1234") == 0);

    StrDeinit(&text);
    IntDeinit(&value);
    return result;
}

bool test_int_bytes_be_round_trip(void) {
    WriteFmt("Testing Int big-endian byte conversion\n");

    u8  bytes[] = {0x12, 0x34, 0x56, 0x78};
    u8  out[4]  = {0};
    Int value   = IntFromBytesBE(bytes, sizeof(bytes));
    u64 written = IntToBytesBE(&value, out, sizeof(out));
    Str text    = IntToHexStr(&value);

    bool result = written == 4;
    result      = result && (MemCompare(out, bytes, sizeof(bytes)) == 0);
    result      = result && (ZstrCompare(text.data, "12345678") == 0);

    StrDeinit(&text);
    IntDeinit(&value);
    return result;
}

bool test_int_binary_round_trip(void) {
    WriteFmt("Testing Int binary round trip\n");

    Int value = IntFromBinary("001011");
    Str text  = IntToBinary(&value);

    bool result = IntToU64(&value) == 11;
    result      = result && (ZstrCompare(text.data, "1011") == 0);

    StrDeinit(&text);
    IntDeinit(&value);
    return result;
}

bool test_int_decimal_round_trip(void) {
    WriteFmt("Testing Int decimal round trip\n");

    const char *digits = "123456789012345678901234567890";
    Int         value  = IntFromStr(digits);
    Str         text   = IntToStr(&value);

    bool result = ZstrCompare(text.data, digits) == 0;

    StrDeinit(&text);
    IntDeinit(&value);
    return result;
}

bool test_int_radix_round_trip(void) {
    WriteFmt("Testing Int radix conversion round trip\n");

    Int value = IntFromStrRadix("zz", 36);
    Str text  = IntToStrRadix(&value, 36, false);

    bool result = IntToU64(&value) == 1295;
    result      = result && (ZstrCompare(text.data, "zz") == 0);

    StrDeinit(&text);
    IntDeinit(&value);
    return result;
}

bool test_int_upper_hex_radix(void) {
    WriteFmt("Testing Int uppercase radix conversion\n");

    Int value = IntFrom(0xBEEF);
    Str text  = IntToStrRadix(&value, 16, true);

    bool result = ZstrCompare(text.data, "BEEF") == 0;

    StrDeinit(&text);
    IntDeinit(&value);
    return result;
}

bool test_int_try_to_str_allocator_inheritance(void) {
    WriteFmt("Testing IntTryToStr allocator behavior\n");

    Int       value = IntFrom(0xBEEF);
    Str       text;
    Allocator alloc = DefaultAllocator();
    bool      ok;

    alloc.effort      = ALLOCATOR_EFFORT_RETRY;
    alloc.retry_limit = 4;

    ok = IntTryToStrRadixAlloc(&text, &value, 16, true, alloc);

    bool result = ok && (ZstrCompare(text.data, "BEEF") == 0) && (text.allocator.effort == alloc.effort) &&
                  (text.allocator.retry_limit == alloc.retry_limit);

    StrDeinit(&text);
    IntDeinit(&value);
    return result;
}

bool test_int_compare_ignores_leading_zeros(void) {
    WriteFmt("Testing IntCompare leading-zero normalization\n");

    Int lhs = IntFromBinary("0001011");
    Int rhs = IntFrom(11);

    bool result = IntCompare(&lhs, &rhs) == 0;
    result      = result && IntEQ(&lhs, &rhs);

    IntDeinit(&lhs);
    IntDeinit(&rhs);
    return result;
}

bool test_int_zero_binary(void) {
    WriteFmt("Testing Int zero binary conversion\n");

    Int  zero  = IntFromBinary("0");
    Str  text  = IntToBinary(&zero);
    bool error = true;

    bool result = IntBitLength(&zero) == 0;
    result      = result && IntIsZero(&zero);
    result      = result && (IntToU64(&zero, &error) == 0);
    result      = result && !error;
    result      = result && (ZstrCompare(text.data, "0") == 0);

    StrDeinit(&text);
    IntDeinit(&zero);
    return result;
}

bool test_int_binary_prefix_and_separators(void) {
    WriteFmt("Testing Int binary prefix and separators\n");

    Int value = IntFromBinary("0b1010_0011");

    bool result = IntToU64(&value) == 163;
    result      = result && (IntBitLength(&value) == 8);

    IntDeinit(&value);
    return result;
}

bool test_int_octal_round_trip(void) {
    WriteFmt("Testing Int octal round trip\n");

    Int value = IntFromOctStr("0o7_55");
    Str text  = IntToOctStr(&value);

    bool result = IntToU64(&value) == 493;
    result      = result && (ZstrCompare(text.data, "755") == 0);

    StrDeinit(&text);
    IntDeinit(&value);
    return result;
}

bool test_int_hex_round_trip(void) {
    WriteFmt("Testing Int hex round trip\n");

    const char *hex   = "deadbeefcafebabe1234";
    Int         value = IntFromHexStr(hex);
    Str         text  = IntToHexStr(&value);

    bool result = ZstrCompare(text.data, hex) == 0;

    StrDeinit(&text);
    IntDeinit(&value);
    return result;
}

bool test_int_from_binary_invalid_digit(void) {
    WriteFmt("Testing IntFromBinary invalid digit handling\n");

    Int parsed = IntFromBinary("10a1");
    Int value  = IntInit();
    bool result = !IntTryFromBinary(&value, "10a1");

    result = result && IntIsZero(&parsed);
    result = result && IntIsZero(&value);

    IntDeinit(&parsed);
    IntDeinit(&value);
    return result;
}

bool test_int_from_decimal_invalid_digit(void) {
    WriteFmt("Testing IntFromStr invalid digit handling\n");

    Int parsed = IntFromStr("12x3");
    Int value  = IntInit();
    bool result = !IntTryFromStr(&value, "12x3");

    result = result && IntIsZero(&parsed);
    result = result && IntIsZero(&value);

    IntDeinit(&parsed);
    IntDeinit(&value);
    return result;
}

bool test_int_from_hex_invalid_digit(void) {
    WriteFmt("Testing IntFromHexStr invalid digit handling\n");

    Int parsed = IntFromHexStr("12g3");
    Int value  = IntInit();
    bool result = !IntTryFromHexStr(&value, "12g3");

    result = result && IntIsZero(&parsed);
    result = result && IntIsZero(&value);

    IntDeinit(&parsed);
    IntDeinit(&value);
    return result;
}

bool test_int_from_radix_invalid_digit(void) {
    WriteFmt("Testing IntFromStrRadix invalid digit handling\n");

    Int parsed = IntFromStrRadix("102", 2);
    Int value  = IntInit();
    bool result = !IntTryFromStrRadix(&value, "102", 2);

    result = result && IntIsZero(&parsed);
    result = result && IntIsZero(&value);

    IntDeinit(&parsed);
    IntDeinit(&value);
    return result;
}

bool test_int_from_radix_invalid_radix(void) {
    WriteFmt("Testing IntFromStrRadix invalid radix handling\n");

    Int parsed = IntFromStrRadix("10", 1);
    Int value  = IntInit();
    bool result = !IntTryFromStrRadix(&value, "10", 1);

    result = result && IntIsZero(&parsed);
    result = result && IntIsZero(&value);

    IntDeinit(&parsed);
    IntDeinit(&value);
    return result;
}

bool test_int_to_u64_overflow(void) {
    WriteFmt("Testing IntToU64 overflow handling\n");

    Int value = IntFrom(1);
    u64 out   = 0;
    bool error = false;

    IntShiftLeft(&value, 64);

    bool result = !IntTryToU64(&value, &out);
    result      = result && (IntToU64(&value, &error) == 0);
    result      = result && error;
    result      = result && (out == 0);

    IntDeinit(&value);
    return result;
}

bool test_int_to_str_radix_invalid_radix(void) {
    WriteFmt("Testing IntToStrRadix invalid radix handling\n");

    Int value = IntFrom(255);
    Str text  = IntToStrRadix(&value, 37, false);

    bool result = text.length == 0;

    StrDeinit(&text);
    IntDeinit(&value);
    return result;
}

bool test_int_from_binary_null(void) {
    WriteFmt("Testing IntFromBinary NULL handling\n");

    Int parsed = IntFromBinary(NULL);
    Int value  = IntInit();
    bool result = !IntTryFromBinary(&value, NULL);

    result = result && IntIsZero(&parsed);
    result = result && IntIsZero(&value);

    IntDeinit(&parsed);
    IntDeinit(&value);
    return result;
}

bool test_int_from_decimal_null(void) {
    WriteFmt("Testing IntFromStr NULL handling\n");

    Int parsed = IntFromStr(NULL);
    Int value  = IntInit();
    bool result = !IntTryFromStr(&value, NULL);

    result = result && IntIsZero(&parsed);
    result = result && IntIsZero(&value);

    IntDeinit(&parsed);
    IntDeinit(&value);
    return result;
}

bool test_int_from_radix_null(void) {
    WriteFmt("Testing IntFromStrRadix NULL handling\n");

    Int parsed = IntFromStrRadix(NULL, 10);
    Int value  = IntInit();
    bool result = !IntTryFromStrRadix(&value, NULL, 10);

    result = result && IntIsZero(&parsed);
    result = result && IntIsZero(&value);

    IntDeinit(&parsed);
    IntDeinit(&value);
    return result;
}

bool test_int_from_octal_null(void) {
    WriteFmt("Testing IntFromOctStr NULL handling\n");

    Int parsed = IntFromOctStr(NULL);
    Int value  = IntInit();
    bool result = !IntTryFromOctStr(&value, NULL);

    result = result && IntIsZero(&parsed);
    result = result && IntIsZero(&value);

    IntDeinit(&parsed);
    IntDeinit(&value);
    return result;
}

bool test_int_from_hex_null(void) {
    WriteFmt("Testing IntFromHexStr NULL handling\n");

    Int parsed = IntFromHexStr(NULL);
    Int value  = IntInit();
    bool result = !IntTryFromHexStr(&value, NULL);

    result = result && IntIsZero(&parsed);
    result = result && IntIsZero(&value);

    IntDeinit(&parsed);
    IntDeinit(&value);
    return result;
}

bool test_int_from_bytes_le_null(void) {
    WriteFmt("Testing IntFromBytesLE NULL handling\n");

    IntFromBytesLE(NULL, 1);
    return false;
}

bool test_int_to_bytes_le_null(void) {
    WriteFmt("Testing IntToBytesLE NULL handling\n");

    Int value = IntFrom(1);
    IntToBytesLE(&value, NULL, 1);
    return false;
}

bool test_int_to_bytes_be_zero_max_len(void) {
    WriteFmt("Testing IntToBytesBE zero max_len handling\n");

    Int value = IntFrom(1);
    u8  byte  = 0;

    IntToBytesBE(&value, &byte, 0);
    return false;
}

int main(void) {
    WriteFmt("[INFO] Starting Int.Convert tests\n\n");

    TestFunction tests[] = {
        test_int_from_unsigned_integer,
        test_int_bytes_le_round_trip,
        test_int_bytes_be_round_trip,
        test_int_binary_round_trip,
        test_int_decimal_round_trip,
        test_int_radix_round_trip,
        test_int_upper_hex_radix,
        test_int_try_to_str_allocator_inheritance,
        test_int_compare_ignores_leading_zeros,
        test_int_zero_binary,
        test_int_binary_prefix_and_separators,
        test_int_octal_round_trip,
        test_int_hex_round_trip,
        test_int_from_binary_invalid_digit,
        test_int_from_decimal_invalid_digit,
        test_int_from_hex_invalid_digit,
        test_int_from_radix_invalid_digit,
        test_int_from_radix_invalid_radix,
        test_int_to_u64_overflow,
        test_int_to_str_radix_invalid_radix,
        test_int_from_binary_null,
        test_int_from_decimal_null,
        test_int_from_radix_null,
        test_int_from_octal_null,
        test_int_from_hex_null,
    };

    TestFunction deadend_tests[] = {
        test_int_from_bytes_le_null,
        test_int_to_bytes_le_null,
        test_int_to_bytes_be_zero_max_len,
    };

    int total_tests         = sizeof(tests) / sizeof(tests[0]);
    int total_deadend_tests = sizeof(deadend_tests) / sizeof(deadend_tests[0]);

    return run_test_suite(tests, total_tests, deadend_tests, total_deadend_tests, "Int.Convert");
}
