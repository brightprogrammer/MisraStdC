/// file      : std/zstr.c
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Operations on NUL-terminated C strings.

#include <Misra/Std/Log.h>
#include <Misra/Std/Memory.h>
#include <Misra/Std/Zstr.h>

size ZstrLen(Zstr str) {
    if (!str) {
        LOG_FATAL("Invalid arguments");
    }

    Zstr s = str;
    while (*s)
        s++;
    return s - str;
}

i32 ZstrCompare(Zstr s1, Zstr s2) {
    if (!s1 || !s2) {
        LOG_FATAL("Invalid arguments");
    }

    while (*s1 && *s1 == *s2) {
        s1++;
        s2++;
    }
    return *(const u8 *)s1 - *(const u8 *)s2;
}

i32 ZstrCompareN(Zstr s1, Zstr s2, size n) {
    if (!s1 || !s2) {
        LOG_FATAL("Invalid arguments");
    }

    size i = 0;
    while (i < n && s1[i] && s2[i]) {
        if (s1[i] != s2[i]) {
            return (i32)(unsigned char)s1[i] - (i32)(unsigned char)s2[i];
        }
        i++;
    }
    if (i == n || (!s1[i] && !s2[i])) {
        return 0;
    }
    return s1[i] ? 1 : -1;
}

static inline u8 zstr_ascii_lower(u8 c) {
    return (c >= 'A' && c <= 'Z') ? (u8)(c - 'A' + 'a') : c;
}

i32 ZstrCompareIgnoreCase(Zstr s1, Zstr s2) {
    if (!s1 || !s2) {
        LOG_FATAL("Invalid arguments");
    }
    while (*s1) {
        u8 c1 = zstr_ascii_lower((u8)*s1);
        u8 c2 = zstr_ascii_lower((u8)*s2);
        if (c1 != c2) {
            return (i32)c1 - (i32)c2;
        }
        ++s1;
        ++s2;
    }
    return -(i32)zstr_ascii_lower((u8)*s2);
}

i32 ZstrCompareNIgnoreCase(Zstr s1, Zstr s2, size n) {
    if (!s1 || !s2) {
        LOG_FATAL("Invalid arguments");
    }
    size i = 0;
    while (i < n && s1[i] && s2[i]) {
        u8 c1 = zstr_ascii_lower((u8)s1[i]);
        u8 c2 = zstr_ascii_lower((u8)s2[i]);
        if (c1 != c2) {
            return (i32)c1 - (i32)c2;
        }
        ++i;
    }
    if (i == n || (!s1[i] && !s2[i])) {
        return 0;
    }
    return s1[i] ? 1 : -1;
}

Zstr ZstrFindChar(Zstr str, char ch) {
    if (!str) {
        LOG_FATAL("Invalid arguments");
    }

    do {
        if (*str == ch) {
            return str;
        }
    } while (*str++);

    return NULL;
}

Zstr zstr_dup_n(Zstr src, size n, Allocator *alloc) {
    if (!src || !alloc) {
        LOG_FATAL("Invalid arguments");
    }

    size len = 0;
    while (len < n && src[len])
        len++;

    char *new_str = (char *)AllocatorAlloc(alloc, len + 1, false);
    if (!new_str) {
        // Allocator failures are not errno-bearing (allocators are
        // caller-supplied, libc-independent), so "LOG_SYS_ERROR" would
        // be misleading. Plain error log.
        LOG_ERROR("allocator allocate failed");
        return NULL;
    }

    MemCopy(new_str, src, len);
    new_str[len] = '\0';
    return new_str;
}

Zstr zstr_dup(Zstr src, Allocator *alloc) {
    if (!src) {
        LOG_FATAL("Invalid arguments");
    }
    return zstr_dup_n(src, ZstrLen(src), alloc);
}

bool zstr_init_clone(void *dst_ptr, const void *src_ptr, const Allocator *alloc) {
    Zstr *dst = (Zstr *)dst_ptr;
    const Zstr *src = (const Zstr *)src_ptr;

    if (!dst || !src || !*src || !alloc) {
        LOG_FATAL("Invalid arguments.");
    }

    *dst = zstr_dup_n(*src, ZstrLen(*src), (Allocator *)alloc);
    return *dst != NULL;
}

