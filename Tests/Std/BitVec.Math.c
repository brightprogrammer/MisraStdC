#include <Misra/Std/Container/Bits.h>
#include <Misra/Std/Log.h>
#include <stdio.h>
#include <Misra/Types.h>
#include <math.h>
#include <float.h>

// Include test utilities
#include "../Util/TestRunner.h"

// Function prototypes for all Math tests
bool test_Bits_hamming_distance_basic(void);
bool test_Bits_hamming_distance_edge_cases(void);
bool test_Bits_jaccard_similarity_basic(void);
bool test_Bits_jaccard_similarity_edge_cases(void);
bool test_Bits_cosine_similarity_basic(void);
bool test_Bits_cosine_similarity_edge_cases(void);
bool test_Bits_dot_product_basic(void);
bool test_Bits_dot_product_edge_cases(void);
bool test_Bits_edit_distance_basic(void);
bool test_Bits_edit_distance_edge_cases(void);
bool test_Bits_correlation_basic(void);
bool test_Bits_correlation_edge_cases(void);
bool test_Bits_entropy_basic(void);
bool test_Bits_entropy_edge_cases(void);
bool test_Bits_alignment_score_basic(void);
bool test_Bits_alignment_score_edge_cases(void);
bool test_Bits_best_alignment_basic(void);
bool test_Bits_best_alignment_edge_cases(void);
bool test_Bits_math_stress_tests(void);

// Deadend tests
bool test_Bits_hamming_distance_null_bv1(void);
bool test_Bits_hamming_distance_null_bv2(void);
bool test_Bits_jaccard_similarity_null_bv1(void);
bool test_Bits_jaccard_similarity_null_bv2(void);
bool test_Bits_cosine_similarity_null_bv1(void);
bool test_Bits_cosine_similarity_null_bv2(void);
bool test_Bits_dot_product_null_bv1(void);
bool test_Bits_dot_product_null_bv2(void);
bool test_Bits_edit_distance_null_bv1(void);
bool test_Bits_edit_distance_null_bv2(void);
bool test_Bits_correlation_null_bv1(void);
bool test_Bits_correlation_null_bv2(void);
bool test_Bits_entropy_null(void);
bool test_Bits_alignment_score_null_bv1(void);
bool test_Bits_alignment_score_null_bv2(void);
bool test_Bits_best_alignment_null_bv1(void);
bool test_Bits_best_alignment_null_bv2(void);

// Test BitsHammingDistance basic functionality
bool test_Bits_hamming_distance_basic(void) {
    printf("Testing BitsHammingDistance basic functionality\n");

    Bits bv1    = BitsInit();
    Bits bv2    = BitsInit();
    bool result = true;

    // Test identical Bitstors
    BitsPush(&bv1, true);
    BitsPush(&bv1, false);
    BitsPush(&bv1, true);
    BitsPush(&bv2, true);
    BitsPush(&bv2, false);
    BitsPush(&bv2, true);

    u64 distance = BitsHammingDistance(&bv1, &bv2);
    result       = result && (distance == 0);

    // Test completely different Bitstors
    BitsClear(&bv2);
    BitsPush(&bv2, false);
    BitsPush(&bv2, true);
    BitsPush(&bv2, false);

    distance = BitsHammingDistance(&bv1, &bv2);
    result   = result && (distance == 3);

    // Test partially different Bitstors
    BitsClear(&bv2);
    BitsPush(&bv2, true); // Same
    BitsPush(&bv2, true); // Different
    BitsPush(&bv2, true); // Same

    distance = BitsHammingDistance(&bv1, &bv2);
    result   = result && (distance == 1);

    BitsDeinit(&bv1);
    BitsDeinit(&bv2);
    return result;
}

// Test BitsHammingDistance edge cases
bool test_Bits_hamming_distance_edge_cases(void) {
    printf("Testing BitsHammingDistance edge cases\n");

    Bits bv1    = BitsInit();
    Bits bv2    = BitsInit();
    bool result = true;

    // Test empty Bitstors
    u64 distance = BitsHammingDistance(&bv1, &bv2);
    result       = result && (distance == 0);

    // Test different lengths
    BitsPush(&bv1, true);
    BitsPush(&bv1, false);
    BitsPush(&bv2, true);

    distance = BitsHammingDistance(&bv1, &bv2);
    result   = result && (distance == 1); // 1 length difference

    // Test one empty, one non-empty
    BitsClear(&bv2);
    distance = BitsHammingDistance(&bv1, &bv2);
    result   = result && (distance == 2); // Length of bv1

    BitsDeinit(&bv1);
    BitsDeinit(&bv2);
    return result;
}

