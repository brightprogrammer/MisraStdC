/// file      : std/container/bitvec/ops.h
/// author    : Generated following Misra project patterns
/// This is free and unencumbered software released into the public domain.
///
/// Bit vector operations - bitwise operations, push/pop, insert/remove

#ifndef MISRA_STD_CONTAINER_BITVEC_OPS_H
#define MISRA_STD_CONTAINER_BITVEC_OPS_H

#include "Type.h"

#ifdef __cplusplus
extern "C" {
#endif

    ///
    /// Push a bit to the end of bitvector.
    /// Grows the bitvector if necessary.
    ///
    /// bv[in]    : Bitvector to push bit to
    /// value[in] : Bit value to push (true/false)
    ///
    /// USAGE:
    ///   BitVecPush(&flags, true);
    ///   BitVecPush(&flags, false);
    ///
    /// TAGS: BitVec, Push, Append
    ///
    void BitVecPush(BitVec *bv, bool value);

    ///
    /// Pop a bit from the end of bitvector.
    /// Returns the value of the bit that was removed.
    ///
    /// bv[in] : Bitvector to pop bit from
    ///
    /// RETURNS : Value of the bit that was popped
    ///
    /// USAGE:
    ///   bool last_bit = BitVecPop(&flags);
    ///
    /// TAGS: BitVec, Pop, Remove
    ///
    bool BitVecPop(BitVec *bv);

    ///
    /// Insert a bit at given index in bitvector.
    /// Shifts all bits at and after the index to the right.
    ///
    /// bv[in]    : Bitvector to insert bit into
    /// idx[in]   : Index at which to insert bit (0-based)
    /// value[in] : Bit value to insert (true/false)
    ///
    /// USAGE:
    ///   BitVecInsert(&flags, 5, true);
    ///
    /// TAGS: BitVec, Insert, Shift
    ///
    void BitVecInsert(BitVec *bv, size idx, bool value);

    ///
    /// Remove a bit at given index from bitvector.
    /// Shifts all bits after the index to the left.
    ///
    /// bv[in]  : Bitvector to remove bit from
    /// idx[in] : Index of bit to remove (0-based)
    ///
    /// USAGE:
    ///   BitVecRemove(&flags, 5);
    ///
    /// TAGS: BitVec, Remove, Shift
    ///
    void BitVecRemove(BitVec *bv, size idx);

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

#ifdef __cplusplus
}
#endif

#endif // MISRA_STD_CONTAINER_BITVEC_OPS_H