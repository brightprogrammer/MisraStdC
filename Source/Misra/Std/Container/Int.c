/// file      : std/container/int.c
/// author    : Generated following Misra project patterns
/// This is free and unencumbered software released into the public domain.
///
/// Arbitrary-precision unsigned integer implementation built on top of BitVec.

#include <Misra/Std/Container/Int.h>
#include <Misra/Std/Zstr.h>
#include <Misra/Std/Container/Int/Private.h>
#include <Misra/Std/Container/BitVec.h>
#include <Misra/Std/Log.h>

typedef struct {
    bool negative;
    Int  magnitude;
} SignedInt;

#define INT_BITS(value) (&(value)->bits)

static void int_normalize(Int *value);
static bool int_validate_radix(u8 radix);
static bool
    int_try_from_str_radix_impl(Int *out, const char *digits, u64 length, u64 start, u8 radix, bool allow_underscores);
static bool int_try_init_with_capacity(Int *out, u64 capacity, Allocator *alloc);
static bool int_try_from_u64_with_allocator(Int *out, u64 value, Allocator *alloc);
static bool int_try_from_i64_with_allocator(Int *out, i64 value, Allocator *alloc);
static bool int_try_clone_value(Int *out, Int *value);
static u64  int_u64_bits(u64 value);

static Int int_wrap(BitVec bits) {
    Int value;

    value.bits = bits;
    return value;
}

static bool int_try_init_with_capacity(Int *out, u64 capacity, Allocator *alloc) {
    if (!out) {
        LOG_FATAL("Invalid arguments");
    }

    *out = int_wrap(BitVecInitWithCapacity(capacity, alloc));
    if (capacity != 0 && out->bits.capacity < capacity) {
        IntDeinit(out);
        *out = IntInit(alloc);
        return false;
    }

    return true;
}

static bool int_try_from_u64_with_allocator(Int *out, u64 value, Allocator *alloc) {
    u64 bits = int_u64_bits(value);

    if (!out) {
        LOG_FATAL("Invalid arguments");
    }

    *out = IntInit(alloc);
    if (bits == 0) {
        return true;
    }

    if (!BitVecTryFromInteger(INT_BITS(out), value, bits, alloc)) {
        IntDeinit(out);
        *out = IntInit(alloc);
        return false;
    }

    return true;
}

static bool int_try_from_i64_with_allocator(Int *out, i64 value, Allocator *alloc) {
    if (value < 0) {
        LOG_ERROR("Int cannot represent negative values");
        return false;
    }

    return int_try_from_u64_with_allocator(out, (u64)value, alloc);
}

static u64 int_significant_bits(Int *value) {
    ValidateInt(value);

    for (u64 i = value->bits.length; i > 0; i--) {
        if (BitVecGet(INT_BITS(value), i - 1)) {
            return i;
        }
    }

    return 0;
}

static u64 int_u64_bits(u64 value) {
    u64 bits = 0;

    while (value != 0) {
        bits++;
        value >>= 1;
    }

    return bits;
}

static u64 int_i64_magnitude(i64 value) {
    if (value < 0) {
        return (u64)(-(value + 1)) + 1;
    }

    return (u64)value;
}

static void int_replace(Int *dst, Int *src) {
    IntDeinit(dst);
    *dst = *src;
}

static void int_swap(Int *a, Int *b) {
    Int tmp = *a;
    *a      = *b;
    *b      = tmp;
}

static SignedInt sint_init(Allocator *alloc) {
    SignedInt value = {.negative = false, .magnitude = IntInit(alloc)};
    return value;
}

static SignedInt sint_from_u64(u64 value, Allocator *alloc) {
    SignedInt result = {.negative = false, .magnitude = int_from_u64(value, alloc)};
    return result;
}

static void sint_deinit(SignedInt *value) {
    IntDeinit(&value->magnitude);
    value->negative = false;
}

static void sint_normalize(SignedInt *value) {
    int_normalize(&value->magnitude);
    if (IntIsZero(&value->magnitude)) {
        value->negative = false;
    }
}

static SignedInt sint_clone(SignedInt *value) {
    SignedInt clone = {.negative = value->negative, .magnitude = IntClone(&value->magnitude)};
    sint_normalize(&clone);
    return clone;
}

static void sint_replace(SignedInt *dst, SignedInt *src) {
    sint_deinit(dst);
    *dst = *src;
}

static bool sint_add(SignedInt *result, SignedInt *a, SignedInt *b) {
    SignedInt temp = sint_init(result->magnitude.bits.allocator);

    if (a->negative == b->negative) {
        if (!int_add(&temp.magnitude, &a->magnitude, &b->magnitude)) {
            sint_deinit(&temp);
            return false;
        }
        temp.negative = a->negative;
    } else {
        int cmp = int_compare(&a->magnitude, &b->magnitude);

        if (cmp == 0) {
            temp.negative = false;
        } else if (cmp > 0) {
            if (!int_sub(&temp.magnitude, &a->magnitude, &b->magnitude)) {
                sint_deinit(&temp);
                return false;
            }
            temp.negative = a->negative;
        } else {
            if (!int_sub(&temp.magnitude, &b->magnitude, &a->magnitude)) {
                sint_deinit(&temp);
                return false;
            }
            temp.negative = b->negative;
        }
    }

    sint_normalize(&temp);
    sint_replace(result, &temp);
    return true;
}

static bool sint_sub(SignedInt *result, SignedInt *a, SignedInt *b) {
    SignedInt neg_b = sint_clone(b);

    if (!IntIsZero(&neg_b.magnitude)) {
        neg_b.negative = !neg_b.negative;
    }

    if (!sint_add(result, a, &neg_b)) {
        sint_deinit(&neg_b);
        return false;
    }
    sint_deinit(&neg_b);
    return true;
}

static bool sint_mul_unsigned(SignedInt *result, SignedInt *a, Int *b) {
    SignedInt temp = sint_init(result->magnitude.bits.allocator);

    if (!int_mul(&temp.magnitude, &a->magnitude, b)) {
        sint_deinit(&temp);
        return false;
    }
    temp.negative = a->negative;
    sint_normalize(&temp);
    sint_replace(result, &temp);
    return true;
}

static void int_normalize(Int *value) {
    ValidateInt(value);
    BitVecResize(INT_BITS(value), int_significant_bits(value));
}

static bool int_is_odd(Int *value) {
    ValidateInt(value);
    return value->bits.length > 0 && BitVecGet(INT_BITS(value), 0);
}

static bool int_is_one(Int *value) {
    ValidateInt(value);
    return IntBitLength(value) == 1 && BitVecGet(INT_BITS(value), 0);
}

static bool int_mul_u64_in_place(Int *value, u64 factor) {
    Int lhs;
    Int rhs;
    Int result = IntInit(value->bits.allocator);

    if (!int_try_clone_value(&lhs, value)) {
        return false;
    }
    if (!int_try_from_u64_with_allocator(&rhs, factor, value->bits.allocator)) {
        IntDeinit(&lhs);
        return false;
    }
    if (!int_mul(&result, &lhs, &rhs)) {
        IntDeinit(&lhs);
        IntDeinit(&rhs);
        IntDeinit(&result);
        return false;
    }

    IntDeinit(&lhs);
    IntDeinit(&rhs);
    int_replace(value, &result);
    return true;
}

static bool int_add_u64_in_place(Int *value, u64 addend) {
    Int lhs;
    Int rhs;
    Int result = IntInit(value->bits.allocator);

    if (!int_try_clone_value(&lhs, value)) {
        return false;
    }
    if (!int_try_from_u64_with_allocator(&rhs, addend, value->bits.allocator)) {
        IntDeinit(&lhs);
        return false;
    }
    if (!int_add(&result, &lhs, &rhs)) {
        IntDeinit(&lhs);
        IntDeinit(&rhs);
        IntDeinit(&result);
        return false;
    }

    IntDeinit(&lhs);
    IntDeinit(&rhs);
    int_replace(value, &result);
    return true;
}

static bool int_validate_radix(u8 radix) {
    if (radix < 2 || radix > 36) {
        LOG_ERROR("radix must be between 2 and 36");
        return false;
    }

    return true;
}

static int int_radix_digit(char ch) {
    if (ch >= '0' && ch <= '9') {
        return ch - '0';
    }
    if (ch >= 'a' && ch <= 'z') {
        return 10 + (ch - 'a');
    }
    if (ch >= 'A' && ch <= 'Z') {
        return 10 + (ch - 'A');
    }

    return -1;
}

static char int_radix_char(u8 digit, bool uppercase) {
    if (digit < 10) {
        return (char)('0' + digit);
    }
    if (uppercase) {
        return (char)('A' + (digit - 10));
    }

    return (char)('a' + (digit - 10));
}

static bool
    int_try_from_str_radix_impl(Int *out, const char *digits, u64 length, u64 start, u8 radix, bool allow_underscores) {
    Int  result;
    bool saw_digit = false;

    if (!out || !digits) {
        LOG_FATAL("Invalid arguments");
    }

    ValidateInt(out);
    result = IntInit(out->bits.allocator);

    if (!int_validate_radix(radix)) {
        return false;
    }

    for (u64 i = start; i < length; i++) {
        int digit = 0;

        if (allow_underscores && digits[i] == '_') {
            continue;
        }

        digit = int_radix_digit(digits[i]);
        if (digit < 0 || digit >= radix) {
            LOG_ERROR("Invalid digit for radix in Int conversion");
            IntDeinit(&result);
            return false;
        }

        saw_digit = true;
        if (!int_mul_u64_in_place(&result, radix) || !int_add_u64_in_place(&result, (u64)digit)) {
            IntDeinit(&result);
            return false;
        }
    }

    if (!saw_digit) {
        LOG_ERROR("No valid digits found");
        IntDeinit(&result);
        return false;
    }

    int_normalize(&result);
    IntDeinit(out);
    *out = result;
    return true;
}

