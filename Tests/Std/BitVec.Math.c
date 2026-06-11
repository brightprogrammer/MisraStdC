#include <Misra/Std/Container/BitVec.h>
#include <Misra/Std/Allocator/Default.h>
#include <Misra/Std/Log.h>
#include <Misra/Types.h>
#include <Misra/Std/Math.h>

// Include test utilities
#include "../Util/TestRunner.h"

// Function prototypes for all Math tests
bool test_bitvec_hamming_distance_basic(void);
bool test_bitvec_hamming_distance_edge_cases(void);
bool test_bitvec_jaccard_similarity_basic(void);
bool test_bitvec_jaccard_similarity_edge_cases(void);
bool test_bitvec_cosine_similarity_basic(void);
bool test_bitvec_cosine_similarity_edge_cases(void);
bool test_bitvec_dot_product_basic(void);
bool test_bitvec_dot_product_edge_cases(void);
bool test_bitvec_edit_distance_basic(void);
bool test_bitvec_edit_distance_edge_cases(void);
bool test_bitvec_correlation_basic(void);
bool test_bitvec_correlation_edge_cases(void);
bool test_bitvec_entropy_basic(void);
bool test_bitvec_entropy_edge_cases(void);
bool test_bitvec_alignment_score_basic(void);
bool test_bitvec_alignment_score_edge_cases(void);
bool test_bitvec_best_alignment_basic(void);
bool test_bitvec_best_alignment_edge_cases(void);
bool test_bitvec_math_stress_tests(void);

// Deadend tests
bool test_bitvec_hamming_distance_null_bv1(void);
bool test_bitvec_hamming_distance_null_bv2(void);
bool test_bitvec_jaccard_similarity_null_bv1(void);
bool test_bitvec_jaccard_similarity_null_bv2(void);
bool test_bitvec_cosine_similarity_null_bv1(void);
bool test_bitvec_cosine_similarity_null_bv2(void);
bool test_bitvec_dot_product_null_bv1(void);
bool test_bitvec_dot_product_null_bv2(void);
bool test_bitvec_edit_distance_null_bv1(void);
bool test_bitvec_edit_distance_null_bv2(void);
bool test_bitvec_correlation_null_bv1(void);
bool test_bitvec_correlation_null_bv2(void);
bool test_bitvec_entropy_null(void);
bool test_bitvec_alignment_score_null_bv1(void);
bool test_bitvec_alignment_score_null_bv2(void);
bool test_bitvec_best_alignment_null_bv1(void);
bool test_bitvec_best_alignment_null_bv2(void);
bool test_jaccard_guard_fires_only_when_both_empty(void);
bool test_jaccard_bv1_shorter_no_abort(void);
bool test_jaccard_bv2_shorter_no_abort(void);
bool test_jaccard_equal_length_uses_bv2_bits(void);
bool test_edit_distance_col0_base_case(void);
bool test_edit_distance_deletion_term(void);
bool test_edit_distance_empty_to_len_base_row(void);
bool test_edit_distance_base_row_fully_filled(void);
bool test_edit_distance_insertion_term(void);
bool test_jaccard_rejects_bad_second_operand(void);
bool test_correlation_bv1_shorter_no_oob(void);
bool test_correlation_bv2_shorter_no_oob(void);
bool test_entropy_unbalanced_value_m8(void);
bool test_best_alignment_all_mismatch_offset(void);
bool test_best_alignment_offset(void);
bool test_best_alignment_empty_overlap_ignored(void);
bool test_best_alignment_first_tie_wins(void);
bool test_best_alignment_picks_max_not_min(void);
bool test_best_alignment_uses_real_score(void);

// Helper: push a 0/1 character string of bits into bv.
static void push_bits(BitVec *bv, const char *bits) {
    for (const char *p = bits; *p != '\0'; p++)
        BitVecPush(bv, *p == '1');
}

