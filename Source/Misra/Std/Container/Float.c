/// file      : std/container/float.c
/// author    : Generated following Misra project patterns
/// This is free and unencumbered software released into the public domain.
///
/// Arbitrary-precision decimal floating-point implementation built on top of Int.

#include <Misra/Std/Container/Float.h>
#include <Misra/Std/Zstr.h>
#include <Misra/Std/Container/Float/Private.h>
#include <Misra/Std/Container/Int.h>
#include <Misra/Std/Log.h>


static void float_normalize(Float *value);
static void float_replace(Float *dst, Float *src);
static bool float_try_int_from_u64(Int *out, u64 value, Allocator *alloc);
static bool float_try_from_u64_value(Float *out, u64 value, Allocator *alloc);
static bool float_try_from_i64_value(Float *out, i64 value, Allocator *alloc);
static bool float_try_from_int_value(Float *out, Int *value);
static bool float_try_from_f32_value(Float *out, float value, Allocator *alloc);
static bool float_try_from_f64_value(Float *out, double value, Allocator *alloc);
static bool float_pow10(Int *out, u64 power, Allocator *alloc);
static bool float_scale_to_exponent(Float *value, i64 target_exponent);
static bool float_try_abs_compare(int *out, Float *lhs, Float *rhs);
static i64  float_add_i64_checked(i64 a, i64 b);
static i64  float_sub_i64_checked(i64 a, i64 b);

static i64 float_add_i64_checked(i64 a, i64 b) {
    if ((b > 0 && a > INT64_MAX - b) || (b < 0 && a < INT64_MIN - b)) {
        LOG_FATAL("Float exponent overflow");
    }

    return a + b;
}

static i64 float_sub_i64_checked(i64 a, i64 b) {
    if ((b > 0 && a < INT64_MIN + b) || (b < 0 && a > INT64_MAX + b)) {
        LOG_FATAL("Float exponent overflow");
    }

    return a - b;
}

// Exact IEEE-754 -> Float construction. Avoids the libc `snprintf`
// round-trip the old implementation used.
//
// Given a finite IEEE binary value `(-1)^neg * mantissa * 2^binexp`,
// we express it in base-10 as `(-1)^neg * sig * 10^dexp`:
//   - binexp >= 0: sig = mantissa << binexp,             dexp = 0
//   - binexp <  0: sig = mantissa * 5^|binexp|,          dexp = binexp
//                  (because value * 10^|binexp|
//                  = mantissa * 5^|binexp|.)
//
// The resulting representation is exact -- f32/f64 -> Float -> back
// is bit-perfect, where the libc %g / %.17g path was shortest-form
// (could lose information).
static bool float_try_from_ieee_bits(Float *out, u64 mantissa, int binexp, bool negative, Allocator *alloc) {
    if (!out) {
        LOG_FATAL("Invalid arguments");
    }
    *out          = FloatInit(alloc);
    out->negative = negative;

    if (mantissa == 0) {
        return true; // signed zero
    }

    if (!float_try_int_from_u64(&out->significand, mantissa, alloc)) {
        return false;
    }

    if (binexp > 0) {
        if (!IntShiftLeft(&out->significand, (u64)binexp)) {
            return false;
        }
        out->exponent = 0;
    } else if (binexp < 0) {
        u64 n    = (u64)(-(i64)binexp);
        Int five = IntInit(alloc);
        Int pow5 = IntInit(alloc);
        Int sig  = IntInit(alloc);
        if (!float_try_int_from_u64(&five, 5u, alloc) || !IntPow(&pow5, &five, n) ||
            !int_mul(&sig, &out->significand, &pow5)) {
            IntDeinit(&five);
            IntDeinit(&pow5);
            IntDeinit(&sig);
            return false;
        }
        IntDeinit(&five);
        IntDeinit(&pow5);
        IntDeinit(&out->significand);
        out->significand = sig;
        out->exponent    = (i64)binexp;
    } else {
        out->exponent = 0;
    }

    float_normalize(out);
    return true;
}

static bool float_try_from_f32_value(Float *out, float value, Allocator *alloc) {
    if (!out) {
        LOG_FATAL("Invalid arguments");
    }
    union {
        f32 f;
        u32 b;
    } u       = {.f = value};
    u32  bits = u.b;
    bool neg  = ((bits >> 31) & 1u) != 0;
    u32  e    = (bits >> 23) & 0xFFu;
    u32  m    = bits & 0x7FFFFFu;
    if (e == 0xFFu) {
        LOG_FATAL("Float from f32 does not represent finite values (Inf/NaN)");
    }
    u64 mantissa;
    int binexp;
    if (e == 0) {
        // Denormal: value = m * 2^(-126 - 23)
        mantissa = (u64)m;
        binexp   = -126 - 23;
    } else {
        // Normal: implicit leading 1
        mantissa = (u64)m | (1ULL << 23);
        binexp   = (int)e - 127 - 23;
    }
    return float_try_from_ieee_bits(out, mantissa, binexp, neg, alloc);
}

