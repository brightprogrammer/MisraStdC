/// file      : std/container/Bits/remove.h
/// author    : Generated following Misra project patterns
/// This is free and unencumbered software released into the public domain.
///
/// Removal operations for Bitstors.

#ifndef MISRA_STD_CONTAINER_Bits_REMOVE_H
#define MISRA_STD_CONTAINER_Bits_REMOVE_H

#include "Type.h"
#include <Misra/Types.h>

#ifdef __cplusplus
extern "C" {
#endif

    ///
    /// Remove multiple consecutive bits starting at a specific position.
    /// All bits after the removed range are shifted left.
    ///
    /// bv[in]     : Bitstor to remove from
    /// idx[in]    : Starting position (0-based)
    /// count[in]  : Number of bits to remove
    ///
    /// USAGE:
    ///   BitsRemoveRange(&flags, 2, 3);  // Remove 3 bits starting at position 2
    ///
    /// TAGS: Remove, Bits, Range, Multiple
    ///
    void BitsRemoveRange(Bits *bv, u64 idx, u64 count);

    ///
    /// Remove the first occurrence of a specific bit value.
    /// Returns true if a bit was found and removed, false otherwise.
    ///
    /// bv[in]     : Bitstor to remove from
    /// value[in]  : Bit value to find and remove (true or false)
    ///
    /// RETURNS: true if bit was found and removed, false if not found
    ///
    /// USAGE:
    ///   bool found = BitsRemoveFirst(&flags, true);
    ///
    /// TAGS: Remove, Bits, First, Value
    ///
    bool BitsRemoveFirst(Bits *bv, bool value);

    ///
    /// Remove the last occurrence of a specific bit value.
    /// Returns true if a bit was found and removed, false otherwise.
    ///
    /// bv[in]     : Bitstor to remove from
    /// value[in]  : Bit value to find and remove (true or false)
    ///
    /// RETURNS: true if bit was found and removed, false if not found
    ///
    /// USAGE:
    ///   bool found = BitsRemoveLast(&flags, false);
    ///
    /// TAGS: Remove, Bits, Last, Value
    ///
    bool BitsRemoveLast(Bits *bv, bool value);

    ///
    /// Remove all occurrences of a specific bit value.
    /// Returns the number of bits that were removed.
    ///
    /// bv[in]     : Bitstor to remove from
    /// value[in]  : Bit value to remove (true or false)
    ///
    /// RETURNS: Number of bits removed
    ///
    /// USAGE:
    ///   u64 removed = BitsRemoveAll(&flags, true);
    ///
    /// TAGS: Remove, Bits, All, Value
    ///
    u64 BitsRemoveAll(Bits *bv, bool value);

    ///
    /// Pop the last bit from Bitstor.
    /// Returns the value of the removed bit.
    ///
    /// bv[in] : Bitstor to pop bit from
    ///
    /// RETURNS: Value of the popped bit (true/false)
    ///
    /// USAGE:
    ///   bool last_bit = BitsPop(&flags);
    ///
    /// TAGS: Bits, Pop, Remove, Last
    ///
    bool BitsPop(Bits *bv);

    ///
    /// Remove a bit at given index from Bitstor.
    /// Shifts all bits after the index to the left.
    ///
    /// bv[in]  : Bitstor to remove bit from
    /// idx[in] : Index of bit to remove (0-based)
    ///
    /// RETURNS: Value of the removed bit (true/false)
    ///
    /// USAGE:
    ///   bool removed_bit = BitsRemove(&flags, 5);
    ///
    /// TAGS: Bits, Remove, Shift, Single
    ///
    bool BitsRemove(Bits *bv, u64 idx);

#ifdef __cplusplus
}
#endif

#endif // MISRA_STD_CONTAINER_Bits_REMOVE_H
