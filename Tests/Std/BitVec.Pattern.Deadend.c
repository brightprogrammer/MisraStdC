#include <Misra/Std/Container/Bits.h>
#include <Misra/Std/Log.h>

#include <stdio.h>
#include <Misra/Types.h>

// Include test utilities
#include "../Util/TestRunner.h"

// Function prototypes for deadend tests
bool test_Bits_find_pattern_null_source(void);
bool test_Bits_find_pattern_null_pattern(void);
bool test_Bits_find_last_pattern_null_source(void);
bool test_Bits_find_last_pattern_null_pattern(void);
bool test_Bits_find_all_pattern_null_source(void);
bool test_Bits_find_all_pattern_null_pattern(void);
bool test_Bits_find_all_pattern_null_results(void);
bool test_Bits_find_all_pattern_zero_max_results(void);
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

// Deadend test 1: BitsFindPattern with NULL source
bool test_Bits_find_pattern_null_source(void) {
    printf("Testing BitsFindPattern(NULL, pattern) - should fatal\n");

    Bits pattern = BitsInit();
    BitsPush(&pattern, true);

    BitsFindPattern(NULL, &pattern); // Should cause LOG_FATAL

    BitsDeinit(&pattern);
    return true;
}

// Deadend test 2: BitsFindPattern with NULL pattern
bool test_Bits_find_pattern_null_pattern(void) {
    printf("Testing BitsFindPattern(source, NULL) - should fatal\n");

    Bits source = BitsInit();
    BitsPush(&source, true);
    BitsPush(&source, false);

    BitsFindPattern(&source, NULL); // Should cause LOG_FATAL

    BitsDeinit(&source);
    return true;
}

// Deadend test 3: BitsFindLastPattern with NULL source
bool test_Bits_find_last_pattern_null_source(void) {
    printf("Testing BitsFindLastPattern(NULL, pattern) - should fatal\n");

    Bits pattern = BitsInit();
    BitsPush(&pattern, true);

    BitsFindLastPattern(NULL, &pattern); // Should cause LOG_FATAL

    BitsDeinit(&pattern);
    return true;
}

// Deadend test 4: BitsFindLastPattern with NULL pattern
bool test_Bits_find_last_pattern_null_pattern(void) {
    printf("Testing BitsFindLastPattern(source, NULL) - should fatal\n");

    Bits source = BitsInit();
    BitsPush(&source, true);
    BitsPush(&source, false);

    BitsFindLastPattern(&source, NULL); // Should cause LOG_FATAL

    BitsDeinit(&source);
    return true;
}

// Deadend test 5: BitsFindAllPattern with NULL source
bool test_Bits_find_all_pattern_null_source(void) {
    printf("Testing BitsFindAllPattern(NULL, pattern, results, 10) - should fatal\n");

    size results[10];

    // Don't create pattern Bits since we're testing NULL source validation
    BitsFindAllPattern(NULL, (Bits *)0x1, results, 10); // Should cause LOG_FATAL

    return true;
}

// Deadend test 6: BitsFindAllPattern with NULL pattern
bool test_Bits_find_all_pattern_null_pattern(void) {
    printf("Testing BitsFindAllPattern(source, NULL, results, 10) - should fatal\n");

    Bits source = BitsInit();
    size results[10];
    BitsPush(&source, true);
    BitsPush(&source, false);

    BitsFindAllPattern(&source, NULL, results, 10); // Should cause LOG_FATAL

    BitsDeinit(&source);
    return true;
}

// Deadend test 7: BitsFindAllPattern with NULL results
bool test_Bits_find_all_pattern_null_results(void) {
    printf("Testing BitsFindAllPattern(source, pattern, NULL, 10) - should fatal\n");

    Bits source  = BitsInit();
    Bits pattern = BitsInit();
    BitsPush(&source, true);
    BitsPush(&source, false);
    BitsPush(&pattern, true);

    BitsFindAllPattern(&source, &pattern, NULL, 10); // Should cause LOG_FATAL

    BitsDeinit(&source);
    BitsDeinit(&pattern);
    return true;
}

// Deadend test 8: BitsFindAllPattern with zero max_results
bool test_Bits_find_all_pattern_zero_max_results(void) {
    printf("Testing BitsFindAllPattern(source, pattern, results, 0) - should fatal\n");

    Bits source  = BitsInit();
    Bits pattern = BitsInit();
    size results[10];
    BitsPush(&source, true);
    BitsPush(&source, false);
    BitsPush(&pattern, true);

    BitsFindAllPattern(&source, &pattern, results, 0); // Should cause LOG_FATAL

    BitsDeinit(&source);
    BitsDeinit(&pattern);
    return true;
}

// Additional deadend tests for missing Pattern functions

bool test_Bits_starts_with_null_source(void) {
    printf("Testing BitsStartsWith(NULL, prefix) - should fatal\n");
    Bits prefix = BitsInit();
    BitsPush(&prefix, true);
    BitsStartsWith(NULL, &prefix);
    BitsDeinit(&prefix);
    return true;
}