u64 IntBitLength(Int *value) {
    return int_significant_bits(value);
}

u64 IntByteLength(Int *value) {
    u64 bits = IntBitLength(value);
    return bits == 0 ? 0 : CEIL_DIV(bits, 8u);
}

bool IntTryLog2(Int *value, u64 *out) {
    if (!value || !out) {
        LOG_FATAL("Invalid arguments");
    }

    ValidateInt(value);

    if (IntIsZero(value)) {
        LOG_ERROR("log2 undefined for zero");
        return false;
    }

    *out = IntBitLength(value) - 1;
    return true;
}

u64 IntLog2WithError(Int *value, bool *error) {
    u64  out = 0;
    bool ok  = IntTryLog2(value, &out);

    if (error) {
        *error = !ok;
    }

    return out;
}

u64 IntTrailingZeroCount(Int *value) {
    ValidateInt(value);

    for (u64 i = 0; i < value->bits.length; i++) {
        if (BitVecGet(INT_BITS(value), i)) {
            return i;
        }
    }

    return 0;
}

bool IntIsZero(Int *value) {
    return IntBitLength(value) == 0;
}

bool IntIsOne(Int *value) {
    return int_is_one(value);
}

bool IntIsEven(Int *value) {
    ValidateInt(value);
    return !int_is_odd(value);
}

bool IntIsOdd(Int *value) {
    return int_is_odd(value);
}

bool IntFitsU64(Int *value) {
    ValidateInt(value);
    return IntBitLength(value) <= 64;
}

bool IntIsPowerOfTwo(Int *value) {
    ValidateInt(value);

    return !IntIsZero(value) && IntBitLength(value) == IntTrailingZeroCount(value) + 1;
}

static bool int_try_clone_value(Int *out, Int *value) {
    if (!out || !value) {
        LOG_FATAL("Invalid arguments");
    }

    ValidateInt(value);
    *out = IntInit(value->bits.allocator);
    if (!BitVecTryClone(INT_BITS(out), INT_BITS(value))) {
        return false;
    }

    int_normalize(out);
    return true;
}

bool IntTryClone(Int *out, Int *value) {
    return int_try_clone_value(out, value);
}

Int IntClone(Int *value) {
    Int clone;

    ValidateInt(value);
    clone = IntInit(value->bits.allocator);
    (void)int_try_clone_value(&clone, value);
    return clone;
}

Int int_from_u64(u64 value, Allocator *alloc) {
    Int result = IntInit(alloc);

    (void)int_try_from_u64_with_allocator(&result, value, alloc);
    return result;
}

Int int_from_i64(i64 value, Allocator *alloc) {
    if (value < 0) {
        LOG_FATAL("Int cannot represent negative values");
    }

    return int_from_u64((u64)value, alloc);
}

bool IntTryToU64(Int *value, u64 *out) {
    if (!value || !out) {
        LOG_FATAL("Invalid arguments");
    }

    ValidateInt(value);

    if (!IntFitsU64(value)) {
        LOG_ERROR("Int value exceeds u64 range");
        return false;
    }

    *out = BitVecToInteger(INT_BITS(value));
    return true;
}

u64 IntToU64WithError(Int *value, bool *error) {
    u64  out = 0;
    bool ok  = IntTryToU64(value, &out);

    if (error) {
        *error = !ok;
    }

    return out;
}

Int int_from_bytes_le(const u8 *bytes, u64 len, Allocator *alloc) {
    if (!bytes && len != 0) {
        LOG_FATAL("bytes is NULL");
    }

    Int result = IntInit(alloc);

    if (len == 0) {
        return result;
    }

    if (!BitVecTryFromBytes(INT_BITS(&result), bytes, len * 8, alloc)) {
        return result;
    }

    int_normalize(&result);
    return result;
}

u64 IntToBytesLE(Int *value, u8 *bytes, u64 max_len) {
    ValidateInt(value);

    if (!bytes) {
        LOG_FATAL("bytes is NULL");
    }
    if (max_len == 0) {
        LOG_FATAL("max_len is 0");
    }

    u64 bytes_needed  = IntByteLength(value);
    u64 bytes_to_copy = MIN2(bytes_needed, max_len);

    if (bytes_to_copy == 0) {
        return 0;
    }

    MemSet(bytes, 0, bytes_to_copy);

    for (u64 i = 0; i < bytes_to_copy; i++) {
        u8 byte = 0;

        for (u64 bit = 0; bit < 8; bit++) {
            u64 bit_idx = i * 8 + bit;

            if (bit_idx < value->bits.length && BitVecGet(INT_BITS(value), bit_idx)) {
                byte |= (u8)(1u << bit);
            }
        }

        bytes[i] = byte;
    }

    return bytes_to_copy;
}

Int int_from_bytes_be(const u8 *bytes, u64 len, Allocator *alloc) {
    if (!bytes && len != 0) {
        LOG_FATAL("bytes is NULL");
    }

    Int result = IntInit(alloc);

    for (u64 i = 0; i < len; i++) {
        if (!IntShiftLeft(&result, 8) || !int_add_u64_in_place(&result, bytes[i])) {
            IntDeinit(&result);
            return IntInit(alloc);
        }
    }

    int_normalize(&result);
    return result;
}

u64 IntToBytesBE(Int *value, u8 *bytes, u64 max_len) {
    ValidateInt(value);

    if (!bytes) {
        LOG_FATAL("bytes is NULL");
    }
    if (max_len == 0) {
        LOG_FATAL("max_len is 0");
    }

    u64 bytes_needed  = IntByteLength(value);
    u64 bytes_to_copy = MIN2(bytes_needed, max_len);

    if (bytes_to_copy == 0) {
        return 0;
    }

    MemSet(bytes, 0, bytes_to_copy);

    for (u64 i = 0; i < bytes_to_copy; i++) {
        u8 byte = 0;

        for (u64 bit = 0; bit < 8; bit++) {
            u64 bit_idx = i * 8 + bit;

            if (bit_idx < value->bits.length && BitVecGet(INT_BITS(value), bit_idx)) {
                byte |= (u8)(1u << bit);
            }
        }

        bytes[bytes_to_copy - 1 - i] = byte;
    }

    return bytes_to_copy;
}

bool int_try_from_str_zstr(Int *out, Zstr decimal) {
    u64 start = 0;
    u64 len   = 0;

    if (!out || !decimal) {
        LOG_FATAL("Invalid arguments");
    }

    len = (u64)ZstrLen(decimal);
    if (len > 0 && decimal[0] == '+') {
        start = 1;
    }

    return int_try_from_str_radix_impl(out, decimal, len, start, 10, true);
}

bool int_try_from_str_str(Int *out, const Str *decimal) {
    u64 start = 0;

    if (!out || !decimal) {
        LOG_FATAL("Invalid arguments");
    }

    if (decimal->length > 0 && decimal->data[0] == '+') {
        start = 1;
    }

    return int_try_from_str_radix_impl(out, decimal->data, decimal->length, start, 10, true);
}

Int int_from_str_zstr(Zstr decimal, Allocator *alloc) {
    Int out = IntInit(alloc);

    (void)int_try_from_str_zstr(&out, decimal);
    return out;
}

Int int_from_str_str(const Str *decimal, Allocator *alloc) {
    Int out = IntInit(alloc);

    (void)int_try_from_str_str(&out, decimal);
    return out;
}

bool int_try_to_str(Str *out, Int *value, Allocator *alloc) {
    return int_try_to_str_radix(out, value, 10, false, alloc);
}

Str int_to_str(Int *value, Allocator *alloc) {
    Str result;

    ValidateInt(value);

    if (!int_try_to_str(&result, value, alloc)) {
        result = StrInit(alloc);
    }

    return result;
}

bool int_try_from_str_radix_zstr(Int *out, Zstr digits, u8 radix) {
    u64 start = 0;
    u64 len   = 0;

    if (!out || !digits) {
        LOG_FATAL("Invalid arguments");
    }
    len = (u64)ZstrLen(digits);
    if (len > 0 && digits[0] == '+') {
        start = 1;
    }

    return int_try_from_str_radix_impl(out, digits, len, start, radix, true);
}

bool int_try_from_str_radix_str(Int *out, const Str *digits, u8 radix) {
    u64 start = 0;

    if (!out || !digits) {
        LOG_FATAL("Invalid arguments");
    }
    if (digits->length > 0 && digits->data[0] == '+') {
        start = 1;
    }

    return int_try_from_str_radix_impl(out, digits->data, digits->length, start, radix, true);
}

Int int_from_str_radix_zstr(Zstr digits, u8 radix, Allocator *alloc) {
    Int out = IntInit(alloc);

    (void)int_try_from_str_radix_zstr(&out, digits, radix);
    return out;
}

Int int_from_str_radix_str(const Str *digits, u8 radix, Allocator *alloc) {
    Int out = IntInit(alloc);

    (void)int_try_from_str_radix_str(&out, digits, radix);
    return out;
}

bool int_try_to_str_radix(Str *out, Int *value, u8 radix, bool uppercase, Allocator *alloc) {
    Int current;
    Str result;

    ValidateInt(value);
    if (!out) {
        LOG_FATAL("Invalid arguments");
    }

    *out = StrInit(alloc);

    if (!int_validate_radix(radix)) {
        return false;
    }

    if (IntIsZero(value)) {
        return StrPushBack(out, '0');
    }

    current = IntClone(value);
    if (IntIsZero(&current)) {
        return false;
    }

    result = StrInit(alloc);

    while (!IntIsZero(&current)) {
        Int quotient = IntInit(alloc);
        u64 digit    = 0;

        digit = int_div_u64_rem(&quotient, &current, radix);
        if (!StrPushBack(&result, int_radix_char((u8)digit, uppercase))) {
            IntDeinit(&quotient);
            IntDeinit(&current);
            StrDeinit(&result);
            return false;
        }

        IntDeinit(&current);
        current = quotient;
    }

    for (u64 i = 0; i < result.length / 2; i++) {
        char tmp                           = result.data[i];
        result.data[i]                     = result.data[result.length - 1 - i];
        result.data[result.length - 1 - i] = tmp;
    }

    IntDeinit(&current);
    *out = result;
    return true;
}