// Test BitsJaccardSimilarity basic functionality
bool test_Bits_jaccard_similarity_basic(void) {
    printf("Testing BitsJaccardSimilarity basic functionality\n");

    Bits bv1    = BitsInit();
    Bits bv2    = BitsInit();
    bool result = true;

    // Test identical Bitstors
    BitsPush(&bv1, true);
    BitsPush(&bv1, false);
    BitsPush(&bv1, true);
    BitsPush(&bv2, true);
    BitsPush(&bv2, false);
    BitsPush(&bv2, true);

    double similarity = BitsJaccardSimilarity(&bv1, &bv2);
    result            = result && (fabs(similarity - 1.0) < 0.001);

    // Test no overlap
    BitsClear(&bv1);
    BitsClear(&bv2);
    BitsPush(&bv1, true);
    BitsPush(&bv1, false);
    BitsPush(&bv2, false);
    BitsPush(&bv2, true);

    similarity = BitsJaccardSimilarity(&bv1, &bv2);
    result     = result && (fabs(similarity - 0.0) < 0.001);

    // Test partial overlap
    BitsClear(&bv1);
    BitsClear(&bv2);
    BitsPush(&bv1, true);
    BitsPush(&bv1, true);
    BitsPush(&bv1, false);
    BitsPush(&bv2, true);
    BitsPush(&bv2, false);
    BitsPush(&bv2, false);
    // Intersection: 1, Union: 2, Jaccard = 1/2 = 0.5

    similarity = BitsJaccardSimilarity(&bv1, &bv2);
    result     = result && (fabs(similarity - 0.5) < 0.001);

    BitsDeinit(&bv1);
    BitsDeinit(&bv2);
    return result;
}

// Test BitsJaccardSimilarity edge cases
bool test_Bits_jaccard_similarity_edge_cases(void) {
    printf("Testing BitsJaccardSimilarity edge cases\n");

    Bits bv1    = BitsInit();
    Bits bv2    = BitsInit();
    bool result = true;

    // Test empty Bitstors
    double similarity = BitsJaccardSimilarity(&bv1, &bv2);
    result            = result && (fabs(similarity - 1.0) < 0.001);

    // Test all zeros
    BitsPush(&bv1, false);
    BitsPush(&bv1, false);
    BitsPush(&bv2, false);
    BitsPush(&bv2, false);

    similarity = BitsJaccardSimilarity(&bv1, &bv2);
    result     = result && (fabs(similarity - 1.0) < 0.001);

    BitsDeinit(&bv1);
    BitsDeinit(&bv2);
    return result;
}

// Test BitsCosineSimilarity basic functionality
bool test_Bits_cosine_similarity_basic(void) {
    printf("Testing BitsCosineSimilarity basic functionality\n");

    Bits bv1    = BitsInit();
    Bits bv2    = BitsInit();
    bool result = true;

    // Test identical Bitstors
    BitsPush(&bv1, true);
    BitsPush(&bv1, false);
    BitsPush(&bv1, true);
    BitsPush(&bv2, true);
    BitsPush(&bv2, false);
    BitsPush(&bv2, true);

    double similarity = BitsCosineSimilarity(&bv1, &bv2);
    result            = result && (fabs(similarity - 1.0) < 0.001);

    // Test orthogonal vectors
    BitsClear(&bv1);
    BitsClear(&bv2);
    BitsPush(&bv1, true);
    BitsPush(&bv1, false);
    BitsPush(&bv2, false);
    BitsPush(&bv2, true);

    similarity = BitsCosineSimilarity(&bv1, &bv2);
    result     = result && (fabs(similarity - 0.0) < 0.001);

    BitsDeinit(&bv1);
    BitsDeinit(&bv2);
    return result;
}

