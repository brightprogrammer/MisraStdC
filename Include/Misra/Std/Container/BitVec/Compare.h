/// file      : std/container/bitvec/compare.h
/// author    : Generated following Misra project patterns
/// This is free and unencumbered software released into the public domain.
///
/// Comparison and set-like operations for bitvectors.

#ifndef MISRA_STD_CONTAINER_BITVEC_COMPARE_H
#define MISRA_STD_CONTAINER_BITVEC_COMPARE_H

#include "Type.h"
#include <Misra/Types.h>

#ifdef __cplusplus
extern "C" {
#endif

    ///
    /// Compare specific ranges of two bitvectors for equality.
    ///
    /// bv1[in]   : First bitvector
    /// start1[in]: Starting position in first bitvector
    /// bv2[in]   : Second bitvector
    /// start2[in]: Starting position in second bitvector
    /// len[in]   : Number of bits to compare
    ///
    /// RETURNS: true if ranges are equal
    ///
    /// USAGE:
    ///   bool equal = BitVecEqualsRange(&bv1, 5, &bv2, 10, 8);
    ///
    /// TAGS: BitVec, Compare, Range, Equal
    ///
    bool BitVecEqualsRange(BitVec *bv1, u64 start1, BitVec *bv2, u64 start2, u64 len);

    ///
    /// Compare ranges of two bitvectors lexicographically.
    ///
    /// bv1[in]   : First bitvector
    /// start1[in]: Starting position in first bitvector
    /// bv2[in]   : Second bitvector
    /// start2[in]: Starting position in second bitvector
    /// len[in]   : Number of bits to compare
    ///
    /// RETURNS: -1 if bv1 < bv2, 0 if equal, 1 if bv1 > bv2
    ///
    /// USAGE:
    ///   int result = BitVecCompareRange(&bv1, 5, &bv2, 10, 8);
    ///
    /// TAGS: BitVec, Compare, Range, Lexicographic
    ///
    int BitVecCompareRange(BitVec *bv1, u64 start1, BitVec *bv2, u64 start2, u64 len);

    ///
    /// Check if first bitvector is a subset of the second.
    /// A bitvector is a subset if all its 1-bits are also 1-bits in the other.
    ///
    /// bv1[in] : Potential subset bitvector
    /// bv2[in] : Potential superset bitvector
    ///
    /// RETURNS: true if bv1 is a subset of bv2
    ///
    /// USAGE:
    ///   bool is_subset = BitVecIsSubset(&small_set, &large_set);
    ///
    /// TAGS: BitVec, Compare, Subset, Set
    ///
    bool BitVecIsSubset(BitVec *bv1, BitVec *bv2);

    ///
    /// Check if first bitvector is a superset of the second.
    /// A bitvector is a superset if it contains all 1-bits from the other.
    ///
    /// bv1[in] : Potential superset bitvector
    /// bv2[in] : Potential subset bitvector
    ///
    /// RETURNS: true if bv1 is a superset of bv2
    ///
    /// USAGE:
    ///   bool is_superset = BitVecIsSuperset(&large_set, &small_set);
    ///
    /// TAGS: BitVec, Compare, Superset, Set
    ///
    bool BitVecIsSuperset(BitVec *bv1, BitVec *bv2);

    ///
    /// Check if two bitvectors are disjoint (have no common 1-bits).
    ///
    /// bv1[in] : First bitvector
    /// bv2[in] : Second bitvector
    ///
    /// RETURNS: true if bitvectors have no common 1-bits
    ///
    /// USAGE:
    ///   bool disjoint = BitVecDisjoint(&set1, &set2);
    ///
    /// TAGS: BitVec, Compare, Disjoint, Set
    ///
    bool BitVecDisjoint(BitVec *bv1, BitVec *bv2);

    ///
    /// Check if two bitvectors overlap (have any common 1-bits).
    ///
    /// bv1[in] : First bitvector
    /// bv2[in] : Second bitvector
    ///
    /// RETURNS: true if bitvectors have any common 1-bits
    ///
    /// USAGE:
    ///   bool overlaps = BitVecOverlaps(&set1, &set2);
    ///
    /// TAGS: BitVec, Compare, Overlaps, Set
    ///
    bool BitVecOverlaps(BitVec *bv1, BitVec *bv2);

    ///
<<<<<<< HEAD
=======
    /// Check if two bitvectors intersect (same as overlaps).
    ///
    /// bv1[in] : First bitvector
    /// bv2[in] : Second bitvector
    ///
    /// RETURNS: true if bitvectors have any common 1-bits
    ///
    /// USAGE:
    ///   bool intersects = BitVecIntersects(&set1, &set2);
    ///
    /// TAGS: BitVec, Compare, Intersects, Set
    ///
    bool BitVecIntersects(BitVec *bv1, BitVec *bv2);

    ///
>>>>>>> 6b0ba0ae675e6ef73ce083d149c2c1afbd4bdf9e
    /// Test equality between two bitvectors.
    /// Two bitvectors are equal if they have the same length and all bits match.
    ///
    /// bv1[in] : First bitvector
    /// bv2[in] : Second bitvector
    ///
    /// RETURNS: true if bitvectors are equal
    ///
    /// USAGE:
    ///   bool equal = BitVecEquals(&flags1, &flags2);
    ///
    /// TAGS: BitVec, Equals, Compare, Test
    ///
    bool BitVecEquals(BitVec *bv1, BitVec *bv2);

    ///
    /// Compare two bitvectors lexicographically.
    /// Comparison is done bit by bit from left to right.
    ///
    /// bv1[in] : First bitvector
    /// bv2[in] : Second bitvector
    ///
    /// RETURNS: -1 if bv1 < bv2, 0 if equal, 1 if bv1 > bv2
    ///
    /// USAGE:
    ///   int result = BitVecCompare(&flags1, &flags2);
    ///
    /// TAGS: BitVec, Compare, Lexicographic
    ///
    int BitVecCompare(BitVec *bv1, BitVec *bv2);

    ///
<<<<<<< HEAD
=======
    /// Compare two bitvectors as binary strings lexicographically.
    ///
    /// bv1[in] : First bitvector
    /// bv2[in] : Second bitvector
    ///
    /// RETURNS: -1 if bv1 < bv2, 0 if equal, 1 if bv1 > bv2
    ///
    /// USAGE:
    ///   int result = BitVecLexCompare(&flags1, &flags2);
    ///
    /// TAGS: BitVec, Compare, Lexicographic, String
    ///
    int BitVecLexCompare(BitVec *bv1, BitVec *bv2);

    ///
>>>>>>> 6b0ba0ae675e6ef73ce083d149c2c1afbd4bdf9e
    /// Compare two bitvectors as unsigned integers.
    /// Treats bitvectors as unsigned binary numbers (LSB first).
    ///
    /// bv1[in] : First bitvector
    /// bv2[in] : Second bitvector
    ///
    /// RETURNS: -1 if bv1 < bv2, 0 if equal, 1 if bv1 > bv2
    ///
    /// USAGE:
    ///   int result = BitVecNumericalCompare(&flags1, &flags2);
    ///
    /// TAGS: BitVec, Compare, Numerical, Integer
    ///
    int BitVecNumericalCompare(BitVec *bv1, BitVec *bv2);

    ///
    /// Compare two bitvectors by their Hamming weights (number of 1s).
    ///
    /// bv1[in] : First bitvector
    /// bv2[in] : Second bitvector
    ///
    /// RETURNS: -1 if bv1 has fewer 1s, 0 if equal, 1 if bv1 has more 1s
    ///
    /// USAGE:
    ///   int result = BitVecWeightCompare(&flags1, &flags2);
    ///
    /// TAGS: BitVec, Compare, Weight, Population
    ///
    int BitVecWeightCompare(BitVec *bv1, BitVec *bv2);

    ///
    /// Compare two bitvectors as signed integers (MSB is sign bit).
    ///
    /// bv1[in] : First bitvector
    /// bv2[in] : Second bitvector
    ///
    /// RETURNS: -1 if bv1 < bv2, 0 if equal, 1 if bv1 > bv2
    ///
    /// USAGE:
    ///   int result = BitVecSignedCompare(&flags1, &flags2);
    ///
    /// TAGS: BitVec, Compare, Signed, Integer
    ///
    int BitVecSignedCompare(BitVec *bv1, BitVec *bv2);

<<<<<<< HEAD

=======
    ///
    /// Test if first bitvector is lexicographically less than second.
    ///
    /// bv1[in] : First bitvector
    /// bv2[in] : Second bitvector
    ///
    /// RETURNS: true if bv1 < bv2 lexicographically
    ///
    /// USAGE:
    ///   bool less = BitVecIsLexicographicallyLess(&flags1, &flags2);
    ///
    /// TAGS: BitVec, Compare, Less, Lexicographic
    ///
    bool BitVecIsLexicographicallyLess(BitVec *bv1, BitVec *bv2);

    ///
    /// Test if first bitvector is numerically less than second.
    ///
    /// bv1[in] : First bitvector
    /// bv2[in] : Second bitvector
    ///
    /// RETURNS: true if bv1 < bv2 numerically
    ///
    /// USAGE:
    ///   bool less = BitVecIsNumericallyLess(&flags1, &flags2);
    ///
    /// TAGS: BitVec, Compare, Less, Numerical
    ///
    bool BitVecIsNumericallyLess(BitVec *bv1, BitVec *bv2);
>>>>>>> 6b0ba0ae675e6ef73ce083d149c2c1afbd4bdf9e

    ///
    /// Check if bits in bitvector are in sorted order.
    /// Useful for certain algorithms and data structures.
    ///
    /// bv[in] : Bitvector to check
    ///
    /// RETURNS: true if bits are in non-decreasing order (0s before 1s)
    ///
    /// USAGE:
    ///   bool sorted = BitVecIsSorted(&flags);
    ///
    /// TAGS: BitVec, Sorted, Order, Check
    ///
    bool BitVecIsSorted(BitVec *bv);

#ifdef __cplusplus
}
#endif

#endif // MISRA_STD_CONTAINER_BITVEC_COMPARE_H