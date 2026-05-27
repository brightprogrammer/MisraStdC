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
    /// SUCCESS : true if ranges are equal
    /// FAILURE : false if either range exceeds its bitvector's length or any bit differs.
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
    /// SUCCESS : -1 if bv1 < bv2, 0 if equal, 1 if bv1 > bv2
    /// FAILURE : Aborts via the validator if either range exceeds its bitvector's length.
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
    /// SUCCESS : true if bv1 is a subset of bv2
    /// FAILURE : false when any 1-bit of `bv1` is not also set in `bv2`.
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
    /// SUCCESS : true if bv1 is a superset of bv2
    /// FAILURE : false when any 1-bit of `bv2` is not also set in `bv1`.
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
    /// SUCCESS : true if bitvectors have no common 1-bits
    /// FAILURE : false when any position holds a 1 in both bitvectors.
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
    /// SUCCESS : true if bitvectors have any common 1-bits
    /// FAILURE : false when the two bitvectors are disjoint.
    ///
    /// USAGE:
    ///   bool overlaps = BitVecOverlaps(&set1, &set2);
    ///
    /// TAGS: BitVec, Compare, Overlaps, Set
    ///
    bool BitVecOverlaps(BitVec *bv1, BitVec *bv2);

    ///
    /// Test equality between two bitvectors.
    /// Two bitvectors are equal if they have the same length and all bits match.
    ///
    /// bv1[in] : First bitvector
    /// bv2[in] : Second bitvector
    ///
    /// SUCCESS : true if bitvectors are equal
    /// FAILURE : false when lengths differ or any bit position differs.
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
    /// SUCCESS : -1 if bv1 < bv2, 0 if equal, 1 if bv1 > bv2
    /// FAILURE : Cannot fail; aborts on a corrupted magic via the validator.
    ///
    /// USAGE:
    ///   int result = BitVecCompare(&flags1, &flags2);
    ///
    /// TAGS: BitVec, Compare, Lexicographic
    ///
    int BitVecCompare(BitVec *bv1, BitVec *bv2);

    ///
    /// Hash a `BitVec` for use as a map key. FNV-1a over the live bits
    /// with the bit length mixed in at the tail, so two bitvectors that
    /// share a byte prefix but differ in length still land in different
    /// buckets. Typed signature; cast to `GenericHash` at the `Map` /
    /// `Vec` callback site.
    ///
    /// bv[in]   : BitVec to hash.
    /// size[in] : Ignored. Included for `GenericHash`-cast compatibility.
    ///
    /// SUCCESS : Returns a stable hash of the bit pattern + length.
    /// FAILURE : Cannot fail; aborts on a corrupted magic via the validator.
    ///
    /// USAGE:
    ///   Map(BitVec, u64) counts = MapInit(bitvec_hash, BitVecCompare, alloc);
    ///
    /// TAGS: BitVec, Hash, GenericHash
    ///
    u64 bitvec_hash(BitVec *bv, u32 size);

    ///
    /// Compare two bitvectors as unsigned integers.
    /// Treats bitvectors as unsigned binary numbers (LSB first).
    ///
    /// bv1[in] : First bitvector
    /// bv2[in] : Second bitvector
    ///
    /// SUCCESS : -1 if bv1 < bv2, 0 if equal, 1 if bv1 > bv2
    /// FAILURE : Cannot fail; aborts on a corrupted magic via the validator.
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
    /// SUCCESS : -1 if bv1 has fewer 1s, 0 if equal, 1 if bv1 has more 1s
    /// FAILURE : Cannot fail; aborts on a corrupted magic via the validator.
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
    /// SUCCESS : -1 if bv1 < bv2, 0 if equal, 1 if bv1 > bv2
    /// FAILURE : Cannot fail; aborts on a corrupted magic via the validator.
    ///
    /// USAGE:
    ///   int result = BitVecSignedCompare(&flags1, &flags2);
    ///
    /// TAGS: BitVec, Compare, Signed, Integer
    ///
    int BitVecSignedCompare(BitVec *bv1, BitVec *bv2);

    ///
    /// Check if bits in bitvector are in sorted order.
    /// Useful for certain algorithms and data structures.
    ///
    /// bv[in] : Bitvector to check
    ///
    /// SUCCESS : true if bits are in non-decreasing order (0s before 1s)
    /// FAILURE : false when any 0-bit follows a 1-bit in `bv`.
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