void zstr_deinit(void *zs_ptr, const Allocator *alloc) {
    Zstr *zs = (Zstr *)zs_ptr;

    if (!zs || !alloc) {
        LOG_FATAL("Invalid arguments");
    }

    if (*zs) {
        AllocatorFree((Allocator *)alloc, (void *)*zs);
        *zs = NULL;
    }
}

static inline bool zstr_is_ws(char c) {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\v' || c == '\f';
}

i64 ZstrToI64(Zstr s, Zstr *endptr) {
    if (!s) {
        if (endptr)
            *endptr = NULL;
        return 0;
    }
    while (zstr_is_ws(*s))
        s++;
    bool neg = false;
    if (*s == '+') {
        s++;
    } else if (*s == '-') {
        neg = true;
        s++;
    }
    // Accumulate in u64 with explicit overflow detection. The bound is
    // INT64_MAX for positive, INT64_MAX+1 (= 2^63) for negative.
    // Overflow saturates so callers see a pinned value rather than a
    // silent wrap.
    Zstr digit_start = s;
    const u64 bound       = neg ? ((u64)1 << 63) : (u64)0x7FFFFFFFFFFFFFFFULL;
    u64       val         = 0;
    bool      saturated   = false;
    while (*s >= '0' && *s <= '9') {
        u64 digit = (u64)(*s - '0');
        if (!saturated) {
            if (val > (bound - digit) / 10) {
                val       = bound;
                saturated = true;
            } else {
                val = val * 10 + digit;
            }
        }
        s++;
    }
    if (endptr) {
        *endptr = s == digit_start ? digit_start : s;
    }
    if (s == digit_start) {
        return 0;
    }
    // Negate in unsigned space so val == 2^63 (the INT64_MIN literal)
    // doesn't trip signed-overflow UB on the (i64) cast + negation.
    return neg ? (i64)(0u - val) : (i64)val;
}

f64 ZstrToF64(Zstr s, Zstr *endptr) {
    if (!s) {
        if (endptr)
            *endptr = NULL;
        return 0.0;
    }
    while (zstr_is_ws(*s))
        s++;
    bool neg = false;
    if (*s == '+') {
        s++;
    } else if (*s == '-') {
        neg = true;
        s++;
    }
    Zstr digit_start = s;
    f64  val         = 0.0;
    while (*s >= '0' && *s <= '9') {
        val = val * 10.0 + (f64)(*s - '0');
        s++;
    }
    if (*s == '.') {
        s++;
        f64 divisor = 10.0;
        while (*s >= '0' && *s <= '9') {
            val     += (f64)(*s - '0') / divisor;
            divisor *= 10.0;
            s++;
        }
    }
    if (s == digit_start && (s[-1] != '.' || (digit_start == s))) {
        if (endptr) {
            *endptr = digit_start;
        }
        return 0.0;
    }
    if (*s == 'e' || *s == 'E') {
        s++;
        bool eneg = false;
        if (*s == '+') {
            s++;
        } else if (*s == '-') {
            eneg = true;
            s++;
        }
        int exp = 0;
        while (*s >= '0' && *s <= '9') {
            exp = exp * 10 + (*s - '0');
            s++;
        }
        f64 mul = 1.0;
        for (int i = 0; i < exp; ++i) {
            mul *= 10.0;
        }
        if (eneg) {
            val /= mul;
        } else {
            val *= mul;
        }
    }
    if (endptr) {
        *endptr = s;
    }
    return neg ? -val : val;
}

Zstr ZstrFindSubstring(Zstr haystack, Zstr needle) {
    if (!needle) {
        LOG_FATAL("Invalid arguments");
    }
    return ZstrFindSubstringN(haystack, needle, ZstrLen(needle));
}

Zstr ZstrFindSubstringN(Zstr haystack, Zstr needle, size needle_len) {
    if (!haystack || !needle) {
        LOG_FATAL("Invalid arguments");
    }
    if (needle_len == 0) {
        return haystack;
    }
    size haystack_len = ZstrLen(haystack);
    if (needle_len > haystack_len) {
        return NULL;
    }
    char first_char = *needle;
    size pos        = 0;
    while (pos <= haystack_len - needle_len) {
        if (haystack[pos] != first_char) {
            pos++;
            continue;
        }
        if (MemCompare(haystack + pos, needle, needle_len) == 0) {
            return haystack + pos;
        }
        pos++;
    }
    return NULL;
}
