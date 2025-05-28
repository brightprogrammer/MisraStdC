/// file      : std/container/str/insert.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// copyright : Copyright (c) 2025, Siddharth Mishra, All rights reserved.
///
/// Insertion functions for Str

#ifndef MISRA_STD_CONTAINER_STR_INSERT_H
#define MISRA_STD_CONTAINER_STR_INSERT_H

#include "Type.h"

#ifdef __cplusplus
extern "C" {
#endif

///
/// Insert char into string of it's type.
/// Insertion index must not exceed string length.
///
/// str[in] : Str to insert char into
/// chr[in] : Character to be inserted
/// idx[in] : Index to insert char at.
///
/// SUCCESS : Returns `str` the string itself on success.
/// FAILURE : Returns `NULL` otherwise.
///
#define StrInsertCharAt(str, chr, idx) VecInsertR((str), (chr), (idx))

///
/// Insert a string of given length into given Str at given index.
///
/// str[in,out] : Str object to insert into.
/// zstr[in]    : Zero-terminated string to be inserted.
/// idx[in]     : Index to insert the string at.
/// len[in]     : Length of string or number of bytes to insert.
///
/// SUCCESS : return
/// FAILURE : Does not return
///
#define StrInsertCstr(str, cstr, idx, len) VecInsertRangeR((str), (cstr), (idx), (len))

///
/// Insert a zero-terminated string into given Str at given index.
///
/// str[in,out] : Str object to insert into.
/// zstr[in]    : Zero-terminated string to be inserted.
/// idx[in]     : Index to insert the string at.
///
/// SUCCESS : return
/// FAILURE : Does not return
///
#define StrInsertZstr(str, zstr, idx) StrInsertCstr((str), (zstr), (idx), strlen(zstr))

///
/// Insert contents of `str2` into `str` at given index.
///
/// str[in,out] : Str object to insert into.
/// str2[in]    : Str object to be inserted.
/// idx[in]     : Index to insert at.
///
/// SUCCESS : return
/// FAILURE : Does not return
///
#define StrInsert(str, str2, idx) StrInsertCstr((str), (str2)->data, (idx), (str2)->length)

///
/// Push a array of characters with given length into this string at the given
/// position.
///
/// str[in,out] : Str to insert array chars into.
/// cstr[in]    : array of characters with given length to be inserted.
/// len [in]    : Number of characters to be appended.
///
/// SUCCESS : `str`
/// FAILURE : NULL
///
#define StrPushCstr(str, cstr, len, pos) VecInsertRangeR((str), (cstr), (pos), (len))

///
/// Push a null-terminated string to this string
/// at given position.
///
/// str[in,out] : Str to insert array chars into.
/// zstr[in]    : Null-terminated string to be appended.
///
/// SUCCESS : `str`
/// FAILURE : NULL
///
#define StrPushZstr(str, zstr, pos) StrPushCstr((str), (zstr), strlen(zstr), (pos))

///
/// Push an array of chars with given length to the back of this string.
///
/// str[in,out] : Str to insert array chars into.
/// cstr[in]    : array of characters with given length to be inserted.
/// len [in]    : Number of characters to be appended.
///
/// SUCCESS : `str`
/// FAILURE : NULL
///
#define StrPushBackCstr(str, cstr, len) VecPushBackArrR((str), (cstr), (len))

///
/// Push a null-terminated string to the back of string.
///
/// str[in,out] : Str to insert array chars into.
/// zstr[in]    : Null-terminated string to be appended.
///
/// SUCCESS : `str`
/// FAILURE : NULL
///
#define StrPushBackZstr(str, zstr) StrPushBackCstr((str), (zstr), strlen((zstr)))

///
/// Push a array of characters with given length to the front of this string
///
/// str[in,out] : Str to insert array chars into.
/// cstr[in]    : array of characters with given length to be inserted.
/// len [in]    : Number of characters to be appended.
///
/// SUCCESS : `str`
/// FAILURE : NULL
///
#define StrPushFrontCstr(str, cstr, len) VecPushFrontArrR((str), (cstr), (len))

///
/// Push a null-terminated string to the front of this string.
///
/// str[in,out] : Str to insert array chars into.
/// zstr[in]    : Null-terminated string to be appended.
///
/// SUCCESS : `str`
/// FAILURE : NULL
///
#define StrPushFrontZstr(str, zstr) StrPushFrontCstr((str), (zstr), strlen((zstr)))

///
/// Push char into string.
///
/// str[in] : Str to push char into
/// chr[in] : Pointer to value to be pushed
///
/// SUCCESS : Returns `str` the string itself on success.
/// FAILURE : Returns `NULL` otherwise.
///
#define StrPushBack(str, chr) VecPushBackR((str), (chr))

///
/// Push char into string front.
///
/// str[in] : Str to push char into
/// chr[in] : Pointer to value to be pushed
///
/// SUCCESS : Returns `str` the string itself on success.
/// FAILURE : Returns `NULL` otherwise.
///
#define StrPushFront(str, chr) VecPushFrontR((str), (chr))

///
/// Merge two strings and store the result in first string.
///
/// str[in,out] : Str to insert array chars into.
/// str2[in]    : Str to be inserted.
///
/// SUCCESS : `str`
/// FAILURE : NULL
///
#define StrMerge(str, str2) VecMergeR((str), (str2))

    ///
    /// Print and append into given string object with given format.
    ///
    /// str[in,out] : Str to print into.
    /// fmt[in] : Format string, followed by variadic arguments.
    ///
    /// SUCCESS : `str`
    /// FAILURE : NULL
    ///
    Str* StrAppendf(Str* str, const char* fmt, ...) FORMAT_STRING(2, 3);

#ifdef __cplusplus
}
#endif

#endif // MISRA_STD_CONTAINER_STR_INSERT_H 
