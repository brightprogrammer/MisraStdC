/// file      : std/container/float/access.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
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
    /// SUCCESS : Returns `true` when the significand is zero.
    /// FAILURE : Returns `false` when the value is non-zero. Cannot fail
    ///           otherwise.
    ///
    /// USAGE:
    ///   bool zero = FloatIsZero(&value);
    ///
    /// TAGS: Float, Access, Zero, Predicate
    ///
    bool FloatIsZero(const Float *value);
    ///
    /// Test whether a floating-point value is negative.
    ///
    /// value[in] : Float to test
    ///
    /// SUCCESS : Returns `true` when the value is non-zero and has a
    ///           negative sign.
    /// FAILURE : Returns `false` otherwise. Cannot fail.
    ///
    /// USAGE:
    ///   bool negative = FloatIsNegative(&value);
    ///
    /// TAGS: Float, Access, Negative, Predicate
    ///
    bool FloatIsNegative(const Float *value);
    ///
    /// Read the base-10 exponent of a float.
    ///
    /// value[in] : Float to inspect
    ///
    /// SUCCESS : Returns the decimal exponent used by the normalized
    ///           representation.
    /// FAILURE : Cannot fail.
    ///
    /// USAGE:
    ///   i64 exp = FloatExponent(&value);
    ///
    /// TAGS: Float, Access, Exponent
    ///
    i64 FloatExponent(const Float *value);

#ifdef __cplusplus
}
#endif

#endif // MISRA_STD_CONTAINER_FLOAT_ACCESS_H
