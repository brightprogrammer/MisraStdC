/// file      : std/container/bitvec/insert.h
/// author    : Generated following Misra project patterns
/// This is free and unencumbered software released into the public domain.
///
/// Insertion operations for bitvectors.

#ifndef MISRA_STD_CONTAINER_BITVEC_INSERT_H
#define MISRA_STD_CONTAINER_BITVEC_INSERT_H

#include "Type.h"
#include <Misra/Types.h>

#define BitVecMustInsertRange(bv, idx, count, value)                                                                   \
    do {                                                                                                               \
        if (!BitVecInsertRange((bv), (idx), (count), (value))) {                                                       \
            LOG_FATAL("BitVecMustInsertRange failed");                                                                 \
        }                                                                                                              \
    } while (0)
#define BitVecMustInsertMultiple(bv, idx, other)                                                                       \
    do {                                                                                                               \
        if (!BitVecInsertMultiple((bv), (idx), (other))) {                                                             \
            LOG_FATAL("BitVecMustInsertMultiple failed");                                                              \
        }                                                                                                              \
    } while (0)
#define BitVecMustInsertPattern(bv, idx, pattern, pattern_bits)                                                        \
    do {                                                                                                               \
        if (!BitVecInsertPattern((bv), (idx), (pattern), (pattern_bits))) {                                            \
            LOG_FATAL("BitVecMustInsertPattern failed");                                                               \
        }                                                                                                              \
    } while (0)
#define BitVecMustPush(bv, value)                                                                                      \
    do {                                                                                                               \
        if (!BitVecPush((bv), (value))) {                                                                              \
            LOG_FATAL("BitVecMustPush failed");                                                                        \
        }                                                                                                              \
    } while (0)
#define BitVecMustInsert(bv, idx, value)                                                                               \
    do {                                                                                                               \
        if (!BitVecInsert((bv), (idx), (value))) {                                                                     \
            LOG_FATAL("BitVecMustInsert failed");                                                                      \
        }                                                                                                              \
    } while (0)

#ifdef __cplusplus
extern "C" {
#endif

    ///
    /// Insert multiple bits of the same value at a specific position.
    /// All existing bits at and after the position are shifted right.
    ///
    /// bv[in]     : Bitvector to insert into
    /// idx[in]    : Position to insert at (0-based)
    /// count[in]  : Number of bits to insert
    /// value[in]  : Value for all inserted bits (true or false)
    ///
    /// USAGE:
    ///   BitVecInsertRange(&flags, 2, 5, true);  // Insert 5 true bits at position 2
    ///
    /// TAGS: Insert, BitVec, Range, Multiple
    ///
    bool BitVecInsertRange(BitVec *bv, u64 idx, u64 count, bool value);

    ///
    /// Insert all bits from another bitvector at a specific position.
    /// All existing bits at and after the position are shifted right.
    ///
    /// bv[in]     : Bitvector to insert into
    /// idx[in]    : Position to insert at (0-based)
    /// other[in]  : Bitvector whose bits to insert
    ///
    /// USAGE:
    ///   BitVecInsertMultiple(&flags, 2, &other_flags);
    ///
    /// TAGS: Insert, BitVec, Multiple, Copy
    ///
    bool BitVecInsertMultiple(BitVec *bv, u64 idx, BitVec *other);

    ///
    /// Insert a bit pattern from a byte at a specific position.
    /// Only the specified number of bits from the pattern are inserted.
    ///
    /// bv[in]           : Bitvector to insert into
    /// idx[in]          : Position to insert at (0-based)
    /// pattern[in]      : Byte containing the bit pattern
    /// pattern_bits[in] : Number of bits to take from pattern (1-8)
    ///
    /// USAGE:
    ///   u8 pattern = 0x0B; // 1011 in binary
    ///   BitVecInsertPattern(&flags, 2, pattern, 4);  // Insert 1011 at position 2
    ///
    /// TAGS: Insert, BitVec, Pattern, Byte
    ///
    bool BitVecInsertPattern(BitVec *bv, u64 idx, u8 pattern, u64 pattern_bits);

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
    /// TAGS: BitVec, Push, Append, Insert
    ///
    bool BitVecPush(BitVec *bv, bool value);

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
    /// TAGS: BitVec, Insert, Shift, Single
    ///
    bool BitVecInsert(BitVec *bv, u64 idx, bool value);

#ifdef __cplusplus
}
#endif

#endif // MISRA_STD_CONTAINER_BITVEC_INSERT_H
