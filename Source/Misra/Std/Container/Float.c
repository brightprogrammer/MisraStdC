/// file      : std/container/float.c
/// author    : Generated following Misra project patterns
/// This is free and unencumbered software released into the public domain.
///
/// Arbitrary-precision decimal floating-point implementation built on top of Int.

#include <Misra/Std/Container/Float.h>
#include <Misra/Std/Container/Int.h>
#include <Misra/Std/Log.h>

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void float_normalize(Float *value);
static void float_replace(Float *dst, Float *src);
static Int  float_pow10(u64 power);
static void float_scale_to_exponent(Float *value, i64 target_exponent);
static int  float_abs_compare(Float *lhs, Float *rhs);
static i64  float_add_i64_checked(i64 a, i64 b);
static i64  float_sub_i64_checked(i64 a, i64 b);
static Float float_from_f32_value(float value);
static Float float_from_f64_value(double value);

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

static Float float_from_f32_value(float value) {
    Float result   = FloatInit();
    char  text[32] = {0};
    int   len      = snprintf(text, sizeof(text), "%.9g", (double)value);

    if (len < 0 || len >= (int)sizeof(text)) {
        LOG_FATAL("Failed to convert f32 to Float");
    }

    if (!FloatTryFromStr(&result, text)) {
        LOG_FATAL("Failed to parse f32 conversion result");
    }

    return result;
}

static Float float_from_f64_value(double value) {
    Float result   = FloatInit();
    char  text[48] = {0};
    int   len      = snprintf(text, sizeof(text), "%.17g", value);

    if (len < 0 || len >= (int)sizeof(text)) {
        LOG_FATAL("Failed to convert f64 to Float");
    }

    if (!FloatTryFromStr(&result, text)) {
        LOG_FATAL("Failed to parse f64 conversion result");
    }

    return result;
}

static void float_replace(Float *dst, Float *src) {
    FloatDeinit(dst);
    *dst = *src;
}

static Int float_pow10(u64 power) {
    Int base   = IntFromU64(10);
    Int result = IntFromU64(1);

    IntPowU64(&result, &base, power);
    IntDeinit(&base);
    return result;
}

static void float_scale_to_exponent(Float *value, i64 target_exponent) {
    ValidateFloat(value);

    if (FloatIsZero(value)) {
        value->exponent = target_exponent;
        return;
    }
    if (target_exponent > value->exponent) {
        LOG_FATAL("target exponent must not exceed current exponent");
    }
    if (target_exponent == value->exponent) {
        return;
    }

    {
        u64 places = (u64)(value->exponent - target_exponent);
        Int factor = float_pow10(places);
        Int scaled = IntInit();

        IntMul(&scaled, &value->significand, &factor);
        IntDeinit(&factor);
        IntDeinit(&value->significand);

        value->significand = scaled;
        value->exponent    = target_exponent;
    }
}

static int float_abs_compare(Float *lhs, Float *rhs) {
    ValidateFloat(lhs);
    ValidateFloat(rhs);

    if (FloatIsZero(lhs) && FloatIsZero(rhs)) {
        return 0;
    }

    {
        i64   target_exponent = lhs->exponent < rhs->exponent ? lhs->exponent : rhs->exponent;
        Float lhs_scaled      = FloatClone(lhs);
        Float rhs_scaled      = FloatClone(rhs);
        int   cmp             = 0;

        float_scale_to_exponent(&lhs_scaled, target_exponent);
        float_scale_to_exponent(&rhs_scaled, target_exponent);
        cmp = IntCompare(&lhs_scaled.significand, &rhs_scaled.significand);

        FloatDeinit(&lhs_scaled);
        FloatDeinit(&rhs_scaled);
        return cmp;
    }
}

