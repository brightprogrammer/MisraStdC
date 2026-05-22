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
/// Capacity in characters: the most the string can hold before the next
/// reallocation. Always `>= StrLen(str)`.
///
/// str[in] : String to query.
///
/// TAGS: Str, Access, Capacity
///
#define StrCapacity(str) VecCapacity(str)

///
/// Allocator backing the string's storage.
///
/// str[in] : String to query.
///
/// TAGS: Str, Access, Allocator
///
#define StrAllocator(str) VecAllocator(str)

///
/// Check whether string is empty.
///
/// str[in] : String to query.
///
/// SUCCESS : Returns `true` when string length is 0.
/// FAILURE : Returns `false` when the string contains at least one
///           character. The string is not modified.
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

///
/// Aligned byte offset of character `idx` from the start of the string's
/// storage buffer. Same contract as VecAlignedOffsetAt, specialised for the
/// char element.
///
/// str[in] : String to query.
/// idx[in] : Character index.
///
/// TAGS: Str, Access, Alignment
///
#define StrAlignedOffsetAt(str, idx) VecAlignedOffsetAt(str, idx)

///
/// Total used storage in bytes for the string. Same contract as VecSize,
/// specialised for the char element.
///
/// str[in] : String to query.
///
/// TAGS: Str, Access, Size, Bytes
///
#define StrSize(str) VecSize(str)

///
/// Find the first character in the string equal to the value at `chr_ptr`,
/// using the provided comparator. Same contract as VecFind, specialised for
/// the char element.
///
/// str[in]      : String to search.
/// chr_ptr[in]  : Pointer to the character value to search for.
/// compare[in]  : Comparator returning `0` for equality.
///
/// SUCCESS : Returns the zero-based index of the first matching character.
///           The string is not modified.
/// FAILURE : Returns `SIZE_MAX` when no character matches. The string is not
///           modified.
///
/// TAGS: Str, Find, Search, Compare
///
#define StrFind(str, chr_ptr, compare) VecFind((str), (chr_ptr), (compare))

///
/// Check whether the string contains a matching character. Same contract as
/// VecContains, specialised for the char element.
///
/// str[in]      : String to search.
/// chr_ptr[in]  : Pointer to the character value to search for.
/// compare[in]  : Comparator returning `0` for equality.
///
/// SUCCESS : Returns `true` when at least one matching character exists.
/// FAILURE : Returns `false` when no character matches.
///
/// TAGS: Str, Contains, Search, Compare
///
#define StrContainsChar(str, chr_ptr, compare) VecContains((str), (chr_ptr), (compare))

#ifdef __cplusplus
}
#endif

#endif // MISRA_STD_CONTAINER_STR_ACCESS_H
