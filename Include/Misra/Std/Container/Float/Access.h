/// file      : std/container/float/access.h
/// author    : Generated following Misra project patterns
/// This is free and unencumbered software released into the public domain.
///
/// State inspection helpers for Float.

#ifndef MISRA_STD_CONTAINER_FLOAT_ACCESS_H
#define MISRA_STD_CONTAINER_FLOAT_ACCESS_H

#include "Type.h"

#ifdef __cplusplus
extern "C" {
#endif

///
/// Test whether a floating-point value is exactly zero.
///
/// value[in] : Float to test
///
/// RETURNS: `true` when the significand is zero.
///
/// USAGE:
///   bool zero = FloatIsZero(&value);
///
/// TAGS: Float, Access, Zero, Predicate
///
bool FloatIsZero(Float *value);
///
/// Test whether a floating-point value is negative.
///
/// value[in] : Float to test
///
/// RETURNS: `true` when the value is non-zero and has a negative sign.
///
/// USAGE:
///   bool negative = FloatIsNegative(&value);
///
/// TAGS: Float, Access, Negative, Predicate
///
bool FloatIsNegative(Float *value);
///
/// Read the base-10 exponent of a float.
///
/// value[in] : Float to inspect
///
/// RETURNS: Decimal exponent used by the normalized representation.
///
/// USAGE:
///   i64 exp = FloatExponent(&value);
///
/// TAGS: Float, Access, Exponent
///
i64  FloatExponent(Float *value);

#ifdef __cplusplus
}
#endif

#endif // MISRA_STD_CONTAINER_FLOAT_ACCESS_H
