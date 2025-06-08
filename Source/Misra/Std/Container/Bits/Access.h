/// file      : std/container/Bits/access.h
/// author    : Generated following Misra project patterns
/// This is free and unencumbered software released into the public domain.
///
/// Bit vector access operations

#ifndef MISRA_STD_CONTAINER_Bits_ACCESS_H
#define MISRA_STD_CONTAINER_Bits_ACCESS_H

#include "Type.h"

#ifdef __cplusplus
extern "C" {
#endif

    ///
    /// Get the value of bit at given index in Bitstor.
    ///
    /// bv[in]  : Bitstor to get bit from
    /// idx[in] : Index of bit to get (0-based)
    ///
    /// RETURNS : true if bit is set, false otherwise
    ///
    /// USAGE:
    ///   bool flag = BitsGet(&flags, 5);
    ///
    /// TAGS: Bits, Access, Get, Boolean
    ///
    bool BitsGet(Bits *bv, u64 idx);

    ///
    /// Set the value of bit at given index in Bitstor.
    ///
    /// bv[in]    : Bitstor to set bit in
    /// idx[in]   : Index of bit to set (0-based)
    /// value[in] : Value to set (true/false)
    ///
    /// USAGE:
    ///   BitsSet(&flags, 5, true);
    ///   BitsSet(&flags, 10, false);
    ///
    /// TAGS: Bits, Access, Set, Boolean
    ///
    void BitsSet(Bits *bv, u64 idx, bool value);

    ///
    /// Flip the value of bit at given index in Bitstor.
    /// Changes 0 to 1 and 1 to 0.
    ///
    /// bv[in]  : Bitstor to flip bit in
    /// idx[in] : Index of bit to flip (0-based)
    ///
    /// USAGE:
    ///   BitsFlip(&flags, 5);
    ///
    /// TAGS: Bits, Access, Flip, Toggle
    ///
    void BitsFlip(Bits *bv, u64 idx);

///
/// Get number of bits currently in Bitstor.
///
/// bv[in] : Bitstor to get length of
///
/// RETURNS : Number of bits in Bitstor
///
/// USAGE:
///   u64 num_bits = BitsLen(&flags);
///
/// TAGS: Bits, Length, Size
///
#define BitsLen(bv) ((bv)->length)

///
/// Get capacity of Bitstor in bits.
///
/// bv[in] : Bitstor to get capacity of
///
/// RETURNS : Maximum number of bits Bitstor can hold without reallocation
///
/// USAGE:
///   u64 max_bits = BitsCapacity(&flags);
///
/// TAGS: Bits, Capacity, Size
///
#define BitsCapacity(bv) ((bv)->capacity)

///
/// Check if Bitstor is empty.
///
/// bv[in] : Bitstor to check
///
/// RETURNS : true if Bitstor has no bits, false otherwise
///
/// USAGE:
///   if (BitsEmpty(&flags)) { /* handle empty case */ }
///
/// TAGS: Bits, Empty, Check
///
#define BitsEmpty(bv) (BitsLen(bv) == 0)

///
/// Get u64 of Bitstor in bytes.
/// This returns the actual memory used by the bit data.
///
/// bv[in] : Bitstor to get byte u64 of
///
/// RETURNS : Number of bytes used to store bits
///
/// USAGE:
///   u64 bytes_used = BitsByteSize(&flags);
///
/// TAGS: Bits, Size, Bytes, Memory
///
#define BitsByteSize(bv) ((bv)->byte_size)

    ///
    /// Count number of bits set to 1 in Bitstor.
    ///
    /// bv[in] : Bitstor to count ones in
    ///
    /// RETURNS : Number of bits set to 1
    ///
    /// USAGE:
    ///   u64 ones = BitsCountOnes(&flags);
    ///
    /// TAGS: Bits, Count, Ones, Population
    ///
    u64 BitsCountOnes(Bits *bv);

    ///
    /// Count number of bits set to 0 in Bitstor.
    ///
    /// bv[in] : Bitstor to count zeros in
    ///
    /// RETURNS : Number of bits set to 0
    ///
    /// USAGE:
    ///   u64 zeros = BitsCountZeros(&flags);
    ///
    /// TAGS: Bits, Count, Zeros
    ///
    u64 BitsCountZeros(Bits *bv);

    ///
    /// Find index of first occurrence of a specific bit value.
    ///
    /// bv[in]    : Bitstor to search in
    /// value[in] : Bit value to find (true or false)
    ///
    /// RETURNS: Index of first occurrence, or SIZE_MAX if not found
    ///
    /// USAGE:
    ///   u64 index = BitsFind(&flags, true);
    ///   if (index != SIZE_MAX) { /* found at index */ }
    ///
    /// TAGS: Bits, Find, Search, Access
    ///
    u64 BitsFind(Bits *bv, bool value);

    ///
    /// Find index of last occurrence of a specific bit value.
    ///
    /// bv[in]    : Bitstor to search in
    /// value[in] : Bit value to find (true or false)
    ///
    /// RETURNS: Index of last occurrence, or SIZE_MAX if not found
    ///
    /// USAGE:
    ///   u64 index = BitsFindLast(&flags, false);
    ///
    /// TAGS: Bits, FindLast, Search, Access
    ///
    u64 BitsFindLast(Bits *bv, bool value);

    ///
    /// Check if all bits in Bitstor match the given value.
    ///
    /// bv[in]    : Bitstor to check
    /// value[in] : Value to check against (true or false)
    ///
    /// RETURNS: true if all bits match the value
    ///
    /// USAGE:
    ///   bool all_set = BitsAll(&flags, true);
    ///
    /// TAGS: Bits, All, Check, Predicate
    ///
    bool BitsAll(Bits *bv, bool value);

    ///
    /// Check if any bit in Bitstor matches the given value.
    ///
    /// bv[in]    : Bitstor to check
    /// value[in] : Value to check for (true or false)
    ///
    /// RETURNS: true if any bit matches the value
    ///
    /// USAGE:
    ///   bool any_set = BitsAny(&flags, true);
    ///
    /// TAGS: Bits, Any, Check, Predicate
    ///
    bool BitsAny(Bits *bv, bool value);

    ///
    /// Check if no bits in Bitstor match the given value.
    ///
    /// bv[in]    : Bitstor to check
    /// value[in] : Value to check against (true or false)
    ///
    /// RETURNS: true if no bits match the value
    ///
    /// USAGE:
    ///   bool none_set = BitsNone(&flags, true);
    ///
    /// TAGS: Bits, None, Check, Predicate
    ///
    bool BitsNone(Bits *bv, bool value);

    ///
    /// Find the longest consecutive sequence of a specific bit value.
    ///
    /// bv[in]    : Bitstor to analyze
    /// value[in] : Bit value to find runs of (true or false)
    ///
    /// RETURNS: Length of longest consecutive sequence
    ///
    /// USAGE:
    ///   u64 longest = BitsLongestRun(&flags, true);
    ///
    /// TAGS: Bits, LongestRun, Analysis, Sequence
    ///
    u64 BitsLongestRun(Bits *bv, bool value);

#ifdef __cplusplus
}
#endif

#endif // MISRA_STD_CONTAINER_Bits_ACCESS_H

