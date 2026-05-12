/// file      : std/container/int/init.h
/// author    : Generated following Misra project patterns
/// This is free and unencumbered software released into the public domain.
///
/// Initialization helpers for Int.

#ifndef MISRA_STD_CONTAINER_INT_INIT_H
#define MISRA_STD_CONTAINER_INT_INIT_H

#include "Type.h"
#include <Misra/Std/Container/BitVec/Init.h>

///
/// Create an empty integer with value `0`.
///
/// USAGE:
///   Int value = IntInit();
///
/// TAGS: Int, Init, Zero, Construct
///
static inline Int IntInitWithAllocator(Allocator alloc) {
    Int value;

    value.bits = BitVecInit(alloc);
    return value;
}

#define INT_INIT_HAS_ARGS_IMPL(_0, _1, count, ...) count
#define INT_INIT_HAS_ARGS(...) INT_INIT_HAS_ARGS_IMPL(__VA_OPT__(,) __VA_ARGS__, 1, 0, 0)
#define IntInit(...) CONCAT(IntInit_, INT_INIT_HAS_ARGS(__VA_ARGS__))(__VA_ARGS__)
#define IntInit_0() IntInitWithAllocator(DefaultAllocator())
#define IntInit_1(alloc) IntInitWithAllocator((alloc))

///
/// Release all storage owned by an integer.
/// The object must not be used again until reinitialized.
///
/// value[in] : Integer to deinitialize
///
/// USAGE:
///   IntDeinit(&value);
///
/// TAGS: Int, Deinit, Destroy, Memory
///
static inline void IntDeinit(Int *value) {
    ValidateInt(value);
    BitVecDeinit(&value->bits);
}

///
/// Reset an integer back to zero while preserving the object itself.
///
/// value[in] : Integer to clear
///
/// USAGE:
///   IntClear(&value);
///
/// TAGS: Int, Clear, Zero, Reset
///
static inline void IntClear(Int *value) {
    ValidateInt(value);
    BitVecClear(&value->bits);
}

#endif // MISRA_STD_CONTAINER_INT_INIT_H
