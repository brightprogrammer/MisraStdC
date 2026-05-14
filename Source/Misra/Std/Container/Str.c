/// file      : std/str.c
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Str implementation

#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <Misra/Std/Container/Str.h>
#include <Misra/Std/Container/Str/Private.h>
#include <Misra/Std/Container/Vec/Private.h>
#include <Misra/Std/Log.h>
#include <Misra/Types.h>

#include "Misra/Std/Utility/StrIter.h"

static Str *string_va_printf(Str *str, const char *fmt, va_list args);

bool StrTryInitFromCstrAlloc(Str *out, const char *cstr, size len, Allocator *alloc) {
    if (!out || !cstr) {
        LOG_FATAL("Invalid arguments");
    }

    *out = str_init_alloc(alloc);
    if (len == 0) {
        return true;
    }

    if (!StrReserve(out, len)) {
        return false;
    }

    MemCopy(out->data, cstr, len);
    out->data[len] = 0;
    out->length    = len;
    return true;
}

Str StrInitFromCstrAlloc(const char *cstr, size len, Allocator *alloc) {
    Str result = str_init_alloc(alloc);

    if (!StrTryInitFromCstrAlloc(&result, cstr, len, alloc)) {
        return result;
    }

    return result;
}

Str *StrPrintf(Str *str, const char *fmt, ...) {
    ValidateStr(str);

    StrClear(str);

    va_list args;
    va_start(args, fmt);
    str = string_va_printf(str, fmt, args);
    va_end(args);

    return str;
}

Str *StrAppendf(Str *str, const char *fmt, ...) {
    ValidateStr(str);

    va_list args;
    va_start(args, fmt);
    str = string_va_printf(str, fmt, args);
    va_end(args);

    return str;
}

static Str *string_va_printf(Str *str, const char *fmt, va_list args) {
    ValidateStr(str);

    va_list args_copy;
    va_copy(args_copy, args);

    // Get size of new string to be added to "str" object.
    size n = vsnprintf(NULL, 0, fmt, args);
    if (!n) {
        LOG_ERROR("invalid size of final string.");
        return NULL;
    }

    // Make more space if required
    if (!StrReserve(str, str->length + n + 1)) {
        va_end(args_copy);
        return NULL;
    }

    // do formatted print at end of string
    vsnprintf(str->data + str->length, n + 1, fmt, args_copy);

    str->length            += n;
    str->data[str->length]  = 0; // null terminate

    va_end(args_copy);

    return str;
}

bool StrInitCopy(Str *dst, const Str *src) {
    if (!dst || !src) {
        LOG_FATAL("Invalid arguments");
    }

    return StrInitCopyAlloc(dst, src, src->allocator);
}

bool StrInitCopyAlloc(void *dst_ptr, const void *src_ptr, const Allocator *alloc) {
    Str       *dst             = (Str *)dst_ptr;
    const Str *src             = (const Str *)src_ptr;
    Allocator *clone_allocator = NULL;

    ValidateStr(src);
    if (!dst) {
        LOG_FATAL("Invalid arguments");
    }

    clone_allocator = alloc ? (Allocator *)alloc : src->allocator;

    MemSet(dst, 0, sizeof(Str));
    *dst             = str_init_alloc(clone_allocator);
    dst->copy_init   = src->copy_init;
    dst->copy_deinit = src->copy_deinit;

    if (!insert_range_into_vec(GENERIC_VEC(dst), src->data, sizeof(char), 0, src->length)) {
        return false;
    }

    return true;
}

void StrDeinit(Str *copy) {
    ValidateStr(copy);
    deinit_vec(GENERIC_VEC(copy), sizeof(char));
}

void StrDeinitAlloc(void *copy, const Allocator *alloc) {
    (void)alloc;
    StrDeinit((Str *)copy);
}

