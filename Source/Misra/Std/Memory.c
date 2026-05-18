/// file      : std/memory.c
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Memory manipulation functions

#include <Misra/Std/Memory.h>
#include <Misra/Sys.h>
#include <Misra/Std/Log.h>

i32 MemCompare(const void *p1, const void *p2, size n) {
    if (n == 0) {
        return 0;
    }

    if (!p1 || !p2) {
        LOG_FATAL("Invalid arguments");
    }

    const u8 *s1 = (const u8 *)p1;
    const u8 *s2 = (const u8 *)p2;
    while (n--) {
        if (*s1 != *s2) {
            return *s1 - *s2;
        }
        s1++;
        s2++;
    }
    return 0;
}

void *MemCopy(void *dst, const void *src, size n) {
    if (n == 0) {
        return dst;
    }

    if (!dst || !src) {
        LOG_FATAL("Invalid arguments");
    }

    u8       *d = (u8 *)dst;
    const u8 *s = (const u8 *)src;
    while (n--) {
        *d++ = *s++;
    }
    return dst;
}

void *MemMove(void *dst, const void *src, size n) {
    if (n == 0) {
        return dst;
    }

    if (!dst || !src) {
        LOG_FATAL("Invalid arguments");
    }

    u8       *d = (u8 *)dst;
    const u8 *s = (const u8 *)src;
    if (d < s) {
        while (n--) {
            *d++ = *s++;
        }
    } else if (d > s) {
        d += n;
        s += n;
        while (n--) {
            *--d = *--s;
        }
    }
    return dst;
}

void *MemSet(void *dst, i32 val, size n) {
    if (n == 0) {
        return dst;
    }

    if (!dst) {
        LOG_FATAL("Invalid arguments");
    }

    u8 *d = (u8 *)dst;
    while (n--) {
        *d++ = (u8)val;
    }
    return dst;
}

size ZstrLen(const char *str) {
    if (!str) {
        LOG_FATAL("Invalid arguments");
    }

    const char *s = str;
    while (*s)
        s++;
    return s - str;
}

i32 ZstrCompare(const char *s1, const char *s2) {
    if (!s1 || !s2) {
        LOG_FATAL("Invalid arguments");
    }

    while (*s1 && *s1 == *s2) {
        s1++;
        s2++;
    }
    return *(const u8 *)s1 - *(const u8 *)s2;
}

i32 ZstrCompareN(const char *s1, const char *s2, size n) {
    if (!s1 || !s2) {
        LOG_FATAL("Invalid arguments");
    }

    // Compare characters up to n
    size i = 0;
    while (i < n && s1[i] && s2[i]) {
        if (s1[i] != s2[i]) {
            return (i32)(unsigned char)s1[i] - (i32)(unsigned char)s2[i];
        }
        i++;
    }

    // If we reached the limit or both strings ended at the same time
    if (i == n || (!s1[i] && !s2[i])) {
        return 0;
    }

    // One string ended before the other
    return s1[i] ? 1 : -1;
}

char *ZstrFindChar(const char *str, char ch) {
    if (!str) {
        LOG_FATAL("Invalid arguments");
    }

    do {
        if (*str == ch) {
            return (char *)str;
        }
    } while (*str++);

    return NULL;
}

char *zstr_dup_n(const char *src, size n, Allocator *alloc) {
    if (!src || !alloc) {
        LOG_FATAL("Invalid arguments");
    }

    size len = 0;
    while (len < n && src[len])
        len++;

    char *new_str = (char *)AllocatorAlloc(alloc, len + 1, false);
    if (!new_str) {
        // Not LOG_SYS_ERROR: allocator failures don't set errno
        // (allocators are caller-supplied, libc-independent), so the
        // "errno" suffix would be misleading.
        LOG_ERROR("allocator allocate failed");
        return NULL;
    }

    MemCopy(new_str, src, len);
    new_str[len] = '\0';
    return new_str;
}

char *zstr_dup(const char *src, Allocator *alloc) {
    if (!src) {
        LOG_FATAL("Invalid arguments");
    }
    return zstr_dup_n(src, ZstrLen(src), alloc);
}

bool zstr_init_clone(void *dst_ptr, const void *src_ptr, const Allocator *alloc) {
    const char       **dst = (const char **)dst_ptr;
    const char *const *src = (const char *const *)src_ptr;

    if (!dst || !src || !*src || !alloc) {
        LOG_FATAL("Invalid arguments.");
    }

    *dst = zstr_dup_n(*src, ZstrLen(*src), (Allocator *)alloc);
    return *dst != NULL;
}