Str int_to_str_radix(Int *value, u8 radix, bool uppercase, Allocator *alloc) {
    Str result;

    ValidateInt(value);

    if (!int_try_to_str_radix(&result, value, radix, uppercase, alloc)) {
        result = StrInit(alloc);
    }

    return result;
}

bool int_try_from_binary_zstr(Int *out, Zstr binary) {
    u64 start = 0;
    u64 len   = 0;

    if (!out || !binary) {
        LOG_FATAL("Invalid arguments");
    }

    len = (u64)ZstrLen(binary);
    if (len >= 2 && binary[0] == '0' && (binary[1] == 'b' || binary[1] == 'B')) {
        start = 2;
    }

    return int_try_from_str_radix_impl(out, binary, len, start, 2, true);
}

bool int_try_from_binary_str(Int *out, const Str *binary) {
    u64 start = 0;

    if (!out || !binary) {
        LOG_FATAL("Invalid arguments");
    }

    if (binary->length >= 2 && binary->data[0] == '0' && (binary->data[1] == 'b' || binary->data[1] == 'B')) {
        start = 2;
    }

    return int_try_from_str_radix_impl(out, binary->data, binary->length, start, 2, true);
}

Int int_from_binary_zstr(Zstr binary, Allocator *alloc) {
    Int out = IntInit(alloc);

    (void)int_try_from_binary_zstr(&out, binary);
    return out;
}

Int int_from_binary_str(const Str *binary, Allocator *alloc) {
    Int out = IntInit(alloc);

    (void)int_try_from_binary_str(&out, binary);
    return out;
}

Str IntToBinary(Int *value) {
    return IntToStrRadix(value, 2, false);
}

bool int_try_from_oct_str_zstr(Int *out, Zstr octal) {
    u64 start = 0;
    u64 len   = 0;

    if (!out || !octal) {
        LOG_FATAL("Invalid arguments");
    }

    len = (u64)ZstrLen(octal);
    if (len >= 2 && octal[0] == '0' && (octal[1] == 'o' || octal[1] == 'O')) {
        start = 2;
    }

    return int_try_from_str_radix_impl(out, octal, len, start, 8, true);
}

bool int_try_from_oct_str_str(Int *out, const Str *octal) {
    u64 start = 0;

    if (!out || !octal) {
        LOG_FATAL("Invalid arguments");
    }

    if (octal->length >= 2 && octal->data[0] == '0' && (octal->data[1] == 'o' || octal->data[1] == 'O')) {
        start = 2;
    }

    return int_try_from_str_radix_impl(out, octal->data, octal->length, start, 8, true);
}

Int int_from_oct_str_zstr(Zstr octal, Allocator *alloc) {
    Int out = IntInit(alloc);

    (void)int_try_from_oct_str_zstr(&out, octal);
    return out;
}

Int int_from_oct_str_str(const Str *octal, Allocator *alloc) {
    Int out = IntInit(alloc);

    (void)int_try_from_oct_str_str(&out, octal);
    return out;
}

Str IntToOctStr(Int *value) {
    return IntToStrRadix(value, 8, false);
}

bool int_try_from_hex_str_zstr(Int *out, Zstr hex) {
    u64 len = 0;

    if (!out || !hex) {
        LOG_FATAL("Invalid arguments");
    }

    len = (u64)ZstrLen(hex);
    return int_try_from_str_radix_impl(out, hex, len, 0, 16, false);
}

bool int_try_from_hex_str_str(Int *out, const Str *hex) {
    if (!out || !hex) {
        LOG_FATAL("Invalid arguments");
    }

    return int_try_from_str_radix_impl(out, hex->data, hex->length, 0, 16, false);
}

Int int_from_hex_str_zstr(Zstr hex, Allocator *alloc) {
    Int out = IntInit(alloc);

    (void)int_try_from_hex_str_zstr(&out, hex);
    return out;
}

Int int_from_hex_str_str(const Str *hex, Allocator *alloc) {
    Int out = IntInit(alloc);

    (void)int_try_from_hex_str_str(&out, hex);
    return out;
}

Str IntToHexStr(Int *value) {
    return IntToStrRadix(value, 16, false);
}

int int_compare(Int *lhs, Int *rhs) {
    ValidateInt(lhs);
    ValidateInt(rhs);

    u64 lhs_bits = IntBitLength(lhs);
    u64 rhs_bits = IntBitLength(rhs);

    if (lhs_bits < rhs_bits) {
        return -1;
    }
    if (lhs_bits > rhs_bits) {
        return 1;
    }

    for (u64 i = lhs_bits; i > 0; i--) {
        bool lhs_bit = BitVecGet(INT_BITS(lhs), i - 1);
        bool rhs_bit = BitVecGet(INT_BITS(rhs), i - 1);

        if (lhs_bit != rhs_bit) {
            return lhs_bit ? 1 : -1;
        }
    }

    return 0;
}

int int_compare_u64(Int *lhs, u64 rhs) {
    ValidateInt(lhs);

    if (IntBitLength(lhs) > 64) {
        return 1;
    }

    u64 lhs_value = 0;

    (void)IntTryToU64(lhs, &lhs_value);

    if (lhs_value < rhs) {
        return -1;
    }
    if (lhs_value > rhs) {
        return 1;
    }

    return 0;
}

int int_compare_i64(Int *lhs, i64 rhs) {
    ValidateInt(lhs);

    if (rhs < 0) {
        return 1;
    }

    return int_compare_u64(lhs, (u64)rhs);
}

bool IntShiftLeft(Int *value, u64 positions) {
    ValidateInt(value);

    u64 bits = IntBitLength(value);

    if (positions == 0) {
        return BitVecResize(INT_BITS(value), bits);
    }
    if (bits == 0) {
        IntClear(value);
        return true;
    }

    if (!BitVecResize(INT_BITS(value), bits + positions)) {
        return false;
    }

    for (u64 i = bits; i > 0; i--) {
        BitVecSet(INT_BITS(value), i - 1 + positions, BitVecGet(INT_BITS(value), i - 1));
    }

    for (u64 i = 0; i < positions; i++) {
        BitVecSet(INT_BITS(value), i, false);
    }

    return true;
}

bool IntShiftRight(Int *value, u64 positions) {
    ValidateInt(value);

    u64 bits = IntBitLength(value);

    if (positions == 0) {
        return BitVecResize(INT_BITS(value), bits);
    }
    if (bits == 0 || positions >= bits) {
        IntClear(value);
        return true;
    }

    for (u64 i = 0; i + positions < bits; i++) {
        BitVecSet(INT_BITS(value), i, BitVecGet(INT_BITS(value), i + positions));
    }

    if (!BitVecResize(INT_BITS(value), bits - positions)) {
        return false;
    }
    int_normalize(value);
    return true;
}

bool int_add(Int *result, Int *a, Int *b) {
    ValidateInt(result);
    ValidateInt(a);
    ValidateInt(b);

    u64  a_bits   = IntBitLength(a);
    u64  b_bits   = IntBitLength(b);
    u64  max_bits = MAX2(a_bits, b_bits);
    Int  temp;
    bool carry = false;

    if (!int_try_init_with_capacity(&temp, max_bits + 1, result->bits.allocator)) {
        return false;
    }

    for (u64 i = 0; i < max_bits; i++) {
        u64 sum = carry ? 1u : 0u;

        if (i < a_bits && BitVecGet(INT_BITS(a), i)) {
            sum++;
        }
        if (i < b_bits && BitVecGet(INT_BITS(b), i)) {
            sum++;
        }

        if (!BitVecPush(INT_BITS(&temp), (sum & 1u) != 0u)) {
            IntDeinit(&temp);
            return false;
        }
        carry = sum >= 2u;
    }

    if (carry) {
        if (!BitVecPush(INT_BITS(&temp), true)) {
            IntDeinit(&temp);
            return false;
        }
    }

    int_normalize(&temp);
    IntDeinit(result);
    *result = temp;
    return true;
}

bool int_add_u64(Int *result, Int *value, u64 addend) {
    ValidateInt(result);
    ValidateInt(value);

    Int temp;

    if (!int_try_clone_value(&temp, value)) {
        return false;
    }
    if (!int_add_u64_in_place(&temp, addend)) {
        IntDeinit(&temp);
        return false;
    }
    int_replace(result, &temp);
    return true;
}

bool int_add_i64(Int *result, Int *value, i64 addend) {
    u64 magnitude = int_i64_magnitude(addend);

    ValidateInt(result);
    ValidateInt(value);

    if (addend >= 0) {
        return int_add_u64(result, value, magnitude);
    }

    if (!int_sub_u64(result, value, magnitude)) {
        LOG_FATAL("int_add would produce a negative result");
    }

    return true;
}

bool int_sub(Int *result, Int *a, Int *b) {
    ValidateInt(result);
    ValidateInt(a);
    ValidateInt(b);

    if (int_compare(a, b) < 0) {
        return false;
    }

    u64  a_bits = IntBitLength(a);
    u64  b_bits = IntBitLength(b);
    Int  temp;
    bool borrow = false;

    if (!int_try_init_with_capacity(&temp, a_bits, result->bits.allocator)) {
        return false;
    }

    for (u64 i = 0; i < a_bits; i++) {
        int ai   = (i < a_bits && BitVecGet(INT_BITS(a), i)) ? 1 : 0;
        int bi   = (i < b_bits && BitVecGet(INT_BITS(b), i)) ? 1 : 0;
        int diff = ai - bi - (borrow ? 1 : 0);

        if (diff < 0) {
            diff   += 2;
            borrow  = true;
        } else {
            borrow = false;
        }

        if (!BitVecPush(INT_BITS(&temp), diff != 0)) {
            IntDeinit(&temp);
            return false;
        }
    }

    int_normalize(&temp);
    IntDeinit(result);
    *result = temp;
    return true;
}

