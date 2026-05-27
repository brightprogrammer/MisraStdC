#include <Misra/Std/Allocator/Default.h>
#include <Misra/Std/Zstr.h>
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

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int value = IntFrom(13, &alloc.base);
    Str text  = IntToBinary(&value);

    bool result = IntBitLength(&value) == 4;
    result      = result && (IntToU64(&value) == 13);
    result      = result && (ZstrCompare(StrBegin(&text), "1101") == 0);

    StrDeinit(&text);
    IntDeinit(&value);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

bool test_int_bytes_le_round_trip(void) {
    WriteFmt("Testing Int little-endian byte conversion\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    u8  bytes[] = {0x34, 0x12, 0xEF, 0xCD};
    u8  out[4]  = {0};
    Int value   = IntFromBytesLE(bytes, sizeof(bytes), &alloc.base);
    u64 written = IntToBytesLE(&value, out, sizeof(out));
    Str text    = IntToHexStr(&value);

    bool result = written == 4;
    result      = result && (MemCompare(out, bytes, sizeof(bytes)) == 0);
    result      = result && (ZstrCompare(StrBegin(&text), "cdef1234") == 0);

    StrDeinit(&text);
    IntDeinit(&value);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

bool test_int_bytes_be_round_trip(void) {
    WriteFmt("Testing Int big-endian byte conversion\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    u8  bytes[] = {0x12, 0x34, 0x56, 0x78};
    u8  out[4]  = {0};
    Int value   = IntFromBytesBE(bytes, sizeof(bytes), &alloc.base);
    u64 written = IntToBytesBE(&value, out, sizeof(out));
    Str text    = IntToHexStr(&value);

    bool result = written == 4;
    result      = result && (MemCompare(out, bytes, sizeof(bytes)) == 0);
    result      = result && (ZstrCompare(StrBegin(&text), "12345678") == 0);

    StrDeinit(&text);
    IntDeinit(&value);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

bool test_int_binary_round_trip(void) {
    WriteFmt("Testing Int binary round trip\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int value = IntFromBinary("001011", &alloc.base);
    Str text  = IntToBinary(&value);

    bool result = IntToU64(&value) == 11;
    result      = result && (ZstrCompare(StrBegin(&text), "1011") == 0);

    StrDeinit(&text);
    IntDeinit(&value);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

bool test_int_decimal_round_trip(void) {
    WriteFmt("Testing Int decimal round trip\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Zstr digits = "123456789012345678901234567890";
    Int  value  = IntFromStr(digits, &alloc.base);
    Str  text   = IntToStr(&value);

    bool result = ZstrCompare(StrBegin(&text), digits) == 0;

    StrDeinit(&text);
    IntDeinit(&value);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

bool test_int_radix_round_trip(void) {
    WriteFmt("Testing Int radix conversion round trip\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int value = IntFromStrRadix("zz", 36, &alloc.base);
    Str text  = IntToStrRadix(&value, 36, false);

    bool result = IntToU64(&value) == 1295;
    result      = result && (ZstrCompare(StrBegin(&text), "zz") == 0);

    StrDeinit(&text);
    IntDeinit(&value);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

bool test_int_upper_hex_radix(void) {
    WriteFmt("Testing Int uppercase radix conversion\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int value = IntFrom(0xBEEF, &alloc.base);
    Str text  = IntToStrRadix(&value, 16, true);

    bool result = ZstrCompare(StrBegin(&text), "BEEF") == 0;

    StrDeinit(&text);
    IntDeinit(&value);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

bool test_int_try_to_str_allocator_inheritance(void) {
    WriteFmt("Testing IntTryToStr allocator behavior\n");

    DefaultAllocator alloc = DefaultAllocatorInit();
    Str              text;
    bool             ok;

    alloc.base.effort      = ALLOCATOR_EFFORT_RETRY;
    alloc.base.retry_limit = 4;

    Int value = IntFrom(0xBEEF, &alloc.base);

    ok = int_try_to_str_radix(&text, &value, 16, true, &alloc.base);

    bool result = ok && (ZstrCompare(StrBegin(&text), "BEEF") == 0) &&
                  (StrAllocator(&text)->effort == alloc.base.effort) &&
                  (StrAllocator(&text)->retry_limit == alloc.base.retry_limit);

    StrDeinit(&text);
    IntDeinit(&value);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

bool test_int_compare_ignores_leading_zeros(void) {
    WriteFmt("Testing IntCompare leading-zero normalization\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int lhs = IntFromBinary("0001011", &alloc.base);
    Int rhs = IntFrom(11, &alloc.base);

    bool result = IntCompare(&lhs, &rhs) == 0;
    result      = result && IntEQ(&lhs, &rhs);

    IntDeinit(&lhs);
    IntDeinit(&rhs);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

bool test_int_zero_binary(void) {
    WriteFmt("Testing Int zero binary conversion\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int  zero  = IntFromBinary("0", &alloc.base);
    Str  text  = IntToBinary(&zero);
    bool error = true;

    bool result = IntBitLength(&zero) == 0;
    result      = result && IntIsZero(&zero);
    result      = result && (IntToU64(&zero, &error) == 0);
    result      = result && !error;
    result      = result && (ZstrCompare(StrBegin(&text), "0") == 0);

    StrDeinit(&text);
    IntDeinit(&zero);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

bool test_int_binary_prefix_and_separators(void) {
    WriteFmt("Testing Int binary prefix and separators\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int value = IntFromBinary("0b1010_0011", &alloc.base);

    bool result = IntToU64(&value) == 163;
    result      = result && (IntBitLength(&value) == 8);

    IntDeinit(&value);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

bool test_int_octal_round_trip(void) {
    WriteFmt("Testing Int octal round trip\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int value = IntFromOctStr("0o7_55", &alloc.base);
    Str text  = IntToOctStr(&value);

    bool result = IntToU64(&value) == 493;
    result      = result && (ZstrCompare(StrBegin(&text), "755") == 0);

    StrDeinit(&text);
    IntDeinit(&value);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

bool test_int_hex_round_trip(void) {
    WriteFmt("Testing Int hex round trip\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Zstr hex   = "deadbeefcafebabe1234";
    Int  value = IntFromHexStr(hex, &alloc.base);
    Str  text  = IntToHexStr(&value);

    bool result = ZstrCompare(StrBegin(&text), hex) == 0;

    StrDeinit(&text);
    IntDeinit(&value);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

bool test_int_from_binary_invalid_digit(void) {
    WriteFmt("Testing IntFromBinary invalid digit handling\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int  parsed = IntFromBinary("10a1", &alloc.base);
    Int  value  = IntInit(&alloc.base);
    bool result = !IntTryFromBinary(&value, "10a1");

    result = result && IntIsZero(&parsed);
    result = result && IntIsZero(&value);

    IntDeinit(&parsed);
    IntDeinit(&value);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

bool test_int_from_decimal_invalid_digit(void) {
    WriteFmt("Testing IntFromStr invalid digit handling\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int  parsed = IntFromStr("12x3", &alloc.base);
    Int  value  = IntInit(&alloc.base);
    bool result = !IntTryFromStr(&value, "12x3");

    result = result && IntIsZero(&parsed);
    result = result && IntIsZero(&value);

    IntDeinit(&parsed);
    IntDeinit(&value);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

bool test_int_from_hex_invalid_digit(void) {
    WriteFmt("Testing IntFromHexStr invalid digit handling\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int  parsed = IntFromHexStr("12g3", &alloc.base);
    Int  value  = IntInit(&alloc.base);
    bool result = !IntTryFromHexStr(&value, "12g3");

    result = result && IntIsZero(&parsed);
    result = result && IntIsZero(&value);

    IntDeinit(&parsed);
    IntDeinit(&value);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

bool test_int_from_radix_invalid_digit(void) {
    WriteFmt("Testing IntFromStrRadix invalid digit handling\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int  parsed = IntFromStrRadix("102", 2, &alloc.base);
    Int  value  = IntInit(&alloc.base);
    bool result = !IntTryFromStrRadix(&value, "102", 2);

    result = result && IntIsZero(&parsed);
    result = result && IntIsZero(&value);

    IntDeinit(&parsed);
    IntDeinit(&value);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

bool test_int_from_radix_invalid_radix(void) {
    WriteFmt("Testing IntFromStrRadix invalid radix handling\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int  parsed = IntFromStrRadix("10", 1, &alloc.base);
    Int  value  = IntInit(&alloc.base);
    bool result = !IntTryFromStrRadix(&value, "10", 1);

    result = result && IntIsZero(&parsed);
    result = result && IntIsZero(&value);

    IntDeinit(&parsed);
    IntDeinit(&value);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

bool test_int_to_u64_overflow(void) {
    WriteFmt("Testing IntToU64 overflow handling\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int  value = IntFrom(1, &alloc.base);
    u64  out   = 0;
    bool error = false;

    IntShiftLeft(&value, 64);

    bool result = !IntTryToU64(&value, &out);
    result      = result && (IntToU64(&value, &error) == 0);
    result      = result && error;
    result      = result && (out == 0);

    IntDeinit(&value);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

bool test_int_to_str_radix_invalid_radix(void) {
    WriteFmt("Testing IntToStrRadix invalid radix handling\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int value = IntFrom(255, &alloc.base);
    Str text  = IntToStrRadix(&value, 37, false);

    bool result = StrLen(&text) == 0;

    StrDeinit(&text);
    IntDeinit(&value);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

bool test_int_from_binary_null(void) {
    WriteFmt("Testing IntFromBinary NULL handling\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int  parsed = IntFromBinary((Zstr)NULL, &alloc.base);
    Int  value  = IntInit(&alloc.base);
    bool result = !IntTryFromBinary(&value, (Zstr)NULL);

    result = result && IntIsZero(&parsed);
    result = result && IntIsZero(&value);

    IntDeinit(&parsed);
    IntDeinit(&value);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

bool test_int_from_decimal_null(void) {
    WriteFmt("Testing IntFromStr NULL handling\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int  parsed = IntFromStr((Zstr)NULL, &alloc.base);
    Int  value  = IntInit(&alloc.base);
    bool result = !IntTryFromStr(&value, (Zstr)NULL);

    result = result && IntIsZero(&parsed);
    result = result && IntIsZero(&value);

    IntDeinit(&parsed);
    IntDeinit(&value);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

bool test_int_from_radix_null(void) {
    WriteFmt("Testing IntFromStrRadix NULL handling\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int  parsed = IntFromStrRadix((Zstr)NULL, 10, &alloc.base);
    Int  value  = IntInit(&alloc.base);
    bool result = !IntTryFromStrRadix(&value, (Zstr)NULL, 10);

    result = result && IntIsZero(&parsed);
    result = result && IntIsZero(&value);

    IntDeinit(&parsed);
    IntDeinit(&value);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

bool test_int_from_octal_null(void) {
    WriteFmt("Testing IntFromOctStr NULL handling\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int  parsed = IntFromOctStr((Zstr)NULL, &alloc.base);
    Int  value  = IntInit(&alloc.base);
    bool result = !IntTryFromOctStr(&value, (Zstr)NULL);

    result = result && IntIsZero(&parsed);
    result = result && IntIsZero(&value);

    IntDeinit(&parsed);
    IntDeinit(&value);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

bool test_int_from_hex_null(void) {
    WriteFmt("Testing IntFromHexStr NULL handling\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int  parsed = IntFromHexStr((Zstr)NULL, &alloc.base);
    Int  value  = IntInit(&alloc.base);
    bool result = !IntTryFromHexStr(&value, (Zstr)NULL);

    result = result && IntIsZero(&parsed);
    result = result && IntIsZero(&value);

    IntDeinit(&parsed);
    IntDeinit(&value);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

bool test_int_from_bytes_le_null(void) {
    WriteFmt("Testing IntFromBytesLE NULL handling\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    IntFromBytesLE(NULL, 1, &alloc.base);
    DefaultAllocatorDeinit(&alloc);
    return false;
}

bool test_int_to_bytes_le_null(void) {
    WriteFmt("Testing IntToBytesLE NULL handling\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int value = IntFrom(1, &alloc.base);
    IntToBytesLE(&value, NULL, 1);
    DefaultAllocatorDeinit(&alloc);
    return false;
}

bool test_int_to_bytes_be_zero_max_len(void) {
    WriteFmt("Testing IntToBytesBE zero max_len handling\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int value = IntFrom(1, &alloc.base);
    u8  byte  = 0;

    IntToBytesBE(&value, &byte, 0);
    DefaultAllocatorDeinit(&alloc);
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
    };

    // NULL-input tests: now expected to LOG_FATAL via the strict-
    // contract rule (programmer errors abort). The deadend driver
    // catches the abort and treats it as PASS.
    TestFunction deadend_tests[] = {
        test_int_from_binary_null,
        test_int_from_decimal_null,
        test_int_from_radix_null,
        test_int_from_octal_null,
        test_int_from_hex_null,
        test_int_from_bytes_le_null,
        test_int_to_bytes_le_null,
        test_int_to_bytes_be_zero_max_len,
    };

    int total_tests         = sizeof(tests) / sizeof(tests[0]);
    int total_deadend_tests = sizeof(deadend_tests) / sizeof(deadend_tests[0]);

    return run_test_suite(tests, total_tests, deadend_tests, total_deadend_tests, "Int.Convert");
}
