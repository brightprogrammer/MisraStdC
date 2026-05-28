/// file      : std/container/bitvec/math.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
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
    /// SUCCESS : Returns the number of bit positions at which `bv1` and
    ///           `bv2` differ. Bits beyond the shorter operand are
    ///           treated as `0`. Neither operand is modified.
    /// FAILURE : Function cannot fail.
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
    /// SUCCESS : Returns the Jaccard similarity coefficient in
    ///           `[0.0, 1.0]`. Neither operand is modified.
    /// FAILURE : Returns `0.0` when both operands have no set bits
    ///           (degenerate union); the caller cannot distinguish that
    ///           from a true zero coefficient.
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
    /// SUCCESS : Returns the cosine similarity coefficient in
    ///           `[0.0, 1.0]`. Neither operand is modified.
    /// FAILURE : Returns `0.0` when either operand has no set bits
    ///           (degenerate magnitude); the caller cannot distinguish
    ///           that from a true zero coefficient.
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
    /// SUCCESS : Returns the number of positions where both bits are
    ///           `1`. Bits beyond the shorter operand are treated as
    ///           `0`. Neither operand is modified.
    /// FAILURE : Function cannot fail.
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
    /// SUCCESS : Returns `true` and writes the minimum number of
    ///           single-bit edits to `*out`. Neither operand is
    ///           modified.
    /// FAILURE : Returns `false` on allocator OOM while allocating the
    ///           Wagner-Fischer scratch buffer. `*out` is left
    ///           untouched.
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
    /// SUCCESS : Returns the minimum number of single-bit edits to
    ///           transform `bv1` into `bv2`. When `error` is non-NULL,
    ///           `*error` is set to `false`. Neither operand is
    ///           modified.
    /// FAILURE : Returns `0` on allocator OOM during the Wagner-Fischer
    ///           scratch allocation. When `error` is non-NULL, `*error`
    ///           is set to `true`; otherwise the caller cannot
    ///           distinguish failure from a true zero distance.
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
    /// SUCCESS : Returns the Pearson correlation coefficient in
    ///           `[-1.0, 1.0]`. Neither operand is modified.
    /// FAILURE : Returns `0.0` when either operand has zero variance
    ///           (all bits equal); the caller cannot distinguish that
    ///           from a true zero coefficient.
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
    /// SUCCESS : Returns the Shannon entropy of the bit pattern, in
    ///           bits, in `[0.0, 1.0]`. The bitvector is not modified.
    /// FAILURE : Returns `0.0` for an empty bitvector or one whose bits
    ///           are all equal; the caller cannot distinguish that
    ///           from a true zero entropy.
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
    /// SUCCESS : Returns the total alignment score computed as
    ///           `match * #matches + mismatch * #mismatches` over the
    ///           overlapping prefix of the two operands. Neither
    ///           operand is modified.
    /// FAILURE : Function cannot fail.
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
    /// SUCCESS : Returns the offset in `bv1` at which `bv2` produces
    ///           the highest alignment score. Neither operand is
    ///           modified.
    /// FAILURE : Returns `SIZE_MAX` when either operand is empty or
    ///           no candidate alignment yields a positive score. The
    ///           bitvectors are not modified.
    ///
    /// USAGE:
    ///   u64 offset = BitVecBestAlignment(&reference, &query);
    ///
    /// TAGS: BitVec, Math, Alignment, Overlap
    ///
    u64 BitVecBestAlignment(BitVec *bv1, BitVec *bv2);

    ///
    /// Analyze run lengths in a bitvector. A run is a sequence of
    /// consecutive identical bits. Two call shapes, dispatched by
    /// argument count.
    ///
    /// Raw form (caller-sized parallel buffers):
    ///   u64 lengths[50]; bool values[50];
    ///   u64 count = BitVecRunLengths(&flags, lengths, values, 50);
    ///   // Returns count of runs written; silently truncates if
    ///   // there are more than `max_runs`.
    ///
    /// Vec form (grows; emits `BitVecRun{length, value}` items):
    ///   BitVecRuns runs = VecInitT(runs, alloc);
    ///   bool ok = BitVecRunLengths(&flags, &runs);
    ///   // `runs.length` is the total count; no truncation.
    ///
    /// bv[in]              : Bitvector to analyze
    /// runs[out]           : (raw) Array to store run lengths
    /// values[out]         : (raw) Array to store run values
    /// max_runs[in]        : (raw) Maximum number of runs to store
    /// out[out]            : (vec) Vec to push `BitVecRun` records into
    ///
    /// SUCCESS : (raw) Number of runs written.
    ///           (vec) `true`; `out` holds every run.
    /// FAILURE : (vec) `false` on allocator OOM during the walk.
    ///
    /// TAGS: BitVec, RunLength, Analysis, Pattern
    ///
    u64  bitvec_run_lengths_raw(BitVec *bv, u64 *runs, bool *values, u64 max_runs);
    bool bitvec_run_lengths_vec(BitVec *bv, BitVecRuns *out);

#define BitVecRunLengths(...)                     OVERLOAD(BitVecRunLengths, __VA_ARGS__)
#define BitVecRunLengths_4(bv, runs, values, max) bitvec_run_lengths_raw((bv), (runs), (values), (max))
#define BitVecRunLengths_2(bv, out_vec)           bitvec_run_lengths_vec((bv), (out_vec))

#ifdef __cplusplus
}
#endif

static inline u64 bitvec_edit_distance_no_error(BitVec *bv1, BitVec *bv2) {
    return BitVecEditDistanceWithError(bv1, bv2, NULL);
}

#define BITVEC_EDIT_DISTANCE_SELECT(_1, _2, _3, NAME, ...) NAME

///
/// Calculate edit distance between two bitvectors. Edit distance is the
/// minimum number of single-bit operations required to transform one into
/// the other.
///
/// This public macro supports both forms:
///
/// - `BitVecEditDistance(bv1, bv2)`         - returns the result, no error channel.
/// - `BitVecEditDistance(bv1, bv2, error)`  - writes the error flag through `error`.
///
/// bv1[in]    : First bitvector.
/// bv2[in]    : Second bitvector.
/// error[out] : Optional pointer set to `true` on failure and `false` on success.
///
/// SUCCESS : Returns the edit distance as a `u64`. Neither operand is
///           modified.
/// FAILURE : Returns `0` on scratch-buffer allocation failure. With the
///           three-argument form `*error` is set to `true`; with the
///           two-argument form the caller cannot distinguish failure from
///           a true zero result.
///
/// USAGE:
///   u64 distance = BitVecEditDistance(&bv1, &bv2);
///
/// TAGS: BitVec, Math, EditDistance, Macro
///
#define BitVecEditDistance(...)                                                                                        \
    BITVEC_EDIT_DISTANCE_SELECT(__VA_ARGS__, BitVecEditDistanceWithError, bitvec_edit_distance_no_error)(__VA_ARGS__)

#endif // MISRA_STD_CONTAINER_BITVEC_MATH_H
