#include <Misra/Std/Allocator/Default.h>
#include <Misra/Std/Zstr.h>
#include <Misra/Std/Allocator/Debug.h>
#include <Misra/Std/Container/Int.h>
#include <Misra/Std/Log.h>
#include <Misra/Std/Memory.h>
#include <Misra/Types.h>

#include "../../Util/TestRunner.h"

static DebugAllocatorConfig blind_cfg(void) {
    return (DebugAllocatorConfig) {.capture_traces = false, .detect_overflow = false, .track_freed_history = false};
}

#define LEAK_CLEAN(dbg) (DebugAllocatorLiveCount(&(dbg)) == 0 && DebugAllocatorLiveBytes(&(dbg)) == 0)

#define LEAK_CFG                                                                                                       \
    ((DebugAllocatorConfig) {.capture_traces = false, .detect_overflow = false, .track_freed_history = false})

bool        test_int_from_unsigned_integer(void);
bool        test_int_bytes_le_round_trip(void);
bool        test_int_bytes_be_round_trip(void);
bool        test_int_binary_round_trip(void);
bool        test_int_decimal_round_trip(void);
bool        test_int_radix_round_trip(void);
bool        test_int_upper_hex_radix(void);
bool        test_int_try_to_str_allocator_inheritance(void);
bool        test_int_compare_ignores_leading_zeros(void);
bool        test_int_zero_binary(void);
bool        test_int_binary_prefix_and_separators(void);
bool        test_int_octal_round_trip(void);
bool        test_int_hex_round_trip(void);
bool        test_int_from_binary_invalid_digit(void);
bool        test_int_from_decimal_invalid_digit(void);
bool        test_int_from_hex_invalid_digit(void);
bool        test_int_from_radix_invalid_digit(void);
bool        test_int_from_radix_invalid_radix(void);
bool        test_int_to_u64_overflow(void);
bool        test_int_to_str_radix_invalid_radix(void);
bool        test_int_from_binary_null(void);
bool        test_int_try_from_binary_null(void);
bool        test_int_from_decimal_null(void);
bool        test_int_try_from_decimal_null(void);
bool        test_int_from_radix_null(void);
bool        test_int_try_from_radix_null(void);
bool        test_int_from_octal_null(void);
bool        test_int_try_from_octal_null(void);
bool        test_int_from_hex_null(void);
bool        test_int_try_from_hex_null(void);
bool        test_int_from_bytes_le_null(void);
bool        test_int_to_bytes_le_null(void);
bool        test_int_to_bytes_be_zero_max_len(void);
static bool test_m10_mul_sparse_bits(void);
bool        test_m12_radix_underscore_only_rejected(void);
bool        test_m12_radix_valid_digit_parses(void);
bool        test_m12_radix_invalid_radix_rejected(void);
static bool test_m17_binary_prefix_lower(void);
static bool test_m17_binary_prefix_upper(void);
static bool test_m17_binary_no_prefix(void);
static bool test_m17_oct_prefix_lower(void);
static bool test_m17_oct_prefix_upper(void);
static bool test_m17_oct_no_prefix(void);
bool        test_m19_to_str_radix_roundtrip(void);
bool        test_m19_from_str_radix_mul_chain(void);
bool        test_m20_radix_digit_uppercase_bounds(void);
bool        test_m20_radix_digit_uppercase_value(void);
bool        test_m20_from_str_str_no_sign(void);
bool        test_m20_from_str_str_plus_sign(void);
bool        test_m21_radix_str_basic(void);
bool        test_m21_radix_str_plus_sign(void);
bool        test_m21_to_bytes_le_count(void);
bool        test_m21_to_bytes_le_single(void);
bool        test_m21_to_bytes_be_count(void);
bool        test_m21_to_bytes_be_single(void);
bool        test_m22_from_str_zstr_leading_plus(void);
bool        test_m22_from_str_zstr_no_sign(void);
bool        test_m23_radix_zstr_plus_sign_skipped(void);
bool        test_m23_oct_zstr_no_prefix_parses_all(void);
bool        test_m23_oct_zstr_leading_zero_not_prefix(void);
bool        test_m23_hash_uses_all_bytes(void);
bool        test_m24_from_bytes_be_null_zero(void);
bool        test_m24_from_bytes_be_roundtrip(void);
bool        test_m24_binary_reject_nonbinary_prefix(void);
bool        test_m24_binary_accept_real_prefix(void);
bool        test_m27_from_bytes_le_null_zero_no_abort(void);
bool        test_m27_from_bytes_le_trailing_zero_normalized(void);
static bool test_m28_from_str_str_parses(void);
static bool test_m28_from_str_radix_str_parses(void);
bool        test_m29_hex_str_str_return_valid_and_value(void);
bool        test_m29_hex_str_str_return_invalid(void);
bool        test_fe_603_to_bytes_be_content(void);

