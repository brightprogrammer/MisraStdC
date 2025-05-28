/// file      : std/container/str/access.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// copyright : Copyright (c) 2025, Siddharth Mishra, All rights reserved.
///
/// Access and search functions for Str

#ifndef MISRA_STD_CONTAINER_STR_ACCESS_H
#define MISRA_STD_CONTAINER_STR_ACCESS_H

#include "Type.h"

#ifdef __cplusplus
extern "C" {
#endif

///
/// Compare two Str objects
///
/// str[in]  : First string
/// ostr[in] : Other string
///
/// RETURN : +ve or -ve depending on above or below in lexical ordering
/// RETURN : 0 if both are equal
///
#define StrCmp(str, ostr) memcmp((str)->data, (ostr)->data, MIN2((str)->length, (ostr)->length))

///
/// Compare string with another const char*
///
/// str[in]  : Pointer to Str object to compare with.
/// cstr[in] : String to compare with.
///
/// RETURN : +ve or -ve depending on above or below in lexical ordering
/// RETURN : 0 if both are equal
///
#define StrCmpCstr(str, cstr) strncmp((str)->data, cstr, (str)->length)

#define StrFirst(str)     VecFirst(str)
#define StrLast(str)      VecLast(str)
#define StrBegin(str)     VecBegin(str)
#define StrEnd(str)       VecEnd(str)
#define StrCharAt(str)    VecAt(str, idx)
#define StrCharPtrAt(str) VecPtrAt(str, idx)

///
/// Find a key in a Str object.
///
/// str[in] : Str object to find str into.
/// key[in] : Str object to find in `str`
///
/// SUCCESS : char* providing position of found string. Pointer is inside `str`
/// FAILURE : NULL
///
#define StrFindStr(str, key) strstr((str)->data, (key)->data)

///
/// Find a key in a Str object.
///
/// str[in] : Str object to find str into.
/// key[in] : const char* string to look for
///
/// SUCCESS : char* providing position of found string. Pointer is inside `str`.
/// FAILURE : NULL
///
#define StrFindZstr(str, key) strstr((str)->data, (key))

    ///
    /// Check if string starts with a null-terminated string (Zstr).
    ///
    /// s[in]     : Str to check.
    /// prefix[in]: Null-terminated prefix string.
    ///
    /// SUCCESS : Returns true if `s` starts with `prefix`.
    /// FAILURE : Returns false.
    ///
    bool StrStartsWithZstr(const Str* s, const char* prefix);

    ///
    /// Check if string ends with a null-terminated string (Zstr).
    ///
    /// s[in]     : Str to check.
    /// suffix[in]: Null-terminated suffix string.
    ///
    /// SUCCESS : Returns true if `s` ends with `suffix`.
    /// FAILURE : Returns false.
    ///
    bool StrEndsWithZstr(const Str* s, const char* suffix);

    ///
    /// Check if string starts with a fixed-length C-style string (Cstr).
    ///
    /// s[in]         : Str to check.
    /// prefix[in]    : Pointer to prefix character array.
    /// prefix_len[in]: Length of prefix.
    ///
    /// SUCCESS : Returns true if `s` starts with `prefix`.
    /// FAILURE : Returns false.
    ///
    bool StrStartsWithCstr(const Str* s, const char* prefix, size prefix_len);

    ///
    /// Check if string ends with a fixed-length C-style string (Cstr).
    ///
    /// s[in]         : Str to check.
    /// suffix[in]    : Pointer to suffix character array.
    /// suffix_len[in]: Length of suffix.
    ///
    /// SUCCESS : Returns true if `s` ends with `suffix`.
    /// FAILURE : Returns false.
    ///
    bool StrEndsWithCstr(const Str* s, const char* suffix, size suffix_len);

    ///
    /// Check if string starts with another Str object.
    ///
    /// s[in]     : Str to check.
    /// prefix[in]: Str to check as prefix.
    ///
    /// SUCCESS : Returns true if `s` starts with `prefix`.
    /// FAILURE : Returns false.
    ///
    bool StrStartsWith(const Str* s, const Str* prefix);

    ///
    /// Check if string ends with another Str object.
    ///
    /// s[in]     : Str to check.
    /// suffix[in]: Str to check as suffix.
    ///
    /// SUCCESS : Returns true if `s` ends with `suffix`.
    /// FAILURE : Returns false.
    ///
    bool StrEndsWith(const Str* s, const Str* suffix);

#ifdef __cplusplus
}
#endif

#endif // MISRA_STD_CONTAINER_STR_ACCESS_H 
