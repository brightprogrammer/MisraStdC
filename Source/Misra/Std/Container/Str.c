/// file      : std/container/str.c
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Str implementation

#include <Misra/Std/Container/Str.h>
#include <Misra/Std/Container/Str/Private.h>
#include <Misra/Std/Zstr.h>
#include <Misra/Std/Container/Vec/Private.h>
#include <Misra/Std/Log.h>
#include <Misra/Types.h>

#include "Misra/Std/Utility/StrIter.h"

// In-tree replacements for the handful of libm bits Str.c historically
// pulled in for float formatting / parsing. All bounded-domain: the
// inputs come from f64-formatting context where precision <= 18 and
// exponents fit in int. IEEE-754 f64 is assumed (true on every Misra
// target).

// IEEE-754 f64 +Inf and quiet NaN constructed from their bit patterns.
// Done via a type-punned union so we don't depend on the compiler
// folding 1.0/0.0 to a constant.
static f64 inf_f64(void) {
    union {
        u64 i;
        f64 d;
    } u = {0x7FF0000000000000ULL};
    return u.d;
}

static f64 nan_f64(void) {
    union {
        u64 i;
        f64 d;
    } u = {0x7FF8000000000000ULL};
    return u.d;
}

// `isnan` / `isinf` without <math.h>. NaN is uniquely the value not
// equal to itself; +/-Inf has the all-ones exponent and zero mantissa.
static bool isnan_f64(f64 x) {
    return x != x;
}

static bool isinf_f64(f64 x) {
    union {
        u64 i;
        f64 d;
    } u;
    u.d = x;
    return (u.i & 0x7FFFFFFFFFFFFFFFULL) == 0x7FF0000000000000ULL;
}

// Round-half-away-from-zero. For |x| >= 2^53 the value is already
// an integer (f64 can't represent fractional bits at that magnitude)
// so we return it unchanged -- this also dodges the UB of casting an
// out-of-i64-range f64 to i64 (UBSan catches it on values like
// 1.23e19 that the StrFromF64 precision-limit tests feed in).
static f64 round_f64(f64 x) {
    const f64 two53 = 9007199254740992.0; // 2^53
    if (x >= two53 || x <= -two53) {
        return x;
    }
    if (x >= 0.0) {
        return (f64)(i64)(x + 0.5);
    }
    return (f64)(i64)(x - 0.5);
}

// 10^exp for arbitrary signed integer exp. Used to apply the decimal
// exponent when parsing scientific notation (`1.5e10`). Exp is bounded
// by the f64 dynamic range (~[-323, 308]); outside that the result
// saturates to 0 or +Inf via repeated multiplication, matching libm's
// behaviour without the lookup-table machinery.
static f64 pow10_f64(int exp) {
    f64 r = 1.0;
    if (exp >= 0) {
        while (exp-- > 0) {
            r *= 10.0;
        }
    } else {
        while (exp++ < 0) {
            r *= 0.1;
        }
    }
    return r;
}