static bool float_try_from_f64_value(Float *out, double value, Allocator *alloc) {
    if (!out) {
        LOG_FATAL("Invalid arguments");
    }
    union {
        f64 f;
        u64 b;
    } u       = {.f = value};
    u64  bits = u.b;
    bool neg  = ((bits >> 63) & 1ull) != 0;
    u64  e    = (bits >> 52) & 0x7FFull;
    u64  m    = bits & 0xFFFFFFFFFFFFFull;
    if (e == 0x7FFull) {
        LOG_FATAL("Float from f64 does not represent finite values (Inf/NaN)");
    }
    u64 mantissa;
    int binexp;
    if (e == 0) {
        mantissa = m;
        binexp   = -1022 - 52;
    } else {
        mantissa = m | (1ULL << 52);
        binexp   = (int)e - 1023 - 52;
    }
    return float_try_from_ieee_bits(out, mantissa, binexp, neg, alloc);
}

static void float_replace(Float *dst, Float *src) {
    FloatDeinit(dst);
    *dst = *src;
}

static bool float_try_int_from_u64(Int *out, u64 value, Allocator *alloc) {
    u64 bits = 0;

    if (!out) {
        LOG_FATAL("Invalid arguments");
    }

    *out = IntInit(alloc);
    if (value == 0) {
        return true;
    }

    for (u64 remaining = value; remaining != 0; remaining >>= 1) {
        bits++;
    }

    if (!BitVecTryFromInteger(&out->bits, value, bits, alloc)) {
        IntDeinit(out);
        *out = IntInit(alloc);
        return false;
    }

    return true;
}

static bool float_pow10(Int *out, u64 power, Allocator *alloc) {
    Int base;
    Int result;

    if (!out) {
        LOG_FATAL("Invalid arguments");
    }
    if (!float_try_int_from_u64(&base, 10, alloc) || !float_try_int_from_u64(&result, 1, alloc)) {
        IntDeinit(&base);
        IntDeinit(&result);
        return false;
    }

    if (!int_pow_u64(&result, &base, power)) {
        IntDeinit(&base);
        IntDeinit(&result);
        return false;
    }
    IntDeinit(&base);
    *out = result;
    return true;
}

static bool float_scale_to_exponent(Float *value, i64 target_exponent) {
    ValidateFloat(value);

    if (FloatIsZero(value)) {
        value->exponent = target_exponent;
        return true;
    }
    if (target_exponent > value->exponent) {
        LOG_FATAL("target exponent must not exceed current exponent");
    }
    if (target_exponent == value->exponent) {
        return true;
    }

    {
        u64 places = (u64)(value->exponent - target_exponent);
        Int factor = IntInit(value->significand.bits.allocator);
        Int scaled = IntInit(value->significand.bits.allocator);

        if (!float_pow10(&factor, places, value->significand.bits.allocator) ||
            !int_mul(&scaled, &value->significand, &factor)) {
            IntDeinit(&factor);
            IntDeinit(&scaled);
            return false;
        }
        IntDeinit(&factor);
        IntDeinit(&value->significand);

        value->significand = scaled;
        value->exponent    = target_exponent;
    }

    return true;
}

static bool float_try_abs_compare(int *out, Float *lhs, Float *rhs) {
    ValidateFloat(lhs);
    ValidateFloat(rhs);
    if (!out) {
        LOG_FATAL("Invalid arguments");
    }

    if (FloatIsZero(lhs) && FloatIsZero(rhs)) {
        *out = 0;
        return true;
    }

    {
        i64   target_exponent = lhs->exponent < rhs->exponent ? lhs->exponent : rhs->exponent;
        Float lhs_scaled      = FloatInit(lhs->significand.bits.allocator);
        Float rhs_scaled      = FloatInit(rhs->significand.bits.allocator);

        if (!FloatTryClone(&lhs_scaled, lhs) || !FloatTryClone(&rhs_scaled, rhs) ||
            !float_scale_to_exponent(&lhs_scaled, target_exponent) ||
            !float_scale_to_exponent(&rhs_scaled, target_exponent)) {
            FloatDeinit(&lhs_scaled);
            FloatDeinit(&rhs_scaled);
            return false;
        }
        *out = int_compare(&lhs_scaled.significand, &rhs_scaled.significand);

        FloatDeinit(&lhs_scaled);
        FloatDeinit(&rhs_scaled);
        return true;
    }
}

