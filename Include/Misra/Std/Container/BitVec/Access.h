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
    /// bv[in]  : Bitvector to get bit from
    /// idx[in] : Index of bit to get (0-based)
    ///
    /// RETURNS : true if bit is set, false otherwise
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
    /// bv[in]    : Bitvector to set bit in
    /// idx[in]   : Index of bit to set (0-based)
    /// value[in] : Value to set (true/false)
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
    /// bv[in]  : Bitvector to flip bit in
    /// idx[in] : Index of bit to flip (0-based)
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
#define BitVecLen(bv) ((bv)->length)

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
#define BitVecCapacity(bv) ((bv)->capacity)

///
/// Check if bitvector is empty.
///
/// bv[in] : Bitvector to check
///
/// RETURNS : true if bitvector has no bits, false otherwise
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
#define BitVecByteSize(bv) ((bv)->byte_size)

    ///
    /// Count number of bits set to 1 in bitvector.
    ///
    /// bv[in] : Bitvector to count ones in
    ///
    /// RETURNS : Number of bits set to 1
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
    /// RETURNS : Number of bits set to 0
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
    /// RETURNS: Index of first occurrence, or SIZE_MAX if not found
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
    /// RETURNS: Index of last occurrence, or SIZE_MAX if not found
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
    /// RETURNS: true if all bits match the value
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
    /// RETURNS: true if any bit matches the value
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
    /// RETURNS: true if no bits match the value
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
    /// RETURNS: Length of longest consecutive sequence
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