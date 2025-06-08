#include <Misra/Std/Container/Bits.h>
#include <Misra/Std/Log.h>

#include <stdio.h>
#include <Misra/Types.h>

// Include test utilities
#include "../Util/TestRunner.h"

// Function prototypes
bool test_Bits_basic_pattern_functions(void);
bool test_Bits_find_pattern(void);
bool test_Bits_find_last_pattern(void);
bool test_Bits_find_all_pattern(void);
bool test_Bits_pattern_edge_cases(void);
bool test_Bits_pattern_stress_tests(void);
bool test_Bits_find_pattern_null_source(void);
bool test_Bits_find_pattern_null_pattern(void);
bool test_Bits_find_last_pattern_null_source(void);
bool test_Bits_find_last_pattern_null_pattern(void);
bool test_Bits_find_all_pattern_null_source(void);
bool test_Bits_find_all_pattern_null_pattern(void);
bool test_Bits_find_all_pattern_null_results(void);
bool test_Bits_find_all_pattern_zero_max_results(void);
bool test_Bits_starts_with_basic(void);
bool test_Bits_starts_with_edge_cases(void);
bool test_Bits_ends_with_basic(void);
bool test_Bits_ends_with_edge_cases(void);
bool test_Bits_contains_basic(void);
bool test_Bits_contains_at_basic(void);
bool test_Bits_contains_at_edge_cases(void);
bool test_Bits_count_pattern_basic(void);
bool test_Bits_rfind_pattern_basic(void);
bool test_Bits_replace_basic(void);
bool test_Bits_replace_all_basic(void);
bool test_Bits_matches_basic(void);
bool test_Bits_fuzzy_match_basic(void);
bool test_Bits_regex_match_basic(void);
bool test_Bits_prefix_match_basic(void);
bool test_Bits_suffix_match_basic(void);
bool test_Bits_starts_with_null_source(void);
bool test_Bits_starts_with_null_prefix(void);
bool test_Bits_ends_with_null_source(void);
bool test_Bits_ends_with_null_suffix(void);
bool test_Bits_contains_at_null_source(void);
bool test_Bits_contains_at_null_pattern(void);
bool test_Bits_replace_null_source(void);
bool test_Bits_matches_null_source(void);
bool test_Bits_regex_match_null_source(void);
bool test_Bits_regex_match_null_pattern(void);
bool test_Bits_prefix_match_null_source(void);
bool test_Bits_prefix_match_null_patterns(void);
bool test_Bits_suffix_match_null_source(void);
bool test_Bits_suffix_match_null_patterns(void);

// Test basic pattern matching functions
bool test_Bits_basic_pattern_functions(void) {
    printf("Testing basic Bits pattern functions\n");

    Bits source  = BitsInit();
    Bits pattern = BitsInit();
    bool result  = true;

    // Create source: 11010011101
    BitsPush(&source, true);  // 0
    BitsPush(&source, true);  // 1
    BitsPush(&source, false); // 2
    BitsPush(&source, true);  // 3
    BitsPush(&source, false); // 4
    BitsPush(&source, false); // 5
    BitsPush(&source, true);  // 6
    BitsPush(&source, true);  // 7
    BitsPush(&source, true);  // 8
    BitsPush(&source, false); // 9
    BitsPush(&source, true);  // 10

    // Create pattern: 101
    BitsPush(&pattern, true);
    BitsPush(&pattern, false);
    BitsPush(&pattern, true);

    // Test basic pattern finding
    u64 index = BitsFindPattern(&source, &pattern);
    result    = result && (index == 1); // Should find pattern at index 1 (101...)

    BitsDeinit(&source);
    BitsDeinit(&pattern);
    return result;
}

