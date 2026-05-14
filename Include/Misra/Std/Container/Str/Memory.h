/// file      : std/container/str/memory.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Memory management functions for Str

#ifndef MISRA_STD_CONTAINER_STR_MEMORY_H
#define MISRA_STD_CONTAINER_STR_MEMORY_H

#include "Type.h"

#ifdef __cplusplus
extern "C" {
#endif

///
/// Try to shrink the allocated capacity of the string back to its current
/// length. Use when previous growth left a large unused tail.
///
/// str[in,out] : Str handle.
///
/// SUCCESS : Returns `true`.
/// FAILURE : Returns `false` on allocation failure during the shrink reallocation.
///           The string is unchanged.
///
/// TAGS: Str, Memory, ReduceSpace
///
#define StrTryReduceSpace(str) VecTryReduceSpace(str)

///
/// Aborting variant of `StrTryReduceSpace`.
///
/// TAGS: Str, Memory, ReduceSpace, Must, Abort
///
#define StrMustTryReduceSpace(str) VecMustTryReduceSpace(str)

///
/// Swap the characters at two given indices in place.
///
/// str[in,out] : Str handle.
/// idx1[in]    : First index in [0, length).
/// idx2[in]    : Second index in [0, length).
///
/// TAGS: Str, Memory, Swap
///
#define StrSwapCharAt(str, idx1, idx2) VecSwapItems((str), (idx1), (idx2))

///
/// Resize the string to exactly `len` characters. Truncates when shrinking and
/// allocates when growing. New characters (when growing) are zero-initialized.
///
/// str[in,out] : Str handle.
/// len[in]     : New length.
///
/// SUCCESS : Returns `true`.
/// FAILURE : Returns `false` on allocation failure when growth is needed. The string
///           is unchanged.
///
/// TAGS: Str, Memory, Resize
///
#define StrResize(str, len) VecResize((str), (len))

///
/// Aborting variant of `StrResize`.
///
/// TAGS: Str, Memory, Resize, Must, Abort
///
#define StrMustResize(str, len) VecMustResize((str), (len))

///
/// Reserve enough capacity to fit at least `n` characters without further
/// allocation. Does not change the string length.
///
/// str[in,out] : Str handle.
/// n[in]       : Minimum capacity in characters.
///
/// SUCCESS : Returns `true`.
/// FAILURE : Returns `false` on allocation failure. The string is unchanged.
///
/// TAGS: Str, Memory, Reserve
///
#define StrReserve(str, n) VecReserve((str), (n))

///
/// Aborting variant of `StrReserve`.
///
/// TAGS: Str, Memory, Reserve, Must, Abort
///
#define StrMustReserve(str, n) VecMustReserve((str), (n))

///
/// Set the string length to 0 while keeping the allocated capacity.
///
/// str[in,out] : Str handle.
///
/// TAGS: Str, Memory, Clear
///
#define StrClear(str) VecClear(str)

///
/// Reverse the characters of the string in place.
///
/// str[in,out] : Str handle.
///
/// TAGS: Str, Memory, Reverse
///
#define StrReverse(str) VecReverse((str))

#ifdef __cplusplus
}
#endif

#endif // MISRA_STD_CONTAINER_STR_MEMORY_H
