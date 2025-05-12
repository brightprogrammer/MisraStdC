---
title: "Source-Misra-Std-Container-Str.c"
meta_title: "Source-Misra-Std-Container-Str.c"
description: "Documentation for Source-Misra-Std-Container-Str.c"
date: 2025-05-12T05:00:00Z
# image: "/images/image-placeholder.png"
categories: ["Vec", "Macro", "Generic"]
author: "Siddharth Mishra"
tags: ["vec", "macro", "generic"]
draft: false
---
```c
/// file      : std/str.c
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// copyright : Copyright (c) 2024, Siddharth Mishra, All rights reserved.
///
/// Str implementation

#include <stdarg.h>
#include <stdio.h>

// ct
#include <Misra/Std/Container/Str.h>
#include <Misra/Std/Log.h>

#include "Misra/Std/Utility/StrIter.h"

static Str* string_va_printf(Str* str, const char* fmt, va_list args);

Str* StrPrintf(Str* str, const char* fmt, ...) {
    if (!str || !fmt) {
        LOG_ERROR("invalid arguments");
        return NULL;
    }

    StrClear(str);

    va_list args;
    va_start(args, fmt);
    str = string_va_printf(str, fmt, args);
    va_end(args);

    return str;
}


Str* StrAppendf(Str* str, const char* fmt, ...) {
    if (!str || !fmt) {
        LOG_ERROR("invalid arguments");
        return NULL;
    }

    va_list args;
    va_start(args, fmt);
    str = string_va_printf(str, fmt, args);
    va_end(args);

    return str;
}


Str* string_va_printf(Str* str, const char* fmt, va_list args) {
    if (!str || !fmt) {
        LOG_ERROR("invalid arguments");
        return NULL;
    }

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
    if (!dst || !src) {
        LOG_ERROR("invalid arguments.");
        return false;
    }

    memset(dst, 0, sizeof(Str));
    dst->copy_init   = src->copy_init;
    dst->copy_deinit = src->copy_deinit;
    dst->alignment   = src->alignment;

    VecMerge(dst, src);
    return true;
}


void StrDeinit(Str* copy) {
    VecDeinit(copy);
}

StrIters StrSplitToIters(Str* s, const char* key) {
    if (!s || !key) {
        LOG_ERROR("Invalid arguments.");
        return (StrIters) {0};
    }

    StrIters sv     = VecInit();
    size     keylen = strlen(key);

    const char* prev = s->data;
    const char* end  = s->data + s->length;

    while (prev <= end) {
        const char* next = strstr(prev, key);
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
    if (!s || !key) {
        LOG_ERROR("Invalid arguments.");
        return (Strs) {0};
    }

    Strs sv     = VecInit();
    size keylen = strlen(key);

    const char* prev = s->data;
    const char* end  = s->data + s->length;

    while (prev <= end) {
        const char* next = strstr(prev, key);
        if (next) {
            VecPushBack(&sv, StrInitFromCstr(prev, next - prev)); // exclude delimiter
            prev = next + keylen;                                 // skip past delimiter
        } else {
            VecPushBack(&sv, StrInitFromCstr(prev, end - prev));  // remaining part
            break;
        }
    }

    return sv;
}

// split direction = 0 means both sides
//                 = -1 means from left
//                 = 1 means from right
Str strip_str(Str* s, const char* chars_to_strip, int split_direction) {
    if (!s) {
        LOG_ERROR("Invalid string.");
        return (Str) {0};
    }

    const char* strip_chars = chars_to_strip ? chars_to_strip : " \t\n\r\v\f";
    const char* start       = s->data;
    const char* end         = s->data + s->length - 1;

    // Trim from the left
    if (split_direction <= 0) {
        while (start <= end && strchr(strip_chars, *start)) {
            start++;
        }
    }

    // Trim from the right
    if (split_direction >= 0) {
        while (end >= start && strchr(strip_chars, *end)) {
            end--;
        }
    }

    size new_len = end >= start ? (end - start + 1) : 0;
    return StrInitFromCstr(start, new_len);
}

static inline bool starts_with(const char* data, size data_len, const char* prefix, size prefix_len) {
    return data_len >= prefix_len && memcmp(data, prefix, prefix_len) == 0;
}

static inline bool ends_with(const char* data, size data_len, const char* suffix, size suffix_len) {
    return data_len >= suffix_len && memcmp(data + data_len - suffix_len, suffix, suffix_len) == 0;
}


bool StrStartsWithZstr(const Str* s, const char* prefix) {
    return starts_with(s->data, s->length, prefix, strlen(prefix));
}

bool StrEndsWithZstr(const Str* s, const char* suffix) {
    return ends_with(s->data, s->length, suffix, strlen(suffix));
}

bool StrStartsWithCstr(const Str* s, const char* prefix, size prefix_len) {
    return starts_with(s->data, s->length, prefix, prefix_len);
}

bool StrEndsWithCstr(const Str* s, const char* suffix, size suffix_len) {
    return ends_with(s->data, s->length, suffix, suffix_len);
}

bool StrStartsWith(const Str* s, const Str* prefix) {
    return starts_with(s->data, s->length, prefix->data, prefix->length);
}

bool StrEndsWith(const Str* s, const Str* suffix) {
    return ends_with(s->data, s->length, suffix->data, suffix->length);
}

// Helper: replace in-place all `match` → `replacement` up to `count`
static void
    str_replace(Str* s, const char* match, size match_len, const char* replacement, size replacement_len, size count) {
    size i        = 0;
    size replaced = 0;

    while (i + match_len <= s->length && replaced < count) {
        if (memcmp(s->data + i, match, match_len) == 0) {
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
    str_replace(s, match, strlen(match), replacement, strlen(replacement), count);
}

void StrReplaceCstr(
    Str*        s,
    const char* match,
    size        match_len,
    const char* replacement,
    size        replacement_len,
    size        count
) {
    str_replace(s, match, match_len, replacement, replacement_len, count);
}

void StrReplace(Str* s, const Str* match, const Str* replacement, size count) {
    str_replace(s, match->data, match->length, replacement->data, replacement->length, count);
}

```