// Test BitsFindPattern function comprehensively
bool test_Bits_find_pattern(void) {
    printf("Testing BitsFindPattern function\n");

    Bits source  = BitsInit();
    Bits pattern = BitsInit();
    bool result  = true;

    // Create source: 110101011010
    for (int i = 0; i < 12; i++) {
        bool bit = (i % 4 == 0 || i % 4 == 1 || i % 4 == 3);
        BitsPush(&source, bit);
    }

    // Pattern 1: 101 (should find multiple occurrences)
    BitsPush(&pattern, true);
    BitsPush(&pattern, false);
    BitsPush(&pattern, true);

    u64 index = BitsFindPattern(&source, &pattern);
    result    = result && (index == 1); // First occurrence at index 1

    // Pattern 2: Single bit pattern
    BitsClear(&pattern);
    BitsPush(&pattern, true);
    index  = BitsFindPattern(&source, &pattern);
    result = result && (index == 0); // First true bit at index 0

    // Pattern 3: Non-existent pattern
    BitsClear(&pattern);
    BitsPush(&pattern, false);
    BitsPush(&pattern, false);
    BitsPush(&pattern, false);
    index  = BitsFindPattern(&source, &pattern);
    result = result && (index == SIZE_MAX); // Should not find three consecutive falses

    // Pattern 4: Pattern at the end
    BitsClear(&pattern);
    BitsPush(&pattern, true);
    BitsPush(&pattern, false);
    // Depending on the exact source pattern, adjust this test

    BitsDeinit(&source);
    BitsDeinit(&pattern);
    return result;
}

// Test BitsFindLastPattern function
bool test_Bits_find_last_pattern(void) {
    printf("Testing BitsFindLastPattern function\n");

    Bits source  = BitsInit();
    Bits pattern = BitsInit();
    bool result  = true;

    // Create source with multiple pattern occurrences: 101010101
    for (int i = 0; i < 9; i++) {
        BitsPush(&source, i % 2 == 0);
    }

    // Pattern: 10
    BitsPush(&pattern, true);
    BitsPush(&pattern, false);

    u64 index = BitsFindLastPattern(&source, &pattern);
    result    = result && (index == 6); // Last occurrence of "10" should be at index 6

    // Test with pattern that occurs only once
    BitsClear(&source);
    BitsClear(&pattern);

    // Source: 110011
    BitsPush(&source, true);
    BitsPush(&source, true);
    BitsPush(&source, false);
    BitsPush(&source, false);
    BitsPush(&source, true);
    BitsPush(&source, true);

    // Pattern: 001
    BitsPush(&pattern, false);
    BitsPush(&pattern, false);
    BitsPush(&pattern, true);

    index  = BitsFindLastPattern(&source, &pattern);
    result = result && (index == 2); // Only occurrence at index 2

    BitsDeinit(&source);
    BitsDeinit(&pattern);
    return result;
}

// Test BitsFindAllPattern function
bool test_Bits_find_all_pattern(void) {
    printf("Testing BitsFindAllPattern function\n");

    Bits source  = BitsInit();
    Bits pattern = BitsInit();
    bool result  = true;

    // Create source: 10101010101
    for (int i = 0; i < 11; i++) {
        BitsPush(&source, i % 2 == 0);
    }

    // Pattern: 101
    BitsPush(&pattern, true);
    BitsPush(&pattern, false);
    BitsPush(&pattern, true);

    size results[10];
    u64  count = BitsFindAllPattern(&source, &pattern, results, 10);

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
    u64 count_limited = BitsFindAllPattern(&source, &pattern, results, 3);
    result            = result && (count_limited == 3); // Should only return first 3 matches

    // Test with non-overlapping pattern
    BitsClear(&source);
    BitsClear(&pattern);

    // Source: 110110110
    for (int i = 0; i < 9; i++) {
        bool bit = (i % 3 == 0 || i % 3 == 1);
        BitsPush(&source, bit);
    }

    // Pattern: 110
    BitsPush(&pattern, true);
    BitsPush(&pattern, true);
    BitsPush(&pattern, false);

    count  = BitsFindAllPattern(&source, &pattern, results, 10);
    result = result && (count == 3); // Should find at 0, 3, 6

    BitsDeinit(&source);
    BitsDeinit(&pattern);
    return result;
}

