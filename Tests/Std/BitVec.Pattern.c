#include <Misra/Std/Container/BitVec.h>
#include <Misra/Std/Log.h>

#include <stdio.h>
#include <Misra/Types.h>

// Include test utilities
#include "../Util/TestRunner.h"

// Function prototypes
bool test_bitvec_basic_pattern_functions(void);
bool test_bitvec_find_pattern(void);
bool test_bitvec_find_last_pattern(void);
bool test_bitvec_find_all_pattern(void);
bool test_bitvec_pattern_edge_cases(void);
bool test_bitvec_pattern_stress_tests(void);
bool test_bitvec_find_pattern_null_source(void);
bool test_bitvec_find_pattern_null_pattern(void);
bool test_bitvec_find_last_pattern_null_source(void);
bool test_bitvec_find_last_pattern_null_pattern(void);
bool test_bitvec_find_all_pattern_null_source(void);
bool test_bitvec_find_all_pattern_null_pattern(void);
bool test_bitvec_find_all_pattern_null_results(void);
bool test_bitvec_find_all_pattern_zero_max_results(void);
bool test_bitvec_starts_with_basic(void);
bool test_bitvec_starts_with_edge_cases(void);
bool test_bitvec_ends_with_basic(void);
bool test_bitvec_ends_with_edge_cases(void);
bool test_bitvec_contains_basic(void);
bool test_bitvec_contains_at_basic(void);
bool test_bitvec_contains_at_edge_cases(void);
bool test_bitvec_count_pattern_basic(void);
bool test_bitvec_rfind_pattern_basic(void);
bool test_bitvec_replace_basic(void);
bool test_bitvec_replace_all_basic(void);
bool test_bitvec_matches_basic(void);
bool test_bitvec_fuzzy_match_basic(void);
bool test_bitvec_regex_match_basic(void);
bool test_bitvec_prefix_match_basic(void);
bool test_bitvec_suffix_match_basic(void);
bool test_bitvec_starts_with_null_source(void);
bool test_bitvec_starts_with_null_prefix(void);
bool test_bitvec_ends_with_null_source(void);
bool test_bitvec_ends_with_null_suffix(void);
bool test_bitvec_contains_at_null_source(void);
bool test_bitvec_contains_at_null_pattern(void);
bool test_bitvec_replace_null_source(void);
bool test_bitvec_matches_null_source(void);
bool test_bitvec_regex_match_null_source(void);
bool test_bitvec_regex_match_null_pattern(void);
bool test_bitvec_prefix_match_null_source(void);
bool test_bitvec_prefix_match_null_patterns(void);
bool test_bitvec_suffix_match_null_source(void);
bool test_bitvec_suffix_match_null_patterns(void);

// Test basic pattern matching functions
bool test_bitvec_basic_pattern_functions(void) {
    WriteFmt("Testing basic BitVec pattern functions\n");

    BitVec source  = BitVecInit();
    BitVec pattern = BitVecInit();
    bool   result  = true;

    // Create source: 11010011101
    BitVecPush(&source, true);  // 0
    BitVecPush(&source, true);  // 1
    BitVecPush(&source, false); // 2
    BitVecPush(&source, true);  // 3
    BitVecPush(&source, false); // 4
    BitVecPush(&source, false); // 5
    BitVecPush(&source, true);  // 6
    BitVecPush(&source, true);  // 7
    BitVecPush(&source, true);  // 8
    BitVecPush(&source, false); // 9
    BitVecPush(&source, true);  // 10

    // Create pattern: 101
    BitVecPush(&pattern, true);
    BitVecPush(&pattern, false);
    BitVecPush(&pattern, true);

    // Test basic pattern finding
    u64 index = BitVecFindPattern(&source, &pattern);
    result    = result && (index == 1); // Should find pattern at index 1 (101...)

    BitVecDeinit(&source);
    BitVecDeinit(&pattern);
    return result;
}

// Test BitVecFindPattern function comprehensively
bool test_bitvec_find_pattern(void) {
    WriteFmt("Testing BitVecFindPattern function\n");

    BitVec source  = BitVecInit();
    BitVec pattern = BitVecInit();
    bool   result  = true;

    // Create source: 110101011010
    for (int i = 0; i < 12; i++) {
        bool bit = (i % 4 == 0 || i % 4 == 1 || i % 4 == 3);
        BitVecPush(&source, bit);
    }

    // Pattern 1: 101 (should find multiple occurrences)
    BitVecPush(&pattern, true);
    BitVecPush(&pattern, false);
    BitVecPush(&pattern, true);

    u64 index = BitVecFindPattern(&source, &pattern);
    result    = result && (index == 1); // First occurrence at index 1

    // Pattern 2: Single bit pattern
    BitVecClear(&pattern);
    BitVecPush(&pattern, true);
    index  = BitVecFindPattern(&source, &pattern);
    result = result && (index == 0); // First true bit at index 0

    // Pattern 3: Non-existent pattern
    BitVecClear(&pattern);
    BitVecPush(&pattern, false);
    BitVecPush(&pattern, false);
    BitVecPush(&pattern, false);
    index  = BitVecFindPattern(&source, &pattern);
    result = result && (index == SIZE_MAX); // Should not find three consecutive falses

    // Pattern 4: Pattern at the end
    BitVecClear(&pattern);
    BitVecPush(&pattern, true);
    BitVecPush(&pattern, false);
    // Depending on the exact source pattern, adjust this test

    BitVecDeinit(&source);
    BitVecDeinit(&pattern);
    return result;
}

