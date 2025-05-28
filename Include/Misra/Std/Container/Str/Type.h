/// file      : std/container/str/type.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// copyright : Copyright (c) 2025, Siddharth Mishra, All rights reserved.
///
/// Basic type definitions for Str

#ifndef MISRA_STD_CONTAINER_STR_TYPE_H
#define MISRA_STD_CONTAINER_STR_TYPE_H

#include <string.h>
#include <Misra/Std/Container/Vec.h>
#include <Misra/Std/Utility/StrIter.h>
#include <Misra/Types.h>

/// 
/// The Str type is a specialization of Vec for characters
///
typedef Vec(char) Str;

///
/// Vector of strings
///
typedef Vec(Str) Strs;

///
/// Validate a Str object (simply uses Vec validation)
///
#define ValidateStr(s)   ValidateVec(s)

///
/// Validate a Strs object (vector of strings)
///
#define ValidateStrs(sv) ValidateVec(sv)

#ifdef _MSC_VER
    static inline char* strndup(const char* s, size n) {
        size  len     = strnlen(s, n); // Only up to n
        char* new_str = (char*)malloc(len + 1);
        if (!new_str)
            return NULL;

        memcpy(new_str, s, len);
        new_str[len] = '\0'; // Null-terminate
        return new_str;
    }
#endif

#endif // MISRA_STD_CONTAINER_STR_TYPE_H 
