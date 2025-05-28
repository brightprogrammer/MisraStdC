/// file      : std/container/str/access.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// copyright : Copyright (c) 2025, Siddharth Mishra, All rights reserved.
///
/// Access functions for Str

#ifndef MISRA_STD_CONTAINER_STR_ACCESS_H
#define MISRA_STD_CONTAINER_STR_ACCESS_H

#include "Type.h"

#ifdef __cplusplus
extern "C" {
#endif

///
/// Access first character in string
///
#define StrFirst(str)     VecFirst(str)

///
/// Access last character in string
///
#define StrLast(str)      VecLast(str)

///
/// Get pointer to first character in string
///
#define StrBegin(str)     VecBegin(str)

///
/// Get pointer to one past the last character in string
///
#define StrEnd(str)       VecEnd(str)

///
/// Access character at given index
///
#define StrCharAt(str, idx)    VecAt(str, idx)

///
/// Get pointer to character at given index
///
#define StrCharPtrAt(str, idx) VecPtrAt(str, idx)

#ifdef __cplusplus
}
#endif

#endif // MISRA_STD_CONTAINER_STR_ACCESS_H 