static void float_normalize(Float *value) {
    ValidateFloat(value);

    if (IntIsZero(&value->significand)) {
        value->negative = false;
        value->exponent = 0;
        return;
    }

    while (int_mod_u64(&value->significand, 10) == 0) {
        Int quotient = IntInit(value->significand.bits.allocator);

        (void)int_div_u64_rem(&quotient, &value->significand, 10);
        IntDeinit(&value->significand);
        value->significand = quotient;
        value->exponent    = float_add_i64_checked(value->exponent, 1);
    }
}

bool FloatIsZero(Float *value) {
    ValidateFloat(value);
    return IntIsZero(&value->significand);
}

bool FloatIsNegative(Float *value) {
    ValidateFloat(value);
    return !FloatIsZero(value) && value->negative;
}

i64 FloatExponent(Float *value) {
    ValidateFloat(value);
    return value->exponent;
}

Float FloatClone(Float *value) {
    Float clone;

    ValidateFloat(value);
    clone = FloatInit(value->significand.bits.allocator);
    (void)FloatTryClone(&clone, value);
    return clone;
}

bool FloatTryClone(Float *out, Float *value) {
    if (!out || !value) {
        LOG_FATAL("Invalid arguments");
    }

    ValidateFloat(value);
    *out          = FloatInit(value->significand.bits.allocator);
    out->negative = value->negative;
    out->exponent = value->exponent;
    if (!IntTryClone(&out->significand, &value->significand)) {
        FloatDeinit(out);
        *out = FloatInit(value->significand.bits.allocator);
        return false;
    }

    return true;
}

static bool float_try_from_u64_value(Float *out, u64 value, Allocator *alloc) {
    if (!out) {
        LOG_FATAL("Invalid arguments");
    }

    *out = FloatInit(alloc);
    if (!float_try_int_from_u64(&out->significand, value, alloc)) {
        FloatDeinit(out);
        *out = FloatInit(alloc);
        return false;
    }

    return true;
}

static bool float_try_from_i64_value(Float *out, i64 value, Allocator *alloc) {
    u64 magnitude = 0;

    if (!out) {
        LOG_FATAL("Invalid arguments");
    }

    if (value < 0) {
        magnitude = (u64)(-(value + 1)) + 1;
    } else {
        magnitude = (u64)value;
    }

    if (!float_try_from_u64_value(out, magnitude, alloc)) {
        return false;
    }

    out->negative = value < 0 && magnitude != 0;
    return true;
}

static bool float_try_from_int_value(Float *out, Int *value) {
    if (!out || !value) {
        LOG_FATAL("Invalid arguments");
    }

    ValidateInt(value);
    *out = FloatInit(value->bits.allocator);
    if (!IntTryClone(&out->significand, value)) {
        FloatDeinit(out);
        *out = FloatInit(value->bits.allocator);
        return false;
    }

    return true;
}

Float float_from_u64(u64 value, Allocator *alloc) {
    Float result;

    result = FloatInit(alloc);
    (void)float_try_from_u64_value(&result, value, alloc);
    float_normalize(&result);
    return result;
}

Float float_from_i64(i64 value, Allocator *alloc) {
    Float result = FloatInit(alloc);

    (void)float_try_from_i64_value(&result, value, alloc);
    float_normalize(&result);
    return result;
}

Float float_from_int(Int *value, Allocator *alloc) {
    Float result;

    ValidateInt(value);
    (void)alloc;
    result = FloatInit(value->bits.allocator);
    (void)float_try_from_int_value(&result, value);
    float_normalize(&result);
    return result;
}

Float float_from_f32(float value, Allocator *alloc) {
    Float result = FloatInit(alloc);

    (void)float_try_from_f32_value(&result, value, alloc);
    return result;
}

Float float_from_f64(double value, Allocator *alloc) {
    Float result = FloatInit(alloc);

    (void)float_try_from_f64_value(&result, value, alloc);
    return result;
}

