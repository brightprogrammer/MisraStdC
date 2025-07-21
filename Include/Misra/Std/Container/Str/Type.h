/// file      : std/container/str/type.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Basic type definitions for Str

#ifndef MISRA_STD_CONTAINER_STR_TYPE_H
#define MISRA_STD_CONTAINER_STR_TYPE_H

#include <string.h>
#include <Misra/Std/Container/Vec/Type.h>
#include <Misra/Std/Utility/Iter/Type.h>
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
/// Validate whether a given `Str` object is valid.
/// Not foolproof but will work most of the time.
/// Aborts if provided `Str` is not valid.
///
/// s[in] : Pointer to `Str` object to validate.
///
/// SUCCESS: Continue execution, meaning given `Str` object is ___most probably___ valid.
/// FAILURE: `abort`
///
void ValidateStr(const Str* s);

///
/// Validate whether a given `Strs` object is valid.
/// Not foolproof but will work most of the time.
/// Aborts if provided `Strs` is not valid.
///
/// vs[in] : Pointer to `Strs` object to validate.
///
/// SUCCESS: Continue execution, meaning given `Strs` object is ___most probably___ valid.
/// FAILURE: `abort`
///
void ValidateStrs(const Strs* vs);


#endif // MISRA_STD_CONTAINER_STR_TYPE_H