bool int_sub_u64(Int *result, Int *value, u64 subtrahend) {
    ValidateInt(result);
    ValidateInt(value);

    Int  rhs;
    bool ok = false;

    if (!int_try_from_u64_with_allocator(&rhs, subtrahend, value->bits.allocator)) {
        return false;
    }
    ok = int_sub(result, value, &rhs);
    IntDeinit(&rhs);
    return ok;
}

bool int_sub_i64(Int *result, Int *value, i64 subtrahend) {
    u64 magnitude = int_i64_magnitude(subtrahend);

    ValidateInt(result);
    ValidateInt(value);

    if (subtrahend >= 0) {
        return int_sub_u64(result, value, magnitude);
    }

    return int_add_u64(result, value, magnitude);
}

bool int_mul(Int *result, Int *a, Int *b) {
    ValidateInt(result);
    ValidateInt(a);
    ValidateInt(b);

    u64 b_bits = IntBitLength(b);
    Int acc    = IntInit(result->bits.allocator);

    if (IntIsZero(a) || IntIsZero(b)) {
        IntDeinit(result);
        *result = acc;
        return true;
    }

    for (u64 i = 0; i < b_bits; i++) {
        if (!BitVecGet(INT_BITS(b), i)) {
            continue;
        }

        Int partial;
        Int next = IntInit(result->bits.allocator);

        if (!int_try_clone_value(&partial, a)) {
            IntDeinit(&acc);
            return false;
        }
        int_normalize(&partial);
        if (!IntShiftLeft(&partial, i) || !int_add(&next, &acc, &partial)) {
            IntDeinit(&acc);
            IntDeinit(&partial);
            IntDeinit(&next);
            return false;
        }

        IntDeinit(&acc);
        acc = next;

        IntDeinit(&partial);
    }

    int_normalize(&acc);
    IntDeinit(result);
    *result = acc;
    return true;
}

bool int_mul_u64(Int *result, Int *value, u64 factor) {
    ValidateInt(result);
    ValidateInt(value);

    Int temp;

    if (!int_try_clone_value(&temp, value)) {
        return false;
    }
    if (!int_mul_u64_in_place(&temp, factor)) {
        IntDeinit(&temp);
        return false;
    }
    int_replace(result, &temp);
    return true;
}

bool int_mul_i64(Int *result, Int *value, i64 factor) {
    if (factor < 0) {
        LOG_FATAL("Int cannot be multiplied by a negative scalar");
    }

    return int_mul_u64(result, value, (u64)factor);
}

bool IntSquare(Int *result, Int *value) {
    return int_mul(result, value, value);
}

bool int_pow(Int *result, Int *base, Int *exponent) {
    ValidateInt(result);
    ValidateInt(base);
    ValidateInt(exponent);

    if (!IntFitsU64(exponent)) {
        LOG_ERROR("Int exponent exceeds u64 range");
        return false;
    }

    return int_pow_u64(result, base, IntToU64(exponent));
}

bool int_pow_u64(Int *result, Int *base, u64 exponent) {
    ValidateInt(result);
    ValidateInt(base);

    Int acc;
    Int current;

    if (!int_try_from_u64_with_allocator(&acc, 1, result->bits.allocator)) {
        return false;
    }
    if (!int_try_clone_value(&current, base)) {
        IntDeinit(&acc);
        return false;
    }

    while (exponent > 0) {
        if (exponent & 1u) {
            Int next = IntInit(result->bits.allocator);

            if (!int_mul(&next, &acc, &current)) {
                IntDeinit(&acc);
                IntDeinit(&current);
                IntDeinit(&next);
                return false;
            }
            IntDeinit(&acc);
            acc = next;
        }

        exponent >>= 1u;
        if (exponent > 0) {
            Int next = IntInit(result->bits.allocator);

            if (!IntSquare(&next, &current)) {
                IntDeinit(&acc);
                IntDeinit(&current);
                IntDeinit(&next);
                return false;
            }
            IntDeinit(&current);
            current = next;
        }
    }

    IntDeinit(&current);
    int_replace(result, &acc);
    return true;
}

bool int_pow_i64(Int *result, Int *base, i64 exponent) {
    if (exponent < 0) {
        LOG_FATAL("Int exponent cannot be negative");
    }

    return int_pow_u64(result, base, (u64)exponent);
}

bool int_div_mod(Int *quotient, Int *remainder, Int *dividend, Int *divisor) {
    ValidateInt(quotient);
    ValidateInt(remainder);
    ValidateInt(dividend);
    ValidateInt(divisor);

    if (quotient == remainder) {
        LOG_FATAL("quotient and remainder must be different objects");
    }
    if (IntIsZero(divisor)) {
        LOG_ERROR("Division by zero");
        return false;
    }

    Int  normalized_dividend = IntInit(quotient->bits.allocator);
    Int  normalized_divisor  = IntInit(quotient->bits.allocator);
    Int  q                   = IntInit(quotient->bits.allocator);
    Int  r                   = IntInit(remainder->bits.allocator);
    bool ok                  = false;

    if (!int_try_clone_value(&normalized_dividend, dividend) || !int_try_clone_value(&normalized_divisor, divisor) ||
        !int_try_clone_value(&r, &normalized_dividend)) {
        goto cleanup;
    }

    if (int_compare(&normalized_dividend, &normalized_divisor) >= 0) {
        u64 dividend_bits = IntBitLength(&normalized_dividend);
        u64 divisor_bits  = IntBitLength(&normalized_divisor);

        IntDeinit(&q);
        if (!int_try_init_with_capacity(&q, dividend_bits - divisor_bits + 1, quotient->bits.allocator)) {
            goto cleanup;
        }

        for (u64 shift = dividend_bits - divisor_bits + 1; shift > 0; shift--) {
            u64 bit     = shift - 1;
            Int shifted = IntInit(quotient->bits.allocator);

            if (!int_try_clone_value(&shifted, &normalized_divisor) || !IntShiftLeft(&shifted, bit)) {
                IntDeinit(&shifted);
                goto cleanup;
            }

            if (IntGE(&r, &shifted)) {
                Int next = IntInit(quotient->bits.allocator);

                if (!int_sub(&next, &r, &shifted)) {
                    IntDeinit(&shifted);
                    IntDeinit(&next);
                    goto cleanup;
                }
                IntDeinit(&r);
                r = next;

                if (IntBitLength(&q) < bit + 1 && !BitVecResize(INT_BITS(&q), bit + 1)) {
                    IntDeinit(&shifted);
                    goto cleanup;
                }
                BitVecSet(INT_BITS(&q), bit, true);
            }

            IntDeinit(&shifted);
        }
    } else {
        IntDeinit(&q);
        q = IntInit(quotient->bits.allocator);
    }

    int_normalize(&q);
    int_normalize(&r);

    IntDeinit(&normalized_dividend);
    IntDeinit(&normalized_divisor);
    int_replace(quotient, &q);
    int_replace(remainder, &r);
    ok = true;
cleanup:
    if (!ok) {
        IntDeinit(&q);
        IntDeinit(&r);
        IntDeinit(&normalized_dividend);
        IntDeinit(&normalized_divisor);
    }
    return ok;
}

bool int_div(Int *result, Int *dividend, Int *divisor) {
    Int quotient  = IntInit(result->bits.allocator);
    Int remainder = IntInit(result->bits.allocator);

    if (!int_div_mod(&quotient, &remainder, dividend, divisor)) {
        IntDeinit(&quotient);
        IntDeinit(&remainder);
        return false;
    }

    IntDeinit(&remainder);
    int_replace(result, &quotient);
    return true;
}

bool int_div_exact(Int *result, Int *dividend, Int *divisor) {
    ValidateInt(result);
    ValidateInt(dividend);
    ValidateInt(divisor);

    if (IntIsZero(divisor)) {
        LOG_ERROR("Division by zero");
        return false;
    }

    Int quotient  = IntInit(result->bits.allocator);
    Int remainder = IntInit(result->bits.allocator);

    if (!int_div_mod(&quotient, &remainder, dividend, divisor)) {
        IntDeinit(&quotient);
        IntDeinit(&remainder);
        return false;
    }
    if (!IntIsZero(&remainder)) {
        IntDeinit(&quotient);
        IntDeinit(&remainder);
        return false;
    }

    IntDeinit(&remainder);
    int_replace(result, &quotient);
    return true;
}

bool int_div_u64(Int *result, Int *dividend, u64 divisor) {
    Int divisor_value = IntInit(dividend->bits.allocator);

    if (!int_try_from_u64_with_allocator(&divisor_value, divisor, dividend->bits.allocator)) {
        IntDeinit(&divisor_value);
        return false;
    }

    bool ok = int_div(result, dividend, &divisor_value);
    IntDeinit(&divisor_value);
    return ok;
}

bool int_div_i64(Int *result, Int *dividend, i64 divisor) {
    Int divisor_value = IntInit(dividend->bits.allocator);

    if (!int_try_from_i64_with_allocator(&divisor_value, divisor, dividend->bits.allocator)) {
        IntDeinit(&divisor_value);
        return false;
    }

    bool ok = int_div(result, dividend, &divisor_value);
    IntDeinit(&divisor_value);
    return ok;
}

bool int_div_exact_u64(Int *result, Int *dividend, u64 divisor) {
    Int divisor_value = IntInit(dividend->bits.allocator);

    if (!int_try_from_u64_with_allocator(&divisor_value, divisor, dividend->bits.allocator)) {
        IntDeinit(&divisor_value);
        return false;
    }

    bool ok = int_div_exact(result, dividend, &divisor_value);
    IntDeinit(&divisor_value);
    return ok;
}