// Test BitVecFindLastPattern function
bool test_bitvec_find_last_pattern(void) {
    WriteFmt("Testing BitVecFindLastPattern function\n");

    BitVec source  = BitVecInit();
    BitVec pattern = BitVecInit();
    bool   result  = true;

    // Create source with multiple pattern occurrences: 101010101
    for (int i = 0; i < 9; i++) {
        BitVecPush(&source, i % 2 == 0);
    }

    // Pattern: 10
    BitVecPush(&pattern, true);
    BitVecPush(&pattern, false);

    u64 index = BitVecFindLastPattern(&source, &pattern);
    result    = result && (index == 6); // Last occurrence of "10" should be at index 6

    // Test with pattern that occurs only once
    BitVecClear(&source);
    BitVecClear(&pattern);

    // Source: 110011
    BitVecPush(&source, true);
    BitVecPush(&source, true);
    BitVecPush(&source, false);
    BitVecPush(&source, false);
    BitVecPush(&source, true);
    BitVecPush(&source, true);

    // Pattern: 001
    BitVecPush(&pattern, false);
    BitVecPush(&pattern, false);
    BitVecPush(&pattern, true);

    index  = BitVecFindLastPattern(&source, &pattern);
    result = result && (index == 2); // Only occurrence at index 2

    BitVecDeinit(&source);
    BitVecDeinit(&pattern);
    return result;
}

// Test BitVecFindAllPattern function
bool test_bitvec_find_all_pattern(void) {
    WriteFmt("Testing BitVecFindAllPattern function\n");

    BitVec source  = BitVecInit();
    BitVec pattern = BitVecInit();
    bool   result  = true;

    // Create source: 10101010101
    for (int i = 0; i < 11; i++) {
        BitVecPush(&source, i % 2 == 0);
    }

    // Pattern: 101
    BitVecPush(&pattern, true);
    BitVecPush(&pattern, false);
    BitVecPush(&pattern, true);

    size results[10];
    u64  count = BitVecFindAllPattern(&source, &pattern, results, 10);

    // Should find pattern at indices 0, 2, 4, 6, 8
    result = result && (count == 5);
    if (count >= 5) {
        result = result && (results[0] == 0);
        result = result && (results[1] == 2);
        result = result && (results[2] == 4);
        result = result && (results[3] == 6);
        result = result && (results[4] == 8);
    }

    // Test with limited results array
    u64 count_limited = BitVecFindAllPattern(&source, &pattern, results, 3);
    result            = result && (count_limited == 3); // Should only return first 3 matches

    // Test with non-overlapping pattern
    BitVecClear(&source);
    BitVecClear(&pattern);

    // Source: 110110110
    for (int i = 0; i < 9; i++) {
        bool bit = (i % 3 == 0 || i % 3 == 1);
        BitVecPush(&source, bit);
    }

    // Pattern: 110
    BitVecPush(&pattern, true);
    BitVecPush(&pattern, true);
    BitVecPush(&pattern, false);

    count  = BitVecFindAllPattern(&source, &pattern, results, 10);
    result = result && (count == 3); // Should find at 0, 3, 6

    BitVecDeinit(&source);
    BitVecDeinit(&pattern);
    return result;
}

// Test edge cases for pattern functions
bool test_bitvec_pattern_edge_cases(void) {
    WriteFmt("Testing BitVec pattern edge cases\n");

    BitVec source  = BitVecInit();
    BitVec pattern = BitVecInit();
    bool   result  = true;

    // Test empty pattern
    BitVecPush(&source, true);
    BitVecPush(&source, false);
    u64 index = BitVecFindPattern(&source, &pattern); // Empty pattern
    result    = result && (index == SIZE_MAX);

    // Test pattern longer than source
    BitVecClear(&pattern);
    for (int i = 0; i < 5; i++) {
        BitVecPush(&pattern, true);
    }
    index  = BitVecFindPattern(&source, &pattern); // Pattern length 5, source length 2
    result = result && (index == SIZE_MAX);

    // Test empty source
    BitVecClear(&source);
    BitVecClear(&pattern);
    BitVecPush(&pattern, true);
    index  = BitVecFindPattern(&source, &pattern);
    result = result && (index == SIZE_MAX);

    // Test exact match (pattern same as source)
    BitVecClear(&source);
    BitVecClear(&pattern);
    BitVecPush(&source, true);
    BitVecPush(&source, false);
    BitVecPush(&source, true);
    BitVecPush(&pattern, true);
    BitVecPush(&pattern, false);
    BitVecPush(&pattern, true);
    index  = BitVecFindPattern(&source, &pattern);
    result = result && (index == 0);

    // Test single bit source and pattern
    BitVecClear(&source);
    BitVecClear(&pattern);
    BitVecPush(&source, true);
    BitVecPush(&pattern, true);
    index  = BitVecFindPattern(&source, &pattern);
    result = result && (index == 0);

    BitVecPush(&pattern, false); // Now pattern is longer than source
    index  = BitVecFindPattern(&source, &pattern);
    result = result && (index == SIZE_MAX);

    // Test BitVecFindAllPattern edge cases
    size results[1];
    BitVecClear(&source);
    BitVecClear(&pattern);
    BitVecPush(&source, true);
    BitVecPush(&pattern, true);

    u64 count = BitVecFindAllPattern(&source, &pattern, results, 1);
    result    = result && (count == 1);
    result    = result && (results[0] == 0);

    BitVecDeinit(&source);
    BitVecDeinit(&pattern);
    return result;
}