void zstr_deinit(void *zs_ptr, const Allocator *alloc) {
    const char **zs = (const char **)zs_ptr;

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

i64 ZstrToI64(const char *s, char **endptr) {
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
    const char *digit_start = s;
    u64         val         = 0;
    while (*s >= '0' && *s <= '9') {
        val = val * 10 + (u64)(*s - '0');
        s++;
    }
    if (endptr) {
        *endptr = (char *)(s == digit_start ? digit_start - (neg || *digit_start ? 1 : 0) : s);
        // If no digits, return endptr at original start; matches strtol shape.
        if (s == digit_start) {
            *endptr = (char *)digit_start;
        }
    }
    if (s == digit_start) {
        return 0;
    }
    return neg ? -(i64)val : (i64)val;
}

f64 ZstrToF64(const char *s, char **endptr) {
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
    const char *digit_start = s;
    f64         val         = 0.0;
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
        // No digits at all (and no leading dot consumed). Same shape
        // as strtod: endptr unchanged from start, value 0.
        if (endptr) {
            *endptr = (char *)digit_start;
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
        *endptr = (char *)s;
    }
    return neg ? -val : val;
}

char *ZstrFindSubstring(const char *haystack, const char *needle) {
    // The earlier hand-rolled loop had an off-by-one in the outer
    // termination guard that made it return NULL whenever
    // `strlen(needle) == strlen(haystack)`, i.e. exact matches at
    // position 0 of a same-length haystack. Delegate to the explicit-
    // length variant, which is correct.
    if (!needle) {
        LOG_FATAL("Invalid arguments");
    }
    return ZstrFindSubstringN(haystack, needle, ZstrLen(needle));
}

char *ZstrFindSubstringN(const char *haystack, const char *needle, size needle_len) {
    if (!haystack || !needle) {
        LOG_FATAL("Invalid arguments");
    }

    // Empty needle matches at the start of haystack
    if (needle_len == 0) {
        return (char *)haystack;
    }

    // Calculate haystack length
    size haystack_len = ZstrLen(haystack);

    // if needle is longer than haystack, is it really a needle?
    if (needle_len > haystack_len) {
        return NULL;
    }

    // First character to match
    char first_char = *needle;

    // Search through haystack
    size pos = 0;
    while (pos <= haystack_len - needle_len) {
        // Find the first character
        if (haystack[pos] != first_char) {
            pos++;
            continue;
        }

        // Compare the substring
        if (MemCompare(haystack + pos, needle, needle_len) == 0) {
            return (char *)(haystack + pos);
        }

        pos++;
    }

    return NULL;
}

// ---------------------------------------------------------------------------
// MemSort: in-tree generic sort. Replaces libc qsort. Quicksort with
// median-of-three pivot, insertion-sort fallback for small partitions
// (<= 16 items), tail-iteration on the larger half to bound stack at
// O(log n). Worst-case O(n^2) on adversarial inputs is accepted -- no
// caller in the project drives sort with attacker-controlled data.
// ---------------------------------------------------------------------------

static inline void memsort_swap(u8 *a, u8 *b, size n) {
    if (a == b)
        return;
    while (n--) {
        u8 t = *a;
        *a++ = *b;
        *b++ = t;
    }
}

static void memsort_insertion(u8 *base, size n, size sz, GenericCompare cmp) {
    for (size i = 1; i < n; ++i) {
        size j = i;
        while (j > 0 && cmp(base + (j - 1) * sz, base + j * sz) > 0) {
            memsort_swap(base + (j - 1) * sz, base + j * sz, sz);
            --j;
        }
    }
}

void MemSort(void *base_, size n_items, size item_size, GenericCompare cmp) {
    if (!base_ || !cmp || item_size == 0 || n_items < 2) {
        return;
    }
    u8  *base = (u8 *)base_;
    size n    = n_items;

    while (n > 16) {
        // Median-of-three pivot: sort the (lo, mid, hi) trio in place so
        // mid ends up as the median, then stash it just before hi as the
        // partition pivot.
        u8 *lo  = base;
        u8 *mid = base + (n / 2) * item_size;
        u8 *hi  = base + (n - 1) * item_size;
        if (cmp(lo, mid) > 0)
            memsort_swap(lo, mid, item_size);
        if (cmp(lo, hi) > 0)
            memsort_swap(lo, hi, item_size);
        if (cmp(mid, hi) > 0)
            memsort_swap(mid, hi, item_size);
        // Pivot now lives at `mid`; move it to position n-2 so we can
        // partition over the inner range [1 .. n-2].
        u8 *pivot = base + (n - 2) * item_size;
        memsort_swap(mid, pivot, item_size);

        size i = 0;
        size j = n - 2;
        for (;;) {
            do {
                ++i;
            } while (cmp(base + i * item_size, pivot) < 0);
            do {
                --j;
            } while (j > 0 && cmp(base + j * item_size, pivot) > 0);
            if (i >= j)
                break;
            memsort_swap(base + i * item_size, base + j * item_size, item_size);
        }
        // Drop pivot into its final slot.
        memsort_swap(base + i * item_size, pivot, item_size);

        // Recurse on the smaller partition; iterate on the larger to
        // keep stack depth bounded at O(log n).
        size left_n  = i;
        size right_n = n - i - 1;
        u8  *right   = base + (i + 1) * item_size;
        if (left_n < right_n) {
            MemSort(base, left_n, item_size, cmp);
            base = right;
            n    = right_n;
        } else {
            MemSort(right, right_n, item_size, cmp);
            n = left_n;
        }
    }

    memsort_insertion(base, n, item_size, cmp);
}