StrIters StrSplitToIters(Str *s, const char *key) {
    ValidateStr(s);

    StrIters    sv     = (StrIters)vec_init_alloc(s->allocator);
    size        keylen = ZstrLen(key);
    const char *prev   = s->data;
    const char *end    = s->data + s->length;

    while (prev <= end) {
        const char *next = ZstrFindSubstring(prev, key);
        if (next) {
            StrIter si = {.data = (char *)prev, .length = next - prev, .pos = 0, .alignment = 1};
            VecPushBack(&sv, si);
            prev = next + keylen;
        } else {
            StrIter si = {.data = (char *)prev, .length = end - prev, .pos = 0, .alignment = 1};
            VecPushBack(&sv, si);
            break;
        }
    }

    return sv;
}

Strs StrSplit(Str *s, const char *key) {
    ValidateStr(s);

    Strs        sv     = (Strs)vec_init_alloc(s->allocator);
    sv.copy_deinit     = (GenericCopyDeinit)StrDeinitAlloc;
    size        keylen = ZstrLen(key);
    const char *prev   = s->data;

    if (prev) {
        const char *end = s->data + s->length;
        while (prev <= end) {
            const char *next = ZstrFindSubstring(prev, key);
            if (next) {
                Str tmp = StrInitFromCstrAlloc(prev, next - prev, s->allocator);
                VecPushBack(&sv, tmp);
                prev = next + keylen;
            } else {
                if (ZstrCompareN(prev, key, end - prev)) {
                    Str tmp = StrInitFromCstrAlloc(prev, end - prev, s->allocator);
                    VecPushBack(&sv, tmp);
                }
                break;
            }
        }
    }

    return sv;
}

static size str_index_of_cstr(const Str *s, const char *key, size key_len) {
    const char *found = NULL;

    ValidateStr(s);

    if (!key) {
        LOG_FATAL("Invalid arguments");
    }

    if (key_len == 0) {
        return 0;
    }

    if (!s->data || s->length < key_len) {
        return SIZE_MAX;
    }

    found = ZstrFindSubstringN(s->data, key, key_len);
    return found ? (size)(found - s->data) : SIZE_MAX;
}

bool StrContains(const Str *s, const Str *key) {
    if (!key) {
        LOG_FATAL("Invalid arguments");
    }

    ValidateStr(key);

    if (key->length == 0) {
        return true;
    }

    return StrContainsCstr(s, key->data, key->length);
}

size StrIndexOfZstr(const Str *s, const char *key) {
    if (!key) {
        LOG_FATAL("Invalid arguments");
    }

    return str_index_of_cstr(s, key, ZstrLen(key));
}

size StrIndexOfCstr(const Str *s, const char *key, size key_len) {
    return str_index_of_cstr(s, key, key_len);
}

size StrIndexOf(const Str *s, const Str *key) {
    if (!key) {
        LOG_FATAL("Invalid arguments");
    }

    ValidateStr(key);

    if (key->length == 0) {
        return 0;
    }

    return str_index_of_cstr(s, key->data, key->length);
}

bool StrContainsZstr(const Str *s, const char *key) {
    return StrIndexOfZstr(s, key) != SIZE_MAX;
}

bool StrContainsCstr(const Str *s, const char *key, size key_len) {
    return StrIndexOfCstr(s, key, key_len) != SIZE_MAX;
}

// Helper function to check if char is in strip_chars
static inline bool is_strip_char(char c, const char *strip_chars) {
    const char *p = strip_chars;
    while (*p) {
        if (c == *p)
            return true;
        p++;
    }
    return false;
}

// split direction = 0 means both sides
//                 = -1 means from left
//                 = 1 means from right
Str strip_str(Str *s, const char *chars_to_strip, int split_direction) {
    ValidateStr(s);

    const char *strip_chars = chars_to_strip ? chars_to_strip : " \t\n\r\v\f";
    const char *start       = s->data;
    const char *end         = s->data + s->length - 1;

    // Trim from the left
    if (split_direction <= 0) {
        while (start <= end && is_strip_char(*start, strip_chars)) {
            start++;
        }
    }

    // Trim from the right
    if (split_direction >= 0) {
        while (end >= start && is_strip_char(*end, strip_chars)) {
            end--;
        }
    }

    size new_len = end >= start ? (end - start + 1) : 0;
    return StrInitFromCstrAlloc(start, new_len, s->allocator);
}