bool FloatToInt(Int *result, Float *value) {
    Int temp = IntInit(result->bits.allocator);

    ValidateInt(result);
    ValidateFloat(value);

    if (FloatIsNegative(value)) {
        IntDeinit(&temp);
        return false;
    }

    if (FloatIsZero(value)) {
        IntDeinit(result);
        *result = temp;
        return true;
    }

    if (value->exponent >= 0) {
        Int factor = IntInit(value->significand.bits.allocator);

        if (!IntTryClone(&temp, &value->significand) ||
            !float_pow10(&factor, (u64)value->exponent, value->significand.bits.allocator) ||
            !int_mul(&temp, &temp, &factor)) {
            IntDeinit(&factor);
            IntDeinit(&temp);
            return false;
        }

        IntDeinit(&factor);
        IntDeinit(result);
        *result = temp;
        return true;
    }

    {
        u64  places = (u64)(-value->exponent);
        Int  factor = IntInit(value->significand.bits.allocator);
        bool ok     = false;

        if (!float_pow10(&factor, places, value->significand.bits.allocator)) {
            IntDeinit(&factor);
            return false;
        }
        ok = int_div_exact(&temp, &value->significand, &factor);
        IntDeinit(&factor);
        if (!ok) {
            IntDeinit(&temp);
            return false;
        }
    }

    IntDeinit(result);
    *result = temp;
    return true;
}

static bool float_try_from_str_impl(Float *out, const char *text, size length) {
    Float result;
    Str   digits;
    size  pos          = 0;
    bool  negative     = false;
    bool  saw_digit    = false;
    bool  saw_decimal  = false;
    i64   fractional   = 0;
    i64   explicit_exp = 0;

    if (!out || !text) {
        LOG_FATAL("Invalid arguments");
    }

    ValidateFloat(out);
    result = FloatInit(out->significand.bits.allocator);
    digits = StrInit(out->significand.bits.allocator);

    if (pos < length && (text[pos] == '+' || text[pos] == '-')) {
        negative = text[pos] == '-';
        pos++;
    }

    for (; pos < length; pos++) {
        char ch = text[pos];

        if (ch >= '0' && ch <= '9') {
            if (!StrPushBack(&digits, ch)) {
                goto fail;
            }
            saw_digit = true;
            if (saw_decimal) {
                if (fractional == INT64_MAX) {
                    LOG_ERROR("Float fractional exponent overflow");
                    goto fail;
                }
                fractional++;
            }
            continue;
        }

        if (ch == '.') {
            if (saw_decimal) {
                LOG_ERROR("Invalid Float format");
                goto fail;
            }

            saw_decimal = true;
            continue;
        }

        if (ch == 'e' || ch == 'E') {
            const char *endptr     = NULL;
            const char *exp_start  = NULL;
            long long   parsed     = 0;
            size        exp_offset = 0;

            pos++;
            if (pos >= length) {
                LOG_ERROR("Invalid Float exponent");
                goto fail;
            }

            // ZstrToI64 needs a NUL-terminated string. Within the Str-arm
            // we have a length-bounded view; the Str values in this
            // codebase are NUL-terminated by construction, so reading via
            // text+pos as a Zstr is safe. We additionally require the
            // parsed exponent to consume to the end of the bounded view
            // for validity.
            exp_start = text + pos;
            parsed    = ZstrToI64(exp_start, &endptr);
            if (endptr == exp_start) {
                LOG_ERROR("Invalid Float exponent");
                goto fail;
            }

            exp_offset = (size)(endptr - text);
            if (exp_offset != length) {
                LOG_ERROR("Invalid Float exponent");
                goto fail;
            }

            explicit_exp = (i64)parsed;
            pos          = exp_offset;
            break;
        }

        LOG_ERROR("Invalid Float format");
        goto fail;
    }

    if (!saw_digit) {
        LOG_ERROR("Invalid Float format");
        goto fail;
    }

    if (!IntTryFromStr(&result.significand, &digits)) {
        goto fail;
    }

    if (explicit_exp < INT64_MIN + fractional) {
        LOG_ERROR("Float exponent overflow");
        goto fail;
    }

    result.negative = negative && !IntIsZero(&result.significand);
    result.exponent = explicit_exp - fractional;

    StrDeinit(&digits);
    float_normalize(&result);
    FloatDeinit(out);
    *out = result;
    return true;

fail:
    StrDeinit(&digits);
    FloatDeinit(&result);
    return false;
}

bool float_try_from_str_zstr(Float *out, Zstr text) {
    if (!out || !text) {
        LOG_FATAL("Invalid arguments");
    }
    return float_try_from_str_impl(out, text, (size)ZstrLen(text));
}

bool float_try_from_str_str(Float *out, const Str *text) {
    if (!out || !text) {
        LOG_FATAL("Invalid arguments");
    }
    return float_try_from_str_impl(out, text->data, text->length);
}

Float float_from_str_zstr(Zstr text, Allocator *alloc) {
    Float result = FloatInit(alloc);

    (void)float_try_from_str_zstr(&result, text);
    return result;
}

Float float_from_str_str(const Str *text, Allocator *alloc) {
    Float result = FloatInit(alloc);

    (void)float_try_from_str_str(&result, text);
    return result;
}