// Stress tests with large data
bool test_bitvec_pattern_stress_tests(void) {
    WriteFmt("Testing BitVec pattern stress tests\n");

    BitVec source  = BitVecInit();
    BitVec pattern = BitVecInit();
    bool   result  = true;

    // Create large source with known pattern
    for (int i = 0; i < 10000; i++) {
        // Create pattern "1010" every 100 bits, rest are "1100"
        if (i % 100 == 0) {
            BitVecPush(&source, true); // Start of 1010 pattern
            BitVecPush(&source, false);
            BitVecPush(&source, true);
            BitVecPush(&source, false);
            i += 3; // Skip next 3 iterations
        } else {
            BitVecPush(&source, i % 2 == 0);
        }
    }

    // Pattern: 1010
    BitVecPush(&pattern, true);
    BitVecPush(&pattern, false);
    BitVecPush(&pattern, true);
    BitVecPush(&pattern, false);

    u64 index = BitVecFindPattern(&source, &pattern);
    result    = result && (index == 0); // Should find first pattern at beginning

    // Test finding all occurrences in large data
    size results[200];
    u64  count = BitVecFindAllPattern(&source, &pattern, results, 200);
    result     = result && (count > 0); // Should find at least some patterns

    BitVecDeinit(&source);
    BitVecDeinit(&pattern);
    return result;
}

// Deadend test 1: BitVecFindPattern with NULL source
bool test_bitvec_find_pattern_null_source(void) {
    WriteFmt("Testing BitVecFindPattern(NULL, pattern) - should fatal\n");

    BitVec pattern = BitVecInit();
    BitVecPush(&pattern, true);

    BitVecFindPattern(NULL, &pattern); // Should cause LOG_FATAL

    BitVecDeinit(&pattern);
    return true;
}

// Deadend test 2: BitVecFindPattern with NULL pattern
bool test_bitvec_find_pattern_null_pattern(void) {
    WriteFmt("Testing BitVecFindPattern(source, NULL) - should fatal\n");

    BitVec source = BitVecInit();
    BitVecPush(&source, true);
    BitVecPush(&source, false);

    BitVecFindPattern(&source, NULL); // Should cause LOG_FATAL

    BitVecDeinit(&source);
    return true;
}

// Deadend test 3: BitVecFindLastPattern with NULL source
bool test_bitvec_find_last_pattern_null_source(void) {
    WriteFmt("Testing BitVecFindLastPattern(NULL, pattern) - should fatal\n");

    BitVec pattern = BitVecInit();
    BitVecPush(&pattern, true);

    BitVecFindLastPattern(NULL, &pattern); // Should cause LOG_FATAL

    BitVecDeinit(&pattern);
    return true;
}

// Deadend test 4: BitVecFindLastPattern with NULL pattern
bool test_bitvec_find_last_pattern_null_pattern(void) {
    WriteFmt("Testing BitVecFindLastPattern(source, NULL) - should fatal\n");

    BitVec source = BitVecInit();
    BitVecPush(&source, true);
    BitVecPush(&source, false);

    BitVecFindLastPattern(&source, NULL); // Should cause LOG_FATAL

    BitVecDeinit(&source);
    return true;
}

// Deadend test 5: BitVecFindAllPattern with NULL source
bool test_bitvec_find_all_pattern_null_source(void) {
    WriteFmt("Testing BitVecFindAllPattern(NULL, pattern, results, 10) - should fatal\n");

    size results[10];

    // Don't create pattern BitVec since we're testing NULL source validation
    BitVecFindAllPattern(NULL, (BitVec *)0x1, results, 10); // Should cause LOG_FATAL

    return true;
}

// Deadend test 6: BitVecFindAllPattern with NULL pattern
bool test_bitvec_find_all_pattern_null_pattern(void) {
    WriteFmt("Testing BitVecFindAllPattern(source, NULL, results, 10) - should fatal\n");

    BitVec source = BitVecInit();
    size   results[10];
    BitVecPush(&source, true);
    BitVecPush(&source, false);

    BitVecFindAllPattern(&source, NULL, results, 10); // Should cause LOG_FATAL

    BitVecDeinit(&source);
    return true;
}

// Deadend test 7: BitVecFindAllPattern with NULL results
bool test_bitvec_find_all_pattern_null_results(void) {
    WriteFmt("Testing BitVecFindAllPattern(source, pattern, NULL, 10) - should fatal\n");

    BitVec source  = BitVecInit();
    BitVec pattern = BitVecInit();
    BitVecPush(&source, true);
    BitVecPush(&source, false);
    BitVecPush(&pattern, true);

    BitVecFindAllPattern(&source, &pattern, NULL, 10); // Should cause LOG_FATAL

    BitVecDeinit(&source);
    BitVecDeinit(&pattern);
    return true;
}

