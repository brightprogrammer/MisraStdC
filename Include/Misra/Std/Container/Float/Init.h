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
/// Create a zero-valued floating-point number.
///
/// USAGE:
///   Float value = FloatInit();
///
/// TAGS: Float, Init, Zero, Construct
///
static inline Float FloatInitAlloc(Allocator alloc) {
    Float value;

    value.negative    = false;
    value.significand = IntInit(alloc);
    value.exponent    = 0;
    return value;
}

#define FLOAT_INIT_HAS_ARGS_IMPL(_0, _1, count, ...) count
#define FLOAT_INIT_HAS_ARGS(...) FLOAT_INIT_HAS_ARGS_IMPL(__VA_OPT__(,) __VA_ARGS__, 1, 0, 0)
#define FloatInit(...) CONCAT(FloatInit_, FLOAT_INIT_HAS_ARGS(__VA_ARGS__))(__VA_ARGS__)
#define FloatInit_0() FloatInitAlloc(DefaultAllocator())
#define FloatInit_1(alloc) FloatInitAlloc((alloc))

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
