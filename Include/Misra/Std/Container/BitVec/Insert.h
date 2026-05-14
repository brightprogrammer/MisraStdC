/// file      : std/container/bitvec/insert.h
/// author    : Generated following Misra project patterns
/// This is free and unencumbered software released into the public domain.
///
/// Insertion operations for bitvectors.

#ifndef MISRA_STD_CONTAINER_BITVEC_INSERT_H
#define MISRA_STD_CONTAINER_BITVEC_INSERT_H

#include "Type.h"
#include <Misra/Types.h>

///
/// Aborting (`Must*`) variants of the fallible bitvector insertion functions
/// declared below.
///
/// Each `BitVecMustXxx(...)` is a statement-style do-while wrapper around the
/// matching `BitVecXxx(...)` function: it calls the underlying fallible form
/// and triggers `LOG_FATAL(...)` if the call returns `false`. Use these at
/// API boundaries where allocation failure is not recoverable. Otherwise
/// prefer the propagating forms.
///
/// SUCCESS : Returns to the caller.
/// FAILURE : Does not return - aborts via `LOG_FATAL` / `SysAbort`.
///
/// TAGS: BitVec, Insert, Must, Abort
///
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
    /// bv[in,out] : Bitvector to insert into.
    /// idx[in]    : Position to insert at (0-based), in [0, length].
    /// count[in]  : Number of bits to insert.
    /// value[in]  : Value for all inserted bits (true or false).
    ///
    /// SUCCESS : Returns `true`. Bitvector length grows by `count`; the
    ///           range `[idx, idx + count)` now holds `count` copies of
    ///           `value`; previous bits at and after `idx` have shifted
    ///           right by `count`. The byte buffer may have grown.
    /// FAILURE : Returns `false` on allocation failure when capacity must
    ///           grow. The bitvector is unchanged.
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
    /// bv[in,out] : Bitvector to insert into.
    /// idx[in]    : Position to insert at (0-based), in [0, length].
    /// other[in]  : Bitvector whose bits to insert.
    ///
    /// SUCCESS : Returns `true`. Bitvector length grows by `other->length`;
    ///           the range `[idx, idx + other->length)` mirrors the bits
    ///           of `other` in order; previous bits at and after `idx`
    ///           have shifted right by `other->length`. `other` is
    ///           untouched.
    /// FAILURE : Returns `false` on allocation failure when capacity must
    ///           grow. The bitvector is unchanged.
    ///
    /// USAGE:
    ///   BitVecInsertMultiple(&flags, 2, &other_flags);
    ///
    /// TAGS: Insert, BitVec, Multiple, Copy
    ///
    bool BitVecInsertMultiple(BitVec *bv, u64 idx, BitVec *other);

    ///
    /// Insert a bit pattern from a byte at a specific position.
    /// Only the specified number of low-order bits from the pattern are inserted.
    ///
    /// bv[in,out]       : Bitvector to insert into.
    /// idx[in]          : Position to insert at (0-based), in [0, length].
    /// pattern[in]      : Byte containing the bit pattern.
    /// pattern_bits[in] : Number of low-order bits to take from `pattern`, 1-8.
    ///
    /// SUCCESS : Returns `true`. Bitvector length grows by `pattern_bits`;
    ///           the inserted bits at `[idx, idx + pattern_bits)` reflect
    ///           the low `pattern_bits` of `pattern` (LSB-first); previous
    ///           bits at and after `idx` have shifted right.
    /// FAILURE : Returns `false` on allocation failure. The bitvector is
    ///           unchanged.
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
    /// bv[in,out] : Bitvector to push bit to.
    /// value[in]  : Bit value to push (true/false).
    ///
    /// SUCCESS : Returns `true`. Bitvector length grows by one; bit at
    ///           index `old_length` is now `value`. The byte buffer may
    ///           have grown.
    /// FAILURE : Returns `false` on allocation failure when capacity must
    ///           grow. The bitvector is unchanged.
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
    /// bv[in,out] : Bitvector to insert bit into.
    /// idx[in]    : Index at which to insert bit (0-based), in [0, length].
    /// value[in]  : Bit value to insert (true/false).
    ///
    /// SUCCESS : Returns `true`. Bitvector length grows by one; bit at
    ///           index `idx` is now `value`; previous bits at and after
    ///           `idx` have shifted right by one.
    /// FAILURE : Returns `false` on allocation failure. The bitvector is
    ///           unchanged.
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
