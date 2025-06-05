#include <Misra/Std/Container/BitVec.h>
#include <Misra/Std/Log.h>

#include <stdio.h>
#include <Misra/Types.h>

// Include test utilities
#include "../Util/TestRunner.h"

// Function prototypes for deadend tests
bool test_bitvec_find_pattern_null_source(void);
bool test_bitvec_find_pattern_null_pattern(void);
bool test_bitvec_find_last_pattern_null_source(void);
bool test_bitvec_find_last_pattern_null_pattern(void);
bool test_bitvec_find_all_pattern_null_source(void);
bool test_bitvec_find_all_pattern_null_pattern(void);
bool test_bitvec_find_all_pattern_null_results(void);
bool test_bitvec_find_all_pattern_zero_max_results(void);
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

// Deadend test 1: BitVecFindPattern with NULL source
bool test_bitvec_find_pattern_null_source(void) {
    printf("Testing BitVecFindPattern(NULL, pattern) - should fatal\n");

    BitVec pattern = BitVecInit();
    BitVecPush(&pattern, true);

    BitVecFindPattern(NULL, &pattern); // Should cause LOG_FATAL

    BitVecDeinit(&pattern);
    return true;
}

// Deadend test 2: BitVecFindPattern with NULL pattern
bool test_bitvec_find_pattern_null_pattern(void) {
    printf("Testing BitVecFindPattern(source, NULL) - should fatal\n");

    BitVec source = BitVecInit();
    BitVecPush(&source, true);
    BitVecPush(&source, false);

    BitVecFindPattern(&source, NULL); // Should cause LOG_FATAL

    BitVecDeinit(&source);
    return true;
}

// Deadend test 3: BitVecFindLastPattern with NULL source
bool test_bitvec_find_last_pattern_null_source(void) {
    printf("Testing BitVecFindLastPattern(NULL, pattern) - should fatal\n");

    BitVec pattern = BitVecInit();
    BitVecPush(&pattern, true);

    BitVecFindLastPattern(NULL, &pattern); // Should cause LOG_FATAL

    BitVecDeinit(&pattern);
    return true;
}

// Deadend test 4: BitVecFindLastPattern with NULL pattern
bool test_bitvec_find_last_pattern_null_pattern(void) {
    printf("Testing BitVecFindLastPattern(source, NULL) - should fatal\n");

    BitVec source = BitVecInit();
    BitVecPush(&source, true);
    BitVecPush(&source, false);

    BitVecFindLastPattern(&source, NULL); // Should cause LOG_FATAL

    BitVecDeinit(&source);
    return true;
}

// Deadend test 5: BitVecFindAllPattern with NULL source
bool test_bitvec_find_all_pattern_null_source(void) {
    printf("Testing BitVecFindAllPattern(NULL, pattern, results, 10) - should fatal\n");

    size results[10];

    // Don't create pattern BitVec since we're testing NULL source validation
    BitVecFindAllPattern(NULL, (BitVec *)0x1, results, 10); // Should cause LOG_FATAL

    return true;
}