// Test edge cases for pattern functions
bool test_Bits_pattern_edge_cases(void) {
    printf("Testing Bits pattern edge cases\n");

    Bits source  = BitsInit();
    Bits pattern = BitsInit();
    bool result  = true;

    // Test empty pattern
    BitsPush(&source, true);
    BitsPush(&source, false);
    u64 index = BitsFindPattern(&source, &pattern); // Empty pattern
    result    = result && (index == SIZE_MAX);

    // Test pattern longer than source
    BitsClear(&pattern);
    for (int i = 0; i < 5; i++) {
        BitsPush(&pattern, true);
    }
    index  = BitsFindPattern(&source, &pattern); // Pattern length 5, source length 2
    result = result && (index == SIZE_MAX);

    // Test empty source
    BitsClear(&source);
    BitsClear(&pattern);
    BitsPush(&pattern, true);
    index  = BitsFindPattern(&source, &pattern);
    result = result && (index == SIZE_MAX);

    // Test exact match (pattern same as source)
    BitsClear(&source);
    BitsClear(&pattern);
    BitsPush(&source, true);
    BitsPush(&source, false);
    BitsPush(&source, true);
    BitsPush(&pattern, true);
    BitsPush(&pattern, false);
    BitsPush(&pattern, true);
    index  = BitsFindPattern(&source, &pattern);
    result = result && (index == 0);

    // Test single bit source and pattern
    BitsClear(&source);
    BitsClear(&pattern);
    BitsPush(&source, true);
    BitsPush(&pattern, true);
    index  = BitsFindPattern(&source, &pattern);
    result = result && (index == 0);

    BitsPush(&pattern, false); // Now pattern is longer than source
    index  = BitsFindPattern(&source, &pattern);
    result = result && (index == SIZE_MAX);

    // Test BitsFindAllPattern edge cases
    size results[1];
    BitsClear(&source);
    BitsClear(&pattern);
    BitsPush(&source, true);
    BitsPush(&pattern, true);

    u64 count = BitsFindAllPattern(&source, &pattern, results, 1);
    result    = result && (count == 1);
    result    = result && (results[0] == 0);

    BitsDeinit(&source);
    BitsDeinit(&pattern);
    return result;
}

// Stress tests with large data
bool test_Bits_pattern_stress_tests(void) {
    printf("Testing Bits pattern stress tests\n");

    Bits source  = BitsInit();
    Bits pattern = BitsInit();
    bool result  = true;

    // Create large source with known pattern
    for (int i = 0; i < 10000; i++) {
        // Create pattern "1010" every 100 bits, rest are "1100"
        if (i % 100 == 0) {
            BitsPush(&source, true); // Start of 1010 pattern
            BitsPush(&source, false);
            BitsPush(&source, true);
            BitsPush(&source, false);
            i += 3; // Skip next 3 iterations
        } else {
            BitsPush(&source, i % 2 == 0);
        }
    }

    // Pattern: 1010
    BitsPush(&pattern, true);
    BitsPush(&pattern, false);
    BitsPush(&pattern, true);
    BitsPush(&pattern, false);

    u64 index = BitsFindPattern(&source, &pattern);
    result    = result && (index == 0); // Should find first pattern at beginning

    // Test finding all occurrences in large data
    size results[200];
    u64  count = BitsFindAllPattern(&source, &pattern, results, 200);
    result     = result && (count > 0); // Should find at least some patterns

    BitsDeinit(&source);
    BitsDeinit(&pattern);
    return result;
}

// Tests for missing Pattern functions

// BitsStartsWith tests
bool test_Bits_starts_with_basic(void) {
    printf("Testing BitsStartsWith basic functionality\n");

    Bits source = BitsInit();
    Bits prefix = BitsInit();
    bool result = true;

    // Create source: 110101
    BitsPush(&source, true);
    BitsPush(&source, true);
    BitsPush(&source, false);
    BitsPush(&source, true);
    BitsPush(&source, false);
    BitsPush(&source, true);

    // Test prefix: 110
    BitsPush(&prefix, true);
    BitsPush(&prefix, true);
    BitsPush(&prefix, false);

    result = result && BitsStartsWith(&source, &prefix);

    // Test non-matching prefix: 101
    BitsClear(&prefix);
    BitsPush(&prefix, true);
    BitsPush(&prefix, false);
    BitsPush(&prefix, true);

    result = result && !BitsStartsWith(&source, &prefix);

    BitsDeinit(&source);
    BitsDeinit(&prefix);
    return result;
}