bool test_int_from_unsigned_integer(void) {
    WriteFmt("Testing IntFrom with unsigned integer\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int value = IntFrom(13, ALLOCATOR_OF(&alloc));
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
    Int value   = IntFromBytesLE(bytes, sizeof(bytes), ALLOCATOR_OF(&alloc));
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
    Int value   = IntFromBytesBE(bytes, sizeof(bytes), ALLOCATOR_OF(&alloc));
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

    Int value = IntFromBinary("001011", ALLOCATOR_OF(&alloc));
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
    Int  value  = IntFromStr(digits, ALLOCATOR_OF(&alloc));
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

    Int value = IntFromStrRadix("zz", 36, ALLOCATOR_OF(&alloc));
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

    Int value = IntFrom(0xBEEF, ALLOCATOR_OF(&alloc));
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

    // intentional bypass: no public setter on `Allocator` for effort /
    // retry_limit -- pre-seeded directly so the inheritance path below
    // can be observed end-to-end.
    alloc.base.effort      = ALLOCATOR_EFFORT_RETRY;
    alloc.base.retry_limit = 4;

    Int value = IntFrom(0xBEEF, ALLOCATOR_OF(&alloc));

    ok = int_try_to_str_radix(&text, &value, 16, true, ALLOCATOR_OF(&alloc));

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

    Int lhs = IntFromBinary("0001011", ALLOCATOR_OF(&alloc));
    Int rhs = IntFrom(11, ALLOCATOR_OF(&alloc));

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

    Int  zero  = IntFromBinary("0", ALLOCATOR_OF(&alloc));
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

    Int value = IntFromBinary("0b1010_0011", ALLOCATOR_OF(&alloc));

    bool result = IntToU64(&value) == 163;
    result      = result && (IntBitLength(&value) == 8);

    IntDeinit(&value);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

bool test_int_octal_round_trip(void) {
    WriteFmt("Testing Int octal round trip\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int value = IntFromOctStr("0o7_55", ALLOCATOR_OF(&alloc));
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
    Int  value = IntFromHexStr(hex, ALLOCATOR_OF(&alloc));
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

    Int  parsed = IntFromBinary("10a1", ALLOCATOR_OF(&alloc));
    Int  value  = IntInit(ALLOCATOR_OF(&alloc));
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

    Int  parsed = IntFromStr("12x3", ALLOCATOR_OF(&alloc));
    Int  value  = IntInit(ALLOCATOR_OF(&alloc));
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

    Int  parsed = IntFromHexStr("12g3", ALLOCATOR_OF(&alloc));
    Int  value  = IntInit(ALLOCATOR_OF(&alloc));
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

    Int  parsed = IntFromStrRadix("102", 2, ALLOCATOR_OF(&alloc));
    Int  value  = IntInit(ALLOCATOR_OF(&alloc));
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

    Int  parsed = IntFromStrRadix("10", 1, ALLOCATOR_OF(&alloc));
    Int  value  = IntInit(ALLOCATOR_OF(&alloc));
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

    Int  value = IntFrom(1, ALLOCATOR_OF(&alloc));
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

    Int value = IntFrom(255, ALLOCATOR_OF(&alloc));
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

    IntFromBinary((Zstr)NULL, ALLOCATOR_OF(&alloc));
    DefaultAllocatorDeinit(&alloc);
    return false;
}

bool test_int_try_from_binary_null(void) {
    WriteFmt("Testing IntTryFromBinary NULL handling\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int value = IntInit(ALLOCATOR_OF(&alloc));
    IntTryFromBinary(&value, (Zstr)NULL);
    IntDeinit(&value);
    DefaultAllocatorDeinit(&alloc);
    return false;
}

bool test_int_from_decimal_null(void) {
    WriteFmt("Testing IntFromStr NULL handling\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    IntFromStr((Zstr)NULL, ALLOCATOR_OF(&alloc));
    DefaultAllocatorDeinit(&alloc);
    return false;
}

bool test_int_try_from_decimal_null(void) {
    WriteFmt("Testing IntTryFromStr NULL handling\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int value = IntInit(ALLOCATOR_OF(&alloc));
    IntTryFromStr(&value, (Zstr)NULL);
    IntDeinit(&value);
    DefaultAllocatorDeinit(&alloc);
    return false;
}

bool test_int_from_radix_null(void) {
    WriteFmt("Testing IntFromStrRadix NULL handling\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    IntFromStrRadix((Zstr)NULL, 10, ALLOCATOR_OF(&alloc));
    DefaultAllocatorDeinit(&alloc);
    return false;
}

bool test_int_try_from_radix_null(void) {
    WriteFmt("Testing IntTryFromStrRadix NULL handling\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int value = IntInit(ALLOCATOR_OF(&alloc));
    IntTryFromStrRadix(&value, (Zstr)NULL, 10);
    IntDeinit(&value);
    DefaultAllocatorDeinit(&alloc);
    return false;
}

bool test_int_from_octal_null(void) {
    WriteFmt("Testing IntFromOctStr NULL handling\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    IntFromOctStr((Zstr)NULL, ALLOCATOR_OF(&alloc));
    DefaultAllocatorDeinit(&alloc);
    return false;
}

bool test_int_try_from_octal_null(void) {
    WriteFmt("Testing IntTryFromOctStr NULL handling\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int value = IntInit(ALLOCATOR_OF(&alloc));
    IntTryFromOctStr(&value, (Zstr)NULL);
    IntDeinit(&value);
    DefaultAllocatorDeinit(&alloc);
    return false;
}

bool test_int_from_hex_null(void) {
    WriteFmt("Testing IntFromHexStr NULL handling\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    IntFromHexStr((Zstr)NULL, ALLOCATOR_OF(&alloc));
    DefaultAllocatorDeinit(&alloc);
    return false;
}

bool test_int_try_from_hex_null(void) {
    WriteFmt("Testing IntTryFromHexStr NULL handling\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int value = IntInit(ALLOCATOR_OF(&alloc));
    IntTryFromHexStr(&value, (Zstr)NULL);
    IntDeinit(&value);
    DefaultAllocatorDeinit(&alloc);
    return false;
}

bool test_int_from_bytes_le_null(void) {
    WriteFmt("Testing IntFromBytesLE NULL handling\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    IntFromBytesLE(NULL, 1, ALLOCATOR_OF(&alloc));
    DefaultAllocatorDeinit(&alloc);
    return false;
}

bool test_int_to_bytes_le_null(void) {
    WriteFmt("Testing IntToBytesLE NULL handling\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int value = IntFrom(1, ALLOCATOR_OF(&alloc));
    IntToBytesLE(&value, NULL, 1);
    DefaultAllocatorDeinit(&alloc);
    return false;
}

bool test_int_to_bytes_be_zero_max_len(void) {
    WriteFmt("Testing IntToBytesBE zero max_len handling\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int value = IntFrom(1, ALLOCATOR_OF(&alloc));
    u8  byte  = 0;

    IntToBytesBE(&value, &byte, 0);
    DefaultAllocatorDeinit(&alloc);
    return false;
}


///
/// int_mul with b having every-other bit set, to ensure the per-bit
/// `continue` / shift / add path tracks correct bit positions. Uses
/// b = 0b10101 = 21, a = 13 -> 273.
///
static bool test_m10_mul_sparse_bits(void) {
    WriteFmt("Testing int_mul sparse-bit multiplier\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int a       = IntFrom((u64)13u, &alloc.base);
    Int b       = IntFrom((u64)21u, &alloc.base);
    Int product = IntInit(&alloc.base);

    bool ok = IntMul(&product, &a, &b);

    bool result = ok && (IntToU64(&product) == (u64)273u);
    /* 273 = 0b100010001, 9 significant bits */
    result = result && (IntBitLength(&product) == 9);

    IntDeinit(&a);
    IntDeinit(&b);
    IntDeinit(&product);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

//
// Kills the `bool saw_digit = false;` initializer mutation
// (Int.c:319, cxx_init_const -> true). If the flag starts true, an
// input containing no actual digits (only an underscore separator,
// which `continue`s) would be accepted as a valid empty parse and
// IntTryFromStrRadix would wrongly return true. Real code keeps
// saw_digit false through the loop and reports the no-digit failure.
//
bool test_m12_radix_underscore_only_rejected(void) {
    WriteFmt("Testing IntTryFromStrRadix rejects underscore-only input\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int  value  = IntInit(ALLOCATOR_OF(&alloc));
    bool result = !IntTryFromStrRadix(&value, "_", 10);

    result = result && IntIsZero(&value);

    IntDeinit(&value);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

//
// Kills the `saw_digit = true;` assignment mutation
// (Int.c:346, cxx_assign_const -> false): if that assignment is
// neutralised, a perfectly valid digit run is reported as "no valid
// digits" and parsing fails. Real code parses "7" to the value 7 and
// returns true.
//
bool test_m12_radix_valid_digit_parses(void) {
    WriteFmt("Testing IntTryFromStrRadix accepts a valid digit run\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int  value  = IntInit(ALLOCATOR_OF(&alloc));
    bool ok     = IntTryFromStrRadix(&value, "7", 10);
    bool result = ok && (IntToU64(&value) == 7);

    IntDeinit(&value);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

//
// Kills the radix-validation call mutation
// (Int.c:328, cxx_replace_scalar_call on int_validate_radix). The
// test pins both directions: a valid radix (10) must still parse, and
// an out-of-range radix (37) must be rejected before any digit is
// consumed. If the validation call result is forced to a constant,
// one of these two assertions fails.
//
bool test_m12_radix_invalid_radix_rejected(void) {
    WriteFmt("Testing IntTryFromStrRadix radix-range gate\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int valid   = IntInit(ALLOCATOR_OF(&alloc));
    Int invalid = IntInit(ALLOCATOR_OF(&alloc));

    bool ok_valid       = IntTryFromStrRadix(&valid, "5", 10);
    bool rejected_radix = !IntTryFromStrRadix(&invalid, "5", 37);

    bool result = ok_valid && (IntToU64(&valid) == 5);
    result      = result && rejected_radix;
    result      = result && IntIsZero(&invalid);

    IntDeinit(&valid);
    IntDeinit(&invalid);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

///
/// Positive correctness guard for int_try_from_binary_str on a prefixed
/// lowercase ("0b") input. Real code detects the "0b" prefix (line 812-813:
/// StrLen>=2 && char0=='0' && (char1=='b'||char1=='B')), sets start=2 (line
/// 814) and parses "101" as binary -> 5.
///
/// Kills several operator/value mutants that all collapse the prefix-detection
/// path on this single input "0b101":
///   - L812:53 (char0=='0' -> !=): condition false -> start stays 0 -> parse
///     "0b101" -> 'b' is an invalid radix-2 digit -> returns false (real: 5).
///   - L813:31 (char1=='b' -> !=): 'b'!='b' false, 'b'=='B' false -> OR false
///     -> start 0 -> parse fails (real: 5).
///   - L812:24 ge_to_lt (StrLen>=2 -> <2): false -> whole cond false -> start 0
///     -> parse "0b101" fails (real: 5).
///   - L814:15 (start=2 -> other const): start=1 -> parse "b101" -> invalid, or
///     start=0 -> parse "0b101" -> invalid -> false (real: 5).
///   - L817:12 replace_scalar_call (int_try_from_str_radix_impl return replaced
///     by a constant): asserting result==true AND value==5 catches a forced
///     false (result wrong) and a forced true with out left at 0 (value wrong).
///
static bool test_m17_binary_prefix_lower(void) {
    WriteFmt("Testing int_try_from_binary_str 0b-prefixed lowercase\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Str  text   = StrInitFromZstr("0b101", &alloc);
    Int  out    = IntInit(&alloc.base);
    bool ok     = int_try_from_binary_str(&out, &text);
    bool result = ok && (IntToU64(&out) == 5u);

    StrDeinit(&text);
    IntDeinit(&out);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

///
/// Positive correctness guard for int_try_from_binary_str on a prefixed
/// UPPERCASE ("0B") input. With lowercase 'b' the OR short-circuits on the
/// first operand, masking the second; an uppercase prefix is the only input
/// that exercises the char1=='B' comparison.
///
/// Kills L813:62 (char1=='B' -> !='B'): real path is char1=='b'? 'B'=='b'
/// false, then 'B'=='B' true -> OR true -> start=2 -> "101" = 5. Mutated:
/// 'B'!='B' false, OR false -> start stays 0 -> parse "0B101" -> 'B' invalid
/// for radix 2 -> returns false. Real returns true/5, mutant false.
///
static bool test_m17_binary_prefix_upper(void) {
    WriteFmt("Testing int_try_from_binary_str 0B-prefixed uppercase\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Str  text   = StrInitFromZstr("0B101", &alloc);
    Int  out    = IntInit(&alloc.base);
    bool ok     = int_try_from_binary_str(&out, &text);
    bool result = ok && (IntToU64(&out) == 5u);

    StrDeinit(&text);
    IntDeinit(&out);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

///
/// Positive correctness guard for int_try_from_binary_str on a NON-prefixed
/// input. The prefix-detection block does not fire (char0=='1'), so the parse
/// begins at the initial value of `start` (line 806: u64 start = 0).
///
/// Kills L806:9 cxx_init_const (start initialised to a non-zero const): a
/// non-prefixed string is the only input where the init value survives to the
/// parse (a prefixed string overwrites start=2). "1101" real start=0 -> binary
/// 1101 = 13; with start=1 the leading '1' is dropped -> "101" = 5. Real 13,
/// mutant 5.
///
static bool test_m17_binary_no_prefix(void) {
    WriteFmt("Testing int_try_from_binary_str non-prefixed\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Str  text   = StrInitFromZstr("1101", &alloc);
    Int  out    = IntInit(&alloc.base);
    bool ok     = int_try_from_binary_str(&out, &text);
    bool result = ok && (IntToU64(&out) == 13u);

    StrDeinit(&text);
    IntDeinit(&out);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

///
/// Positive correctness guard for int_try_from_oct_str_str on a prefixed
/// lowercase ("0o") input. Real code detects the "0o" prefix (line 861-862),
/// sets start=2 (line 863) and parses "17" as octal -> 1*8 + 7 = 15.
///
/// Kills the prefix-detection mutants that collapse on "0o17":
///   - L861:51 (char0=='0' -> !=): cond false -> start 0 -> parse "0o17" -> 'o'
///     is an invalid radix-8 digit -> false (real: 15).
///   - L862:30 (char1=='o' -> !=): 'o'!='o' false, 'o'=='O' false -> OR false
///     -> start 0 -> parse fails (real: 15).
///   - L861:23 ge_to_lt (StrLen>=2 -> <2): false -> whole cond false -> start 0
///     -> parse "0o17" fails (real: 15).
///   - L863:15 (start=2 -> other const): start=1 -> "o17" invalid, or start=0
///     -> "0o17" invalid -> false (real: 15).
///   - L866:12 replace_scalar_call (impl return replaced by a constant):
///     result==true AND value==15 catches forced false and forced true with
///     out left at 0.
///
static bool test_m17_oct_prefix_lower(void) {
    WriteFmt("Testing int_try_from_oct_str_str 0o-prefixed lowercase\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Str  text   = StrInitFromZstr("0o17", &alloc);
    Int  out    = IntInit(&alloc.base);
    bool ok     = int_try_from_oct_str_str(&out, &text);
    bool result = ok && (IntToU64(&out) == 15u);

    StrDeinit(&text);
    IntDeinit(&out);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

///
/// Positive correctness guard for int_try_from_oct_str_str on a prefixed
/// UPPERCASE ("0O") input -- the only input exercising the char1=='O'
/// comparison (lowercase 'o' short-circuits the OR on the first operand).
///
/// Kills L862:60 (char1=='O' -> !='O'): real path is char1=='o'? 'O'=='o'
/// false, then 'O'=='O' true -> OR true -> start=2 -> "17" = 15. Mutated:
/// 'O'!='O' false, OR false -> start stays 0 -> parse "0O17" -> 'O' invalid for
/// radix 8 -> false. Real true/15, mutant false.
///
static bool test_m17_oct_prefix_upper(void) {
    WriteFmt("Testing int_try_from_oct_str_str 0O-prefixed uppercase\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Str  text   = StrInitFromZstr("0O17", &alloc);
    Int  out    = IntInit(&alloc.base);
    bool ok     = int_try_from_oct_str_str(&out, &text);
    bool result = ok && (IntToU64(&out) == 15u);

    StrDeinit(&text);
    IntDeinit(&out);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

///
/// Positive correctness guard for int_try_from_oct_str_str on a NON-prefixed
/// input. The prefix-detection block does not fire (char0=='1'), so the parse
/// begins at the initial value of `start` (line 855: u64 start = 0).
///
/// Kills L855:9 cxx_init_const (start initialised non-zero): "17" real start=0
/// -> octal 17 = 15; with start=1 the leading '1' is dropped -> "7" = 7. Real
/// 15, mutant 7.
///
static bool test_m17_oct_no_prefix(void) {
    WriteFmt("Testing int_try_from_oct_str_str non-prefixed\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Str  text   = StrInitFromZstr("17", &alloc);
    Int  out    = IntInit(&alloc.base);
    bool ok     = int_try_from_oct_str_str(&out, &text);
    bool result = ok && (IntToU64(&out) == 15u);

    StrDeinit(&text);
    IntDeinit(&out);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Exercises int_try_to_str_radix end-to-end: a non-zero value is repeatedly
// divided by the radix, each remainder mapped to a radix char, then reversed.
// A correct digit extraction loop must produce the exact decimal and hex
// strings below for several magnitudes (single digit, multi digit, large).
bool test_m19_to_str_radix_roundtrip(void) {
    WriteFmt("Testing int_try_to_str_radix digit extraction\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int small = IntFrom(7u, &alloc.base);
    Int mid   = IntFrom(255u, &alloc.base);
    Int big   = IntFromStr("123456789012345678901234567890", &alloc.base);

    Str dec_small = IntToStrRadix(&small, 10, false, &alloc.base);
    Str hex_mid   = IntToStrRadix(&mid, 16, false, &alloc.base);
    Str hex_mid_u = IntToStrRadix(&mid, 16, true, &alloc.base);
    Str dec_big   = IntToStrRadix(&big, 10, false, &alloc.base);

    bool result = (ZstrCompare(StrBegin(&dec_small), "7") == 0);
    result      = result && (ZstrCompare(StrBegin(&hex_mid), "ff") == 0);
    result      = result && (ZstrCompare(StrBegin(&hex_mid_u), "FF") == 0);
    result      = result && (ZstrCompare(StrBegin(&dec_big), "123456789012345678901234567890") == 0);

    StrDeinit(&dec_small);
    StrDeinit(&hex_mid);
    StrDeinit(&hex_mid_u);
    StrDeinit(&dec_big);
    IntDeinit(&small);
    IntDeinit(&mid);
    IntDeinit(&big);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Parsing a multi-digit radix string drives int_mul_u64_in_place once per
// digit (result = result*radix + digit). The big decimal below cannot be
// reconstructed unless every multiply-by-radix-in-place is exact, so the
// round-trip parse->serialize equality guards the in-place multiply path.
bool test_m19_from_str_radix_mul_chain(void) {
    WriteFmt("Testing int_mul_u64_in_place via radix parse\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int parsed_dec = IntFromStrRadix("987654321987654321", 10, &alloc.base);
    Int parsed_hex = IntFromStrRadix("deadbeef", 16, &alloc.base);

    Str back_dec = IntToStrRadix(&parsed_dec, 10, false, &alloc.base);
    Str back_hex = IntToStrRadix(&parsed_hex, 16, false, &alloc.base);

    bool result = (ZstrCompare(StrBegin(&back_dec), "987654321987654321") == 0);
    result      = result && (ZstrCompare(StrBegin(&back_hex), "deadbeef") == 0);

    StrDeinit(&back_dec);
    StrDeinit(&back_hex);
    IntDeinit(&parsed_dec);
    IntDeinit(&parsed_hex);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Kills the relational mutants on int_radix_digit's uppercase branch
//   line 298:  if (ch >= 'A' && ch <= 'Z')
// by parsing "AZ" in radix 36. 'A' is the lower boundary (digit 10) and 'Z'
// is the upper boundary (digit 35). Real code matches both and computes
//   A*36 + Z = 10*36 + 35 = 395.
//   - `>=`->`>`  : 'A' (== 'A') no longer matches -> 'A' becomes an invalid
//                  digit -> parse fails -> result != 395.
//   - `>=`->`<`  : ch < 'A' -> no uppercase letter matches -> parse fails.
//   - `<=`->`<`  : 'Z' (== 'Z') no longer matches -> 'Z' invalid -> parse fails.
//   - `<=`->`>`  : ch > 'Z' -> 'A'/'Z' don't match -> parse fails.
// Real code yields 395; every mutant fails to parse, giving a distinguishable
// result (parse failure -> we observe a non-true return / wrong magnitude).
bool test_m20_radix_digit_uppercase_bounds(void) {
    WriteFmt("Testing int_radix_digit uppercase boundary chars (radix 36 \"AZ\")\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int  out    = IntInit(&alloc.base);
    bool parsed = IntTryFromStrRadix(&out, "AZ", 36);

    bool result = parsed && (IntToU64(&out) == 395);

    IntDeinit(&out);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Kills the arithmetic mutants on int_radix_digit's uppercase return
//   line 299:  return 10 + (ch - 'A');
// by parsing "FF" in radix 16 (each 'F' is digit 15, so FF == 255).
//   - `10 + (ch - 'A')` -> `10 - (ch - 'A')` : 'F' digit becomes 10-5 = 5,
//        so "FF" parses as 5*16+5 = 85 instead of 255.
//   - `ch - 'A'`        -> `ch + 'A'`         : 'F' digit becomes 10+(70+65)
//        = 145 (>= radix 16) -> rejected as invalid digit -> parse fails.
// Real code yields 255; both mutants diverge (wrong value or parse failure).
bool test_m20_radix_digit_uppercase_value(void) {
    WriteFmt("Testing int_radix_digit uppercase value math (radix 16 \"FF\")\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int  out    = IntInit(&alloc.base);
    bool parsed = IntTryFromStrRadix(&out, "FF", 16);

    bool result = parsed && (IntToU64(&out) == 255);

    IntDeinit(&out);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Kills two mutants on int_try_from_str_str that corrupt the parse start
// offset when there is no leading '+':
//   line 637:  u64 start = 0;          (cxx_init_const flips 0 -> nonzero)
//   line 644:  start = 1;              (only reached when first char is '+')
// Parsing the Str "123" (no '+') must keep start == 0 so all three digits are
// consumed -> 123. If line 637's initialiser is mutated to a nonzero constant,
// the first digit '1' is skipped -> "23" -> 23 != 123. (Line 644 is not on this
// path, so this test isolates the init-const mutant.)
bool test_m20_from_str_str_no_sign(void) {
    WriteFmt("Testing int_try_from_str_str start offset with no sign (Str \"123\")\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Str  text   = StrInitFromZstr("123", &alloc);
    Int  out    = IntInit(&alloc.base);
    bool parsed = IntTryFromStr(&out, &text);

    bool result = parsed && (IntToU64(&out) == 123);

    IntDeinit(&out);
    StrDeinit(&text);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Kills the leading-'+' handling mutants on int_try_from_str_str:
//   line 643:  if (StrLen(decimal) > 0 && StrCharAt(decimal, 0) == '+')
//   line 644:      start = 1;
//   line 647:  return int_try_from_str_radix_impl(..., start, 10, true);
// Parsing the Str "+5" must strip the '+' (start = 1) and yield 5.
//   - 643 `>`->`<=` : StrLen("+5")(=2) <= 0 is false -> '+' not stripped ->
//        radix-10 parse of "+5" hits invalid digit '+' -> returns false.
//   - 643 `==`->`!=`: '+' != '+' is false -> '+' not stripped -> parse fails.
//   - 644 cxx_assign_const (start = 1 -> other) : '+' not skipped (or wrong
//        skip) -> parse of "+5" fails on '+'.
//   - 647 cxx_replace_scalar_call : the impl call is replaced by a scalar
//        (false), so the function returns false instead of true.
// Real code returns true with value 5; every mutant returns false (or wrong
// value), a distinguishable caller-observable outcome.
bool test_m20_from_str_str_plus_sign(void) {
    WriteFmt("Testing int_try_from_str_str leading '+' handling (Str \"+5\")\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Str  text   = StrInitFromZstr("+5", &alloc);
    Int  out    = IntInit(&alloc.base);
    bool parsed = IntTryFromStr(&out, &text);

    bool result = parsed && (IntToU64(&out) == 5);

    IntDeinit(&out);
    StrDeinit(&text);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

//
// int_try_from_str_radix_str: parsing a plain digit run through the Str
// overload must yield the exact value and report success. Kills the
// init-const mutation on `start` (a nonzero start would drop the leading
// digit, giving 23 instead of 123) and the replace-scalar mutation on the
// delegated parse call (which would leave `out` untouched / wrong result).
//
bool test_m21_radix_str_basic(void) {
    WriteFmt("Testing int_try_from_str_radix_str basic decimal parse\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Str  digits = StrInitFromZstr("123", ALLOCATOR_OF(&alloc));
    Int  value  = IntInit(ALLOCATOR_OF(&alloc));
    bool ok     = IntTryFromStrRadix(&value, &digits, 10);

    bool result = ok;
    result      = result && (IntToU64(&value) == 123);

    IntDeinit(&value);
    StrDeinit(&digits);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

//
// int_try_from_str_radix_str: a leading '+' sign must be consumed by the
// `start = 1` bump so the remaining run parses cleanly. Real code parses
// "+5" -> 5. Kills the gt->le mutation (StrLen>0 becoming StrLen==0 skips
// the sign branch), the eq->ne mutation (== '+' becoming != '+' skips it),
// and the assign-const mutation on `start = 1` (a different constant would
// index past the digits, producing an empty run -> parse failure). Under
// any of those mutations the '+' stays in the digit stream and the parse
// fails, so `ok` would be false and the value would not be 5.
//
bool test_m21_radix_str_plus_sign(void) {
    WriteFmt("Testing int_try_from_str_radix_str leading plus sign\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Str  digits = StrInitFromZstr("+5", ALLOCATOR_OF(&alloc));
    Int  value  = IntInit(ALLOCATOR_OF(&alloc));
    bool ok     = IntTryFromStrRadix(&value, &digits, 10);

    bool result = ok;
    result      = result && (IntToU64(&value) == 5);

    IntDeinit(&value);
    StrDeinit(&digits);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

//
// IntToBytesLE: a value that needs only 2 bytes serialized into an 8-byte
// buffer must report exactly 2 bytes written. Kills the init-const and
// replace-scalar mutations on `bytes_needed = IntByteLength(value)`: with a
// constant byte count, bytes_to_copy = MIN(const, 8) would differ from 2,
// so the written count (and the byte payload) would be wrong.
//
bool test_m21_to_bytes_le_count(void) {
    WriteFmt("Testing IntToBytesLE writes exact byte count\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int value     = IntFrom(0x1234, ALLOCATOR_OF(&alloc));
    u8  out[8]    = {0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA};
    u64 written   = IntToBytesLE(&value, out, sizeof(out));
    u8  expect[2] = {0x34, 0x12};

    bool result = (written == 2);
    result      = result && (MemCompare(out, expect, 2) == 0);

    IntDeinit(&value);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

//
// IntToBytesLE: a single-byte value exercises the inner bit loop where
// bit_idx reaches BitVecLen. The bound `bit_idx < BitVecLen` keeps the
// short-circuited BitVecGet in range; the lt->le mutation makes the guard
// `bit_idx <= BitVecLen`, calling BitVecGet at idx == length, which
// LOG_FATALs (out-of-range). Real code returns 1 byte == 0x01.
//
bool test_m21_to_bytes_le_single(void) {
    WriteFmt("Testing IntToBytesLE single-byte value\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int value   = IntFrom(1, ALLOCATOR_OF(&alloc));
    u8  out[4]  = {0xAA, 0xAA, 0xAA, 0xAA};
    u64 written = IntToBytesLE(&value, out, sizeof(out));

    bool result = (written == 1);
    result      = result && (out[0] == 0x01);

    IntDeinit(&value);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

//
// IntToBytesBE: mirror of the LE count guard for the big-endian path.
// Kills the init-const and replace-scalar mutations on
// `bytes_needed = IntByteLength(value)`.
//
bool test_m21_to_bytes_be_count(void) {
    WriteFmt("Testing IntToBytesBE writes exact byte count\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int value     = IntFrom(0x1234, ALLOCATOR_OF(&alloc));
    u8  out[8]    = {0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA};
    u64 written   = IntToBytesBE(&value, out, sizeof(out));
    u8  expect[2] = {0x12, 0x34};

    bool result = (written == 2);
    result      = result && (MemCompare(out, expect, 2) == 0);

    IntDeinit(&value);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

//
// IntToBytesBE: a single-byte value. The outer loop bound
// `i < bytes_to_copy` keeps the index `bytes_to_copy - 1 - i` in range.
// The lt->le mutation `i <= bytes_to_copy` runs one extra iteration with
// i == bytes_to_copy, computing bytes[(u64)-1] -- a wild out-of-bounds
// write that crashes. Real code returns 1 byte == 0x01.
//
bool test_m21_to_bytes_be_single(void) {
    WriteFmt("Testing IntToBytesBE single-byte value\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int value   = IntFrom(1, ALLOCATOR_OF(&alloc));
    u8  out[4]  = {0xAA, 0xAA, 0xAA, 0xAA};
    u64 written = IntToBytesBE(&value, out, sizeof(out));

    bool result = (written == 1);
    result      = result && (out[0] == 0x01);

    IntDeinit(&value);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// int_try_from_str_zstr leading-'+' handling.
//
// Kills:
//  - line 629 cxx_gt_to_le on `len > 0`: with `len <= 0` the guard is false
//    for the non-empty "+5", so start stays 0 and the '+' is fed to the radix
//    parser as a digit, which rejects it (returns false).
//  - line 630 cxx_assign_const on `start = 1;`: forcing start to a constant
//    (0) likewise leaves the '+' in the digit run, so parsing fails.
// Real code skips the leading '+' and parses 5.
bool test_m22_from_str_zstr_leading_plus(void) {
    WriteFmt("Testing int_try_from_str_zstr skips a leading '+'\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int         out  = IntInit(&alloc.base);
    const char *text = "+5";

    bool ok     = IntTryFromStr(&out, text);
    bool result = ok;
    result      = result && (IntToU64(&out) == 5u);

    IntDeinit(&out);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// int_try_from_str_zstr without a sign: a plain decimal must parse to its
// value regardless of the leading-'+' branch. Guards the normal path so the
// '+' mutants above are isolated to the sign-skip behaviour.
bool test_m22_from_str_zstr_no_sign(void) {
    WriteFmt("Testing int_try_from_str_zstr parses a plain decimal\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int         out  = IntInit(&alloc.base);
    const char *text = "42";

    bool ok     = IntTryFromStr(&out, text);
    bool result = ok;
    result      = result && (IntToU64(&out) == 42u);

    IntDeinit(&out);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// L688 (gt_to_le) and L689 (assign_const): a leading '+' must be skipped so
// that "+5" parses as 5. Under either mutation `start` stays 0 and the parser
// hits '+' as a digit and fails.
bool test_m23_radix_zstr_plus_sign_skipped(void) {
    WriteFmt("Testing int_try_from_str_radix_zstr leading-plus\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int  out = IntInit(&alloc.base);
    bool ok  = IntTryFromStrRadix(&out, "+5", (u8)10);

    bool result = ok && (IntToU64(&out) == 5);

    IntDeinit(&out);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// L839 (init_const): `start` must initialise to 0 so a prefix-less octal
// string is parsed from index 0. "17" octal == 15. If start started at 1 the
// mutant would parse only "7" == 7.
bool test_m23_oct_zstr_no_prefix_parses_all(void) {
    WriteFmt("Testing int_try_from_oct_str_zstr no-prefix\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int  out = IntInit(&alloc.base);
    bool ok  = IntTryFromOctStr(&out, "17");

    bool result = ok && (IntToU64(&out) == 15);

    IntDeinit(&out);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// L847 col69 (eq_to_ne): the prefix check is `octal[1]=='o' || octal[1]=='O'`.
// For "017" octal[1]=='1', so neither branch matches and start stays 0 ->
// "017" octal == 15. Under `octal[1] != 'O'` the OR becomes true, start jumps
// to 2 and the mutant parses only "7" == 7.
bool test_m23_oct_zstr_leading_zero_not_prefix(void) {
    WriteFmt("Testing int_try_from_oct_str_zstr leading-zero-not-prefix\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int  out = IntInit(&alloc.base);
    bool ok  = IntTryFromOctStr(&out, "017");

    bool result = ok && (IntToU64(&out) == 15);

    IntDeinit(&out);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// L936 (post_inc_to_post_dec): the byte loop must advance forward over every
// significant byte. With `i--` the counter wraps after one iteration so only
// byte[0] is mixed. Values 1 (one 0x01 byte) and 257 (two 0x01 bytes) share
// byte[0]; the real hash distinguishes them, the mutant collapses them.
bool test_m23_hash_uses_all_bytes(void) {
    WriteFmt("Testing int_hash mixes every byte\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int a = IntFrom(1, &alloc.base);
    Int b = IntFrom(257, &alloc.base);

    u64  ha     = int_hash(&a, 0);
    u64  hb     = int_hash(&b, 0);
    bool result = (ha != hb);

    IntDeinit(&a);
    IntDeinit(&b);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// ---------------------------------------------------------------------------
// int_from_bytes_be NULL guard.
// The guard `if (!bytes && len != 0)` LOG_FATALs only when a NULL buffer is
// paired with a nonzero length. Calling IntFromBytesBE(NULL, 0) is a valid
// request for the empty number and must return 0.
//
// Kills L567:23 cxx_ne_to_eq: `len != 0` -> `len == 0` makes the NULL,0 call
// abort instead of returning the zero Int.
// ---------------------------------------------------------------------------
bool test_m24_from_bytes_be_null_zero(void) {
    WriteFmt("Testing IntFromBytesBE(NULL, 0) -> 0\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int value = IntFromBytesBE((const u8 *)0, 0, &alloc.base);

    bool fail = (IntCompare(&value, 0u) != 0);

    IntDeinit(&value);
    DefaultAllocatorDeinit(&alloc);
    return !fail;
}

// Big-endian byte round-trip: {0x01, 0x02, 0x03} -> 0x010203 = 66051.
// Exercises the shift-left + add-byte accumulation loop and confirms the
// constructed magnitude is exact.
bool test_m24_from_bytes_be_roundtrip(void) {
    WriteFmt("Testing IntFromBytesBE big-endian accumulation\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    u8  bytes[3] = {0x01, 0x02, 0x03};
    Int value    = IntFromBytesBE(bytes, sizeof(bytes), &alloc.base);

    bool fail = (IntCompare(&value, 0x010203u) != 0);

    IntDeinit(&value);
    DefaultAllocatorDeinit(&alloc);
    return !fail;
}

// ---------------------------------------------------------------------------
// int_try_from_binary_zstr prefix detection.
// Prefix condition: binary[0]=='0' && (binary[1]=='b' || binary[1]=='B').
//
// Kills L798:72 cxx_eq_to_ne: `binary[1] == 'B'` -> `binary[1] != 'B'`. For
// "0c1" the real code leaves start=0 and rejects the non-binary 'c'
// (returns false). The mutant satisfies `'c' != 'B'`, strips a phantom 2-char
// prefix, parses "1", and wrongly returns true.
// ---------------------------------------------------------------------------
bool test_m24_binary_reject_nonbinary_prefix(void) {
    WriteFmt("Testing IntTryFromBinary rejects \"0c1\"\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int  out    = IntInit(&alloc.base);
    bool parsed = IntTryFromBinary(&out, "0c1");

    bool fail = parsed; // real code must reject this string.

    IntDeinit(&out);
    DefaultAllocatorDeinit(&alloc);
    return !fail;
}

// Companion positive vector: a genuine "0b101" must parse to 5. Guards the
// prefix branch is still taken for real 'b' prefixes.
bool test_m24_binary_accept_real_prefix(void) {
    WriteFmt("Testing IntTryFromBinary accepts \"0b101\" -> 5\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int  out    = IntInit(&alloc.base);
    bool parsed = IntTryFromBinary(&out, "0b101");

    bool fail = !parsed;
    fail      = fail || (IntCompare(&out, 5u) != 0);

    IntDeinit(&out);
    DefaultAllocatorDeinit(&alloc);
    return !fail;
}

// Kills cxx_ne_to_eq on int_from_bytes_le's NULL guard
//   line 512:  if (!bytes && len != 0) { LOG_FATAL("bytes is NULL"); }
// A (NULL, 0) call is legal: bytes is NULL but len is 0, so the guard does not
// fire and the function returns an empty (zero) Int via the len==0 early return.
//   - mutant (`!=` -> `==`): `!bytes && len == 0` is true for (NULL, 0), so it
//     hits LOG_FATAL and aborts.
// Real code returns a zero Int cleanly; the mutant crashes -> distinguishable.
bool test_m27_from_bytes_le_null_zero_no_abort(void) {
    WriteFmt("Testing IntFromBytesLE(NULL, 0) yields zero without aborting\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int value = IntFromBytesLE(NULL, 0, ALLOCATOR_OF(&alloc));

    bool result = IntIsZero(&value);

    IntDeinit(&value);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Kills cxx_remove_void_call on int_from_bytes_le's normalize
//   line 526:  int_normalize(&result);
// bitvec_try_from_bytes resizes the backing BitVec to len*8 bits regardless of
// leading-zero high bytes. Bytes {0x05, 0x00} (LE) encode the value 5 but
// occupy 16 raw bits; int_normalize trims to the 3 significant bits.
//   - removing int_normalize leaves the bit length at 16 instead of 3.
// Real code: value 5, IntBitLength == 3; mutant: IntBitLength == 16.
bool test_m27_from_bytes_le_trailing_zero_normalized(void) {
    WriteFmt("Testing IntFromBytesLE normalizes trailing zero bytes\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    u8  bytes[] = {0x05, 0x00};
    Int value   = IntFromBytesLE(bytes, sizeof(bytes), ALLOCATOR_OF(&alloc));

    bool result = (IntToU64(&value) == 5);
    result      = result && (IntBitLength(&value) == 3);

    IntDeinit(&value);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

///
/// int_from_str_str (the Str* overload of IntFromStr): the body's sole job
/// is to call int_try_from_str_str into the freshly-init'd zero Int. If
/// that call is replaced by a scalar constant, no parsing happens and the
/// result stays 0. Parsing "12345" through the Str path must yield 12345.
///
static bool test_m28_from_str_str_parses(void) {
    WriteFmt("Testing int_from_str_str parses Str decimal\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Str digits = StrInitFromZstr("12345", &alloc.base);
    Int value  = IntFromStr(&digits, &alloc.base);

    bool result = (IntToU64(&value) == (u64)12345u);

    IntDeinit(&value);
    StrDeinit(&digits);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

///
/// int_from_str_radix_str (the Str* overload of IntFromStrRadix): same
/// shape as above but for the radix path. Parsing "ff" in base 16 through
/// the Str path must yield 255. A replaced call leaves the result 0.
///
static bool test_m28_from_str_radix_str_parses(void) {
    WriteFmt("Testing int_from_str_radix_str parses Str hex\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Str digits = StrInitFromZstr("ff", &alloc.base);
    Int value  = IntFromStrRadix(&digits, (u8)16u, &alloc.base);

    bool result = (IntToU64(&value) == (u64)255u);

    IntDeinit(&value);
    StrDeinit(&digits);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

///
/// int_try_from_hex_str_str: the `Str*` overload parses a hex Str and returns a
/// bool that flows out as the function's own return value (Int.c:903 col 12,
/// `return int_try_from_str_radix_impl(...)`). cxx_replace_scalar_call would
/// force that return to a constant, decoupling it from the actual parse result.
/// A valid hex input ("ff" == 255) must return true AND populate the value;
/// asserting both the boolean and the parsed magnitude pins the real call.
///
bool test_m29_hex_str_str_return_valid_and_value(void) {
    WriteFmt("Testing int_try_from_hex_str_str valid return + value\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Str hex   = StrInitFromZstr("ff", &alloc.base);
    Int value = IntInit(&alloc.base);

    bool ok = int_try_from_hex_str_str(&value, &hex);

    bool result = ok;
    result      = result && (IntToU64(&value) == 255u);

    StrDeinit(&hex);
    IntDeinit(&value);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

///
/// int_try_from_hex_str_str must return false on an invalid hex digit ("12g3").
/// Pairs with the success test: together they pin Int.c:903's returned bool to
/// the genuine parse outcome (true for valid, false for invalid), so no scalar
/// constant can satisfy both. A real reject also leaves the value at zero.
///
bool test_m29_hex_str_str_return_invalid(void) {
    WriteFmt("Testing int_try_from_hex_str_str invalid return\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Str hex   = StrInitFromZstr("12g3", &alloc.base);
    Int value = IntInit(&alloc.base);

    bool ok = int_try_from_hex_str_str(&value, &hex);

    bool result = !ok;
    result      = result && IntIsZero(&value);

    StrDeinit(&hex);
    IntDeinit(&value);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// 603: IntToBytesBE inner copy loop `for (i = 0; i < bytes_to_copy; i++)`.
// The faithful swap `i >= bytes_to_copy` is false at entry (i=0), so the
// loop body never runs and the written bytes stay at their MemSet-zero
// value instead of the big-endian digits. 0x0102 must serialise to
// {0x01, 0x02}; the mutant leaves {0x00, 0x00}.
bool test_fe_603_to_bytes_be_content(void) {
    WriteFmt("Testing IntToBytesBE big-endian content\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    Int value = IntFrom(0x0102, &alloc.base);
    u8  buf[4];
    MemSet(buf, 0xAA, sizeof(buf));

    u64 written = IntToBytesBE(&value, buf, sizeof(buf));

    bool result = written == 2;
    result      = result && (buf[0] == 0x01);
    result      = result && (buf[1] == 0x02);

    IntDeinit(&value);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

bool test_to_str_radix_no_leak(void) {
    DebugAllocator dbg = DebugAllocatorInitWith(LEAK_CFG);
    Allocator     *a   = ALLOCATOR_OF(&dbg);

    // Drives int_try_to_str_radix loop (760, 772) and int_div_u64_rem
    // (1583/1584) and int_mod_u64 (1630).
    Int  v   = IntFrom(1234567u, a);
    Str  s   = IntToStrRadix(&v, 10, false, a);
    bool ok  = StrLen(&s) == 7;
    u64  rem = 0;
    {
        Int q = IntInit(a);
        rem   = int_div_u64_rem(&q, &v, 1000u);
        IntDeinit(&q);
    }
    ok = ok && rem == 567u;
    ok = ok && int_mod_u64(&v, 100u) == 67u;

    StrDeinit(&s);
    IntDeinit(&v);
    ok = ok && LEAK_CLEAN(dbg);
    DebugAllocatorDeinit(&dbg);
    return ok;
}

bool test_from_bytes_be_no_leak(void) {
    DebugAllocator dbg = DebugAllocatorInitWith(LEAK_CFG);

    u8  bytes[] = {0x01, 0x02, 0x03, 0x04};
    Int v       = IntFromBytesBE(bytes, sizeof(bytes), &dbg);

    bool ok = IntToU64(&v) == 0x01020304u;

    IntDeinit(&v);
    ok = ok && LEAK_CLEAN(dbg);
    DebugAllocatorDeinit(&dbg);
    return ok;
}

bool test_from_str_nonempty_out_no_leak(void) {
    DebugAllocator dbg = DebugAllocatorInitWith(LEAK_CFG);
    Allocator     *a   = ALLOCATOR_OF(&dbg);

    Int out = IntFrom(987654321u, a); // pre-populated: holds a live buffer

    bool ok = IntTryFromStr(&out, "123456789");
    ok      = ok && IntToU64(&out) == 123456789u;

    IntDeinit(&out);
    ok = ok && LEAK_CLEAN(dbg);
    DebugAllocatorDeinit(&dbg);
    return ok;
}

bool test_blind_to_str_radix_big(void) {
    DebugAllocator dbg   = DebugAllocatorInitWith(blind_cfg());
    Allocator     *alloc = ALLOCATOR_OF(&dbg);

    Int value  = IntFromStr("123456789012345678901234567890", alloc);
    Str hex    = IntToStrRadix(&value, 16, false, alloc);
    Int parsed = IntFromStrRadix(&hex, 16, alloc);

    bool ok = IntCompare(&value, &parsed) == 0;

    StrDeinit(&hex);
    IntDeinit(&value);
    IntDeinit(&parsed);

    ok = ok && DebugAllocatorLiveCount(&dbg) == 0;
    DebugAllocatorDeinit(&dbg);
    return ok;
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
        test_m10_mul_sparse_bits,
        test_m12_radix_underscore_only_rejected,
        test_m12_radix_valid_digit_parses,
        test_m12_radix_invalid_radix_rejected,
        test_m17_binary_prefix_lower,
        test_m17_binary_prefix_upper,
        test_m17_binary_no_prefix,
        test_m17_oct_prefix_lower,
        test_m17_oct_prefix_upper,
        test_m17_oct_no_prefix,
        test_m19_to_str_radix_roundtrip,
        test_m19_from_str_radix_mul_chain,
        test_m20_radix_digit_uppercase_bounds,
        test_m20_radix_digit_uppercase_value,
        test_m20_from_str_str_no_sign,
        test_m20_from_str_str_plus_sign,
        test_m21_radix_str_basic,
        test_m21_radix_str_plus_sign,
        test_m21_to_bytes_le_count,
        test_m21_to_bytes_le_single,
        test_m21_to_bytes_be_count,
        test_m21_to_bytes_be_single,
        test_m22_from_str_zstr_leading_plus,
        test_m22_from_str_zstr_no_sign,
        test_m23_radix_zstr_plus_sign_skipped,
        test_m23_oct_zstr_no_prefix_parses_all,
        test_m23_oct_zstr_leading_zero_not_prefix,
        test_m23_hash_uses_all_bytes,
        test_m24_from_bytes_be_null_zero,
        test_m24_from_bytes_be_roundtrip,
        test_m24_binary_reject_nonbinary_prefix,
        test_m24_binary_accept_real_prefix,
        test_m27_from_bytes_le_null_zero_no_abort,
        test_m27_from_bytes_le_trailing_zero_normalized,
        test_m28_from_str_str_parses,
        test_m28_from_str_radix_str_parses,
        test_m29_hex_str_str_return_valid_and_value,
        test_m29_hex_str_str_return_invalid,
        test_fe_603_to_bytes_be_content,
        test_to_str_radix_no_leak,
        test_from_bytes_be_no_leak,
        test_from_str_nonempty_out_no_leak,
        test_blind_to_str_radix_big,
    };

    // NULL-input tests: now expected to LOG_FATAL via the strict-
    // contract rule (programmer errors abort). The deadend driver
    // catches the abort and treats it as PASS.
    TestFunction deadend_tests[] = {
        test_int_from_binary_null,
        test_int_try_from_binary_null,
        test_int_from_decimal_null,
        test_int_try_from_decimal_null,
        test_int_from_radix_null,
        test_int_try_from_radix_null,
        test_int_from_octal_null,
        test_int_try_from_octal_null,
        test_int_from_hex_null,
        test_int_try_from_hex_null,
        test_int_from_bytes_le_null,
        test_int_to_bytes_le_null,
        test_int_to_bytes_be_zero_max_len,
    };

    int total_tests         = sizeof(tests) / sizeof(tests[0]);
    int total_deadend_tests = sizeof(deadend_tests) / sizeof(deadend_tests[0]);

    return run_test_suite(tests, total_tests, deadend_tests, total_deadend_tests, "Int.Convert");
}
