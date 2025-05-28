/// file      : std/container/str/init.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// copyright : Copyright (c) 2025, Siddharth Mishra, All rights reserved.
///
/// Initialization functions for Str

#ifndef MISRA_STD_CONTAINER_STR_INIT_H
#define MISRA_STD_CONTAINER_STR_INIT_H

#include "Type.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
///
/// Initializes a Str object from a C-style string (`cstr`) with a specified length (`len`).
/// This macro creates a new Str object and copies up to `len` characters from `cstr`.
///
/// cstr[in]    : Pointer to the null-terminated C-style string to initialize from.
/// len[in]     : The number of characters to copy from `cstr`.
///
/// SUCCESS : Returns a newly created Str object with its `data` field pointing to a
///           newly allocated memory containing a copy of the first `len` characters
///           of `cstr`. The `length` and `capacity` fields are set to `len`.
///           `copy_init` and `copy_deinit` are set to NULL, and `alignment` is set to 1.
///
/// FAILURE : Returns a Str object with `data` set to NULL if memory allocation using
///           `ZstrDupN` fails. In such a case, `length` and `capacity` will likely be
///           uninitialized or zero. It's crucial to check the `data` field for NULL
///           after using this macro to handle potential memory allocation errors.
///
#    define StrInitFromCstr(cstr, len)                                                                                 \
        (Str {                                                                                                         \
            .data        = ZstrDupN((char*)(cstr), (len)),                                                             \
            .length      = (len),                                                                                      \
            .capacity    = (len),                                                                                      \
            .copy_init   = NULL,                                                                                       \
            .copy_deinit = NULL,                                                                                       \
            .alignment   = 1                                                                                           \
        })
#else
#    define StrInitFromCstr(cstr, len)                                                                                 \
        ((Str) {.data        = ZstrDupN((char*)(cstr), (len)),                                                         \
                .length      = (len),                                                                                  \
                .capacity    = (len),                                                                                  \
                .copy_init   = NULL,                                                                                   \
                .copy_deinit = NULL,                                                                                   \
                .alignment   = 1})
#endif

///
/// Initializes a Str object from a null-terminated C-style string (`zstr`).
/// This macro calculates the length of `zstr` using `strlen` and then calls
/// `StrInitFromCstr` to create the Str object.
///
/// zstr[in]    : Pointer to the null-terminated C-style string to initialize from.
///
/// SUCCESS : Returns a newly created Str object with its `data` field pointing to a
///           newly allocated memory containing a copy of `zstr`. The `length` and
///           `capacity` fields are set to the length of `zstr`. `copy_init` and
///           `copy_deinit` are set to NULL, and `alignment` is set to 1.
///
/// FAILURE : Returns a Str object with `data` set to NULL if memory allocation using
///           `strndup` (called internally by `StrInitFromCstr`) fails. In such a case,
///           `length` and `capacity` will likely be uninitialized or zero. It's crucial
///           to check the `data` field for NULL after using this macro to handle
///           potential memory allocation errors.
///
#define StrInitFromZstr(zstr) StrInitFromCstr((zstr), strlen(zstr))

///
/// Initialize a Str object using another one
///
#define StrInitFromStr(str) StrInitFromCstr((str)->data, (str)->length)
#define StrDup(str)         StrInitFromStr(str)

    ///
    /// Init the string using the given format string and arguments.
    /// Current contents of string will be cleared out
    ///
    /// str[in,out] : Str to be inited with format string.
    /// fmt[in]     : Format string, with variadic arguments following.
    ///
    /// SUCCESS : `str`
    /// FAILURE : NULL
    ///
    Str* StrPrintf(Str* str, const char* fmt, ...) FORMAT_STRING(2, 3);

#ifdef __cplusplus
///
/// Initialize given string.
///
/// str : Pointer to string memory that needs to be initialized.
///
/// SUCCESS : `str`
/// FAILURE : NULL
///
#    define StrInit() (StrVecInit())
#else
#    define StrInit() ((Str)VecInit())
#endif

///
/// Initialize given string but use memory from stack.
/// Such strings cannot be dynamically resized!!
///
/// str[in] : Pointer to string memory that needs to be initialized.
/// ne[in]  : Number of elements to allocate stack memory for.
///
/// SUCCESS : `str`
/// FAILURE : NULL
///
#define StrInitStack(str, ne, scoped_body) VecInitStack(str, ne, scoped_body)

    ///
    /// Deinit str by freeing all allocations.
    ///
    /// str : Pointer to string to be deinited
    ///
    void StrDeinit(Str* str);

    ///
    /// Copy data from `src` to `dst`
    ///
    /// dst[out] : Str object to copy into.
    /// src[in]  : Str object to copy from.
    ///
    /// SUCCESS : true
    /// FAILURE : false
    ///
    bool StrInitCopy(Str* dst, const Str* src);

#ifdef __cplusplus
}
#endif

#endif // MISRA_STD_CONTAINER_STR_INIT_H 