bool test_Bits_starts_with_edge_cases(void) {
    printf("Testing BitsStartsWith edge cases\n");

    Bits source = BitsInit();
    Bits prefix = BitsInit();
    bool result = true;

    // Test empty prefix (should always match)
    BitsPush(&source, true);
    result = result && BitsStartsWith(&source, &prefix);

    // Test prefix longer than source
    BitsClear(&source);
    BitsPush(&prefix, true);
    BitsPush(&prefix, false);
    result = result && !BitsStartsWith(&source, &prefix);

    // Test equal length
    BitsClear(&source);
    BitsClear(&prefix);
    BitsPush(&source, true);
    BitsPush(&source, false);
    BitsPush(&prefix, true);
    BitsPush(&prefix, false);
    result = result && BitsStartsWith(&source, &prefix);

    BitsDeinit(&source);
    BitsDeinit(&prefix);
    return result;
}

// BitsEndsWith tests
bool test_Bits_ends_with_basic(void) {
    printf("Testing BitsEndsWith basic functionality\n");

    Bits source = BitsInit();
    Bits suffix = BitsInit();
    bool result = true;

    // Create source: 110101
    BitsPush(&source, true);
    BitsPush(&source, true);
    BitsPush(&source, false);
    BitsPush(&source, true);
    BitsPush(&source, false);
    BitsPush(&source, true);

    // Test suffix: 101
    BitsPush(&suffix, true);
    BitsPush(&suffix, false);
    BitsPush(&suffix, true);

    result = result && BitsEndsWith(&source, &suffix);

    // Test non-matching suffix: 110
    BitsClear(&suffix);
    BitsPush(&suffix, true);
    BitsPush(&suffix, true);
    BitsPush(&suffix, false);

    result = result && !BitsEndsWith(&source, &suffix);

    BitsDeinit(&source);
    BitsDeinit(&suffix);
    return result;
}

bool test_Bits_ends_with_edge_cases(void) {
    printf("Testing BitsEndsWith edge cases\n");

    Bits source = BitsInit();
    Bits suffix = BitsInit();
    bool result = true;

    // Test empty suffix (should always match)
    BitsPush(&source, true);
    result = result && BitsEndsWith(&source, &suffix);

    // Test suffix longer than source
    BitsClear(&source);
    BitsPush(&suffix, true);
    BitsPush(&suffix, false);
    result = result && !BitsEndsWith(&source, &suffix);

    BitsDeinit(&source);
    BitsDeinit(&suffix);
    return result;
}

// BitsContains tests
bool test_Bits_contains_basic(void) {
    printf("Testing BitsContains basic functionality\n");

    Bits source  = BitsInit();
    Bits pattern = BitsInit();
    bool result  = true;

    // Create source: 1101011
    BitsPush(&source, true);
    BitsPush(&source, true);
    BitsPush(&source, false);
    BitsPush(&source, true);
    BitsPush(&source, false);
    BitsPush(&source, true);
    BitsPush(&source, true);

    // Test pattern: 101 (exists at positions 2 and 4)
    BitsPush(&pattern, true);
    BitsPush(&pattern, false);
    BitsPush(&pattern, true);

    result = result && BitsContains(&source, &pattern);

    // Test non-existing pattern: 000
    BitsClear(&pattern);
    BitsPush(&pattern, false);
    BitsPush(&pattern, false);
    BitsPush(&pattern, false);

    result = result && !BitsContains(&source, &pattern);

    BitsDeinit(&source);
    BitsDeinit(&pattern);
    return result;
}