static inline bool starts_with(const char *data, size data_len, const char *prefix, size prefix_len) {
    return data_len >= prefix_len && MemCompare(data, prefix, prefix_len) == 0;
}

static inline bool ends_with(const char *data, size data_len, const char *suffix, size suffix_len) {
    return data_len >= suffix_len && MemCompare(data + data_len - suffix_len, suffix, suffix_len) == 0;
}

bool StrStartsWithZstr(const Str *s, const char *prefix) {
    ValidateStr(s);
    return starts_with(s->data, s->length, prefix, ZstrLen(prefix));
}

bool StrEndsWithZstr(const Str *s, const char *suffix) {
    ValidateStr(s);
    return ends_with(s->data, s->length, suffix, ZstrLen(suffix));
}

bool StrStartsWithCstr(const Str *s, const char *prefix, size prefix_len) {
    ValidateStr(s);
    return starts_with(s->data, s->length, prefix, prefix_len);
}

bool StrEndsWithCstr(const Str *s, const char *suffix, size suffix_len) {
    ValidateStr(s);
    return ends_with(s->data, s->length, suffix, suffix_len);
}

bool StrStartsWith(const Str *s, const Str *prefix) {
    ValidateStr(s);
    return starts_with(s->data, s->length, prefix->data, prefix->length);
}

bool StrEndsWith(const Str *s, const Str *suffix) {
    ValidateStr(s);
    return ends_with(s->data, s->length, suffix->data, suffix->length);
}

// Helper: replace in-place all `match` → `replacement` up to `count`
static void
    str_replace(Str *s, const char *match, size match_len, const char *replacement, size replacement_len, size count) {
    ValidateStr(s);
    size i        = 0;
    size replaced = 0;

    while (i + match_len <= s->length && replaced < count) {
        if (MemCompare(s->data + i, match, match_len) == 0) {
            StrDeleteRange(s, i, match_len);
            StrInsertCstr(s, replacement, i, replacement_len);
            i        += replacement_len;
            replaced += 1;
        } else {
            i++;
        }
    }
}

void StrReplaceZstr(Str *s, const char *match, const char *replacement, size count) {
    ValidateStr(s);
    str_replace(s, match, ZstrLen(match), replacement, ZstrLen(replacement), count);
}

void StrReplaceCstr(
    Str        *s,
    const char *match,
    size        match_len,
    const char *replacement,
    size        replacement_len,
    size        count
) {
    ValidateStr(s);
    str_replace(s, match, match_len, replacement, replacement_len, count);
}

void StrReplace(Str *s, const Str *match, const Str *replacement, size count) {
    ValidateStr(s);
    str_replace(s, match->data, match->length, replacement->data, replacement->length, count);
}

// Helper function to convert a single digit to character
static inline char digit_to_char(u8 digit, bool uppercase) {
    if (digit < 10)
        return '0' + digit;
    return (uppercase ? 'A' : 'a') + (digit - 10);
}

// Helper function to convert character to digit
static inline bool char_to_digit(char c, u8 *digit, u8 base) {
    if (IS_DIGIT(c)) {
        *digit = c - '0';
    } else if (IN_RANGE(c, 'a', 'z')) {
        *digit = c - 'a' + 10;
    } else if (IN_RANGE(c, 'A', 'Z')) {
        *digit = c - 'A' + 10;
    } else {
        return false;
    }
    return *digit < base;
}

// Helper function to validate base values
static inline bool is_valid_base(u8 base) {
    return base != 1 && base <= 36;
}

