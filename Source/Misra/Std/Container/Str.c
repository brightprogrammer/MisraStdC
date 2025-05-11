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


void StrDeinitCopy(Str* copy) {
    if (!copy) {
        LOG_ERROR("invalid arguments.");
        return;
    }

    if (copy->data) {
        memset(copy->data, 0, copy->length);
        free(copy->data);
    }

    memset(copy, 0, sizeof(Str));
}

StrIters split_str_into_iters(Str* s, const char* key) {
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

Strs split_str(Str* s, const char* key) {
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
