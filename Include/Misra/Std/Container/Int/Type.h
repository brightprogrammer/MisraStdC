/// file      : std/container/int/type.h
/// author    : Generated following Misra project patterns
/// This is free and unencumbered software released into the public domain.
///
/// Int stores non-negative arbitrary-precision integers using BitVec storage.

#ifndef MISRA_STD_CONTAINER_INT_TYPE_H
#define MISRA_STD_CONTAINER_INT_TYPE_H

#include <Misra/Std/Container/BitVec/Type.h>

typedef struct {
    BitVec bits;
} Int;

static inline void ValidateInt(const Int *value) {
    ValidateBitVec(value ? &value->bits : NULL);
}

#endif // MISRA_STD_CONTAINER_INT_TYPE_H