// Deadend test 8: BitVecFindAllPattern with zero max_results
bool test_bitvec_find_all_pattern_zero_max_results(void) {
    WriteFmt("Testing BitVecFindAllPattern(source, pattern, results, 0) - should fatal\n");

    BitVec source  = BitVecInit();
    BitVec pattern = BitVecInit();
    size   results[10];
    BitVecPush(&source, true);
    BitVecPush(&source, false);
    BitVecPush(&pattern, true);

    BitVecFindAllPattern(&source, &pattern, results, 0); // Should cause LOG_FATAL

    BitVecDeinit(&source);
    BitVecDeinit(&pattern);
    return true;
}

// Tests for missing Pattern functions

// BitVecStartsWith tests
bool test_bitvec_starts_with_basic(void) {
    WriteFmt("Testing BitVecStartsWith basic functionality\n");

    BitVec source = BitVecInit();
    BitVec prefix = BitVecInit();
    bool   result = true;

    // Create source: 110101
    BitVecPush(&source, true);
    BitVecPush(&source, true);
    BitVecPush(&source, false);
    BitVecPush(&source, true);
    BitVecPush(&source, false);
    BitVecPush(&source, true);

    // Test prefix: 110
    BitVecPush(&prefix, true);
    BitVecPush(&prefix, true);
    BitVecPush(&prefix, false);

    result = result && BitVecStartsWith(&source, &prefix);

    // Test non-matching prefix: 101
    BitVecClear(&prefix);
    BitVecPush(&prefix, true);
    BitVecPush(&prefix, false);
    BitVecPush(&prefix, true);

    result = result && !BitVecStartsWith(&source, &prefix);

    BitVecDeinit(&source);
    BitVecDeinit(&prefix);
    return result;
}

bool test_bitvec_starts_with_edge_cases(void) {
    WriteFmt("Testing BitVecStartsWith edge cases\n");

    BitVec source = BitVecInit();
    BitVec prefix = BitVecInit();
    bool   result = true;

    // Test empty prefix (should always match)
    BitVecPush(&source, true);
    result = result && BitVecStartsWith(&source, &prefix);

    // Test prefix longer than source
    BitVecClear(&source);
    BitVecPush(&prefix, true);
    BitVecPush(&prefix, false);
    result = result && !BitVecStartsWith(&source, &prefix);

    // Test equal length
    BitVecClear(&source);
    BitVecClear(&prefix);
    BitVecPush(&source, true);
    BitVecPush(&source, false);
    BitVecPush(&prefix, true);
    BitVecPush(&prefix, false);
    result = result && BitVecStartsWith(&source, &prefix);

    BitVecDeinit(&source);
    BitVecDeinit(&prefix);
    return result;
}

// BitVecEndsWith tests
bool test_bitvec_ends_with_basic(void) {
    WriteFmt("Testing BitVecEndsWith basic functionality\n");

    BitVec source = BitVecInit();
    BitVec suffix = BitVecInit();
    bool   result = true;

    // Create source: 110101
    BitVecPush(&source, true);
    BitVecPush(&source, true);
    BitVecPush(&source, false);
    BitVecPush(&source, true);
    BitVecPush(&source, false);
    BitVecPush(&source, true);

    // Test suffix: 101
    BitVecPush(&suffix, true);
    BitVecPush(&suffix, false);
    BitVecPush(&suffix, true);

    result = result && BitVecEndsWith(&source, &suffix);

    // Test non-matching suffix: 110
    BitVecClear(&suffix);
    BitVecPush(&suffix, true);
    BitVecPush(&suffix, true);
    BitVecPush(&suffix, false);

    result = result && !BitVecEndsWith(&source, &suffix);

    BitVecDeinit(&source);
    BitVecDeinit(&suffix);
    return result;
}

bool test_bitvec_ends_with_edge_cases(void) {
    WriteFmt("Testing BitVecEndsWith edge cases\n");

    BitVec source = BitVecInit();
    BitVec suffix = BitVecInit();
    bool   result = true;

    // Test empty suffix (should always match)
    BitVecPush(&source, true);
    result = result && BitVecEndsWith(&source, &suffix);

    // Test suffix longer than source
    BitVecClear(&source);
    BitVecPush(&suffix, true);
    BitVecPush(&suffix, false);
    result = result && !BitVecEndsWith(&source, &suffix);

    BitVecDeinit(&source);
    BitVecDeinit(&suffix);
    return result;
}

