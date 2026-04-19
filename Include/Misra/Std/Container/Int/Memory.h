/// file      : std/container/int/memory.h
/// author    : Generated following Misra project patterns
/// This is free and unencumbered software released into the public domain.
///
/// Copy-oriented helpers for Int.

#ifndef MISRA_STD_CONTAINER_INT_MEMORY_H
#define MISRA_STD_CONTAINER_INT_MEMORY_H

#include "Type.h"

#ifdef __cplusplus
extern "C" {
#endif

///
/// Create a deep copy of an integer.
///
/// value[in] : Integer to clone
///
/// RETURNS: Independent copy of `value`.
///
/// USAGE:
///   Int copy = IntClone(&value);
///
/// TAGS: Int, Memory, Clone, Copy
///
Int IntClone(Int *value);

#ifdef __cplusplus
}
#endif

#endif // MISRA_STD_CONTAINER_INT_MEMORY_H
