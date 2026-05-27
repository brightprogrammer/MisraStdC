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
/// Shrink the string's allocated capacity back to its current length.
/// Use when previous growth left a large unused tail. See `VecTryReduceSpace`
/// for the full SUCCESS/FAILURE contract.
///
/// TAGS: Str, Memory, ReduceSpace
///
#define StrTryReduceSpace(str) VecTryReduceSpace(str)

///
/// Aborting variant of `StrTryReduceSpace`: shrink the string's capacity
/// to its current length, aborting on allocation failure. See
/// `VecMustTryReduceSpace` for the full SUCCESS/FAILURE contract.
///
/// TAGS: Str, Memory, ReduceSpace, Must, Abort
///
#define StrMustTryReduceSpace(str) VecMustTryReduceSpace(str)

///
/// Swap the characters at two given indices in place. `idx1` and `idx2`
/// must lie in `[0, length)`. See `VecSwapItems` for the full
/// SUCCESS/FAILURE contract.
///
/// TAGS: Str, Memory, Swap
///
#define StrSwapCharAt(str, idx1, idx2) VecSwapItems((str), (idx1), (idx2))

///
/// Resize the string to exactly `len` characters. Truncates when shrinking
/// and allocates when growing; new characters (when growing) are
/// zero-initialised. See `VecResize` for the full SUCCESS/FAILURE
/// contract.
///
/// TAGS: Str, Memory, Resize
///
#define StrResize(str, len) VecResize((str), (len))

///
/// Aborting variant of `StrResize`: resize the string to `len` characters,
/// aborting on allocation failure. See `VecMustResize` for the full
/// SUCCESS/FAILURE contract.
///
/// TAGS: Str, Memory, Resize, Must, Abort
///
#define StrMustResize(str, len) VecMustResize((str), (len))

///
/// Reserve enough capacity to fit at least `n` characters without further
/// allocation. Does not change the string length. See `VecReserve` for
/// the full SUCCESS/FAILURE contract.
///
/// TAGS: Str, Memory, Reserve
///
#define StrReserve(str, n) VecReserve((str), (n))

///
/// Aborting variant of `StrReserve`: reserve `n` characters of capacity,
/// aborting on allocation failure. See `VecMustReserve` for the full
/// SUCCESS/FAILURE contract.
///
/// TAGS: Str, Memory, Reserve, Must, Abort
///
#define StrMustReserve(str, n) VecMustReserve((str), (n))

///
/// Set the string length to 0 while keeping the allocated capacity, so
/// subsequent writes can reuse the buffer without reallocation. See
/// `VecClear` for the full SUCCESS/FAILURE contract.
///
/// TAGS: Str, Memory, Clear
///
#define StrClear(str) VecClear(str)

///
/// Reverse the characters of the string in place. See `VecReverse` for
/// the full SUCCESS/FAILURE contract.
///
/// TAGS: Str, Memory, Reverse
///
#define StrReverse(str) VecReverse((str))

///
/// Reorder the characters of `str` in place into non-decreasing order under
/// `compare`. Typical uses are canonicalising a character set, or sorting
/// characters before a uniqueness pass; length and capacity are unchanged.
///
/// str[in,out]  : Str handle.
/// compare[in]  : Comparator with `strcmp`-style return on two `char`s.
///
/// SUCCESS : Characters are now in non-decreasing order according to `compare`;
///           the string's length is unchanged.
/// FAILURE : Function cannot fail. A NULL comparator or invalid string is a
///           caller bug and aborts via `LOG_FATAL`.
///
/// See `VecSort` for the full SUCCESS/FAILURE contract.
///
/// TAGS: Str, Ops, Sort
///
#define StrSort(str, compare) VecSort((str), (compare))

#ifdef __cplusplus
}
#endif

#endif // MISRA_STD_CONTAINER_STR_MEMORY_H