// BitVecContains tests
bool test_bitvec_contains_basic(void) {
    WriteFmt("Testing BitVecContains basic functionality\n");

    BitVec source  = BitVecInit();
    BitVec pattern = BitVecInit();
    bool   result  = true;

    // Create source: 1101011
    BitVecPush(&source, true);
    BitVecPush(&source, true);
    BitVecPush(&source, false);
    BitVecPush(&source, true);
    BitVecPush(&source, false);
    BitVecPush(&source, true);
    BitVecPush(&source, true);

    // Test pattern: 101 (exists at positions 2 and 4)
    BitVecPush(&pattern, true);
    BitVecPush(&pattern, false);
    BitVecPush(&pattern, true);

    result = result && BitVecContains(&source, &pattern);

    // Test non-existing pattern: 000
    BitVecClear(&pattern);
    BitVecPush(&pattern, false);
    BitVecPush(&pattern, false);
    BitVecPush(&pattern, false);

    result = result && !BitVecContains(&source, &pattern);

    BitVecDeinit(&source);
    BitVecDeinit(&pattern);
    return result;
}

// BitVecContainsAt tests
bool test_bitvec_contains_at_basic(void) {
    WriteFmt("Testing BitVecContainsAt basic functionality\n");

    BitVec source  = BitVecInit();
    BitVec pattern = BitVecInit();
    bool   result  = true;

    // Create source: 1101011
    BitVecPush(&source, true);
    BitVecPush(&source, true);
    BitVecPush(&source, false);
    BitVecPush(&source, true);
    BitVecPush(&source, false);
    BitVecPush(&source, true);
    BitVecPush(&source, true);

    // Test pattern: 101
    BitVecPush(&pattern, true);
    BitVecPush(&pattern, false);
    BitVecPush(&pattern, true);

    result = result && BitVecContainsAt(&source, &pattern, 1);  // Should match
    result = result && !BitVecContainsAt(&source, &pattern, 0); // Should not match
    result = result && BitVecContainsAt(&source, &pattern, 3);  // Should match

    BitVecDeinit(&source);
    BitVecDeinit(&pattern);
    return result;
}

bool test_bitvec_contains_at_edge_cases(void) {
    WriteFmt("Testing BitVecContainsAt edge cases\n");

    BitVec source  = BitVecInit();
    BitVec pattern = BitVecInit();
    bool   result  = true;

    // Create small source
    BitVecPush(&source, true);
    BitVecPush(&source, false);

    // Test pattern that extends beyond source
    BitVecPush(&pattern, true);
    BitVecPush(&pattern, false);
    BitVecPush(&pattern, true);

    result = result && !BitVecContainsAt(&source, &pattern, 0); // Pattern too long

    BitVecDeinit(&source);
    BitVecDeinit(&pattern);
    return result;
}

// BitVecCountPattern tests
bool test_bitvec_count_pattern_basic(void) {
    WriteFmt("Testing BitVecCountPattern basic functionality\n");

    BitVec source  = BitVecInit();
    BitVec pattern = BitVecInit();
    bool   result  = true;

    // Create source: 101010101
    for (int i = 0; i < 9; i++) {
        BitVecPush(&source, i % 2 == 0);
    }

    // Test pattern: 101 (should find 4 occurrences at 0, 2, 4, 6)
    BitVecPush(&pattern, true);
    BitVecPush(&pattern, false);
    BitVecPush(&pattern, true);

    u64 count = BitVecCountPattern(&source, &pattern);
    result    = result && (count == 4);

    // Test pattern: 010 (should find 3 occurrences at 1, 3, 5)
    BitVecClear(&pattern);
    BitVecPush(&pattern, false);
    BitVecPush(&pattern, true);
    BitVecPush(&pattern, false);

    count  = BitVecCountPattern(&source, &pattern);
    result = result && (count == 3);

    BitVecDeinit(&source);
    BitVecDeinit(&pattern);
    return result;
}

// BitVecRFindPattern tests
bool test_bitvec_rfind_pattern_basic(void) {
    WriteFmt("Testing BitVecRFindPattern basic functionality\n");

    BitVec source  = BitVecInit();
    BitVec pattern = BitVecInit();
    bool   result  = true;

    // Create source: 101101101
    BitVecPush(&source, true);
    BitVecPush(&source, false);
    BitVecPush(&source, true);
    BitVecPush(&source, true);
    BitVecPush(&source, false);
    BitVecPush(&source, true);
    BitVecPush(&source, true);
    BitVecPush(&source, false);
    BitVecPush(&source, true);

    // Test pattern: 101
    BitVecPush(&pattern, true);
    BitVecPush(&pattern, false);
    BitVecPush(&pattern, true);

    // Search from index 8 backwards
    u64 pos = BitVecRFindPattern(&source, &pattern, 8);
    result  = result && (pos == 6); // Should find at position 6

    // Search from index 5 backwards
    pos    = BitVecRFindPattern(&source, &pattern, 5);
    result = result && (pos == 3); // Should find at position 3

    BitVecDeinit(&source);
    BitVecDeinit(&pattern);
    return result;
}

