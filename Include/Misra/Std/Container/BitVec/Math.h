/// file      : std/container/bitvec/math.h
/// author    : Generated following Misra project patterns
/// This is free and unencumbered software released into the public domain.
///
/// Mathematical and statistical operations for bitvectors.

#ifndef MISRA_STD_CONTAINER_BITVEC_MATH_H
#define MISRA_STD_CONTAINER_BITVEC_MATH_H

#include "Type.h"
#include <Misra/Types.h>

#ifdef __cplusplus
extern "C" {
#endif

    ///
    /// Calculate Hamming distance between two bitvectors.
    /// Hamming distance is the number of positions where bits differ.
    ///
    /// bv1[in] : First bitvector
    /// bv2[in] : Second bitvector
    ///
    /// RETURNS: Number of differing bits
    ///
    /// USAGE:
    ///   u64 distance = BitVecHammingDistance(&bv1, &bv2);
    ///
    /// TAGS: BitVec, Math, Hamming, Distance
    ///
    u64 BitVecHammingDistance(BitVec *bv1, BitVec *bv2);

    ///
    /// Calculate Jaccard similarity between two bitvectors.
    /// Jaccard similarity = |intersection| / |union|
    ///
    /// bv1[in] : First bitvector
    /// bv2[in] : Second bitvector
    ///
    /// RETURNS: Jaccard similarity coefficient (0.0 to 1.0)
    ///
    /// USAGE:
    ///   double similarity = BitVecJaccardSimilarity(&bv1, &bv2);
    ///
    /// TAGS: BitVec, Math, Jaccard, Similarity
    ///
    double BitVecJaccardSimilarity(BitVec *bv1, BitVec *bv2);

    ///
    /// Calculate cosine similarity between two bitvectors.
    /// Treats bitvectors as binary vectors and computes cosine of angle between them.
    ///
    /// bv1[in] : First bitvector
    /// bv2[in] : Second bitvector
    ///
    /// RETURNS: Cosine similarity coefficient (0.0 to 1.0)
    ///
    /// USAGE:
    ///   double similarity = BitVecCosineSimilarity(&bv1, &bv2);
    ///
    /// TAGS: BitVec, Math, Cosine, Similarity
    ///
    double BitVecCosineSimilarity(BitVec *bv1, BitVec *bv2);

    ///
    /// Calculate dot product of two bitvectors.
    /// Dot product is the count of positions where both bits are 1.
    ///
    /// bv1[in] : First bitvector
    /// bv2[in] : Second bitvector
    ///
    /// RETURNS: Number of positions where both bits are 1
    ///
    /// USAGE:
    ///   u64 dot_product = BitVecDotProduct(&bv1, &bv2);
    ///
    /// TAGS: BitVec, Math, DotProduct, Intersection
    ///
    u64 BitVecDotProduct(BitVec *bv1, BitVec *bv2);

    ///
    /// Calculate edit distance between two bitvectors.
    /// Edit distance is minimum number of single-bit operations to transform one into the other.
    ///
    /// bv1[in] : First bitvector
    /// bv2[in] : Second bitvector
    ///
    /// RETURNS: `true` on success, `false` when scratch allocation fails.
    ///
    /// The computed distance is written to `out` on success.
    ///
    /// USAGE:
    ///   u64 distance;
    ///   bool ok = BitVecTryEditDistance(&bv1, &bv2, &distance);
    ///
    /// TAGS: BitVec, Math, EditDistance, Transform, Fallible
    ///
    bool BitVecTryEditDistance(BitVec *bv1, BitVec *bv2, u64 *out);

    ///
    /// Calculate edit distance between two bitvectors.
    /// Edit distance is minimum number of single-bit operations to transform one into the other.
    ///
    /// bv1[in]   : First bitvector
    /// bv2[in]   : Second bitvector
    /// error[out] : Optional pointer set to `true` on failure and `false` on success
    ///
    /// RETURNS: Minimum edit distance, or `0` on failure.
    ///
    /// USAGE:
    ///   u64 distance = BitVecEditDistance(&bv1, &bv2);
    ///
    /// TAGS: BitVec, Math, EditDistance, Transform
    ///
    u64 BitVecEditDistanceWithError(BitVec *bv1, BitVec *bv2, bool *error);

    ///
    /// Calculate Pearson correlation coefficient between two bitvectors.
    /// Treats bits as 0/1 values and computes linear correlation.
    ///
    /// bv1[in] : First bitvector
    /// bv2[in] : Second bitvector
    ///
    /// RETURNS: Correlation coefficient (-1.0 to 1.0)
    ///
    /// USAGE:
    ///   double correlation = BitVecCorrelation(&bv1, &bv2);
    ///
    /// TAGS: BitVec, Math, Correlation, Statistics
    ///
    double BitVecCorrelation(BitVec *bv1, BitVec *bv2);

    ///
    /// Calculate information entropy of a bitvector.
    /// Entropy measures the randomness/information content of the bit pattern.
    ///
    /// bv[in] : Bitvector to analyze
    ///
    /// RETURNS: Entropy value in bits (0.0 to 1.0)
    ///
    /// USAGE:
    ///   double entropy = BitVecEntropy(&flags);
    ///
    /// TAGS: BitVec, Math, Entropy, Information
    ///
    double BitVecEntropy(BitVec *bv);

    ///
    /// Calculate alignment score between two bitvectors.
    /// Used in bioinformatics-style sequence alignment with match/mismatch scoring.
    ///
    /// bv1[in]      : First bitvector
    /// bv2[in]      : Second bitvector
    /// match[in]    : Score for matching bits
    /// mismatch[in] : Score for mismatching bits
    ///
    /// RETURNS: Total alignment score
    ///
    /// USAGE:
    ///   int score = BitVecAlignmentScore(&seq1, &seq2, 2, -1);
    ///
    /// TAGS: BitVec, Math, Alignment, Bioinformatics
    ///
    int BitVecAlignmentScore(BitVec *bv1, BitVec *bv2, int match, int mismatch);

    ///
    /// Find best overlapping alignment between two bitvectors.
    /// Returns the offset that gives the best alignment score.
    ///
    /// bv1[in] : First bitvector (reference)
    /// bv2[in] : Second bitvector (query)
    ///
    /// RETURNS: Best alignment offset, or SIZE_MAX if no good alignment
    ///
    /// USAGE:
    ///   u64 offset = BitVecBestAlignment(&reference, &query);
    ///
    /// TAGS: BitVec, Math, Alignment, Overlap
    ///
    u64 BitVecBestAlignment(BitVec *bv1, BitVec *bv2);

    ///
    /// Analyze run lengths in a bitvector.
    /// A run is a sequence of consecutive identical bits.
    /// Results array must be pre-allocated with sufficient space.
    ///
    /// bv[in]         : Bitvector to analyze
    /// runs[out]      : Array to store run lengths
    /// values[out]    : Array to store run values (true/false)
    /// max_runs[in]   : Maximum number of runs to store
    ///
    /// RETURNS: Number of runs found
    ///
    /// USAGE:
    ///   u64 run_lengths[50];
    ///   bool run_values[50];
    ///   u64 count = BitVecRunLengths(&flags, run_lengths, run_values, 50);
    ///
    /// TAGS: BitVec, RunLength, Analysis, Pattern
    ///
    u64 BitVecRunLengths(BitVec *bv, u64 *runs, bool *values, u64 max_runs);

#ifdef __cplusplus
}
#endif

static inline u64 MISRA_PRIV_BitVecEditDistanceNoError(BitVec *bv1, BitVec *bv2) {
    return BitVecEditDistanceWithError(bv1, bv2, NULL);
}

#define MISRA_PRIV_BITVEC_EDIT_DISTANCE_SELECT(_1, _2, _3, NAME, ...) NAME
#define BitVecEditDistance(...)                                                                                        \
    MISRA_PRIV_BITVEC_EDIT_DISTANCE_SELECT(__VA_ARGS__, BitVecEditDistanceWithError, MISRA_PRIV_BitVecEditDistanceNoError)(__VA_ARGS__)

#endif // MISRA_STD_CONTAINER_BITVEC_MATH_H