// Helper function to skip prefixes for explicit bases
static inline size_t skip_prefix(const Str *str, size_t pos, u8 base) {
    if (pos + 2 > str->length || str->data[pos] != '0') {
        return pos;
    }

    char prefix_char = str->data[pos + 1];

    switch (base) {
        case 2 : // Binary: "0b" or "0B"
            if (prefix_char == 'b' || prefix_char == 'B') {
                return pos + 2;
            }
            break;

        case 8 : // Octal: "0o" or "0O"
            if (prefix_char == 'o' || prefix_char == 'O') {
                return pos + 2;
            }
            break;

        case 16 : // Hexadecimal: "0x" or "0X"
            if (prefix_char == 'x' || prefix_char == 'X') {
                return pos + 2;
            }
            break;

        default :
            break;
    }

    return pos;
}

// ======================================
// Conversion Functions - New Clean Implementation
// ======================================

Str *StrFromU64(Str *str, u64 value, const StrIntFormat *config) {
    ValidateStr(str);

    if (!config) {
        config = &STR_INT_DEFAULT;
    }

    if (!is_valid_base(config->base)) {
        LOG_ERROR("Invalid base: {}", config->base);
        return NULL;
    }

    StrClear(str);

    // Add prefix if requested
    if (config->use_prefix) {
        if (!StrPushBack(str, '0')) {
            return NULL;
        }
        if (config->base == 2) {
            if (!StrPushBack(str, 'b')) {
                return NULL;
            }
        } else if (config->base == 8) {
            if (!StrPushBack(str, 'o')) {
                return NULL;
            }
        } else if (config->base == 16) {
            if (!StrPushBack(str, 'x')) {
                return NULL;
            }
        }
    }

    // Convert number to string
    if (value == 0) {
        if (!StrPushBack(str, '0')) {
            return NULL;
        }
    } else {
        char   buffer[65];
        size_t pos = 0;

        while (value > 0) {
            buffer[pos++]  = digit_to_char(value % config->base, config->uppercase);
            value         /= config->base;
        }

        // Add digits in correct order
        while (pos > 0) {
            if (!StrPushBack(str, buffer[--pos])) {
                return NULL;
            }
        }
    }

    return str;
}

Str *StrFromI64(Str *str, i64 value, const StrIntFormat *config) {
    ValidateStr(str);

    if (!config) {
        config = &STR_INT_DEFAULT;
    }

    if (!is_valid_base(config->base)) {
        LOG_ERROR("Invalid base: {}", config->base);
        return NULL;
    }

    // Handle negative numbers
    bool is_negative = value < 0;
    u64  abs_value;

    if (value == INT64_MIN) {
        abs_value = (u64)INT64_MIN;
    } else {
        abs_value = is_negative ? (u64)(-value) : (u64)value;
    }

    // Use StrFromU64 for the conversion
    if (!StrFromU64(str, abs_value, config)) {
        return NULL;
    }

    // Add sign for negative decimal numbers AFTER conversion
    if (is_negative && config->base == 10) {
        // Insert the negative sign at the beginning
        if (!StrInsertCharAt(str, '-', 0)) {
            return NULL;
        }
    }

    return str;
}

