/// file      : std/container/float/memory.h
/// author    : Generated following Misra project patterns
/// This is free and unencumbered software released into the public domain.
///
/// Copy-oriented helpers for Float.

#ifndef MISRA_STD_CONTAINER_FLOAT_MEMORY_H
#define MISRA_STD_CONTAINER_FLOAT_MEMORY_H

#include "Type.h"

#ifdef __cplusplus
extern "C" {
#endif

///
/// Create a deep copy of a floating-point value.
///
/// value[in] : Float to clone
///
/// RETURNS: Independent copy of `value`.
///
/// USAGE:
///   Float copy = FloatClone(&value);
///
/// TAGS: Float, Memory, Clone, Copy
///
Float FloatClone(Float *value);

#ifdef __cplusplus
}
#endif

#endif // MISRA_STD_CONTAINER_FLOAT_MEMORY_H