bool test_Bits_starts_with_null_prefix(void) {
    printf("Testing BitsStartsWith(source, NULL) - should fatal\n");
    Bits source = BitsInit();
    BitsPush(&source, true);
    BitsStartsWith(&source, NULL);
    BitsDeinit(&source);
    return true;
}

bool test_Bits_ends_with_null_source(void) {
    printf("Testing BitsEndsWith(NULL, suffix) - should fatal\n");
    Bits suffix = BitsInit();
    BitsPush(&suffix, true);
    BitsEndsWith(NULL, &suffix);
    BitsDeinit(&suffix);
    return true;
}

bool test_Bits_ends_with_null_suffix(void) {
    printf("Testing BitsEndsWith(source, NULL) - should fatal\n");
    Bits source = BitsInit();
    BitsPush(&source, true);
    BitsEndsWith(&source, NULL);
    BitsDeinit(&source);
    return true;
}

bool test_Bits_contains_at_null_source(void) {
    printf("Testing BitsContainsAt(NULL, pattern, 0) - should fatal\n");
    Bits pattern = BitsInit();
    BitsPush(&pattern, true);
    BitsContainsAt(NULL, &pattern, 0);
    BitsDeinit(&pattern);
    return true;
}

bool test_Bits_contains_at_null_pattern(void) {
    printf("Testing BitsContainsAt(source, NULL, 0) - should fatal\n");
    Bits source = BitsInit();
    BitsPush(&source, true);
    BitsContainsAt(&source, NULL, 0);
    BitsDeinit(&source);
    return true;
}

bool test_Bits_replace_null_source(void) {
    printf("Testing BitsReplace(NULL, old, new) - should fatal\n");

    // Don't create Bitss since we're testing NULL source validation
    BitsReplace(NULL, (Bits *)0x1, (Bits *)0x1);
    return true;
}

bool test_Bits_matches_null_source(void) {
    printf("Testing BitsMatches(NULL, pattern, wildcard) - should fatal\n");
    Bits pattern  = BitsInit();
    Bits wildcard = BitsInit();
    BitsPush(&pattern, true);
    BitsPush(&wildcard, false);
    BitsMatches(NULL, &pattern, &wildcard);
    BitsDeinit(&pattern);
    BitsDeinit(&wildcard);
    return true;
}

bool test_Bits_prefix_match_null_source(void) {
    printf("Testing BitsPrefixMatch(NULL, patterns, 1) - should fatal\n");
    Bits patterns[1] = {BitsInit()};
    BitsPush(&patterns[0], true);
    BitsPrefixMatch(NULL, patterns, 1);
    BitsDeinit(&patterns[0]);
    return true;
}

bool test_Bits_prefix_match_null_patterns(void) {
    printf("Testing BitsPrefixMatch(source, NULL, 1) - should fatal\n");
    Bits source = BitsInit();
    BitsPush(&source, true);
    BitsPrefixMatch(&source, NULL, 1);
    BitsDeinit(&source);
    return true;
}

bool test_Bits_suffix_match_null_source(void) {
    printf("Testing BitsSuffixMatch(NULL, patterns, 1) - should fatal\n");
    Bits patterns[1] = {BitsInit()};
    BitsPush(&patterns[0], true);
    BitsSuffixMatch(NULL, patterns, 1);
    BitsDeinit(&patterns[0]);
    return true;
}

bool test_Bits_suffix_match_null_patterns(void) {
    printf("Testing BitsSuffixMatch(source, NULL, 1) - should fatal\n");
    Bits source = BitsInit();
    BitsPush(&source, true);
    BitsSuffixMatch(&source, NULL, 1);
    BitsDeinit(&source);
    return true;
}

// Main function that runs all deadend tests
int main(void) {
    printf("[INFO] Starting Bits.Pattern.Deadend tests\n\n");

    // Deadend tests that would cause program termination
    TestFunction deadend_tests[] = {
        test_Bits_find_pattern_null_source,      test_Bits_find_pattern_null_pattern,
        test_Bits_find_last_pattern_null_source, test_Bits_find_last_pattern_null_pattern,
        test_Bits_find_all_pattern_null_source,  test_Bits_find_all_pattern_null_pattern,
        test_Bits_find_all_pattern_null_results, test_Bits_find_all_pattern_zero_max_results,
        test_Bits_starts_with_null_source,       test_Bits_starts_with_null_prefix,
        test_Bits_ends_with_null_source,         test_Bits_ends_with_null_suffix,
        test_Bits_contains_at_null_source,       test_Bits_contains_at_null_pattern,
        test_Bits_replace_null_source,           test_Bits_matches_null_source,
        test_Bits_regex_match_null_source,       test_Bits_regex_match_null_pattern,
        test_Bits_prefix_match_null_source,      test_Bits_prefix_match_null_patterns,
        test_Bits_suffix_match_null_source,      test_Bits_suffix_match_null_patterns
    };

    int total_deadend_tests = sizeof(deadend_tests) / sizeof(deadend_tests[0]);

    // Run all deadend tests using the centralized test driver
    return run_test_suite(NULL, 0, deadend_tests, total_deadend_tests, "Bits.Pattern.Deadend");
}