Str *StrFromF64(Str *str, f64 value, const StrFloatFormat *config) {
    ValidateStr(str);

    if (!config) {
        config = &STR_FLOAT_DEFAULT;
    }

    if (config->precision > 17) {
        LOG_ERROR("Precision {} exceeds maximum (17)", config->precision);
        return NULL;
    }

    StrClear(str);

    // Handle special cases
    if (isnan(value)) {
        const char *nan_str = config->uppercase ? "NAN" : "nan";
        for (size_t i = 0; i < 3; i++) {
            if (!StrPushBack(str, nan_str[i])) {
                return NULL;
            }
        }
        return str;
    }

    if (isinf(value)) {
        if (value < 0) {
            if (!StrPushBack(str, '-')) {
                return NULL;
            }
        } else if (config->always_sign) {
            if (!StrPushBack(str, '+')) {
                return NULL;
            }
        }
        const char *inf_str = config->uppercase ? "INF" : "inf";
        for (size_t i = 0; i < 3; i++) {
            if (!StrPushBack(str, inf_str[i])) {
                return NULL;
            }
        }
        return str;
    }

    // Handle sign
    if (value < 0) {
        if (!StrPushBack(str, '-')) {
            return NULL;
        }
        value = -value;
    } else if (config->always_sign) {
        if (!StrPushBack(str, '+')) {
            return NULL;
        }
    }

    // Simple implementation for now
    bool use_sci = config->force_sci || (value != 0.0 && (value < 0.0001 || value >= 1e7));

    if (use_sci) {
        // Scientific notation
        int exp      = 0;
        f64 mantissa = value;

        if (mantissa != 0.0) {
            while (mantissa >= 10.0) {
                mantissa /= 10.0;
                exp++;
            }
            while (mantissa < 1.0) {
                mantissa *= 10.0;
                exp--;
            }
        }

        // Format mantissa with proper rounding
        i64 int_part = (i64)mantissa;
        if (!StrPushBack(str, '0' + int_part)) {
            return NULL;
        }

        if (config->precision > 0) {
            if (!StrPushBack(str, '.')) {
                return NULL;
            }
            f64 frac_part = mantissa - int_part;

            for (u8 i = 0; i < config->precision; i++) {
                frac_part *= 10.0;
                int digit  = (int)(frac_part + 0.5); // Round each digit individually
                if (digit >= 10)
                    digit = 9;                       // Clamp to prevent overflow
                if (!StrPushBack(str, '0' + digit)) {
                    return NULL;
                }
                frac_part -= (int)frac_part; // Remove the integer part
            }
        }

        // Add exponent
        if (!StrPushBack(str, config->uppercase ? 'E' : 'e')) {
            return NULL;
        }
        if (exp >= 0) {
            if (!StrPushBack(str, '+')) {
                return NULL;
            }
        } else {
            if (!StrPushBack(str, '-')) {
                return NULL;
            }
            exp = -exp;
        }

        // Format exponent digits (always at least 2 digits)
        if (exp == 0) {
            if (!StrPushBack(str, '0') || !StrPushBack(str, '0')) {
                return NULL;
            }
        } else {
            char   exp_buf[8];
            size_t exp_pos = 0;
            while (exp > 0) {
                exp_buf[exp_pos++]  = '0' + (exp % 10);
                exp                /= 10;
            }
            // Pad to at least 2 digits
            while (exp_pos < 2) {
                exp_buf[exp_pos++] = '0';
            }
            while (exp_pos > 0) {
                if (!StrPushBack(str, exp_buf[--exp_pos])) {
                    return NULL;
                }
            }
        }
    } else {
        // Standard notation
        i64 int_part = (i64)value;

        // Format integer part
        if (int_part == 0) {
            if (!StrPushBack(str, '0')) {
                return NULL;
            }
        } else {
            char   int_buf[32];
            size_t int_pos = 0;
            while (int_part > 0) {
                int_buf[int_pos++]  = '0' + (int_part % 10);
                int_part           /= 10;
            }
            while (int_pos > 0) {
                if (!StrPushBack(str, int_buf[--int_pos])) {
                    return NULL;
                }
            }
        }

        // Format fractional part
        if (config->precision > 0) {
            if (!StrPushBack(str, '.')) {
                return NULL;
            }

            // Apply rounding to the entire fractional part first
            f64 scale = 1.0;
            for (u8 i = 0; i < config->precision; i++) {
                scale *= 10.0;
            }
            f64 rounded_value = round(value * scale) / scale;
            f64 frac_part     = rounded_value - (i64)rounded_value;

            for (u8 i = 0; i < config->precision; i++) {
                frac_part *= 10.0;
                int digit  = (int)frac_part;

                // Handle floating-point precision errors: if we're very close to the next integer, round up
                if (frac_part - digit > 0.999999) {
                    digit++;
                }

                if (!StrPushBack(str, '0' + digit)) {
                    return NULL;
                }
                frac_part -= digit;
            }
        }
    }

    return str;
}

