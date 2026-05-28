/// file      : std/container/int/type.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Int stores non-negative arbitrary-precision integers using BitVec storage.

#ifndef MISRA_STD_CONTAINER_INT_TYPE_H
#define MISRA_STD_CONTAINER_INT_TYPE_H

#include <Misra/Std/Container/BitVec/Type.h>

///
/// Arbitrary-precision unsigned integer value.
/// `Int` stores the magnitude as a little-endian bit-vector with no sign bit.
///
/// USAGE:
///   Int value = IntFrom(42);
///
/// FIELDS:
/// - bits : Backing bit storage for the integer magnitude. Treat as internal representation.
///
/// TAGS: Int, Integer, Unsigned, Type, ArbitraryPrecision
///
typedef struct {
    BitVec bits;
} Int;

///
/// Validate whether a given `Int` object is structurally valid.
/// Aborts if the pointer is NULL or the embedded `BitVec` is invalid.
///
/// value[in] : Pointer to the `Int` object to validate
///
/// SUCCESS: Returns to caller when `value` appears valid.
/// FAILURE: Aborts the process when `value` is invalid.
///
/// TAGS: Int, Validate, Safety, Debug
///
static inline void ValidateInt(const Int *value) {
    ValidateBitVec(value ? &value->bits : NULL);
}

#endif // MISRA_STD_CONTAINER_INT_TYPE_H
