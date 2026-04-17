/// file      : std/container/float/init.h
/// author    : Generated following Misra project patterns
/// This is free and unencumbered software released into the public domain.
///
/// Initialization helpers for Float.

#ifndef MISRA_STD_CONTAINER_FLOAT_INIT_H
#define MISRA_STD_CONTAINER_FLOAT_INIT_H

#include "Type.h"
#include <Misra/Std/Container/Int/Init.h>

static inline Float FloatInit(void) {
    Float value;

    value.negative    = false;
    value.significand = IntInit();
    value.exponent    = 0;
    return value;
}

static inline void FloatDeinit(Float *value) {
    ValidateFloat(value);
    IntDeinit(&value->significand);
    value->negative = false;
    value->exponent = 0;
}

static inline void FloatClear(Float *value) {
    ValidateFloat(value);
    IntClear(&value->significand);
    value->negative = false;
    value->exponent = 0;
}

#endif // MISRA_STD_CONTAINER_FLOAT_INIT_H
