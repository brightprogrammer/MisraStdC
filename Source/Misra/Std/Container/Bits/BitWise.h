/// file      : std/container/Bits/bitwise.h
/// author    : Generated following Misra project patterns
/// This is free and unencumbered software released into the public domain.
///
/// Bit vector bitwise operations - AND, OR, XOR, NOT, shifts, rotations, reverse

#ifndef MISRA_STD_CONTAINER_Bits_BITWISE_H
#define MISRA_STD_CONTAINER_Bits_BITWISE_H

#include "Type.h"

#ifdef __cplusplus
extern "C" {
#endif

    ///
    /// Perform bitwise AND operation between two Bitstors.
    /// Result is stored in the first Bitstor.
    ///
    /// a[in]       : First Bitstor operand
    /// b[in]       : Second Bitstor operand
    ///
    /// USAGE:
    ///   Bits res = BitsAnd(&flags1, &flags2);
    ///
    /// TAGS: Bits, And, Bitwise, Operation
    ///
    Bits BitsAnd(Bits *a, Bits *b);

    ///
    /// Perform bitwise OR operation between two Bitstors.
    /// Result is stored in the first Bitstor.
    ///
    /// a[in]       : First Bitstor operand
    /// b[in]       : Second Bitstor operand
    ///
    /// USAGE:
    ///   Bits res = BitsOr(&flags1, &flags2);
    ///
    /// TAGS: Bits, Or, Bitwise, Operation
    ///
    Bits BitsOr(Bits *a, Bits *b);

    ///
    /// Perform bitwise XOR operation between two Bitstors.
    /// Result is stored in the first Bitstor.
    ///
    /// a[in]       : First Bitstor operand
    /// b[in]       : Second Bitstor operand
    ///
    /// USAGE:
    ///   Bits res = BitsXor(&flags1, &flags2);
    ///
    /// TAGS: Bits, Xor, Bitwise, Operation
    ///
    Bits BitsXor(Bits *a, Bits *b);

    ///
    /// Perform bitwise NOT operation on a Bitstor.
    /// Result is stored in the first Bitstor.
    ///
    /// bv[in]      : Bitstor operand
    ///
    /// USAGE:
    ///   Bits res = BitsNot(&flags);
    ///
    /// TAGS: Bits, Not, Bitwise, Operation
    ///
    Bits BitsNot(Bits *bv);

    ///
    /// Shift all bits in Bitstor to the left by specified positions.
    /// New bits on the right are filled with zeros.
    ///
    /// bv[in]        : Bitstor to shift
    /// positions[in] : Number of positions to shift left
    ///
    /// USAGE:
    ///   Bits res = BitsShiftLeft(&flags, 3);
    ///
    /// TAGS: Bits, Shift, Left, Operation
    ///
    Bits BitsShiftLeft(Bits *bv, u64 positions);

    ///
    /// Shift all bits in Bitstor to the right by specified positions.
    /// New bits on the left are filled with zeros.
    ///
    /// bv[in]        : Bitstor to shift
    /// positions[in] : Number of positions to shift right
    ///
    /// USAGE:
    ///   Bits res = BitsShiftRight(&flags, 2);
    ///
    /// TAGS: Bits, Shift, Right, Operation
    ///
    Bits BitsShiftRight(Bits *bv, u64 positions);

    ///
    /// Rotate all bits in Bitstor to the left by specified positions.
    /// Bits that fall off the left end wrap around to the right.
    ///
    /// bv[in]        : Bitstor to rotate
    /// positions[in] : Number of positions to rotate left
    ///
    /// USAGE:
    ///   Bits res = BitsRotateLeft(&flags, 5);
    ///
    /// TAGS: Bits, Rotate, Left, Circular
    ///
    Bits BitsRotateLeft(Bits *bv, u64 positions);

    ///
    /// Rotate all bits in Bitstor to the right by specified positions.
    /// Bits that fall off the right end wrap around to the left.
    ///
    /// bv[in]        : Bitstor to rotate
    /// positions[in] : Number of positions to rotate right
    ///
    /// USAGE:
    ///   Bits res = BitsRotateRight(&flags, 3);
    ///
    /// TAGS: Bits, Rotate, Right, Circular
    ///
    Bits BitsRotateRight(Bits *bv, u64 positions);

    ///
    /// Reverse the order of all bits in Bitstor.
    /// First bit becomes last, last becomes first, etc.
    ///
    /// bv[in] : Bitstor to reverse
    ///
    /// USAGE:
    ///   Bits res = BitsReverse(&flags);
    ///
    /// TAGS: Bits, Reverse, Order
    ///
    Bits BitsReverse(Bits *bv);

#ifdef __cplusplus
}
#endif

#endif // MISRA_STD_CONTAINER_Bits_BITWISE_H