// Deadend test 6: BitVecFindAllPattern with NULL pattern
bool test_bitvec_find_all_pattern_null_pattern(void) {
    printf("Testing BitVecFindAllPattern(source, NULL, results, 10) - should fatal\n");

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
    printf("Testing BitVecFindAllPattern(source, pattern, NULL, 10) - should fatal\n");

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
    printf("Testing BitVecFindAllPattern(source, pattern, results, 0) - should fatal\n");

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

// Additional deadend tests for missing Pattern functions

bool test_bitvec_starts_with_null_source(void) {
    printf("Testing BitVecStartsWith(NULL, prefix) - should fatal\n");
    BitVec prefix = BitVecInit();
    BitVecPush(&prefix, true);
    BitVecStartsWith(NULL, &prefix);
    BitVecDeinit(&prefix);
    return true;
}

bool test_bitvec_starts_with_null_prefix(void) {
    printf("Testing BitVecStartsWith(source, NULL) - should fatal\n");
    BitVec source = BitVecInit();
    BitVecPush(&source, true);
    BitVecStartsWith(&source, NULL);
    BitVecDeinit(&source);
    return true;
}

bool test_bitvec_ends_with_null_source(void) {
    printf("Testing BitVecEndsWith(NULL, suffix) - should fatal\n");
    BitVec suffix = BitVecInit();
    BitVecPush(&suffix, true);
    BitVecEndsWith(NULL, &suffix);
    BitVecDeinit(&suffix);
    return true;
}

bool test_bitvec_ends_with_null_suffix(void) {
    printf("Testing BitVecEndsWith(source, NULL) - should fatal\n");
    BitVec source = BitVecInit();
    BitVecPush(&source, true);
    BitVecEndsWith(&source, NULL);
    BitVecDeinit(&source);
    return true;
}

bool test_bitvec_contains_at_null_source(void) {
    printf("Testing BitVecContainsAt(NULL, pattern, 0) - should fatal\n");
    BitVec pattern = BitVecInit();
    BitVecPush(&pattern, true);
    BitVecContainsAt(NULL, &pattern, 0);
    BitVecDeinit(&pattern);
    return true;
}

bool test_bitvec_contains_at_null_pattern(void) {
    printf("Testing BitVecContainsAt(source, NULL, 0) - should fatal\n");
    BitVec source = BitVecInit();
    BitVecPush(&source, true);
    BitVecContainsAt(&source, NULL, 0);
    BitVecDeinit(&source);
    return true;
}

bool test_bitvec_replace_null_source(void) {
    printf("Testing BitVecReplace(NULL, old, new) - should fatal\n");

    // Don't create BitVecs since we're testing NULL source validation
    BitVecReplace(NULL, (BitVec *)0x1, (BitVec *)0x1);
    return true;
}

bool test_bitvec_matches_null_source(void) {
    printf("Testing BitVecMatches(NULL, pattern, wildcard) - should fatal\n");
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
    printf("Testing BitVecRegexMatch(NULL, pattern) - should fatal\n");
    BitVecRegexMatch(NULL, "101");
    return true;
}

bool test_bitvec_regex_match_null_pattern(void) {
    printf("Testing BitVecRegexMatch(source, NULL) - should fatal\n");
    BitVec source = BitVecInit();
    BitVecPush(&source, true);
    BitVecRegexMatch(&source, NULL);
    BitVecDeinit(&source);
    return true;
}

bool test_bitvec_prefix_match_null_source(void) {
    printf("Testing BitVecPrefixMatch(NULL, patterns, 1) - should fatal\n");
    BitVec patterns[1] = {BitVecInit()};
    BitVecPush(&patterns[0], true);
    BitVecPrefixMatch(NULL, patterns, 1);
    BitVecDeinit(&patterns[0]);
    return true;
}

bool test_bitvec_prefix_match_null_patterns(void) {
    printf("Testing BitVecPrefixMatch(source, NULL, 1) - should fatal\n");
    BitVec source = BitVecInit();
    BitVecPush(&source, true);
    BitVecPrefixMatch(&source, NULL, 1);
    BitVecDeinit(&source);
    return true;
}

bool test_bitvec_suffix_match_null_source(void) {
    printf("Testing BitVecSuffixMatch(NULL, patterns, 1) - should fatal\n");
    BitVec patterns[1] = {BitVecInit()};
    BitVecPush(&patterns[0], true);
    BitVecSuffixMatch(NULL, patterns, 1);
    BitVecDeinit(&patterns[0]);
    return true;
}

bool test_bitvec_suffix_match_null_patterns(void) {
    printf("Testing BitVecSuffixMatch(source, NULL, 1) - should fatal\n");
    BitVec source = BitVecInit();
    BitVecPush(&source, true);
    BitVecSuffixMatch(&source, NULL, 1);
    BitVecDeinit(&source);
    return true;
}

// Main function that runs all deadend tests
int main(void) {
    printf("[INFO] Starting BitVec.Pattern.Deadend tests\n\n");

    // Deadend tests that would cause program termination
    TestFunction deadend_tests[] = {
        test_bitvec_find_pattern_null_source,      test_bitvec_find_pattern_null_pattern,
        test_bitvec_find_last_pattern_null_source, test_bitvec_find_last_pattern_null_pattern,
        test_bitvec_find_all_pattern_null_source,  test_bitvec_find_all_pattern_null_pattern,
        test_bitvec_find_all_pattern_null_results, test_bitvec_find_all_pattern_zero_max_results,
        test_bitvec_starts_with_null_source,       test_bitvec_starts_with_null_prefix,
        test_bitvec_ends_with_null_source,         test_bitvec_ends_with_null_suffix,
        test_bitvec_contains_at_null_source,       test_bitvec_contains_at_null_pattern,
        test_bitvec_replace_null_source,           test_bitvec_matches_null_source,
        test_bitvec_regex_match_null_source,       test_bitvec_regex_match_null_pattern,
        test_bitvec_prefix_match_null_source,      test_bitvec_prefix_match_null_patterns,
        test_bitvec_suffix_match_null_source,      test_bitvec_suffix_match_null_patterns
    };

    int total_deadend_tests = sizeof(deadend_tests) / sizeof(deadend_tests[0]);

    // Run all deadend tests using the centralized test driver
    return run_test_suite(NULL, 0, deadend_tests, total_deadend_tests, "BitVec.Pattern.Deadend");
}