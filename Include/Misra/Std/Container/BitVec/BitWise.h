/// file      : std/container/bitvec/bitwise.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
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
    /// Result is stored in `result`. Operand lengths must match.
    ///
    /// result[out] : Bitvector to store result in. Pre-sized to the
    ///               operand length by the helper if needed.
    /// a[in]       : First bitvector operand.
    /// b[in]       : Second bitvector operand.
    ///
    /// SUCCESS : Returns to the caller. `result->length == a->length`;
    ///           each bit `result[i] == a[i] & b[i]`. The operands `a`
    ///           and `b` are unchanged.
    /// FAILURE : Function cannot fail in an observable way - mismatched
    ///           operand lengths or invalid bitvectors are caller bugs
    ///           and abort via `LOG_FATAL`.
    ///
    /// USAGE:
    ///   BitVecAnd(&result, &flags1, &flags2);
    ///
    /// TAGS: BitVec, And, Bitwise, Operation
    ///
    void BitVecAnd(BitVec *result, BitVec *a, BitVec *b);

    ///
    /// Perform bitwise OR operation between two bitvectors.
    /// Result is stored in `result`. Operand lengths must match.
    ///
    /// result[out] : Bitvector to store result in.
    /// a[in]       : First bitvector operand.
    /// b[in]       : Second bitvector operand.
    ///
    /// SUCCESS : Returns to the caller. `result->length == a->length`;
    ///           each bit `result[i] == a[i] | b[i]`. The operands are
    ///           unchanged.
    /// FAILURE : Function cannot fail. Mismatched operand lengths or
    ///           invalid bitvectors are caller bugs and abort via
    ///           `LOG_FATAL`.
    ///
    /// USAGE:
    ///   BitVecOr(&result, &flags1, &flags2);
    ///
    /// TAGS: BitVec, Or, Bitwise, Operation
    ///
    void BitVecOr(BitVec *result, BitVec *a, BitVec *b);

    ///
    /// Perform bitwise XOR operation between two bitvectors.
    /// Result is stored in `result`. Operand lengths must match.
    ///
    /// result[out] : Bitvector to store result in.
    /// a[in]       : First bitvector operand.
    /// b[in]       : Second bitvector operand.
    ///
    /// SUCCESS : Returns to the caller. `result->length == a->length`;
    ///           each bit `result[i] == a[i] ^ b[i]`. The operands are
    ///           unchanged.
    /// FAILURE : Function cannot fail. Mismatched operand lengths or
    ///           invalid bitvectors are caller bugs and abort via
    ///           `LOG_FATAL`.
    ///
    /// USAGE:
    ///   BitVecXor(&result, &flags1, &flags2);
    ///
    /// TAGS: BitVec, Xor, Bitwise, Operation
    ///
    void BitVecXor(BitVec *result, BitVec *a, BitVec *b);

    ///
    /// Perform bitwise NOT operation on a bitvector.
    /// Result is stored in `result`.
    ///
    /// result[out] : Bitvector to store result in.
    /// bv[in]      : Bitvector operand.
    ///
    /// SUCCESS : Returns to the caller. `result->length == bv->length`;
    ///           each bit `result[i] == !bv[i]`. The operand `bv` is
    ///           unchanged.
    /// FAILURE : Function cannot fail. Invalid bitvectors are caller
    ///           bugs and abort via `LOG_FATAL`.
    ///
    /// USAGE:
    ///   BitVecNot(&result, &flags);
    ///
    /// TAGS: BitVec, Not, Bitwise, Operation
    ///
    void BitVecNot(BitVec *result, BitVec *bv);

    ///
    /// Shift all bits in bitvector to the left by specified positions.
    /// New bits on the right (low end) are filled with zeros.
    ///
    /// bv[in,out]    : Bitvector to shift in place.
    /// positions[in] : Number of positions to shift left.
    ///
    /// SUCCESS : Returns to the caller. Each previous bit at index `i`
    ///           is now at index `i + positions` (when in range); bits
    ///           at indices < `positions` are now `false`; bits that
    ///           shifted past `length - 1` are discarded. Length and
    ///           capacity are unchanged.
    /// FAILURE : Function cannot fail. An invalid bitvector is a caller
    ///           bug and aborts via `LOG_FATAL`.
    ///
    /// USAGE:
    ///   BitVecShiftLeft(&flags, 3);
    ///
    /// TAGS: BitVec, Shift, Left, Operation
    ///
    void BitVecShiftLeft(BitVec *bv, u64 positions);

    ///
    /// Shift all bits in bitvector to the right by specified positions.
    /// New bits on the left (high end) are filled with zeros.
    ///
    /// bv[in,out]    : Bitvector to shift in place.
    /// positions[in] : Number of positions to shift right.
    ///
    /// SUCCESS : Returns to the caller. Each previous bit at index `i`
    ///           is now at index `i - positions` (when non-negative);
    ///           bits at indices >= `length - positions` are now
    ///           `false`; bits that shifted past 0 are discarded. Length
    ///           and capacity are unchanged.
    /// FAILURE : Function cannot fail. An invalid bitvector is a caller
    ///           bug and aborts via `LOG_FATAL`.
    ///
    /// USAGE:
    ///   BitVecShiftRight(&flags, 2);
    ///
    /// TAGS: BitVec, Shift, Right, Operation
    ///
    void BitVecShiftRight(BitVec *bv, u64 positions);

    ///
    /// Rotate all bits in bitvector to the left by specified positions.
    /// Bits that fall off the high end wrap around to the low end.
    ///
    /// bv[in,out]    : Bitvector to rotate in place.
    /// positions[in] : Number of positions to rotate left. Effective
    ///                 rotation is `positions % length`.
    ///
    /// SUCCESS : Returns to the caller. Each previous bit at index `i`
    ///           is now at index `(i + positions) % length`. No bits
    ///           are lost; length and capacity are unchanged.
    /// FAILURE : Function cannot fail. An invalid bitvector is a caller
    ///           bug and aborts via `LOG_FATAL`.
    ///
    /// USAGE:
    ///   BitVecRotateLeft(&flags, 5);
    ///
    /// TAGS: BitVec, Rotate, Left, Circular
    ///
    void BitVecRotateLeft(BitVec *bv, u64 positions);

    ///
    /// Rotate all bits in bitvector to the right by specified positions.
    /// Bits that fall off the low end wrap around to the high end.
    ///
    /// bv[in,out]    : Bitvector to rotate in place.
    /// positions[in] : Number of positions to rotate right. Effective
    ///                 rotation is `positions % length`.
    ///
    /// SUCCESS : Returns to the caller. Each previous bit at index `i`
    ///           is now at index `(i + length - positions) % length`.
    ///           No bits are lost; length and capacity are unchanged.
    /// FAILURE : Function cannot fail. An invalid bitvector is a caller
    ///           bug and aborts via `LOG_FATAL`.
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
    /// bv[in,out] : Bitvector to reverse in place.
    ///
    /// SUCCESS : Returns to the caller. The bit at index `i` is now the
    ///           previous bit at index `length - 1 - i`. Length and
    ///           capacity are unchanged.
    /// FAILURE : Function cannot fail. An invalid bitvector is a caller
    ///           bug and aborts via `LOG_FATAL`.
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