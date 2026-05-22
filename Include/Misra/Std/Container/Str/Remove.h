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

///
/// Remove the character at `idx` without preserving order: the previously-last
/// character is swapped into the removed slot. Same contract as VecRemoveFast,
/// specialised for the char element.
///
/// str[in,out] : Str handle.
/// chr[out]    : Optional destination for the removed character. Pass `NULL`
///               to discard it.
/// idx[in]     : Position in [0, length).
///
/// TAGS: Str, Remove, Char, Fast, Unordered
///
#define StrRemoveFast(str, chr, idx) VecRemoveFast((str), (chr), (idx))

///
/// Remove `count` characters starting at `start` without preserving order.
/// Same contract as VecRemoveRangeFast, specialised for the char element.
///
/// str[in,out] : Str handle.
/// rd[out]     : Optional destination buffer for the removed characters.
///               Pass `NULL` to discard them.
/// start[in]   : First removed index.
/// count[in]   : Number of characters to remove.
///
/// TAGS: Str, Remove, Range, Fast, Unordered
///
#define StrRemoveRangeFast(str, rd, start, count) VecRemoveRangeFast((str), (rd), (start), (count))

///
/// Delete the character at `idx` using the fast (order-not-preserving) path.
/// Same contract as VecDeleteFast, specialised for the char element.
///
/// str[in,out] : Str handle.
/// idx[in]     : Position in [0, length).
///
/// TAGS: Str, Delete, Fast, Unordered
///
#define StrDeleteFast(str, idx) VecDeleteFast((str), (idx))

///
/// Delete `count` characters starting at `start` using the fast
/// (order-not-preserving) path. Same contract as VecDeleteRangeFast,
/// specialised for the char element.
///
/// str[in,out] : Str handle.
/// start[in]   : First deleted index.
/// count[in]   : Number of characters to delete.
///
/// TAGS: Str, Delete, Range, Fast, Unordered
///
#define StrDeleteRangeFast(str, start, count) VecDeleteRangeFast((str), (start), (count))

#ifdef __cplusplus
}
#endif

#endif // MISRA_STD_CONTAINER_STR_REMOVE_H