bool float_try_to_str(Str *out, Float *value, Allocator *alloc) {
    Str digits;
    Str result;

    ValidateFloat(value);
    if (!out) {
        LOG_FATAL("Invalid arguments");
    }

    *out = StrInit(alloc);

    if (FloatIsZero(value)) {
        return StrPushBack(out, '0');
    }

    if (!int_try_to_str(&digits, &value->significand, alloc)) {
        return false;
    }

    result = StrInit(alloc);

    if (value->negative) {
        if (!StrPushBack(&result, '-')) {
            goto fail;
        }
    }

    if (value->exponent >= 0) {
        if (!StrMerge(&result, &digits)) {
            goto fail;
        }

        for (i64 i = 0; i < value->exponent; i++) {
            if (!StrPushBack(&result, '0')) {
                goto fail;
            }
        }
    } else {
        i64 split = (i64)digits.length + value->exponent;

        if (split > 0) {
            for (i64 i = 0; i < split; i++) {
                if (!StrPushBack(&result, digits.data[i])) {
                    goto fail;
                }
            }

            if (!StrPushBack(&result, '.')) {
                goto fail;
            }

            for (u64 i = (u64)split; i < digits.length; i++) {
                if (!StrPushBack(&result, digits.data[i])) {
                    goto fail;
                }
            }
        } else {
            if (!StrPushBackZstr(&result, "0.")) {
                goto fail;
            }

            for (i64 i = 0; i < -split; i++) {
                if (!StrPushBack(&result, '0')) {
                    goto fail;
                }
            }

            if (!StrMerge(&result, &digits)) {
                goto fail;
            }
        }
    }

    StrDeinit(&digits);
    *out = result;
    return true;

fail:
    StrDeinit(&digits);
    StrDeinit(&result);
    return false;
}

Str float_to_str(Float *value, Allocator *alloc) {
    Str result;

    ValidateFloat(value);

    if (!float_try_to_str(&result, value, alloc)) {
        result = StrInit(alloc);
    }

    return result;
}

int float_compare_with_error(Float *lhs, Float *rhs, bool *error) {
    int cmp = 0;

    if (error) {
        *error = false;
    }

    ValidateFloat(lhs);
    ValidateFloat(rhs);

    if (FloatIsZero(lhs) && FloatIsZero(rhs)) {
        return 0;
    }
    if (FloatIsNegative(lhs) != FloatIsNegative(rhs)) {
        return FloatIsNegative(lhs) ? -1 : 1;
    }

    if (!float_try_abs_compare(&cmp, lhs, rhs)) {
        if (error) {
            *error = true;
        }
        return 0;
    }
    return FloatIsNegative(lhs) ? -cmp : cmp;
}

int float_compare(Float *lhs, Float *rhs) {
    return float_compare_with_error(lhs, rhs, NULL);
}

int float_compare_int_with_error(Float *lhs, Int *rhs, bool *error) {
    Float rhs_value = FloatInit(lhs->significand.bits.allocator);
    int   cmp       = 0;

    ValidateFloat(lhs);
    ValidateInt(rhs);
    if (error) {
        *error = false;
    }

    if (!float_try_from_int_value(&rhs_value, rhs)) {
        if (error) {
            *error = true;
        }
        FloatDeinit(&rhs_value);
        return 0;
    }
    cmp = float_compare_with_error(lhs, &rhs_value, error);
    FloatDeinit(&rhs_value);
    return cmp;
}

int float_compare_int(Float *lhs, Int *rhs) {
    return float_compare_int_with_error(lhs, rhs, NULL);
}

int float_compare_u64_with_error(Float *lhs, u64 rhs, bool *error) {
    Float rhs_value = FloatInit(lhs->significand.bits.allocator);
    int   cmp       = 0;

    ValidateFloat(lhs);
    if (error) {
        *error = false;
    }

    if (!float_try_from_u64_value(&rhs_value, rhs, lhs->significand.bits.allocator)) {
        if (error) {
            *error = true;
        }
        FloatDeinit(&rhs_value);
        return 0;
    }
    cmp = float_compare_with_error(lhs, &rhs_value, error);
    FloatDeinit(&rhs_value);
    return cmp;
}

int float_compare_u64(Float *lhs, u64 rhs) {
    return float_compare_u64_with_error(lhs, rhs, NULL);
}

int float_compare_i64_with_error(Float *lhs, i64 rhs, bool *error) {
    Float rhs_value = FloatInit(lhs->significand.bits.allocator);
    int   cmp       = 0;

    ValidateFloat(lhs);
    if (error) {
        *error = false;
    }

    if (!float_try_from_i64_value(&rhs_value, rhs, lhs->significand.bits.allocator)) {
        if (error) {
            *error = true;
        }
        FloatDeinit(&rhs_value);
        return 0;
    }
    cmp = float_compare_with_error(lhs, &rhs_value, error);
    FloatDeinit(&rhs_value);
    return cmp;
}

