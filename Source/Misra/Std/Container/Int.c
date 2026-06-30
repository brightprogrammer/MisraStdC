/// file      : std/container/int.c
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Arbitrary-precision unsigned integer implementation built on top of BitVec.

#include <Misra/Std/Container/Int.h>
#include <Misra/Std/Zstr.h>
#include <Misra/Std/Container/Int/Private.h>
#include <Misra/Std/Container/BitVec.h>
#include <Misra/Std/Log.h>

#if defined(_MSC_VER) && !defined(__clang__) && (defined(_M_X64) || defined(_M_AMD64))
#    include <immintrin.h>
#endif

typedef struct {
    bool negative;
    Int  magnitude;
} SignedInt;

#define INT_BITS(value) (&(value)->bits)

static void        int_normalize(Int *value);
static inline u64  int_load_le8(const u8 *data, u64 bit_len, u64 off);
static inline void int_store_le8(u8 *data, u64 byte_len, u64 off, u64 value);
static bool        int_validate_radix(u8 radix);
static bool int_try_from_str_radix_impl(Int *out, Zstr digits, u64 length, u64 start, u8 radix, bool allow_underscores);
static bool int_try_from_i64_with_allocator(Int *out, i64 value, Allocator *alloc);
static bool int_try_clone_value(Int *out, const Int *value);
static u64  int_u64_bits(u64 value);

bool int_try_from_u64(Int *out, u64 value, Allocator *alloc) {
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

    return int_try_from_u64(out, (u64)value, alloc);
}