// BitsContainsAt tests
bool test_Bits_contains_at_basic(void) {
    printf("Testing BitsContainsAt basic functionality\n");

    Bits source  = BitsInit();
    Bits pattern = BitsInit();
    bool result  = true;

    // Create source: 1101011
    BitsPush(&source, true);
    BitsPush(&source, true);
    BitsPush(&source, false);
    BitsPush(&source, true);
    BitsPush(&source, false);
    BitsPush(&source, true);
    BitsPush(&source, true);

    // Test pattern: 101
    BitsPush(&pattern, true);
    BitsPush(&pattern, false);
    BitsPush(&pattern, true);

    result = result && BitsContainsAt(&source, &pattern, 1);  // Should match
    result = result && !BitsContainsAt(&source, &pattern, 0); // Should not match
    result = result && BitsContainsAt(&source, &pattern, 3);  // Should match

    BitsDeinit(&source);
    BitsDeinit(&pattern);
    return result;
}

bool test_Bits_contains_at_edge_cases(void) {
    printf("Testing BitsContainsAt edge cases\n");

    Bits source  = BitsInit();
    Bits pattern = BitsInit();
    bool result  = true;

    // Create small source
    BitsPush(&source, true);
    BitsPush(&source, false);

    // Test pattern that extends beyond source
    BitsPush(&pattern, true);
    BitsPush(&pattern, false);
    BitsPush(&pattern, true);

    result = result && !BitsContainsAt(&source, &pattern, 0); // Pattern too long

    BitsDeinit(&source);
    BitsDeinit(&pattern);
    return result;
}

// BitsCountPattern tests
bool test_Bits_count_pattern_basic(void) {
    printf("Testing BitsCountPattern basic functionality\n");

    Bits source  = BitsInit();
    Bits pattern = BitsInit();
    bool result  = true;

    // Create source: 101010101
    for (int i = 0; i < 9; i++) {
        BitsPush(&source, i % 2 == 0);
    }

    // Test pattern: 101 (should find 4 occurrences at 0, 2, 4, 6)
    BitsPush(&pattern, true);
    BitsPush(&pattern, false);
    BitsPush(&pattern, true);

    u64 count = BitsCountPattern(&source, &pattern);
    result    = result && (count == 4);

    // Test pattern: 010 (should find 3 occurrences at 1, 3, 5)
    BitsClear(&pattern);
    BitsPush(&pattern, false);
    BitsPush(&pattern, true);
    BitsPush(&pattern, false);

    count  = BitsCountPattern(&source, &pattern);
    result = result && (count == 3);

    BitsDeinit(&source);
    BitsDeinit(&pattern);
    return result;
}

// BitsRFindPattern tests
bool test_Bits_rfind_pattern_basic(void) {
    printf("Testing BitsRFindPattern basic functionality\n");

    Bits source  = BitsInit();
    Bits pattern = BitsInit();
    bool result  = true;

    // Create source: 101101101
    BitsPush(&source, true);
    BitsPush(&source, false);
    BitsPush(&source, true);
    BitsPush(&source, true);
    BitsPush(&source, false);
    BitsPush(&source, true);
    BitsPush(&source, true);
    BitsPush(&source, false);
    BitsPush(&source, true);

    // Test pattern: 101
    BitsPush(&pattern, true);
    BitsPush(&pattern, false);
    BitsPush(&pattern, true);

    // Search from index 8 backwards
    u64 pos = BitsRFindPattern(&source, &pattern, 8);
    result  = result && (pos == 6); // Should find at position 6

    // Search from index 5 backwards
    pos    = BitsRFindPattern(&source, &pattern, 5);
    result = result && (pos == 3); // Should find at position 3

    BitsDeinit(&source);
    BitsDeinit(&pattern);
    return result;
}