bool StrToU64(const Str *str, u64 *value, const StrParseConfig *config) {
    ValidateStr(str);

    if (!value) {
        LOG_ERROR("NULL output pointer");
        return false;
    }

    if (!config) {
        config = &STR_PARSE_DEFAULT;
    }

    u8 base = config->base;
    if (base != 0 && !is_valid_base(base)) {
        LOG_ERROR("Invalid base: {}", base);
        return false;
    }

    // Skip whitespace
    size_t pos = 0;
    while (pos < str->length && IS_SPACE(str->data[pos]))
        pos++;

    if (pos >= str->length) {
        LOG_ERROR("Empty string");
        return false;
    }

    // Auto-detect base
    if (base == 0) {
        base = 10;
        if (pos + 2 <= str->length && str->data[pos] == '0') {
            char prefix = str->data[pos + 1];
            if (prefix == 'x' || prefix == 'X') {
                base  = 16;
                pos  += 2;
            } else if (prefix == 'b' || prefix == 'B') {
                base  = 2;
                pos  += 2;
            } else if (prefix == 'o' || prefix == 'O') {
                base  = 8;
                pos  += 2;
            }
        }
    } else {
        pos = skip_prefix(str, pos, base);
    }

    // Convert digits
    u64  result      = 0;
    bool have_digits = false;

    while (pos < str->length) {
        u8 digit;
        if (!char_to_digit(str->data[pos], &digit, base)) {
            if (IS_SPACE(str->data[pos]))
                break;
            LOG_ERROR("Invalid digit for base {}: {c}", base, str->data[pos]);
            return false;
        }

        // Check overflow
        if (result > (UINT64_MAX - digit) / base) {
            LOG_ERROR("Overflow");
            return false;
        }

        result      = result * base + digit;
        have_digits = true;
        pos++;
    }

    // Skip trailing whitespace
    while (pos < str->length && IS_SPACE(str->data[pos]))
        pos++;

    if (config->strict && pos < str->length) {
        LOG_ERROR("Extra characters after number");
        return false;
    }

    if (!have_digits) {
        LOG_ERROR("No valid digits found");
        return false;
    }

    *value = result;
    return true;
}

bool StrToI64(const Str *str, i64 *value, const StrParseConfig *config) {
    ValidateStr(str);

    if (!value) {
        LOG_ERROR("NULL output pointer");
        return false;
    }

    if (!config) {
        config = &STR_PARSE_DEFAULT;
    }

    // Skip whitespace
    size_t pos = 0;
    while (pos < str->length && IS_SPACE(str->data[pos]))
        pos++;

    if (pos >= str->length) {
        LOG_ERROR("Empty string");
        return false;
    }

    // Handle sign
    bool negative = false;
    if (str->data[pos] == '-') {
        negative = true;
        pos++;
    } else if (str->data[pos] == '+') {
        pos++;
    }

    // Create substring view without sign (no allocation - data is borrowed
    // from the input str and capacity stays 0 so StrToU64 never tries to
    // grow it).
    Str temp_str      = str_init_alloc(str->allocator);
    temp_str.data     = str->data + pos;
    temp_str.length   = str->length - pos;
    temp_str.capacity = str->length - pos;

    u64 unsigned_value;
    if (!StrToU64(&temp_str, &unsigned_value, config)) {
        return false;
    }

    // Check overflow
    if (negative) {
        if (unsigned_value > 9223372036854775808ULL) {
            LOG_ERROR("Overflow");
            return false;
        }
        *value = -(i64)unsigned_value;
    } else {
        if (unsigned_value > 9223372036854775807ULL) {
            LOG_ERROR("Overflow");
            return false;
        }
        *value = (i64)unsigned_value;
    }

    return true;
}