// BitVecReplace tests
bool test_bitvec_replace_basic(void) {
    WriteFmt("Testing BitVecReplace basic functionality\n");

    BitVec source      = BitVecInit();
    BitVec old_pattern = BitVecInit();
    BitVec new_pattern = BitVecInit();
    bool   result      = true;

    // Create source: 110110
    BitVecPush(&source, true);
    BitVecPush(&source, true);
    BitVecPush(&source, false);
    BitVecPush(&source, true);
    BitVecPush(&source, true);
    BitVecPush(&source, false);

    // Old pattern: 110
    BitVecPush(&old_pattern, true);
    BitVecPush(&old_pattern, true);
    BitVecPush(&old_pattern, false);

    // New pattern: 101
    BitVecPush(&new_pattern, true);
    BitVecPush(&new_pattern, false);
    BitVecPush(&new_pattern, true);

    bool replaced = BitVecReplace(&source, &old_pattern, &new_pattern);
    result        = result && replaced;

    // Check result should be: 101110
    result = result && (BitVecLen(&source) == 6);
    result = result && (BitVecGet(&source, 0) == true);
    result = result && (BitVecGet(&source, 1) == false);
    result = result && (BitVecGet(&source, 2) == true);

    BitVecDeinit(&source);
    BitVecDeinit(&old_pattern);
    BitVecDeinit(&new_pattern);
    return result;
}

// BitVecReplaceAll tests
bool test_bitvec_replace_all_basic(void) {
    WriteFmt("Testing BitVecReplaceAll basic functionality\n");

    BitVec source      = BitVecInit();
    BitVec old_pattern = BitVecInit();
    BitVec new_pattern = BitVecInit();
    bool   result      = true;

    // Create source: 110110110
    for (int i = 0; i < 3; i++) {
        BitVecPush(&source, true);
        BitVecPush(&source, true);
        BitVecPush(&source, false);
    }

    // Old pattern: 110
    BitVecPush(&old_pattern, true);
    BitVecPush(&old_pattern, true);
    BitVecPush(&old_pattern, false);

    // New pattern: 01
    BitVecPush(&new_pattern, false);
    BitVecPush(&new_pattern, true);

    u64 replacements = BitVecReplaceAll(&source, &old_pattern, &new_pattern);
    result           = result && (replacements == 3);

    // Check final length
    result = result && (BitVecLen(&source) == 6); // 3 * 2 = 6

    BitVecDeinit(&source);
    BitVecDeinit(&old_pattern);
    BitVecDeinit(&new_pattern);
    return result;
}

// BitVecMatches tests
bool test_bitvec_matches_basic(void) {
    WriteFmt("Testing BitVecMatches basic functionality\n");

    BitVec source   = BitVecInit();
    BitVec pattern  = BitVecInit();
    BitVec wildcard = BitVecInit();
    bool   result   = true;

    // Create source: 1101
    BitVecPush(&source, true);
    BitVecPush(&source, true);
    BitVecPush(&source, false);
    BitVecPush(&source, true);

    // Create pattern: 1?01 (where ? can be anything)
    BitVecPush(&pattern, true);
    BitVecPush(&pattern, false); // This will be wildcarded
    BitVecPush(&pattern, false);
    BitVecPush(&pattern, true);

    // Create wildcard: 0100 (1 means wildcard, 0 means must match exactly)
    BitVecPush(&wildcard, false);
    BitVecPush(&wildcard, true); // Wildcard position
    BitVecPush(&wildcard, false);
    BitVecPush(&wildcard, false);

    result = result && BitVecMatches(&source, &pattern, &wildcard);

    BitVecDeinit(&source);
    BitVecDeinit(&pattern);
    BitVecDeinit(&wildcard);
    return result;
}

// BitVecFuzzyMatch tests
bool test_bitvec_fuzzy_match_basic(void) {
    WriteFmt("Testing BitVecFuzzyMatch basic functionality\n");

    BitVec source  = BitVecInit();
    BitVec pattern = BitVecInit();
    bool   result  = true;

    // Create source: 110100111
    BitVecPush(&source, true);
    BitVecPush(&source, true);
    BitVecPush(&source, false);
    BitVecPush(&source, true);
    BitVecPush(&source, false);
    BitVecPush(&source, false);
    BitVecPush(&source, true);
    BitVecPush(&source, true);
    BitVecPush(&source, true);

    // Create pattern: 111 (should match at position 6 with 0 errors)
    BitVecPush(&pattern, true);
    BitVecPush(&pattern, true);
    BitVecPush(&pattern, true);

    u64 pos = BitVecFuzzyMatch(&source, &pattern, 0);
    result  = result && (pos == 6);

    // Test with 1 error allowed
    pos    = BitVecFuzzyMatch(&source, &pattern, 1);
    result = result && (pos == 0); // Should match 110 with 1 error

    BitVecDeinit(&source);
    BitVecDeinit(&pattern);
    return result;
}

// BitVecRegexMatch tests
bool test_bitvec_regex_match_basic(void) {
    WriteFmt("Testing BitVecRegexMatch basic functionality\n");

    BitVec source = BitVecInit();
    bool   result = true;

    // Create source: 101010
    BitVecPush(&source, true);
    BitVecPush(&source, false);
    BitVecPush(&source, true);
    BitVecPush(&source, false);
    BitVecPush(&source, true);
    BitVecPush(&source, false);

    // Test simple substring match
    result = result && BitVecRegexMatch(&source, "101");
    result = result && !BitVecRegexMatch(&source, "111");

    BitVecDeinit(&source);
    return result;
}

