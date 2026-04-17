/// file      : std/container/float/type.h
/// author    : Generated following Misra project patterns
/// This is free and unencumbered software released into the public domain.
///
/// Decimal arbitrary-precision floating-point storage built on top of Int.

#ifndef MISRA_STD_CONTAINER_FLOAT_TYPE_H
#define MISRA_STD_CONTAINER_FLOAT_TYPE_H

#include <Misra/Std/Container/Int/Type.h>

typedef struct {
    bool negative;
    Int  significand;
    i64  exponent;
} Float;

static inline void ValidateFloat(const Float *value) {
    ValidateInt(value ? &value->significand : NULL);
}

#endif // MISRA_STD_CONTAINER_FLOAT_TYPE_H