static void float_normalize(Float *value) {
    ValidateFloat(value);

    if (IntIsZero(&value->significand)) {
        value->negative = false;
        value->exponent = 0;
        return;
    }

    while (MISRA_PRIV_IntModU64(&value->significand, 10) == 0) {
        Int quotient = IntInit();

        (void)MISRA_PRIV_IntDivU64Rem(&quotient, &value->significand, 10);
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
    Float clone = FloatInit();

    ValidateFloat(value);
    clone.negative    = value->negative;
    clone.exponent    = value->exponent;
    clone.significand = IntClone(&value->significand);
    return clone;
}

Float FloatFromU64(u64 value) {
    Float result = FloatInit();

    result.significand = IntFromU64(value);
    float_normalize(&result);
    return result;
}

Float FloatFromI64(i64 value) {
    Float result    = FloatInit();
    u64   magnitude = 0;

    if (value < 0) {
        magnitude = (u64)(-(value + 1)) + 1;
    } else {
        magnitude = (u64)value;
    }

    result.significand = IntFromU64(magnitude);
    result.negative    = value < 0 && magnitude != 0;
    float_normalize(&result);
    return result;
}

Float FloatFromInt(Int *value) {
    Float result = FloatInit();

    ValidateInt(value);
    result.significand = IntClone(value);
    float_normalize(&result);
    return result;
}

Float FloatFromF32(float value) {
    return float_from_f32_value(value);
}

Float FloatFromF64(double value) {
    return float_from_f64_value(value);
}

bool FloatToInt(Int *result, Float *value) {
    Int temp = IntInit();

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
        Int factor = IntInit();

        temp = IntClone(&value->significand);
        factor = float_pow10((u64)value->exponent);
        IntMul(&temp, &temp, &factor);

        IntDeinit(&factor);
        IntDeinit(result);
        *result = temp;
        return true;
    }

    {
        u64  places = (u64)(-value->exponent);
        Int  factor = float_pow10(places);
        bool ok     = IntDivExact(&temp, &value->significand, &factor);

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

bool FloatTryFromStr(Float *out, const char *text) {
    Float result       = FloatInit();
    Str   digits       = StrInit();
    size  pos          = 0;
    bool  negative     = false;
    bool  saw_digit    = false;
    bool  saw_decimal  = false;
    i64   fractional   = 0;
    i64   explicit_exp = 0;

    if (!out || !text) {
        LOG_ERROR("Invalid arguments");
        return false;
    }

    ValidateFloat(out);

    if (text[pos] == '+' || text[pos] == '-') {
        negative = text[pos] == '-';
        pos++;
    }

    for (; text[pos] != '\0'; pos++) {
        char ch = text[pos];

        if (ch >= '0' && ch <= '9') {
            StrPushBack(&digits, ch);
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
            char     *endptr = NULL;
            long long parsed = 0;

            pos++;
            if (text[pos] == '\0') {
                LOG_ERROR("Invalid Float exponent");
                goto fail;
            }

            errno  = 0;
            parsed = strtoll(text + pos, &endptr, 10);
            if (errno == ERANGE || endptr == text + pos || *endptr != '\0') {
                LOG_ERROR("Invalid Float exponent");
                goto fail;
            }

            explicit_exp = (i64)parsed;
            pos          = (size)(endptr - text);
            break;
        }

        LOG_ERROR("Invalid Float format");
        goto fail;
    }

    if (!saw_digit) {
        LOG_ERROR("Invalid Float format");
        goto fail;
    }

    if (!IntTryFromStr(&result.significand, digits.data)) {
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

Float FloatFromStr(const char *text) {
    Float result = FloatInit();

    (void)FloatTryFromStr(&result, text);
    return result;
}

Str FloatToStr(Float *value) {
    Str digits = StrInit();
    Str result = StrInit();

    ValidateFloat(value);

    if (FloatIsZero(value)) {
        return StrInitFromZstr("0");
    }

    digits = IntToStr(&value->significand);

    if (value->negative) {
        StrPushBack(&result, '-');
    }

    if (value->exponent >= 0) {
        StrMerge(&result, &digits);

        for (i64 i = 0; i < value->exponent; i++) {
            StrPushBack(&result, '0');
        }
    } else {
        i64 split = (i64)digits.length + value->exponent;

        if (split > 0) {
            for (i64 i = 0; i < split; i++) {
                StrPushBack(&result, digits.data[i]);
            }

            StrPushBack(&result, '.');

            for (u64 i = (u64)split; i < digits.length; i++) {
                StrPushBack(&result, digits.data[i]);
            }
        } else {
            StrPushBackZstr(&result, "0.");

            for (i64 i = 0; i < -split; i++) {
                StrPushBack(&result, '0');
            }

            StrMerge(&result, &digits);
        }
    }

    StrDeinit(&digits);
    return result;
}

int(FloatCompare)(Float *lhs, Float *rhs) {
    int cmp = 0;

    ValidateFloat(lhs);
    ValidateFloat(rhs);

    if (FloatIsZero(lhs) && FloatIsZero(rhs)) {
        return 0;
    }
    if (FloatIsNegative(lhs) != FloatIsNegative(rhs)) {
        return FloatIsNegative(lhs) ? -1 : 1;
    }

    cmp = float_abs_compare(lhs, rhs);
    return FloatIsNegative(lhs) ? -cmp : cmp;
}

int FloatCompareInt(Float *lhs, Int *rhs) {
    Float rhs_value = FloatFromInt(rhs);
    int   cmp       = FloatCompare(lhs, &rhs_value);

    FloatDeinit(&rhs_value);
    return cmp;
}

int FloatCompareU64(Float *lhs, u64 rhs) {
    Float rhs_value = FloatFromU64(rhs);
    int   cmp       = FloatCompare(lhs, &rhs_value);

    FloatDeinit(&rhs_value);
    return cmp;
}

int FloatCompareI64(Float *lhs, i64 rhs) {
    Float rhs_value = FloatFromI64(rhs);
    int   cmp       = FloatCompare(lhs, &rhs_value);

    FloatDeinit(&rhs_value);
    return cmp;
}

int FloatCompareF32(Float *lhs, float rhs) {
    Float rhs_value = float_from_f32_value(rhs);
    int   cmp       = FloatCompare(lhs, &rhs_value);

    FloatDeinit(&rhs_value);
    return cmp;
}

int FloatCompareF64(Float *lhs, double rhs) {
    Float rhs_value = float_from_f64_value(rhs);
    int   cmp       = FloatCompare(lhs, &rhs_value);

    FloatDeinit(&rhs_value);
    return cmp;
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

void(FloatAdd)(Float *result, Float *a, Float *b) {
    Float lhs  = FloatClone(a);
    Float rhs  = FloatClone(b);
    Float temp = FloatInit();
    i64   exp  = 0;

    ValidateFloat(result);
    ValidateFloat(a);
    ValidateFloat(b);

    exp = lhs.exponent < rhs.exponent ? lhs.exponent : rhs.exponent;
    float_scale_to_exponent(&lhs, exp);
    float_scale_to_exponent(&rhs, exp);

    temp.exponent = exp;

    if (lhs.negative == rhs.negative) {
        IntAdd(&temp.significand, &lhs.significand, &rhs.significand);
        temp.negative = lhs.negative;
    } else {
        int cmp = IntCompare(&lhs.significand, &rhs.significand);

        if (cmp > 0) {
            (void)IntSub(&temp.significand, &lhs.significand, &rhs.significand);
            temp.negative = lhs.negative;
        } else if (cmp < 0) {
            (void)IntSub(&temp.significand, &rhs.significand, &lhs.significand);
            temp.negative = rhs.negative;
        } else {
            FloatClear(&temp);
        }
    }

    float_normalize(&temp);
    FloatDeinit(&lhs);
    FloatDeinit(&rhs);
    float_replace(result, &temp);
}

void FloatAddInt(Float *result, Float *a, Int *b) {
    Float rhs = FloatFromInt(b);

    FloatAdd(result, a, &rhs);
    FloatDeinit(&rhs);
}

void FloatAddU64(Float *result, Float *a, u64 b) {
    Float rhs = FloatFromU64(b);

    FloatAdd(result, a, &rhs);
    FloatDeinit(&rhs);
}

void FloatAddI64(Float *result, Float *a, i64 b) {
    Float rhs = FloatFromI64(b);

    FloatAdd(result, a, &rhs);
    FloatDeinit(&rhs);
}

void FloatAddF32(Float *result, Float *a, float b) {
    Float rhs = float_from_f32_value(b);

    FloatAdd(result, a, &rhs);
    FloatDeinit(&rhs);
}

void FloatAddF64(Float *result, Float *a, double b) {
    Float rhs = float_from_f64_value(b);

    FloatAdd(result, a, &rhs);
    FloatDeinit(&rhs);
}

void(FloatSub)(Float *result, Float *a, Float *b) {
    Float rhs = FloatClone(b);

    ValidateFloat(result);
    ValidateFloat(a);
    ValidateFloat(b);

    FloatNegate(&rhs);
    FloatAdd(result, a, &rhs);
    FloatDeinit(&rhs);
}

void FloatSubInt(Float *result, Float *a, Int *b) {
    Float rhs = FloatFromInt(b);

    FloatSub(result, a, &rhs);
    FloatDeinit(&rhs);
}

void FloatSubU64(Float *result, Float *a, u64 b) {
    Float rhs = FloatFromU64(b);

    FloatSub(result, a, &rhs);
    FloatDeinit(&rhs);
}

void FloatSubI64(Float *result, Float *a, i64 b) {
    Float rhs = FloatFromI64(b);

    FloatSub(result, a, &rhs);
    FloatDeinit(&rhs);
}

void FloatSubF32(Float *result, Float *a, float b) {
    Float rhs = float_from_f32_value(b);

    FloatSub(result, a, &rhs);
    FloatDeinit(&rhs);
}

void FloatSubF64(Float *result, Float *a, double b) {
    Float rhs = float_from_f64_value(b);

    FloatSub(result, a, &rhs);
    FloatDeinit(&rhs);
}

void(FloatMul)(Float *result, Float *a, Float *b) {
    Float temp = FloatInit();

    ValidateFloat(result);
    ValidateFloat(a);
    ValidateFloat(b);

    IntMul(&temp.significand, &a->significand, &b->significand);
    temp.negative = FloatIsNegative(a) != FloatIsNegative(b);
    temp.exponent = float_add_i64_checked(a->exponent, b->exponent);

    float_normalize(&temp);
    float_replace(result, &temp);
}

void FloatMulInt(Float *result, Float *a, Int *b) {
    Float rhs = FloatFromInt(b);

    FloatMul(result, a, &rhs);
    FloatDeinit(&rhs);
}

void FloatMulU64(Float *result, Float *a, u64 b) {
    Float rhs = FloatFromU64(b);

    FloatMul(result, a, &rhs);
    FloatDeinit(&rhs);
}

void FloatMulI64(Float *result, Float *a, i64 b) {
    Float rhs = FloatFromI64(b);

    FloatMul(result, a, &rhs);
    FloatDeinit(&rhs);
}

void FloatMulF32(Float *result, Float *a, float b) {
    Float rhs = float_from_f32_value(b);

    FloatMul(result, a, &rhs);
    FloatDeinit(&rhs);
}

void FloatMulF64(Float *result, Float *a, double b) {
    Float rhs = float_from_f64_value(b);

    FloatMul(result, a, &rhs);
    FloatDeinit(&rhs);
}

bool(FloatDiv)(Float *result, Float *a, Float *b, u64 precision) {
    Float temp   = FloatInit();
    Int   scale  = IntInit();
    Int   scaled = IntInit();

    ValidateFloat(result);
    ValidateFloat(a);
    ValidateFloat(b);

    if (FloatIsZero(b)) {
        LOG_ERROR("Division by zero");
        return false;
    }
    if (FloatIsZero(a)) {
        Float zero = FloatInit();

        FloatDeinit(result);
        *result = zero;
        return true;
    }

    scale = float_pow10(precision);
    IntMul(&scaled, &a->significand, &scale);
    if (!IntDiv(&temp.significand, &scaled, &b->significand)) {
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

bool FloatDivInt(Float *result, Float *a, Int *b, u64 precision) {
    Float rhs = FloatFromInt(b);
    bool  ok  = FloatDiv(result, a, &rhs, precision);

    FloatDeinit(&rhs);
    return ok;
}

bool FloatDivU64(Float *result, Float *a, u64 b, u64 precision) {
    Float rhs = FloatFromU64(b);
    bool  ok  = FloatDiv(result, a, &rhs, precision);

    FloatDeinit(&rhs);
    return ok;
}

bool FloatDivI64(Float *result, Float *a, i64 b, u64 precision) {
    Float rhs = FloatFromI64(b);
    bool  ok  = FloatDiv(result, a, &rhs, precision);

    FloatDeinit(&rhs);
    return ok;
}

bool FloatDivF32(Float *result, Float *a, float b, u64 precision) {
    Float rhs = float_from_f32_value(b);
    bool  ok  = FloatDiv(result, a, &rhs, precision);

    FloatDeinit(&rhs);
    return ok;
}

bool FloatDivF64(Float *result, Float *a, double b, u64 precision) {
    Float rhs = float_from_f64_value(b);
    bool  ok  = FloatDiv(result, a, &rhs, precision);

    FloatDeinit(&rhs);
    return ok;
}
