/// file      : std/memory.c
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Memory manipulation functions

#include <Misra/Std/Memory.h>
#include <Misra/Sys.h>
#include <Misra/Std/Log.h>

i32 MemCompare(const void *p1, const void *p2, size n) {
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

char *ZstrDupNWithAllocator(const char *src, size n, Allocator alloc) {
    if (!src) {
        LOG_FATAL("Invalid arguments");
    }

    size len = 0;
    while (len < n && src[len])
        len++;

    char *new_str = (char *)AllocatorAlloc(&alloc, len + 1, 1, false);
    if (!new_str) {
        LOG_SYS_ERROR("allocator allocate failed");
        return NULL;
    }

    MemCopy(new_str, src, len);
    new_str[len] = '\0'; // Null-terminate
    return new_str;
}

char *ZstrDupN(const char *src, size n) {
    return ZstrDupNWithAllocator(src, n, DefaultAllocator());
}

char *ZstrDup(const char *src) {
    return ZstrDupN(src, ZstrLen(src));
}

bool ZstrInitClone(const char **dst, const char **src) {
    if (!dst || !src || !*src) {
        LOG_FATAL("Invalid arguments.");
    }

    *dst = ZstrDup(*src);
    return *dst != NULL;
}

bool ZstrInitCloneWithAllocator(void *dst_ptr, const void *src_ptr, const Allocator *alloc) {
    const char **dst = (const char **)dst_ptr;
    const char *const *src = (const char *const *)src_ptr;

    if (!dst || !src || !*src) {
        LOG_FATAL("Invalid arguments.");
    }

    *dst = ZstrDupNWithAllocator(*src, ZstrLen(*src), alloc ? *alloc : DefaultAllocator());
    return *dst != NULL;
}

void ZstrDeinit(const char **zs) {
    if (!zs) {
        LOG_FATAL("Invalid arguments");
    }

    if (*zs)
        FREE(*zs);
}

void ZstrDeinitWithAllocator(void *zs_ptr, const Allocator *alloc) {
    const char **zs = (const char **)zs_ptr;

    if (!zs) {
        LOG_FATAL("Invalid arguments");
    }

    if (*zs) {
        Allocator allocator = alloc ? *alloc : DefaultAllocator();
        AllocatorFree(&allocator, (void *)*zs, ZstrLen(*zs) + 1, 1);
        *zs = NULL;
    }
}

char *ZstrFindSubstring(const char *haystack, const char *needle) {
    if (!haystack || !needle) {
        LOG_FATAL("Invalid arguments");
    }

    const char *p2;
    const char *p1_advance = haystack;
    for (p2 = needle; *p2; p2++) {
        p1_advance++;     // increment ahead of time
    }
    p2 = needle;
    while (*p1_advance) { // test the end of pattern
        p1_advance = haystack;
        while (1) {
            if (!*p2)
                return (char *)haystack;
            if (*p1_advance++ != *p2++)
                break;
        }
        p2 = needle;
        haystack++;
    }
    return NULL;
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
