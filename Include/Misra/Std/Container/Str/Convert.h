/// file      : std/container/str/convert.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// String conversion functions for Str

#ifndef MISRA_STD_CONTAINER_STR_CONVERT_H
#define MISRA_STD_CONTAINER_STR_CONVERT_H

#include "Type.h"

#ifdef __cplusplus
extern "C" {
#endif

///
/// Convert an unsigned 64-bit integer to string with given base
///
/// str[out]    : String to store the result in
/// value[in]   : Value to convert
/// base[in]    : Base to use (2-36)
/// uppercase[in] : Whether to use uppercase letters for bases > 10
///
/// SUCCESS : Returns str
/// FAILURE : Returns NULL if base is invalid or memory allocation fails
///
Str* StrFromU64(Str* str, u64 value, u8 base, bool uppercase);

///
/// Convert a signed 64-bit integer to string with given base
///
/// str[out]    : String to store the result in
/// value[in]   : Value to convert
/// base[in]    : Base to use (2-36)
/// uppercase[in] : Whether to use uppercase letters for bases > 10
///
/// SUCCESS : Returns str
/// FAILURE : Returns NULL if base is invalid or memory allocation fails
///
Str* StrFromI64(Str* str, i64 value, u8 base, bool uppercase);

///
/// Convert a double to string with scientific notation if needed
///
/// str[out]     : String to store the result in
/// value[in]    : Value to convert
/// precision[in] : Number of decimal places (1-17)
/// force_sci[in] : Force scientific notation even for normal numbers
/// uppercase[in] : Whether to use uppercase E for scientific notation
///
/// SUCCESS : Returns str
/// FAILURE : Returns NULL if precision is invalid or memory allocation fails
///
Str* StrFromF64(Str* str, f64 value, u8 precision, bool force_sci, bool uppercase);

///
/// Convert string to unsigned 64-bit integer
///
/// str[in]     : String to convert
/// value[out]  : Where to store the result
/// base[in]    : Base to use (0 for auto-detect, 2-36 otherwise)
///
/// SUCCESS : Returns true and stores result in value
/// FAILURE : Returns false if conversion fails
///
bool StrToU64(const Str* str, u64* value, u8 base);

///
/// Convert string to signed 64-bit integer
///
/// str[in]     : String to convert
/// value[out]  : Where to store the result
/// base[in]    : Base to use (0 for auto-detect, 2-36 otherwise)
///
/// SUCCESS : Returns true and stores result in value
/// FAILURE : Returns false if conversion fails
///
bool StrToI64(const Str* str, i64* value, u8 base);

///
/// Convert string to double
///
/// str[in]     : String to convert
/// value[out]  : Where to store the result
///
/// SUCCESS : Returns true and stores result in value
/// FAILURE : Returns false if conversion fails
///
bool StrToF64(const Str* str, f64* value);

#ifdef __cplusplus
}
#endif

#endif // MISRA_STD_CONTAINER_STR_CONVERT_H 
