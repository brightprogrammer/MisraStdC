/// file      : std/container/Bits/insert.h
/// author    : Generated following Misra project patterns
/// This is free and unencumbered software released into the public domain.
///
/// Insertion operations for Bitstors.

#ifndef MISRA_STD_CONTAINER_Bits_INSERT_H
#define MISRA_STD_CONTAINER_Bits_INSERT_H

#include "Type.h"
#include <Misra/Types.h>

#ifdef __cplusplus
extern "C" {
#endif

    ///
    /// Insert multiple bits of the same value at a specific position.
    /// All existing bits at and after the position are shifted right.
    ///
    /// bv[in]     : Bitstor to insert into
    /// idx[in]    : Position to insert at (0-based)
    /// count[in]  : Number of bits to insert
    /// value[in]  : Value for all inserted bits (true or false)
    ///
    /// USAGE:
    ///   BitsInsertRange(&flags, 2, 5, true);  // Insert 5 true bits at position 2
    ///
    /// TAGS: Insert, Bits, Range, Multiple
    ///
    void BitsInsertMultiple(Bits *bv, u64 idx, u64 count, bool value);

    ///
    /// Insert all bits from another Bitstor at a specific position.
    /// All existing bits at and after the position are shifted right.
    ///
    /// bv[in]     : Bitstor to insert into
    /// idx[in]    : Position to insert at (0-based)
    /// other[in]  : Bitstor whose bits to insert
    ///
    /// USAGE:
    ///   BitsInsertMultiple(&flags, 2, &other_flags);
    ///
    /// TAGS: Insert, Bits, Multiple, Copy
    ///
    void BitsConcatAt(Bits *bv, u64 idx, Bits *other);

    ///
    /// Insert a bit pattern from a byte at a specific position.
    /// Only the specified number of bits from the pattern are inserted.
    ///
    /// bv[in]           : Bitstor to insert into
    /// idx[in]          : Position to insert at (0-based)
    /// pattern[in]      : u64 containing the bit pattern
    /// pattern_bits[in] : Number of bits to take from pattern. Repeats pattern if `pattern_bits > 64`
    ///
    /// USAGE:
    ///   u8 pattern = 0xAB; // 10101011 in binary
    ///   BitsInsertPattern(&flags, 2, pattern, 4);  // Insert 1010 at position 2
    ///
    /// TAGS: Insert, Bits, Pattern, Byte
    ///
    void BitsInsertPattern(Bits *bv, u64 idx, u64 pattern, u64 pattern_bits);

    ///
    /// Push a bit to the end of Bitstor.
    /// Grows the Bitstor if necessary.
    ///
    /// bv[in]    : Bitstor to push bit to
    /// value[in] : Bit value to push (true/false)
    ///
    /// USAGE:
    ///   BitsPush(&flags, true);
    ///   BitsPush(&flags, false);
    ///
    /// TAGS: Bits, Push, Append, Insert
    ///
    void BitsPush(Bits *bv, bool value);

    ///
    /// Insert a bit at given index in Bitstor.
    /// Shifts all bits at and after the index to the right.
    ///
    /// bv[in]    : Bitstor to insert bit into
    /// idx[in]   : Index at which to insert bit (0-based)
    /// value[in] : Bit value to insert (true/false)
    ///
    /// USAGE:
    ///   BitsInsert(&flags, 5, true);
    ///
    /// TAGS: Bits, Insert, Shift, Single
    ///
    void BitsInsert(Bits *bv, u64 idx, bool value);

#ifdef __cplusplus
}
#endif

#endif // MISRA_STD_CONTAINER_Bits_INSERT_H