int float_compare_i64(Float *lhs, i64 rhs) {
    return float_compare_i64_with_error(lhs, rhs, NULL);
}

int float_compare_f32_with_error(Float *lhs, float rhs, bool *error) {
    Float rhs_value = FloatInit(lhs->significand.bits.allocator);
    int   cmp       = 0;

    ValidateFloat(lhs);
    if (error) {
        *error = false;
    }

    if (!float_try_from_f32_value(&rhs_value, rhs, lhs->significand.bits.allocator)) {
        if (error) {
            *error = true;
        }
        FloatDeinit(&rhs_value);
        return 0;
    }
    cmp = float_compare_with_error(lhs, &rhs_value, error);
    FloatDeinit(&rhs_value);
    return cmp;
}

int float_compare_f32(Float *lhs, float rhs) {
    return float_compare_f32_with_error(lhs, rhs, NULL);
}

int float_compare_f64_with_error(Float *lhs, double rhs, bool *error) {
    Float rhs_value = FloatInit(lhs->significand.bits.allocator);
    int   cmp       = 0;

    ValidateFloat(lhs);
    if (error) {
        *error = false;
    }

    if (!float_try_from_f64_value(&rhs_value, rhs, lhs->significand.bits.allocator)) {
        if (error) {
            *error = true;
        }
        FloatDeinit(&rhs_value);
        return 0;
    }
    cmp = float_compare_with_error(lhs, &rhs_value, error);
    FloatDeinit(&rhs_value);
    return cmp;
}

int float_compare_f64(Float *lhs, double rhs) {
    return float_compare_f64_with_error(lhs, rhs, NULL);
}

void FloatNegate(Float *value) {
    ValidateFloat(value);

    if (!FloatIsZero(value)) {
        value->negative = !value->negative;
    }
}

void FloatAbs(Float *value) {
    ValidateFloat(value);
    value->negative = false;
}

bool float_add(Float *result, Float *a, Float *b) {
    Float lhs;
    Float rhs;
    Float temp;
    i64   exp = 0;

    ValidateFloat(result);
    ValidateFloat(a);
    ValidateFloat(b);
    lhs  = FloatInit(a->significand.bits.allocator);
    rhs  = FloatInit(b->significand.bits.allocator);
    temp = FloatInit(result->significand.bits.allocator);

    if (!FloatTryClone(&lhs, a) || !FloatTryClone(&rhs, b)) {
        FloatDeinit(&lhs);
        FloatDeinit(&rhs);
        FloatDeinit(&temp);
        return false;
    }

    exp = lhs.exponent < rhs.exponent ? lhs.exponent : rhs.exponent;
    if (!float_scale_to_exponent(&lhs, exp) || !float_scale_to_exponent(&rhs, exp)) {
        FloatDeinit(&lhs);
        FloatDeinit(&rhs);
        FloatDeinit(&temp);
        return false;
    }

    temp.exponent = exp;

    if (lhs.negative == rhs.negative) {
        if (!int_add(&temp.significand, &lhs.significand, &rhs.significand)) {
            FloatDeinit(&lhs);
            FloatDeinit(&rhs);
            FloatDeinit(&temp);
            return false;
        }
        temp.negative = lhs.negative;
    } else {
        int cmp = int_compare(&lhs.significand, &rhs.significand);

        if (cmp > 0) {
            if (!int_sub(&temp.significand, &lhs.significand, &rhs.significand)) {
                FloatDeinit(&lhs);
                FloatDeinit(&rhs);
                FloatDeinit(&temp);
                return false;
            }
            temp.negative = lhs.negative;
        } else if (cmp < 0) {
            if (!int_sub(&temp.significand, &rhs.significand, &lhs.significand)) {
                FloatDeinit(&lhs);
                FloatDeinit(&rhs);
                FloatDeinit(&temp);
                return false;
            }
            temp.negative = rhs.negative;
        } else {
            FloatClear(&temp);
        }
    }

    float_normalize(&temp);
    FloatDeinit(&lhs);
    FloatDeinit(&rhs);
    float_replace(result, &temp);
    return true;
}

bool float_add_int(Float *result, Float *a, Int *b) {
    Float rhs = FloatInit(result->significand.bits.allocator);

    if (!float_try_from_int_value(&rhs, b)) {
        return false;
    }
    bool ok = float_add(result, a, &rhs);
    FloatDeinit(&rhs);
    return ok;
}

