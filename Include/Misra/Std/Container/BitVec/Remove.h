/// file      : std/container/bitvec/remove.h
/// author    : Generated following Misra project patterns
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
    /// bv[in]     : Bitvector to remove from
    /// idx[in]    : Starting position (0-based)
    /// count[in]  : Number of bits to remove
    ///
    /// USAGE:
    ///   BitVecRemoveRange(&flags, 2, 3);  // Remove 3 bits starting at position 2
    ///
    /// TAGS: Remove, BitVec, Range, Multiple
    ///
    void BitVecRemoveRange(BitVec *bv, u64 idx, u64 count);

    ///
    /// Remove the first occurrence of a specific bit value.
    /// Returns true if a bit was found and removed, false otherwise.
    ///
    /// bv[in]     : Bitvector to remove from
    /// value[in]  : Bit value to find and remove (true or false)
    ///
    /// RETURNS: true if bit was found and removed, false if not found
    ///
    /// USAGE:
    ///   bool found = BitVecRemoveFirst(&flags, true);
    ///
    /// TAGS: Remove, BitVec, First, Value
    ///
    bool BitVecRemoveFirst(BitVec *bv, bool value);

    ///
    /// Remove the last occurrence of a specific bit value.
    /// Returns true if a bit was found and removed, false otherwise.
    ///
    /// bv[in]     : Bitvector to remove from
    /// value[in]  : Bit value to find and remove (true or false)
    ///
    /// RETURNS: true if bit was found and removed, false if not found
    ///
    /// USAGE:
    ///   bool found = BitVecRemoveLast(&flags, false);
    ///
    /// TAGS: Remove, BitVec, Last, Value
    ///
    bool BitVecRemoveLast(BitVec *bv, bool value);

    ///
    /// Remove all occurrences of a specific bit value.
    /// Returns the number of bits that were removed.
    ///
    /// bv[in]     : Bitvector to remove from
    /// value[in]  : Bit value to remove (true or false)
    ///
    /// RETURNS: Number of bits removed
    ///
    /// USAGE:
    ///   u64 removed = BitVecRemoveAll(&flags, true);
    ///
    /// TAGS: Remove, BitVec, All, Value
    ///
    u64 BitVecRemoveAll(BitVec *bv, bool value);

    ///
    /// Pop the last bit from bitvector.
    /// Returns the value of the removed bit.
    ///
    /// bv[in] : Bitvector to pop bit from
    ///
    /// RETURNS: Value of the popped bit (true/false)
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
    /// bv[in]  : Bitvector to remove bit from
    /// idx[in] : Index of bit to remove (0-based)
    ///
    /// RETURNS: Value of the removed bit (true/false)
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