bool StrToF64(const Str *str, f64 *value, const StrParseConfig *config) {
    ValidateStr(str);

    if (!value) {
        LOG_ERROR("NULL output pointer");
        return false;
    }

    if (!config) {
        config = &STR_PARSE_DEFAULT;
    }

    // Skip whitespace
    size_t pos = 0;
    while (pos < str->length && IS_SPACE(str->data[pos]))
        pos++;

    if (pos >= str->length) {
        LOG_ERROR("Empty string");
        return false;
    }

    // Check for special values
    if (str->length - pos >= 3) {
        char c1 = TO_LOWER(str->data[pos]);
        char c2 = TO_LOWER(str->data[pos + 1]);
        char c3 = TO_LOWER(str->data[pos + 2]);

        if (c1 == 'n' && c2 == 'a' && c3 == 'n') {
            if (str->length - pos == 3 || IS_SPACE(str->data[pos + 3])) {
                *value = NAN;
                return true;
            }
        }
        if (c1 == 'i' && c2 == 'n' && c3 == 'f') {
            if (str->length - pos == 3 || IS_SPACE(str->data[pos + 3])) {
                *value = INFINITY;
                return true;
            }
        }
    }

    // Handle sign
    bool negative = false;
    if (str->data[pos] == '-') {
        negative = true;
        pos++;

        // Check for "-inf"
        if (str->length - pos >= 3) {
            char c1 = TO_LOWER(str->data[pos]);
            char c2 = TO_LOWER(str->data[pos + 1]);
            char c3 = TO_LOWER(str->data[pos + 2]);

            if (c1 == 'i' && c2 == 'n' && c3 == 'f') {
                if (str->length - pos == 3 || IS_SPACE(str->data[pos + 3])) {
                    *value = -INFINITY;
                    return true;
                }
            }
        }
    } else if (str->data[pos] == '+') {
        pos++;
    }

    // Parse number
    f64  result      = 0.0;
    bool have_digits = false;

    // Parse integer part
    while (pos < str->length && IS_DIGIT(str->data[pos])) {
        result      = result * 10.0 + (str->data[pos] - '0');
        have_digits = true;
        pos++;
    }

    // Parse fractional part
    if (pos < str->length && str->data[pos] == '.') {
        pos++;
        f64 scale = 0.1;

        while (pos < str->length && IS_DIGIT(str->data[pos])) {
            result      += (str->data[pos] - '0') * scale;
            scale       *= 0.1;
            have_digits  = true;
            pos++;
        }
    }

    // Parse exponent
    if (pos < str->length && (str->data[pos] == 'e' || str->data[pos] == 'E')) {
        pos++;

        bool exp_negative = false;
        if (pos < str->length) {
            if (str->data[pos] == '-') {
                exp_negative = true;
                pos++;
            } else if (str->data[pos] == '+') {
                pos++;
            }
        }

        i32  exponent        = 0;
        bool have_exp_digits = false;

        while (pos < str->length && IS_DIGIT(str->data[pos])) {
            exponent        = exponent * 10 + (str->data[pos] - '0');
            have_exp_digits = true;
            pos++;
        }

        if (!have_exp_digits) {
            LOG_ERROR("Missing exponent digits");
            return false;
        }

        if (exp_negative)
            exponent = -exponent;
        result *= pow(10.0, exponent);
    }

    // Skip trailing whitespace
    while (pos < str->length && IS_SPACE(str->data[pos]))
        pos++;

    if (config->strict && pos < str->length) {
        LOG_ERROR("Extra characters after number");
        return false;
    }

    if (!have_digits) {
        LOG_ERROR("No valid digits found");
        return false;
    }

    *value = negative ? -result : result;
    return true;
}

void ValidateStr(const Str *s) {
    return ValidateVec(s);
}

void ValidateStrs(const Strs *vs) {
    ValidateVec(vs);
    VecForeachPtr(vs, sp) {
        ValidateStr(sp);
    }
}

// ======================================
// Configuration Structures
// ======================================

// Default configurations
const StrIntFormat STR_INT_DEFAULT =
    {.base = 10, .uppercase = false, .use_prefix = false, .pad_zeros = false, .min_width = 0};

const StrFloatFormat STR_FLOAT_DEFAULT =
    {.precision = 6, .force_sci = false, .uppercase = false, .trim_zeros = false, .always_sign = false};

const StrParseConfig STR_PARSE_DEFAULT = {.strict = false, .trim_space = true, .base = 0};
