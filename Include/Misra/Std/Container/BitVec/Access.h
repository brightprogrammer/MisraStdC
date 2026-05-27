/// file      : std/container/bitvec/access.h
/// author    : Generated following Misra project patterns
/// This is free and unencumbered software released into the public domain.
///
/// Bit vector access operations

#ifndef MISRA_STD_CONTAINER_BITVEC_ACCESS_H
#define MISRA_STD_CONTAINER_BITVEC_ACCESS_H

#include "Type.h"

#ifdef __cplusplus
extern "C" {
#endif

    ///
    /// Get the value of bit at given index in bitvector.
    ///
    /// bv[in]  : Bitvector to get bit from.
    /// idx[in] : Index of bit to get (0-based), in [0, length).
    ///
    /// SUCCESS : Returns `true` when the bit is set, `false` when it is
    ///           clear. The bitvector is not modified.
    /// FAILURE : Function cannot fail. An out-of-range `idx` is a caller
    ///           bug and aborts via `LOG_FATAL`.
    ///
    /// USAGE:
    ///   bool flag = BitVecGet(&flags, 5);
    ///
    /// TAGS: BitVec, Access, Get, Boolean
    ///
    bool BitVecGet(BitVec *bv, u64 idx);

    ///
    /// Set the value of bit at given index in bitvector.
    ///
    /// bv[in,out] : Bitvector to set bit in.
    /// idx[in]    : Index of bit to set (0-based), in [0, length).
    /// value[in]  : Value to set (true/false).
    ///
    /// SUCCESS : Returns to the caller. The bit at `idx` is now `value`.
    ///           Length and capacity are unchanged; all other bits are
    ///           unchanged.
    /// FAILURE : Function cannot fail. An out-of-range `idx` is a caller
    ///           bug and aborts via `LOG_FATAL`.
    ///
    /// USAGE:
    ///   BitVecSet(&flags, 5, true);
    ///   BitVecSet(&flags, 10, false);
    ///
    /// TAGS: BitVec, Access, Set, Boolean
    ///
    void BitVecSet(BitVec *bv, u64 idx, bool value);

    ///
    /// Flip the value of bit at given index in bitvector.
    /// Changes 0 to 1 and 1 to 0.
    ///
    /// bv[in,out] : Bitvector to flip bit in.
    /// idx[in]    : Index of bit to flip (0-based), in [0, length).
    ///
    /// SUCCESS : Returns to the caller. The bit at `idx` is now the
    ///           logical inverse of its previous value. Length, capacity,
    ///           and all other bits are unchanged.
    /// FAILURE : Function cannot fail. An out-of-range `idx` is a caller
    ///           bug and aborts via `LOG_FATAL`.
    ///
    /// USAGE:
    ///   BitVecFlip(&flags, 5);
    ///
    /// TAGS: BitVec, Access, Flip, Toggle
    ///
    void BitVecFlip(BitVec *bv, u64 idx);

///
/// Get number of bits currently in bitvector.
///
/// bv[in] : Bitvector to get length of
///
/// RETURNS : Number of bits in bitvector
///
/// USAGE:
///   u64 num_bits = BitVecLen(&flags);
///
/// TAGS: BitVec, Length, Size
///
#define BitVecLen(bv) ((void)0, (bv)->length)

///
/// Get capacity of bitvector in bits.
///
/// bv[in] : Bitvector to get capacity of
///
/// RETURNS : Maximum number of bits bitvector can hold without reallocation
///
/// USAGE:
///   u64 max_bits = BitVecCapacity(&flags);
///
/// TAGS: BitVec, Capacity, Size
///
#define BitVecCapacity(bv) ((void)0, (bv)->capacity)

///
/// Pointer to the raw u64-packed storage backing the bitvector. The
/// caller MUST NOT free this pointer or write past `BitVecByteSize(bv)`
/// bytes. Useful for serialising the underlying bits or for hand-rolled
/// bit-level routines outside the standard `BitVecGet`/`Set`/`Flip`
/// access pattern.
///
/// bv[in] : Bitvector to query.
///
/// RETURNS : Pointer to the backing u64 array, or NULL when the
///           bitvector has never been allocated into.
///
/// TAGS: BitVec, Access, Data
///
#define BitVecData(bv) ((void)0, (bv)->data)

///
/// Check whether bitvector is empty.
///
/// bv[in] : Bitvector to query.
///
/// SUCCESS : Returns `true` when bitvector length is 0.
/// FAILURE : Returns `false` when the bitvector contains at least one
///           bit. The bitvector is not modified.
///
/// USAGE:
///   if (BitVecEmpty(&flags)) { /* handle empty case */ }
///
/// TAGS: BitVec, Empty, Check
///
#define BitVecEmpty(bv) (BitVecLen(bv) == 0)

///
/// Get u64 of bitvector in bytes.
/// This returns the actual memory used by the bit data.
///
/// bv[in] : Bitvector to get byte u64 of
///
/// RETURNS : Number of bytes used to store bits
///
/// USAGE:
///   u64 bytes_used = BitVecByteSize(&flags);
///
/// TAGS: BitVec, Size, Bytes, Memory
///
#define BitVecByteSize(bv) ((void)0, (bv)->byte_size)

    ///
    /// Count number of bits set to 1 in bitvector.
    ///
    /// bv[in] : Bitvector to count ones in
    ///
    /// SUCCESS : Returns the number of bits set to `1`. The bitvector is
    ///           not modified. Returns `0` for an empty bitvector.
    /// FAILURE : Function cannot fail.
    ///
    /// USAGE:
    ///   u64 ones = BitVecCountOnes(&flags);
    ///
    /// TAGS: BitVec, Count, Ones, Population
    ///
    u64 BitVecCountOnes(BitVec *bv);

    ///
    /// Count number of bits set to 0 in bitvector.
    ///
    /// bv[in] : Bitvector to count zeros in
    ///
    /// SUCCESS : Returns the number of bits set to `0`. The bitvector is
    ///           not modified. Returns `0` for an empty bitvector.
    /// FAILURE : Function cannot fail.
    ///
    /// USAGE:
    ///   u64 zeros = BitVecCountZeros(&flags);
    ///
    /// TAGS: BitVec, Count, Zeros
    ///
    u64 BitVecCountZeros(BitVec *bv);

    ///
    /// Find index of first occurrence of a specific bit value.
    ///
    /// bv[in]    : Bitvector to search in
    /// value[in] : Bit value to find (true or false)
    ///
    /// SUCCESS : Returns the zero-based index of the first bit equal to
    ///           `value`. The bitvector is not modified.
    /// FAILURE : Returns `SIZE_MAX` when no bit matches (including an
    ///           empty bitvector). The bitvector is not modified.
    ///
    /// USAGE:
    ///   u64 index = BitVecFind(&flags, true);
    ///   if (index != SIZE_MAX) { /* found at index */ }
    ///
    /// TAGS: BitVec, Find, Search, Access
    ///
    u64 BitVecFind(BitVec *bv, bool value);

    ///
    /// Find index of last occurrence of a specific bit value.
    ///
    /// bv[in]    : Bitvector to search in
    /// value[in] : Bit value to find (true or false)
    ///
    /// SUCCESS : Returns the zero-based index of the last bit equal to
    ///           `value`. The bitvector is not modified.
    /// FAILURE : Returns `SIZE_MAX` when no bit matches (including an
    ///           empty bitvector). The bitvector is not modified.
    ///
    /// USAGE:
    ///   u64 index = BitVecFindLast(&flags, false);
    ///
    /// TAGS: BitVec, FindLast, Search, Access
    ///
    u64 BitVecFindLast(BitVec *bv, bool value);

    ///
    /// Check if all bits in bitvector match the given value.
    ///
    /// bv[in]    : Bitvector to check
    /// value[in] : Value to check against (true or false)
    ///
    /// SUCCESS : Returns `true` when every bit equals `value` (vacuously
    ///           true on an empty bitvector). The bitvector is not
    ///           modified.
    /// FAILURE : Returns `false` when at least one bit differs from
    ///           `value`. The bitvector is not modified.
    ///
    /// USAGE:
    ///   bool all_set = BitVecAll(&flags, true);
    ///
    /// TAGS: BitVec, All, Check, Predicate
    ///
    bool BitVecAll(BitVec *bv, bool value);

    ///
    /// Check if any bit in bitvector matches the given value.
    ///
    /// bv[in]    : Bitvector to check
    /// value[in] : Value to check for (true or false)
    ///
    /// SUCCESS : Returns `true` when at least one bit equals `value`.
    ///           The bitvector is not modified.
    /// FAILURE : Returns `false` when no bit matches (including an empty
    ///           bitvector). The bitvector is not modified.
    ///
    /// USAGE:
    ///   bool any_set = BitVecAny(&flags, true);
    ///
    /// TAGS: BitVec, Any, Check, Predicate
    ///
    bool BitVecAny(BitVec *bv, bool value);

    ///
    /// Check if no bits in bitvector match the given value.
    ///
    /// bv[in]    : Bitvector to check
    /// value[in] : Value to check against (true or false)
    ///
    /// SUCCESS : Returns `true` when no bit equals `value` (vacuously
    ///           true on an empty bitvector). The bitvector is not
    ///           modified.
    /// FAILURE : Returns `false` when at least one bit equals `value`.
    ///           The bitvector is not modified.
    ///
    /// USAGE:
    ///   bool none_set = BitVecNone(&flags, true);
    ///
    /// TAGS: BitVec, None, Check, Predicate
    ///
    bool BitVecNone(BitVec *bv, bool value);

    ///
    /// Find the longest consecutive sequence of a specific bit value.
    ///
    /// bv[in]    : Bitvector to analyze
    /// value[in] : Bit value to find runs of (true or false)
    ///
    /// SUCCESS : Returns the length of the longest consecutive run of
    ///           bits equal to `value`. The bitvector is not modified.
    /// FAILURE : Returns `0` when no bit matches `value` (including an
    ///           empty bitvector). The bitvector is not modified.
    ///
    /// USAGE:
    ///   u64 longest = BitVecLongestRun(&flags, true);
    ///
    /// TAGS: BitVec, LongestRun, Analysis, Sequence
    ///
    u64 BitVecLongestRun(BitVec *bv, bool value);

#ifdef __cplusplus
}
#endif

#endif // MISRA_STD_CONTAINER_BITVEC_ACCESS_H