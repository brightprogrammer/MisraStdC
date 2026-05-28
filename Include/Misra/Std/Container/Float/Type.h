/// file      : std/container/float/type.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Decimal arbitrary-precision floating-point storage built on top of Int.

#ifndef MISRA_STD_CONTAINER_FLOAT_TYPE_H
#define MISRA_STD_CONTAINER_FLOAT_TYPE_H

#include <Misra/Std/Container/Int/Type.h>

///
/// Decimal arbitrary-precision floating-point value.
/// The number represented is `(-1)^negative * significand * 10^exponent`.
///
/// USAGE:
///   Float value = FloatFromStr("12.5");
///
/// FIELDS:
/// - negative    : Sign flag. Zero is always normalized to non-negative.
/// - significand : Integer significand stored without decimal point.
/// - exponent    : Base-10 exponent applied to the significand.
///
/// TAGS: Float, Decimal, Type, ArbitraryPrecision
///
typedef struct {
    bool negative;
    Int  significand;
    i64  exponent;
} Float;

///
/// Validate whether a given `Float` object is structurally valid.
/// Aborts if the pointer is NULL or the embedded significand is invalid.
///
/// value[in] : Pointer to the `Float` object to validate
///
/// SUCCESS: Returns to caller when `value` appears valid.
/// FAILURE: Aborts the process when `value` is invalid.
///
/// TAGS: Float, Validate, Safety, Debug
///
static inline void ValidateFloat(const Float *value) {
    ValidateInt(value ? &value->significand : NULL);
}

#endif // MISRA_STD_CONTAINER_FLOAT_TYPE_H
