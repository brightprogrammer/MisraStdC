/// file      : std/container/int/init.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Initialization helpers for Int.

#ifndef MISRA_STD_CONTAINER_INT_INIT_H
#define MISRA_STD_CONTAINER_INT_INIT_H

#include "Type.h"
#include <Misra/Std/Container/BitVec/Init.h>

///
/// Initialize an Int (numeric value 0). Inside a `Scope` block the
/// allocator argument may be omitted (`MisraScope` is used). Otherwise
/// pass a typed allocator handle or a raw `Allocator *`.
///
/// USAGE:
///   Scope(alloc, DefaultAllocator) {
///       Int value = IntInit();
///       ...
///   }
///
/// SUCCESS : Returns a numerically-zero `Int` whose backing bitvector is bound to the chosen
///           allocator.
/// FAILURE : Cannot fail at construction; first allocator OOM surfaces from later math/grow.
///
/// TAGS: Int, Init, Zero, Construct
///
#define IntInit(...)             OVERLOAD(IntInit, __VA_ARGS__)
#define IntInit_0()              ((Int) {.bits = BitVecInit_1(MisraScope)})
#define IntInit_1(allocator_ptr) ((Int) {.bits = BitVecInit_1(allocator_ptr)})

///
/// Release all storage owned by an integer.
/// The object must not be used again until reinitialized.
///
/// value[in,out] : Integer to deinitialize
///
/// SUCCESS : Underlying bitvector freed; `value` left in the zeroed post-deinit state.
/// FAILURE : Cannot fail; aborts on a corrupted magic via the validator.
///
/// TAGS: Int, Deinit, Memory
///
static inline void IntDeinit(Int *value) {
    ValidateInt(value);
    BitVecDeinit(&value->bits);
}

///
/// Reset an integer back to zero while preserving the object itself.
///
/// value[in,out] : Integer to clear
///
/// SUCCESS : Numeric value becomes 0; backing bitvector capacity is retained.
/// FAILURE : Cannot fail; aborts on a corrupted magic via the validator.
///
/// TAGS: Int, Clear, Zero, Reset
///
static inline void IntClear(Int *value) {
    ValidateInt(value);
    BitVecClear(&value->bits);
}

#endif // MISRA_STD_CONTAINER_INT_INIT_H