// Test BitsCosineSimilarity edge cases
bool test_Bits_cosine_similarity_edge_cases(void) {
    printf("Testing BitsCosineSimilarity edge cases\n");

    Bits bv1    = BitsInit();
    Bits bv2    = BitsInit();
    bool result = true;

    // Test zero vectors
    BitsPush(&bv1, false);
    BitsPush(&bv1, false);
    BitsPush(&bv2, false);
    BitsPush(&bv2, false);

    double similarity = BitsCosineSimilarity(&bv1, &bv2);
    result            = result && (similarity == 0.0);

    // Test one zero vector
    BitsClear(&bv2);
    BitsPush(&bv2, true);
    BitsPush(&bv2, false);

    similarity = BitsCosineSimilarity(&bv1, &bv2);
    result     = result && (similarity == 0.0);

    BitsDeinit(&bv1);
    BitsDeinit(&bv2);
    return result;
}

// Test BitsDotProduct basic functionality
bool test_Bits_dot_product_basic(void) {
    printf("Testing BitsDotProduct basic functionality\n");

    Bits bv1    = BitsInit();
    Bits bv2    = BitsInit();
    bool result = true;

    // Test basic dot product
    BitsPush(&bv1, true);
    BitsPush(&bv1, false);
    BitsPush(&bv1, true);
    BitsPush(&bv1, true);
    BitsPush(&bv2, true);
    BitsPush(&bv2, true);
    BitsPush(&bv2, false);
    BitsPush(&bv2, true);

    u64 product = BitsDotProduct(&bv1, &bv2);
    result      = result && (product == 2); // Positions 0 and 3

    // Test no overlap
    BitsClear(&bv1);
    BitsClear(&bv2);
    BitsPush(&bv1, true);
    BitsPush(&bv1, false);
    BitsPush(&bv2, false);
    BitsPush(&bv2, true);

    product = BitsDotProduct(&bv1, &bv2);
    result  = result && (product == 0);

    BitsDeinit(&bv1);
    BitsDeinit(&bv2);
    return result;
}

// Test BitsDotProduct edge cases
bool test_Bits_dot_product_edge_cases(void) {
    printf("Testing BitsDotProduct edge cases\n");

    Bits bv1    = BitsInit();
    Bits bv2    = BitsInit();
    bool result = true;

    // Test empty Bitstors
    u64 product = BitsDotProduct(&bv1, &bv2);
    result      = result && (product == 0);

    // Test different lengths
    BitsPush(&bv1, true);
    BitsPush(&bv1, true);
    BitsPush(&bv1, true);
    BitsPush(&bv2, true);
    BitsPush(&bv2, false);

    product = BitsDotProduct(&bv1, &bv2);
    result  = result && (product == 1); // Only first position counts

    BitsDeinit(&bv1);
    BitsDeinit(&bv2);
    return result;
}

// Test BitsEditDistance basic functionality
bool test_Bits_edit_distance_basic(void) {
    printf("Testing BitsEditDistance basic functionality\n");

    Bits bv1    = BitsInit();
    Bits bv2    = BitsInit();
    bool result = true;

    // Test identical strings
    BitsPush(&bv1, true);
    BitsPush(&bv1, false);
    BitsPush(&bv1, true);
    BitsPush(&bv2, true);
    BitsPush(&bv2, false);
    BitsPush(&bv2, true);

    u64 distance = BitsEditDistance(&bv1, &bv2);
    result       = result && (distance == 0);

    // Test single substitution
    BitsClear(&bv2);
    BitsPush(&bv2, true);
    BitsPush(&bv2, true); // Changed from false
    BitsPush(&bv2, true);

    distance = BitsEditDistance(&bv1, &bv2);
    result   = result && (distance == 1);

    // Test insertion
    BitsClear(&bv2);
    BitsPush(&bv2, true);
    BitsPush(&bv2, false);
    BitsPush(&bv2, true);
    BitsPush(&bv2, false); // Extra bit

    distance = BitsEditDistance(&bv1, &bv2);
    result   = result && (distance == 1);

    BitsDeinit(&bv1);
    BitsDeinit(&bv2);
    return result;
}

// Test BitsEditDistance edge cases
bool test_Bits_edit_distance_edge_cases(void) {
    printf("Testing BitsEditDistance edge cases\n");

    Bits bv1    = BitsInit();
    Bits bv2    = BitsInit();
    bool result = true;

    // Test empty to empty
    u64 distance = BitsEditDistance(&bv1, &bv2);
    result       = result && (distance == 0);

    // Test empty to non-empty
    BitsPush(&bv2, true);
    BitsPush(&bv2, false);

    distance = BitsEditDistance(&bv1, &bv2);
    result   = result && (distance == 2);

    // Test non-empty to empty
    distance = BitsEditDistance(&bv2, &bv1);
    result   = result && (distance == 2);

    BitsDeinit(&bv1);
    BitsDeinit(&bv2);
    return result;
}

