/// file      : std/container/str/remove.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Removal functions for Str

#ifndef MISRA_STD_CONTAINER_STR_REMOVE_H
#define MISRA_STD_CONTAINER_STR_REMOVE_H

#include "Type.h"

#ifdef __cplusplus
extern "C" {
#endif

///
/// Remove the last character from the string and optionally store it.
///
/// str[in,out] : Str handle.
/// chr[out]    : Optional destination for the popped character. Pass `NULL` to
///               just delete the character.
///
/// TAGS: Str, Remove, Pop, Back
///
#define StrPopBack(str, chr) VecPopBack((str), (chr))

///
/// Remove the first character from the string and optionally store it.
///
/// str[in,out] : Str handle.
/// chr[out]    : Optional destination for the popped character. Pass `NULL` to
///               just delete the character.
///
/// TAGS: Str, Remove, Pop, Front
///
#define StrPopFront(str, chr) VecPopFront((str), (chr))

///
/// Remove the character at `idx` and optionally store it. Trailing characters
/// shift one slot to the left.
///
/// str[in,out] : Str handle.
/// chr[out]    : Optional destination for the removed character. Pass `NULL`
///               to just delete it.
/// idx[in]     : Position in [0, length).
///
/// TAGS: Str, Remove, Char
///
#define StrRemove(str, chr, idx) VecRemove((str), (chr), (idx))

///
/// Remove `count` characters starting at `start` and optionally copy them out
/// into the provided buffer.
///
/// str[in,out] : Str handle.
/// rd[out]     : Optional destination buffer of at least `count` bytes. Pass
///               `NULL` to discard the removed bytes.
/// start[in]   : First removed index.
/// count[in]   : Number of characters to remove.
///
/// TAGS: Str, Remove, Range
///
#define StrRemoveRange(str, rd, start, count) VecRemoveRange((str), (rd), (start), (count))

///
/// Delete the last character of the string.
///
/// str[in,out] : Str handle.
///
/// TAGS: Str, Delete, Back
///
#define StrDeleteLastChar(str) VecDeleteLast(str)

///
/// Delete the character at `idx`. Trailing characters shift one slot to the
/// left.
///
/// str[in,out] : Str handle.
/// idx[in]     : Position in [0, length).
///
/// TAGS: Str, Delete
///
#define StrDelete(str, idx) VecDelete((str), (idx))

///
/// Delete `count` characters starting at `start`.
///
/// str[in,out] : Str handle.
/// start[in]   : First deleted index.
/// count[in]   : Number of characters to delete.
///
/// TAGS: Str, Delete, Range
///
#define StrDeleteRange(str, start, count) VecDeleteRange((str), (start), (count))

#ifdef __cplusplus
}
#endif

#endif // MISRA_STD_CONTAINER_STR_REMOVE_H
