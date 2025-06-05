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
    bool BitVecGet(BitVec *bv, size idx);

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
    void BitVecSet(BitVec *bv, size idx, bool value);

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
    void BitVecFlip(BitVec *bv, size idx);

///
/// Get number of bits currently in bitvector.
///
/// bv[in] : Bitvector to get length of
///
/// RETURNS : Number of bits in bitvector
///
/// USAGE:
///   size num_bits = BitVecLen(&flags);
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
///   size max_bits = BitVecCapacity(&flags);
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
/// Get size of bitvector in bytes.
/// This returns the actual memory used by the bit data.
///
/// bv[in] : Bitvector to get byte size of
///
/// RETURNS : Number of bytes used to store bits
///
/// USAGE:
///   size bytes_used = BitVecByteSize(&flags);
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
    ///   size ones = BitVecCountOnes(&flags);
    ///
    /// TAGS: BitVec, Count, Ones, Population
    ///
    size BitVecCountOnes(BitVec *bv);

    ///
    /// Count number of bits set to 0 in bitvector.
    ///
    /// bv[in] : Bitvector to count zeros in
    ///
    /// RETURNS : Number of bits set to 0
    ///
    /// USAGE:
    ///   size zeros = BitVecCountZeros(&flags);
    ///
    /// TAGS: BitVec, Count, Zeros
    ///
    size BitVecCountZeros(BitVec *bv);

#ifdef __cplusplus
}
#endif

#endif // MISRA_STD_CONTAINER_BITVEC_ACCESS_H