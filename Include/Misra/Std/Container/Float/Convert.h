/// file      : std/container/float/convert.h
/// author    : Generated following Misra project patterns
/// This is free and unencumbered software released into the public domain.
///
/// Conversion helpers for Float.

#ifndef MISRA_STD_CONTAINER_FLOAT_CONVERT_H
#define MISRA_STD_CONTAINER_FLOAT_CONVERT_H

#include "Private.h"
#include <Misra/Std/Container/Str.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef __cplusplus
#    define FLOAT_FROM_DISPATCH(value)                                                                                 \
        _Generic(                                                                                                      \
            (value),                                                                                                   \
            Int *: FloatFromInt,                                                                                       \
            unsigned char: FloatFromU64,                                                                               \
            unsigned short: FloatFromU64,                                                                              \
            unsigned int: FloatFromU64,                                                                                \
            unsigned long: FloatFromU64,                                                                               \
            unsigned long long: FloatFromU64,                                                                          \
            signed char: FloatFromI64,                                                                                 \
            signed short: FloatFromI64,                                                                                \
            signed int: FloatFromI64,                                                                                  \
            signed long: FloatFromI64,                                                                                 \
            signed long long: FloatFromI64,                                                                            \
            float: FloatFromF32,                                                                                       \
            double: FloatFromF64                                                                                       \
        )

///
/// Convert a numeric value into an arbitrary-precision float.
/// Dispatches on the type of `value`.
///
/// value[in] : Integer, `Int`, `float`, or `double` source value
///
/// RETURNS: Float representing the same numeric value.
///
/// USAGE:
///   Float value = FloatFrom(42);
///
/// TAGS: Float, Convert, Import, Generic
///
#    define FloatFrom(value) FLOAT_FROM_DISPATCH(value)(value)
#endif

    bool FloatToInt(Int *result, Float *value);

    ///
    /// Parse a decimal string into a float.
    /// Supports an optional sign, decimal point, and scientific exponent.
    ///
    bool FloatTryFromStr(Float *out, const char *text);

    ///
    /// Compatibility wrapper for `FloatTryFromStr(...)`.
    ///
    /// RETURNS: Parsed floating-point value, or zero on failure.
    ///
    Float FloatFromStr(const char *text);

    ///
    /// Convert a float to a decimal string using an explicit allocator.
    ///
    /// out[out]  : Destination string.
    /// value[in] : Float to convert.
    /// alloc[in] : Allocator to bind to the produced string.
    ///
    /// RETURNS: `true` on success, `false` on allocation failure.
    ///
    /// TAGS: Float, Convert, String, Allocator
    ///
    bool FloatTryToStrAlloc(Str *out, Float *value, Allocator alloc);
    bool FloatTryToStr(Str *out, Float *value);
    Str  FloatToStr(Float *value);

#ifdef __cplusplus
}
#endif

#endif // MISRA_STD_CONTAINER_FLOAT_CONVERT_H