bool float_add_u64(Float *result, Float *a, u64 b) {
    Float rhs = FloatInit(result->significand.bits.allocator);

    if (!float_try_from_u64_value(&rhs, b, result->significand.bits.allocator)) {
        return false;
    }
    bool ok = float_add(result, a, &rhs);
    FloatDeinit(&rhs);
    return ok;
}

bool float_add_i64(Float *result, Float *a, i64 b) {
    Float rhs = FloatInit(result->significand.bits.allocator);

    if (!float_try_from_i64_value(&rhs, b, result->significand.bits.allocator)) {
        return false;
    }
    bool ok = float_add(result, a, &rhs);
    FloatDeinit(&rhs);
    return ok;
}

bool float_add_f32(Float *result, Float *a, float b) {
    Float rhs = FloatInit(result->significand.bits.allocator);

    if (!float_try_from_f32_value(&rhs, b, result->significand.bits.allocator)) {
        return false;
    }
    bool ok = float_add(result, a, &rhs);
    FloatDeinit(&rhs);
    return ok;
}

bool float_add_f64(Float *result, Float *a, double b) {
    Float rhs = FloatInit(result->significand.bits.allocator);

    if (!float_try_from_f64_value(&rhs, b, result->significand.bits.allocator)) {
        return false;
    }
    bool ok = float_add(result, a, &rhs);
    FloatDeinit(&rhs);
    return ok;
}

bool float_sub(Float *result, Float *a, Float *b) {
    Float rhs = FloatInit(b->significand.bits.allocator);

    ValidateFloat(result);
    ValidateFloat(a);
    ValidateFloat(b);

    if (!FloatTryClone(&rhs, b)) {
        return false;
    }
    FloatNegate(&rhs);
    bool ok = float_add(result, a, &rhs);
    FloatDeinit(&rhs);
    return ok;
}

bool float_sub_int(Float *result, Float *a, Int *b) {
    Float rhs = FloatInit(result->significand.bits.allocator);

    if (!float_try_from_int_value(&rhs, b)) {
        return false;
    }
    bool ok = float_sub(result, a, &rhs);
    FloatDeinit(&rhs);
    return ok;
}

bool float_sub_u64(Float *result, Float *a, u64 b) {
    Float rhs = FloatInit(result->significand.bits.allocator);

    if (!float_try_from_u64_value(&rhs, b, result->significand.bits.allocator)) {
        return false;
    }
    bool ok = float_sub(result, a, &rhs);
    FloatDeinit(&rhs);
    return ok;
}

bool float_sub_i64(Float *result, Float *a, i64 b) {
    Float rhs = FloatInit(result->significand.bits.allocator);

    if (!float_try_from_i64_value(&rhs, b, result->significand.bits.allocator)) {
        return false;
    }
    bool ok = float_sub(result, a, &rhs);
    FloatDeinit(&rhs);
    return ok;
}

bool float_sub_f32(Float *result, Float *a, float b) {
    Float rhs = FloatInit(result->significand.bits.allocator);

    if (!float_try_from_f32_value(&rhs, b, result->significand.bits.allocator)) {
        return false;
    }
    bool ok = float_sub(result, a, &rhs);
    FloatDeinit(&rhs);
    return ok;
}

bool float_sub_f64(Float *result, Float *a, double b) {
    Float rhs = FloatInit(result->significand.bits.allocator);

    if (!float_try_from_f64_value(&rhs, b, result->significand.bits.allocator)) {
        return false;
    }
    bool ok = float_sub(result, a, &rhs);
    FloatDeinit(&rhs);
    return ok;
}

bool float_mul(Float *result, Float *a, Float *b) {
    Float temp = FloatInit(result->significand.bits.allocator);

    ValidateFloat(result);
    ValidateFloat(a);
    ValidateFloat(b);

    if (!int_mul(&temp.significand, &a->significand, &b->significand)) {
        FloatDeinit(&temp);
        return false;
    }
    temp.negative = FloatIsNegative(a) != FloatIsNegative(b);
    temp.exponent = float_add_i64_checked(a->exponent, b->exponent);

    float_normalize(&temp);
    float_replace(result, &temp);
    return true;
}

bool float_mul_int(Float *result, Float *a, Int *b) {
    Float rhs = FloatInit(result->significand.bits.allocator);

    if (!float_try_from_int_value(&rhs, b)) {
        return false;
    }
    bool ok = float_mul(result, a, &rhs);
    FloatDeinit(&rhs);
    return ok;
}

