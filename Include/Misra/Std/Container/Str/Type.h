/// file      : std/container/str/type.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Basic type definitions for Str

#ifndef MISRA_STD_CONTAINER_STR_TYPE_H
#define MISRA_STD_CONTAINER_STR_TYPE_H

#include <string.h>
#include <Misra/Std/Container/Vec.h>
#include <Misra/Std/Utility/StrIter.h>
#include <Misra/Types.h>

/// 
/// The Str type is a specialization of Vec for characters
///
typedef Vec(char) Str;

///
/// Vector of strings
///
typedef Vec(Str) Strs;

///
/// Validate a Str object (simply uses Vec validation)
///
#define ValidateStr(s)   ValidateVec(s)

///
/// Validate a Strs object (vector of strings)
///
#define ValidateStrs(sv) ValidateVec(sv)

#endif // MISRA_STD_CONTAINER_STR_TYPE_H 