// BitsReplace tests
bool test_Bits_replace_basic(void) {
    printf("Testing BitsReplace basic functionality\n");

    Bits source      = BitsInit();
    Bits old_pattern = BitsInit();
    Bits new_pattern = BitsInit();
    bool result      = true;

    // Create source: 110110
    BitsPush(&source, true);
    BitsPush(&source, true);
    BitsPush(&source, false);
    BitsPush(&source, true);
    BitsPush(&source, true);
    BitsPush(&source, false);

    // Old pattern: 110
    BitsPush(&old_pattern, true);
    BitsPush(&old_pattern, true);
    BitsPush(&old_pattern, false);

    // New pattern: 101
    BitsPush(&new_pattern, true);
    BitsPush(&new_pattern, false);
    BitsPush(&new_pattern, true);

    bool replaced = BitsReplace(&source, &old_pattern, &new_pattern);
    result        = result && replaced;

    // Check result should be: 101110
    result = result && (BitsLen(&source) == 6);
    result = result && (BitsGet(&source, 0) == true);
    result = result && (BitsGet(&source, 1) == false);
    result = result && (BitsGet(&source, 2) == true);

    BitsDeinit(&source);
    BitsDeinit(&old_pattern);
    BitsDeinit(&new_pattern);
    return result;
}

// BitsReplaceAll tests
bool test_Bits_replace_all_basic(void) {
    printf("Testing BitsReplaceAll basic functionality\n");

    Bits source      = BitsInit();
    Bits old_pattern = BitsInit();
    Bits new_pattern = BitsInit();
    bool result      = true;

    // Create source: 110110110
    for (int i = 0; i < 3; i++) {
        BitsPush(&source, true);
        BitsPush(&source, true);
        BitsPush(&source, false);
    }

    // Old pattern: 110
    BitsPush(&old_pattern, true);
    BitsPush(&old_pattern, true);
    BitsPush(&old_pattern, false);

    // New pattern: 01
    BitsPush(&new_pattern, false);
    BitsPush(&new_pattern, true);

    u64 replacements = BitsReplaceAll(&source, &old_pattern, &new_pattern);
    result           = result && (replacements == 3);

    // Check final length
    result = result && (BitsLen(&source) == 6); // 3 * 2 = 6

    BitsDeinit(&source);
    BitsDeinit(&old_pattern);
    BitsDeinit(&new_pattern);
    return result;
}

// BitsMatches tests
bool test_Bits_matches_basic(void) {
    printf("Testing BitsMatches basic functionality\n");

    Bits source   = BitsInit();
    Bits pattern  = BitsInit();
    Bits wildcard = BitsInit();
    bool result   = true;

    // Create source: 1101
    BitsPush(&source, true);
    BitsPush(&source, true);
    BitsPush(&source, false);
    BitsPush(&source, true);

    // Create pattern: 1?01 (where ? can be anything)
    BitsPush(&pattern, true);
    BitsPush(&pattern, false); // This will be wildcarded
    BitsPush(&pattern, false);
    BitsPush(&pattern, true);

    // Create wildcard: 0100 (1 means wildcard, 0 means must match exactly)
    BitsPush(&wildcard, false);
    BitsPush(&wildcard, true); // Wildcard position
    BitsPush(&wildcard, false);
    BitsPush(&wildcard, false);

    result = result && BitsMatches(&source, &pattern, &wildcard);

    BitsDeinit(&source);
    BitsDeinit(&pattern);
    BitsDeinit(&wildcard);
    return result;
}

// BitsFuzzyMatch tests
bool test_Bits_fuzzy_match_basic(void) {
    printf("Testing BitsFuzzyMatch basic functionality\n");

    Bits source  = BitsInit();
    Bits pattern = BitsInit();
    bool result  = true;

    // Create source: 110100111
    BitsPush(&source, true);
    BitsPush(&source, true);
    BitsPush(&source, false);
    BitsPush(&source, true);
    BitsPush(&source, false);
    BitsPush(&source, false);
    BitsPush(&source, true);
    BitsPush(&source, true);
    BitsPush(&source, true);

    // Create pattern: 111 (should match at position 6 with 0 errors)
    BitsPush(&pattern, true);
    BitsPush(&pattern, true);
    BitsPush(&pattern, true);

    u64 pos = BitsFuzzyMatch(&source, &pattern, 0);
    result  = result && (pos == 6);

    // Test with 1 error allowed
    pos    = BitsFuzzyMatch(&source, &pattern, 1);
    result = result && (pos == 0); // Should match 110 with 1 error

    BitsDeinit(&source);
    BitsDeinit(&pattern);
    return result;
}