// Test BitsCorrelation basic functionality
bool test_Bits_correlation_basic(void) {
    printf("Testing BitsCorrelation basic functionality\n");

    Bits bv1    = BitsInit();
    Bits bv2    = BitsInit();
    bool result = true;

    // Test perfect correlation
    BitsPush(&bv1, true);
    BitsPush(&bv1, false);
    BitsPush(&bv1, true);
    BitsPush(&bv1, false);
    BitsPush(&bv2, true);
    BitsPush(&bv2, false);
    BitsPush(&bv2, true);
    BitsPush(&bv2, false);

    double correlation = BitsCorrelation(&bv1, &bv2);
    result             = result && (fabs(correlation - 1.0) < 0.001);

    // Test perfect anti-correlation
    BitsClear(&bv2);
    BitsPush(&bv2, false);
    BitsPush(&bv2, true);
    BitsPush(&bv2, false);
    BitsPush(&bv2, true);

    correlation = BitsCorrelation(&bv1, &bv2);
    result      = result && (fabs(correlation + 1.0) < 0.001);

    BitsDeinit(&bv1);
    BitsDeinit(&bv2);
    return result;
}

// Test BitsCorrelation edge cases
bool test_Bits_correlation_edge_cases(void) {
    printf("Testing BitsCorrelation edge cases\n");

    Bits bv1    = BitsInit();
    Bits bv2    = BitsInit();
    bool result = true;

    // Test empty Bitstors
    double correlation = BitsCorrelation(&bv1, &bv2);
    result             = result && (correlation == 1.0);

    // Test uniform vectors
    BitsPush(&bv1, true);
    BitsPush(&bv1, true);
    BitsPush(&bv1, true);
    BitsPush(&bv2, true);
    BitsPush(&bv2, true);
    BitsPush(&bv2, true);

    correlation = BitsCorrelation(&bv1, &bv2);
    result      = result && (correlation == 0.0); // No variance

    BitsDeinit(&bv1);
    BitsDeinit(&bv2);
    return result;
}

// Test BitsEntropy basic functionality
bool test_Bits_entropy_basic(void) {
    printf("Testing BitsEntropy basic functionality\n");

    Bits bv     = BitsInit();
    bool result = true;

    // Test maximum entropy (equal 0s and 1s)
    BitsPush(&bv, true);
    BitsPush(&bv, false);
    BitsPush(&bv, true);
    BitsPush(&bv, false);

    double entropy = BitsEntropy(&bv);
    result         = result && (fabs(entropy - 1.0) < 0.001);

    // Test minimum entropy (all same)
    BitsClear(&bv);
    BitsPush(&bv, true);
    BitsPush(&bv, true);
    BitsPush(&bv, true);

    entropy = BitsEntropy(&bv);
    result  = result && (entropy == 0.0);

    BitsDeinit(&bv);
    return result;
}

// Test BitsEntropy edge cases
bool test_Bits_entropy_edge_cases(void) {
    printf("Testing BitsEntropy edge cases\n");

    Bits bv     = BitsInit();
    bool result = true;

    // Test empty Bitstor
    double entropy = BitsEntropy(&bv);
    result         = result && (entropy == 0.0);

    // Test single bit
    BitsPush(&bv, true);
    entropy = BitsEntropy(&bv);
    result  = result && (entropy == 0.0);

    BitsDeinit(&bv);
    return result;
}

// Test BitsAlignmentScore basic functionality
bool test_Bits_alignment_score_basic(void) {
    printf("Testing BitsAlignmentScore basic functionality\n");

    Bits bv1    = BitsInit();
    Bits bv2    = BitsInit();
    bool result = true;

    // Test perfect match
    BitsPush(&bv1, true);
    BitsPush(&bv1, false);
    BitsPush(&bv1, true);
    BitsPush(&bv2, true);
    BitsPush(&bv2, false);
    BitsPush(&bv2, true);

    int score = BitsAlignmentScore(&bv1, &bv2, 2, -1);
    result    = result && (score == 6); // 3 matches * 2

    // Test perfect mismatch
    BitsClear(&bv2);
    BitsPush(&bv2, false);
    BitsPush(&bv2, true);
    BitsPush(&bv2, false);

    score  = BitsAlignmentScore(&bv1, &bv2, 2, -1);
    result = result && (score == -3); // 3 mismatches * -1

    BitsDeinit(&bv1);
    BitsDeinit(&bv2);
    return result;
}

