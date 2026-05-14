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
            Int *: float_from_int,                                                                                     \
            unsigned char: float_from_u64,                                                                             \
            unsigned short: float_from_u64,                                                                            \
            unsigned int: float_from_u64,                                                                              \
            unsigned long: float_from_u64,                                                                             \
            unsigned long long: float_from_u64,                                                                        \
            signed char: float_from_i64,                                                                               \
            signed short: float_from_i64,                                                                              \
            signed int: float_from_i64,                                                                                \
            signed long: float_from_i64,                                                                               \
            signed long long: float_from_i64,                                                                          \
            float: float_from_f32,                                                                                     \
            double: float_from_f64                                                                                     \
        )

///
/// Convert a numeric value into an arbitrary-precision float.
/// Dispatches on the type of `value`.
///
/// value[in] : Integer, `Int`, `float`, or `double` source value
///
/// SUCCESS : Returns Float representing the same numeric value.
///
/// USAGE:
///   Float value = FloatFrom(42);
///
/// TAGS: Float, Convert, Import, Generic
///
#    define FloatFrom(value) FLOAT_FROM_DISPATCH(value)(value)
#endif

    ///
    /// Convert a float to an arbitrary-precision integer (truncation toward
    /// zero - the fractional portion is discarded).
    ///
    /// result[out] : Destination integer.
    /// value[in]   : Float to convert.
    ///
    /// SUCCESS : Returns `true`. `*result` holds the integer part of
    ///           `value` (sign preserved). The source float is unchanged.
    /// FAILURE : Returns `false` on allocation failure during the
    ///           magnitude transfer. `*result` is left in a valid but
    ///           unspecified state.
    ///
    /// TAGS: Float, Convert, Int, Truncate
    ///
    bool FloatToInt(Int *result, Float *value);

    ///
    /// Parse a decimal string into a float.
    /// Supports an optional sign, decimal point, and scientific exponent.
    ///
    bool FloatTryFromStr(Float *out, const char *text);

    ///
    /// Compatibility wrapper for `FloatTryFromStr(...)`.
    ///
    /// SUCCESS : Returns Parsed floating-point value, or zero on failure.
    ///
    Float FloatFromStr(const char *text);

    ///
    /// Convert a float to a decimal string using an explicit allocator.
    ///
    /// out[out]  : Destination string.
    /// value[in] : Float to convert.
    /// alloc[in] : Allocator to bind to the produced string.
    ///
    /// SUCCESS : Returns `true`. The result has been computed and the
    ///           destination object updated.
    /// FAILURE : Returns `false` on allocation failure. The destination is left
    ///           in a valid but unspecified state on partial failure.
    ///
    /// TAGS: Float, Convert, String, Allocator
    ///
    bool FloatTryToStrAlloc(Str *out, Float *value, Allocator alloc);

    ///
    /// Convert a float to a decimal string using the default allocator.
    ///
    /// out[out]  : Destination string.
    /// value[in] : Float to convert.
    ///
    /// SUCCESS : Returns `true`. `*out` holds the decimal representation of
    ///           `value` (sign, integer part, fractional part).
    /// FAILURE : Returns `false` on allocation failure. `*out` is left in
    ///           a valid but unspecified state.
    ///
    /// TAGS: Float, Convert, String, Decimal
    ///
    bool FloatTryToStr(Str *out, Float *value);

    ///
    /// Convert a float to a decimal string using the default allocator.
    ///
    /// value[in] : Float to convert.
    ///
    /// SUCCESS : Returns a freshly allocated `Str` holding the decimal
    ///           representation of `value`.
    /// FAILURE : Returns an empty `Str` on allocation failure. Use
    ///           `FloatTryToStr` if you need explicit failure propagation.
    ///
    /// TAGS: Float, Convert, String, Decimal
    ///
    Str FloatToStr(Float *value);

#ifdef __cplusplus
}
#endif

#endif // MISRA_STD_CONTAINER_FLOAT_CONVERT_H
