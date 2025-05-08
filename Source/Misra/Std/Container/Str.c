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
    size_t n = vsnprintf(NULL, 0, fmt, args);
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
    }

    if (copy->data) {
        memset(copy->data, 0, copy->length);
        free(copy->data);
    }

    memset(copy, 0, sizeof(Str));
}