// Test BitsAlignmentScore edge cases
bool test_Bits_alignment_score_edge_cases(void) {
    printf("Testing BitsAlignmentScore edge cases\n");

    Bits bv1    = BitsInit();
    Bits bv2    = BitsInit();
    bool result = true;

    // Test empty Bitstors
    int score = BitsAlignmentScore(&bv1, &bv2, 1, -1);
    result    = result && (score == 0);

    // Test different lengths (only overlapping region scored)
    BitsPush(&bv1, true);
    BitsPush(&bv1, false);
    BitsPush(&bv1, true);
    BitsPush(&bv2, true);
    BitsPush(&bv2, false);

    score  = BitsAlignmentScore(&bv1, &bv2, 1, -1);
    result = result && (score == 2); // 2 matches

    BitsDeinit(&bv1);
    BitsDeinit(&bv2);
    return result;
}

// Test BitsBestAlignment basic functionality
bool test_Bits_best_alignment_basic(void) {
    printf("Testing BitsBestAlignment basic functionality\n");

    Bits bv1    = BitsInit();
    Bits bv2    = BitsInit();
    bool result = true;

    // Create bv1: 1100110
    BitsPush(&bv1, true);
    BitsPush(&bv1, true);
    BitsPush(&bv1, false);
    BitsPush(&bv1, false);
    BitsPush(&bv1, true);
    BitsPush(&bv1, true);
    BitsPush(&bv1, false);

    // Create bv2: 110 (should match at position 0 and 4)
    BitsPush(&bv2, true);
    BitsPush(&bv2, true);
    BitsPush(&bv2, false);

    u64 best_pos = BitsBestAlignment(&bv1, &bv2);
    result       = result && (best_pos == 0 || best_pos == 4);

    BitsDeinit(&bv1);
    BitsDeinit(&bv2);
    return result;
}

// Test BitsBestAlignment edge cases
bool test_Bits_best_alignment_edge_cases(void) {
    printf("Testing BitsBestAlignment edge cases\n");

    Bits bv1    = BitsInit();
    Bits bv2    = BitsInit();
    bool result = true;

    // Test empty Bitstors
    u64 best_pos = BitsBestAlignment(&bv1, &bv2);
    result       = result && (best_pos == 0);

    // Test bv2 longer than bv1
    BitsPush(&bv1, true);
    BitsPush(&bv2, true);
    BitsPush(&bv2, false);

    best_pos = BitsBestAlignment(&bv1, &bv2);
    result   = result && (best_pos == 0);

    BitsDeinit(&bv1);
    BitsDeinit(&bv2);
    return result;
}

// Stress test for Math functions
bool test_Bits_math_stress_tests(void) {
    printf("Testing Bits Math stress tests\n");

    Bits bv1    = BitsInit();
    Bits bv2    = BitsInit();
    bool result = true;

    // Create large Bitstors
    for (int i = 0; i < 1000; i++) {
        BitsPush(&bv1, i % 2 == 0);
        BitsPush(&bv2, i % 3 == 0);
    }

    // Test that all functions complete without crashing
    u64    hamming     = BitsHammingDistance(&bv1, &bv2);
    double jaccard     = BitsJaccardSimilarity(&bv1, &bv2);
    double cosine      = BitsCosineSimilarity(&bv1, &bv2);
    u64    dot_prod    = BitsDotProduct(&bv1, &bv2);
    double correlation = BitsCorrelation(&bv1, &bv2);
    double entropy1    = BitsEntropy(&bv1);
    int    align_score = BitsAlignmentScore(&bv1, &bv2, 1, -1);
    u64    best_align  = BitsBestAlignment(&bv1, &bv2);

    // Test edit distance with smaller vectors (expensive operation)
    Bits small1 = BitsInit();
    Bits small2 = BitsInit();
    for (int i = 0; i < 50; i++) {
        BitsPush(&small1, i % 2 == 0);
        BitsPush(&small2, i % 3 == 0);
    }
    u64 edit_dist = BitsEditDistance(&small1, &small2);

    result = result && (hamming < 1000);
    result = result && (jaccard >= 0.0 && jaccard <= 1.0);
    result = result && (cosine >= 0.0 && cosine <= 1.0);
    result = result && (dot_prod < 1000);
    result = result && (correlation >= -1.0 && correlation <= 1.0);
    result = result && (entropy1 >= 0.0 && entropy1 <= 1.0);
    // Note: align_score can be 0 for some patterns, so just check it's computed
    result = result && (align_score >= -1000 && align_score <= 1000);
    result = result && (best_align <= 1000); // SIZE_MAX is valid
    result = result && (edit_dist < 100);

    BitsDeinit(&bv1);
    BitsDeinit(&bv2);
    BitsDeinit(&small1);
    BitsDeinit(&small2);
    return result;
}

