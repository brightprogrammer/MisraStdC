/// file      : std/container/float/init.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Initialization helpers for Float.

#ifndef MISRA_STD_CONTAINER_FLOAT_INIT_H
#define MISRA_STD_CONTAINER_FLOAT_INIT_H

#include "Type.h"
#include <Misra/Std/Container/Int/Init.h>

///
/// Initialize a Float (numeric value 0). Inside a `Scope` block the
/// allocator argument may be omitted (`MisraScope` is used). Otherwise
/// pass a typed allocator handle or a raw `Allocator *`.
///
/// USAGE:
///   Scope(alloc, DefaultAllocator) {
///       Float value = FloatInit();
///       ...
///   }
///
/// SUCCESS : Returns a numerically-zero `Float` (sign positive, significand zero, exponent 0)
///           bound to the chosen allocator.
/// FAILURE : Cannot fail at construction; first allocator OOM surfaces from later math/grow.
///
/// TAGS: Float, Init, Zero, Construct
///
#define FloatInit(...)             OVERLOAD(FloatInit, __VA_ARGS__)
#define FloatInit_0()              ((Float) {.negative = false, .significand = IntInit_1(MisraScope), .exponent = 0})
#define FloatInit_1(allocator_ptr) ((Float) {.negative = false, .significand = IntInit_1(allocator_ptr), .exponent = 0})

///
/// Release all storage owned by a floating-point value.
///
/// value[in,out] : Float to deinitialize
///
/// SUCCESS : Significand storage released; `value` left in the zeroed post-deinit state.
/// FAILURE : Cannot fail; aborts on a corrupted magic via the validator.
///
/// TAGS: Float, Deinit, Memory
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
/// value[in,out] : Float to clear
///
/// SUCCESS : Numeric value becomes 0 (positive sign, zero significand, zero exponent);
///           significand capacity is retained.
/// FAILURE : Cannot fail; aborts on a corrupted magic via the validator.
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