bool str_try_init_from_cstr(Str *out, Zstr cstr, size len, Allocator *alloc) {
    if (!out || !cstr) {
        LOG_FATAL("Invalid arguments");
    }

    *out = StrInit(alloc);
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

Str str_init_from_cstr(Zstr cstr, size len, Allocator *alloc) {
    Str result = StrInit(alloc);

    // Try-form leaves `result` empty-but-valid on OOM, so the caller
    // sees the same empty Str whether the input was zero-length or the
    // allocation failed -- matches the by-value-init contract documented
    // in StrInitFromCstr.
    (void)str_try_init_from_cstr(&result, cstr, len, alloc);
    return result;
}

bool StrInitCopy(Str *dst, const Str *src) {
    if (!dst || !src) {
        LOG_FATAL("Invalid arguments");
    }

    return str_init_copy(dst, src, src->allocator);
}

bool str_init_copy(void *dst_ptr, const void *src_ptr, const Allocator *alloc) {
    Str       *dst             = (Str *)dst_ptr;
    const Str *src             = (const Str *)src_ptr;
    Allocator *clone_allocator = NULL;

    ValidateStr(src);
    if (!dst) {
        LOG_FATAL("Invalid arguments");
    }

    clone_allocator = alloc ? (Allocator *)alloc : src->allocator;

    MemSet(dst, 0, sizeof(Str));
    *dst             = StrInit(clone_allocator);
    dst->copy_init   = src->copy_init;
    dst->copy_deinit = src->copy_deinit;

    if (!insert_range_into_vec(GENERIC_VEC(dst), (const u8 *)src->data, sizeof(char), 0, src->length)) {
        return false;
    }

    return true;
}

void StrDeinit(Str *copy) {
    ValidateStr(copy);
    deinit_vec(GENERIC_VEC(copy), sizeof(char));
}

void str_deinit(void *copy, const Allocator *alloc) {
    (void)alloc;
    StrDeinit((Str *)copy);
}

// FNV-1a over the string's byte view. `ignored_size` is the generic-callback
// shape (sizeof the value-type slot), not the Str's character count -- the
// real length lives inside the Str header itself.
u64 str_hash(const void *data, u32 ignored_size) {
    const Str *str  = (const Str *)data;
    u64        hash = 1469598103934665603ULL;
    size       idx  = 0;

    (void)ignored_size;
    ValidateStr(str);

    for (idx = 0; idx < StrLen(str); idx++) {
        hash ^= (u64)(unsigned char)StrCharAt(str, idx);
        hash *= 1099511628211ULL;
    }

    return hash;
}

i32 str_compare(const void *lhs, const void *rhs) {
    const Str *a   = (const Str *)lhs;
    const Str *b   = (const Str *)rhs;
    size       min = 0;
    i32        cmp = 0;

    ValidateStr(a);
    ValidateStr(b);

    min = StrLen(a) < StrLen(b) ? StrLen(a) : StrLen(b);
    cmp = MemCompare(StrBegin(a), StrBegin(b), min);

    if (cmp != 0) {
        return cmp;
    }
    if (StrLen(a) < StrLen(b)) {
        return -1;
    }
    if (StrLen(a) > StrLen(b)) {
        return 1;
    }
    return 0;
}

i32 str_cmp_str(const Str *s, const Str *other) {
    ValidateStr(s);
    ValidateStr(other);
    return str_compare(s, other);
}

i32 str_cmp_zstr(const Str *s, Zstr other) {
    ValidateStr(s);
    return ZstrCompare(StrBegin(s), other);
}

i32 str_cmp_cstr(const Str *s, Zstr other, size other_len) {
    ValidateStr(s);
    return ZstrCompareN(StrBegin(s), other, other_len);
}

i32 str_cmp_str_ignore_case(const Str *s, const Str *other) {
    ValidateStr(s);
    ValidateStr(other);
    return ZstrCompareIgnoreCase(StrBegin(s), StrBegin(other));
}

i32 str_cmp_zstr_ignore_case(const Str *s, Zstr other) {
    ValidateStr(s);
    return ZstrCompareIgnoreCase(StrBegin(s), other);
}

i32 str_cmp_cstr_ignore_case(const Str *s, Zstr other, size other_len) {
    ValidateStr(s);
    return ZstrCompareNIgnoreCase(StrBegin(s), other, other_len);
}

Zstr str_find_cstr(const Str *s, Zstr key, size key_len) {
    ValidateStr(s);
    return ZstrFindSubstringN(StrBegin(s), key, key_len);
}

Zstr str_find_zstr(const Str *s, Zstr key) {
    ValidateStr(s);
    return ZstrFindSubstring(StrBegin(s), key);
}

Zstr str_find_str(const Str *s, const Str *key) {
    ValidateStr(s);
    ValidateStr(key);
    return ZstrFindSubstringN(StrBegin(s), StrBegin(key), StrLen(key));
}

static StrIters str_split_to_iters_impl(Str *s, Zstr key, size keylen) {
    ValidateStr(s);

    StrIters sv   = (StrIters)VecInit(s->allocator);
    Zstr     prev = s->data;
    Zstr     end  = s->data + s->length;

    while (prev <= end) {
        Zstr next = ZstrFindSubstringN(prev, key, keylen);
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

StrIters str_split_to_iters_zstr(Str *s, Zstr key) {
    return str_split_to_iters_impl(s, key, ZstrLen(key));
}

StrIters str_split_to_iters_str(Str *s, const Str *key) {
    if (!key) {
        LOG_FATAL("Invalid arguments");
    }
    return str_split_to_iters_impl(s, key->data, key->length);
}

static Strs str_split_impl(Str *s, Zstr key, size keylen) {
    ValidateStr(s);

    Strs sv        = (Strs)VecInit(s->allocator);
    sv.copy_deinit = (GenericCopyDeinit)str_deinit;
    Zstr prev      = s->data;

    if (prev) {
        Zstr end = s->data + s->length;
        while (prev <= end) {
            Zstr next = ZstrFindSubstringN(prev, key, keylen);
            if (next) {
                Str tmp = StrInitFromCstr(prev, next - prev, s->allocator);
                VecPushBack(&sv, tmp);
                prev = next + keylen;
            } else {
                if (ZstrCompareN(prev, key, end - prev)) {
                    Str tmp = StrInitFromCstr(prev, end - prev, s->allocator);
                    VecPushBack(&sv, tmp);
                }
                break;
            }
        }
    }

    return sv;
}

Strs str_split_zstr(Str *s, Zstr key) {
    return str_split_impl(s, key, ZstrLen(key));
}

Strs str_split_str(Str *s, const Str *key) {
    if (!key) {
        LOG_FATAL("Invalid arguments");
    }
    return str_split_impl(s, key->data, key->length);
}

size str_index_of_cstr(const Str *s, Zstr key, size key_len) {
    Zstr found = NULL;

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

size str_index_of_zstr(const Str *s, Zstr key) {
    if (!key) {
        LOG_FATAL("Invalid arguments");
    }

    return str_index_of_cstr(s, key, ZstrLen(key));
}

size str_index_of_str(const Str *s, const Str *key) {
    if (!key) {
        LOG_FATAL("Invalid arguments");
    }

    ValidateStr(key);

    if (key->length == 0) {
        return 0;
    }

    return str_index_of_cstr(s, key->data, key->length);
}

bool str_contains_cstr(const Str *s, Zstr key, size key_len) {
    return str_index_of_cstr(s, key, key_len) != SIZE_MAX;
}

bool str_contains_zstr(const Str *s, Zstr key) {
    return str_index_of_zstr(s, key) != SIZE_MAX;
}

bool str_contains_str(const Str *s, const Str *key) {
    if (!key) {
        LOG_FATAL("Invalid arguments");
    }

    ValidateStr(key);

    if (key->length == 0) {
        return true;
    }

    return str_contains_cstr(s, key->data, key->length);
}

static inline bool is_strip_char(char c, Zstr strip_chars) {
    Zstr p = strip_chars;
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
Str strip_str(Str *s, Zstr chars_to_strip, int split_direction) {
    ValidateStr(s);

    // Empty Str: `s->data` may be NULL or unallocated; forming
    // `s->data + s->length - 1` would be UB. Return an empty
    // result bound to the same allocator.
    if (s->length == 0) {
        return StrInitFromCstr("", 0, s->allocator);
    }

    Zstr strip_chars = chars_to_strip ? chars_to_strip : " \t\n\r\v\f";
    Zstr start       = s->data;
    Zstr end         = s->data + s->length - 1;

    if (split_direction <= 0) {
        while (start <= end && is_strip_char(*start, strip_chars)) {
            start++;
        }
    }

    if (split_direction >= 0) {
        while (end >= start && is_strip_char(*end, strip_chars)) {
            end--;
        }
    }

    size new_len = end >= start ? (end - start + 1) : 0;
    return StrInitFromCstr(start, new_len, s->allocator);
}

static inline bool starts_with(Zstr data, size data_len, Zstr prefix, size prefix_len) {
    return data_len >= prefix_len && MemCompare(data, prefix, prefix_len) == 0;
}

static inline bool ends_with(Zstr data, size data_len, Zstr suffix, size suffix_len) {
    return data_len >= suffix_len && MemCompare(data + data_len - suffix_len, suffix, suffix_len) == 0;
}

bool str_starts_with_zstr(const Str *s, Zstr prefix) {
    ValidateStr(s);
    return starts_with(s->data, s->length, prefix, ZstrLen(prefix));
}

bool str_ends_with_zstr(const Str *s, Zstr suffix) {
    ValidateStr(s);
    return ends_with(s->data, s->length, suffix, ZstrLen(suffix));
}

bool str_starts_with_cstr(const Str *s, Zstr prefix, size prefix_len) {
    ValidateStr(s);
    return starts_with(s->data, s->length, prefix, prefix_len);
}

bool str_ends_with_cstr(const Str *s, Zstr suffix, size suffix_len) {
    ValidateStr(s);
    return ends_with(s->data, s->length, suffix, suffix_len);
}

bool str_starts_with_str(const Str *s, const Str *prefix) {
    ValidateStr(s);
    return starts_with(s->data, s->length, prefix->data, prefix->length);
}

bool str_ends_with_str(const Str *s, const Str *suffix) {
    ValidateStr(s);
    return ends_with(s->data, s->length, suffix->data, suffix->length);
}

void str_replace_cstr(Str *s, Zstr match, size match_len, Zstr replacement, size replacement_len, size count) {
    ValidateStr(s);
    size i        = 0;
    size replaced = 0;

    while (i + match_len <= s->length && replaced < count) {
        if (MemCompare(s->data + i, match, match_len) == 0) {
            StrDeleteRange(s, i, match_len);
            StrInsertMany(s, replacement, replacement_len, i);
            i        += replacement_len;
            replaced += 1;
        } else {
            i++;
        }
    }
}

void str_replace_zstr(Str *s, Zstr match, Zstr replacement, size count) {
    ValidateStr(s);
    str_replace_cstr(s, match, ZstrLen(match), replacement, ZstrLen(replacement), count);
}

void str_replace_str(Str *s, const Str *match, const Str *replacement, size count) {
    ValidateStr(s);
    str_replace_cstr(s, match->data, match->length, replacement->data, replacement->length, count);
}

static inline char digit_to_char(u8 digit, bool uppercase) {
    if (digit < 10)
        return '0' + digit;
    return (uppercase ? 'A' : 'a') + (digit - 10);
}

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

static inline bool is_valid_base(u8 base) {
    return base != 1 && base <= 36;
}

static inline size skip_prefix(const Str *str, size pos, u8 base) {
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

    if (config->use_prefix) {
        if (!StrPushBackR(str, '0')) {
            return NULL;
        }
        if (config->base == 2) {
            if (!StrPushBackR(str, 'b')) {
                return NULL;
            }
        } else if (config->base == 8) {
            if (!StrPushBackR(str, 'o')) {
                return NULL;
            }
        } else if (config->base == 16) {
            if (!StrPushBackR(str, 'x')) {
                return NULL;
            }
        }
    }

    if (value == 0) {
        if (!StrPushBackR(str, '0')) {
            return NULL;
        }
    } else {
        bool ok = true;
        StrInitStack(buffer, 65) {
            char *data = StrBegin(&buffer);
            size  pos  = 0;

            while (value > 0) {
                data[pos++]  = digit_to_char(value % config->base, config->uppercase);
                value       /= config->base;
            }

            // First loop wrote digits LSB-first into the scratch; replay
            // them in reverse so the destination ends up MSB-first.
            while (pos > 0) {
                if (!StrPushBackR(str, data[--pos])) {
                    ok = false;
                    break;
                }
            }
        }
        if (!ok) {
            return NULL;
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

    bool is_negative = value < 0;
    u64  abs_value;

    if (value == INT64_MIN) {
        // -INT64_MIN overflows i64; the bit pattern of INT64_MIN cast to u64
        // is already |INT64_MIN|, so use it directly instead of negating.
        abs_value = (u64)INT64_MIN;
    } else {
        abs_value = is_negative ? (u64)(-value) : (u64)value;
    }

    if (!StrFromU64(str, abs_value, config)) {
        return NULL;
    }

    // Sign only applies to base-10 here; binary/octal/hex print the unsigned
    // bit pattern, where a leading '-' would change the meaning.
    if (is_negative && config->base == 10) {
        if (!StrInsertR(str, '-', 0)) {
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

    if (isnan_f64(value)) {
        Zstr nan_str = config->uppercase ? "NAN" : "nan";
        for (size i = 0; i < 3; i++) {
            if (!StrPushBackR(str, nan_str[i])) {
                return NULL;
            }
        }
        return str;
    }

    if (isinf_f64(value)) {
        if (value < 0) {
            if (!StrPushBackR(str, '-')) {
                return NULL;
            }
        } else if (config->always_sign) {
            if (!StrPushBackR(str, '+')) {
                return NULL;
            }
        }
        Zstr inf_str = config->uppercase ? "INF" : "inf";
        for (size i = 0; i < 3; i++) {
            if (!StrPushBackR(str, inf_str[i])) {
                return NULL;
            }
        }
        return str;
    }

    if (value < 0) {
        if (!StrPushBackR(str, '-')) {
            return NULL;
        }
        value = -value;
    } else if (config->always_sign) {
        if (!StrPushBackR(str, '+')) {
            return NULL;
        }
    }

    bool use_sci = config->force_sci || (value != 0.0 && (value < 0.0001 || value >= 1e7));

    if (use_sci) {
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

        i64 int_part = (i64)mantissa;
        if (!StrPushBackR(str, '0' + int_part)) {
            return NULL;
        }

        if (config->precision > 0) {
            if (!StrPushBackR(str, '.')) {
                return NULL;
            }
            f64 frac_part = mantissa - int_part;

            for (u8 i = 0; i < config->precision; i++) {
                frac_part *= 10.0;
                int digit  = (int)(frac_part + 0.5);
                // Each digit is rounded independently above; clamp the +0.5
                // overflow case (e.g. 9.6 -> 10) back to a single decimal digit.
                if (digit >= 10)
                    digit = 9;
                if (!StrPushBackR(str, '0' + digit)) {
                    return NULL;
                }
                frac_part -= (int)frac_part;
            }
        }

        if (!StrPushBackR(str, config->uppercase ? 'E' : 'e')) {
            return NULL;
        }
        if (exp >= 0) {
            if (!StrPushBackR(str, '+')) {
                return NULL;
            }
        } else {
            if (!StrPushBackR(str, '-')) {
                return NULL;
            }
            exp = -exp;
        }

        if (exp == 0) {
            // Two-digit minimum matches printf("%e") convention.
            if (!StrPushBackR(str, '0') || !StrPushBackR(str, '0')) {
                return NULL;
            }
        } else {
            bool ok = true;
            StrInitStack(exp_buf, 8) {
                char *data    = StrBegin(&exp_buf);
                size  exp_pos = 0;
                while (exp > 0) {
                    data[exp_pos++]  = '0' + (exp % 10);
                    exp             /= 10;
                }
                // Two-digit minimum matches printf("%e") convention.
                while (exp_pos < 2) {
                    data[exp_pos++] = '0';
                }
                while (exp_pos > 0) {
                    if (!StrPushBackR(str, data[--exp_pos])) {
                        ok = false;
                        break;
                    }
                }
            }
            if (!ok) {
                return NULL;
            }
        }
    } else {
        i64 int_part = (i64)value;

        if (int_part == 0) {
            if (!StrPushBackR(str, '0')) {
                return NULL;
            }
        } else {
            bool ok = true;
            StrInitStack(int_buf, 32) {
                char *data    = StrBegin(&int_buf);
                size  int_pos = 0;
                while (int_part > 0) {
                    data[int_pos++]  = '0' + (int_part % 10);
                    int_part        /= 10;
                }
                while (int_pos > 0) {
                    if (!StrPushBackR(str, data[--int_pos])) {
                        ok = false;
                        break;
                    }
                }
            }
            if (!ok) {
                return NULL;
            }
        }

        if (config->precision > 0) {
            if (!StrPushBackR(str, '.')) {
                return NULL;
            }

            // Round once at full precision so per-digit truncation below
            // produces the half-away-from-zero result printf would.
            f64 scale = 1.0;
            for (u8 i = 0; i < config->precision; i++) {
                scale *= 10.0;
            }
            f64 rounded_value = round_f64(value * scale) / scale;
            f64 frac_part     = rounded_value - (i64)rounded_value;

            for (u8 i = 0; i < config->precision; i++) {
                frac_part *= 10.0;
                int digit  = (int)frac_part;

                // Catch the .999999... that floating point would otherwise
                // truncate to a digit short of the next integer.
                if (frac_part - digit > 0.999999) {
                    digit++;
                }

                if (!StrPushBackR(str, '0' + digit)) {
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
        LOG_FATAL("NULL output pointer");
    }

    if (!config) {
        config = &STR_PARSE_DEFAULT;
    }

    u8 base = config->base;
    if (base != 0 && !is_valid_base(base)) {
        LOG_ERROR("Invalid base: {}", base);
        return false;
    }

    size pos = 0;
    while (pos < str->length && IS_SPACE(str->data[pos]))
        pos++;

    if (pos >= str->length) {
        LOG_ERROR("Empty string");
        return false;
    }

    if (base == 0) {
        base = 10;
        // C-style prefixes only when the caller didn't fix the base.
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

        // (UINT64_MAX - digit) / base is the largest `result` that can absorb
        // one more digit without wrapping; anything above is an overflow.
        if (result > (UINT64_MAX - digit) / base) {
            LOG_ERROR("Overflow");
            return false;
        }

        result      = result * base + digit;
        have_digits = true;
        pos++;
    }

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
        LOG_FATAL("NULL output pointer");
    }

    if (!config) {
        config = &STR_PARSE_DEFAULT;
    }

    size pos = 0;
    while (pos < str->length && IS_SPACE(str->data[pos]))
        pos++;

    if (pos >= str->length) {
        LOG_ERROR("Empty string");
        return false;
    }

    bool negative = false;
    if (str->data[pos] == '-') {
        negative = true;
        pos++;
    } else if (str->data[pos] == '+') {
        pos++;
    }

    // Borrowed substring view: data points into the caller's bytes and
    // capacity stays 0 so StrToU64 never tries to grow it.
    Str temp_str      = StrInit(str->allocator);
    temp_str.data     = str->data + pos;
    temp_str.length   = str->length - pos;
    temp_str.capacity = str->length - pos;

    u64 unsigned_value;
    if (!StrToU64(&temp_str, &unsigned_value, config)) {
        return false;
    }

    // For negatives the absolute value can reach 2^63 (INT64_MIN).
    // Negating that as a signed i64 is UB -- do the negation in unsigned
    // space and reinterpret.
    if (negative) {
        if (unsigned_value > 9223372036854775808ULL) {
            LOG_ERROR("Overflow");
            return false;
        }
        *value = (i64)(0u - unsigned_value);
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
        LOG_FATAL("NULL output pointer");
    }

    if (!config) {
        config = &STR_PARSE_DEFAULT;
    }

    size pos = 0;
    while (pos < str->length && IS_SPACE(str->data[pos]))
        pos++;

    if (pos >= str->length) {
        LOG_ERROR("Empty string");
        return false;
    }

    // Unsigned NaN / Inf literals. The trailing IS_SPACE check rejects
    // things like "nan_garbage" while still accepting "nan ".
    if (str->length - pos >= 3) {
        char c1 = TO_LOWER(str->data[pos]);
        char c2 = TO_LOWER(str->data[pos + 1]);
        char c3 = TO_LOWER(str->data[pos + 2]);

        if (c1 == 'n' && c2 == 'a' && c3 == 'n') {
            if (str->length - pos == 3 || IS_SPACE(str->data[pos + 3])) {
                *value = nan_f64();
                return true;
            }
        }
        if (c1 == 'i' && c2 == 'n' && c3 == 'f') {
            if (str->length - pos == 3 || IS_SPACE(str->data[pos + 3])) {
                *value = inf_f64();
                return true;
            }
        }
    }

    bool negative = false;
    if (str->data[pos] == '-') {
        negative = true;
        pos++;

        // Catch "-inf" before the numeric parser, since the sign was
        // already consumed.
        if (str->length - pos >= 3) {
            char c1 = TO_LOWER(str->data[pos]);
            char c2 = TO_LOWER(str->data[pos + 1]);
            char c3 = TO_LOWER(str->data[pos + 2]);

            if (c1 == 'i' && c2 == 'n' && c3 == 'f') {
                if (str->length - pos == 3 || IS_SPACE(str->data[pos + 3])) {
                    *value = -inf_f64();
                    return true;
                }
            }
        }
    } else if (str->data[pos] == '+') {
        pos++;
    }

    f64  result      = 0.0;
    bool have_digits = false;

    while (pos < str->length && IS_DIGIT(str->data[pos])) {
        result      = result * 10.0 + (str->data[pos] - '0');
        have_digits = true;
        pos++;
    }

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

        // Cap the running exponent at 1024: f64 saturates to 0 / +Inf well
        // before that, and unbounded `exponent * 10 + digit` is signed
        // overflow UB on inputs like "1e2147483648" (UBSan flags it).
        while (pos < str->length && IS_DIGIT(str->data[pos])) {
            if (exponent < 1024) {
                exponent = exponent * 10 + (str->data[pos] - '0');
            }
            have_exp_digits = true;
            pos++;
        }

        if (!have_exp_digits) {
            LOG_ERROR("Missing exponent digits");
            return false;
        }

        if (exp_negative)
            exponent = -exponent;
        result *= pow10_f64(exponent);
    }

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

const StrIntFormat STR_INT_DEFAULT =
    {.base = 10, .uppercase = false, .use_prefix = false, .pad_zeros = false, .min_width = 0};

const StrFloatFormat STR_FLOAT_DEFAULT =
    {.precision = 6, .force_sci = false, .uppercase = false, .trim_zeros = false, .always_sign = false};

const StrParseConfig STR_PARSE_DEFAULT = {.strict = false, .trim_space = true, .base = 0};