bool int_div_exact_i64(Int *result, Int *dividend, i64 divisor) {
    Int divisor_value = IntInit(dividend->bits.allocator);

    if (!int_try_from_i64_with_allocator(&divisor_value, divisor, dividend->bits.allocator)) {
        IntDeinit(&divisor_value);
        return false;
    }

    bool ok = int_div_exact(result, dividend, &divisor_value);
    IntDeinit(&divisor_value);
    return ok;
}

bool int_div_mod_u64(Int *quotient, Int *remainder, Int *dividend, u64 divisor) {
    Int divisor_value = IntInit(dividend->bits.allocator);

    if (!int_try_from_u64_with_allocator(&divisor_value, divisor, dividend->bits.allocator)) {
        IntDeinit(&divisor_value);
        return false;
    }

    bool ok = int_div_mod(quotient, remainder, dividend, &divisor_value);
    IntDeinit(&divisor_value);
    return ok;
}

bool int_div_mod_i64(Int *quotient, Int *remainder, Int *dividend, i64 divisor) {
    Int divisor_value = IntInit(dividend->bits.allocator);

    if (!int_try_from_i64_with_allocator(&divisor_value, divisor, dividend->bits.allocator)) {
        IntDeinit(&divisor_value);
        return false;
    }

    bool ok = int_div_mod(quotient, remainder, dividend, &divisor_value);
    IntDeinit(&divisor_value);
    return ok;
}

u64 int_div_u64_rem(Int *quotient, Int *dividend, u64 divisor) {
    ValidateInt(quotient);
    ValidateInt(dividend);

    if (divisor == 0) {
        LOG_ERROR("Division by zero");
        return 0;
    }

    Int divisor_value = IntInit(dividend->bits.allocator);
    Int remainder     = IntInit(quotient->bits.allocator);
    u64 rem           = 0;

    if (!int_try_from_u64_with_allocator(&divisor_value, divisor, dividend->bits.allocator)) {
        IntDeinit(&divisor_value);
        IntDeinit(&remainder);
        return 0;
    }

    if (!int_div_mod(quotient, &remainder, dividend, &divisor_value)) {
        IntDeinit(&divisor_value);
        IntDeinit(&remainder);
        return 0;
    }
    rem = IntToU64(&remainder);

    IntDeinit(&divisor_value);
    IntDeinit(&remainder);
    return rem;
}

bool int_mod(Int *result, Int *dividend, Int *divisor) {
    Int quotient  = IntInit(result->bits.allocator);
    Int remainder = IntInit(result->bits.allocator);

    if (!int_div_mod(&quotient, &remainder, dividend, divisor)) {
        IntDeinit(&quotient);
        IntDeinit(&remainder);
        return false;
    }

    IntDeinit(&quotient);
    int_replace(result, &remainder);
    return true;
}

bool int_mod_u64_into(Int *result, Int *dividend, u64 divisor) {
    Int quotient = IntInit(result->bits.allocator);

    bool ok = int_div_mod_u64(&quotient, result, dividend, divisor);
    IntDeinit(&quotient);
    return ok;
}

bool int_mod_i64_into(Int *result, Int *dividend, i64 divisor) {
    Int quotient = IntInit(result->bits.allocator);

    bool ok = int_div_mod_i64(&quotient, result, dividend, divisor);
    IntDeinit(&quotient);
    return ok;
}

u64 int_mod_u64(Int *value, u64 modulus) {
    ValidateInt(value);

    if (modulus == 0) {
        LOG_ERROR("modulus is zero");
        return 0;
    }

    Int quotient = IntInit(value->bits.allocator);
    u64 rem      = int_div_u64_rem(&quotient, value, modulus);

    IntDeinit(&quotient);
    return rem;
}

bool IntGCD(Int *result, Int *a, Int *b) {
    ValidateInt(result);
    ValidateInt(a);
    ValidateInt(b);

    Int x = IntInit(a->bits.allocator);
    Int y = IntInit(b->bits.allocator);

    if (!int_try_clone_value(&x, a) || !int_try_clone_value(&y, b)) {
        IntDeinit(&x);
        IntDeinit(&y);
        return false;
    }

    while (!IntIsZero(&y)) {
        Int r = IntInit(result->bits.allocator);

        if (!int_mod(&r, &x, &y)) {
            IntDeinit(&x);
            IntDeinit(&y);
            IntDeinit(&r);
            return false;
        }
        IntDeinit(&x);
        x = y;
        y = r;
    }

    int_replace(result, &x);
    IntDeinit(&y);
    return true;
}

bool IntLCM(Int *result, Int *a, Int *b) {
    ValidateInt(result);
    ValidateInt(a);
    ValidateInt(b);

    if (IntIsZero(a) || IntIsZero(b)) {
        Int zero = IntInit(result->bits.allocator);
        int_replace(result, &zero);
        return true;
    }

    Int gcd      = IntInit(result->bits.allocator);
    Int quotient = IntInit(result->bits.allocator);
    Int lcm      = IntInit(result->bits.allocator);

    if (!IntGCD(&gcd, a, b) || !int_div(&quotient, a, &gcd) || !int_mul(&lcm, &quotient, b)) {
        IntDeinit(&gcd);
        IntDeinit(&quotient);
        IntDeinit(&lcm);
        return false;
    }

    IntDeinit(&gcd);
    IntDeinit(&quotient);
    int_replace(result, &lcm);
    return true;
}

bool IntRootRem(Int *root, Int *remainder, Int *value, u64 degree) {
    ValidateInt(root);
    ValidateInt(remainder);
    ValidateInt(value);

    if (root == remainder) {
        LOG_FATAL("root and remainder must be different objects");
    }
    if (degree == 0) {
        LOG_ERROR("root degree is zero");
        return false;
    }

    if (IntIsZero(value)) {
        Int zero_root = IntInit(root->bits.allocator);
        Int zero_rem  = IntInit(remainder->bits.allocator);

        int_replace(root, &zero_root);
        int_replace(remainder, &zero_rem);
        return true;
    }
    if (degree == 1) {
        Int exact_root = IntInit(root->bits.allocator);
        Int zero_rem   = IntInit(remainder->bits.allocator);

        if (!IntTryClone(&exact_root, value)) {
            IntDeinit(&exact_root);
            IntDeinit(&zero_rem);
            return false;
        }
        int_replace(root, &exact_root);
        int_replace(remainder, &zero_rem);
        return true;
    }

    u64 bits       = IntBitLength(value);
    u64 high_shift = bits / degree;
    Int low        = IntInit(root->bits.allocator);
    Int high       = IntInit(root->bits.allocator);
    Int best       = IntInit(root->bits.allocator);
    Int one        = IntInit(root->bits.allocator);

    if ((bits % degree) != 0) {
        high_shift++;
    }
    if (high_shift == 0) {
        high_shift = 1;
    }

    if (!int_try_from_u64_with_allocator(&high, 1, root->bits.allocator) ||
        !int_try_from_u64_with_allocator(&one, 1, root->bits.allocator)) {
        IntDeinit(&low);
        IntDeinit(&high);
        IntDeinit(&best);
        IntDeinit(&one);
        return false;
    }

    if (!IntShiftLeft(&high, high_shift)) {
        IntDeinit(&low);
        IntDeinit(&high);
        IntDeinit(&best);
        IntDeinit(&one);
        return false;
    }

    while (IntLE(&low, &high)) {
        Int sum     = IntInit(root->bits.allocator);
        Int mid     = IntInit(root->bits.allocator);
        Int mid_pow = IntInit(root->bits.allocator);
        int cmp     = 0;

        if (!int_add(&sum, &low, &high) || !IntShiftRight(&sum, 1)) {
            IntDeinit(&sum);
            IntDeinit(&mid);
            IntDeinit(&mid_pow);
            IntDeinit(&low);
            IntDeinit(&high);
            IntDeinit(&best);
            IntDeinit(&one);
            return false;
        }
        mid = sum;

        if (!int_pow_u64(&mid_pow, &mid, degree)) {
            IntDeinit(&mid_pow);
            IntDeinit(&mid);
            IntDeinit(&low);
            IntDeinit(&high);
            IntDeinit(&best);
            IntDeinit(&one);
            return false;
        }
        cmp = int_compare(&mid_pow, value);

        if (cmp <= 0) {
            Int next = IntInit(root->bits.allocator);

            IntDeinit(&best);
            if (!IntTryClone(&best, &mid)) {
                IntDeinit(&mid_pow);
                IntDeinit(&mid);
                IntDeinit(&next);
                IntDeinit(&low);
                IntDeinit(&high);
                IntDeinit(&best);
                IntDeinit(&one);
                return false;
            }
            if (!int_add(&next, &mid, &one)) {
                IntDeinit(&mid_pow);
                IntDeinit(&mid);
                IntDeinit(&next);
                IntDeinit(&low);
                IntDeinit(&high);
                IntDeinit(&best);
                IntDeinit(&one);
                return false;
            }
            IntDeinit(&low);
            low = next;
        } else {
            Int next = IntInit(root->bits.allocator);

            if (IntEQ(&mid, &one) || IntIsZero(&mid)) {
                IntDeinit(&high);
                high = IntInit(root->bits.allocator);
            } else {
                if (!int_sub(&next, &mid, &one)) {
                    IntDeinit(&mid_pow);
                    IntDeinit(&mid);
                    IntDeinit(&next);
                    IntDeinit(&low);
                    IntDeinit(&high);
                    IntDeinit(&best);
                    IntDeinit(&one);
                    return false;
                }
                IntDeinit(&high);
                high = next;
            }
        }

        IntDeinit(&mid_pow);
        IntDeinit(&mid);
    }

    {
        Int power = IntInit(root->bits.allocator);
        Int rem   = IntInit(remainder->bits.allocator);

        if (!int_pow_u64(&power, &best, degree) || !int_sub(&rem, value, &power)) {
            IntDeinit(&power);
            IntDeinit(&rem);
            IntDeinit(&low);
            IntDeinit(&high);
            IntDeinit(&one);
            IntDeinit(&best);
            return false;
        }

        IntDeinit(&power);
        IntDeinit(&low);
        IntDeinit(&high);
        IntDeinit(&one);

        int_replace(root, &best);
        int_replace(remainder, &rem);
    }

    return true;
}

