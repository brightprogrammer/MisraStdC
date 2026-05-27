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
/// Pass `NULL` for `chr` to discard. See `VecPopBack` for the full
/// SUCCESS/FAILURE contract.
///
/// TAGS: Str, Remove, Pop, Back
///
#define StrPopBack(str, chr) VecPopBack((str), (chr))

///
/// Remove the first character from the string and optionally store it.
/// Trailing characters shift one slot to the left. Pass `NULL` for `chr`
/// to discard. See `VecPopFront` for the full SUCCESS/FAILURE contract.
///
/// TAGS: Str, Remove, Pop, Front
///
#define StrPopFront(str, chr) VecPopFront((str), (chr))

///
/// Remove the character at `idx` and optionally store it; trailing
/// characters shift one slot to the left. Pass `NULL` for `chr` to
/// discard. `idx` must lie in `[0, length)`. See `VecRemove` for the
/// full SUCCESS/FAILURE contract.
///
/// TAGS: Str, Remove, Char
///
#define StrRemove(str, chr, idx) VecRemove((str), (chr), (idx))

///
/// Remove `count` characters starting at `start` and optionally copy
/// them into `rd`. Pass `NULL` for `rd` to discard. See `VecRemoveRange`
/// for the full SUCCESS/FAILURE contract.
///
/// TAGS: Str, Remove, Range
///
#define StrRemoveRange(str, rd, start, count) VecRemoveRange((str), (rd), (start), (count))

///
/// Delete the last character of the string in place. See `VecDeleteLast`
/// for the full SUCCESS/FAILURE contract.
///
/// TAGS: Str, Delete, Back
///
#define StrDeleteLastChar(str) VecDeleteLast(str)

///
/// Delete the character at `idx`; trailing characters shift one slot to
/// the left. `idx` must lie in `[0, length)`. See `VecDelete` for the
/// full SUCCESS/FAILURE contract.
///
/// TAGS: Str, Delete
///
#define StrDelete(str, idx) VecDelete((str), (idx))

///
/// Delete `count` characters starting at `start`; trailing characters
/// shift to fill the hole. See `VecDeleteRange` for the full
/// SUCCESS/FAILURE contract.
///
/// TAGS: Str, Delete, Range
///
#define StrDeleteRange(str, start, count) VecDeleteRange((str), (start), (count))

///
/// Remove the character at `idx` without preserving order: the
/// previously-last character is swapped into the removed slot. Pass
/// `NULL` for `chr` to discard. See `VecRemoveFast` for the full
/// SUCCESS/FAILURE contract.
///
/// TAGS: Str, Remove, Char, Fast, Unordered
///
#define StrRemoveFast(str, chr, idx) VecRemoveFast((str), (chr), (idx))

///
/// Remove `count` characters starting at `start` without preserving
/// order; trailing characters are swapped into the hole. Pass `NULL` for
/// `rd` to discard. See `VecRemoveRangeFast` for the full
/// SUCCESS/FAILURE contract.
///
/// TAGS: Str, Remove, Range, Fast, Unordered
///
#define StrRemoveRangeFast(str, rd, start, count) VecRemoveRangeFast((str), (rd), (start), (count))

///
/// Delete the character at `idx` using the fast (order-not-preserving)
/// path: the previously-last character is swapped into the deleted
/// slot. See `VecDeleteFast` for the full SUCCESS/FAILURE contract.
///
/// TAGS: Str, Delete, Fast, Unordered
///
#define StrDeleteFast(str, idx) VecDeleteFast((str), (idx))

///
/// Delete `count` characters starting at `start` using the fast
/// (order-not-preserving) path. See `VecDeleteRangeFast` for the full
/// SUCCESS/FAILURE contract.
///
/// TAGS: Str, Delete, Range, Fast, Unordered
///
#define StrDeleteRangeFast(str, start, count) VecDeleteRangeFast((str), (start), (count))

#ifdef __cplusplus
}
#endif

#endif // MISRA_STD_CONTAINER_STR_REMOVE_H
