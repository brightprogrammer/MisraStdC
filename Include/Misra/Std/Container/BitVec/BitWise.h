/// file      : std/container/bitvec/bitwise.h
/// author    : Generated following Misra project patterns
/// This is free and unencumbered software released into the public domain.
///
/// Bit vector bitwise operations - AND, OR, XOR, NOT, shifts, rotations, reverse

#ifndef MISRA_STD_CONTAINER_BITVEC_BITWISE_H
#define MISRA_STD_CONTAINER_BITVEC_BITWISE_H

#include "Type.h"

#ifdef __cplusplus
extern "C" {
#endif

    ///
    /// Perform bitwise AND operation between two bitvectors.
    /// Result is stored in the first bitvector.
    ///
    /// result[out] : Bitvector to store result in
    /// a[in]       : First bitvector operand
    /// b[in]       : Second bitvector operand
    ///
    /// USAGE:
    ///   BitVecAnd(&result, &flags1, &flags2);
    ///
    /// TAGS: BitVec, And, Bitwise, Operation
    ///
    void BitVecAnd(BitVec *result, BitVec *a, BitVec *b);

    ///
    /// Perform bitwise OR operation between two bitvectors.
    /// Result is stored in the first bitvector.
    ///
    /// result[out] : Bitvector to store result in
    /// a[in]       : First bitvector operand
    /// b[in]       : Second bitvector operand
    ///
    /// USAGE:
    ///   BitVecOr(&result, &flags1, &flags2);
    ///
    /// TAGS: BitVec, Or, Bitwise, Operation
    ///
    void BitVecOr(BitVec *result, BitVec *a, BitVec *b);

    ///
    /// Perform bitwise XOR operation between two bitvectors.
    /// Result is stored in the first bitvector.
    ///
    /// result[out] : Bitvector to store result in
    /// a[in]       : First bitvector operand
    /// b[in]       : Second bitvector operand
    ///
    /// USAGE:
    ///   BitVecXor(&result, &flags1, &flags2);
    ///
    /// TAGS: BitVec, Xor, Bitwise, Operation
    ///
    void BitVecXor(BitVec *result, BitVec *a, BitVec *b);

    ///
    /// Perform bitwise NOT operation on a bitvector.
    /// Result is stored in the first bitvector.
    ///
    /// result[out] : Bitvector to store result in
    /// bv[in]      : Bitvector operand
    ///
    /// USAGE:
    ///   BitVecNot(&result, &flags);
    ///
    /// TAGS: BitVec, Not, Bitwise, Operation
    ///
    void BitVecNot(BitVec *result, BitVec *bv);

    ///
    /// Shift all bits in bitvector to the left by specified positions.
    /// New bits on the right are filled with zeros.
    ///
    /// bv[in]        : Bitvector to shift
    /// positions[in] : Number of positions to shift left
    ///
    /// USAGE:
    ///   BitVecShiftLeft(&flags, 3);
    ///
    /// TAGS: BitVec, Shift, Left, Operation
    ///
    void BitVecShiftLeft(BitVec *bv, u64 positions);

    ///
    /// Shift all bits in bitvector to the right by specified positions.
    /// New bits on the left are filled with zeros.
    ///
    /// bv[in]        : Bitvector to shift
    /// positions[in] : Number of positions to shift right
    ///
    /// USAGE:
    ///   BitVecShiftRight(&flags, 2);
    ///
    /// TAGS: BitVec, Shift, Right, Operation
    ///
    void BitVecShiftRight(BitVec *bv, u64 positions);

    ///
    /// Rotate all bits in bitvector to the left by specified positions.
    /// Bits that fall off the left end wrap around to the right.
    ///
    /// bv[in]        : Bitvector to rotate
    /// positions[in] : Number of positions to rotate left
    ///
    /// USAGE:
    ///   BitVecRotateLeft(&flags, 5);
    ///
    /// TAGS: BitVec, Rotate, Left, Circular
    ///
    void BitVecRotateLeft(BitVec *bv, u64 positions);

    ///
    /// Rotate all bits in bitvector to the right by specified positions.
    /// Bits that fall off the right end wrap around to the left.
    ///
    /// bv[in]        : Bitvector to rotate
    /// positions[in] : Number of positions to rotate right
    ///
    /// USAGE:
    ///   BitVecRotateRight(&flags, 3);
    ///
    /// TAGS: BitVec, Rotate, Right, Circular
    ///
    void BitVecRotateRight(BitVec *bv, u64 positions);

    ///
    /// Reverse the order of all bits in bitvector.
    /// First bit becomes last, last becomes first, etc.
    ///
    /// bv[in] : Bitvector to reverse
    ///
    /// USAGE:
    ///   BitVecReverse(&flags);
    ///
    /// TAGS: BitVec, Reverse, Order
    ///
    void BitVecReverse(BitVec *bv);

#ifdef __cplusplus
}
#endif

#endif // MISRA_STD_CONTAINER_BITVEC_BITWISE_H