// Deadend tests - each function with NULL parameters

bool test_Bits_hamming_distance_null_bv1(void) {
    printf("Testing BitsHammingDistance(NULL, bv2) - should fatal\n");
    Bits bv2 = BitsInit();
    BitsPush(&bv2, true);
    BitsHammingDistance(NULL, &bv2);
    BitsDeinit(&bv2);
    return true;
}

bool test_Bits_hamming_distance_null_bv2(void) {
    printf("Testing BitsHammingDistance(bv1, NULL) - should fatal\n");
    Bits bv1 = BitsInit();
    BitsPush(&bv1, true);
    BitsHammingDistance(&bv1, NULL);
    BitsDeinit(&bv1);
    return true;
}

bool test_Bits_jaccard_similarity_null_bv1(void) {
    printf("Testing BitsJaccardSimilarity(NULL, bv2) - should fatal\n");
    Bits bv2 = BitsInit();
    BitsPush(&bv2, true);
    BitsJaccardSimilarity(NULL, &bv2);
    BitsDeinit(&bv2);
    return true;
}

bool test_Bits_jaccard_similarity_null_bv2(void) {
    printf("Testing BitsJaccardSimilarity(bv1, NULL) - should fatal\n");
    Bits bv1 = BitsInit();
    BitsPush(&bv1, true);
    BitsJaccardSimilarity(&bv1, NULL);
    BitsDeinit(&bv1);
    return true;
}

bool test_Bits_cosine_similarity_null_bv1(void) {
    printf("Testing BitsCosineSimilarity(NULL, bv2) - should fatal\n");
    Bits bv2 = BitsInit();
    BitsPush(&bv2, true);
    BitsCosineSimilarity(NULL, &bv2);
    BitsDeinit(&bv2);
    return true;
}

bool test_Bits_cosine_similarity_null_bv2(void) {
    printf("Testing BitsCosineSimilarity(bv1, NULL) - should fatal\n");
    Bits bv1 = BitsInit();
    BitsPush(&bv1, true);
    BitsCosineSimilarity(&bv1, NULL);
    BitsDeinit(&bv1);
    return true;
}

bool test_Bits_dot_product_null_bv1(void) {
    printf("Testing BitsDotProduct(NULL, bv2) - should fatal\n");
    Bits bv2 = BitsInit();
    BitsPush(&bv2, true);
    BitsDotProduct(NULL, &bv2);
    BitsDeinit(&bv2);
    return true;
}

bool test_Bits_dot_product_null_bv2(void) {
    printf("Testing BitsDotProduct(bv1, NULL) - should fatal\n");
    Bits bv1 = BitsInit();
    BitsPush(&bv1, true);
    BitsDotProduct(&bv1, NULL);
    BitsDeinit(&bv1);
    return true;
}

bool test_Bits_edit_distance_null_bv1(void) {
    printf("Testing BitsEditDistance(NULL, bv2) - should fatal\n");
    Bits bv2 = BitsInit();
    BitsPush(&bv2, true);
    BitsEditDistance(NULL, &bv2);
    BitsDeinit(&bv2);
    return true;
}

bool test_Bits_edit_distance_null_bv2(void) {
    printf("Testing BitsEditDistance(bv1, NULL) - should fatal\n");
    Bits bv1 = BitsInit();
    BitsPush(&bv1, true);
    BitsEditDistance(&bv1, NULL);
    BitsDeinit(&bv1);
    return true;
}

bool test_Bits_correlation_null_bv1(void) {
    printf("Testing BitsCorrelation(NULL, bv2) - should fatal\n");
    Bits bv2 = BitsInit();
    BitsPush(&bv2, true);
    BitsCorrelation(NULL, &bv2);
    BitsDeinit(&bv2);
    return true;
}

