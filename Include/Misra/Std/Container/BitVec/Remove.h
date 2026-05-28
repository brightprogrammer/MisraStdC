/// file      : std/container/bitvec/remove.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Removal operations for bitvectors.

#ifndef MISRA_STD_CONTAINER_BITVEC_REMOVE_H
#define MISRA_STD_CONTAINER_BITVEC_REMOVE_H

#include "Type.h"
#include <Misra/Types.h>

#ifdef __cplusplus
extern "C" {
#endif

    ///
    /// Remove multiple consecutive bits starting at a specific position.
    /// All bits after the removed range are shifted left.
    ///
    /// bv[in,out] : Bitvector to remove from.
    /// idx[in]    : Starting position (0-based), in [0, length).
    /// count[in]  : Number of bits to remove; `idx + count` must not exceed
    ///              the length.
    ///
    /// SUCCESS : Returns to the caller. Bitvector length shrinks by
    ///           `count`; bits previously at indices >= `idx + count`
    ///           have shifted left by `count`. Capacity is unchanged.
    /// FAILURE : Function cannot fail. An out-of-range `idx + count` is
    ///           a caller bug and aborts via `LOG_FATAL`.
    ///
    /// USAGE:
    ///   BitVecRemoveRange(&flags, 2, 3);  // Remove 3 bits starting at position 2
    ///
    /// TAGS: Remove, BitVec, Range, Multiple
    ///
    void BitVecRemoveRange(BitVec *bv, u64 idx, u64 count);

    ///
    /// Remove the first occurrence of a specific bit value.
    ///
    /// bv[in,out] : Bitvector to remove from.
    /// value[in]  : Bit value to find and remove (true or false).
    ///
    /// SUCCESS : Returns `true`. The first bit equal to `value` has been
    ///           removed; bitvector length shrinks by one; subsequent
    ///           bits have shifted left by one.
    /// FAILURE : Returns `false` when no bit equal to `value` was found.
    ///           The bitvector is unchanged.
    ///
    /// USAGE:
    ///   bool found = BitVecRemoveFirst(&flags, true);
    ///
    /// TAGS: Remove, BitVec, First, Value
    ///
    bool BitVecRemoveFirst(BitVec *bv, bool value);

    ///
    /// Remove the last occurrence of a specific bit value.
    ///
    /// bv[in,out] : Bitvector to remove from.
    /// value[in]  : Bit value to find and remove (true or false).
    ///
    /// SUCCESS : Returns `true`. The last bit equal to `value` has been
    ///           removed; bitvector length shrinks by one; bits after
    ///           the removed position have shifted left by one (if any).
    /// FAILURE : Returns `false` when no bit equal to `value` was found.
    ///           The bitvector is unchanged.
    ///
    /// USAGE:
    ///   bool found = BitVecRemoveLast(&flags, false);
    ///
    /// TAGS: Remove, BitVec, Last, Value
    ///
    bool BitVecRemoveLast(BitVec *bv, bool value);

    ///
    /// Remove all occurrences of a specific bit value.
    ///
    /// bv[in,out] : Bitvector to remove from.
    /// value[in]  : Bit value to remove (true or false).
    ///
    /// SUCCESS : Returns the count of bits removed. Bitvector length
    ///           shrinks by the returned count; remaining bits are
    ///           compacted to the front in their original relative order.
    /// FAILURE : Returns `0` when no bit equal to `value` exists. The
    ///           bitvector is unchanged.
    ///
    /// USAGE:
    ///   u64 removed = BitVecRemoveAll(&flags, true);
    ///
    /// TAGS: Remove, BitVec, All, Value
    ///
    u64 BitVecRemoveAll(BitVec *bv, bool value);

    ///
    /// Pop the last bit from bitvector.
    ///
    /// bv[in,out] : Bitvector to pop bit from.
    ///
    /// SUCCESS : Returns the value of the removed bit (true/false).
    ///           Bitvector length shrinks by one; capacity is unchanged.
    /// FAILURE : Function cannot fail. Calling on an empty bitvector is
    ///           a caller bug and aborts via `LOG_FATAL`.
    ///
    /// USAGE:
    ///   bool last_bit = BitVecPop(&flags);
    ///
    /// TAGS: BitVec, Pop, Remove, Last
    ///
    bool BitVecPop(BitVec *bv);

    ///
    /// Remove a bit at given index from bitvector.
    /// Shifts all bits after the index to the left.
    ///
    /// bv[in,out] : Bitvector to remove bit from.
    /// idx[in]    : Index of bit to remove (0-based), in [0, length).
    ///
    /// SUCCESS : Returns the value of the removed bit (true/false).
    ///           Bitvector length shrinks by one; bits previously at
    ///           indices > `idx` have shifted left by one.
    /// FAILURE : Function cannot fail. An out-of-range `idx` is a caller
    ///           bug and aborts via `LOG_FATAL`.
    ///
    /// USAGE:
    ///   bool removed_bit = BitVecRemove(&flags, 5);
    ///
    /// TAGS: BitVec, Remove, Shift, Single
    ///
    bool BitVecRemove(BitVec *bv, u64 idx);

#ifdef __cplusplus
}
#endif

#endif // MISRA_STD_CONTAINER_BITVEC_REMOVE_H