bool float_mul_u64(Float *result, Float *a, u64 b) {
    Float rhs = FloatInit(result->significand.bits.allocator);

    if (!float_try_from_u64_value(&rhs, b, result->significand.bits.allocator)) {
        return false;
    }
    bool ok = float_mul(result, a, &rhs);
    FloatDeinit(&rhs);
    return ok;
}

bool float_mul_i64(Float *result, Float *a, i64 b) {
    Float rhs = FloatInit(result->significand.bits.allocator);

    if (!float_try_from_i64_value(&rhs, b, result->significand.bits.allocator)) {
        return false;
    }
    bool ok = float_mul(result, a, &rhs);
    FloatDeinit(&rhs);
    return ok;
}

bool float_mul_f32(Float *result, Float *a, float b) {
    Float rhs = FloatInit(result->significand.bits.allocator);

    if (!float_try_from_f32_value(&rhs, b, result->significand.bits.allocator)) {
        return false;
    }
    bool ok = float_mul(result, a, &rhs);
    FloatDeinit(&rhs);
    return ok;
}

bool float_mul_f64(Float *result, Float *a, double b) {
    Float rhs = FloatInit(result->significand.bits.allocator);

    if (!float_try_from_f64_value(&rhs, b, result->significand.bits.allocator)) {
        return false;
    }
    bool ok = float_mul(result, a, &rhs);
    FloatDeinit(&rhs);
    return ok;
}

bool float_div(Float *result, Float *a, Float *b, u64 precision) {
    Float temp   = FloatInit(result->significand.bits.allocator);
    Int   scale  = IntInit(result->significand.bits.allocator);
    Int   scaled = IntInit(result->significand.bits.allocator);

    ValidateFloat(result);
    ValidateFloat(a);
    ValidateFloat(b);

    if (FloatIsZero(b)) {
        LOG_ERROR("Division by zero");
        return false;
    }
    if (FloatIsZero(a)) {
        Float zero = FloatInit(result->significand.bits.allocator);

        FloatDeinit(result);
        *result = zero;
        return true;
    }

    if (!float_pow10(&scale, precision, result->significand.bits.allocator) ||
        !int_mul(&scaled, &a->significand, &scale)) {
        IntDeinit(&scale);
        IntDeinit(&scaled);
        FloatDeinit(&temp);
        return false;
    }
    if (!int_div(&temp.significand, &scaled, &b->significand)) {
        IntDeinit(&scale);
        IntDeinit(&scaled);
        FloatDeinit(&temp);
        return false;
    }

    temp.negative = FloatIsNegative(a) != FloatIsNegative(b);
    temp.exponent = float_sub_i64_checked(float_sub_i64_checked(a->exponent, b->exponent), (i64)precision);

    IntDeinit(&scale);
    IntDeinit(&scaled);

    float_normalize(&temp);
    float_replace(result, &temp);
    return true;
}

bool float_div_int(Float *result, Float *a, Int *b, u64 precision) {
    Float rhs = FloatInit(result->significand.bits.allocator);
    bool  ok  = false;

    if (!float_try_from_int_value(&rhs, b)) {
        return false;
    }
    ok = float_div(result, a, &rhs, precision);
    FloatDeinit(&rhs);
    return ok;
}

bool float_div_u64(Float *result, Float *a, u64 b, u64 precision) {
    Float rhs = FloatInit(result->significand.bits.allocator);
    bool  ok  = false;

    if (!float_try_from_u64_value(&rhs, b, result->significand.bits.allocator)) {
        return false;
    }
    ok = float_div(result, a, &rhs, precision);
    FloatDeinit(&rhs);
    return ok;
}

bool float_div_i64(Float *result, Float *a, i64 b, u64 precision) {
    Float rhs = FloatInit(result->significand.bits.allocator);
    bool  ok  = false;

    if (!float_try_from_i64_value(&rhs, b, result->significand.bits.allocator)) {
        return false;
    }
    ok = float_div(result, a, &rhs, precision);
    FloatDeinit(&rhs);
    return ok;
}

bool float_div_f32(Float *result, Float *a, float b, u64 precision) {
    Float rhs = FloatInit(result->significand.bits.allocator);
    bool  ok  = false;

    if (!float_try_from_f32_value(&rhs, b, result->significand.bits.allocator)) {
        return false;
    }
    ok = float_div(result, a, &rhs, precision);
    FloatDeinit(&rhs);
    return ok;
}

bool float_div_f64(Float *result, Float *a, double b, u64 precision) {
    Float rhs = FloatInit(result->significand.bits.allocator);
    bool  ok  = false;

    if (!float_try_from_f64_value(&rhs, b, result->significand.bits.allocator)) {
        return false;
    }
    ok = float_div(result, a, &rhs, precision);
    FloatDeinit(&rhs);
    return ok;
}
