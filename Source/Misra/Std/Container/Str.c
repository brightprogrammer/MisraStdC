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
#include <Misra/Std/Log.h>
#include <Misra/Types.h>

#include "Misra/Std/Utility/StrIter.h"

static Str* string_va_printf(Str* str, const char* fmt, va_list args);

Str* StrPrintf(Str* str, const char* fmt, ...) {
    ValidateStr(str);

    StrClear(str);

    va_list args;
    va_start(args, fmt);
    str = string_va_printf(str, fmt, args);
    va_end(args);

    return str;
}

Str* StrAppendf(Str* str, const char* fmt, ...) {
    ValidateStr(str);

    va_list args;
    va_start(args, fmt);
    str = string_va_printf(str, fmt, args);
    va_end(args);

    return str;
}

static Str* string_va_printf(Str* str, const char* fmt, va_list args) {
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
    StrReserve(str, str->length + n + 1);

    // do formatted print at end of string
    vsnprintf(str->data + str->length, n + 1, fmt, args_copy);

    str->length            += n;
    str->data[str->length]  = 0; // null terminate

    va_end(args_copy);

    return str;
}

bool StrInitCopy(Str* dst, const Str* src) {
    ValidateStr(src);

    MemSet(dst, 0, sizeof(Str));
    dst->copy_init   = src->copy_init;
    dst->copy_deinit = src->copy_deinit;
    dst->alignment   = src->alignment;

    VecMergeR(dst, src);
    ValidateStr(dst);

    return true;
}

void StrDeinit(Str* copy) {
    ValidateStr(copy);
    if (copy->data) {
        FREE(copy->data);
    }
    *copy = StrInit();
}

StrIters StrSplitToIters(Str* s, const char* key) {
    ValidateStr(s);

    StrIters sv     = VecInit();
    size     keylen = ZstrLen(key);

    const char* prev = s->data;
    const char* end  = s->data + s->length;

    while (prev <= end) {
        const char* next = ZstrFindSubstring(prev, key);
        if (next) {
            StrIter si = {.data = (char*)prev, .length = next - prev, .pos = 0, .alignment = 1};
            VecPushBack(&sv, si);
            prev = next + keylen; // skip past delimiter
        } else {
            StrIter si = {.data = (char*)prev, .length = end - prev, .pos = 0, .alignment = 1};
            VecPushBack(&sv, si);
            break;
        }
    }

    return sv;
}

Strs StrSplit(Str* s, const char* key) {
    ValidateStr(s);

    Strs sv     = VecInitWithDeepCopy(NULL, StrDeinit);
    size keylen = ZstrLen(key);

    const char* prev = s->data;

    if (prev) {
        const char* end = s->data + s->length;
        while (prev <= end) {
            const char* next = ZstrFindSubstring(prev, key);
            if (next) {
                Str tmp = StrInitFromCstr(prev, next - prev);
                VecPushBack(&sv, tmp); // exclude delimiter
                prev = next + keylen;  // skip past delimiter
            } else {
                if (ZstrCompareN(prev, key, end - prev)) {
                    Str tmp = StrInitFromCstr(prev, end - prev);
                    VecPushBack(&sv, tmp); // remaining part
                }
                break;
            }
        }
    }

    return sv;
}