// BitsPrefixMatch tests
bool test_Bits_prefix_match_basic(void) {
    printf("Testing BitsPrefixMatch basic functionality\n");

    Bits source = BitsInit();
    Bits patterns[3];
    bool result = true;

    // Initialize patterns
    for (int i = 0; i < 3; i++) {
        patterns[i] = BitsInit();
    }

    // Create source: 110101
    BitsPush(&source, true);
    BitsPush(&source, true);
    BitsPush(&source, false);
    BitsPush(&source, true);
    BitsPush(&source, false);
    BitsPush(&source, true);

    // Pattern 0: 111 (should not match)
    BitsPush(&patterns[0], true);
    BitsPush(&patterns[0], true);
    BitsPush(&patterns[0], true);

    // Pattern 1: 110 (should match)
    BitsPush(&patterns[1], true);
    BitsPush(&patterns[1], true);
    BitsPush(&patterns[1], false);

    // Pattern 2: 101 (should not match as prefix)
    BitsPush(&patterns[2], true);
    BitsPush(&patterns[2], false);
    BitsPush(&patterns[2], true);

    u64 match_idx = BitsPrefixMatch(&source, patterns, 3);
    result        = result && (match_idx == 1);

    for (int i = 0; i < 3; i++) {
        BitsDeinit(&patterns[i]);
    }
    BitsDeinit(&source);
    return result;
}

// BitsSuffixMatch tests
bool test_Bits_suffix_match_basic(void) {
    printf("Testing BitsSuffixMatch basic functionality\n");

    Bits source = BitsInit();
    Bits patterns[3];
    bool result = true;

    // Initialize patterns
    for (int i = 0; i < 3; i++) {
        patterns[i] = BitsInit();
    }

    // Create source: 110101
    BitsPush(&source, true);
    BitsPush(&source, true);
    BitsPush(&source, false);
    BitsPush(&source, true);
    BitsPush(&source, false);
    BitsPush(&source, true);

    // Pattern 0: 111 (should not match)
    BitsPush(&patterns[0], true);
    BitsPush(&patterns[0], true);
    BitsPush(&patterns[0], true);

    // Pattern 1: 101 (should match as suffix)
    BitsPush(&patterns[1], true);
    BitsPush(&patterns[1], false);
    BitsPush(&patterns[1], true);

    // Pattern 2: 110 (should not match as suffix)
    BitsPush(&patterns[2], true);
    BitsPush(&patterns[2], true);
    BitsPush(&patterns[2], false);

    u64 match_idx = BitsSuffixMatch(&source, patterns, 3);
    result        = result && (match_idx == 1);

    for (int i = 0; i < 3; i++) {
        BitsDeinit(&patterns[i]);
    }
    BitsDeinit(&source);
    return result;
}

// Main function that runs all tests
int main(void) {
    printf("[INFO] Starting Bits.Pattern.Simple tests\n\n");

    // Array of test functions
    TestFunction tests[] = {
        test_Bits_basic_pattern_functions, test_Bits_find_pattern,           test_Bits_find_last_pattern,
        test_Bits_find_all_pattern,        test_Bits_pattern_edge_cases,     test_Bits_pattern_stress_tests,
        test_Bits_starts_with_basic,       test_Bits_starts_with_edge_cases, test_Bits_ends_with_basic,
        test_Bits_ends_with_edge_cases,    test_Bits_contains_basic,         test_Bits_contains_at_basic,
        test_Bits_contains_at_edge_cases,  test_Bits_count_pattern_basic,    test_Bits_rfind_pattern_basic,
        test_Bits_replace_basic,           test_Bits_replace_all_basic,      test_Bits_matches_basic,
        test_Bits_fuzzy_match_basic,       test_Bits_regex_match_basic,      test_Bits_prefix_match_basic,
        test_Bits_suffix_match_basic
    };

    int total_tests = sizeof(tests) / sizeof(tests[0]);

    // Run all tests using the centralized test driver
    return run_test_suite(tests, total_tests, NULL, 0, "Bits.Pattern.Simple");
}