bool IntRoot(Int *result, Int *value, u64 degree) {
    Int root      = IntInit(result->bits.allocator);
    Int remainder = IntInit(result->bits.allocator);

    if (!IntRootRem(&root, &remainder, value, degree)) {
        IntDeinit(&root);
        IntDeinit(&remainder);
        return false;
    }

    IntDeinit(&remainder);
    int_replace(result, &root);
    return true;
}

bool IntSqrtRem(Int *root, Int *remainder, Int *value) {
    return IntRootRem(root, remainder, value, 2);
}

bool IntSqrt(Int *result, Int *value) {
    return IntRoot(result, value, 2);
}

bool IntIsPerfectSquare(Int *value) {
    ValidateInt(value);

    Int  root      = IntInit(value->bits.allocator);
    Int  remainder = IntInit(value->bits.allocator);
    bool result    = false;

    if (!IntSqrtRem(&root, &remainder, value)) {
        IntDeinit(&root);
        IntDeinit(&remainder);
        return false;
    }
    result = IntIsZero(&remainder);

    IntDeinit(&root);
    IntDeinit(&remainder);
    return result;
}

bool IntIsPerfectPower(Int *value) {
    ValidateInt(value);

    if (IntIsZero(value) || IntBitLength(value) == 1) {
        return true;
    }

    u64 max_degree = 0;

    if (!IntTryLog2(value, &max_degree)) {
        return false;
    }

    for (u64 degree = 2; degree <= max_degree; degree++) {
        Int  root      = IntInit(value->bits.allocator);
        Int  remainder = IntInit(value->bits.allocator);
        bool exact     = false;

        if (!IntRootRem(&root, &remainder, value, degree)) {
            IntDeinit(&root);
            IntDeinit(&remainder);
            return false;
        }
        exact = IntIsZero(&remainder);

        IntDeinit(&root);
        IntDeinit(&remainder);

        if (exact) {
            return true;
        }
    }

    return false;
}

bool IntTryJacobi(int *out, Int *a, Int *n) {
    ValidateInt(a);
    ValidateInt(n);

    if (!out) {
        LOG_FATAL("Invalid arguments");
    }

    if (IntIsZero(n) || IntIsEven(n)) {
        LOG_ERROR("n must be non-zero and odd");
        return false;
    }

    Int aa     = IntInit(a->bits.allocator);
    Int nn     = IntInit(n->bits.allocator);
    int result = 1;

    if (!int_try_clone_value(&nn, n) || !int_mod(&aa, a, &nn)) {
        IntDeinit(&aa);
        IntDeinit(&nn);
        return false;
    }

    while (!IntIsZero(&aa)) {
        while (IntIsEven(&aa)) {
            u64 n_mod_8 = 0;

            if (!IntShiftRight(&aa, 1)) {
                IntDeinit(&aa);
                IntDeinit(&nn);
                return false;
            }
            n_mod_8 = int_mod_u64(&nn, 8);
            if (n_mod_8 == 3 || n_mod_8 == 5) {
                result = -result;
            }
        }

        int_swap(&aa, &nn);

        if (int_mod_u64(&aa, 4) == 3 && int_mod_u64(&nn, 4) == 3) {
            result = -result;
        }

        if (!int_mod(&aa, &aa, &nn)) {
            IntDeinit(&aa);
            IntDeinit(&nn);
            return false;
        }
    }

    IntDeinit(&aa);
    if (int_compare_u64(&nn, 1) != 0) {
        IntDeinit(&nn);
        *out = 0;
        return true;
    }

    IntDeinit(&nn);
    *out = result;
    return true;
}

int IntJacobiWithError(Int *a, Int *n, bool *error) {
    int  out = 0;
    bool ok  = IntTryJacobi(&out, a, n);

    if (error) {
        *error = !ok;
    }

    return out;
}

bool IntModAdd(Int *result, Int *a, Int *b, Int *modulus) {
    ValidateInt(result);
    ValidateInt(a);
    ValidateInt(b);
    ValidateInt(modulus);

    if (IntIsZero(modulus)) {
        LOG_ERROR("modulus is zero");
        return false;
    }

    Int ar  = IntInit(result->bits.allocator);
    Int br  = IntInit(result->bits.allocator);
    Int sum = IntInit(result->bits.allocator);

    if (!int_mod(&ar, a, modulus) || !int_mod(&br, b, modulus) || !int_add(&sum, &ar, &br) ||
        !int_mod(result, &sum, modulus)) {
        IntDeinit(&ar);
        IntDeinit(&br);
        IntDeinit(&sum);
        return false;
    }

    IntDeinit(&ar);
    IntDeinit(&br);
    IntDeinit(&sum);
    return true;
}

bool IntModSub(Int *result, Int *a, Int *b, Int *modulus) {
    ValidateInt(result);
    ValidateInt(a);
    ValidateInt(b);
    ValidateInt(modulus);

    if (IntIsZero(modulus)) {
        LOG_ERROR("modulus is zero");
        return false;
    }

    Int ar = IntInit(result->bits.allocator);
    Int br = IntInit(result->bits.allocator);

    if (!int_mod(&ar, a, modulus) || !int_mod(&br, b, modulus)) {
        IntDeinit(&ar);
        IntDeinit(&br);
        return false;
    }

    if (IntGE(&ar, &br)) {
        if (!int_sub(result, &ar, &br)) {
            IntDeinit(&ar);
            IntDeinit(&br);
            return false;
        }
    } else {
        Int diff = IntInit(result->bits.allocator);

        if (!int_sub(&diff, &br, &ar)) {
            IntDeinit(&ar);
            IntDeinit(&br);
            IntDeinit(&diff);
            return false;
        }
        if (IntIsZero(&diff)) {
            Int zero = IntInit(result->bits.allocator);
            int_replace(result, &zero);
        } else {
            if (!int_sub(result, modulus, &diff)) {
                IntDeinit(&ar);
                IntDeinit(&br);
                IntDeinit(&diff);
                return false;
            }
        }

        IntDeinit(&diff);
    }

    IntDeinit(&ar);
    IntDeinit(&br);
    return true;
}

bool IntModMul(Int *result, Int *a, Int *b, Int *modulus) {
    ValidateInt(result);
    ValidateInt(a);
    ValidateInt(b);
    ValidateInt(modulus);

    if (IntIsZero(modulus)) {
        LOG_ERROR("modulus is zero");
        return false;
    }

    Int ar   = IntInit(result->bits.allocator);
    Int br   = IntInit(result->bits.allocator);
    Int prod = IntInit(result->bits.allocator);

    if (!int_mod(&ar, a, modulus) || !int_mod(&br, b, modulus) || !int_mul(&prod, &ar, &br) ||
        !int_mod(result, &prod, modulus)) {
        IntDeinit(&ar);
        IntDeinit(&br);
        IntDeinit(&prod);
        return false;
    }

    IntDeinit(&ar);
    IntDeinit(&br);
    IntDeinit(&prod);
    return true;
}

bool IntModDiv(Int *result, Int *a, Int *b, Int *modulus) {
    ValidateInt(result);
    ValidateInt(a);
    ValidateInt(b);
    ValidateInt(modulus);

    if (IntIsZero(modulus)) {
        LOG_ERROR("modulus is zero");
        return false;
    }

    Int  inverse = IntInit(result->bits.allocator);
    Int  value   = IntInit(result->bits.allocator);
    bool ok      = false;

    ok = IntModInv(&inverse, b, modulus);
    if (!ok) {
        IntDeinit(&inverse);
        IntDeinit(&value);
        return false;
    }

    if (!IntModMul(&value, a, &inverse, modulus)) {
        IntDeinit(&inverse);
        IntDeinit(&value);
        return false;
    }

    IntDeinit(&inverse);
    int_replace(result, &value);
    return true;
}

bool IntSquareMod(Int *result, Int *value, Int *modulus) {
    return IntModMul(result, value, value, modulus);
}

bool int_pow_u64_mod(Int *result, Int *base, u64 exponent, Int *modulus) {
    ValidateInt(result);
    ValidateInt(base);
    ValidateInt(modulus);

    if (IntIsZero(modulus)) {
        LOG_FATAL("modulus is zero");
    }

    Int acc      = IntInit(result->bits.allocator);
    Int base_mod = IntInit(result->bits.allocator);

    if (!int_try_from_u64_with_allocator(&acc, 1, result->bits.allocator)) {
        IntDeinit(&base_mod);
        return false;
    }
    if (!int_mod(&acc, &acc, modulus) || !int_mod(&base_mod, base, modulus)) {
        IntDeinit(&acc);
        IntDeinit(&base_mod);
        return false;
    }

    while (exponent > 0) {
        if (exponent & 1u) {
            Int next = IntInit(result->bits.allocator);

            if (!IntModMul(&next, &acc, &base_mod, modulus)) {
                IntDeinit(&acc);
                IntDeinit(&base_mod);
                IntDeinit(&next);
                return false;
            }
            IntDeinit(&acc);
            acc = next;
        }

        exponent >>= 1u;
        if (exponent > 0) {
            Int next = IntInit(result->bits.allocator);

            if (!IntModMul(&next, &base_mod, &base_mod, modulus)) {
                IntDeinit(&acc);
                IntDeinit(&base_mod);
                IntDeinit(&next);
                return false;
            }
            IntDeinit(&base_mod);
            base_mod = next;
        }
    }

    IntDeinit(&base_mod);
    int_replace(result, &acc);
    return true;
}

