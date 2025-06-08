/// file      : std/container/Bits/compare.h
/// author    : Generated following Misra project patterns
/// This is free and unencumbered software released into the public domain.
///
/// Comparison and set-like operations for Bits bitvectors.

#ifndef MISRA_STD_CONTAINER_Bits_COMPARE_H
#define MISRA_STD_CONTAINER_Bits_COMPARE_H

#include "Type.h"
#include <Misra/Types.h>

#ifdef __cplusplus
extern "C" {
#endif

    ///
    /// Compare specific ranges of two Bits bitvectors for equality.
    ///
    /// bv1[in]   : First Bits bitvector
    /// start1[in]: Starting position in first Bits bitvector
    /// bv2[in]   : Second Bits bitvector
    /// start2[in]: Starting position in second Bits bitvector
    /// len[in]   : Number of bits to compare
    ///
    /// RETURNS: true if ranges are equal
    ///
    /// USAGE:
    ///   bool equal = BitsEqualsRange(&bv1, 5, &bv2, 10, 8);
    ///
    /// TAGS: Bits, Compare, Range, Equal
    ///
    bool BitsEqualsRange(Bits *bv1, u64 start1, Bits *bv2, u64 start2, u64 len);

    ///
    /// Compare ranges of two Bits bitvectors lexicographically.
    ///
    /// bv1[in]   : First Bits bitvector
    /// start1[in]: Starting position in first Bits bitvector
    /// bv2[in]   : Second Bits bitvector
    /// start2[in]: Starting position in second Bits bitvector
    /// len[in]   : Number of bits to compare
    ///
    /// RETURNS: -1 if bv1 < bv2, 0 if equal, 1 if bv1 > bv2
    ///
    /// USAGE:
    ///   int result = BitsCompareRange(&bv1, 5, &bv2, 10, 8);
    ///
    /// TAGS: Bits, Compare, Range, Lexicographic
    ///
    int BitsCompareRange(Bits *bv1, u64 start1, Bits *bv2, u64 start2, u64 len);

    ///
    /// Check if first Bits bitvector is a subset of the second.
    /// A Bits bitvector is a subset if all its 1-bits are also 1-bits in the other.
    ///
    /// bv1[in] : Potential subset Bits bitvector
    /// bv2[in] : Potential superset Bits bitvector
    ///
    /// RETURNS: true if bv1 is a subset of bv2
    ///
    /// USAGE:
    ///   bool is_subset = BitsIsSubset(&small_set, &large_set);
    ///
    /// TAGS: Bits, Compare, Subset, Set
    ///
    bool BitsIsSubset(Bits *bv1, Bits *bv2);

    ///
    /// Check if first Bits bitvector is a superset of the second.
    /// A Bits bitvector is a superset if it contains all 1-bits from the other.
    ///
    /// bv1[in] : Potential superset Bits bitvector
    /// bv2[in] : Potential subset Bits bitvector
    ///
    /// RETURNS: true if bv1 is a superset of bv2
    ///
    /// USAGE:
    ///   bool is_superset = BitsIsSuperset(&large_set, &small_set);
    ///
    /// TAGS: Bits, Compare, Superset, Set
    ///
    bool BitsIsSuperset(Bits *bv1, Bits *bv2);

    ///
    /// Check if two Bits bitvectors are disjoint (have no common 1-bits).
    ///
    /// bv1[in] : First Bits bitvector
    /// bv2[in] : Second Bits bitvector
    ///
    /// RETURNS: true if Bits bitvectors have no common 1-bits
    ///
    /// USAGE:
    ///   bool disjoint = BitsDisjoint(&set1, &set2);
    ///
    /// TAGS: Bits, Compare, Disjoint, Set
    ///
    bool BitsDisjoint(Bits *bv1, Bits *bv2);

    ///
    /// Check if two Bits bitvectors overlap (have any common 1-bits).
    ///
    /// bv1[in] : First Bits bitvector
    /// bv2[in] : Second Bits bitvector
    ///
    /// RETURNS: true if Bits bitvectors have any common 1-bits
    ///
    /// USAGE:
    ///   bool overlaps = BitsOverlaps(&set1, &set2);
    ///
    /// TAGS: Bits, Compare, Overlaps, Set
    ///
    bool BitsOverlaps(Bits *bv1, Bits *bv2);

    ///
    /// Check if two Bits bitvectors intersect (same as overlaps).
    ///
    /// bv1[in] : First Bits bitvector
    /// bv2[in] : Second Bits bitvector
    ///
    /// RETURNS: true if Bits bitvectors have any common 1-bits
    ///
    /// USAGE:
    ///   bool intersects = BitsIntersects(&set1, &set2);
    ///
    /// TAGS: Bits, Compare, Intersects, Set
    ///
    bool BitsIntersects(Bits *bv1, Bits *bv2);

    ///
    /// Test equality between two Bits bitvectors.
    /// Two Bits bitvectors are equal if they have the same length and all bits match.
    ///
    /// bv1[in] : First Bits bitvector
    /// bv2[in] : Second Bits bitvector
    ///
    /// RETURNS: true if Bits bitvectors are equal
    ///
    /// USAGE:
    ///   bool equal = BitsEquals(&flags1, &flags2);
    ///
    /// TAGS: Bits, Equals, Compare, Test
    ///
    bool BitsEquals(Bits *bv1, Bits *bv2);

    ///
    /// Compare two Bits bitvectors lexicographically.
    /// Comparison is done bit by bit from left to right.
    ///
    /// bv1[in] : First Bits bitvector
    /// bv2[in] : Second Bits bitvector
    ///
    /// RETURNS: -1 if bv1 < bv2, 0 if equal, 1 if bv1 > bv2
    ///
    /// USAGE:
    ///   int result = BitsCompare(&flags1, &flags2);
    ///
    /// TAGS: Bits, Compare, Lexicographic
    ///
    int BitsCompare(Bits *bv1, Bits *bv2);

    ///
    /// Compare two Bits bitvectors as unsigned integers.
    /// Bit vectors are treated as two numbers of native endianness.
    ///
    /// bv1[in] : First Bits bitvector
    /// bv2[in] : Second Bits bitvector
    ///
    /// RETURNS: -1 if bv1 < bv2, 0 if equal, 1 if bv1 > bv2
    ///
    /// USAGE:
    ///   int result = BitsNumericalCompare(&flags1, &flags2);
    ///
    /// TAGS: Bits, Compare, Numerical, Integer
    ///
    int BitsUnsignedCompare(Bits *bv1, Bits *bv2);

    ///
    /// Compare two Bits bitvectors as unsigned integers.
    /// Bit vectors are treated as two numbers of big endianness.
    ///
    /// bv1[in] : First Bits bitvector
    /// bv2[in] : Second Bits bitvector
    ///
    /// RETURNS: -1 if bv1 < bv2, 0 if equal, 1 if bv1 > bv2
    ///
    /// USAGE:
    ///   int result = BitsNumericalCompare(&flags1, &flags2);
    ///
    /// TAGS: Bits, Compare, Numerical, Integer
    ///
    int BitsUnsignedCompareBE(Bits *bv1, Bits *bv2);

    ///
    /// Compare two Bits bitvectors as unsigned integers.
    /// Bit vectors are treated as two numbers of little endianness.
    ///
    /// bv1[in] : First Bits bitvector
    /// bv2[in] : Second Bits bitvector
    ///
    /// RETURNS: -1 if bv1 < bv2, 0 if equal, 1 if bv1 > bv2
    ///
    /// USAGE:
    ///   int result = BitsNumericalCompare(&flags1, &flags2);
    ///
    /// TAGS: Bits, Compare, Numerical, Integer
    ///
    int BitsUnsignedCompareLE(Bits *bv1, Bits *bv2);

    ///
    /// Compare two Bits bitvectors as signed integers (MSB is sign bit).
    ///
    /// bv1[in] : First Bits bitvector
    /// bv2[in] : Second Bits bitvector
    ///
    /// RETURNS: -1 if bv1 < bv2, 0 if equal, 1 if bv1 > bv2
    ///
    /// USAGE:
    ///   int result = BitsSignedCompare(&flags1, &flags2);
    ///
    /// TAGS: Bits, Compare, Signed, Integer
    ///
    int BitsSignedCompare(Bits *bv1, Bits *bv2);

    ///
    /// Compare two Bits bitvectors as signed integers as big-endian.
    ///
    /// bv1[in] : First Bits bitvector
    /// bv2[in] : Second Bits bitvector
    ///
    /// RETURNS: -1 if bv1 < bv2, 0 if equal, 1 if bv1 > bv2
    ///
    /// USAGE:
    ///   int result = BitsSignedCompare(&flags1, &flags2);
    ///
    /// TAGS: Bits, Compare, Signed, Integer
    ///
    int BitsSignedCompareBE(Bits *bv1, Bits *bv2);

    ///
    /// Compare two Bits bitvectors as signed integers as little-endian.
    ///
    /// bv1[in] : First Bits bitvector
    /// bv2[in] : Second Bits bitvector
    ///
    /// RETURNS: -1 if bv1 < bv2, 0 if equal, 1 if bv1 > bv2
    ///
    /// USAGE:
    ///   int result = BitsSignedCompare(&flags1, &flags2);
    ///
    /// TAGS: Bits, Compare, Signed, Integer
    ///
    int BitsSignedCompareLE(Bits *bv1, Bits *bv2);

    ///
    /// Compare two Bits bitvectors by their Hamming weights (number of 1s).
    ///
    /// bv1[in] : First Bits bitvector
    /// bv2[in] : Second Bits bitvector
    ///
    /// RETURNS: -1 if bv1 has fewer 1s, 0 if equal, 1 if bv1 has more 1s
    ///
    /// USAGE:
    ///   int result = BitsWeightCompare(&flags1, &flags2);
    ///
    /// TAGS: Bits, Compare, Weight, Population
    ///
    int BitsWeightCompare(Bits *bv1, Bits *bv2);

    ///
    /// Check if bits in Bits bitvector are in sorted order.
    /// Useful for certain algorithms and data structures.
    ///
    /// bv[in] : Bits bitvector to check
    ///
    /// RETURNS: true if bits are in non-decreasing order (0s before 1s)
    ///
    /// USAGE:
    ///   bool sorted = BitsIsSorted(&flags);
    ///
    /// TAGS: Bits, Sorted, Order, Check
    ///
    bool BitsIsSorted(Bits *bv);

#ifdef __cplusplus
}
#endif

#endif // MISRA_STD_CONTAINER_Bits_COMPARE_H
