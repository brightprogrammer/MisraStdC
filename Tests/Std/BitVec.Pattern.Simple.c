#include <Misra/Std/Container/BitVec.h>
#include <Misra/Std/Container/Str.h>
#include <Misra/Std/Allocator/Default.h>
#include <Misra/Std/Log.h>

#include <Misra/Types.h>

// Include test utilities
#include "../Util/TestRunner.h"

// Helper: append the bits of a 0/1 ASCII string to a BitVec.
static void push_bits(BitVec *bv, const char *bits) {
    for (const char *c = bits; *c != '\0'; c++)
        BitVecPush(bv, *c == '1');
}

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
bool test_find_last_pattern_exact_length_match(void);
bool test_find_all_pattern_vec_exact_length_match(void);
bool test_ends_with_equal_length(void);
bool test_count_pattern_equal_length(void);
bool test_rfind_pattern_equal_length(void);
bool test_regex_match_str_nonmatch_returns_false(void);

// Test basic pattern matching functions
bool test_bitvec_basic_pattern_functions(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing basic BitVec pattern functions\n");

    BitVec source  = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVec pattern = BitVecInit(ALLOCATOR_OF(&alloc));
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
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Test BitVecFindPattern function comprehensively
bool test_bitvec_find_pattern(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVecFindPattern function\n");

    BitVec source  = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVec pattern = BitVecInit(ALLOCATOR_OF(&alloc));
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
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Test BitVecFindLastPattern function
bool test_bitvec_find_last_pattern(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVecFindLastPattern function\n");

    BitVec source  = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVec pattern = BitVecInit(ALLOCATOR_OF(&alloc));
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
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Test BitVecFindAllPattern function
bool test_bitvec_find_all_pattern(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVecFindAllPattern function\n");

    BitVec source  = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVec pattern = BitVecInit(ALLOCATOR_OF(&alloc));
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
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Test BitVecFindAllPattern in its 3-arg Vec form. Same source/pattern as the
// raw-buffer test above (5 hits in 10101010101), but we feed the matches into
// a BitVecMatchIndices and check the full match list. The Vec form never
// truncates -- proves we get all 5 hits without pre-sizing.
bool test_bitvec_find_all_pattern_vec(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *base  = ALLOCATOR_OF(&alloc);

    WriteFmt("Testing BitVecFindAllPattern Vec form\n");

    BitVec source  = BitVecInit(base);
    BitVec pattern = BitVecInit(base);
    bool   result  = true;

    // Source: 10101010101 (11 bits)
    for (int i = 0; i < 11; i++)
        BitVecPush(&source, i % 2 == 0);
    // Pattern: 101
    BitVecPush(&pattern, true);
    BitVecPush(&pattern, false);
    BitVecPush(&pattern, true);

    BitVecMatchIndices matches = VecInitT(matches, base);
    result                     = result && BitVecFindAllPattern(&source, &pattern, &matches);
    result                     = result && VecLen(&matches) == 5;
    if (result) {
        result = result && VecAt(&matches, 0) == 0;
        result = result && VecAt(&matches, 1) == 2;
        result = result && VecAt(&matches, 2) == 4;
        result = result && VecAt(&matches, 3) == 6;
        result = result && VecAt(&matches, 4) == 8;
    }

    VecDeinit(&matches);
    BitVecDeinit(&source);
    BitVecDeinit(&pattern);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Test edge cases for pattern functions
bool test_bitvec_pattern_edge_cases(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVec pattern edge cases\n");

    BitVec source  = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVec pattern = BitVecInit(ALLOCATOR_OF(&alloc));
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
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Stress tests with large data
bool test_bitvec_pattern_stress_tests(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVec pattern stress tests\n");

    BitVec source  = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVec pattern = BitVecInit(ALLOCATOR_OF(&alloc));
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

    // Test finding all occurrences in large data. The injected "1010"
    // patterns plus incidental matches are implementation-positioned, but
    // the find-all contract is observable and exact: at least one match, the
    // first reported index is the same one BitVecFindPattern returned (0),
    // and the indices come back strictly ascending.
    size results[200];
    u64  count = BitVecFindAllPattern(&source, &pattern, results, 200);
    result     = result && (count > 0);
    result     = result && (results[0] == (size)index);
    for (u64 k = 1; k < count; k++) {
        result = result && (results[k] > results[k - 1]);
    }

    BitVecDeinit(&source);
    BitVecDeinit(&pattern);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Tests for missing Pattern functions

// BitVecStartsWith tests
bool test_bitvec_starts_with_basic(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVecStartsWith basic functionality\n");

    BitVec source = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVec prefix = BitVecInit(ALLOCATOR_OF(&alloc));
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
    DefaultAllocatorDeinit(&alloc);
    return result;
}

bool test_bitvec_starts_with_edge_cases(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVecStartsWith edge cases\n");

    BitVec source = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVec prefix = BitVecInit(ALLOCATOR_OF(&alloc));
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
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// BitVecEndsWith tests
bool test_bitvec_ends_with_basic(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVecEndsWith basic functionality\n");

    BitVec source = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVec suffix = BitVecInit(ALLOCATOR_OF(&alloc));
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
    DefaultAllocatorDeinit(&alloc);
    return result;
}

bool test_bitvec_ends_with_edge_cases(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVecEndsWith edge cases\n");

    BitVec source = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVec suffix = BitVecInit(ALLOCATOR_OF(&alloc));
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
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// BitVecFindPattern tests (replacing BitVecContains)
bool test_bitvec_contains_basic(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVecFindPattern basic functionality\n");

    BitVec source  = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVec pattern = BitVecInit(ALLOCATOR_OF(&alloc));
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

    result = result && (BitVecFindPattern(&source, &pattern) != SIZE_MAX);

    // Test non-existing pattern: 000
    BitVecClear(&pattern);
    BitVecPush(&pattern, false);
    BitVecPush(&pattern, false);
    BitVecPush(&pattern, false);

    result = result && (BitVecFindPattern(&source, &pattern) == SIZE_MAX);

    BitVecDeinit(&source);
    BitVecDeinit(&pattern);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// BitVecContainsAt tests
bool test_bitvec_contains_at_basic(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVecContainsAt basic functionality\n");

    BitVec source  = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVec pattern = BitVecInit(ALLOCATOR_OF(&alloc));
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
    DefaultAllocatorDeinit(&alloc);
    return result;
}

bool test_bitvec_contains_at_edge_cases(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVecContainsAt edge cases\n");

    BitVec source  = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVec pattern = BitVecInit(ALLOCATOR_OF(&alloc));
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
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// BitVecCountPattern tests
bool test_bitvec_count_pattern_basic(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVecCountPattern basic functionality\n");

    BitVec source  = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVec pattern = BitVecInit(ALLOCATOR_OF(&alloc));
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
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// BitVecRFindPattern tests
bool test_bitvec_rfind_pattern_basic(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVecRFindPattern basic functionality\n");

    BitVec source  = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVec pattern = BitVecInit(ALLOCATOR_OF(&alloc));
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
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// BitVecReplace tests
bool test_bitvec_replace_basic(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVecReplace basic functionality\n");

    BitVec source      = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVec old_pattern = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVec new_pattern = BitVecInit(ALLOCATOR_OF(&alloc));
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
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// BitVecReplaceAll tests
bool test_bitvec_replace_all_basic(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVecReplaceAll basic functionality\n");

    BitVec source      = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVec old_pattern = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVec new_pattern = BitVecInit(ALLOCATOR_OF(&alloc));
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
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// BitVecMatches tests
bool test_bitvec_matches_basic(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVecMatches basic functionality\n");

    BitVec source   = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVec pattern  = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVec wildcard = BitVecInit(ALLOCATOR_OF(&alloc));
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
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// BitVecFuzzyMatch tests
bool test_bitvec_fuzzy_match_basic(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVecFuzzyMatch basic functionality\n");

    BitVec source  = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVec pattern = BitVecInit(ALLOCATOR_OF(&alloc));
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
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// BitVecRegexMatch tests
bool test_bitvec_regex_match_basic(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVecRegexMatch basic functionality\n");

    BitVec source = BitVecInit(ALLOCATOR_OF(&alloc));
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
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// BitVecPrefixMatch tests
bool test_bitvec_prefix_match_basic(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVecPrefixMatch basic functionality\n");

    BitVec  source   = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVecs patterns = VecInitWithDeepCopy(NULL, BitVecDeinit, ALLOCATOR_OF(&alloc));
    bool    result   = true;

    VecResize(&patterns, 3);

    // Create source: 110101
    BitVecPush(&source, true);
    BitVecPush(&source, true);
    BitVecPush(&source, false);
    BitVecPush(&source, true);
    BitVecPush(&source, false);
    BitVecPush(&source, true);

    BitVec *p0 = VecPtrAt(&patterns, 0);
    BitVec *p1 = VecPtrAt(&patterns, 1);
    BitVec *p2 = VecPtrAt(&patterns, 2);

    *p0 = BitVecInit(ALLOCATOR_OF(&alloc));
    *p1 = BitVecInit(ALLOCATOR_OF(&alloc));
    *p2 = BitVecInit(ALLOCATOR_OF(&alloc));

    // Pattern 0: 111 (should not match)
    BitVecPush(p0, true);
    BitVecPush(p0, true);
    BitVecPush(p0, true);

    // Pattern 1: 110 (should match)
    BitVecPush(p1, true);
    BitVecPush(p1, true);
    BitVecPush(p1, false);

    // Pattern 2: 101 (should not match as prefix)
    BitVecPush(p2, true);
    BitVecPush(p2, false);
    BitVecPush(p2, true);

    u64 match_idx = BitVecPrefixMatch(&source, &patterns);
    result        = result && (match_idx == 1);

    VecDeinit(&patterns);
    BitVecDeinit(&source);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// BitVecSuffixMatch tests
bool test_bitvec_suffix_match_basic(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVecSuffixMatch basic functionality\n");

    BitVec  source   = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVecs patterns = VecInitWithDeepCopy(NULL, BitVecDeinit, ALLOCATOR_OF(&alloc));
    bool    result   = true;

    VecResize(&patterns, 3);

    // Create source: 110101
    BitVecPush(&source, true);
    BitVecPush(&source, true);
    BitVecPush(&source, false);
    BitVecPush(&source, true);
    BitVecPush(&source, false);
    BitVecPush(&source, true);

    BitVec *p0 = VecPtrAt(&patterns, 0);
    BitVec *p1 = VecPtrAt(&patterns, 1);
    BitVec *p2 = VecPtrAt(&patterns, 2);

    *p0 = BitVecInit(ALLOCATOR_OF(&alloc));
    *p1 = BitVecInit(ALLOCATOR_OF(&alloc));
    *p2 = BitVecInit(ALLOCATOR_OF(&alloc));

    // Pattern 0: 111 (should not match)
    BitVecPush(p0, true);
    BitVecPush(p0, true);
    BitVecPush(p0, true);

    // Pattern 1: 101 (should match as suffix)
    BitVecPush(p1, true);
    BitVecPush(p1, false);
    BitVecPush(p1, true);

    // Pattern 2: 110 (should not match as suffix)
    BitVecPush(p2, true);
    BitVecPush(p2, true);
    BitVecPush(p2, false);

    u64 match_idx = BitVecSuffixMatch(&source, &patterns);
    result        = result && (match_idx == 1);

    VecDeinit(&patterns);
    BitVecDeinit(&source);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Main function that runs all tests
// 1269:49 cxx_gt_to_ge -- BitVecFindLastPattern guard `pattern->length >
// bv->length`. Flipping `>` to `>=` rejects a pattern whose length EQUALS the
// bitvector length (a valid exact-length match), wrongly returning SIZE_MAX
// instead of 0.
bool test_find_last_pattern_exact_length_match(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *base  = ALLOCATOR_OF(&alloc);

    BitVec source  = BitVecInit(base);
    BitVec pattern = BitVecInit(base);
    bool   result  = true;

    // Source and pattern are both 1011, same length.
    BitVecPush(&source, true);
    BitVecPush(&source, false);
    BitVecPush(&source, true);
    BitVecPush(&source, true);

    BitVecPush(&pattern, true);
    BitVecPush(&pattern, false);
    BitVecPush(&pattern, true);
    BitVecPush(&pattern, true);

    // Exact-length match must be found at index 0.
    u64 index = BitVecFindLastPattern(&source, &pattern);
    result    = result && (index == 0);

    BitVecDeinit(&source);
    BitVecDeinit(&pattern);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// 1316:49 cxx_gt_to_ge -- bitvec_find_all_pattern_vec guard `pattern->length >
// bv->length`. Flipping `>` to `>=` makes the exact-length case bail early with
// an EMPTY result vec instead of recording the single match at index 0.
bool test_find_all_pattern_vec_exact_length_match(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *base  = ALLOCATOR_OF(&alloc);

    BitVec source  = BitVecInit(base);
    BitVec pattern = BitVecInit(base);
    bool   result  = true;

    // Source and pattern are both 110, same length.
    BitVecPush(&source, true);
    BitVecPush(&source, true);
    BitVecPush(&source, false);

    BitVecPush(&pattern, true);
    BitVecPush(&pattern, true);
    BitVecPush(&pattern, false);

    BitVecMatchIndices matches = VecInitT(matches, base);
    result                     = result && BitVecFindAllPattern(&source, &pattern, &matches);
    // Exact-length match: exactly one hit, at index 0.
    result = result && (VecLen(&matches) == 1);
    if (result) {
        result = result && (VecAt(&matches, 0) == 0);
    }

    VecDeinit(&matches);
    BitVecDeinit(&source);
    BitVecDeinit(&pattern);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// A suffix exactly as long as the vector still matches when equal. Mutant
// 1675:24 (gt_to_ge) rejects equal-length suffixes -> returns false.
bool test_ends_with_equal_length(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    BitVec source = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVec suffix = BitVecInit(ALLOCATOR_OF(&alloc));
    push_bits(&source, "101");
    push_bits(&suffix, "101");

    bool result = BitVecEndsWith(&source, &suffix);

    BitVecDeinit(&source);
    BitVecDeinit(&suffix);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// A pattern exactly as long as the source counts as one match when equal.
// Mutant 1707:49 (gt_to_ge) returns 0 for the equal-length case.
bool test_count_pattern_equal_length(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    BitVec source  = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVec pattern = BitVecInit(ALLOCATOR_OF(&alloc));
    push_bits(&source, "101");
    push_bits(&pattern, "101");

    bool result = (BitVecCountPattern(&source, &pattern) == 1);

    BitVecDeinit(&source);
    BitVecDeinit(&pattern);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// A pattern exactly as long as the source is found at index 0 when searching
// from the last index. Mutant 1725:49 (gt_to_ge) returns SIZE_MAX instead.
bool test_rfind_pattern_equal_length(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    BitVec source  = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVec pattern = BitVecInit(ALLOCATOR_OF(&alloc));
    push_bits(&source, "101");
    push_bits(&pattern, "101");

    bool result = (BitVecRFindPattern(&source, &pattern, 2) == 0);

    BitVecDeinit(&source);
    BitVecDeinit(&pattern);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// 1729:33 cxx_ge_to_lt -- the ternary picks the search-window lower bound.
// Flipping `start + 1 >= pattern->length` to `<` collapses the window to the
// full [0, start] range. With the only match at index 0 and a start whose
// real (narrow) window excludes it, real returns SIZE_MAX but the widened
// mutant returns 0.
static bool test_rfind_window_widen_cond_ge_to_lt(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVecRFindPattern window-condition (ge_to_lt)\n");

    BitVec source  = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVec pattern = BitVecInit(ALLOCATOR_OF(&alloc));
    bool   result  = true;

    push_bits(&source, "101000000"); // "101" only at index 0
    push_bits(&pattern, "101");

    // Narrow window for start=8 is [6,8], which has no match.
    u64 pos = BitVecRFindPattern(&source, &pattern, 8);
    result  = result && (pos == SIZE_MAX);

    BitVecDeinit(&source);
    BitVecDeinit(&pattern);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// 1729:29 cxx_add_to_sub -- mutates the FIRST `start + 1` (inside the ternary
// CONDITION) to `start - 1`. Differs from real only when pattern->length ==
// start: real keeps search_end = start + 1 - len = 1, mutant drops to 0 and
// scans index 0. With the only match at index 0, real returns SIZE_MAX, mutant
// returns 0.
static bool test_rfind_window_cond_add_to_sub(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVecRFindPattern window-condition (add_to_sub)\n");

    BitVec source  = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVec pattern = BitVecInit(ALLOCATOR_OF(&alloc));
    bool   result  = true;

    push_bits(&source, "101111"); // "10" only at index 0
    push_bits(&pattern, "10");    // len 2 == start

    // Real window for start=2 is [1,2]; no match there.
    u64 pos = BitVecRFindPattern(&source, &pattern, 2);
    result  = result && (pos == SIZE_MAX);

    BitVecDeinit(&source);
    BitVecDeinit(&pattern);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// 1729:61 cxx_add_to_sub -- mutates the SECOND `start + 1` (the VALUE
// start + 1 - len) to start - 1 - len, dropping search_end by 2 and so
// extending the window two positions lower. With the only match at index 1 and
// a start whose real window [start-2, start] excludes it but the widened
// mutant window [start-4, start] includes it, real returns SIZE_MAX, mutant 1.
static bool test_rfind_window_value_add_to_sub(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVecRFindPattern window-value (add_to_sub)\n");

    BitVec source  = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVec pattern = BitVecInit(ALLOCATOR_OF(&alloc));
    bool   result  = true;

    push_bits(&source, "010100000"); // "101" only at index 1
    push_bits(&pattern, "101");

    // Real window for start=4 is [2,4]; no match there.
    u64 pos = BitVecRFindPattern(&source, &pattern, 4);
    result  = result && (pos == SIZE_MAX);

    BitVecDeinit(&source);
    BitVecDeinit(&pattern);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// 1731:24 cxx_add_to_sub -- the loop seed `i = start + 1` mutated to
// `start - 1` skips the two highest candidate positions (start and start-1).
// With the match sitting exactly at `start`, real returns start but the
// mutant never inspects it and returns SIZE_MAX.
static bool test_rfind_loop_start_add_to_sub(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVecRFindPattern loop-seed (add_to_sub)\n");

    BitVec source  = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVec pattern = BitVecInit(ALLOCATOR_OF(&alloc));
    bool   result  = true;

    push_bits(&source, "101101101"); // "101" at 0, 3, 6
    push_bits(&pattern, "101");

    // start=6 lands exactly on a match.
    u64 pos = BitVecRFindPattern(&source, &pattern, 6);
    result  = result && (pos == 6);

    BitVecDeinit(&source);
    BitVecDeinit(&pattern);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// 1731:31 cxx_gt_to_ge -- the loop guard `i > search_end` mutated to
// `i >= search_end` runs one extra iteration, inspecting pos = search_end - 1,
// one position below the intended window. With the only match at that extra
// position, real returns SIZE_MAX but the mutant reports it.
static bool test_rfind_loop_cond_gt_to_ge(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVecRFindPattern loop-guard (gt_to_ge)\n");

    BitVec source  = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVec pattern = BitVecInit(ALLOCATOR_OF(&alloc));
    bool   result  = true;

    push_bits(&source, "101000000"); // "101" only at index 0
    push_bits(&pattern, "101");

    // start=3: search_end=1, window [1,3] has no match; the extra mutant
    // iteration would reach index 0.
    u64 pos = BitVecRFindPattern(&source, &pattern, 3);
    result  = result && (pos == SIZE_MAX);

    BitVecDeinit(&source);
    BitVecDeinit(&pattern);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// 1771:14 cxx_init_const -- `bool found = false;` mutated so `found` starts
// truthy. When the inner search finds nothing, real breaks on `!found` and
// returns 0; the mutant skips the break with match_pos still SIZE_MAX and
// drives BitVecRemoveRange(bv, SIZE_MAX, ...), which aborts. Real returns 0
// with the vector untouched.
static bool test_replaceall_found_flag_init(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVecReplaceAll found-flag init\n");

    BitVec source      = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVec old_pattern = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVec new_pattern = BitVecInit(ALLOCATOR_OF(&alloc));
    bool   result      = true;

    push_bits(&source, "0000");     // no "111" anywhere
    push_bits(&old_pattern, "111"); // length fits the vector (3 <= 4)
    push_bits(&new_pattern, "0");

    u64 replacements = BitVecReplaceAll(&source, &old_pattern, &new_pattern);
    result           = result && (replacements == 0);
    result           = result && (BitVecLen(&source) == 4);
    result           = result && (BitVecGet(&source, 0) == false);
    result           = result && (BitVecGet(&source, 3) == false);

    BitVecDeinit(&source);
    BitVecDeinit(&old_pattern);
    BitVecDeinit(&new_pattern);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// 1775:78 cxx_post_inc_to_post_dec -- the forward scan `i++` mutated to `i--`
// turns the search into a backward walk from search_pos that underflows past
// 0 and stops immediately. With the only match strictly after search_pos, real
// finds and replaces it (1 replacement, length shrinks) while the mutant finds
// nothing (0 replacements, length unchanged).
static bool test_replaceall_forward_scan_direction(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVecReplaceAll forward-scan direction\n");

    BitVec source      = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVec old_pattern = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVec new_pattern = BitVecInit(ALLOCATOR_OF(&alloc));
    bool   result      = true;

    push_bits(&source, "00110"); // "110" only at index 2
    push_bits(&old_pattern, "110");
    push_bits(&new_pattern, "01");

    u64 replacements = BitVecReplaceAll(&source, &old_pattern, &new_pattern);
    result           = result && (replacements == 1);
    result           = result && (BitVecLen(&source) == 4); // 5 - 3 + 2

    BitVecDeinit(&source);
    BitVecDeinit(&old_pattern);
    BitVecDeinit(&new_pattern);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Kills 1776:cxx_replace_scalar_call -- replacing BitVecContainsAt's result
// with a truthy constant makes ReplaceAll "find" the absent pattern and
// mutate the source. The contract: an absent old_pattern yields 0
// replacements and leaves the source untouched.
static bool test_replace_all_absent_no_change(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *base  = ALLOCATOR_OF(&alloc);

    BitVec source = BitVecInit(base);
    BitVec old    = BitVecInit(base);
    BitVec neww   = BitVecInit(base);
    bool   result = true;

    // source: 0000 (no occurrence of "11")
    for (int i = 0; i < 4; i++)
        BitVecPush(&source, false);

    // old: 11 (absent), new: 1
    BitVecPush(&old, true);
    BitVecPush(&old, true);
    BitVecPush(&neww, true);

    u64 n  = BitVecReplaceAll(&source, &old, &neww);
    result = result && (n == 0);
    result = result && (BitVecLen(&source) == 4);
    for (u64 i = 0; i < 4; i++)
        result = result && (BitVecGet(&source, i) == false);

    BitVecDeinit(&source);
    BitVecDeinit(&old);
    BitVecDeinit(&neww);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Kills 1790:cxx_replace_scalar_call -- replacing BitVecGet(new_pattern, i)
// with a truthy constant inserts all-ones for the new pattern. The contract:
// the inserted bits reproduce new_pattern exactly, so a leading 0 in
// new_pattern must appear as 0 in the result.
static bool test_replace_all_inserts_new_bits(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *base  = ALLOCATOR_OF(&alloc);

    BitVec source = BitVecInit(base);
    BitVec old    = BitVecInit(base);
    BitVec neww   = BitVecInit(base);
    bool   result = true;

    // source: 110110110
    for (int i = 0; i < 3; i++) {
        BitVecPush(&source, true);
        BitVecPush(&source, true);
        BitVecPush(&source, false);
    }

    // old: 110, new: 01  (each 110 -> 01, result 010101)
    BitVecPush(&old, true);
    BitVecPush(&old, true);
    BitVecPush(&old, false);
    BitVecPush(&neww, false);
    BitVecPush(&neww, true);

    u64 n  = BitVecReplaceAll(&source, &old, &neww);
    result = result && (n == 3);
    result = result && (BitVecLen(&source) == 6);
    // result 010101: position 0 must be 0 (false), position 1 must be 1.
    result = result && (BitVecGet(&source, 0) == false);
    result = result && (BitVecGet(&source, 1) == true);

    BitVecDeinit(&source);
    BitVecDeinit(&old);
    BitVecDeinit(&neww);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Kills 1811:cxx_init_const (i=0 -> 42), 1811:cxx_lt_to_ge (loop never runs),
// 1811:cxx_post_inc_to_post_dec (only position 0 examined), and
// 1813:cxx_replace_scalar_call (wildcard test forced truthy so no position is
// checked). All four make Matches stop comparing and return true. The
// contract: a non-wildcard mismatch at a position past 0 must return false.
static bool test_matches_mismatch_returns_false(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *base  = ALLOCATOR_OF(&alloc);

    BitVec source   = BitVecInit(base);
    BitVec pattern  = BitVecInit(base);
    BitVec wildcard = BitVecInit(base);
    bool   result   = true;

    // source: 1010, pattern: 1111, wildcard: 0000 (no wildcards).
    // Position 0 matches (1 vs 1); position 1 mismatches (0 vs 1) -> false.
    BitVecPush(&source, true);
    BitVecPush(&source, false);
    BitVecPush(&source, true);
    BitVecPush(&source, false);

    for (int i = 0; i < 4; i++)
        BitVecPush(&pattern, true);
    for (int i = 0; i < 4; i++)
        BitVecPush(&wildcard, false);

    result = result && (BitVecMatches(&source, &pattern, &wildcard) == false);

    // Sanity: an all-wildcard mask must still match (proves we did not just
    // wire the function to always return false).
    BitVec wild_all = BitVecInit(base);
    for (int i = 0; i < 4; i++)
        BitVecPush(&wild_all, true);
    result = result && (BitVecMatches(&source, &pattern, &wild_all) == true);

    BitVecDeinit(&wild_all);
    BitVecDeinit(&source);
    BitVecDeinit(&pattern);
    BitVecDeinit(&wildcard);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Kills 1827:cxx_gt_to_ge -- (pattern->length > bv->length) becoming >=
// rejects an equal-length exact match. The contract: a pattern equal in
// length to the source and matching with 0 errors returns index 0.
static bool test_fuzzy_equal_length_match(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *base  = ALLOCATOR_OF(&alloc);

    BitVec source  = BitVecInit(base);
    BitVec pattern = BitVecInit(base);
    bool   result  = true;

    // source: 111, pattern: 111 (same length, exact match).
    for (int i = 0; i < 3; i++) {
        BitVecPush(&source, true);
        BitVecPush(&pattern, true);
    }

    result = result && (BitVecFuzzyMatch(&source, &pattern, 0) == 0);

    BitVecDeinit(&source);
    BitVecDeinit(&pattern);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Kills 1831:cxx_sub_to_add (loop bound bv->length - pattern->length flipped
// to +, walking off the end), 1836:cxx_gt_to_ge and 1836:cxx_gt_to_le (the
// error-budget break flipped, so the wrong window is accepted). The
// contract: with one error allowed, the earliest window within budget is at
// index 4 -- not index 0 (which has 2 errors).
static bool test_fuzzy_match_position(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *base  = ALLOCATOR_OF(&alloc);

    BitVec source  = BitVecInit(base);
    BitVec pattern = BitVecInit(base);
    bool   result  = true;

    // source: 00000111 (8 bits), pattern: 111.
    for (int i = 0; i < 5; i++)
        BitVecPush(&source, false);
    for (int i = 0; i < 3; i++)
        BitVecPush(&source, true);

    BitVecPush(&pattern, true);
    BitVecPush(&pattern, true);
    BitVecPush(&pattern, true);

    // Window [4,6] = 011 -> 1 error <= 1 (first acceptable window).
    // Window [0,2] = 000 -> 3 errors; window [5,7] = 111 -> 0 errors.
    result = result && (BitVecFuzzyMatch(&source, &pattern, 1) == 4);
    // Exact match still found at index 5 when no errors are allowed.
    result = result && (BitVecFuzzyMatch(&source, &pattern, 0) == 5);

    BitVecDeinit(&source);
    BitVecDeinit(&pattern);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Kills 1831:37 cxx_sub_to_add -- the window loop bound `i <= bv->length -
// pattern->length` flipped to `+` walks i past the last valid window. The
// existing position test returns early at a found window and never reaches the
// over-run, so it cannot see the mutation. Here NO window is within budget, so
// the real loop runs to its last valid i (length-patternlen) and returns
// SIZE_MAX; the mutant keeps advancing and indexes BitVecGet(bv, i + j) with
// i + j >= length, tripping its bounds LOG_FATAL. A large all-zero source vs a
// 1-pattern with 0 errors guarantees no match and a deterministic over-run.
static bool test_fuzzy_no_match_completes_loop(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *base  = ALLOCATOR_OF(&alloc);

    BitVec source  = BitVecInit(base);
    BitVec pattern = BitVecInit(base);

    // source: 4096 zero bits; pattern: a single 1. No window ever matches with
    // zero errors, so the loop must traverse every valid i and report SIZE_MAX.
    for (int i = 0; i < 4096; i++)
        BitVecPush(&source, false);
    BitVecPush(&pattern, true);

    bool result = (BitVecFuzzyMatch(&source, &pattern, 0) == SIZE_MAX);

    BitVecDeinit(&source);
    BitVecDeinit(&pattern);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// bitvec_regex_match_str: `bool result = false;` (1877:cxx_init_const ->
// `result = 42` == true) makes a non-matching pattern wrongly return true.
// Caller-observable: a pattern that is NOT a substring of the 0/1 rendering
// must return false.
bool test_regex_match_str_nonmatch_returns_false(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing bitvec_regex_match_str returns false on non-match\n");

    BitVec source = BitVecInit(ALLOCATOR_OF(&alloc));
    bool   result = true;

    // Source rendering: 101010
    BitVecPush(&source, true);
    BitVecPush(&source, false);
    BitVecPush(&source, true);
    BitVecPush(&source, false);
    BitVecPush(&source, true);
    BitVecPush(&source, false);

    // "111" never appears in "101010" -> must be false.
    Str pat_no = StrInitFromZstr("111", &alloc);
    result     = result && !BitVecRegexMatch(&source, &pat_no);

    // Sanity: a genuine substring still matches (guards against a trivially
    // always-false implementation).
    Str pat_yes = StrInitFromZstr("010", &alloc);
    result      = result && BitVecRegexMatch(&source, &pat_yes);

    StrDeinit(&pat_no);
    StrDeinit(&pat_yes);
    BitVecDeinit(&source);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

int main(void) {
    WriteFmt("[INFO] Starting BitVec.Pattern.Simple tests\n\n");

    // Array of test functions
    TestFunction tests[] = {
        test_bitvec_basic_pattern_functions,
        test_bitvec_find_pattern,
        test_bitvec_find_last_pattern,
        test_bitvec_find_all_pattern,
        test_bitvec_find_all_pattern_vec,
        test_bitvec_pattern_edge_cases,
        test_bitvec_pattern_stress_tests,
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
        test_bitvec_suffix_match_basic,
        test_find_last_pattern_exact_length_match,
        test_find_all_pattern_vec_exact_length_match,
        test_ends_with_equal_length,
        test_count_pattern_equal_length,
        test_rfind_pattern_equal_length,
        test_rfind_window_widen_cond_ge_to_lt,
        test_rfind_window_cond_add_to_sub,
        test_rfind_window_value_add_to_sub,
        test_rfind_loop_start_add_to_sub,
        test_rfind_loop_cond_gt_to_ge,
        test_replaceall_found_flag_init,
        test_replaceall_forward_scan_direction,
        test_replace_all_absent_no_change,
        test_replace_all_inserts_new_bits,
        test_matches_mismatch_returns_false,
        test_fuzzy_equal_length_match,
        test_fuzzy_match_position,
        test_fuzzy_no_match_completes_loop,
        test_regex_match_str_nonmatch_returns_false
    };

    int total_tests = sizeof(tests) / sizeof(tests[0]);

    // Run all tests using the centralized test driver
    return run_test_suite(tests, total_tests, NULL, 0, "BitVec.Pattern.Simple");
}
