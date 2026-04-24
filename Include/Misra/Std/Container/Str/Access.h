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
/// Access first character in string
///
#define StrFirst(str) VecFirst(str)

///
/// Access last character in string
///
#define StrLast(str) VecLast(str)

///
/// Get pointer to first character in string
///
#define StrBegin(str) VecBegin(str)

///
/// Get pointer to one past the last character in string
///
#define StrEnd(str) VecEnd(str)

///
/// Access character at given index
///
#define StrCharAt(str, idx) VecAt(str, idx)

///
/// Get pointer to character at given index
///
#define StrCharPtrAt(str, idx) VecPtrAt(str, idx)

#ifdef __cplusplus
}
#endif

#endif // MISRA_STD_CONTAINER_STR_ACCESS_H