static u64 int_significant_bits(const Int *value) {
    ValidateInt(value);

    u64 len = BitVecLen(INT_BITS(value));

    if (len == 0) {
        return 0;
    }

    // Skip whole zero limbs from the top in 64-bit strides, then locate the
    // highest set bit inside the top non-zero limb. int_load_le8 masks the top
    // limb to `len`, so stale bits above the length never leak in.
    const u8 *d   = BitVecData(INT_BITS(value));
    u64       off = ((len - 1u) / 64u) * 8u;

    for (;;) {
        u64 v = int_load_le8(d, len, off);

        if (v != 0) {
            // Highest set bit within the limb by binary search (floor(log2 v)).
            u64 hi = 0;

            if (v >> 32) {
                hi  += 32;
                v  >>= 32;
            }
            if (v >> 16) {
                hi  += 16;
                v  >>= 16;
            }
            if (v >> 8) {
                hi  += 8;
                v  >>= 8;
            }
            if (v >> 4) {
                hi  += 4;
                v  >>= 4;
            }
            if (v >> 2) {
                hi  += 2;
                v  >>= 2;
            }
            hi += v >> 1;
            return off * 8u + hi + 1u;
        }
        if (off == 0) {
            break;
        }
        off -= 8u;
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
    SignedInt temp = sint_init(IntAllocator(&result->magnitude));

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
    SignedInt temp = sint_init(IntAllocator(&result->magnitude));

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

static bool int_is_odd(const Int *value) {
    ValidateInt(value);
    return BitVecLen(INT_BITS(value)) > 0 && BitVecGet(INT_BITS(value), 0);
}

static bool int_is_one(const Int *value) {
    ValidateInt(value);
    return IntBitLength(value) == 1 && BitVecGet(INT_BITS(value), 0);
}

static bool int_mul_u64_in_place(Int *value, u64 factor) {
    Int lhs;
    Int rhs;
    Int result = IntInit(IntAllocator(value));

    if (!int_try_clone_value(&lhs, value)) {
        return false;
    }
    if (!int_try_from_u64(&rhs, factor, IntAllocator(value))) {
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
    Int result = IntInit(IntAllocator(value));

    if (!int_try_clone_value(&lhs, value)) {
        return false;
    }
    if (!int_try_from_u64(&rhs, addend, IntAllocator(value))) {
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
    int_try_from_str_radix_impl(Int *out, Zstr digits, u64 length, u64 start, u8 radix, bool allow_underscores) {
    Int  result;
    bool saw_digit = false;

    if (!out || !digits) {
        LOG_FATAL("Invalid arguments");
    }

    ValidateInt(out);
    result = IntInit(IntAllocator(out));

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

u64 IntBitLength(const Int *value) {
    return int_significant_bits(value);
}

u64 IntByteLength(const Int *value) {
    u64 bits = IntBitLength(value);
    return bits == 0 ? 0 : CEIL_DIV(bits, 8u);
}

bool IntTryLog2(const Int *value, u64 *out) {
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

u64 IntLog2WithError(const Int *value, bool *error) {
    u64  out = 0;
    bool ok  = IntTryLog2(value, &out);

    if (error) {
        *error = !ok;
    }

    return out;
}

u64 IntTrailingZeroCount(const Int *value) {
    ValidateInt(value);

    for (u64 i = 0; i < BitVecLen(INT_BITS(value)); i++) {
        if (BitVecGet(INT_BITS(value), i)) {
            return i;
        }
    }

    return 0;
}

bool IntIsZero(const Int *value) {
    return IntBitLength(value) == 0;
}

bool IntIsOne(const Int *value) {
    return int_is_one(value);
}

bool IntIsEven(const Int *value) {
    ValidateInt(value);
    return !int_is_odd(value);
}

bool IntIsOdd(const Int *value) {
    return int_is_odd(value);
}

bool IntFitsU64(const Int *value) {
    ValidateInt(value);
    return IntBitLength(value) <= 64;
}

bool IntIsPowerOfTwo(const Int *value) {
    ValidateInt(value);

    return !IntIsZero(value) && IntBitLength(value) == IntTrailingZeroCount(value) + 1;
}

static bool int_try_clone_value(Int *out, const Int *value) {
    if (!out || !value) {
        LOG_FATAL("Invalid arguments");
    }

    ValidateInt(value);
    *out = IntInit(IntAllocator(value));
    if (!BitVecTryClone(INT_BITS(out), INT_BITS(value))) {
        return false;
    }

    int_normalize(out);
    return true;
}

bool IntTryClone(Int *out, const Int *value) {
    return int_try_clone_value(out, value);
}

Int IntClone(const Int *value) {
    Int clone;

    ValidateInt(value);
    clone = IntInit(IntAllocator(value));
    (void)int_try_clone_value(&clone, value);
    return clone;
}

Int int_from_u64(u64 value, Allocator *alloc) {
    Int result = IntInit(alloc);

    (void)int_try_from_u64(&result, value, alloc);
    return result;
}

Int int_from_i64(i64 value, Allocator *alloc) {
    if (value < 0) {
        LOG_FATAL("Int cannot represent negative values");
    }

    return int_from_u64((u64)value, alloc);
}

bool IntTryToU64(const Int *value, u64 *out) {
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

u64 IntToU64WithError(const Int *value, bool *error) {
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

u64 IntToBytesLE(const Int *value, u8 *bytes, u64 max_len) {
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

            if (bit_idx < BitVecLen(INT_BITS(value)) && BitVecGet(INT_BITS(value), bit_idx)) {
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

u64 IntToBytesBE(const Int *value, u8 *bytes, u64 max_len) {
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

            if (bit_idx < BitVecLen(INT_BITS(value)) && BitVecGet(INT_BITS(value), bit_idx)) {
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

    if (StrLen(decimal) > 0 && StrCharAt(decimal, 0) == '+') {
        start = 1;
    }

    return int_try_from_str_radix_impl(out, StrBegin(decimal), StrLen(decimal), start, 10, true);
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

bool int_try_to_str(Str *out, const Int *value, Allocator *alloc) {
    return int_try_to_str_radix(out, value, 10, false, alloc);
}

Str int_to_str(const Int *value, Allocator *alloc) {
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
    if (StrLen(digits) > 0 && StrCharAt(digits, 0) == '+') {
        start = 1;
    }

    return int_try_from_str_radix_impl(out, StrBegin(digits), StrLen(digits), start, radix, true);
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

bool int_try_to_str_radix(Str *out, const Int *value, u8 radix, bool uppercase, Allocator *alloc) {
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
        return StrPushBackR(out, '0');
    }

    current = IntClone(value);
    if (IntIsZero(&current)) {
        return false;
    }

    result = StrInit(alloc);

    // Extract digits in the largest radix^k chunk that still fits in a u64, so
    // each big-integer division yields k digits at once. Splitting those k
    // digits is plain u64 arithmetic (no BitVec ops, no validation), so the
    // count of validated big-integer divisions drops by a factor of k.
    u64 chunk        = 1;
    u32 chunk_digits = 0;

    while (chunk <= UINT64_MAX / radix) {
        chunk *= radix;
        chunk_digits++;
    }

    // Hoist the per-chunk allocations out of the loop: build the chunk divisor
    // once and reuse the quotient/remainder buffers (the quotient reserved once
    // to the value's width) instead of allocating a fresh Int per digit-chunk.
    Int chunk_divisor = IntInit(alloc);
    Int quotient      = IntInit(alloc);
    Int remainder     = IntInit(alloc);

    if (!int_try_from_u64(&chunk_divisor, chunk, alloc) || !IntReserve(&quotient, IntBitLength(value))) {
        IntDeinit(&chunk_divisor);
        IntDeinit(&quotient);
        IntDeinit(&remainder);
        IntDeinit(&current);
        StrDeinit(&result);
        return false;
    }

    while (!IntIsZero(&current)) {
        u64  rem  = 0;
        bool last = false;

        if (!int_div_mod(&quotient, &remainder, &current, &chunk_divisor)) {
            IntDeinit(&chunk_divisor);
            IntDeinit(&quotient);
            IntDeinit(&remainder);
            IntDeinit(&current);
            StrDeinit(&result);
            return false;
        }
        rem  = IntToU64(&remainder);
        last = IntIsZero(&quotient);

        // A non-final chunk emits exactly chunk_digits digits (zero-padded for
        // place value); the most significant chunk stops at its top digit.
        for (u32 k = 0; (k < chunk_digits) && !(last && rem == 0 && k > 0); k++) {
            if (!StrPushBackR(&result, int_radix_char((u8)(rem % radix), uppercase))) {
                IntDeinit(&chunk_divisor);
                IntDeinit(&quotient);
                IntDeinit(&remainder);
                IntDeinit(&current);
                StrDeinit(&result);
                return false;
            }
            rem /= radix;
        }

        // current <- quotient, reusing both buffers (the old current is
        // overwritten by the next division).
        {
            Int tmp  = current;
            current  = quotient;
            quotient = tmp;
        }
    }

    IntDeinit(&chunk_divisor);
    IntDeinit(&quotient);
    IntDeinit(&remainder);

    for (u64 i = 0; i < StrLen(&result) / 2; i++) {
        char *lhs = StrCharPtrAt(&result, i);
        char *rhs = StrCharPtrAt(&result, StrLen(&result) - 1 - i);
        char  tmp = *lhs;
        *lhs      = *rhs;
        *rhs      = tmp;
    }

    IntDeinit(&current);
    *out = result;
    return true;
}

Str int_to_str_radix(const Int *value, u8 radix, bool uppercase, Allocator *alloc) {
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

    if (StrLen(binary) >= 2 && StrCharAt(binary, 0) == '0' &&
        (StrCharAt(binary, 1) == 'b' || StrCharAt(binary, 1) == 'B')) {
        start = 2;
    }

    return int_try_from_str_radix_impl(out, StrBegin(binary), StrLen(binary), start, 2, true);
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

Str IntToBinary(const Int *value) {
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

    if (StrLen(octal) >= 2 && StrCharAt(octal, 0) == '0' &&
        (StrCharAt(octal, 1) == 'o' || StrCharAt(octal, 1) == 'O')) {
        start = 2;
    }

    return int_try_from_str_radix_impl(out, StrBegin(octal), StrLen(octal), start, 8, true);
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

Str IntToOctStr(const Int *value) {
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

    return int_try_from_str_radix_impl(out, StrBegin(hex), StrLen(hex), 0, 16, false);
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

Str IntToHexStr(const Int *value) {
    return IntToStrRadix(value, 16, false);
}

// FNV-1a over the significant magnitude bytes. `Int` is unsigned by
// design, so there's no sign byte to mix in.
u64 int_hash(const void *data, u32 size) {
    const Int *value = (const Int *)data;
    u64        hash  = 1469598103934665603ULL;

    (void)size;
    ValidateInt(value);

    u64       bits      = IntBitLength(value);
    u64       bytes     = bits == 0 ? 0 : CEIL_DIV(bits, 8u);
    const u8 *magnitude = (const u8 *)BitVecData(INT_BITS(value));
    for (u64 i = 0; i < bytes; i++) {
        hash ^= (u64)magnitude[i];
        hash *= 1099511628211ULL;
    }
    return hash;
}

i32 int_compare(const void *lhs, const void *rhs) {
    const Int *a = (const Int *)lhs;
    const Int *b = (const Int *)rhs;

    ValidateInt(a);
    ValidateInt(b);

    u64 a_bits = IntBitLength(a);
    u64 b_bits = IntBitLength(b);

    if (a_bits != b_bits) {
        return a_bits < b_bits ? -1 : 1;
    }
    if (a_bits == 0) {
        return 0;
    }

    const u8 *ad  = BitVecData(INT_BITS(a));
    const u8 *bd  = BitVecData(INT_BITS(b));
    u64       n   = (a_bits + 7u) / 8u;
    u64       off = ((n - 1u) / 8u) * 8u;

    for (;;) {
        u64 av = int_load_le8(ad, a_bits, off);
        u64 bv = int_load_le8(bd, a_bits, off);

        if (av != bv) {
            return av > bv ? 1 : -1;
        }
        if (off == 0) {
            break;
        }
        off -= 8u;
    }

    return 0;
}

int int_compare_u64(const Int *lhs, u64 rhs) {
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

int int_compare_i64(const Int *lhs, i64 rhs) {
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

    u64 new_bits = bits + positions;

    if (!BitVecResize(INT_BITS(value), new_bits)) {
        return false;
    }

    // Funnel shift over 64-bit limbs, high to low so each source limb is read
    // before it is overwritten: out[k] = src[k-ls] << bs | src[k-ls-1] >> (64-bs).
    u8 *d          = BitVecData(INT_BITS(value));
    u64 limb_shift = positions / 64u;
    u64 bit_shift  = positions % 64u;
    u64 n_out      = (new_bits + 63u) / 64u;
    u64 byte_len   = (new_bits + 7u) / 8u;

    for (u64 k = n_out; k > 0; k--) {
        u64 idx = k - 1u;
        u64 out = idx >= limb_shift ? int_load_le8(d, new_bits, (idx - limb_shift) * 8u) << bit_shift : 0;

        if (bit_shift != 0 && idx > limb_shift) {
            out |= int_load_le8(d, new_bits, (idx - limb_shift - 1u) * 8u) >> (64u - bit_shift);
        }
        int_store_le8(d, byte_len, idx * 8u, out);
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

    u64 new_bits = bits - positions;

    // Funnel shift over 64-bit limbs, low to high: out[k] = src[k+ls] >> bs |
    // src[k+ls+1] << (64-bs). Written limbs are below the limbs still being read.
    u8 *d          = BitVecData(INT_BITS(value));
    u64 limb_shift = positions / 64u;
    u64 bit_shift  = positions % 64u;
    u64 n_out      = (new_bits + 63u) / 64u;
    u64 byte_len   = (bits + 7u) / 8u;

    for (u64 k = 0; k < n_out; k++) {
        u64 out = int_load_le8(d, bits, (k + limb_shift) * 8u) >> bit_shift;

        if (bit_shift != 0) {
            out |= int_load_le8(d, bits, (k + limb_shift + 1u) * 8u) << (64u - bit_shift);
        }
        int_store_le8(d, byte_len, k * 8u, out);
    }

    if (!BitVecResize(INT_BITS(value), new_bits)) {
        return false;
    }
    int_normalize(value);
    return true;
}

// out = a + b + carry_in (carry_in is 0 or 1); returns the carry-out.
static inline u64 int_add_carry_u64(u64 a, u64 b, u64 carry_in, u64 *out) {
#if defined(__GNUC__) || defined(__clang__)
    u64 partial;
    u64 c1 = __builtin_add_overflow(a, b, &partial) ? 1u : 0u;
    u64 c2 = __builtin_add_overflow(partial, carry_in, out) ? 1u : 0u;
    return c1 | c2;
#elif defined(_MSC_VER) && (defined(_M_X64) || defined(_M_AMD64))
    unsigned long long r = 0;
    unsigned char      c = _addcarry_u64((unsigned char)carry_in, a, b, &r);
    *out                 = (u64)r;
    return (u64)c;
#else
    u64 sum = a + b;
    u64 c1  = sum < a ? 1u : 0u;
    *out    = sum + carry_in;
    u64 c2  = *out < sum ? 1u : 0u;
    return c1 | c2;
#endif
}

// Full 64x64 -> 128 product: returns the low 64 bits, *hi receives the high 64.
static inline u64 int_mul_wide_u64(u64 a, u64 b, u64 *hi) {
#if defined(__GNUC__) || defined(__clang__)
    __uint128_t p = (__uint128_t)a * (__uint128_t)b;
    *hi           = (u64)(p >> 64);
    return (u64)p;
#elif defined(_MSC_VER) && (defined(_M_X64) || defined(_M_AMD64))
    unsigned long long h  = 0;
    u64                lo = (u64)_umul128(a, b, &h);
    *hi                   = (u64)h;
    return lo;
#else
    u64 a_lo  = a & 0xFFFFFFFFu;
    u64 a_hi  = a >> 32u;
    u64 b_lo  = b & 0xFFFFFFFFu;
    u64 b_hi  = b >> 32u;
    u64 ll    = a_lo * b_lo;
    u64 lh    = a_lo * b_hi;
    u64 hl    = a_hi * b_lo;
    u64 hh    = a_hi * b_hi;
    u64 cross = (ll >> 32u) + (lh & 0xFFFFFFFFu) + (hl & 0xFFFFFFFFu);
    *hi       = hh + (lh >> 32u) + (hl >> 32u) + (cross >> 32u);
    return (cross << 32u) | (ll & 0xFFFFFFFFu);
#endif
}

// Read up to 8 bytes of `data` at byte offset `off` as a little-endian u64,
// then mask off any bits at or above `bit_len` (the operand's significant bit
// count) -- the top partial byte can hold stale bits above the value's length
// (a shrink-resize does not clear them). Byte order is explicit so the result
// is correct regardless of host endianness.
static inline u64 int_load_le8(const u8 *data, u64 bit_len, u64 off) {
    u64 byte_len = (bit_len + 7u) / 8u;
    u64 avail    = off < byte_len ? byte_len - off : 0;
    u64 k        = avail < 8 ? avail : 8;
    u64 v        = 0;

    if (k == 8u) {
        // Whole limb in bounds: one little-endian load instead of a byte loop.
        MemCopy(&v, data + off, 8u);
        v = FROM_LITTLE_ENDIAN8(v);
    } else {
        for (u64 j = 0; j < k; j++) {
            v |= (u64)data[off + j] << (8u * j);
        }
    }

    u64 bit_off = off * 8u;
    if (bit_off >= bit_len) {
        return 0;
    }

    u64 valid = bit_len - bit_off;
    if (valid < 64u) {
        v &= ((u64)1 << valid) - 1u;
    }
    return v;
}

static inline void int_store_le8(u8 *data, u64 byte_len, u64 off, u64 value) {
    u64 avail = off < byte_len ? byte_len - off : 0;
    u64 k     = avail < 8 ? avail : 8;

    if (k == 8u) {
        // Whole limb in bounds: one little-endian store instead of a byte loop.
        u64 le = TO_LITTLE_ENDIAN8(value);
        MemCopy(data + off, &le, 8u);
    } else {
        for (u64 j = 0; j < k; j++) {
            data[off + j] = (u8)(value >> (8u * j));
        }
    }
}

bool int_add(Int *result, const Int *a, const Int *b) {
    ValidateInt(result);
    ValidateInt(a);
    ValidateInt(b);

    u64 a_bits   = IntBitLength(a);
    u64 b_bits   = IntBitLength(b);
    u64 max_bits = MAX2(a_bits, b_bits);

    // a + b < 2^(max_bits+1), so max_bits+1 bits always hold the sum (incl. carry).
    if (!BitVecResize(INT_BITS(result), max_bits + 1)) {
        return false;
    }

    // Magnitudes are little-endian byte buffers (bit i in byte i/8), so we add
    // 64 bits per step with the hardware carry chain instead of bit-by-bit. A
    // left-to-right limb ripple keeps it safe when result aliases a or b.
    u8       *rd    = BitVecData(INT_BITS(result));
    const u8 *ad    = BitVecData(INT_BITS(a));
    const u8 *bd    = BitVecData(INT_BITS(b));
    u64       n     = (max_bits + 1u + 7u) / 8u;
    u64       carry = 0;

    for (u64 off = 0; off < n; off += 8) {
        u64 av = int_load_le8(ad, a_bits, off);
        u64 bv = int_load_le8(bd, b_bits, off);
        u64 r  = 0;

        carry = int_add_carry_u64(av, bv, carry, &r);
        int_store_le8(rd, n, off, r);
    }

    int_normalize(result);
    return true;
}

bool int_add_u64(Int *result, const Int *value, u64 addend) {
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

bool int_add_i64(Int *result, const Int *value, i64 addend) {
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

// out = a - b - borrow_in (borrow_in is 0 or 1); returns the borrow-out.
static inline u64 int_sub_borrow_u64(u64 a, u64 b, u64 borrow_in, u64 *out) {
#if defined(__GNUC__) || defined(__clang__)
    u64 partial;
    u64 b1 = __builtin_sub_overflow(a, b, &partial) ? 1u : 0u;
    u64 b2 = __builtin_sub_overflow(partial, borrow_in, out) ? 1u : 0u;
    return b1 | b2;
#elif defined(_MSC_VER) && (defined(_M_X64) || defined(_M_AMD64))
    unsigned long long r  = 0;
    unsigned char      bo = _subborrow_u64((unsigned char)borrow_in, a, b, &r);
    *out                  = (u64)r;
    return (u64)bo;
#else
    u64 d  = a - b;
    u64 b1 = a < b ? 1u : 0u;
    *out   = d - borrow_in;
    u64 b2 = d < borrow_in ? 1u : 0u;
    return b1 | b2;
#endif
}

bool int_sub(Int *result, const Int *a, const Int *b) {
    ValidateInt(result);
    ValidateInt(a);
    ValidateInt(b);

    if (int_compare(a, b) < 0) {
        return false;
    }

    u64 a_bits = IntBitLength(a);
    u64 b_bits = IntBitLength(b);

    if (!BitVecResize(INT_BITS(result), a_bits)) {
        return false;
    }

    u8       *rd     = BitVecData(INT_BITS(result));
    const u8 *ad     = BitVecData(INT_BITS(a));
    const u8 *bd     = BitVecData(INT_BITS(b));
    u64       n      = (a_bits + 7u) / 8u;
    u64       borrow = 0;

    for (u64 off = 0; off < n; off += 8) {
        u64 av = int_load_le8(ad, a_bits, off);
        u64 bv = int_load_le8(bd, b_bits, off);
        u64 r  = 0;

        borrow = int_sub_borrow_u64(av, bv, borrow, &r);
        int_store_le8(rd, n, off, r);
    }

    int_normalize(result);
    return true;
}

bool int_sub_u64(Int *result, const Int *value, u64 subtrahend) {
    ValidateInt(result);
    ValidateInt(value);

    Int  rhs;
    bool ok = false;

    if (!int_try_from_u64(&rhs, subtrahend, IntAllocator(value))) {
        return false;
    }
    ok = int_sub(result, value, &rhs);
    IntDeinit(&rhs);
    return ok;
}

bool int_sub_i64(Int *result, const Int *value, i64 subtrahend) {
    u64 magnitude = int_i64_magnitude(subtrahend);

    ValidateInt(result);
    ValidateInt(value);

    if (subtrahend >= 0) {
        return int_sub_u64(result, value, magnitude);
    }

    return int_add_u64(result, value, magnitude);
}

bool int_mul(Int *result, const Int *a, const Int *b) {
    ValidateInt(result);
    ValidateInt(a);
    ValidateInt(b);

    if (IntIsZero(a) || IntIsZero(b)) {
        IntClear(result);
        return true;
    }

    // The accumulator is built directly in result, reusing its buffer. result is
    // read back limb-by-limb during accumulation, so it must stay independent of
    // the operands: clone a/b only if they alias result.
    Int  a_copy = IntInit(IntAllocator(result));
    Int  b_copy = IntInit(IntAllocator(result));
    bool ok     = false;

    if (a == result) {
        if (!int_try_clone_value(&a_copy, a)) {
            goto cleanup;
        }
        a = &a_copy;
    }
    if (b == result) {
        if (!int_try_clone_value(&b_copy, b)) {
            goto cleanup;
        }
        b = &b_copy;
    }

    {
        u64 a_bits  = IntBitLength(a);
        u64 b_bits  = IntBitLength(b);
        u64 a_words = (a_bits + 63u) / 64u;
        u64 b_words = (b_bits + 63u) / 64u;
        u64 r_words = a_words + b_words;

        // Whole 64-bit limbs (a_words + b_words holds any product), zero-filled,
        // so every limb store is in bounds and accumulation starts clean.
        IntClear(result);
        if (!BitVecResize(INT_BITS(result), r_words * 64u)) {
            goto cleanup;
        }

        u8       *rd  = BitVecData(INT_BITS(result));
        const u8 *ad  = BitVecData(INT_BITS(a));
        const u8 *bd  = BitVecData(INT_BITS(b));
        u64       n_r = r_words * 8u;

        for (u64 ia = 0; ia < a_words; ia++) {
            u64 ai    = int_load_le8(ad, a_bits, ia * 8u);
            u64 carry = 0;

            for (u64 jb = 0; jb < b_words; jb++) {
                u64 bj  = int_load_le8(bd, b_bits, jb * 8u);
                u64 off = (ia + jb) * 8u;
                u64 hi  = 0;
                u64 lo  = int_mul_wide_u64(ai, bj, &hi);
                u64 cur = int_load_le8(rd, r_words * 64u, off);
                u64 s1  = 0;
                u64 s2  = 0;
                u64 k1  = int_add_carry_u64(cur, lo, 0, &s1);
                u64 k2  = int_add_carry_u64(s1, carry, 0, &s2);

                int_store_le8(rd, n_r, off, s2);
                carry = hi + k1 + k2;
            }

            for (u64 off = (ia + b_words) * 8u; carry != 0; off += 8u) {
                u64 cur = int_load_le8(rd, r_words * 64u, off);
                u64 t   = 0;

                carry = int_add_carry_u64(cur, carry, 0, &t);
                int_store_le8(rd, n_r, off, t);
            }
        }

        int_normalize(result);
        ok = true;
    }

cleanup:
    IntDeinit(&a_copy);
    IntDeinit(&b_copy);
    return ok;
}

bool int_mul_u64(Int *result, const Int *value, u64 factor) {
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

bool int_mul_i64(Int *result, const Int *value, i64 factor) {
    if (factor < 0) {
        LOG_FATAL("Int cannot be multiplied by a negative scalar");
    }

    return int_mul_u64(result, value, (u64)factor);
}

bool IntSquare(Int *result, const Int *value) {
    return int_mul(result, value, value);
}

bool int_pow(Int *result, const Int *base, const Int *exponent) {
    ValidateInt(result);
    ValidateInt(base);
    ValidateInt(exponent);

    if (!IntFitsU64(exponent)) {
        LOG_ERROR("Int exponent exceeds u64 range");
        return false;
    }

    return int_pow_u64(result, base, IntToU64(exponent));
}

bool int_pow_u64(Int *result, const Int *base, u64 exponent) {
    ValidateInt(result);
    ValidateInt(base);

    Int acc;
    Int current;

    if (!int_try_from_u64(&acc, 1, IntAllocator(result))) {
        return false;
    }
    if (!int_try_clone_value(&current, base)) {
        IntDeinit(&acc);
        return false;
    }

    // One scratch buffer, reserved once and reused each iteration via swap: with
    // int_mul writing in place, scratch keeps its capacity across iterations
    // instead of a fresh Init per multiply.
    Int scratch = IntInit(IntAllocator(result));

    while (exponent > 0) {
        if (exponent & 1u) {
            if (!int_mul(&scratch, &acc, &current)) {
                IntDeinit(&acc);
                IntDeinit(&current);
                IntDeinit(&scratch);
                return false;
            }
            int_swap(&acc, &scratch);
        }

        exponent >>= 1u;
        if (exponent > 0) {
            if (!IntSquare(&scratch, &current)) {
                IntDeinit(&acc);
                IntDeinit(&current);
                IntDeinit(&scratch);
                return false;
            }
            int_swap(&current, &scratch);
        }
    }

    IntDeinit(&scratch);
    IntDeinit(&current);
    int_replace(result, &acc);
    return true;
}

bool int_pow_i64(Int *result, const Int *base, i64 exponent) {
    if (exponent < 0) {
        LOG_FATAL("Int exponent cannot be negative");
    }

    return int_pow_u64(result, base, (u64)exponent);
}

bool int_div_mod(Int *quotient, Int *remainder, const Int *dividend, const Int *divisor) {
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

    // dividend < divisor: quotient = 0, remainder = dividend.
    if (int_compare(dividend, divisor) < 0) {
        Int r0 = IntInit(IntAllocator(remainder));

        if (!int_try_clone_value(&r0, dividend)) {
            IntDeinit(&r0);
            return false;
        }
        IntClear(quotient);
        int_replace(remainder, &r0);
        return true;
    }

    // Shift-the-remainder long division, built directly in the output objects so
    // their existing capacity is reused (no per-call output allocation). The
    // remainder accumulates the dividend MSB-first; at each step remainder <<= 1,
    // pull in the next dividend bit, and if remainder >= divisor subtract it in
    // place and set the quotient bit. The loop reads dividend and divisor while
    // mutating the outputs, so any input that aliases an output is cloned first.
    Int  dividend_copy = IntInit(IntAllocator(quotient));
    Int  divisor_copy  = IntInit(IntAllocator(quotient));
    bool ok            = false;

    if (dividend == quotient || dividend == remainder) {
        if (!int_try_clone_value(&dividend_copy, dividend)) {
            goto cleanup;
        }
        dividend = &dividend_copy;
    }
    if (divisor == quotient || divisor == remainder) {
        if (!int_try_clone_value(&divisor_copy, divisor)) {
            goto cleanup;
        }
        divisor = &divisor_copy;
    }

    {
        u64 dividend_bits = IntBitLength(dividend);
        u64 divisor_bits  = IntBitLength(divisor);
        u64 r_words       = (divisor_bits + 1u + 63u) / 64u;

        // Size the outputs to whole 64-bit limbs and zero them, so the loop below
        // works on raw limb buffers: the four operands are validated once here
        // (and via BitVecData), and nothing inside the per-bit loop re-validates
        // or re-scans. The remainder needs divisor_bits+1 bits at most, rounded up
        // to a whole limb so its shift never runs off the buffer.
        IntClear(quotient);
        IntClear(remainder);
        if (!BitVecResize(INT_BITS(quotient), dividend_bits) || !BitVecResize(INT_BITS(remainder), r_words * 64u)) {
            goto cleanup;
        }

        {
            u8       *qd     = BitVecData(INT_BITS(quotient));
            u8       *rd     = BitVecData(INT_BITS(remainder));
            const u8 *dd     = BitVecData(INT_BITS(dividend));
            const u8 *vd     = BitVecData(INT_BITS(divisor));
            u64       rbytes = r_words * 8u;
            u64       rbits  = r_words * 64u;

            for (u64 bit = dividend_bits; bit > 0; bit--) {
                u64 i     = bit - 1u;
                u64 carry = 0;
                int cmp   = 0;

                // remainder <<= 1 (raw limb funnel), then pull in dividend bit i.
                for (u64 w = 0; w < r_words; w++) {
                    u64 v = int_load_le8(rd, rbits, w * 8u);

                    int_store_le8(rd, rbytes, w * 8u, (v << 1) | carry);
                    carry = v >> 63u;
                }
                if ((dd[i >> 3u] >> (i & 7u)) & 1u) {
                    int_store_le8(rd, rbytes, 0, int_load_le8(rd, rbits, 0) | 1u);
                }

                // remainder >= divisor ? (raw compare, high limb first)
                for (u64 w = r_words; w-- > 0;) {
                    u64 rv = int_load_le8(rd, rbits, w * 8u);
                    u64 dv = int_load_le8(vd, divisor_bits, w * 8u);

                    if (rv != dv) {
                        cmp = rv > dv ? 1 : -1;
                        break;
                    }
                }

                if (cmp >= 0) {
                    // remainder -= divisor (raw borrow chain), set quotient bit i.
                    u64 borrow = 0;

                    for (u64 w = 0; w < r_words; w++) {
                        u64 rv  = int_load_le8(rd, rbits, w * 8u);
                        u64 dv  = int_load_le8(vd, divisor_bits, w * 8u);
                        u64 out = 0;

                        borrow = int_sub_borrow_u64(rv, dv, borrow, &out);
                        int_store_le8(rd, rbytes, w * 8u, out);
                    }
                    qd[i >> 3u] |= (u8)(1u << (i & 7u));
                }
            }
        }

        int_normalize(quotient);
        int_normalize(remainder);
        ok = true;
    }

cleanup:
    IntDeinit(&dividend_copy);
    IntDeinit(&divisor_copy);
    return ok;
}

bool int_div(Int *result, const Int *dividend, const Int *divisor) {
    Int remainder = IntInit(IntAllocator(result));

    // Quotient written straight into result (int_div_mod is in-place and clones
    // any input that aliases an output), so result's buffer is reused.
    if (!int_div_mod(result, &remainder, dividend, divisor)) {
        IntDeinit(&remainder);
        return false;
    }

    IntDeinit(&remainder);
    return true;
}

bool int_div_exact(Int *result, const Int *dividend, const Int *divisor) {
    ValidateInt(result);
    ValidateInt(dividend);
    ValidateInt(divisor);

    if (IntIsZero(divisor)) {
        LOG_ERROR("Division by zero");
        return false;
    }

    Int quotient  = IntInit(IntAllocator(result));
    Int remainder = IntInit(IntAllocator(result));

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

bool int_div_u64(Int *result, const Int *dividend, u64 divisor) {
    Int divisor_value = IntInit(IntAllocator(dividend));

    if (!int_try_from_u64(&divisor_value, divisor, IntAllocator(dividend))) {
        IntDeinit(&divisor_value);
        return false;
    }

    bool ok = int_div(result, dividend, &divisor_value);
    IntDeinit(&divisor_value);
    return ok;
}

bool int_div_i64(Int *result, const Int *dividend, i64 divisor) {
    Int divisor_value = IntInit(IntAllocator(dividend));

    if (!int_try_from_i64_with_allocator(&divisor_value, divisor, IntAllocator(dividend))) {
        IntDeinit(&divisor_value);
        return false;
    }

    bool ok = int_div(result, dividend, &divisor_value);
    IntDeinit(&divisor_value);
    return ok;
}

bool int_div_exact_u64(Int *result, const Int *dividend, u64 divisor) {
    Int divisor_value = IntInit(IntAllocator(dividend));

    if (!int_try_from_u64(&divisor_value, divisor, IntAllocator(dividend))) {
        IntDeinit(&divisor_value);
        return false;
    }

    bool ok = int_div_exact(result, dividend, &divisor_value);
    IntDeinit(&divisor_value);
    return ok;
}

bool int_div_exact_i64(Int *result, const Int *dividend, i64 divisor) {
    Int divisor_value = IntInit(IntAllocator(dividend));

    if (!int_try_from_i64_with_allocator(&divisor_value, divisor, IntAllocator(dividend))) {
        IntDeinit(&divisor_value);
        return false;
    }

    bool ok = int_div_exact(result, dividend, &divisor_value);
    IntDeinit(&divisor_value);
    return ok;
}

bool int_div_mod_u64(Int *quotient, Int *remainder, const Int *dividend, u64 divisor) {
    Int divisor_value = IntInit(IntAllocator(dividend));

    if (!int_try_from_u64(&divisor_value, divisor, IntAllocator(dividend))) {
        IntDeinit(&divisor_value);
        return false;
    }

    bool ok = int_div_mod(quotient, remainder, dividend, &divisor_value);
    IntDeinit(&divisor_value);
    return ok;
}

bool int_div_mod_i64(Int *quotient, Int *remainder, const Int *dividend, i64 divisor) {
    Int divisor_value = IntInit(IntAllocator(dividend));

    if (!int_try_from_i64_with_allocator(&divisor_value, divisor, IntAllocator(dividend))) {
        IntDeinit(&divisor_value);
        return false;
    }

    bool ok = int_div_mod(quotient, remainder, dividend, &divisor_value);
    IntDeinit(&divisor_value);
    return ok;
}

u64 int_div_u64_rem(Int *quotient, const Int *dividend, u64 divisor) {
    ValidateInt(quotient);
    ValidateInt(dividend);

    if (divisor == 0) {
        LOG_ERROR("Division by zero");
        return 0;
    }

    Int divisor_value = IntInit(IntAllocator(dividend));
    Int remainder     = IntInit(IntAllocator(quotient));
    u64 rem           = 0;

    if (!int_try_from_u64(&divisor_value, divisor, IntAllocator(dividend))) {
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

bool int_mod(Int *result, const Int *dividend, const Int *divisor) {
    Int quotient = IntInit(IntAllocator(result));

    // Remainder written straight into result (int_div_mod is in-place and clones
    // any input that aliases an output), so result's buffer is reused.
    if (!int_div_mod(&quotient, result, dividend, divisor)) {
        IntDeinit(&quotient);
        return false;
    }

    IntDeinit(&quotient);
    return true;
}

bool int_mod_u64_into(Int *result, const Int *dividend, u64 divisor) {
    Int quotient = IntInit(IntAllocator(result));

    bool ok = int_div_mod_u64(&quotient, result, dividend, divisor);
    IntDeinit(&quotient);
    return ok;
}

bool int_mod_i64_into(Int *result, const Int *dividend, i64 divisor) {
    Int quotient = IntInit(IntAllocator(result));

    bool ok = int_div_mod_i64(&quotient, result, dividend, divisor);
    IntDeinit(&quotient);
    return ok;
}

u64 int_mod_u64(const Int *value, u64 modulus) {
    ValidateInt(value);

    if (modulus == 0) {
        LOG_ERROR("modulus is zero");
        return 0;
    }

    Int quotient = IntInit(IntAllocator(value));
    u64 rem      = int_div_u64_rem(&quotient, value, modulus);

    IntDeinit(&quotient);
    return rem;
}

bool IntGCD(Int *result, const Int *a, const Int *b) {
    ValidateInt(result);
    ValidateInt(a);
    ValidateInt(b);

    Int x = IntInit(IntAllocator(a));
    Int y = IntInit(IntAllocator(b));

    if (!int_try_clone_value(&x, a) || !int_try_clone_value(&y, b)) {
        IntDeinit(&x);
        IntDeinit(&y);
        return false;
    }

    while (!IntIsZero(&y)) {
        Int r = IntInit(IntAllocator(result));

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

bool IntLCM(Int *result, const Int *a, const Int *b) {
    ValidateInt(result);
    ValidateInt(a);
    ValidateInt(b);

    if (IntIsZero(a) || IntIsZero(b)) {
        Int zero = IntInit(IntAllocator(result));
        int_replace(result, &zero);
        return true;
    }

    Int gcd      = IntInit(IntAllocator(result));
    Int quotient = IntInit(IntAllocator(result));
    Int lcm      = IntInit(IntAllocator(result));

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

bool IntRootRem(Int *root, Int *remainder, const Int *value, u64 degree) {
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
        Int zero_root = IntInit(IntAllocator(root));
        Int zero_rem  = IntInit(IntAllocator(remainder));

        int_replace(root, &zero_root);
        int_replace(remainder, &zero_rem);
        return true;
    }
    if (degree == 1) {
        Int exact_root = IntInit(IntAllocator(root));
        Int zero_rem   = IntInit(IntAllocator(remainder));

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
    Int low        = IntInit(IntAllocator(root));
    Int high       = IntInit(IntAllocator(root));
    Int best       = IntInit(IntAllocator(root));
    Int one        = IntInit(IntAllocator(root));

    if ((bits % degree) != 0) {
        high_shift++;
    }
    if (high_shift == 0) {
        high_shift = 1;
    }

    if (!int_try_from_u64(&high, 1, IntAllocator(root)) || !int_try_from_u64(&one, 1, IntAllocator(root))) {
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
        Int sum     = IntInit(IntAllocator(root));
        Int mid     = IntInit(IntAllocator(root));
        Int mid_pow = IntInit(IntAllocator(root));
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
            Int next = IntInit(IntAllocator(root));

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
            Int next = IntInit(IntAllocator(root));

            if (IntEQ(&mid, &one) || IntIsZero(&mid)) {
                IntDeinit(&high);
                high = IntInit(IntAllocator(root));
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
        Int power = IntInit(IntAllocator(root));
        Int rem   = IntInit(IntAllocator(remainder));

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

bool IntRoot(Int *result, const Int *value, u64 degree) {
    Int root      = IntInit(IntAllocator(result));
    Int remainder = IntInit(IntAllocator(result));

    if (!IntRootRem(&root, &remainder, value, degree)) {
        IntDeinit(&root);
        IntDeinit(&remainder);
        return false;
    }

    IntDeinit(&remainder);
    int_replace(result, &root);
    return true;
}

bool IntSqrtRem(Int *root, Int *remainder, const Int *value) {
    return IntRootRem(root, remainder, value, 2);
}

bool IntSqrt(Int *result, const Int *value) {
    return IntRoot(result, value, 2);
}

bool IntIsPerfectSquare(const Int *value) {
    ValidateInt(value);

    Int  root      = IntInit(IntAllocator(value));
    Int  remainder = IntInit(IntAllocator(value));
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

bool IntIsPerfectPower(const Int *value) {
    ValidateInt(value);

    if (IntIsZero(value) || IntBitLength(value) == 1) {
        return true;
    }

    u64 max_degree = 0;

    if (!IntTryLog2(value, &max_degree)) {
        return false;
    }

    for (u64 degree = 2; degree <= max_degree; degree++) {
        Int  root      = IntInit(IntAllocator(value));
        Int  remainder = IntInit(IntAllocator(value));
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

bool IntTryJacobi(int *out, const Int *a, const Int *n) {
    ValidateInt(a);
    ValidateInt(n);

    if (!out) {
        LOG_FATAL("Invalid arguments");
    }

    if (IntIsZero(n) || IntIsEven(n)) {
        LOG_ERROR("n must be non-zero and odd");
        return false;
    }

    Int aa     = IntInit(IntAllocator(a));
    Int nn     = IntInit(IntAllocator(n));
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

int IntJacobiWithError(const Int *a, const Int *n, bool *error) {
    int  out = 0;
    bool ok  = IntTryJacobi(&out, a, n);

    if (error) {
        *error = !ok;
    }

    return out;
}

bool IntModAdd(Int *result, const Int *a, const Int *b, const Int *modulus) {
    ValidateInt(result);
    ValidateInt(a);
    ValidateInt(b);
    ValidateInt(modulus);

    if (IntIsZero(modulus)) {
        LOG_ERROR("modulus is zero");
        return false;
    }

    Int ar  = IntInit(IntAllocator(result));
    Int br  = IntInit(IntAllocator(result));
    Int sum = IntInit(IntAllocator(result));

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

bool IntModSub(Int *result, const Int *a, const Int *b, const Int *modulus) {
    ValidateInt(result);
    ValidateInt(a);
    ValidateInt(b);
    ValidateInt(modulus);

    if (IntIsZero(modulus)) {
        LOG_ERROR("modulus is zero");
        return false;
    }

    Int ar = IntInit(IntAllocator(result));
    Int br = IntInit(IntAllocator(result));

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
        Int diff = IntInit(IntAllocator(result));

        if (!int_sub(&diff, &br, &ar)) {
            IntDeinit(&ar);
            IntDeinit(&br);
            IntDeinit(&diff);
            return false;
        }
        if (IntIsZero(&diff)) {
            Int zero = IntInit(IntAllocator(result));
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

bool IntModMul(Int *result, const Int *a, const Int *b, const Int *modulus) {
    ValidateInt(result);
    ValidateInt(a);
    ValidateInt(b);
    ValidateInt(modulus);

    if (IntIsZero(modulus)) {
        LOG_ERROR("modulus is zero");
        return false;
    }

    Int ar   = IntInit(IntAllocator(result));
    Int br   = IntInit(IntAllocator(result));
    Int prod = IntInit(IntAllocator(result));

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

bool IntModDiv(Int *result, const Int *a, const Int *b, const Int *modulus) {
    ValidateInt(result);
    ValidateInt(a);
    ValidateInt(b);
    ValidateInt(modulus);

    if (IntIsZero(modulus)) {
        LOG_ERROR("modulus is zero");
        return false;
    }

    Int  inverse = IntInit(IntAllocator(result));
    Int  value   = IntInit(IntAllocator(result));
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

bool IntSquareMod(Int *result, const Int *value, const Int *modulus) {
    return IntModMul(result, value, value, modulus);
}

bool int_pow_u64_mod(Int *result, const Int *base, u64 exponent, const Int *modulus) {
    ValidateInt(result);
    ValidateInt(base);
    ValidateInt(modulus);

    if (IntIsZero(modulus)) {
        LOG_FATAL("modulus is zero");
    }

    Int acc      = IntInit(IntAllocator(result));
    Int base_mod = IntInit(IntAllocator(result));

    if (!int_try_from_u64(&acc, 1, IntAllocator(result))) {
        IntDeinit(&base_mod);
        return false;
    }
    if (!int_mod(&acc, &acc, modulus) || !int_mod(&base_mod, base, modulus)) {
        IntDeinit(&acc);
        IntDeinit(&base_mod);
        return false;
    }

    Int scratch = IntInit(IntAllocator(result));

    while (exponent > 0) {
        if (exponent & 1u) {
            if (!IntModMul(&scratch, &acc, &base_mod, modulus)) {
                IntDeinit(&acc);
                IntDeinit(&base_mod);
                IntDeinit(&scratch);
                return false;
            }
            int_swap(&acc, &scratch);
        }

        exponent >>= 1u;
        if (exponent > 0) {
            if (!IntModMul(&scratch, &base_mod, &base_mod, modulus)) {
                IntDeinit(&acc);
                IntDeinit(&base_mod);
                IntDeinit(&scratch);
                return false;
            }
            int_swap(&base_mod, &scratch);
        }
    }

    IntDeinit(&scratch);
    IntDeinit(&base_mod);
    int_replace(result, &acc);
    return true;
}

bool int_pow_mod(Int *result, const Int *base, const Int *exponent, const Int *modulus) {
    ValidateInt(result);
    ValidateInt(base);
    ValidateInt(exponent);
    ValidateInt(modulus);

    if (IntIsZero(modulus)) {
        LOG_ERROR("modulus is zero");
        return false;
    }

    Int acc      = IntInit(IntAllocator(result));
    Int base_mod = IntInit(IntAllocator(result));
    Int exp      = IntInit(IntAllocator(exponent));

    if (!int_try_from_u64(&acc, 1, IntAllocator(result)) || !IntTryClone(&exp, exponent) ||
        !int_mod(&acc, &acc, modulus) || !int_mod(&base_mod, base, modulus)) {
        IntDeinit(&acc);
        IntDeinit(&base_mod);
        IntDeinit(&exp);
        return false;
    }

    Int scratch = IntInit(IntAllocator(result));

    while (!IntIsZero(&exp)) {
        if (int_is_odd(&exp)) {
            if (!IntModMul(&scratch, &acc, &base_mod, modulus)) {
                IntDeinit(&acc);
                IntDeinit(&base_mod);
                IntDeinit(&exp);
                IntDeinit(&scratch);
                return false;
            }
            int_swap(&acc, &scratch);
        }

        if (!IntShiftRight(&exp, 1)) {
            IntDeinit(&acc);
            IntDeinit(&base_mod);
            IntDeinit(&exp);
            IntDeinit(&scratch);
            return false;
        }
        if (!IntIsZero(&exp)) {
            if (!IntModMul(&scratch, &base_mod, &base_mod, modulus)) {
                IntDeinit(&acc);
                IntDeinit(&base_mod);
                IntDeinit(&exp);
                IntDeinit(&scratch);
                return false;
            }
            int_swap(&base_mod, &scratch);
        }
    }

    IntDeinit(&scratch);
    IntDeinit(&exp);
    IntDeinit(&base_mod);
    int_replace(result, &acc);
    return true;
}

bool int_pow_i64_mod(Int *result, const Int *base, i64 exponent, const Int *modulus) {
    if (exponent < 0) {
        LOG_FATAL("Int exponent cannot be negative");
    }

    return int_pow_u64_mod(result, base, (u64)exponent, modulus);
}

bool IntModInv(Int *result, const Int *value, const Int *modulus) {
    ValidateInt(result);
    ValidateInt(value);
    ValidateInt(modulus);

    if (IntIsZero(modulus)) {
        LOG_ERROR("modulus is zero");
        return false;
    }

    Int       reduced = IntInit(IntAllocator(result));
    SignedInt t       = sint_init(IntAllocator(result));
    SignedInt new_t   = sint_from_u64(1, IntAllocator(result));
    Int       r       = IntInit(IntAllocator(modulus));
    Int       new_r   = IntInit(IntAllocator(result));
    Int       one     = int_from_u64(1, IntAllocator(result));
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
        Int       q       = IntInit(IntAllocator(result));
        Int       rem     = IntInit(IntAllocator(result));
        SignedInt q_new_t = sint_init(IntAllocator(result));
        SignedInt next_t  = sint_init(IntAllocator(result));
        Int       next_r  = IntInit(IntAllocator(result));

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
        Int positive = IntInit(IntAllocator(result));
        Int mag_mod  = IntInit(IntAllocator(result));

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

bool IntModSqrt(Int *result, const Int *value, const Int *modulus) {
    ValidateInt(result);
    ValidateInt(value);
    ValidateInt(modulus);

    if (IntIsZero(modulus)) {
        LOG_ERROR("modulus is zero");
        return false;
    }

    Int  a  = IntInit(IntAllocator(result));
    bool ok = false;

    if (!int_mod(&a, value, modulus)) {
        IntDeinit(&a);
        return false;
    }

    if (IntIsZero(&a)) {
        Int zero = IntInit(IntAllocator(result));
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
        Int exponent = IntInit(IntAllocator(modulus));
        Int root     = IntInit(IntAllocator(result));

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
        Int q        = IntInit(IntAllocator(modulus));
        Int z        = IntInit(IntAllocator(modulus));
        Int c        = IntInit(IntAllocator(result));
        Int t        = IntInit(IntAllocator(result));
        Int r        = IntInit(IntAllocator(result));
        Int exponent = IntInit(IntAllocator(result));
        u64 m        = 0;

        if (!IntTryClone(&q, modulus) || !int_try_from_u64(&z, 2, IntAllocator(modulus)) || !int_sub_u64(&q, &q, 1)) {
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
            Int t_power = IntInit(IntAllocator(&t));
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

            Int scratch = IntInit(IntAllocator(result));

            for (i = 1; i < m; i++) {
                if (!IntSquareMod(&scratch, &t_power, modulus)) {
                    IntDeinit(&scratch);
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
                int_swap(&t_power, &scratch);

                if (int_compare_u64(&t_power, 1) == 0) {
                    break;
                }
            }

            IntDeinit(&scratch);

            if (i == m) {
                IntDeinit(&t_power);
                break;
            }

            {
                Int b    = IntInit(IntAllocator(&c));
                Int b_sq = IntInit(IntAllocator(result));
                Int next = IntInit(IntAllocator(result));

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
                    Int square = IntInit(IntAllocator(result));

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
                next = IntInit(IntAllocator(result));
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

bool IntIsProbablePrimeWithError(const Int *value, bool *error) {
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
        Int  d           = IntInit(IntAllocator(value));
        Int  n_minus_one = IntInit(IntAllocator(value));
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
            Int base = IntInit(IntAllocator(value));
            Int x    = IntInit(IntAllocator(value));

            if (!int_try_from_u64(&base, bases[i], IntAllocator(value))) {
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
                    Int next = IntInit(IntAllocator(value));

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

bool IntNextPrime(Int *result, const Int *value) {
    bool error = false;

    ValidateInt(result);
    ValidateInt(value);

    if (int_compare_u64(value, 1) <= 0) {
        Int two = IntInit(IntAllocator(result));

        if (!int_try_from_u64(&two, 2, IntAllocator(result))) {
            IntDeinit(&two);
            return false;
        }
        int_replace(result, &two);
        return true;
    }

    Int candidate = IntInit(IntAllocator(result));

    if (!IntTryClone(&candidate, value)) {
        IntDeinit(&candidate);
        return false;
    }

    if (!int_add_u64(&candidate, &candidate, 1)) {
        IntDeinit(&candidate);
        return false;
    }
    if (int_compare_u64(&candidate, 2) <= 0) {
        Int two = IntInit(IntAllocator(result));

        if (!int_try_from_u64(&two, 2, IntAllocator(result))) {
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
