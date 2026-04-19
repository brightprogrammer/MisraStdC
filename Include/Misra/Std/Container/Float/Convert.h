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
#    define MISRA_FLOAT_FROM_DISPATCH(value)                                                                           \
        _Generic(                                                                                                      \
            (value),                                                                                                   \
            Int: MISRA_PRIV_FloatFromValueInt,                                                                         \
            Int *: MISRA_PRIV_FloatFromInt,                                                                            \
            const Int *: MISRA_PRIV_FloatFromConstInt,                                                                 \
            unsigned char: MISRA_PRIV_FloatFromU64,                                                                    \
            unsigned short: MISRA_PRIV_FloatFromU64,                                                                   \
            unsigned int: MISRA_PRIV_FloatFromU64,                                                                     \
            unsigned long: MISRA_PRIV_FloatFromU64,                                                                    \
            unsigned long long: MISRA_PRIV_FloatFromU64,                                                               \
            signed char: MISRA_PRIV_FloatFromI64,                                                                      \
            signed short: MISRA_PRIV_FloatFromI64,                                                                     \
            signed int: MISRA_PRIV_FloatFromI64,                                                                       \
            signed long: MISRA_PRIV_FloatFromI64,                                                                      \
            signed long long: MISRA_PRIV_FloatFromI64,                                                                 \
            float: MISRA_PRIV_FloatFromF32,                                                                            \
            double: MISRA_PRIV_FloatFromF64                                                                            \
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
#    define FloatFrom(value) MISRA_FLOAT_FROM_DISPATCH(value)(value)
#endif
///
/// Convert a float to an integer when no fractional or negative part remains.
///
/// result[out] : Destination integer
/// value[in]   : Float to convert
///
/// RETURNS: `true` on exact non-negative conversion, otherwise `false`.
///
/// USAGE:
///   bool ok = FloatToInt(&integer, &value);
///
/// TAGS: Float, Convert, Int, Export
///
bool  FloatToInt(Int *result, Float *value);
///
/// Parse a decimal string into a float.
/// Supports an optional sign, decimal point, and scientific exponent.
///
/// text[in] : Input string
///
/// RETURNS: Parsed floating-point value.
///
/// WARN: Aborts on malformed input.
///
/// USAGE:
///   Float value = FloatFromStr("-1.25e6");
///
/// TAGS: Float, Convert, String, Parse
///
Float FloatFromStr(const char *text);
///
/// Convert a float to a normalized decimal string.
///
/// value[in] : Float to convert
///
/// RETURNS: Decimal string representation without scientific notation.
///
/// USAGE:
///   Str text = FloatToStr(&value);
///
/// TAGS: Float, Convert, String, Format
///
Str   FloatToStr(Float *value);

#ifdef __cplusplus
}
#endif

#endif // MISRA_STD_CONTAINER_FLOAT_CONVERT_H
