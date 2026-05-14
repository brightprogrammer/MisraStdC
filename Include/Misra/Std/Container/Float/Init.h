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
/// Initialize a Float (numeric value 0) bound to an allocator. Argument
/// may be a typed allocator pointer (`&heap`) or raw `Allocator *` —
/// the underlying `IntInit` / `BitVecInit` macros route through
/// `ALLOCATOR_OF`.
///
/// USAGE:
///   DefaultAllocator a = DefaultAllocatorInit();
///   Float value = FloatInit(&a);
///
/// TAGS: Float, Init, Zero, Construct
///
#define FloatInit(allocator_ptr)                                                                                       \
    ((Float) {.negative = false, .significand = IntInit(allocator_ptr), .exponent = 0})

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