bool test_Bits_correlation_null_bv2(void) {
    printf("Testing BitsCorrelation(bv1, NULL) - should fatal\n");
    Bits bv1 = BitsInit();
    BitsPush(&bv1, true);
    BitsCorrelation(&bv1, NULL);
    BitsDeinit(&bv1);
    return true;
}

bool test_Bits_entropy_null(void) {
    printf("Testing BitsEntropy(NULL) - should fatal\n");
    BitsEntropy(NULL);
    return true;
}

bool test_Bits_alignment_score_null_bv1(void) {
    printf("Testing BitsAlignmentScore(NULL, bv2, 1, -1) - should fatal\n");
    Bits bv2 = BitsInit();
    BitsPush(&bv2, true);
    BitsAlignmentScore(NULL, &bv2, 1, -1);
    BitsDeinit(&bv2);
    return true;
}

bool test_Bits_alignment_score_null_bv2(void) {
    printf("Testing BitsAlignmentScore(bv1, NULL, 1, -1) - should fatal\n");
    Bits bv1 = BitsInit();
    BitsPush(&bv1, true);
    BitsAlignmentScore(&bv1, NULL, 1, -1);
    BitsDeinit(&bv1);
    return true;
}

bool test_Bits_best_alignment_null_bv1(void) {
    printf("Testing BitsBestAlignment(NULL, bv2) - should fatal\n");
    Bits bv2 = BitsInit();
    BitsPush(&bv2, true);
    BitsBestAlignment(NULL, &bv2);
    BitsDeinit(&bv2);
    return true;
}

bool test_Bits_best_alignment_null_bv2(void) {
    printf("Testing BitsBestAlignment(bv1, NULL) - should fatal\n");
    Bits bv1 = BitsInit();
    BitsPush(&bv1, true);
    BitsBestAlignment(&bv1, NULL);
    BitsDeinit(&bv1);
    return true;
}

// Main function that runs all tests
int main(void) {
    printf("[INFO] Starting Bits.Math tests\n\n");

    // Array of normal test functions
    TestFunction tests[] = {
        test_Bits_hamming_distance_basic,
        test_Bits_hamming_distance_edge_cases,
        test_Bits_jaccard_similarity_basic,
        test_Bits_jaccard_similarity_edge_cases,
        test_Bits_cosine_similarity_basic,
        test_Bits_cosine_similarity_edge_cases,
        test_Bits_dot_product_basic,
        test_Bits_dot_product_edge_cases,
        test_Bits_edit_distance_basic,
        test_Bits_edit_distance_edge_cases,
        test_Bits_correlation_basic,
        test_Bits_correlation_edge_cases,
        test_Bits_entropy_basic,
        test_Bits_entropy_edge_cases,
        test_Bits_alignment_score_basic,
        test_Bits_alignment_score_edge_cases,
        test_Bits_best_alignment_basic,
        test_Bits_best_alignment_edge_cases,
        test_Bits_math_stress_tests
    };

    // Array of deadend test functions
    TestFunction deadend_tests[] = {
        test_Bits_hamming_distance_null_bv1,
        test_Bits_hamming_distance_null_bv2,
        test_Bits_jaccard_similarity_null_bv1,
        test_Bits_jaccard_similarity_null_bv2,
        test_Bits_cosine_similarity_null_bv1,
        test_Bits_cosine_similarity_null_bv2,
        test_Bits_dot_product_null_bv1,
        test_Bits_dot_product_null_bv2,
        test_Bits_edit_distance_null_bv1,
        test_Bits_edit_distance_null_bv2,
        test_Bits_correlation_null_bv1,
        test_Bits_correlation_null_bv2,
        test_Bits_entropy_null,
        test_Bits_alignment_score_null_bv1,
        test_Bits_alignment_score_null_bv2,
        test_Bits_best_alignment_null_bv1,
        test_Bits_best_alignment_null_bv2
    };

    int total_tests         = sizeof(tests) / sizeof(tests[0]);
    int total_deadend_tests = sizeof(deadend_tests) / sizeof(deadend_tests[0]);

    // Run all tests using the centralized test driver
    return run_test_suite(tests, total_tests, deadend_tests, total_deadend_tests, "Bits.Math");
}
