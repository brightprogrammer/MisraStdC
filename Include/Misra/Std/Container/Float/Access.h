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

bool FloatIsZero(Float *value);
bool FloatIsNegative(Float *value);
i64  FloatExponent(Float *value);

#ifdef __cplusplus
}
#endif

#endif // MISRA_STD_CONTAINER_FLOAT_ACCESS_H
