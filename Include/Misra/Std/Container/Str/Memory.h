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
/// Try reducing memory footprint of string.
/// This is to be used when we know actual allocated memory for vec is large,
/// and we won't need it in future, so we can reduce it to whatever's required at
/// the moment.
///
/// str[in,out] : Str
///
/// SUCCESS : return
/// FAILURE : Does not return
///
#define StrTryReduceSpace(str) VecTryReduceSpace(str)

///
/// Swap chars at given indices.
///
/// str[in,out] : Str to swap chars in.
/// idx1[in]  : Index/Position of first char.
/// idx1[in]  : Index/Position of second char.
///
/// SUCCESS : return
/// FAILURE : Does not return
///
#define StrSwapCharAt(str, idx1, idx2) VecSwapItems((str), (idx1), (idx2))

///
/// Resize string.
/// If length is smaller than current capacity, string length is shrinked.
/// If length is greater than current capacity, space is reserved and string is expanded.
///
/// vec[in,out] : Str to be resized.
/// len[in]     : New length of string.
///
/// SUCCESS : return
/// FAILURE : Does not return
///
#define StrResize(str, len) VecResize((str), (len))

///
/// Reserve space for string.
///
/// vec[in,out] : Str to be resized.
/// len[in]     : New capacity of string.
///
/// SUCCESS : return
/// FAILURE : Does not return
///
#define StrReserve(str, n) VecReserve((str), (n))

///
/// Set string length to 0.
///
/// vec[in,out] : Str to be cleared.
///
/// SUCCESS :
/// FAILURE : NULL
///
#define StrClear(str) VecClear(str)

///
/// Reverse contents of this string.
///
/// str[in,out] : Str to be reversed.
///
/// SUCCESS : `str`
/// FAILURE : NULL
///
#define StrReverse(str) VecReverse((str))

#ifdef __cplusplus
}
#endif

#endif // MISRA_STD_CONTAINER_STR_MEMORY_H

