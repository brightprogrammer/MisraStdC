/// file      : std/container/int/init.h
/// author    : Generated following Misra project patterns
/// This is free and unencumbered software released into the public domain.
///
/// Initialization helpers for Int.

#ifndef MISRA_STD_CONTAINER_INT_INIT_H
#define MISRA_STD_CONTAINER_INT_INIT_H

#include "Type.h"
#include <Misra/Std/Container/BitVec/Init.h>

static inline Int IntInit(void) {
    Int value;

    value.bits = BitVecInit();
    return value;
}

static inline void IntDeinit(Int *value) {
    ValidateInt(value);
    BitVecDeinit(&value->bits);
}

static inline void IntClear(Int *value) {
    ValidateInt(value);
    BitVecClear(&value->bits);
}

#endif // MISRA_STD_CONTAINER_INT_INIT_H