bool int_pow_mod(Int *result, Int *base, Int *exponent, Int *modulus) {
    ValidateInt(result);
    ValidateInt(base);
    ValidateInt(exponent);
    ValidateInt(modulus);

    if (IntIsZero(modulus)) {
        LOG_ERROR("modulus is zero");
        return false;
    }

    Int acc      = IntInit(result->bits.allocator);
    Int base_mod = IntInit(result->bits.allocator);
    Int exp      = IntInit(exponent->bits.allocator);

    if (!int_try_from_u64_with_allocator(&acc, 1, result->bits.allocator) || !IntTryClone(&exp, exponent) ||
        !int_mod(&acc, &acc, modulus) || !int_mod(&base_mod, base, modulus)) {
        IntDeinit(&acc);
        IntDeinit(&base_mod);
        IntDeinit(&exp);
        return false;
    }

    while (!IntIsZero(&exp)) {
        if (int_is_odd(&exp)) {
            Int next = IntInit(result->bits.allocator);

            if (!IntModMul(&next, &acc, &base_mod, modulus)) {
                IntDeinit(&acc);
                IntDeinit(&base_mod);
                IntDeinit(&exp);
                IntDeinit(&next);
                return false;
            }
            IntDeinit(&acc);
            acc = next;
        }

        if (!IntShiftRight(&exp, 1)) {
            IntDeinit(&acc);
            IntDeinit(&base_mod);
            IntDeinit(&exp);
            return false;
        }
        if (!IntIsZero(&exp)) {
            Int next = IntInit(result->bits.allocator);

            if (!IntModMul(&next, &base_mod, &base_mod, modulus)) {
                IntDeinit(&acc);
                IntDeinit(&base_mod);
                IntDeinit(&exp);
                IntDeinit(&next);
                return false;
            }
            IntDeinit(&base_mod);
            base_mod = next;
        }
    }

    IntDeinit(&exp);
    IntDeinit(&base_mod);
    int_replace(result, &acc);
    return true;
}

bool int_pow_i64_mod(Int *result, Int *base, i64 exponent, Int *modulus) {
    if (exponent < 0) {
        LOG_FATAL("Int exponent cannot be negative");
    }

    return int_pow_u64_mod(result, base, (u64)exponent, modulus);
}

bool IntModInv(Int *result, Int *value, Int *modulus) {
    ValidateInt(result);
    ValidateInt(value);
    ValidateInt(modulus);

    if (IntIsZero(modulus)) {
        LOG_ERROR("modulus is zero");
        return false;
    }

    Int       reduced = IntInit(result->bits.allocator);
    SignedInt t       = sint_init(result->bits.allocator);
    SignedInt new_t   = sint_from_u64(1, result->bits.allocator);
    Int       r       = IntInit(modulus->bits.allocator);
    Int       new_r   = IntInit(result->bits.allocator);
    Int       one     = int_from_u64(1, result->bits.allocator);
    bool      ok      = false;

    if (!IntTryClone(&r, modulus) || !int_mod(&reduced, value, modulus)) {
        IntDeinit(&reduced);
        IntDeinit(&r);
        IntDeinit(&new_r);
        IntDeinit(&one);
        sint_deinit(&t);
        sint_deinit(&new_t);
        return false;
    }
    if (!IntTryClone(&new_r, &reduced)) {
        IntDeinit(&reduced);
        IntDeinit(&r);
        IntDeinit(&new_r);
        IntDeinit(&one);
        sint_deinit(&t);
        sint_deinit(&new_t);
        return false;
    }

    while (!IntIsZero(&new_r)) {
        Int       q       = IntInit(result->bits.allocator);
        Int       rem     = IntInit(result->bits.allocator);
        SignedInt q_new_t = sint_init(result->bits.allocator);
        SignedInt next_t  = sint_init(result->bits.allocator);
        Int       next_r  = IntInit(result->bits.allocator);

        if (!int_div_mod(&q, &rem, &r, &new_r) || !sint_mul_unsigned(&q_new_t, &new_t, &q) ||
            !sint_sub(&next_t, &t, &q_new_t)) {
            IntDeinit(&q);
            IntDeinit(&rem);
            IntDeinit(&next_r);
            sint_deinit(&q_new_t);
            sint_deinit(&next_t);
            IntDeinit(&reduced);
            IntDeinit(&r);
            IntDeinit(&new_r);
            IntDeinit(&one);
            sint_deinit(&t);
            sint_deinit(&new_t);
            return false;
        }

        IntDeinit(&next_r);
        next_r = rem;

        sint_deinit(&t);
        t     = new_t;
        new_t = next_t;

        IntDeinit(&r);
        r     = new_r;
        new_r = next_r;

        sint_deinit(&q_new_t);
        IntDeinit(&q);
    }

    if (IntEQ(&r, &one)) {
        Int positive = IntInit(result->bits.allocator);
        Int mag_mod  = IntInit(result->bits.allocator);

        if (!int_mod(&mag_mod, &t.magnitude, modulus)) {
            IntDeinit(&positive);
            IntDeinit(&mag_mod);
            IntDeinit(&reduced);
            IntDeinit(&r);
            IntDeinit(&new_r);
            IntDeinit(&one);
            sint_deinit(&t);
            sint_deinit(&new_t);
            return false;
        }
        if (t.negative && !IntIsZero(&mag_mod)) {
            if (!int_sub(&positive, modulus, &mag_mod)) {
                IntDeinit(&positive);
                IntDeinit(&mag_mod);
                IntDeinit(&reduced);
                IntDeinit(&r);
                IntDeinit(&new_r);
                IntDeinit(&one);
                sint_deinit(&t);
                sint_deinit(&new_t);
                return false;
            }
        } else {
            if (!int_mod(&positive, &t.magnitude, modulus)) {
                IntDeinit(&positive);
                IntDeinit(&mag_mod);
                IntDeinit(&reduced);
                IntDeinit(&r);
                IntDeinit(&new_r);
                IntDeinit(&one);
                sint_deinit(&t);
                sint_deinit(&new_t);
                return false;
            }
        }

        IntDeinit(&mag_mod);
        int_replace(result, &positive);
        ok = true;
    }

    IntDeinit(&reduced);
    IntDeinit(&r);
    IntDeinit(&new_r);
    IntDeinit(&one);
    sint_deinit(&t);
    sint_deinit(&new_t);
    return ok;
}