// BitVecPrefixMatch tests
bool test_bitvec_prefix_match_basic(void) {
    WriteFmt("Testing BitVecPrefixMatch basic functionality\n");

    BitVec source = BitVecInit();
    BitVec patterns[3];
    bool   result = true;

    // Initialize patterns
    for (int i = 0; i < 3; i++) {
        patterns[i] = BitVecInit();
    }

    // Create source: 110101
    BitVecPush(&source, true);
    BitVecPush(&source, true);
    BitVecPush(&source, false);
    BitVecPush(&source, true);
    BitVecPush(&source, false);
    BitVecPush(&source, true);

    // Pattern 0: 111 (should not match)
    BitVecPush(&patterns[0], true);
    BitVecPush(&patterns[0], true);
    BitVecPush(&patterns[0], true);

    // Pattern 1: 110 (should match)
    BitVecPush(&patterns[1], true);
    BitVecPush(&patterns[1], true);
    BitVecPush(&patterns[1], false);

    // Pattern 2: 101 (should not match as prefix)
    BitVecPush(&patterns[2], true);
    BitVecPush(&patterns[2], false);
    BitVecPush(&patterns[2], true);

    u64 match_idx = BitVecPrefixMatch(&source, patterns, 3);
    result        = result && (match_idx == 1);

    for (int i = 0; i < 3; i++) {
        BitVecDeinit(&patterns[i]);
    }
    BitVecDeinit(&source);
    return result;
}

// BitVecSuffixMatch tests
bool test_bitvec_suffix_match_basic(void) {
    WriteFmt("Testing BitVecSuffixMatch basic functionality\n");

    BitVec source = BitVecInit();
    BitVec patterns[3];
    bool   result = true;

    // Initialize patterns
    for (int i = 0; i < 3; i++) {
        patterns[i] = BitVecInit();
    }

    // Create source: 110101
    BitVecPush(&source, true);
    BitVecPush(&source, true);
    BitVecPush(&source, false);
    BitVecPush(&source, true);
    BitVecPush(&source, false);
    BitVecPush(&source, true);

    // Pattern 0: 111 (should not match)
    BitVecPush(&patterns[0], true);
    BitVecPush(&patterns[0], true);
    BitVecPush(&patterns[0], true);

    // Pattern 1: 101 (should match as suffix)
    BitVecPush(&patterns[1], true);
    BitVecPush(&patterns[1], false);
    BitVecPush(&patterns[1], true);

    // Pattern 2: 110 (should not match as suffix)
    BitVecPush(&patterns[2], true);
    BitVecPush(&patterns[2], true);
    BitVecPush(&patterns[2], false);

    u64 match_idx = BitVecSuffixMatch(&source, patterns, 3);
    result        = result && (match_idx == 1);

    for (int i = 0; i < 3; i++) {
        BitVecDeinit(&patterns[i]);
    }
    BitVecDeinit(&source);
    return result;
}

// Additional deadend tests for missing Pattern functions

bool test_bitvec_starts_with_null_source(void) {
    WriteFmt("Testing BitVecStartsWith(NULL, prefix) - should fatal\n");
    BitVec prefix = BitVecInit();
    BitVecPush(&prefix, true);
    BitVecStartsWith(NULL, &prefix);
    BitVecDeinit(&prefix);
    return true;
}

bool test_bitvec_starts_with_null_prefix(void) {
    WriteFmt("Testing BitVecStartsWith(source, NULL) - should fatal\n");
    BitVec source = BitVecInit();
    BitVecPush(&source, true);
    BitVecStartsWith(&source, NULL);
    BitVecDeinit(&source);
    return true;
}

bool test_bitvec_ends_with_null_source(void) {
    WriteFmt("Testing BitVecEndsWith(NULL, suffix) - should fatal\n");
    BitVec suffix = BitVecInit();
    BitVecPush(&suffix, true);
    BitVecEndsWith(NULL, &suffix);
    BitVecDeinit(&suffix);
    return true;
}

bool test_bitvec_ends_with_null_suffix(void) {
    WriteFmt("Testing BitVecEndsWith(source, NULL) - should fatal\n");
    BitVec source = BitVecInit();
    BitVecPush(&source, true);
    BitVecEndsWith(&source, NULL);
    BitVecDeinit(&source);
    return true;
}

bool test_bitvec_contains_at_null_source(void) {
    WriteFmt("Testing BitVecContainsAt(NULL, pattern, 0) - should fatal\n");
    BitVec pattern = BitVecInit();
    BitVecPush(&pattern, true);
    BitVecContainsAt(NULL, &pattern, 0);
    BitVecDeinit(&pattern);
    return true;
}

bool test_bitvec_contains_at_null_pattern(void) {
    WriteFmt("Testing BitVecContainsAt(source, NULL, 0) - should fatal\n");
    BitVec source = BitVecInit();
    BitVecPush(&source, true);
    BitVecContainsAt(&source, NULL, 0);
    BitVecDeinit(&source);
    return true;
}

bool test_bitvec_replace_null_source(void) {
    WriteFmt("Testing BitVecReplace(NULL, old, new) - should fatal\n");

    // Don't create BitVecs since we're testing NULL source validation
    BitVecReplace(NULL, (BitVec *)0x1, (BitVec *)0x1);
    return true;
}

