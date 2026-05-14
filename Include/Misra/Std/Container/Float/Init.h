/// file      : std/container/float/init.h
/// author    : Generated following Misra project patterns
/// This is free and unencumbered software released into the public domain.
///
/// Initialization helpers for Float.

#ifndef MISRA_STD_CONTAINER_FLOAT_INIT_H
#define MISRA_STD_CONTAINER_FLOAT_INIT_H

#include "Type.h"
#include <Misra/Std/Container/Int/Init.h>

///
/// Initialize a Float (numeric value 0) bound to an `Allocator *`. The
/// argument is a raw `Allocator *` (use `&heap.base`,
/// `ALLOCATOR_OF(&heap)`, or `MisraScope` to obtain one).
///
/// USAGE:
///   Float value = FloatInit(MisraScope);
///
/// TAGS: Float, Init, Zero, Construct
///
#define FloatInit(alloc_ptr) ((Float) {.negative = false, .significand = IntInit(alloc_ptr), .exponent = 0})

///
/// Release all storage owned by a floating-point value.
///
/// value[in] : Float to deinitialize
///
/// USAGE:
///   FloatDeinit(&value);
///
/// TAGS: Float, Deinit, Destroy, Memory
///
static inline void FloatDeinit(Float *value) {
    ValidateFloat(value);
    IntDeinit(&value->significand);
    value->negative = false;
    value->exponent = 0;
}

///
/// Reset a floating-point value to numeric zero.
///
/// value[in] : Float to clear
///
/// USAGE:
///   FloatClear(&value);
///
/// TAGS: Float, Clear, Zero, Reset
///
static inline void FloatClear(Float *value) {
    ValidateFloat(value);
    IntClear(&value->significand);
    value->negative = false;
    value->exponent = 0;
}

#endif // MISRA_STD_CONTAINER_FLOAT_INIT_H
