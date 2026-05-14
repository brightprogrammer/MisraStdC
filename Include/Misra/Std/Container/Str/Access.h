/// file      : std/container/str/access.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Access functions for Str

#ifndef MISRA_STD_CONTAINER_STR_ACCESS_H
#define MISRA_STD_CONTAINER_STR_ACCESS_H

#include "Type.h"

#ifdef __cplusplus
extern "C" {
#endif

///
/// Get string length in characters.
///
/// str[in] : String to query.
///
/// SUCCESS : Length of string.
/// FAILURE : Function cannot fail.
///
/// TAGS: Str, Length, Query
///
#define StrLen(str) VecLen(str)

///
/// Check whether string is empty.
///
/// str[in] : String to query.
///
/// SUCCESS : `true` when string length is 0.
/// FAILURE : `false`
///
/// TAGS: Str, Empty, Query
///
#define StrEmpty(str) (StrLen(str) == 0)

///
/// Access the first character of the string by value. Caller must ensure the
/// string is non-empty.
///
/// str[in] : String to query.
///
/// TAGS: Str, Access, First
///
#define StrFirst(str) VecFirst(str)

///
/// Access the last character of the string by value. Caller must ensure the
/// string is non-empty.
///
/// str[in] : String to query.
///
/// TAGS: Str, Access, Last
///
#define StrLast(str) VecLast(str)

///
/// Pointer to the first character of the string. Equivalent to `str->data`.
///
/// str[in] : String to query.
///
/// TAGS: Str, Access, Iterator, Begin
///
#define StrBegin(str) VecBegin(str)

///
/// Pointer one past the last character of the string. Suitable as an iteration
/// sentinel for `[begin, end)` loops.
///
/// str[in] : String to query.
///
/// TAGS: Str, Access, Iterator, End
///
#define StrEnd(str) VecEnd(str)

///
/// Access the character at `idx` by value. Caller must ensure `idx < length`.
///
/// str[in] : String to query.
/// idx[in] : Index in [0, length).
///
/// TAGS: Str, Access, Index
///
#define StrCharAt(str, idx) VecAt(str, idx)

///
/// Pointer to the character at `idx`. Caller must ensure `idx < length`.
///
/// str[in] : String to query.
/// idx[in] : Index in [0, length).
///
/// TAGS: Str, Access, Index, Pointer
///
#define StrCharPtrAt(str, idx) VecPtrAt(str, idx)

#ifdef __cplusplus
}
#endif

#endif // MISRA_STD_CONTAINER_STR_ACCESS_H