// Helper function to check if char is in strip_chars
static inline bool is_strip_char(char c, const char* strip_chars) {
    const char* p = strip_chars;
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
Str strip_str(Str* s, const char* chars_to_strip, int split_direction) {
    ValidateStr(s);

    const char* strip_chars = chars_to_strip ? chars_to_strip : " \t\n\r\v\f";
    const char* start       = s->data;
    const char* end         = s->data + s->length - 1;

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
    return StrInitFromCstr(start, new_len);
}

static inline bool starts_with(const char* data, size data_len, const char* prefix, size prefix_len) {
    return data_len >= prefix_len && MemCompare(data, prefix, prefix_len) == 0;
}

static inline bool ends_with(const char* data, size data_len, const char* suffix, size suffix_len) {
    return data_len >= suffix_len && MemCompare(data + data_len - suffix_len, suffix, suffix_len) == 0;
}

bool StrStartsWithZstr(const Str* s, const char* prefix) {
    ValidateStr(s);
    return starts_with(s->data, s->length, prefix, ZstrLen(prefix));
}

bool StrEndsWithZstr(const Str* s, const char* suffix) {
    ValidateStr(s);
    return ends_with(s->data, s->length, suffix, ZstrLen(suffix));
}

bool StrStartsWithCstr(const Str* s, const char* prefix, size prefix_len) {
    ValidateStr(s);
    return starts_with(s->data, s->length, prefix, prefix_len);
}

bool StrEndsWithCstr(const Str* s, const char* suffix, size suffix_len) {
    ValidateStr(s);
    return ends_with(s->data, s->length, suffix, suffix_len);
}

bool StrStartsWith(const Str* s, const Str* prefix) {
    ValidateStr(s);
    return starts_with(s->data, s->length, prefix->data, prefix->length);
}

bool StrEndsWith(const Str* s, const Str* suffix) {
    ValidateStr(s);
    return ends_with(s->data, s->length, suffix->data, suffix->length);
}

// Helper: replace in-place all `match` → `replacement` up to `count`
static void
    str_replace(Str* s, const char* match, size match_len, const char* replacement, size replacement_len, size count) {
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

void StrReplaceZstr(Str* s, const char* match, const char* replacement, size count) {
    ValidateStr(s);
    str_replace(s, match, ZstrLen(match), replacement, ZstrLen(replacement), count);
}

void StrReplaceCstr(
    Str*        s,
    const char* match,
    size        match_len,
    const char* replacement,
    size        replacement_len,
    size        count
) {
    ValidateStr(s);
    str_replace(s, match, match_len, replacement, replacement_len, count);
}

void StrReplace(Str* s, const Str* match, const Str* replacement, size count) {
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
static inline bool char_to_digit(char c, u8* digit, u8 base) {
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

Str* StrFromU64(Str* str, u64 value, u8 base, bool uppercase) {
    ValidateStr(str);

    if (base < 2 || base > 36) {
        LOG_ERROR("Invalid base: %u", base);
        return NULL;
    }

    // Clear the string
    StrClear(str);

    // Handle zero specially
    if (value == 0) {
        // Add prefix for special bases
        if (base == 2) {
            StrPushBack(str, '0');
            StrPushBack(str, 'b');
            StrPushBack(str, '0');
        } else if (base == 8) {
            StrPushBack(str, '0');
            StrPushBack(str, 'o');
            StrPushBack(str, '0');
        } else if (base == 16) {
            StrPushBack(str, '0');
            StrPushBack(str, 'x');
            StrPushBack(str, '0');
        } else {
            StrPushBack(str, '0');
        }
        return str;
    }

    // Convert digits in reverse order
    char   buffer[65]; // Max 64 bits in binary + null terminator
    size_t pos = 0;

    while (value > 0) {
        buffer[pos++]  = digit_to_char(value % base, uppercase);
        value         /= base;
    }

    // Add prefix for special bases
    if (base == 2) {
        StrPushBack(str, '0');
        StrPushBack(str, 'b');
    } else if (base == 8) {
        StrPushBack(str, '0');
        StrPushBack(str, 'o');
    } else if (base == 16) {
        StrPushBack(str, '0');
        StrPushBack(str, 'x');
    }

    // Copy digits in correct order
    while (pos > 0) {
        StrPushBack(str, buffer[--pos]);
    }

    return str;
}

Str* StrFromI64(Str* str, i64 value, u8 base, bool uppercase) {
    ValidateStr(str);

    if (base < 2 || base > 36) {
        LOG_ERROR("Invalid base: %u", base);
        return NULL;
    }

    // Handle negative numbers
    bool is_negative = value < 0;
    u64  abs_value   = is_negative ? (u64)(-value) : (u64)value;

    // Clear the string
    StrClear(str);

    // Add sign for negative numbers only for decimal (base 10)
    if (is_negative && base == 10) {
        StrPushBack(str, '-');
    }

    // Add prefix for special bases
    if (base == 2) {
        StrPushBack(str, '0');
        StrPushBack(str, 'b');
    } else if (base == 8) {
        StrPushBack(str, '0');
        StrPushBack(str, 'o');
    } else if (base == 16) {
        StrPushBack(str, '0');
        StrPushBack(str, 'x');
    }

    // Handle zero specially
    if (abs_value == 0) {
        StrPushBack(str, '0');
        return str;
    }

    // Convert digits in reverse order
    char   buffer[65]; // Max 64 bits in binary + null terminator
    size_t pos = 0;

    while (abs_value > 0) {
        buffer[pos++]  = digit_to_char(abs_value % base, uppercase);
        abs_value     /= base;
    }

    // Copy digits in correct order
    while (pos > 0) {
        StrPushBack(str, buffer[--pos]);
    }

    return str;
}

static void append_fraction(Str* str, f64 frac, u8 precision) {
    if (precision == 0)
        return;

    StrPushBack(str, '.');

    f64 scaled  = frac * pow(10.0, precision);
    i64 rounded = (i64)(scaled + 0.5);

    char   buf[18]; // Max 17 digits + null terminator safety
    size_t pos = 0;

    if (rounded == 0) {
        for (u8 i = 0; i < precision; i++) {
            buf[pos++] = '0';
        }
    } else {
        while (rounded > 0) {
            buf[pos++]  = '0' + (rounded % 10);
            rounded    /= 10;
        }
        while (pos < precision) {
            buf[pos++] = '0';
        }

        // Reverse digits
        for (size_t i = 0; i < pos / 2; i++) {
            char tmp         = buf[i];
            buf[i]           = buf[pos - 1 - i];
            buf[pos - 1 - i] = tmp;
        }
    }

    for (size_t i = 0; i < pos; i++) {
        StrPushBack(str, buf[i]);
    }
}

Str* StrFromF64(Str* str, f64 value, u8 precision, bool force_sci, bool uppercase) {
    ValidateStr(str);
    StrClear(str);

    // Handle special cases first to avoid calculations that could cause crashes
    if (isnan(value)) {
        // Manually push characters instead of using StrPushBackZstr
        const char* nan_str = uppercase ? "NAN" : "nan";
        size_t      len     = 3; // "nan" or "NAN" is 3 characters

        // Ensure we have enough capacity
        if (str->capacity < len) {
            VecReserve(str, len);
        }

        // Manually copy characters
        for (size_t i = 0; i < len; i++) {
            StrPushBack(str, nan_str[i]);
        }

        return str;
    }

    if (isinf(value)) {
        // Manually push characters instead of using StrPushBackZstr
        if (value < 0) {
            StrPushBack(str, '-');
        }

        const char* inf_str = uppercase ? "INF" : "inf";
        size_t      len     = 3; // "inf" or "INF" is 3 characters

        // Ensure we have enough capacity
        size_t total_needed = str->length + len;
        if (str->capacity < total_needed) {
            VecReserve(str, total_needed);
        }

        // Manually copy characters
        for (size_t i = 0; i < len; i++) {
            StrPushBack(str, inf_str[i]);
        }

        return str;
    }

    // Handle signed zero
    if (value == 0.0) {
        if (signbit(value))
            StrPushBack(str, '-');
        StrPushBack(str, '0');
        if (precision > 0) {
            StrPushBack(str, '.');
            for (u8 i = 0; i < precision; i++) {
                StrPushBack(str, '0');
            }
        }
        return str;
    }

    // Handle negative numbers
    bool is_negative = value < 0.0 || signbit(value);
    if (is_negative) {
        value = -value;
    }

    // Determine if we need scientific notation
    // Only compute log10 for regular finite numbers
    bool use_sci = force_sci || (value < 0.0001) || (value >= 1e7);
    if (!use_sci && value > 0.0) {
        // Only compute log10 for positive, non-zero, finite values
        use_sci = (floor(log10(value)) >= 7);
    }

    // Create a temporary string for the numeric part
    Str temp = StrInit();

    if (use_sci) {
        // Normalize to [1.0, 10.0)
        int exp      = 0;
        f64 mantissa = value;

        while (mantissa >= 10.0) {
            mantissa /= 10.0;
            exp++;
        }
        while (mantissa < 1.0 && mantissa > 0.0) { // Ensure mantissa is positive
            mantissa *= 10.0;
            exp--;
        }

        i64 int_part  = (i64)mantissa;
        f64 frac_part = mantissa - int_part;

        StrFromI64(&temp, int_part, 10, false);
        append_fraction(&temp, frac_part, precision);

        // Append exponent
        StrPushBack(&temp, uppercase ? 'E' : 'e');
        StrPushBack(&temp, exp < 0 ? '-' : '+');

        int abs_exp = exp < 0 ? -exp : exp;
        if (abs_exp < 10)
            StrPushBack(&temp, '0');

        // Use snprintf to format the exponent safely
        char exp_buf[12];
        snprintf(exp_buf, sizeof(exp_buf), "%d", abs_exp);
        StrPushBackZstr(&temp, exp_buf);
    } else {
        i64 int_part  = (i64)value;
        f64 frac_part = value - int_part;

        StrFromI64(&temp, int_part, 10, false);
        append_fraction(&temp, frac_part, precision);
    }

    // Add the negative sign if needed, then the numeric part
    if (is_negative) {
        StrPushBack(str, '-');
    }
    StrMerge(str, &temp);
    StrDeinit(&temp);

    return str;
}

bool StrToU64(const Str* str, u64* value, u8 base) {
    ValidateStr(str);

    if (!value) {
        LOG_ERROR("NULL output pointer");
        return false;
    }

    if (base > 36) {
        LOG_ERROR("Invalid base: %u", base);
        return false;
    }

    // Skip whitespace
    size_t pos = 0;
    while (pos < str->length && IS_SPACE(str->data[pos]))
        pos++;

    // Check for empty string
    if (pos >= str->length) {
        LOG_ERROR("Empty string");
        return false;
    }

    // Handle base prefixes
    if (base == 0) {
        if (pos + 2 <= str->length) {
            if (str->data[pos] == '0') {
                if (str->data[pos + 1] == 'x' || str->data[pos + 1] == 'X') {
                    base  = 16;
                    pos  += 2;
                } else if (str->data[pos + 1] == 'b' || str->data[pos + 1] == 'B') {
                    base  = 2;
                    pos  += 2;
                } else if (str->data[pos + 1] == 'o' || str->data[pos + 1] == 'O') {
                    base  = 8;
                    pos  += 2;
                }
            }
        }

        // If still auto-detect, check if it looks like a hex number without prefix
        if (base == 0) {
            // Check for hex characters (a-f) in the string
            bool looks_like_hex = false;
            for (size_t i = pos; i < str->length && !IS_SPACE(str->data[i]); i++) {
                char c = str->data[i];
                if ((c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F')) {
                    looks_like_hex = true;
                    break;
                }
            }

            if (looks_like_hex) {
                base = 16;
            } else {
                base = 10; // Default to decimal
            }
        }
    }

    // Convert digits
    u64  result      = 0;
    bool have_digits = false;

    while (pos < str->length) {
        u8 digit;
        if (!char_to_digit(str->data[pos], &digit, base)) {
            if (IS_SPACE(str->data[pos]))
                break; // Stop at whitespace
            LOG_ERROR("Invalid digit for base %u: %c", base, str->data[pos]);
            return false;
        }

        // Check for overflow
        if (result > ((u64)-1 - digit) / base) { // Replace UINT64_MAX with (u64)-1
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

    // Check that we consumed all characters
    if (pos < str->length) {
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

bool StrToI64(const Str* str, i64* value, u8 base) {
    ValidateStr(str);

    if (!value) {
        LOG_ERROR("NULL output pointer");
        return false;
    }

    if (base > 36) {
        LOG_ERROR("Invalid base: %u", base);
        return false;
    }

    // Skip whitespace
    size_t pos = 0;
    while (pos < str->length && IS_SPACE(str->data[pos]))
        pos++;

    // Check for empty string
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

    // Handle base prefixes
    if (base == 0) {
        if (pos + 2 <= str->length) {
            if (str->data[pos] == '0') {
                if (str->data[pos + 1] == 'x' || str->data[pos + 1] == 'X') {
                    base  = 16;
                    pos  += 2;
                } else if (str->data[pos + 1] == 'b' || str->data[pos + 1] == 'B') {
                    base  = 2;
                    pos  += 2;
                } else if (str->data[pos + 1] == 'o' || str->data[pos + 1] == 'O') {
                    base  = 8;
                    pos  += 2;
                }
            }
        }
        if (base == 0)
            base = 10; // Default to decimal
    }

    // Convert using unsigned function
    // Create a proper temporary string with all required fields
    Str temp_str = {
        .data        = str->data + pos,
        .length      = str->length - pos,
        .capacity    = str->length - pos,
        .copy_init   = NULL,
        .copy_deinit = NULL,
        .alignment   = 1
    };

    u64 unsigned_value;
    if (!StrToU64(&temp_str, &unsigned_value, base)) {
        return false;
    }

    // Check for overflow
    if (negative) {
        // Use 9223372036854775808ULL (2^63) for the minimum value magnitude
        if (unsigned_value > 9223372036854775808ULL) { // INT64_MIN absolute value
            LOG_ERROR("Overflow");
            return false;
        }
        *value = -(i64)unsigned_value;
    } else {
        // Use 9223372036854775807ULL (2^63 - 1) for the maximum value
        if (unsigned_value > 9223372036854775807ULL) { // INT64_MAX
            LOG_ERROR("Overflow");
            return false;
        }
        *value = (i64)unsigned_value;
    }

    return true;
}

bool StrToF64(const Str* str, f64* value) {
    ValidateStr(str);

    if (!value) {
        LOG_ERROR("NULL output pointer");
        return false;
    }

    // Skip whitespace
    size_t pos = 0;
    while (pos < str->length && IS_SPACE(str->data[pos]))
        pos++;

    // Check for empty string
    if (pos >= str->length) {
        LOG_ERROR("Empty string");
        return false;
    }

    // Check for special values - ensuring they're exactly 'inf' or 'nan'
    // followed by end of string or whitespace
    if (str->length - pos >= 3) {
        char c1 = TO_LOWER(str->data[pos]);
        char c2 = TO_LOWER(str->data[pos + 1]);
        char c3 = TO_LOWER(str->data[pos + 2]);

        if (c1 == 'n' && c2 == 'a' && c3 == 'n') {
            // Make sure it's exactly "nan" (followed by whitespace or end of string)
            if (str->length - pos == 3 || IS_SPACE(str->data[pos + 3])) {
                *value = NAN;
                return true;
            }
        }
        if (c1 == 'i' && c2 == 'n' && c3 == 'f') {
            // Make sure it's exactly "inf" (followed by whitespace or end of string)
            if (str->length - pos == 3 || IS_SPACE(str->data[pos + 3])) {
                *value = INFINITY;
                return true;
            }
        }
    }

    // Check for exponent overflow/underflow by examining the exponent in scientific notation
    size_t check_pos = pos;

    // Skip past sign for exponent checking
    if (check_pos < str->length && (str->data[check_pos] == '-' || str->data[check_pos] == '+')) {
        check_pos++;
    }

    // Skip past integer part for exponent checking
    while (check_pos < str->length && IS_DIGIT(str->data[check_pos])) {
        check_pos++;
    }

    // Skip past decimal point and fractional part for exponent checking
    if (check_pos < str->length && str->data[check_pos] == '.') {
        check_pos++;
        while (check_pos < str->length && IS_DIGIT(str->data[check_pos])) {
            check_pos++;
        }
    }

    // If we have scientific notation, examine the exponent for overflow/underflow
    if (check_pos < str->length && (str->data[check_pos] == 'e' || str->data[check_pos] == 'E')) {
        check_pos++;

        // Get exponent sign
        int exp_sign = 1;
        if (check_pos < str->length) {
            if (str->data[check_pos] == '-') {
                exp_sign = -1;
                check_pos++;
            } else if (str->data[check_pos] == '+') {
                check_pos++;
            }
        }

        // Parse exponent value
        int  exp_val         = 0;
        bool have_exp_digits = false;

        while (check_pos < str->length && IS_DIGIT(str->data[check_pos])) {
            exp_val         = exp_val * 10 + (str->data[check_pos] - '0');
            have_exp_digits = true;
            check_pos++;
        }

        if (have_exp_digits) {
            exp_val *= exp_sign;

            // Double has approx 15-17 decimal digits of precision
            // The exponent range for double is approximately -308 to +308
            if (exp_val > 308) {
                LOG_ERROR("Exponent %d exceeds maximum representable f64 exponent (308)", exp_val);
                return false;
            } else if (exp_val <= -324) {
                LOG_ERROR("Exponent %d is below minimum representable f64 exponent (-324)", exp_val);
                return false;
            }
        }
    }

    // Handle sign
    bool negative = false;
    if (str->data[pos] == '-') {
        negative = true;
        pos++;

        // Check for "-inf" after consuming the negative sign
        if (str->length - pos >= 3) {
            char c1 = TO_LOWER(str->data[pos]);
            char c2 = TO_LOWER(str->data[pos + 1]);
            char c3 = TO_LOWER(str->data[pos + 2]);

            if (c1 == 'i' && c2 == 'n' && c3 == 'f') {
                // Make sure it's exactly "-inf" (followed by whitespace or end of string)
                if (str->length - pos == 3 || IS_SPACE(str->data[pos + 3])) {
                    *value = -INFINITY;
                    return true;
                }
            }
        }
    } else if (str->data[pos] == '+') {
        pos++;
    }

    // Parse integer part
    f64  result      = 0.0;
    bool have_digits = false;

    while (pos < str->length && IS_DIGIT(str->data[pos])) {
        result      = result * 10.0 + (str->data[pos] - '0');
        have_digits = true;
        pos++;
    }

    // Parse fractional part
    if (pos < str->length && str->data[pos] == '.') {
        pos++;
        f64 fraction = 0.0;
        f64 scale    = 0.1;

        while (pos < str->length && IS_DIGIT(str->data[pos])) {
            fraction    += (str->data[pos] - '0') * scale;
            scale       *= 0.1;
            have_digits  = true;
            pos++;
        }

        result += fraction;
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

    // Check that we consumed all characters
    if (pos < str->length) {
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