// Test BitVecHammingDistance basic functionality
bool test_bitvec_hamming_distance_basic(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVecHammingDistance basic functionality\n");

    BitVec bv1    = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVec bv2    = BitVecInit(ALLOCATOR_OF(&alloc));
    bool   result = true;

    // Test identical bitvectors
    BitVecPush(&bv1, true);
    BitVecPush(&bv1, false);
    BitVecPush(&bv1, true);
    BitVecPush(&bv2, true);
    BitVecPush(&bv2, false);
    BitVecPush(&bv2, true);

    u64 distance = BitVecHammingDistance(&bv1, &bv2);
    result       = result && (distance == 0);

    // Test completely different bitvectors
    BitVecClear(&bv2);
    BitVecPush(&bv2, false);
    BitVecPush(&bv2, true);
    BitVecPush(&bv2, false);

    distance = BitVecHammingDistance(&bv1, &bv2);
    result   = result && (distance == 3);

    // Test partially different bitvectors
    BitVecClear(&bv2);
    BitVecPush(&bv2, true); // Same
    BitVecPush(&bv2, true); // Different
    BitVecPush(&bv2, true); // Same

    distance = BitVecHammingDistance(&bv1, &bv2);
    result   = result && (distance == 1);

    BitVecDeinit(&bv1);
    BitVecDeinit(&bv2);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Test BitVecHammingDistance edge cases
bool test_bitvec_hamming_distance_edge_cases(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVecHammingDistance edge cases\n");

    BitVec bv1    = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVec bv2    = BitVecInit(ALLOCATOR_OF(&alloc));
    bool   result = true;

    // Test empty bitvectors
    u64 distance = BitVecHammingDistance(&bv1, &bv2);
    result       = result && (distance == 0);

    // Test different lengths
    BitVecPush(&bv1, true);
    BitVecPush(&bv1, false);
    BitVecPush(&bv2, true);

    distance = BitVecHammingDistance(&bv1, &bv2);
    result   = result && (distance == 1); // 1 length difference

    // Test one empty, one non-empty
    BitVecClear(&bv2);
    distance = BitVecHammingDistance(&bv1, &bv2);
    result   = result && (distance == 2); // Length of bv1

    BitVecDeinit(&bv1);
    BitVecDeinit(&bv2);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Test BitVecJaccardSimilarity basic functionality
bool test_bitvec_jaccard_similarity_basic(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVecJaccardSimilarity basic functionality\n");

    BitVec bv1    = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVec bv2    = BitVecInit(ALLOCATOR_OF(&alloc));
    bool   result = true;

    // Test identical bitvectors
    BitVecPush(&bv1, true);
    BitVecPush(&bv1, false);
    BitVecPush(&bv1, true);
    BitVecPush(&bv2, true);
    BitVecPush(&bv2, false);
    BitVecPush(&bv2, true);

    double similarity = BitVecJaccardSimilarity(&bv1, &bv2);
    result            = result && (F64Abs(similarity - 1.0) < 0.001);

    // Test no overlap
    BitVecClear(&bv1);
    BitVecClear(&bv2);
    BitVecPush(&bv1, true);
    BitVecPush(&bv1, false);
    BitVecPush(&bv2, false);
    BitVecPush(&bv2, true);

    similarity = BitVecJaccardSimilarity(&bv1, &bv2);
    result     = result && (F64Abs(similarity - 0.0) < 0.001);

    // Test partial overlap
    BitVecClear(&bv1);
    BitVecClear(&bv2);
    BitVecPush(&bv1, true);
    BitVecPush(&bv1, true);
    BitVecPush(&bv1, false);
    BitVecPush(&bv2, true);
    BitVecPush(&bv2, false);
    BitVecPush(&bv2, false);
    // Intersection: 1, Union: 2, Jaccard = 1/2 = 0.5

    similarity = BitVecJaccardSimilarity(&bv1, &bv2);
    result     = result && (F64Abs(similarity - 0.5) < 0.001);

    BitVecDeinit(&bv1);
    BitVecDeinit(&bv2);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Test BitVecJaccardSimilarity edge cases
bool test_bitvec_jaccard_similarity_edge_cases(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVecJaccardSimilarity edge cases\n");

    BitVec bv1    = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVec bv2    = BitVecInit(ALLOCATOR_OF(&alloc));
    bool   result = true;

    // Test empty bitvectors
    double similarity = BitVecJaccardSimilarity(&bv1, &bv2);
    result            = result && (F64Abs(similarity - 1.0) < 0.001);

    // Test all zeros
    BitVecPush(&bv1, false);
    BitVecPush(&bv1, false);
    BitVecPush(&bv2, false);
    BitVecPush(&bv2, false);

    similarity = BitVecJaccardSimilarity(&bv1, &bv2);
    result     = result && (F64Abs(similarity - 1.0) < 0.001);

    BitVecDeinit(&bv1);
    BitVecDeinit(&bv2);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Test BitVecCosineSimilarity basic functionality
bool test_bitvec_cosine_similarity_basic(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVecCosineSimilarity basic functionality\n");

    BitVec bv1    = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVec bv2    = BitVecInit(ALLOCATOR_OF(&alloc));
    bool   result = true;

    // Test identical bitvectors
    BitVecPush(&bv1, true);
    BitVecPush(&bv1, false);
    BitVecPush(&bv1, true);
    BitVecPush(&bv2, true);
    BitVecPush(&bv2, false);
    BitVecPush(&bv2, true);

    double similarity = BitVecCosineSimilarity(&bv1, &bv2);
    result            = result && (F64Abs(similarity - 1.0) < 0.001);

    // Test orthogonal vectors
    BitVecClear(&bv1);
    BitVecClear(&bv2);
    BitVecPush(&bv1, true);
    BitVecPush(&bv1, false);
    BitVecPush(&bv2, false);
    BitVecPush(&bv2, true);

    similarity = BitVecCosineSimilarity(&bv1, &bv2);
    result     = result && (F64Abs(similarity - 0.0) < 0.001);

    BitVecDeinit(&bv1);
    BitVecDeinit(&bv2);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Test BitVecCosineSimilarity edge cases
bool test_bitvec_cosine_similarity_edge_cases(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVecCosineSimilarity edge cases\n");

    BitVec bv1    = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVec bv2    = BitVecInit(ALLOCATOR_OF(&alloc));
    bool   result = true;

    // Test zero vectors
    BitVecPush(&bv1, false);
    BitVecPush(&bv1, false);
    BitVecPush(&bv2, false);
    BitVecPush(&bv2, false);

    double similarity = BitVecCosineSimilarity(&bv1, &bv2);
    result            = result && (similarity == 0.0);

    // Test one zero vector
    BitVecClear(&bv2);
    BitVecPush(&bv2, true);
    BitVecPush(&bv2, false);

    similarity = BitVecCosineSimilarity(&bv1, &bv2);
    result     = result && (similarity == 0.0);

    BitVecDeinit(&bv1);
    BitVecDeinit(&bv2);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Test BitVecDotProduct basic functionality
bool test_bitvec_dot_product_basic(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVecDotProduct basic functionality\n");

    BitVec bv1    = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVec bv2    = BitVecInit(ALLOCATOR_OF(&alloc));
    bool   result = true;

    // Test basic dot product
    BitVecPush(&bv1, true);
    BitVecPush(&bv1, false);
    BitVecPush(&bv1, true);
    BitVecPush(&bv1, true);
    BitVecPush(&bv2, true);
    BitVecPush(&bv2, true);
    BitVecPush(&bv2, false);
    BitVecPush(&bv2, true);

    u64 product = BitVecDotProduct(&bv1, &bv2);
    result      = result && (product == 2); // Positions 0 and 3

    // Test no overlap
    BitVecClear(&bv1);
    BitVecClear(&bv2);
    BitVecPush(&bv1, true);
    BitVecPush(&bv1, false);
    BitVecPush(&bv2, false);
    BitVecPush(&bv2, true);

    product = BitVecDotProduct(&bv1, &bv2);
    result  = result && (product == 0);

    BitVecDeinit(&bv1);
    BitVecDeinit(&bv2);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Test BitVecDotProduct edge cases
bool test_bitvec_dot_product_edge_cases(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVecDotProduct edge cases\n");

    BitVec bv1    = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVec bv2    = BitVecInit(ALLOCATOR_OF(&alloc));
    bool   result = true;

    // Test empty bitvectors
    u64 product = BitVecDotProduct(&bv1, &bv2);
    result      = result && (product == 0);

    // Test different lengths
    BitVecPush(&bv1, true);
    BitVecPush(&bv1, true);
    BitVecPush(&bv1, true);
    BitVecPush(&bv2, true);
    BitVecPush(&bv2, false);

    product = BitVecDotProduct(&bv1, &bv2);
    result  = result && (product == 1); // Only first position counts

    BitVecDeinit(&bv1);
    BitVecDeinit(&bv2);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Test BitVecEditDistance basic functionality
bool test_bitvec_edit_distance_basic(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVecEditDistance basic functionality\n");

    BitVec bv1    = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVec bv2    = BitVecInit(ALLOCATOR_OF(&alloc));
    bool   result = true;

    // Test identical strings
    BitVecPush(&bv1, true);
    BitVecPush(&bv1, false);
    BitVecPush(&bv1, true);
    BitVecPush(&bv2, true);
    BitVecPush(&bv2, false);
    BitVecPush(&bv2, true);

    bool error    = true;
    u64  distance = BitVecEditDistance(&bv1, &bv2, &error);
    result        = result && !error && (distance == 0);

    // Test single substitution
    BitVecClear(&bv2);
    BitVecPush(&bv2, true);
    BitVecPush(&bv2, true); // Changed from false
    BitVecPush(&bv2, true);

    distance = BitVecEditDistance(&bv1, &bv2);
    result   = result && (distance == 1);

    // Test insertion
    BitVecClear(&bv2);
    BitVecPush(&bv2, true);
    BitVecPush(&bv2, false);
    BitVecPush(&bv2, true);
    BitVecPush(&bv2, false); // Extra bit

    distance = BitVecEditDistance(&bv1, &bv2);
    result   = result && (distance == 1);

    BitVecDeinit(&bv1);
    BitVecDeinit(&bv2);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Test BitVecEditDistance edge cases
bool test_bitvec_edit_distance_edge_cases(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVecEditDistance edge cases\n");

    BitVec bv1    = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVec bv2    = BitVecInit(ALLOCATOR_OF(&alloc));
    bool   result = true;

    // Test empty to empty
    u64 distance = BitVecEditDistance(&bv1, &bv2);
    result       = result && (distance == 0);

    // Test empty to non-empty
    BitVecPush(&bv2, true);
    BitVecPush(&bv2, false);

    distance = BitVecEditDistance(&bv1, &bv2);
    result   = result && (distance == 2);

    // Test non-empty to empty
    distance = BitVecEditDistance(&bv2, &bv1);
    result   = result && (distance == 2);

    BitVecDeinit(&bv1);
    BitVecDeinit(&bv2);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Test BitVecCorrelation basic functionality
bool test_bitvec_correlation_basic(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVecCorrelation basic functionality\n");

    BitVec bv1    = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVec bv2    = BitVecInit(ALLOCATOR_OF(&alloc));
    bool   result = true;

    // Test perfect correlation
    BitVecPush(&bv1, true);
    BitVecPush(&bv1, false);
    BitVecPush(&bv1, true);
    BitVecPush(&bv1, false);
    BitVecPush(&bv2, true);
    BitVecPush(&bv2, false);
    BitVecPush(&bv2, true);
    BitVecPush(&bv2, false);

    double correlation = BitVecCorrelation(&bv1, &bv2);
    result             = result && (F64Abs(correlation - 1.0) < 0.001);

    // Test perfect anti-correlation
    BitVecClear(&bv2);
    BitVecPush(&bv2, false);
    BitVecPush(&bv2, true);
    BitVecPush(&bv2, false);
    BitVecPush(&bv2, true);

    correlation = BitVecCorrelation(&bv1, &bv2);
    result      = result && (F64Abs(correlation + 1.0) < 0.001);

    BitVecDeinit(&bv1);
    BitVecDeinit(&bv2);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Test BitVecCorrelation edge cases
bool test_bitvec_correlation_edge_cases(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVecCorrelation edge cases\n");

    BitVec bv1    = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVec bv2    = BitVecInit(ALLOCATOR_OF(&alloc));
    bool   result = true;

    // Test empty bitvectors
    double correlation = BitVecCorrelation(&bv1, &bv2);
    result             = result && (correlation == 1.0);

    // Test uniform vectors
    BitVecPush(&bv1, true);
    BitVecPush(&bv1, true);
    BitVecPush(&bv1, true);
    BitVecPush(&bv2, true);
    BitVecPush(&bv2, true);
    BitVecPush(&bv2, true);

    correlation = BitVecCorrelation(&bv1, &bv2);
    result      = result && (correlation == 0.0); // No variance

    BitVecDeinit(&bv1);
    BitVecDeinit(&bv2);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Test BitVecEntropy basic functionality
bool test_bitvec_entropy_basic(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVecEntropy basic functionality\n");

    BitVec bv     = BitVecInit(ALLOCATOR_OF(&alloc));
    bool   result = true;

    // Test maximum entropy (equal 0s and 1s)
    BitVecPush(&bv, true);
    BitVecPush(&bv, false);
    BitVecPush(&bv, true);
    BitVecPush(&bv, false);

    double entropy = BitVecEntropy(&bv);
    result         = result && (F64Abs(entropy - 1.0) < 0.001);

    // Test minimum entropy (all same)
    BitVecClear(&bv);
    BitVecPush(&bv, true);
    BitVecPush(&bv, true);
    BitVecPush(&bv, true);

    entropy = BitVecEntropy(&bv);
    result  = result && (entropy == 0.0);

    BitVecDeinit(&bv);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Test BitVecEntropy edge cases
bool test_bitvec_entropy_edge_cases(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVecEntropy edge cases\n");

    BitVec bv     = BitVecInit(ALLOCATOR_OF(&alloc));
    bool   result = true;

    // Test empty bitvector
    double entropy = BitVecEntropy(&bv);
    result         = result && (entropy == 0.0);

    // Test single bit
    BitVecPush(&bv, true);
    entropy = BitVecEntropy(&bv);
    result  = result && (entropy == 0.0);

    BitVecDeinit(&bv);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Test BitVecAlignmentScore basic functionality
bool test_bitvec_alignment_score_basic(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVecAlignmentScore basic functionality\n");

    BitVec bv1    = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVec bv2    = BitVecInit(ALLOCATOR_OF(&alloc));
    bool   result = true;

    // Test perfect match
    BitVecPush(&bv1, true);
    BitVecPush(&bv1, false);
    BitVecPush(&bv1, true);
    BitVecPush(&bv2, true);
    BitVecPush(&bv2, false);
    BitVecPush(&bv2, true);

    int score = BitVecAlignmentScore(&bv1, &bv2, 2, -1);
    result    = result && (score == 6); // 3 matches * 2

    // Test perfect mismatch
    BitVecClear(&bv2);
    BitVecPush(&bv2, false);
    BitVecPush(&bv2, true);
    BitVecPush(&bv2, false);

    score  = BitVecAlignmentScore(&bv1, &bv2, 2, -1);
    result = result && (score == -3); // 3 mismatches * -1

    BitVecDeinit(&bv1);
    BitVecDeinit(&bv2);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Test BitVecAlignmentScore edge cases
bool test_bitvec_alignment_score_edge_cases(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVecAlignmentScore edge cases\n");

    BitVec bv1    = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVec bv2    = BitVecInit(ALLOCATOR_OF(&alloc));
    bool   result = true;

    // Test empty bitvectors
    int score = BitVecAlignmentScore(&bv1, &bv2, 1, -1);
    result    = result && (score == 0);

    // Test different lengths (only overlapping region scored)
    BitVecPush(&bv1, true);
    BitVecPush(&bv1, false);
    BitVecPush(&bv1, true);
    BitVecPush(&bv2, true);
    BitVecPush(&bv2, false);

    score  = BitVecAlignmentScore(&bv1, &bv2, 1, -1);
    result = result && (score == 2); // 2 matches

    BitVecDeinit(&bv1);
    BitVecDeinit(&bv2);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Test BitVecBestAlignment basic functionality
bool test_bitvec_best_alignment_basic(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVecBestAlignment basic functionality\n");

    BitVec bv1    = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVec bv2    = BitVecInit(ALLOCATOR_OF(&alloc));
    bool   result = true;

    // Create bv1: 1100110
    BitVecPush(&bv1, true);
    BitVecPush(&bv1, true);
    BitVecPush(&bv1, false);
    BitVecPush(&bv1, false);
    BitVecPush(&bv1, true);
    BitVecPush(&bv1, true);
    BitVecPush(&bv1, false);

    // Create bv2: 110 (should match at position 0 and 4)
    BitVecPush(&bv2, true);
    BitVecPush(&bv2, true);
    BitVecPush(&bv2, false);

    u64 best_pos = BitVecBestAlignment(&bv1, &bv2);
    result       = result && (best_pos == 0 || best_pos == 4);

    BitVecDeinit(&bv1);
    BitVecDeinit(&bv2);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Test BitVecBestAlignment edge cases
bool test_bitvec_best_alignment_edge_cases(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVecBestAlignment edge cases\n");

    BitVec bv1    = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVec bv2    = BitVecInit(ALLOCATOR_OF(&alloc));
    bool   result = true;

    // Test empty bitvectors
    u64 best_pos = BitVecBestAlignment(&bv1, &bv2);
    result       = result && (best_pos == 0);

    // Test bv2 longer than bv1
    BitVecPush(&bv1, true);
    BitVecPush(&bv2, true);
    BitVecPush(&bv2, false);

    best_pos = BitVecBestAlignment(&bv1, &bv2);
    result   = result && (best_pos == 0);

    BitVecDeinit(&bv1);
    BitVecDeinit(&bv2);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Stress test for Math functions
bool test_bitvec_math_stress_tests(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVec Math stress tests\n");

    BitVec bv1    = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVec bv2    = BitVecInit(ALLOCATOR_OF(&alloc));
    bool   result = true;

    // Create large bitvectors
    for (int i = 0; i < 1000; i++) {
        BitVecPush(&bv1, i % 2 == 0);
        BitVecPush(&bv2, i % 3 == 0);
    }

    // Test that all functions complete without crashing
    u64    hamming     = BitVecHammingDistance(&bv1, &bv2);
    double jaccard     = BitVecJaccardSimilarity(&bv1, &bv2);
    double cosine      = BitVecCosineSimilarity(&bv1, &bv2);
    u64    dot_prod    = BitVecDotProduct(&bv1, &bv2);
    double correlation = BitVecCorrelation(&bv1, &bv2);
    double entropy1    = BitVecEntropy(&bv1);
    int    align_score = BitVecAlignmentScore(&bv1, &bv2, 1, -1);
    u64    best_align  = BitVecBestAlignment(&bv1, &bv2);

    // Test edit distance with smaller vectors (expensive operation)
    BitVec small1 = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVec small2 = BitVecInit(ALLOCATOR_OF(&alloc));
    for (int i = 0; i < 50; i++) {
        BitVecPush(&small1, i % 2 == 0);
        BitVecPush(&small2, i % 3 == 0);
    }
    u64 edit_dist = BitVecEditDistance(&small1, &small2);

    result = result && (hamming < 1000);
    result = result && (jaccard >= 0.0 && jaccard <= 1.0);
    result = result && (cosine >= 0.0 && cosine <= 1.0);
    result = result && (dot_prod < 1000);
    result = result && (correlation >= -1.0 && correlation <= 1.0);
    result = result && (entropy1 >= 0.0 && entropy1 <= 1.0);
    // bucket B (MULL-DISCOVERY-CONVENTIONS): the exact alignment score is an
    // implementation-chosen function of the scoring strategy; the caller-
    // observable contract is the bound -- with match=+1 / mismatch=-1 over
    // <=1000 compared bits the score must lie in [-1000, 1000].
    result = result && (align_score >= -1000 && align_score <= 1000);
    result = result && (best_align <= 1000); // SIZE_MAX is valid
    result = result && (edit_dist < 100);

    BitVecDeinit(&bv1);
    BitVecDeinit(&bv2);
    BitVecDeinit(&small1);
    BitVecDeinit(&small2);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Deadend tests - each function with NULL parameters

bool test_bitvec_hamming_distance_null_bv1(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVecHammingDistance(NULL, bv2) - should fatal\n");
    BitVec bv2 = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVecPush(&bv2, true);
    BitVecHammingDistance(NULL, &bv2);
    BitVecDeinit(&bv2);
    DefaultAllocatorDeinit(&alloc);
    return true;
}

bool test_bitvec_hamming_distance_null_bv2(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVecHammingDistance(bv1, NULL) - should fatal\n");
    BitVec bv1 = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVecPush(&bv1, true);
    BitVecHammingDistance(&bv1, NULL);
    BitVecDeinit(&bv1);
    DefaultAllocatorDeinit(&alloc);
    return true;
}

bool test_bitvec_jaccard_similarity_null_bv1(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVecJaccardSimilarity(NULL, bv2) - should fatal\n");
    BitVec bv2 = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVecPush(&bv2, true);
    BitVecJaccardSimilarity(NULL, &bv2);
    BitVecDeinit(&bv2);
    DefaultAllocatorDeinit(&alloc);
    return true;
}

bool test_bitvec_jaccard_similarity_null_bv2(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVecJaccardSimilarity(bv1, NULL) - should fatal\n");
    BitVec bv1 = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVecPush(&bv1, true);
    BitVecJaccardSimilarity(&bv1, NULL);
    BitVecDeinit(&bv1);
    DefaultAllocatorDeinit(&alloc);
    return true;
}

bool test_bitvec_cosine_similarity_null_bv1(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVecCosineSimilarity(NULL, bv2) - should fatal\n");
    BitVec bv2 = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVecPush(&bv2, true);
    BitVecCosineSimilarity(NULL, &bv2);
    BitVecDeinit(&bv2);
    DefaultAllocatorDeinit(&alloc);
    return true;
}

bool test_bitvec_cosine_similarity_null_bv2(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVecCosineSimilarity(bv1, NULL) - should fatal\n");
    BitVec bv1 = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVecPush(&bv1, true);
    BitVecCosineSimilarity(&bv1, NULL);
    BitVecDeinit(&bv1);
    DefaultAllocatorDeinit(&alloc);
    return true;
}

bool test_bitvec_dot_product_null_bv1(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVecDotProduct(NULL, bv2) - should fatal\n");
    BitVec bv2 = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVecPush(&bv2, true);
    BitVecDotProduct(NULL, &bv2);
    BitVecDeinit(&bv2);
    DefaultAllocatorDeinit(&alloc);
    return true;
}

bool test_bitvec_dot_product_null_bv2(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVecDotProduct(bv1, NULL) - should fatal\n");
    BitVec bv1 = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVecPush(&bv1, true);
    BitVecDotProduct(&bv1, NULL);
    BitVecDeinit(&bv1);
    DefaultAllocatorDeinit(&alloc);
    return true;
}

bool test_bitvec_edit_distance_null_bv1(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVecEditDistance(NULL, bv2) - should fatal\n");
    BitVec bv2 = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVecPush(&bv2, true);
    BitVecEditDistance(NULL, &bv2);
    BitVecDeinit(&bv2);
    DefaultAllocatorDeinit(&alloc);
    return true;
}

bool test_bitvec_edit_distance_null_bv2(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVecEditDistance(bv1, NULL) - should fatal\n");
    BitVec bv1 = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVecPush(&bv1, true);
    BitVecEditDistance(&bv1, NULL);
    BitVecDeinit(&bv1);
    DefaultAllocatorDeinit(&alloc);
    return true;
}

bool test_bitvec_correlation_null_bv1(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVecCorrelation(NULL, bv2) - should fatal\n");
    BitVec bv2 = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVecPush(&bv2, true);
    BitVecCorrelation(NULL, &bv2);
    BitVecDeinit(&bv2);
    DefaultAllocatorDeinit(&alloc);
    return true;
}

bool test_bitvec_correlation_null_bv2(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVecCorrelation(bv1, NULL) - should fatal\n");
    BitVec bv1 = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVecPush(&bv1, true);
    BitVecCorrelation(&bv1, NULL);
    BitVecDeinit(&bv1);
    DefaultAllocatorDeinit(&alloc);
    return true;
}

bool test_bitvec_entropy_null(void) {
    WriteFmt("Testing BitVecEntropy(NULL) - should fatal\n");
    BitVecEntropy(NULL);
    return true;
}

bool test_bitvec_alignment_score_null_bv1(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVecAlignmentScore(NULL, bv2, 1, -1) - should fatal\n");
    BitVec bv2 = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVecPush(&bv2, true);
    BitVecAlignmentScore(NULL, &bv2, 1, -1);
    BitVecDeinit(&bv2);
    DefaultAllocatorDeinit(&alloc);
    return true;
}

bool test_bitvec_alignment_score_null_bv2(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVecAlignmentScore(bv1, NULL, 1, -1) - should fatal\n");
    BitVec bv1 = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVecPush(&bv1, true);
    BitVecAlignmentScore(&bv1, NULL, 1, -1);
    BitVecDeinit(&bv1);
    DefaultAllocatorDeinit(&alloc);
    return true;
}

bool test_bitvec_best_alignment_null_bv1(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVecBestAlignment(NULL, bv2) - should fatal\n");
    BitVec bv2 = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVecPush(&bv2, true);
    BitVecBestAlignment(NULL, &bv2);
    BitVecDeinit(&bv2);
    DefaultAllocatorDeinit(&alloc);
    return true;
}

bool test_bitvec_best_alignment_null_bv2(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVecBestAlignment(bv1, NULL) - should fatal\n");
    BitVec bv1 = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVecPush(&bv1, true);
    BitVecBestAlignment(&bv1, NULL);
    BitVecDeinit(&bv1);
    DefaultAllocatorDeinit(&alloc);
    return true;
}

// Main function that runs all tests
// Kills 1427:21 and 1427:41 cxx_eq_to_ne -- the early `return 1.0` guard
// `bv1->length == 0 && bv2->length == 0`. Flipping either `==` to `!=` makes
// the guard fire for an empty-vs-non-empty pair, returning 1.0 instead of the
// true Jaccard of 0.0 for two disjoint single-bit vectors.
bool test_jaccard_guard_fires_only_when_both_empty(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVecJaccardSimilarity guard only fires when both empty\n");

    BitVec empty = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVec one   = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVecPush(&one, true);

    bool result = true;

    // empty vs {1}: intersection 0, union 1 -> Jaccard 0.0 (kills 1427:41).
    double s1 = BitVecJaccardSimilarity(&empty, &one);
    result    = result && (F64Abs(s1 - 0.0) < 0.001);

    // {1} vs empty: same -> 0.0 (kills 1427:21).
    double s2 = BitVecJaccardSimilarity(&one, &empty);
    result    = result && (F64Abs(s2 - 0.0) < 0.001);

    BitVecDeinit(&empty);
    BitVecDeinit(&one);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Kills 1439:24 cxx_lt_to_le -- `(i < bv1->length)` guarding BitVecGet(bv1, i).
// With `<=`, when bv1 is the shorter vector the loop calls BitVecGet(bv1,
// bv1->length) which LOG_FATALs. A valid differing-length Jaccard must NOT
// abort and must return the correct 1.0.
bool test_jaccard_bv1_shorter_no_abort(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVecJaccardSimilarity with bv1 shorter than bv2\n");

    BitVec bv1 = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVec bv2 = BitVecInit(ALLOCATOR_OF(&alloc));

    // bv1 = {1}, bv2 = {1, 0}. Intersection 1 (pos 0), union 1 -> 1.0.
    BitVecPush(&bv1, true);
    BitVecPush(&bv2, true);
    BitVecPush(&bv2, false);

    double s      = BitVecJaccardSimilarity(&bv1, &bv2);
    bool   result = (F64Abs(s - 1.0) < 0.001);

    BitVecDeinit(&bv1);
    BitVecDeinit(&bv2);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Kills 1440:24 cxx_lt_to_le -- `(i < bv2->length)` guarding BitVecGet(bv2, i).
// With `<=`, when bv2 is the shorter vector the loop calls BitVecGet(bv2,
// bv2->length) which LOG_FATALs. A valid differing-length Jaccard must NOT
// abort and must return the correct 1.0.
bool test_jaccard_bv2_shorter_no_abort(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVecJaccardSimilarity with bv2 shorter than bv1\n");

    BitVec bv1 = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVec bv2 = BitVecInit(ALLOCATOR_OF(&alloc));

    // bv1 = {1, 0}, bv2 = {1}. Intersection 1 (pos 0), union 1 -> 1.0.
    BitVecPush(&bv1, true);
    BitVecPush(&bv1, false);
    BitVecPush(&bv2, true);

    double s      = BitVecJaccardSimilarity(&bv1, &bv2);
    bool   result = (F64Abs(s - 1.0) < 0.001);

    BitVecDeinit(&bv1);
    BitVecDeinit(&bv2);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Kills 1440:24 cxx_lt_to_ge -- `(i < bv2->length)` flipped to `(i >=
// bv2->length)`. In the valid region (i < len) the ternary then yields false
// instead of BitVecGet(bv2, i), so bv2's set bits vanish from the union. For
// an all-zero bv1 vs an all-one bv2 of equal length the real Jaccard is 0.0
// (union = count of bv2 ones); the mutant sees union 0 and returns 1.0.
bool test_jaccard_equal_length_uses_bv2_bits(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVecJaccardSimilarity reads bv2 bits in valid region\n");

    BitVec bv1 = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVec bv2 = BitVecInit(ALLOCATOR_OF(&alloc));

    // bv1 = {0, 0}, bv2 = {1, 1}. Intersection 0, union 2 -> 0.0.
    BitVecPush(&bv1, false);
    BitVecPush(&bv1, false);
    BitVecPush(&bv2, true);
    BitVecPush(&bv2, true);

    double s      = BitVecJaccardSimilarity(&bv1, &bv2);
    bool   result = (F64Abs(s - 0.0) < 0.001);

    BitVecDeinit(&bv1);
    BitVecDeinit(&bv2);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Kills 1525:21 cxx_assign_const -- `curr_row[0] = i` (column-0 base case of
// the edit-distance DP: deleting the i-bit bv1 prefix down to empty costs i)
// replaced with `curr_row[0] = 42`. For bv1 = {0,1}, bv2 = {1} the true
// distance is 1; the corrupted column-0 base forces the mutant to 2.
bool test_edit_distance_col0_base_case(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVecEditDistance column-0 base case\n");

    BitVec bv1 = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVec bv2 = BitVecInit(ALLOCATOR_OF(&alloc));

    BitVecPush(&bv1, false);
    BitVecPush(&bv1, true);
    BitVecPush(&bv2, true);

    u64  dist   = BitVecEditDistance(&bv1, &bv2);
    bool result = (dist == 1);

    BitVecDeinit(&bv1);
    BitVecDeinit(&bv2);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Kills 1530:17 cxx_init_const -- `u64 deletion = prev_row[j] + 1` (the
// deletion candidate of the edit-distance recurrence) replaced with
// `deletion = 42`. For bv1 = {0,1}, bv2 = {0} the deletion path is the unique
// minimizer: true distance is 1, the mutant yields 2.
bool test_edit_distance_deletion_term(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVecEditDistance deletion term\n");

    BitVec bv1 = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVec bv2 = BitVecInit(ALLOCATOR_OF(&alloc));

    BitVecPush(&bv1, false);
    BitVecPush(&bv1, true);
    BitVecPush(&bv2, false);

    u64  dist   = BitVecEditDistance(&bv1, &bv2);
    bool result = (dist == 1);

    BitVecDeinit(&bv1);
    BitVecDeinit(&bv2);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Kills 1520:23 cxx_le_to_lt -- the base-case row loop `for (j = 0; j <= len2;
// j++) prev_row[j] = j` becoming `j < len2` leaves prev_row[len2] uninitialised.
// With an EMPTY bv1 the i-loop never runs and the result is exactly
// prev_row[len2], whose correct value is len2 (turning empty into a len2 prefix
// takes len2 insertions). The mutant returns whatever garbage the scratch row
// held. A throwaway distance over longer vectors first stamps large stale
// values into the recycled scratch buffer so the missing init is observable
// (garbage != len2) rather than a coincidental zero.
bool test_edit_distance_empty_to_len_base_row(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVecEditDistance empty->len base row fill\n");

    // Stamp large values into the scratch arena: a 6-vs-6 edit distance fills
    // (len2+1)-wide rows with values up to ~6, then frees them for reuse.
    // warm_b length == target bv2 length (5) so the freed scratch rows are the
    // exact size the empty->5 distance will recycle, guaranteeing the stale
    // (large) values land in prev_row[len2].
    BitVec warm_a = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVec warm_b = BitVecInit(ALLOCATOR_OF(&alloc));
    for (int i = 0; i < 5; i++) {
        BitVecPush(&warm_a, (i % 2) == 0);
        BitVecPush(&warm_b, (i % 3) == 0);
    }
    (void)BitVecEditDistance(&warm_a, &warm_b);
    BitVecDeinit(&warm_a);
    BitVecDeinit(&warm_b);

    BitVec bv1 = BitVecInit(ALLOCATOR_OF(&alloc)); // empty
    BitVec bv2 = BitVecInit(ALLOCATOR_OF(&alloc));
    for (int i = 0; i < 5; i++) {
        BitVecPush(&bv2, true);
    }

    u64  dist   = BitVecEditDistance(&bv1, &bv2);
    bool result = (dist == 5);

    BitVecDeinit(&bv1);
    BitVecDeinit(&bv2);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Kills 1520:33 cxx_post_inc_to_post_dec -- the base-row loop counter `j++`
// becoming `j--` underflows after the j==0 write, so the loop exits having set
// only prev_row[0]; prev_row[1..len2] stay uninitialised. Same caller-
// observable handle as above: empty->len must be exactly len. The warm-up
// pre-soils the scratch so the unfilled cells are not coincidentally correct.
bool test_edit_distance_base_row_fully_filled(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVecEditDistance base row counter advances forward\n");

    // warm_b length == target bv2 length (4): freed scratch rows match the size
    // the empty->4 distance recycles, so the unfilled prev_row[len2] holds a
    // stale large value rather than a coincidental 4.
    BitVec warm_a = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVec warm_b = BitVecInit(ALLOCATOR_OF(&alloc));
    for (int i = 0; i < 4; i++) {
        BitVecPush(&warm_a, (i % 2) == 1);
        BitVecPush(&warm_b, (i % 2) == 0);
    }
    (void)BitVecEditDistance(&warm_a, &warm_b);
    BitVecDeinit(&warm_a);
    BitVecDeinit(&warm_b);

    BitVec bv1 = BitVecInit(ALLOCATOR_OF(&alloc)); // empty
    BitVec bv2 = BitVecInit(ALLOCATOR_OF(&alloc));
    for (int i = 0; i < 4; i++) {
        BitVecPush(&bv2, false);
    }

    u64  dist   = BitVecEditDistance(&bv1, &bv2);
    bool result = (dist == 4);

    BitVecDeinit(&bv1);
    BitVecDeinit(&bv2);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Kills 1531:43 cxx_sub_to_add -- the insertion candidate `curr_row[j - 1] + 1`
// becoming `curr_row[j + 1] + 1` reads the wrong (not-yet-computed / past-end)
// cell. For bv1 = {1} and bv2 = {1,1,1} the answer is reached purely by
// insertions: turn the single matching bit into three takes two insertions
// (distance 2). The real left-neighbour recurrence yields 2; the right-
// neighbour misread propagates a wrong (larger) candidate and changes it.
bool test_edit_distance_insertion_term(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVecEditDistance insertion term\n");

    BitVec bv1 = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVec bv2 = BitVecInit(ALLOCATOR_OF(&alloc));

    BitVecPush(&bv1, true);
    BitVecPush(&bv2, true);
    BitVecPush(&bv2, true);
    BitVecPush(&bv2, true);

    u64  dist   = BitVecEditDistance(&bv1, &bv2);
    bool result = (dist == 2);

    BitVecDeinit(&bv1);
    BitVecDeinit(&bv2);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Deadend: kills 1425:5 cxx_remove_void_call -- ValidateBitVec(bv2) in
// BitVecJaccardSimilarity. bv1 is a valid empty vector and bv2 is bad-magic
// with length 0, so the early `bv1->length == 0 && bv2->length == 0` guard
// returns 1.0 before line 1433's BitVecDotProduct would re-validate bv2. Only
// the dropped validate keeps real code aborting on the bad-magic bv2.
bool test_jaccard_rejects_bad_second_operand(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVecJaccardSimilarity rejects bad second operand\n");

    BitVec empty = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVec bad   = {0};

    BitVecJaccardSimilarity(&empty, &bad);

    BitVecDeinit(&empty);
    DefaultAllocatorDeinit(&alloc);
    return false;
}

// BitVecCorrelation: the read of bv1 is guarded by `i < bv1->length`. If that
// guard were `i <= bv1->length` (lt_to_le, line 1574) then for a bv1 shorter
// than bv2 the loop would read BitVecGet(bv1, bv1->length), which is out of
// bounds and aborts. On real code the call returns a finite correlation in
// [-1, 1] without aborting.
bool test_correlation_bv1_shorter_no_oob(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVecCorrelation with bv1 shorter than bv2 (no OOB)\n");

    BitVec bv1    = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVec bv2    = BitVecInit(ALLOCATOR_OF(&alloc));
    bool   result = true;

    // bv1 length 2, bv2 length 4 -> bv1 is the shorter operand.
    BitVecPush(&bv1, true);
    BitVecPush(&bv1, false);

    BitVecPush(&bv2, true);
    BitVecPush(&bv2, false);
    BitVecPush(&bv2, true);
    BitVecPush(&bv2, false);

    double correlation = BitVecCorrelation(&bv1, &bv2);
    result             = result && (correlation >= -1.0 && correlation <= 1.0);

    BitVecDeinit(&bv1);
    BitVecDeinit(&bv2);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Mirror of the above for the bv2 read guard `i < bv2->length` (lt_to_le, line
// 1575). With bv2 the shorter operand the mutated guard reads
// BitVecGet(bv2, bv2->length) and aborts; real code returns a finite value.
bool test_correlation_bv2_shorter_no_oob(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVecCorrelation with bv2 shorter than bv1 (no OOB)\n");

    BitVec bv1    = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVec bv2    = BitVecInit(ALLOCATOR_OF(&alloc));
    bool   result = true;

    // bv1 length 4, bv2 length 2 -> bv2 is the shorter operand.
    BitVecPush(&bv1, true);
    BitVecPush(&bv1, false);
    BitVecPush(&bv1, true);
    BitVecPush(&bv1, false);

    BitVecPush(&bv2, true);
    BitVecPush(&bv2, false);

    double correlation = BitVecCorrelation(&bv1, &bv2);
    result             = result && (correlation >= -1.0 && correlation <= 1.0);

    BitVecDeinit(&bv1);
    BitVecDeinit(&bv2);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// BitVecEntropy uses `-(p1*log2(p1) + p0*log2(p0))`. The existing entropy tests
// use a balanced vector (p1 == p0 == 0.5), where multiplying vs dividing by
// log2(0.5) == -1 are indistinguishable. An UNBALANCED vector (3 ones, 1 zero)
// gives p1=0.75, p0=0.25 and a Shannon entropy of ~0.81128. The mul_to_div
// mutation on the second term (line 1606) instead yields ~0.43628.
bool test_entropy_unbalanced_value_m8(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVecEntropy on an unbalanced (3 ones, 1 zero) vector\n");

    BitVec bv     = BitVecInit(ALLOCATOR_OF(&alloc));
    bool   result = true;

    BitVecPush(&bv, true);
    BitVecPush(&bv, true);
    BitVecPush(&bv, true);
    BitVecPush(&bv, false);

    double entropy = BitVecEntropy(&bv);
    // True value 0.811278; mutant value 0.436278 is well outside this band.
    result = result && (F64Abs(entropy - 0.811278) < 0.01);

    BitVecDeinit(&bv);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// BitVecBestAlignment slides bv2 across bv1 and returns the offset of the
// highest-scoring overlap. With bv1 = 0000 and bv2 = 11 every real overlap is a
// mismatch, so the best (least-negative) score belongs to offset 3, which has a
// single overlapping mismatch (score -1) versus -2 for the fuller overlaps. The
// real result is therefore offset 3. This single observable value kills the
// whole family of survivors:
//   - eq_to_ne on the empty-input guards (1631) -> would early-return 0
//   - init_const best_score=42 (1636)          -> no score beats 42 -> 0
//   - le_to_gt on the offset loop (1638)        -> loop never runs -> 0
//   - post_inc_to_post_dec on offset (1638)     -> only offset 0 tried -> 0
//   - init_const overlap=42 (1640)              -> phantom offset 4 wins -> 4
//   - init_const inner index i=42 (1642)        -> inner loop never runs -> 0
bool test_best_alignment_all_mismatch_offset(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVecBestAlignment all-mismatch best offset == 3\n");

    BitVec bv1    = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVec bv2    = BitVecInit(ALLOCATOR_OF(&alloc));
    bool   result = true;

    // bv1 = 0 0 0 0
    BitVecPush(&bv1, false);
    BitVecPush(&bv1, false);
    BitVecPush(&bv1, false);
    BitVecPush(&bv1, false);

    // bv2 = 1 1
    BitVecPush(&bv2, true);
    BitVecPush(&bv2, true);

    u64 best_offset = BitVecBestAlignment(&bv1, &bv2);
    result          = result && (best_offset == 3);

    BitVecDeinit(&bv1);
    BitVecDeinit(&bv2);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// sqrt_f64 via BitVecCosineSimilarity: `u.d = x;` (31:cxx_assign_const ->
// `u.d = 42`) makes the root always sqrt(42), so cosine of two identical
// vectors stops being 1.0. Caller-observable similarity value.
static bool test_cosine_similarity_identical_is_one(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVecCosineSimilarity of identical vectors == 1.0\n");

    BitVec bv1    = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVec bv2    = BitVecInit(ALLOCATOR_OF(&alloc));
    bool   result = true;

    // Four set bits each: dot=4, |a|=|b|=sqrt(4)=2, cosine = 4/(2*2) = 1.0.
    // With sqrt forced to sqrt(42), cosine collapses to 4/42 ~= 0.095.
    for (u64 i = 0; i < 4; i++) {
        BitVecPush(&bv1, true);
        BitVecPush(&bv2, true);
    }

    double similarity = BitVecCosineSimilarity(&bv1, &bv2);
    result            = result && (F64Abs(similarity - 1.0) < 0.001);

    BitVecDeinit(&bv1);
    BitVecDeinit(&bv2);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// log2_f64 via BitVecEntropy: `y = (m-1.0) / (m+1.0)` (61:cxx_div_to_mul)
// and `y2 = y * y;` (62:cxx_init_const -> `y2 = 42`) both corrupt the
// mantissa series, yielding a wrong log2 and hence a wrong entropy for an
// unbalanced distribution (mantissa != 1.0). Caller-observable value.
static bool test_entropy_unbalanced_value_m12(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVecEntropy of a 3:1 distribution\n");

    BitVec bv     = BitVecInit(ALLOCATOR_OF(&alloc));
    bool   result = true;

    // 3 ones, 1 zero -> p1=0.75, p0=0.25.
    // entropy = -(0.75*log2(0.75) + 0.25*log2(0.25)) ~= 0.8112782517.
    BitVecPush(&bv, true);
    BitVecPush(&bv, true);
    BitVecPush(&bv, true);
    BitVecPush(&bv, false);

    double entropy = BitVecEntropy(&bv);
    result         = result && (F64Abs(entropy - 0.8112782517394319) < 0.001);

    BitVecDeinit(&bv);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Kills 63:cxx_mul_to_div -- the mantissa series term `y2 * (...)` becoming
// `y2 / (...)` corrupts log2(x) for any x whose mantissa is not 1.0. A
// balanced (p=0.5) vector hits y=0 and hides the bug, so use an unbalanced
// 1-of-4 vector: Shannon entropy is 0.8112781245, which the real series
// reproduces to sub-0.001 but the mutant misses by ~0.045.
static bool test_entropy_unbalanced_value_m13(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *base  = ALLOCATOR_OF(&alloc);

    BitVec bv     = BitVecInit(base);
    bool   result = true;

    // 1000 -> one 1, three 0s : p1 = 0.25, p0 = 0.75.
    BitVecPush(&bv, true);
    BitVecPush(&bv, false);
    BitVecPush(&bv, false);
    BitVecPush(&bv, false);

    double entropy = BitVecEntropy(&bv);
    result         = result && (F64Abs(entropy - 0.8112781245) < 0.001);

    BitVecDeinit(&bv);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Real algorithm slides bv2 across bv1 scoring +1 per matching bit and -1 per
// mismatch, returning the offset of the maximum score (first on a tie).
//
// bv1 = 1011, bv2 = 11:
//   off0: 10 vs 11 -> 0   off1: 01 vs 11 -> 0
//   off2: 11 vs 11 -> +2  off3: 1  vs 1  -> +1
// Unique max is offset 2. Kills 1642:27 (lt_to_ge collapses the inner loop so
// nothing is ever scored -> returns 0) and 1642:73 (post_inc->post_dec scores
// only the first overlap bit, making offset 0 the earliest +1 -> returns 0).
bool test_best_alignment_offset(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    BitVec bv1 = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVec bv2 = BitVecInit(ALLOCATOR_OF(&alloc));
    push_bits(&bv1, "1011");
    push_bits(&bv2, "11");

    bool result = (BitVecBestAlignment(&bv1, &bv2) == 2);

    BitVecDeinit(&bv1);
    BitVecDeinit(&bv2);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// The empty-overlap offset (offset == bv1->length, score 0, overlap 0) must be
// ignored. bv1 = 11, bv2 = 000:
//   off0: 11 vs 00 -> -2   off1: 1 vs 0 -> -1   off2: overlap 0, score 0
// Real keeps the best real overlap -> offset 1. Mutant 1651:21 (overlap>0 ->
// overlap>=0) lets the zero-overlap offset 2 win with its bogus score 0.
bool test_best_alignment_empty_overlap_ignored(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    BitVec bv1 = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVec bv2 = BitVecInit(ALLOCATOR_OF(&alloc));
    push_bits(&bv1, "11");
    push_bits(&bv2, "000");

    bool result = (BitVecBestAlignment(&bv1, &bv2) == 1);

    BitVecDeinit(&bv1);
    BitVecDeinit(&bv2);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Tie-break: first offset achieving the max score wins. bv1 = 1010, bv2 = 10:
//   off0: 10 vs 10 -> +2   off2: 10 vs 10 -> +2 (tie)
// Real (>) keeps offset 0; mutant 1651:34 (gt_to_ge) keeps the last tie -> 2.
bool test_best_alignment_first_tie_wins(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    BitVec bv1 = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVec bv2 = BitVecInit(ALLOCATOR_OF(&alloc));
    push_bits(&bv1, "1010");
    push_bits(&bv2, "10");

    bool result = (BitVecBestAlignment(&bv1, &bv2) == 0);

    BitVecDeinit(&bv1);
    BitVecDeinit(&bv2);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Picks the maximum, not the minimum. bv1 = 0011, bv2 = 11:
//   off0: 00 vs 11 -> -2   off1: 01 vs 11 -> 0
//   off2: 11 vs 11 -> +2   off3: 1 vs 1 -> +1
// Real returns offset 2 (max). Mutant 1651:34 (gt_to_le) never improves on the
// INT32_MIN seed so best_offset is stuck at 0.
bool test_best_alignment_picks_max_not_min(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    BitVec bv1 = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVec bv2 = BitVecInit(ALLOCATOR_OF(&alloc));
    push_bits(&bv1, "0011");
    push_bits(&bv2, "11");

    bool result = (BitVecBestAlignment(&bv1, &bv2) == 2);

    BitVecDeinit(&bv1);
    BitVecDeinit(&bv2);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// best_score must track the real score. Same bv1 = 0011, bv2 = 11 (max at
// offset 2). Mutant 1652:25 (assign_const) sets best_score = 42 on the very
// first overlap (offset 0), so no later, genuinely-better offset can beat it
// and best_offset is frozen at 0.
bool test_best_alignment_uses_real_score(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    BitVec bv1 = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVec bv2 = BitVecInit(ALLOCATOR_OF(&alloc));
    push_bits(&bv1, "0011");
    push_bits(&bv2, "11");

    bool result = (BitVecBestAlignment(&bv1, &bv2) == 2);

    BitVecDeinit(&bv1);
    BitVecDeinit(&bv2);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

int main(void) {
    WriteFmt("[INFO] Starting BitVec.Math tests\n\n");

    // Array of normal test functions
    TestFunction tests[] = {
        test_bitvec_hamming_distance_basic,
        test_bitvec_hamming_distance_edge_cases,
        test_bitvec_jaccard_similarity_basic,
        test_bitvec_jaccard_similarity_edge_cases,
        test_bitvec_cosine_similarity_basic,
        test_bitvec_cosine_similarity_edge_cases,
        test_bitvec_dot_product_basic,
        test_bitvec_dot_product_edge_cases,
        test_bitvec_edit_distance_basic,
        test_bitvec_edit_distance_edge_cases,
        test_bitvec_correlation_basic,
        test_bitvec_correlation_edge_cases,
        test_bitvec_entropy_basic,
        test_bitvec_entropy_edge_cases,
        test_bitvec_alignment_score_basic,
        test_bitvec_alignment_score_edge_cases,
        test_bitvec_best_alignment_basic,
        test_bitvec_best_alignment_edge_cases,
        test_bitvec_math_stress_tests,
        test_jaccard_guard_fires_only_when_both_empty,
        test_jaccard_bv1_shorter_no_abort,
        test_jaccard_bv2_shorter_no_abort,
        test_jaccard_equal_length_uses_bv2_bits,
        test_edit_distance_col0_base_case,
        test_edit_distance_deletion_term,
        test_edit_distance_empty_to_len_base_row,
        test_edit_distance_base_row_fully_filled,
        test_edit_distance_insertion_term,
        test_correlation_bv1_shorter_no_oob,
        test_correlation_bv2_shorter_no_oob,
        test_entropy_unbalanced_value_m8,
        test_best_alignment_all_mismatch_offset,
        test_cosine_similarity_identical_is_one,
        test_entropy_unbalanced_value_m12,
        test_entropy_unbalanced_value_m13,
        test_best_alignment_offset,
        test_best_alignment_empty_overlap_ignored,
        test_best_alignment_first_tie_wins,
        test_best_alignment_picks_max_not_min,
        test_best_alignment_uses_real_score
    };

    // Array of deadend test functions
    TestFunction deadend_tests[] = {
        test_bitvec_hamming_distance_null_bv1,
        test_bitvec_hamming_distance_null_bv2,
        test_bitvec_jaccard_similarity_null_bv1,
        test_bitvec_jaccard_similarity_null_bv2,
        test_bitvec_cosine_similarity_null_bv1,
        test_bitvec_cosine_similarity_null_bv2,
        test_bitvec_dot_product_null_bv1,
        test_bitvec_dot_product_null_bv2,
        test_bitvec_edit_distance_null_bv1,
        test_bitvec_edit_distance_null_bv2,
        test_bitvec_correlation_null_bv1,
        test_bitvec_correlation_null_bv2,
        test_bitvec_entropy_null,
        test_bitvec_alignment_score_null_bv1,
        test_bitvec_alignment_score_null_bv2,
        test_bitvec_best_alignment_null_bv1,
        test_bitvec_best_alignment_null_bv2,
        test_jaccard_rejects_bad_second_operand
    };

    int total_tests         = sizeof(tests) / sizeof(tests[0]);
    int total_deadend_tests = sizeof(deadend_tests) / sizeof(deadend_tests[0]);

    // Run all tests using the centralized test driver
    return run_test_suite(tests, total_tests, deadend_tests, total_deadend_tests, "BitVec.Math");
}