bool IntModSqrt(Int *result, Int *value, Int *modulus) {
    ValidateInt(result);
    ValidateInt(value);
    ValidateInt(modulus);

    if (IntIsZero(modulus)) {
        LOG_ERROR("modulus is zero");
        return false;
    }

    Int  a  = IntInit(result->bits.allocator);
    bool ok = false;

    if (!int_mod(&a, value, modulus)) {
        IntDeinit(&a);
        return false;
    }

    if (IntIsZero(&a)) {
        Int zero = IntInit(result->bits.allocator);
        int_replace(result, &zero);
        IntDeinit(&a);
        return true;
    }
    if (int_compare_u64(modulus, 2) == 0) {
        int_replace(result, &a);
        return true;
    }
    {
        bool prime_error = false;
        bool prime       = IntIsProbablePrime(modulus, &prime_error);

        if (prime_error) {
            IntDeinit(&a);
            return false;
        }
        if (IntIsEven(modulus) || !prime) {
            IntDeinit(&a);
            return false;
        }
    }
    {
        int jacobi = 0;
        if (!IntTryJacobi(&jacobi, &a, modulus) || jacobi != 1) {
            IntDeinit(&a);
            return false;
        }
    }
    if (int_mod_u64(modulus, 4) == 3) {
        Int exponent = IntInit(modulus->bits.allocator);
        Int root     = IntInit(result->bits.allocator);

        if (!IntTryClone(&exponent, modulus) || !int_add_u64(&exponent, &exponent, 1) || !IntShiftRight(&exponent, 2) ||
            !int_pow_mod(&root, &a, &exponent, modulus)) {
            IntDeinit(&exponent);
            IntDeinit(&root);
            IntDeinit(&a);
            return false;
        }

        IntDeinit(&exponent);
        IntDeinit(&a);
        int_replace(result, &root);
        return true;
    }

    {
        Int q        = IntInit(modulus->bits.allocator);
        Int z        = IntInit(modulus->bits.allocator);
        Int c        = IntInit(result->bits.allocator);
        Int t        = IntInit(result->bits.allocator);
        Int r        = IntInit(result->bits.allocator);
        Int exponent = IntInit(result->bits.allocator);
        u64 m        = 0;

        if (!IntTryClone(&q, modulus) || !int_try_from_u64_with_allocator(&z, 2, modulus->bits.allocator) ||
            !int_sub_u64(&q, &q, 1)) {
            IntDeinit(&q);
            IntDeinit(&z);
            IntDeinit(&c);
            IntDeinit(&t);
            IntDeinit(&r);
            IntDeinit(&exponent);
            IntDeinit(&a);
            return false;
        }
        while (IntIsEven(&q)) {
            if (!IntShiftRight(&q, 1)) {
                IntDeinit(&q);
                IntDeinit(&z);
                IntDeinit(&c);
                IntDeinit(&t);
                IntDeinit(&r);
                IntDeinit(&exponent);
                IntDeinit(&a);
                return false;
            }
            m++;
        }

        while (true) {
            int jacobi = 0;

            if (!IntTryJacobi(&jacobi, &z, modulus)) {
                IntDeinit(&q);
                IntDeinit(&z);
                IntDeinit(&c);
                IntDeinit(&t);
                IntDeinit(&r);
                IntDeinit(&exponent);
                IntDeinit(&a);
                return false;
            }

            if (jacobi == -1) {
                break;
            }

            if (!int_add_u64(&z, &z, 1)) {
                IntDeinit(&q);
                IntDeinit(&z);
                IntDeinit(&c);
                IntDeinit(&t);
                IntDeinit(&r);
                IntDeinit(&exponent);
                IntDeinit(&a);
                return false;
            }
        }

        if (!int_pow_mod(&c, &z, &q, modulus) || !int_pow_mod(&t, &a, &q, modulus)) {
            IntDeinit(&q);
            IntDeinit(&z);
            IntDeinit(&c);
            IntDeinit(&t);
            IntDeinit(&r);
            IntDeinit(&exponent);
            IntDeinit(&a);
            return false;
        }

        if (!IntTryClone(&exponent, &q) || !int_add_u64(&exponent, &exponent, 1) || !IntShiftRight(&exponent, 1) ||
            !int_pow_mod(&r, &a, &exponent, modulus)) {
            IntDeinit(&q);
            IntDeinit(&z);
            IntDeinit(&c);
            IntDeinit(&t);
            IntDeinit(&r);
            IntDeinit(&exponent);
            IntDeinit(&a);
            return false;
        }

        while (int_compare_u64(&t, 1) != 0) {
            Int t_power = IntInit(t.bits.allocator);
            u64 i       = 0;

            if (!IntTryClone(&t_power, &t)) {
                IntDeinit(&t_power);
                IntDeinit(&q);
                IntDeinit(&z);
                IntDeinit(&c);
                IntDeinit(&t);
                IntDeinit(&r);
                IntDeinit(&exponent);
                IntDeinit(&a);
                return false;
            }

            for (i = 1; i < m; i++) {
                Int next = IntInit(result->bits.allocator);

                if (!IntSquareMod(&next, &t_power, modulus)) {
                    IntDeinit(&next);
                    IntDeinit(&t_power);
                    IntDeinit(&q);
                    IntDeinit(&z);
                    IntDeinit(&c);
                    IntDeinit(&t);
                    IntDeinit(&r);
                    IntDeinit(&exponent);
                    IntDeinit(&a);
                    return false;
                }
                IntDeinit(&t_power);
                t_power = next;

                if (int_compare_u64(&t_power, 1) == 0) {
                    break;
                }
            }

            if (i == m) {
                IntDeinit(&t_power);
                break;
            }

            {
                Int b    = IntInit(c.bits.allocator);
                Int b_sq = IntInit(result->bits.allocator);
                Int next = IntInit(result->bits.allocator);

                if (!IntTryClone(&b, &c)) {
                    IntDeinit(&b);
                    IntDeinit(&b_sq);
                    IntDeinit(&next);
                    IntDeinit(&t_power);
                    IntDeinit(&q);
                    IntDeinit(&z);
                    IntDeinit(&c);
                    IntDeinit(&t);
                    IntDeinit(&r);
                    IntDeinit(&exponent);
                    IntDeinit(&a);
                    return false;
                }

                for (u64 j = 0; j + i + 1 < m; j++) {
                    Int square = IntInit(result->bits.allocator);

                    if (!IntSquareMod(&square, &b, modulus)) {
                        IntDeinit(&square);
                        IntDeinit(&b);
                        IntDeinit(&b_sq);
                        IntDeinit(&next);
                        IntDeinit(&t_power);
                        IntDeinit(&q);
                        IntDeinit(&z);
                        IntDeinit(&c);
                        IntDeinit(&t);
                        IntDeinit(&r);
                        IntDeinit(&exponent);
                        IntDeinit(&a);
                        return false;
                    }
                    IntDeinit(&b);
                    b = square;
                }

                if (!IntModMul(&next, &r, &b, modulus)) {
                    IntDeinit(&b);
                    IntDeinit(&b_sq);
                    IntDeinit(&next);
                    IntDeinit(&t_power);
                    IntDeinit(&q);
                    IntDeinit(&z);
                    IntDeinit(&c);
                    IntDeinit(&t);
                    IntDeinit(&r);
                    IntDeinit(&exponent);
                    IntDeinit(&a);
                    return false;
                }
                IntDeinit(&r);
                r = next;

                if (!IntSquareMod(&b_sq, &b, modulus)) {
                    IntDeinit(&b);
                    IntDeinit(&b_sq);
                    IntDeinit(&t_power);
                    IntDeinit(&q);
                    IntDeinit(&z);
                    IntDeinit(&c);
                    IntDeinit(&t);
                    IntDeinit(&r);
                    IntDeinit(&exponent);
                    IntDeinit(&a);
                    return false;
                }
                next = IntInit(result->bits.allocator);
                if (!IntModMul(&next, &t, &b_sq, modulus)) {
                    IntDeinit(&b);
                    IntDeinit(&b_sq);
                    IntDeinit(&next);
                    IntDeinit(&t_power);
                    IntDeinit(&q);
                    IntDeinit(&z);
                    IntDeinit(&c);
                    IntDeinit(&t);
                    IntDeinit(&r);
                    IntDeinit(&exponent);
                    IntDeinit(&a);
                    return false;
                }
                IntDeinit(&t);
                t = next;

                IntDeinit(&c);
                c = b_sq;
                IntDeinit(&b);
            }

            IntDeinit(&t_power);
            m = i;
        }

        ok = int_compare_u64(&t, 1) == 0;
        IntDeinit(&q);
        IntDeinit(&z);
        IntDeinit(&c);
        IntDeinit(&t);
        IntDeinit(&exponent);
        IntDeinit(&a);

        if (ok) {
            int_replace(result, &r);
        } else {
            IntDeinit(&r);
        }
    }

    return ok;
}

bool IntIsProbablePrimeWithError(Int *value, bool *error) {
    static const u64 bases[] = {2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37};

    if (error) {
        *error = false;
    }
    ValidateInt(value);

    if (int_compare_u64(value, 2) < 0) {
        return false;
    }
    if (int_compare_u64(value, 2) == 0) {
        return true;
    }
    if (IntIsEven(value)) {
        return false;
    }

    for (u64 i = 0; i < (u64)(sizeof(bases) / sizeof(bases[0])); i++) {
        if (int_compare_u64(value, bases[i]) == 0) {
            return true;
        }
        if (int_mod_u64(value, bases[i]) == 0) {
            return false;
        }
    }

    {
        Int  d           = IntInit(value->bits.allocator);
        Int  n_minus_one = IntInit(value->bits.allocator);
        u64  s           = 0;
        bool probable    = true;

        if (!IntTryClone(&d, value) || !int_sub_u64(&d, &d, 1)) {
            IntDeinit(&d);
            IntDeinit(&n_minus_one);
            return false;
        }
        if (!IntTryClone(&n_minus_one, &d)) {
            IntDeinit(&d);
            IntDeinit(&n_minus_one);
            return false;
        }

        while (IntIsEven(&d)) {
            if (!IntShiftRight(&d, 1)) {
                IntDeinit(&d);
                IntDeinit(&n_minus_one);
                return false;
            }
            s++;
        }

        for (u64 i = 0; i < (u64)(sizeof(bases) / sizeof(bases[0])); i++) {
            Int base = IntInit(value->bits.allocator);
            Int x    = IntInit(value->bits.allocator);

            if (!int_try_from_u64_with_allocator(&base, bases[i], value->bits.allocator)) {
                IntDeinit(&base);
                IntDeinit(&x);
                IntDeinit(&d);
                IntDeinit(&n_minus_one);
                return false;
            }

            if (int_compare(&base, value) >= 0) {
                IntDeinit(&base);
                IntDeinit(&x);
                continue;
            }

            if (!int_pow_mod(&x, &base, &d, value)) {
                IntDeinit(&base);
                IntDeinit(&x);
                IntDeinit(&d);
                IntDeinit(&n_minus_one);
                return false;
            }
            if ((int_compare_u64(&x, 1) == 0) || IntEQ(&x, &n_minus_one)) {
                IntDeinit(&base);
                IntDeinit(&x);
                continue;
            }

            {
                bool witness = true;

                for (u64 r = 1; r < s; r++) {
                    Int next = IntInit(value->bits.allocator);

                    if (!IntSquareMod(&next, &x, value)) {
                        IntDeinit(&next);
                        IntDeinit(&base);
                        IntDeinit(&x);
                        IntDeinit(&d);
                        IntDeinit(&n_minus_one);
                        return false;
                    }
                    IntDeinit(&x);
                    x = next;

                    if (IntEQ(&x, &n_minus_one)) {
                        witness = false;
                        break;
                    }
                }

                if (witness) {
                    probable = false;
                }
            }

            IntDeinit(&base);
            IntDeinit(&x);
            if (!probable) {
                break;
            }
        }

        IntDeinit(&d);
        IntDeinit(&n_minus_one);
        return probable;
    }
}

bool IntNextPrime(Int *result, Int *value) {
    bool error = false;

    ValidateInt(result);
    ValidateInt(value);

    if (int_compare_u64(value, 1) <= 0) {
        Int two = IntInit(result->bits.allocator);

        if (!int_try_from_u64_with_allocator(&two, 2, result->bits.allocator)) {
            IntDeinit(&two);
            return false;
        }
        int_replace(result, &two);
        return true;
    }

    Int candidate = IntInit(result->bits.allocator);

    if (!IntTryClone(&candidate, value)) {
        IntDeinit(&candidate);
        return false;
    }

    if (!int_add_u64(&candidate, &candidate, 1)) {
        IntDeinit(&candidate);
        return false;
    }
    if (int_compare_u64(&candidate, 2) <= 0) {
        Int two = IntInit(result->bits.allocator);

        if (!int_try_from_u64_with_allocator(&two, 2, result->bits.allocator)) {
            IntDeinit(&two);
            IntDeinit(&candidate);
            return false;
        }
        IntDeinit(&candidate);
        int_replace(result, &two);
        return true;
    }
    if (IntIsEven(&candidate)) {
        if (!int_add_u64(&candidate, &candidate, 1)) {
            IntDeinit(&candidate);
            return false;
        }
    }

    while (!IntIsProbablePrimeWithError(&candidate, &error)) {
        if (error) {
            IntDeinit(&candidate);
            return false;
        }
        if (!int_add_u64(&candidate, &candidate, 2)) {
            IntDeinit(&candidate);
            return false;
        }
    }

    int_replace(result, &candidate);
    return true;
}
