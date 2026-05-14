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
///
/// Create an empty integer with value `0`.
///
/// `IntInit(&heap)` where `heap` is a typed allocator like `HeapAllocator`.
///
#define IntInit(typed_alloc_ptr) ((Int) {.bits = BitVecInit(typed_alloc_ptr)})

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
