/// file      : std/container/int/access.h
/// author    : Generated following Misra project patterns
/// This is free and unencumbered software released into the public domain.
///
/// Numeric state inspection helpers for Int.

#ifndef MISRA_STD_CONTAINER_INT_ACCESS_H
#define MISRA_STD_CONTAINER_INT_ACCESS_H

#include "Type.h"

#ifdef __cplusplus
extern "C" {
#endif

u64  IntBitLength(Int *value);
u64  IntByteLength(Int *value);
u64  IntLog2(Int *value);
u64  IntTrailingZeroCount(Int *value);
bool IntIsZero(Int *value);
bool IntIsOne(Int *value);
bool IntIsEven(Int *value);
bool IntIsOdd(Int *value);
bool IntFitsU64(Int *value);
bool IntIsPowerOfTwo(Int *value);

#ifdef __cplusplus
}
#endif

#endif // MISRA_STD_CONTAINER_INT_ACCESS_H
