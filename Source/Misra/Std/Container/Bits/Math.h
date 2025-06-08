/// file      : std/container/Bits/math.h
/// author    : Generated following Misra project patterns
/// This is free and unencumbered software released into the public domain.
///
/// Mathematical and statistical operations for Bitstors.

#ifndef MISRA_STD_CONTAINER_Bits_MATH_H
#define MISRA_STD_CONTAINER_Bits_MATH_H

#include "Type.h"
#include <Misra/Types.h>

#ifdef __cplusplus
extern "C" {
#endif

    ///
    /// Calculate Hamming distance between two Bitstors.
    /// Hamming distance is the number of positions where bits differ.
    ///
    /// bv1[in] : First Bitstor
    /// bv2[in] : Second Bitstor
    ///
    /// RETURNS: Number of differing bits
    ///
    /// USAGE:
    ///   u64 distance = BitsHammingDistance(&bv1, &bv2);
    ///
    /// TAGS: Bits, Math, Hamming, Distance
    ///
    u64 BitsHammingDistance(Bits *bv1, Bits *bv2);

    ///
    /// Calculate Jaccard similarity between two Bitstors.
    /// Jaccard similarity = |intersection| / |union|
    ///
    /// bv1[in] : First Bitstor
    /// bv2[in] : Second Bitstor
    ///
    /// RETURNS: Jaccard similarity coefficient (0.0 to 1.0)
    ///
    /// USAGE:
    ///   double similarity = BitsJaccardSimilarity(&bv1, &bv2);
    ///
    /// TAGS: Bits, Math, Jaccard, Similarity
    ///
    double BitsJaccardSimilarity(Bits *bv1, Bits *bv2);

    ///
    /// Calculate cosine similarity between two Bitstors.
    /// Treats Bitstors as binary vectors and computes cosine of angle between them.
    ///
    /// bv1[in] : First Bitstor
    /// bv2[in] : Second Bitstor
    ///
    /// RETURNS: Cosine similarity coefficient (0.0 to 1.0)
    ///
    /// USAGE:
    ///   double similarity = BitsCosineSimilarity(&bv1, &bv2);
    ///
    /// TAGS: Bits, Math, Cosine, Similarity
    ///
    double BitsCosineSimilarity(Bits *bv1, Bits *bv2);

    ///
    /// Calculate dot product of two Bitstors.
    /// Dot product is the count of positions where both bits are 1.
    ///
    /// bv1[in] : First Bitstor
    /// bv2[in] : Second Bitstor
    ///
    /// RETURNS: Number of positions where both bits are 1
    ///
    /// USAGE:
    ///   u64 dot_product = BitsDotProduct(&bv1, &bv2);
    ///
    /// TAGS: Bits, Math, DotProduct, Intersection
    ///
    u64 BitsDotProduct(Bits *bv1, Bits *bv2);

    ///
    /// Calculate edit distance between two Bitstors.
    /// Edit distance is minimum number of single-bit operations to transform one into the other.
    ///
    /// bv1[in] : First Bitstor
    /// bv2[in] : Second Bitstor
    ///
    /// RETURNS: Minimum edit distance
    ///
    /// USAGE:
    ///   u64 distance = BitsEditDistance(&bv1, &bv2);
    ///
    /// TAGS: Bits, Math, EditDistance, Transform
    ///
    u64 BitsEditDistance(Bits *bv1, Bits *bv2);

    ///
    /// Calculate Pearson correlation coefficient between two Bitstors.
    /// Treats bits as 0/1 values and computes linear correlation.
    ///
    /// bv1[in] : First Bitstor
    /// bv2[in] : Second Bitstor
    ///
    /// RETURNS: Correlation coefficient (-1.0 to 1.0)
    ///
    /// USAGE:
    ///   double correlation = BitsCorrelation(&bv1, &bv2);
    ///
    /// TAGS: Bits, Math, Correlation, Statistics
    ///
    double BitsCorrelation(Bits *bv1, Bits *bv2);

    ///
    /// Calculate information entropy of a Bitstor.
    /// Entropy measures the randomness/information content of the bit pattern.
    ///
    /// bv[in] : Bitstor to analyze
    ///
    /// RETURNS: Entropy value in bits (0.0 to 1.0)
    ///
    /// USAGE:
    ///   double entropy = BitsEntropy(&flags);
    ///
    /// TAGS: Bits, Math, Entropy, Information
    ///
    double BitsEntropy(Bits *bv);

    ///
    /// Calculate alignment score between two Bitstors.
    /// Used in bioinformatics-style sequence alignment with match/mismatch scoring.
    ///
    /// bv1[in]      : First Bitstor
    /// bv2[in]      : Second Bitstor
    /// match[in]    : Score for matching bits
    /// mismatch[in] : Score for mismatching bits
    ///
    /// RETURNS: Total alignment score
    ///
    /// USAGE:
    ///   int score = BitsAlignmentScore(&seq1, &seq2, 2, -1);
    ///
    /// TAGS: Bits, Math, Alignment, Bioinformatics
    ///
    int BitsAlignmentScore(Bits *bv1, Bits *bv2, int match, int mismatch);

    ///
    /// Find best overlapping alignment between two Bitstors.
    /// Returns the offset that gives the best alignment score.
    ///
    /// bv1[in] : First Bitstor (reference)
    /// bv2[in] : Second Bitstor (query)
    ///
    /// RETURNS: Best alignment offset, or SIZE_MAX if no good alignment
    ///
    /// USAGE:
    ///   u64 offset = BitsBestAlignment(&reference, &query);
    ///
    /// TAGS: Bits, Math, Alignment, Overlap
    ///
    u64 BitsBestAlignment(Bits *bv1, Bits *bv2);

#ifdef __cplusplus
}
#endif

#endif // MISRA_STD_CONTAINER_Bits_MATH_H
