/// file      : std/container/int.c
/// author    : Generated following Misra project patterns
/// This is free and unencumbered software released into the public domain.
///
/// Arbitrary-precision unsigned integer implementation built on top of BitVec.

#include <Misra/Std/Container/Int.h>
#include <Misra/Std/Container/BitVec.h>
#include <Misra/Std/Log.h>
#include <string.h>

typedef struct {
    bool negative;
    Int  magnitude;
} SignedInt;

#define INT_BITS(value) (&(value)->bits)

static void int_normalize(Int *value);
static void int_validate_radix(u8 radix);
static Int  int_from_str_radix_strict(const char *digits, u8 radix);

static Int int_wrap(BitVec bits) {
    Int value;

    value.bits = bits;
    return value;
}

static Int int_init_with_capacity(u64 capacity) {
    return int_wrap(BitVecInitWithCapacity(capacity));
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

static SignedInt sint_init(void) {
    SignedInt value = {.negative = false, .magnitude = IntInit()};
    return value;
}

static SignedInt sint_from_u64(u64 value) {
    SignedInt result = {.negative = false, .magnitude = IntFromU64(value)};
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

static void sint_add(SignedInt *result, SignedInt *a, SignedInt *b) {
    SignedInt temp = sint_init();

    if (a->negative == b->negative) {
        IntAdd(&temp.magnitude, &a->magnitude, &b->magnitude);
        temp.negative = a->negative;
    } else {
        int cmp = IntCompare(&a->magnitude, &b->magnitude);

        if (cmp == 0) {
            temp.negative = false;
        } else if (cmp > 0) {
            (void)IntSub(&temp.magnitude, &a->magnitude, &b->magnitude);
            temp.negative = a->negative;
        } else {
            (void)IntSub(&temp.magnitude, &b->magnitude, &a->magnitude);
            temp.negative = b->negative;
        }
    }

    sint_normalize(&temp);
    sint_replace(result, &temp);
}

static void sint_sub(SignedInt *result, SignedInt *a, SignedInt *b) {
    SignedInt neg_b = sint_clone(b);

    if (!IntIsZero(&neg_b.magnitude)) {
        neg_b.negative = !neg_b.negative;
    }

    sint_add(result, a, &neg_b);
    sint_deinit(&neg_b);
}

static void sint_mul_unsigned(SignedInt *result, SignedInt *a, Int *b) {
    SignedInt temp = sint_init();

    IntMul(&temp.magnitude, &a->magnitude, b);
    temp.negative = a->negative;
    sint_normalize(&temp);
    sint_replace(result, &temp);
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

static void int_mul_u64_in_place(Int *value, u64 factor) {
    Int lhs    = IntClone(value);
    Int rhs    = IntFromU64(factor);
    Int result = IntInit();

    IntMul(&result, &lhs, &rhs);

    IntDeinit(&lhs);
    IntDeinit(&rhs);
    int_replace(value, &result);
}

static void int_add_u64_in_place(Int *value, u64 addend) {
    Int lhs    = IntClone(value);
    Int rhs    = IntFromU64(addend);
    Int result = IntInit();

    IntAdd(&result, &lhs, &rhs);

    IntDeinit(&lhs);
    IntDeinit(&rhs);
    int_replace(value, &result);
}

static void int_validate_radix(u8 radix) {
    if (radix < 2 || radix > 36) {
        LOG_FATAL("radix must be between 2 and 36");
    }
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

static Int int_from_str_radix_impl(const char *digits, u64 start, u8 radix) {
    Int  result    = IntInit();
    bool saw_digit = false;

    int_validate_radix(radix);

    for (u64 i = start; digits[i] != '\0'; i++) {
        int digit = 0;

        if (digits[i] == '_') {
            continue;
        }

        digit = int_radix_digit(digits[i]);
        if (digit < 0 || digit >= radix) {
            LOG_FATAL("Invalid digit for radix in Int conversion");
        }

        saw_digit = true;
        int_mul_u64_in_place(&result, radix);
        int_add_u64_in_place(&result, (u64)digit);
    }

    if (!saw_digit) {
        return result;
    }

    int_normalize(&result);
    return result;
}

static Int int_from_str_radix_strict(const char *digits, u8 radix) {
    Int  result    = IntInit();
    bool saw_digit = false;

    if (!digits) {
        LOG_FATAL("digits is NULL");
    }

    int_validate_radix(radix);

    for (u64 i = 0; digits[i] != '\0'; i++) {
        int digit = int_radix_digit(digits[i]);

        if (digit < 0 || digit >= radix) {
            LOG_FATAL("Invalid digit for radix in Int conversion");
        }

        saw_digit = true;
        int_mul_u64_in_place(&result, radix);
        int_add_u64_in_place(&result, (u64)digit);
    }

    if (!saw_digit) {
        return result;
    }

    int_normalize(&result);
    return result;
}

u64 IntBitLength(Int *value) {
    return int_significant_bits(value);
}

u64 IntByteLength(Int *value) {
    u64 bits = IntBitLength(value);
    return bits == 0 ? 0 : (bits + 7) / 8;
}

u64 IntLog2(Int *value) {
    ValidateInt(value);

    if (IntIsZero(value)) {
        LOG_FATAL("log2 undefined for zero");
    }

    return IntBitLength(value) - 1;
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

Int IntClone(Int *value) {
    ValidateInt(value);

    Int clone = int_wrap(BitVecClone(INT_BITS(value)));
    int_normalize(&clone);
    return clone;
}

Int IntFromU64(u64 value) {
    u64 bits = int_u64_bits(value);

    if (bits == 0) {
        return IntInit();
    }

    return int_wrap(BitVecFromInteger(value, bits));
}

u64 IntToU64(Int *value) {
    ValidateInt(value);

    if (!IntFitsU64(value)) {
        LOG_FATAL("Int value exceeds u64 range");
    }

    return BitVecToInteger(INT_BITS(value));
}

Int IntFromBytesLE(const u8 *bytes, u64 len) {
    if (!bytes && len != 0) {
        LOG_FATAL("bytes is NULL");
    }

    if (len == 0) {
        return IntInit();
    }

    Int result = int_wrap(BitVecFromBytes(bytes, len * 8));
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

    memset(bytes, 0, bytes_to_copy);

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

Int IntFromBytesBE(const u8 *bytes, u64 len) {
    if (!bytes && len != 0) {
        LOG_FATAL("bytes is NULL");
    }

    Int result = IntInit();

    for (u64 i = 0; i < len; i++) {
        IntShiftLeft(&result, 8);
        int_add_u64_in_place(&result, bytes[i]);
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

    memset(bytes, 0, bytes_to_copy);

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

Int IntFromStr(const char *decimal) {
    if (!decimal) {
        LOG_FATAL("decimal is NULL");
    }

    u64 start = 0;

    if (decimal[0] == '+') {
        start = 1;
    }

    return int_from_str_radix_impl(decimal, start, 10);
}

Str IntToStr(Int *value) {
    return IntToStrRadix(value, 10, false);
}

Int IntFromStrRadix(const char *digits, u8 radix) {
    u64 start = 0;

    if (!digits) {
        LOG_FATAL("digits is NULL");
    }
    if (digits[0] == '+') {
        start = 1;
    }

    return int_from_str_radix_impl(digits, start, radix);
}

Str IntToStrRadix(Int *value, u8 radix, bool uppercase) {
    ValidateInt(value);
    int_validate_radix(radix);

    if (IntIsZero(value)) {
        return StrInitFromZstr("0");
    }

    Int current   = IntClone(value);
    Str result    = StrInit();

    while (!IntIsZero(&current)) {
        Int quotient = IntInit();
        u64 digit    = 0;

        digit = IntDivU64Rem(&quotient, &current, radix);
        StrPushBack(&result, int_radix_char((u8)digit, uppercase));

        IntDeinit(&current);
        current = quotient;
    }

    for (u64 i = 0; i < result.length / 2; i++) {
        char tmp                           = result.data[i];
        result.data[i]                    = result.data[result.length - 1 - i];
        result.data[result.length - 1 - i] = tmp;
    }

    IntDeinit(&current);
    return result;
}

Int IntFromBinary(const char *binary) {
    if (!binary) {
        LOG_FATAL("binary is NULL");
    }

    u64 start  = 0;
    u64 len    = (u64)strlen(binary);

    if (len >= 2 && binary[0] == '0' && (binary[1] == 'b' || binary[1] == 'B')) {
        start = 2;
    }

    return int_from_str_radix_impl(binary, start, 2);
}

Str IntToBinary(Int *value) {
    return IntToStrRadix(value, 2, false);
}

Int IntFromOctStr(const char *octal) {
    if (!octal) {
        LOG_FATAL("octal is NULL");
    }

    u64 start = 0;
    u64 len   = (u64)strlen(octal);

    if (len >= 2 && octal[0] == '0' && (octal[1] == 'o' || octal[1] == 'O')) {
        start = 2;
    }

    return int_from_str_radix_impl(octal, start, 8);
}

Str IntToOctStr(Int *value) {
    return IntToStrRadix(value, 8, false);
}

Int IntFromHexStr(const char *hex) {
    if (!hex) {
        LOG_FATAL("hex is NULL");
    }

    return int_from_str_radix_strict(hex, 16);
}

Str IntToHexStr(Int *value) {
    return IntToStrRadix(value, 16, false);
}

int IntCompare(Int *lhs, Int *rhs) {
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

int IntCompareU64(Int *lhs, u64 rhs) {
    ValidateInt(lhs);

    if (IntBitLength(lhs) > 64) {
        return 1;
    }

    u64 lhs_value = IntToU64(lhs);

    if (lhs_value < rhs) {
        return -1;
    }
    if (lhs_value > rhs) {
        return 1;
    }

    return 0;
}

int IntCompareI64(Int *lhs, i64 rhs) {
    ValidateInt(lhs);

    if (rhs < 0) {
        return 1;
    }

    return IntCompareU64(lhs, (u64)rhs);
}

void IntShiftLeft(Int *value, u64 positions) {
    ValidateInt(value);

    u64 bits = IntBitLength(value);

    if (positions == 0) {
        BitVecResize(INT_BITS(value), bits);
        return;
    }
    if (bits == 0) {
        IntClear(value);
        return;
    }

    BitVecResize(INT_BITS(value), bits + positions);

    for (u64 i = bits; i > 0; i--) {
        BitVecSet(INT_BITS(value), i - 1 + positions, BitVecGet(INT_BITS(value), i - 1));
    }

    for (u64 i = 0; i < positions; i++) {
        BitVecSet(INT_BITS(value), i, false);
    }
}

void IntShiftRight(Int *value, u64 positions) {
    ValidateInt(value);

    u64 bits = IntBitLength(value);

    if (positions == 0) {
        BitVecResize(INT_BITS(value), bits);
        return;
    }
    if (bits == 0 || positions >= bits) {
        IntClear(value);
        return;
    }

    for (u64 i = 0; i + positions < bits; i++) {
        BitVecSet(INT_BITS(value), i, BitVecGet(INT_BITS(value), i + positions));
    }

    BitVecResize(INT_BITS(value), bits - positions);
    int_normalize(value);
}

void(IntAdd)(Int *result, Int *a, Int *b) {
    ValidateInt(result);
    ValidateInt(a);
    ValidateInt(b);

    u64 a_bits   = IntBitLength(a);
    u64 b_bits   = IntBitLength(b);
    u64 max_bits = MAX2(a_bits, b_bits);
    Int temp     = int_init_with_capacity(max_bits + 1);
    bool carry   = false;

    for (u64 i = 0; i < max_bits; i++) {
        u64 sum = carry ? 1u : 0u;

        if (i < a_bits && BitVecGet(INT_BITS(a), i)) {
            sum++;
        }
        if (i < b_bits && BitVecGet(INT_BITS(b), i)) {
            sum++;
        }

        BitVecPush(INT_BITS(&temp), (sum & 1u) != 0u);
        carry = sum >= 2u;
    }

    if (carry) {
        BitVecPush(INT_BITS(&temp), true);
    }

    int_normalize(&temp);
    IntDeinit(result);
    *result = temp;
}

void IntAddU64(Int *result, Int *value, u64 addend) {
    ValidateInt(result);
    ValidateInt(value);

    Int temp = IntClone(value);

    int_add_u64_in_place(&temp, addend);
    int_replace(result, &temp);
}

void IntAddI64(Int *result, Int *value, i64 addend) {
    u64 magnitude = int_i64_magnitude(addend);

    ValidateInt(result);
    ValidateInt(value);

    if (addend >= 0) {
        IntAddU64(result, value, magnitude);
        return;
    }

    if (!IntSubU64(result, value, magnitude)) {
        LOG_FATAL("IntAdd would produce a negative result");
    }
}

bool(IntSub)(Int *result, Int *a, Int *b) {
    ValidateInt(result);
    ValidateInt(a);
    ValidateInt(b);

    if (IntCompare(a, b) < 0) {
        return false;
    }

    u64 a_bits = IntBitLength(a);
    u64 b_bits = IntBitLength(b);
    Int temp   = int_init_with_capacity(a_bits);
    bool borrow = false;

    for (u64 i = 0; i < a_bits; i++) {
        int ai   = (i < a_bits && BitVecGet(INT_BITS(a), i)) ? 1 : 0;
        int bi   = (i < b_bits && BitVecGet(INT_BITS(b), i)) ? 1 : 0;
        int diff = ai - bi - (borrow ? 1 : 0);

        if (diff < 0) {
            diff += 2;
            borrow = true;
        } else {
            borrow = false;
        }

        BitVecPush(INT_BITS(&temp), diff != 0);
    }

    int_normalize(&temp);
    IntDeinit(result);
    *result = temp;
    return true;
}

bool IntSubU64(Int *result, Int *value, u64 subtrahend) {
    ValidateInt(result);
    ValidateInt(value);

    Int rhs = IntFromU64(subtrahend);
    bool ok = IntSub(result, value, &rhs);

    IntDeinit(&rhs);
    return ok;
}

bool IntSubI64(Int *result, Int *value, i64 subtrahend) {
    u64 magnitude = int_i64_magnitude(subtrahend);

    ValidateInt(result);
    ValidateInt(value);

    if (subtrahend >= 0) {
        return IntSubU64(result, value, magnitude);
    }

    IntAddU64(result, value, magnitude);
    return true;
}

void IntMul(Int *result, Int *a, Int *b) {
    ValidateInt(result);
    ValidateInt(a);
    ValidateInt(b);

    u64 b_bits = IntBitLength(b);
    Int acc    = IntInit();

    if (IntIsZero(a) || IntIsZero(b)) {
        IntDeinit(result);
        *result = acc;
        return;
    }

    for (u64 i = 0; i < b_bits; i++) {
        if (!BitVecGet(INT_BITS(b), i)) {
            continue;
        }

        Int partial = IntClone(a);
        Int next    = IntInit();

        int_normalize(&partial);
        IntShiftLeft(&partial, i);
        IntAdd(&next, &acc, &partial);

        IntDeinit(&acc);
        acc = next;

        IntDeinit(&partial);
    }

    int_normalize(&acc);
    IntDeinit(result);
    *result = acc;
}

void IntMulU64(Int *result, Int *value, u64 factor) {
    ValidateInt(result);
    ValidateInt(value);

    Int temp = IntClone(value);

    int_mul_u64_in_place(&temp, factor);
    int_replace(result, &temp);
}

void IntSquare(Int *result, Int *value) {
    IntMul(result, value, value);
}

void IntPowU64(Int *result, Int *base, u64 exponent) {
    ValidateInt(result);
    ValidateInt(base);

    Int acc      = IntFromU64(1);
    Int current  = IntClone(base);

    while (exponent > 0) {
        if (exponent & 1u) {
            Int next = IntInit();

            IntMul(&next, &acc, &current);
            IntDeinit(&acc);
            acc = next;
        }

        exponent >>= 1u;
        if (exponent > 0) {
            Int next = IntInit();

            IntSquare(&next, &current);
            IntDeinit(&current);
            current = next;
        }
    }

    IntDeinit(&current);
    int_replace(result, &acc);
}

void IntDivMod(Int *quotient, Int *remainder, Int *dividend, Int *divisor) {
    ValidateInt(quotient);
    ValidateInt(remainder);
    ValidateInt(dividend);
    ValidateInt(divisor);

    if (quotient == remainder) {
        LOG_FATAL("quotient and remainder must be different objects");
    }
    if (IntIsZero(divisor)) {
        LOG_FATAL("Division by zero");
    }

    Int normalized_dividend = IntClone(dividend);
    Int normalized_divisor  = IntClone(divisor);
    Int q                   = IntInit();
    Int r                   = IntClone(&normalized_dividend);

    if (IntCompare(&normalized_dividend, &normalized_divisor) >= 0) {
        u64 dividend_bits = IntBitLength(&normalized_dividend);
        u64 divisor_bits  = IntBitLength(&normalized_divisor);

        q = int_init_with_capacity(dividend_bits - divisor_bits + 1);

        for (u64 shift = dividend_bits - divisor_bits + 1; shift > 0; shift--) {
            u64 bit = shift - 1;
            Int shifted = IntClone(&normalized_divisor);

            IntShiftLeft(&shifted, bit);

            if (IntGE(&r, &shifted)) {
                Int next = IntInit();

                (void)IntSub(&next, &r, &shifted);
                IntDeinit(&r);
                r = next;

                if (IntBitLength(&q) < bit + 1) {
                    BitVecResize(INT_BITS(&q), bit + 1);
                }
                BitVecSet(INT_BITS(&q), bit, true);
            }

            IntDeinit(&shifted);
        }
    } else {
        IntDeinit(&q);
        q = IntInit();
    }

    int_normalize(&q);
    int_normalize(&r);

    IntDeinit(&normalized_dividend);
    IntDeinit(&normalized_divisor);
    int_replace(quotient, &q);
    int_replace(remainder, &r);
}

void IntDiv(Int *result, Int *dividend, Int *divisor) {
    Int quotient  = IntInit();
    Int remainder = IntInit();

    IntDivMod(&quotient, &remainder, dividend, divisor);
    IntDeinit(&remainder);
    int_replace(result, &quotient);
}

bool IntDivExact(Int *result, Int *dividend, Int *divisor) {
    ValidateInt(result);
    ValidateInt(dividend);
    ValidateInt(divisor);

    if (IntIsZero(divisor)) {
        LOG_FATAL("Division by zero");
    }

    Int quotient  = IntInit();
    Int remainder = IntInit();

    IntDivMod(&quotient, &remainder, dividend, divisor);
    if (!IntIsZero(&remainder)) {
        IntDeinit(&quotient);
        IntDeinit(&remainder);
        return false;
    }

    IntDeinit(&remainder);
    int_replace(result, &quotient);
    return true;
}

u64 IntDivU64Rem(Int *quotient, Int *dividend, u64 divisor) {
    ValidateInt(quotient);
    ValidateInt(dividend);

    if (divisor == 0) {
        LOG_FATAL("Division by zero");
    }

    Int divisor_value = IntFromU64(divisor);
    Int remainder     = IntInit();
    u64 rem           = 0;

    IntDivMod(quotient, &remainder, dividend, &divisor_value);
    rem = IntToU64(&remainder);

    IntDeinit(&divisor_value);
    IntDeinit(&remainder);
    return rem;
}

void IntMod(Int *result, Int *dividend, Int *divisor) {
    Int quotient  = IntInit();
    Int remainder = IntInit();

    IntDivMod(&quotient, &remainder, dividend, divisor);
    IntDeinit(&quotient);
    int_replace(result, &remainder);
}

u64 IntModU64(Int *value, u64 modulus) {
    ValidateInt(value);

    if (modulus == 0) {
        LOG_FATAL("modulus is zero");
    }

    Int quotient = IntInit();
    u64 rem      = IntDivU64Rem(&quotient, value, modulus);

    IntDeinit(&quotient);
    return rem;
}

void IntGCD(Int *result, Int *a, Int *b) {
    ValidateInt(result);
    ValidateInt(a);
    ValidateInt(b);

    Int x = IntClone(a);
    Int y = IntClone(b);

    while (!IntIsZero(&y)) {
        Int r = IntInit();

        IntMod(&r, &x, &y);
        IntDeinit(&x);
        x = y;
        y = r;
    }

    int_replace(result, &x);
    IntDeinit(&y);
}

void IntLCM(Int *result, Int *a, Int *b) {
    ValidateInt(result);
    ValidateInt(a);
    ValidateInt(b);

    if (IntIsZero(a) || IntIsZero(b)) {
        Int zero = IntInit();
        int_replace(result, &zero);
        return;
    }

    Int gcd      = IntInit();
    Int quotient = IntInit();
    Int lcm      = IntInit();

    IntGCD(&gcd, a, b);
    IntDiv(&quotient, a, &gcd);
    IntMul(&lcm, &quotient, b);

    IntDeinit(&gcd);
    IntDeinit(&quotient);
    int_replace(result, &lcm);
}

void IntRootRem(Int *root, Int *remainder, Int *value, u64 degree) {
    ValidateInt(root);
    ValidateInt(remainder);
    ValidateInt(value);

    if (root == remainder) {
        LOG_FATAL("root and remainder must be different objects");
    }
    if (degree == 0) {
        LOG_FATAL("root degree is zero");
    }

    if (IntIsZero(value)) {
        Int zero_root = IntInit();
        Int zero_rem  = IntInit();

        int_replace(root, &zero_root);
        int_replace(remainder, &zero_rem);
        return;
    }
    if (degree == 1) {
        Int exact_root = IntClone(value);
        Int zero_rem   = IntInit();

        int_replace(root, &exact_root);
        int_replace(remainder, &zero_rem);
        return;
    }

    u64 bits       = IntBitLength(value);
    u64 high_shift = bits / degree;
    Int low        = IntInit();
    Int high       = IntFromU64(1);
    Int best       = IntInit();
    Int one        = IntFromU64(1);

    if ((bits % degree) != 0) {
        high_shift++;
    }
    if (high_shift == 0) {
        high_shift = 1;
    }

    IntShiftLeft(&high, high_shift);

    while (IntLE(&low, &high)) {
        Int sum     = IntInit();
        Int mid     = IntInit();
        Int mid_pow = IntInit();
        int cmp     = 0;

        IntAdd(&sum, &low, &high);
        IntShiftRight(&sum, 1);
        mid = sum;

        IntPowU64(&mid_pow, &mid, degree);
        cmp = IntCompare(&mid_pow, value);

        if (cmp <= 0) {
            Int next = IntInit();

            IntDeinit(&best);
            best = IntClone(&mid);
            IntAdd(&next, &mid, &one);
            IntDeinit(&low);
            low = next;
        } else {
            Int next = IntInit();

            if (IntEQ(&mid, &one) || IntIsZero(&mid)) {
                IntDeinit(&high);
                high = IntInit();
            } else {
                (void)IntSub(&next, &mid, &one);
                IntDeinit(&high);
                high = next;
            }
        }

        IntDeinit(&mid_pow);
        IntDeinit(&mid);
    }

    {
        Int power = IntInit();
        Int rem   = IntInit();

        IntPowU64(&power, &best, degree);
        (void)IntSub(&rem, value, &power);

        IntDeinit(&power);
        IntDeinit(&low);
        IntDeinit(&high);
        IntDeinit(&one);

        int_replace(root, &best);
        int_replace(remainder, &rem);
    }
}

void IntRoot(Int *result, Int *value, u64 degree) {
    Int root      = IntInit();
    Int remainder = IntInit();

    IntRootRem(&root, &remainder, value, degree);
    IntDeinit(&remainder);
    int_replace(result, &root);
}

void IntSqrtRem(Int *root, Int *remainder, Int *value) {
    IntRootRem(root, remainder, value, 2);
}

void IntSqrt(Int *result, Int *value) {
    IntRoot(result, value, 2);
}

bool IntIsPerfectSquare(Int *value) {
    ValidateInt(value);

    Int root      = IntInit();
    Int remainder = IntInit();
    bool result   = false;

    IntSqrtRem(&root, &remainder, value);
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

    u64 max_degree = IntLog2(value);

    for (u64 degree = 2; degree <= max_degree; degree++) {
        Int root      = IntInit();
        Int remainder = IntInit();
        bool exact    = false;

        IntRootRem(&root, &remainder, value, degree);
        exact = IntIsZero(&remainder);

        IntDeinit(&root);
        IntDeinit(&remainder);

        if (exact) {
            return true;
        }
    }

    return false;
}

int IntJacobi(Int *a, Int *n) {
    ValidateInt(a);
    ValidateInt(n);

    if (IntIsZero(n) || IntIsEven(n)) {
        LOG_FATAL("n must be non-zero and odd");
    }

    Int aa = IntInit();
    Int nn = IntClone(n);
    int result = 1;

    IntMod(&aa, a, &nn);

    while (!IntIsZero(&aa)) {
        while (IntIsEven(&aa)) {
            u64 n_mod_8 = 0;

            IntShiftRight(&aa, 1);
            n_mod_8 = IntModU64(&nn, 8);
            if (n_mod_8 == 3 || n_mod_8 == 5) {
                result = -result;
            }
        }

        int_swap(&aa, &nn);

        if (IntModU64(&aa, 4) == 3 && IntModU64(&nn, 4) == 3) {
            result = -result;
        }

        IntMod(&aa, &aa, &nn);
    }

    IntDeinit(&aa);
    if (IntCompareU64(&nn, 1) != 0) {
        IntDeinit(&nn);
        return 0;
    }

    IntDeinit(&nn);
    return result;
}

void IntModAdd(Int *result, Int *a, Int *b, Int *modulus) {
    ValidateInt(result);
    ValidateInt(a);
    ValidateInt(b);
    ValidateInt(modulus);

    if (IntIsZero(modulus)) {
        LOG_FATAL("modulus is zero");
    }

    Int ar  = IntInit();
    Int br  = IntInit();
    Int sum = IntInit();

    IntMod(&ar, a, modulus);
    IntMod(&br, b, modulus);
    IntAdd(&sum, &ar, &br);
    IntMod(result, &sum, modulus);

    IntDeinit(&ar);
    IntDeinit(&br);
    IntDeinit(&sum);
}

void IntModSub(Int *result, Int *a, Int *b, Int *modulus) {
    ValidateInt(result);
    ValidateInt(a);
    ValidateInt(b);
    ValidateInt(modulus);

    if (IntIsZero(modulus)) {
        LOG_FATAL("modulus is zero");
    }

    Int ar = IntInit();
    Int br = IntInit();

    IntMod(&ar, a, modulus);
    IntMod(&br, b, modulus);

    if (IntGE(&ar, &br)) {
        (void)IntSub(result, &ar, &br);
    } else {
        Int diff = IntInit();

        (void)IntSub(&diff, &br, &ar);
        if (IntIsZero(&diff)) {
            Int zero = IntInit();
            int_replace(result, &zero);
        } else {
            (void)IntSub(result, modulus, &diff);
        }

        IntDeinit(&diff);
    }

    IntDeinit(&ar);
    IntDeinit(&br);
}

void IntModMul(Int *result, Int *a, Int *b, Int *modulus) {
    ValidateInt(result);
    ValidateInt(a);
    ValidateInt(b);
    ValidateInt(modulus);

    if (IntIsZero(modulus)) {
        LOG_FATAL("modulus is zero");
    }

    Int ar   = IntInit();
    Int br   = IntInit();
    Int prod = IntInit();

    IntMod(&ar, a, modulus);
    IntMod(&br, b, modulus);
    IntMul(&prod, &ar, &br);
    IntMod(result, &prod, modulus);

    IntDeinit(&ar);
    IntDeinit(&br);
    IntDeinit(&prod);
}

bool IntModDiv(Int *result, Int *a, Int *b, Int *modulus) {
    ValidateInt(result);
    ValidateInt(a);
    ValidateInt(b);
    ValidateInt(modulus);

    if (IntIsZero(modulus)) {
        LOG_FATAL("modulus is zero");
    }

    Int inverse = IntInit();
    Int value   = IntInit();
    bool ok     = false;

    ok = IntModInv(&inverse, b, modulus);
    if (!ok) {
        IntDeinit(&inverse);
        IntDeinit(&value);
        return false;
    }

    IntModMul(&value, a, &inverse, modulus);

    IntDeinit(&inverse);
    int_replace(result, &value);
    return true;
}

void IntSquareMod(Int *result, Int *value, Int *modulus) {
    IntModMul(result, value, value, modulus);
}

void IntPowU64Mod(Int *result, Int *base, u64 exponent, Int *modulus) {
    ValidateInt(result);
    ValidateInt(base);
    ValidateInt(modulus);

    if (IntIsZero(modulus)) {
        LOG_FATAL("modulus is zero");
    }

    Int acc      = IntFromU64(1);
    Int base_mod = IntInit();

    IntMod(&acc, &acc, modulus);
    IntMod(&base_mod, base, modulus);

    while (exponent > 0) {
        if (exponent & 1u) {
            Int next = IntInit();

            IntModMul(&next, &acc, &base_mod, modulus);
            IntDeinit(&acc);
            acc = next;
        }

        exponent >>= 1u;
        if (exponent > 0) {
            Int next = IntInit();

            IntModMul(&next, &base_mod, &base_mod, modulus);
            IntDeinit(&base_mod);
            base_mod = next;
        }
    }

    IntDeinit(&base_mod);
    int_replace(result, &acc);
}

void IntModPow(Int *result, Int *base, Int *exponent, Int *modulus) {
    ValidateInt(result);
    ValidateInt(base);
    ValidateInt(exponent);
    ValidateInt(modulus);

    if (IntIsZero(modulus)) {
        LOG_FATAL("modulus is zero");
    }

    Int acc      = IntFromU64(1);
    Int base_mod = IntInit();
    Int exp      = IntClone(exponent);

    IntMod(&acc, &acc, modulus);
    IntMod(&base_mod, base, modulus);

    while (!IntIsZero(&exp)) {
        if (int_is_odd(&exp)) {
            Int next = IntInit();

            IntModMul(&next, &acc, &base_mod, modulus);
            IntDeinit(&acc);
            acc = next;
        }

        IntShiftRight(&exp, 1);
        if (!IntIsZero(&exp)) {
            Int next = IntInit();

            IntModMul(&next, &base_mod, &base_mod, modulus);
            IntDeinit(&base_mod);
            base_mod = next;
        }
    }

    IntDeinit(&exp);
    IntDeinit(&base_mod);
    int_replace(result, &acc);
}

bool IntModInv(Int *result, Int *value, Int *modulus) {
    ValidateInt(result);
    ValidateInt(value);
    ValidateInt(modulus);

    if (IntIsZero(modulus)) {
        LOG_FATAL("modulus is zero");
    }

    Int       reduced = IntInit();
    SignedInt t       = sint_init();
    SignedInt new_t   = sint_from_u64(1);
    Int       r       = IntClone(modulus);
    Int       new_r   = IntInit();
    Int       one     = IntFromU64(1);
    bool      ok      = false;

    IntMod(&reduced, value, modulus);
    new_r = IntClone(&reduced);

    while (!IntIsZero(&new_r)) {
        Int       q        = IntInit();
        Int       rem      = IntInit();
        SignedInt q_new_t  = sint_init();
        SignedInt next_t   = sint_init();
        Int       next_r   = IntInit();

        IntDivMod(&q, &rem, &r, &new_r);
        sint_mul_unsigned(&q_new_t, &new_t, &q);
        sint_sub(&next_t, &t, &q_new_t);

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
        Int positive = IntInit();
        Int mag_mod   = IntInit();

        IntMod(&mag_mod, &t.magnitude, modulus);
        if (t.negative && !IntIsZero(&mag_mod)) {
            (void)IntSub(&positive, modulus, &mag_mod);
        } else {
            IntMod(&positive, &t.magnitude, modulus);
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
        LOG_FATAL("modulus is zero");
    }

    Int a = IntInit();
    bool ok = false;

    IntMod(&a, value, modulus);

    if (IntIsZero(&a)) {
        Int zero = IntInit();
        int_replace(result, &zero);
        IntDeinit(&a);
        return true;
    }
    if (IntCompareU64(modulus, 2) == 0) {
        int_replace(result, &a);
        return true;
    }
    if (IntIsEven(modulus) || !IntIsProbablePrime(modulus)) {
        IntDeinit(&a);
        return false;
    }
    if (IntJacobi(&a, modulus) != 1) {
        IntDeinit(&a);
        return false;
    }
    if (IntModU64(modulus, 4) == 3) {
        Int exponent = IntClone(modulus);
        Int root     = IntInit();

        IntAddU64(&exponent, &exponent, 1);
        IntShiftRight(&exponent, 2);
        IntModPow(&root, &a, &exponent, modulus);

        IntDeinit(&exponent);
        IntDeinit(&a);
        int_replace(result, &root);
        return true;
    }

    {
        Int q       = IntClone(modulus);
        Int z       = IntFromU64(2);
        Int c       = IntInit();
        Int t       = IntInit();
        Int r       = IntInit();
        Int exponent = IntInit();
        u64 m       = 0;

        (void)IntSubU64(&q, &q, 1);
        while (IntIsEven(&q)) {
            IntShiftRight(&q, 1);
            m++;
        }

        while (IntJacobi(&z, modulus) != -1) {
            IntAddU64(&z, &z, 1);
        }

        IntModPow(&c, &z, &q, modulus);
        IntModPow(&t, &a, &q, modulus);

        exponent = IntClone(&q);
        IntAddU64(&exponent, &exponent, 1);
        IntShiftRight(&exponent, 1);
        IntModPow(&r, &a, &exponent, modulus);

        while (IntCompareU64(&t, 1) != 0) {
            Int t_power = IntClone(&t);
            u64 i       = 0;

            for (i = 1; i < m; i++) {
                Int next = IntInit();

                IntSquareMod(&next, &t_power, modulus);
                IntDeinit(&t_power);
                t_power = next;

                if (IntCompareU64(&t_power, 1) == 0) {
                    break;
                }
            }

            if (i == m) {
                IntDeinit(&t_power);
                break;
            }

            {
                Int b    = IntClone(&c);
                Int b_sq = IntInit();
                Int next = IntInit();

                for (u64 j = 0; j + i + 1 < m; j++) {
                    Int square = IntInit();

                    IntSquareMod(&square, &b, modulus);
                    IntDeinit(&b);
                    b = square;
                }

                IntModMul(&next, &r, &b, modulus);
                IntDeinit(&r);
                r = next;

                IntSquareMod(&b_sq, &b, modulus);
                next = IntInit();
                IntModMul(&next, &t, &b_sq, modulus);
                IntDeinit(&t);
                t = next;

                IntDeinit(&c);
                c = b_sq;
                IntDeinit(&b);
            }

            IntDeinit(&t_power);
            m = i;
        }

        ok = IntCompareU64(&t, 1) == 0;
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

bool IntIsProbablePrime(Int *value) {
    static const u64 bases[] = {2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37};

    ValidateInt(value);

    if (IntCompareU64(value, 2) < 0) {
        return false;
    }
    if (IntCompareU64(value, 2) == 0) {
        return true;
    }
    if (IntIsEven(value)) {
        return false;
    }

    for (u64 i = 0; i < (u64)(sizeof(bases) / sizeof(bases[0])); i++) {
        if (IntCompareU64(value, bases[i]) == 0) {
            return true;
        }
        if (IntModU64(value, bases[i]) == 0) {
            return false;
        }
    }

    {
        Int d           = IntClone(value);
        Int n_minus_one = IntInit();
        u64 s           = 0;
        bool probable   = true;

        (void)IntSubU64(&d, &d, 1);
        n_minus_one = IntClone(&d);

        while (IntIsEven(&d)) {
            IntShiftRight(&d, 1);
            s++;
        }

        for (u64 i = 0; i < (u64)(sizeof(bases) / sizeof(bases[0])); i++) {
            Int base = IntFromU64(bases[i]);
            Int x    = IntInit();

            if (IntCompare(&base, value) >= 0) {
                IntDeinit(&base);
                IntDeinit(&x);
                continue;
            }

            IntModPow(&x, &base, &d, value);
            if ((IntCompareU64(&x, 1) == 0) || IntEQ(&x, &n_minus_one)) {
                IntDeinit(&base);
                IntDeinit(&x);
                continue;
            }

            {
                bool witness = true;

                for (u64 r = 1; r < s; r++) {
                    Int next = IntInit();

                    IntSquareMod(&next, &x, value);
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

void IntNextPrime(Int *result, Int *value) {
    ValidateInt(result);
    ValidateInt(value);

    if (IntCompareU64(value, 1) <= 0) {
        Int two = IntFromU64(2);
        int_replace(result, &two);
        return;
    }

    Int candidate = IntClone(value);

    IntAddU64(&candidate, &candidate, 1);
    if (IntCompareU64(&candidate, 2) <= 0) {
        Int two = IntFromU64(2);
        IntDeinit(&candidate);
        int_replace(result, &two);
        return;
    }
    if (IntIsEven(&candidate)) {
        IntAddU64(&candidate, &candidate, 1);
    }

    while (!IntIsProbablePrime(&candidate)) {
        IntAddU64(&candidate, &candidate, 2);
    }

    int_replace(result, &candidate);
}