bool test_bitvec_matches_null_source(void) {
    WriteFmt("Testing BitVecMatches(NULL, pattern, wildcard) - should fatal\n");
    BitVec pattern  = BitVecInit();
    BitVec wildcard = BitVecInit();
    BitVecPush(&pattern, true);
    BitVecPush(&wildcard, false);
    BitVecMatches(NULL, &pattern, &wildcard);
    BitVecDeinit(&pattern);
    BitVecDeinit(&wildcard);
    return true;
}

bool test_bitvec_regex_match_null_source(void) {
    WriteFmt("Testing BitVecRegexMatch(NULL, pattern) - should fatal\n");
    BitVecRegexMatch(NULL, "101");
    return true;
}

bool test_bitvec_regex_match_null_pattern(void) {
    WriteFmt("Testing BitVecRegexMatch(source, NULL) - should fatal\n");
    BitVec source = BitVecInit();
    BitVecPush(&source, true);
    BitVecRegexMatch(&source, NULL);
    BitVecDeinit(&source);
    return true;
}

bool test_bitvec_prefix_match_null_source(void) {
    WriteFmt("Testing BitVecPrefixMatch(NULL, patterns, 1) - should fatal\n");
    BitVec patterns[1] = {BitVecInit()};
    BitVecPush(&patterns[0], true);
    BitVecPrefixMatch(NULL, patterns, 1);
    BitVecDeinit(&patterns[0]);
    return true;
}

bool test_bitvec_prefix_match_null_patterns(void) {
    WriteFmt("Testing BitVecPrefixMatch(source, NULL, 1) - should fatal\n");
    BitVec source = BitVecInit();
    BitVecPush(&source, true);
    BitVecPrefixMatch(&source, NULL, 1);
    BitVecDeinit(&source);
    return true;
}

bool test_bitvec_suffix_match_null_source(void) {
    WriteFmt("Testing BitVecSuffixMatch(NULL, patterns, 1) - should fatal\n");
    BitVec patterns[1] = {BitVecInit()};
    BitVecPush(&patterns[0], true);
    BitVecSuffixMatch(NULL, patterns, 1);
    BitVecDeinit(&patterns[0]);
    return true;
}

bool test_bitvec_suffix_match_null_patterns(void) {
    WriteFmt("Testing BitVecSuffixMatch(source, NULL, 1) - should fatal\n");
    BitVec source = BitVecInit();
    BitVecPush(&source, true);
    BitVecSuffixMatch(&source, NULL, 1);
    BitVecDeinit(&source);
    return true;
}

// Main function that runs all tests
int main(void) {
    WriteFmt("[INFO] Starting BitVec.Pattern tests\n\n");

    // Array of test functions
    TestFunction tests[] = {
        test_bitvec_basic_pattern_functions,
        test_bitvec_find_pattern,
        test_bitvec_find_last_pattern,
        test_bitvec_find_all_pattern,
        test_bitvec_pattern_edge_cases,
        test_bitvec_pattern_stress_tests,
        // New Pattern function tests
        test_bitvec_starts_with_basic,
        test_bitvec_starts_with_edge_cases,
        test_bitvec_ends_with_basic,
        test_bitvec_ends_with_edge_cases,
        test_bitvec_contains_basic,
        test_bitvec_contains_at_basic,
        test_bitvec_contains_at_edge_cases,
        test_bitvec_count_pattern_basic,
        test_bitvec_rfind_pattern_basic,
        test_bitvec_replace_basic,
        test_bitvec_replace_all_basic,
        test_bitvec_matches_basic,
        test_bitvec_fuzzy_match_basic,
        test_bitvec_regex_match_basic,
        test_bitvec_prefix_match_basic,
        test_bitvec_suffix_match_basic
    };

    // Deadend tests that would cause program termination
    TestFunction deadend_tests[] = {
        test_bitvec_find_pattern_null_source,
        test_bitvec_find_pattern_null_pattern,
        test_bitvec_find_last_pattern_null_source,
        test_bitvec_find_last_pattern_null_pattern,
        test_bitvec_find_all_pattern_null_source,
        test_bitvec_find_all_pattern_null_pattern,
        test_bitvec_find_all_pattern_null_results,
        test_bitvec_find_all_pattern_zero_max_results,
        // New Pattern function deadend tests
        test_bitvec_starts_with_null_source,
        test_bitvec_starts_with_null_prefix,
        test_bitvec_ends_with_null_source,
        test_bitvec_ends_with_null_suffix,
        test_bitvec_contains_at_null_source,
        test_bitvec_contains_at_null_pattern,
        test_bitvec_replace_null_source,
        test_bitvec_matches_null_source,
        test_bitvec_regex_match_null_source,
        test_bitvec_regex_match_null_pattern,
        test_bitvec_prefix_match_null_source,
        test_bitvec_prefix_match_null_patterns,
        test_bitvec_suffix_match_null_source,
        test_bitvec_suffix_match_null_patterns
    };

    int total_tests         = sizeof(tests) / sizeof(tests[0]);
    int total_deadend_tests = sizeof(deadend_tests) / sizeof(deadend_tests[0]);

    // Run all tests using the centralized test driver
    return run_test_suite(tests, total_tests, deadend_tests, total_deadend_tests, "BitVec.Pattern");
}
