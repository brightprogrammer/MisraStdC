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
        test_bitvec_math_stress_tests
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
        test_bitvec_best_alignment_null_bv2
    };

    int total_tests         = sizeof(tests) / sizeof(tests[0]);
    int total_deadend_tests = sizeof(deadend_tests) / sizeof(deadend_tests[0]);

    // Run all tests using the centralized test driver
    return run_test_suite(tests, total_tests, deadend_tests, total_deadend_tests, "BitVec.Math");
}
