#include <Misra/Std/Container/BitVec.h>
#include <Misra/Std/Allocator/Default.h>
#include <Misra/Std/Log.h>

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
bool test_find_last_null_bv_aborts(void);
bool test_find_all_pattern_vec_null_bv_aborts(void);
bool test_find_all_pattern_vec_null_pattern_aborts(void);
bool test_count_pattern_null_source(void);
bool test_count_pattern_null_pattern(void);
bool test_rfind_pattern_null_source(void);
bool test_rfind_pattern_null_pattern(void);
bool test_replace_null_new_pattern_not_found(void);
bool test_replaceall_null_bv(void);
bool test_replaceall_empty_null_old(void);
bool test_replaceall_empty_null_new(void);
bool test_prefix_match_null_bv_empty_patterns(void);
bool test_suffix_match_null_bv_empty_patterns(void);

// Helper: append the bits of a 0/1 ASCII string to a BitVec.
static void push_bits(BitVec *bv, const char *bits) {
    for (const char *c = bits; *c != '\0'; c++)
        BitVecPush(bv, *c == '1');
}

// Deadend test 1: BitVecFindPattern with NULL source
bool test_bitvec_find_pattern_null_source(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVecFindPattern(NULL, pattern) - should fatal\n");

    BitVec pattern = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVecPush(&pattern, true);

    BitVecFindPattern(NULL, &pattern); // Should cause LOG_FATAL

    BitVecDeinit(&pattern);
    DefaultAllocatorDeinit(&alloc);
    return true;
}

// Deadend test 2: BitVecFindPattern with NULL pattern
bool test_bitvec_find_pattern_null_pattern(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVecFindPattern(source, NULL) - should fatal\n");

    BitVec source = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVecPush(&source, true);
    BitVecPush(&source, false);

    BitVecFindPattern(&source, NULL); // Should cause LOG_FATAL

    BitVecDeinit(&source);
    DefaultAllocatorDeinit(&alloc);
    return true;
}

// Deadend test 3: BitVecFindLastPattern with NULL source
bool test_bitvec_find_last_pattern_null_source(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVecFindLastPattern(NULL, pattern) - should fatal\n");

    BitVec pattern = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVecPush(&pattern, true);

    BitVecFindLastPattern(NULL, &pattern); // Should cause LOG_FATAL

    BitVecDeinit(&pattern);
    DefaultAllocatorDeinit(&alloc);
    return true;
}

// Deadend test 4: BitVecFindLastPattern with NULL pattern
bool test_bitvec_find_last_pattern_null_pattern(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVecFindLastPattern(source, NULL) - should fatal\n");

    BitVec source = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVecPush(&source, true);
    BitVecPush(&source, false);

    BitVecFindLastPattern(&source, NULL); // Should cause LOG_FATAL

    BitVecDeinit(&source);
    DefaultAllocatorDeinit(&alloc);
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
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVecFindAllPattern(source, NULL, results, 10) - should fatal\n");

    BitVec source = BitVecInit(ALLOCATOR_OF(&alloc));
    size   results[10];
    BitVecPush(&source, true);
    BitVecPush(&source, false);

    BitVecFindAllPattern(&source, NULL, results, 10); // Should cause LOG_FATAL

    BitVecDeinit(&source);
    DefaultAllocatorDeinit(&alloc);
    return true;
}

// Deadend test 7: BitVecFindAllPattern with NULL results
bool test_bitvec_find_all_pattern_null_results(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVecFindAllPattern(source, pattern, NULL, 10) - should fatal\n");

    BitVec source  = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVec pattern = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVecPush(&source, true);
    BitVecPush(&source, false);
    BitVecPush(&pattern, true);

    BitVecFindAllPattern(&source, &pattern, NULL, 10); // Should cause LOG_FATAL

    BitVecDeinit(&source);
    BitVecDeinit(&pattern);
    DefaultAllocatorDeinit(&alloc);
    return true;
}

// Deadend test 8: BitVecFindAllPattern with zero max_results
bool test_bitvec_find_all_pattern_zero_max_results(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVecFindAllPattern(source, pattern, results, 0) - should fatal\n");

    BitVec source  = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVec pattern = BitVecInit(ALLOCATOR_OF(&alloc));
    size   results[10];
    BitVecPush(&source, true);
    BitVecPush(&source, false);
    BitVecPush(&pattern, true);

    BitVecFindAllPattern(&source, &pattern, results, 0); // Should cause LOG_FATAL

    BitVecDeinit(&source);
    BitVecDeinit(&pattern);
    DefaultAllocatorDeinit(&alloc);
    return true;
}

// Additional deadend tests for missing Pattern functions

bool test_bitvec_starts_with_null_source(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVecStartsWith(NULL, prefix) - should fatal\n");
    BitVec prefix = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVecPush(&prefix, true);
    BitVecStartsWith(NULL, &prefix);
    BitVecDeinit(&prefix);
    DefaultAllocatorDeinit(&alloc);
    return true;
}

bool test_bitvec_starts_with_null_prefix(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVecStartsWith(source, NULL) - should fatal\n");
    BitVec source = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVecPush(&source, true);
    BitVecStartsWith(&source, NULL);
    BitVecDeinit(&source);
    DefaultAllocatorDeinit(&alloc);
    return true;
}

bool test_bitvec_ends_with_null_source(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVecEndsWith(NULL, suffix) - should fatal\n");
    BitVec suffix = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVecPush(&suffix, true);
    BitVecEndsWith(NULL, &suffix);
    BitVecDeinit(&suffix);
    DefaultAllocatorDeinit(&alloc);
    return true;
}

bool test_bitvec_ends_with_null_suffix(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVecEndsWith(source, NULL) - should fatal\n");
    BitVec source = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVecPush(&source, true);
    BitVecEndsWith(&source, NULL);
    BitVecDeinit(&source);
    DefaultAllocatorDeinit(&alloc);
    return true;
}

bool test_bitvec_contains_at_null_source(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVecContainsAt(NULL, pattern, 0) - should fatal\n");
    BitVec pattern = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVecPush(&pattern, true);
    BitVecContainsAt(NULL, &pattern, 0);
    BitVecDeinit(&pattern);
    DefaultAllocatorDeinit(&alloc);
    return true;
}

bool test_bitvec_contains_at_null_pattern(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVecContainsAt(source, NULL, 0) - should fatal\n");
    BitVec source = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVecPush(&source, true);
    BitVecContainsAt(&source, NULL, 0);
    BitVecDeinit(&source);
    DefaultAllocatorDeinit(&alloc);
    return true;
}

bool test_bitvec_replace_null_source(void) {
    WriteFmt("Testing BitVecReplace(NULL, old, new) - should fatal\n");

    // Don't create BitVecs since we're testing NULL source validation
    BitVecReplace(NULL, (BitVec *)0x1, (BitVec *)0x1);
    return true;
}

bool test_bitvec_matches_null_source(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVecMatches(NULL, pattern, wildcard) - should fatal\n");
    BitVec pattern  = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVec wildcard = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVecPush(&pattern, true);
    BitVecPush(&wildcard, false);
    BitVecMatches(NULL, &pattern, &wildcard);
    BitVecDeinit(&pattern);
    BitVecDeinit(&wildcard);
    DefaultAllocatorDeinit(&alloc);
    return true;
}

bool test_bitvec_regex_match_null_source(void) {
    WriteFmt("Testing BitVecRegexMatch(NULL, pattern) - should fatal\n");
    BitVecRegexMatch(NULL, "101");
    return true;
}

bool test_bitvec_regex_match_null_pattern(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVecRegexMatch(source, NULL) - should fatal\n");
    BitVec source = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVecPush(&source, true);
    BitVecRegexMatch(&source, (Zstr)NULL);
    BitVecDeinit(&source);
    DefaultAllocatorDeinit(&alloc);
    return true;
}

bool test_bitvec_prefix_match_null_source(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    WriteFmt("Testing BitVecPrefixMatch(NULL, patterns, 1) - should fatal\n");
    BitVecs vp = VecInitWithDeepCopy(NULL, BitVecDeinit, ALLOCATOR_OF(&alloc));
    BitVecPush(VecPtrAt(&vp, 0), true);
    BitVecPrefixMatch(NULL, &vp);
    VecDeinit(&vp);
    DefaultAllocatorDeinit(&alloc);
    return true;
}

bool test_bitvec_prefix_match_null_patterns(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVecPrefixMatch(source, NULL, 1) - should fatal\n");
    BitVec source = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVecPush(&source, true);
    BitVecPrefixMatch(&source, NULL);
    BitVecDeinit(&source);
    DefaultAllocatorDeinit(&alloc);
    return true;
}

bool test_bitvec_suffix_match_null_source(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    WriteFmt("Testing BitVecSuffixMatch(NULL, patterns, 1) - should fatal\n");
    BitVecs vp = VecInitWithDeepCopy(NULL, BitVecDeinit, ALLOCATOR_OF(&alloc));
    BitVecPush(VecPtrAt(&vp, 0), true);
    BitVecSuffixMatch(NULL, &vp);
    VecDeinit(&vp);
    DefaultAllocatorDeinit(&alloc);
    return true;
}

bool test_bitvec_suffix_match_null_patterns(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVecSuffixMatch(source, NULL, 1) - should fatal\n");
    BitVec source = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVecPush(&source, true);
    BitVecSuffixMatch(&source, NULL);
    BitVecDeinit(&source);
    DefaultAllocatorDeinit(&alloc);
    return true;
}

// 1187:5 cxx_remove_void_call -- BitVecFindLast drops `ValidateBitVec(bv)`.
// With a NULL bitvector real code aborts via the validator; deleting the call
// would let the NULL flow into `bv->length` instead.
bool test_find_last_null_bv_aborts(void) {
    WriteFmt("Testing BitVecFindLast with NULL bitvector\n");
    BitVecFindLast(NULL, true);
    return true; // Should never reach here.
}

// 1309:5 cxx_remove_void_call -- bitvec_find_all_pattern_vec drops
// `ValidateBitVec(bv)`. NULL bitvector must abort via the validator.
bool test_find_all_pattern_vec_null_bv_aborts(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *base  = ALLOCATOR_OF(&alloc);

    WriteFmt("Testing BitVecFindAllPattern (vec) with NULL bitvector\n");

    BitVec pattern = BitVecInit(base);
    BitVecPush(&pattern, true);

    BitVecMatchIndices matches = VecInitT(matches, base);
    BitVecFindAllPattern(NULL, &pattern, &matches);
    return true; // Should never reach here.
}

// 1310:5 cxx_remove_void_call -- bitvec_find_all_pattern_vec drops
// `ValidateBitVec(pattern)`. NULL pattern must abort via the validator.
bool test_find_all_pattern_vec_null_pattern_aborts(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *base  = ALLOCATOR_OF(&alloc);

    WriteFmt("Testing BitVecFindAllPattern (vec) with NULL pattern\n");

    BitVec source = BitVecInit(base);
    BitVecPush(&source, true);

    BitVecMatchIndices matches = VecInitT(matches, base);
    BitVecFindAllPattern(&source, NULL, &matches);
    return true; // Should never reach here.
}

// Mutant 1704:5 (remove_void_call) drops ValidateBitVec(bv).
bool test_count_pattern_null_source(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    BitVec pattern = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVecPush(&pattern, true);

    BitVecCountPattern(NULL, &pattern); // Should LOG_FATAL

    BitVecDeinit(&pattern);
    DefaultAllocatorDeinit(&alloc);
    return true;
}

// Mutant 1705:5 (remove_void_call) drops ValidateBitVec(pattern).
bool test_count_pattern_null_pattern(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    BitVec source = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVecPush(&source, true);

    BitVecCountPattern(&source, NULL); // Should LOG_FATAL

    BitVecDeinit(&source);
    DefaultAllocatorDeinit(&alloc);
    return true;
}

// Mutant 1722:5 (remove_void_call) drops ValidateBitVec(bv).
bool test_rfind_pattern_null_source(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    BitVec pattern = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVecPush(&pattern, true);

    BitVecRFindPattern(NULL, &pattern, 0); // Should LOG_FATAL

    BitVecDeinit(&pattern);
    DefaultAllocatorDeinit(&alloc);
    return true;
}

// Mutant 1723:5 (remove_void_call) drops ValidateBitVec(pattern).
bool test_rfind_pattern_null_pattern(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    BitVec source = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVecPush(&source, true);

    BitVecRFindPattern(&source, NULL, 0); // Should LOG_FATAL

    BitVecDeinit(&source);
    DefaultAllocatorDeinit(&alloc);
    return true;
}

// 1744:5 cxx_remove_void_call -- removing ValidateBitVec(new_pattern) in
// BitVecReplace. Real validates new_pattern up front and aborts on NULL. The
// mutant skips it; with old_pattern absent the function returns false before
// ever touching new_pattern, so the abort is lost.
bool test_replace_null_new_pattern_not_found(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVecReplace(src, old, NULL) with old absent - should fatal\n");

    BitVec source      = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVec old_pattern = BitVecInit(ALLOCATOR_OF(&alloc));
    push_bits(&source, "0000");
    push_bits(&old_pattern, "111");             // not present in source

    BitVecReplace(&source, &old_pattern, NULL); // real: LOG_FATAL

    BitVecDeinit(&source);
    BitVecDeinit(&old_pattern);
    DefaultAllocatorDeinit(&alloc);
    return true;
}

// 1763:5 cxx_remove_void_call -- removing ValidateBitVec(bv) in
// BitVecReplaceAll. A zero-initialized (uninitialized) BitVec fails the magic
// check, so real aborts. The mutant reads length 0, never enters the loop, and
// returns 0 without aborting.
bool test_replaceall_null_bv(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVecReplaceAll(uninitialized, old, new) - should fatal\n");

    BitVec bad         = {0}; // magic mismatch -> ValidateBitVec aborts
    BitVec old_pattern = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVec new_pattern = BitVecInit(ALLOCATOR_OF(&alloc));
    push_bits(&old_pattern, "1");
    push_bits(&new_pattern, "0");

    BitVecReplaceAll(&bad, &old_pattern, &new_pattern); // real: LOG_FATAL

    BitVecDeinit(&old_pattern);
    BitVecDeinit(&new_pattern);
    DefaultAllocatorDeinit(&alloc);
    return true;
}

// 1764:5 cxx_remove_void_call -- removing ValidateBitVec(old_pattern) in
// BitVecReplaceAll. With an EMPTY bv the while loop body never runs, so the
// later BitVecContainsAt (which would re-validate old_pattern) is never
// reached. Real aborts on the NULL old_pattern up front; the mutant returns 0.
bool test_replaceall_empty_null_old(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVecReplaceAll(empty, NULL, new) - should fatal\n");

    BitVec source      = BitVecInit(ALLOCATOR_OF(&alloc)); // empty
    BitVec new_pattern = BitVecInit(ALLOCATOR_OF(&alloc));
    push_bits(&new_pattern, "0");

    BitVecReplaceAll(&source, NULL, &new_pattern); // real: LOG_FATAL

    BitVecDeinit(&source);
    BitVecDeinit(&new_pattern);
    DefaultAllocatorDeinit(&alloc);
    return true;
}

// 1765:5 cxx_remove_void_call -- removing ValidateBitVec(new_pattern) in
// BitVecReplaceAll. new_pattern is only touched after a match. With an EMPTY bv
// there is no match, so the mutant never reaches new_pattern. Real validates it
// up front and aborts on NULL.
bool test_replaceall_empty_null_new(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVecReplaceAll(empty, old, NULL) - should fatal\n");

    BitVec source      = BitVecInit(ALLOCATOR_OF(&alloc)); // empty
    BitVec old_pattern = BitVecInit(ALLOCATOR_OF(&alloc));
    push_bits(&old_pattern, "1");

    BitVecReplaceAll(&source, &old_pattern, NULL); // real: LOG_FATAL

    BitVecDeinit(&source);
    BitVecDeinit(&old_pattern);
    DefaultAllocatorDeinit(&alloc);
    return true;
}

// Kills 1804:cxx_remove_void_call -- ValidateBitVec(pattern) in BitVecMatches.
// Real code aborts on a NULL pattern; the mutant skips the guard and derefs.
static bool test_matches_null_pattern_aborts(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *base  = ALLOCATOR_OF(&alloc);

    BitVec source   = BitVecInit(base);
    BitVec wildcard = BitVecInit(base);
    BitVecPush(&source, true);
    BitVecPush(&wildcard, false);

    BitVecMatches(&source, NULL, &wildcard); // must abort

    BitVecDeinit(&source);
    BitVecDeinit(&wildcard);
    DefaultAllocatorDeinit(&alloc);
    return false;
}

// Kills 1805:cxx_remove_void_call -- ValidateBitVec(wildcard) in BitVecMatches.
static bool test_matches_null_wildcard_aborts(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *base  = ALLOCATOR_OF(&alloc);

    BitVec source  = BitVecInit(base);
    BitVec pattern = BitVecInit(base);
    BitVecPush(&source, true);
    BitVecPush(&pattern, true);

    BitVecMatches(&source, &pattern, NULL); // must abort

    BitVecDeinit(&source);
    BitVecDeinit(&pattern);
    DefaultAllocatorDeinit(&alloc);
    return false;
}

// Kills 1824:cxx_remove_void_call -- ValidateBitVec(bv) in BitVecFuzzyMatch.
static bool test_fuzzy_null_source_aborts(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *base  = ALLOCATOR_OF(&alloc);

    BitVec pattern = BitVecInit(base);
    BitVecPush(&pattern, true);

    BitVecFuzzyMatch(NULL, &pattern, 0); // must abort

    BitVecDeinit(&pattern);
    DefaultAllocatorDeinit(&alloc);
    return false;
}

// Kills 1825:cxx_remove_void_call -- ValidateBitVec(pattern) in
// BitVecFuzzyMatch.
static bool test_fuzzy_null_pattern_aborts(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *base  = ALLOCATOR_OF(&alloc);

    BitVec source = BitVecInit(base);
    BitVecPush(&source, true);

    BitVecFuzzyMatch(&source, NULL, 0); // must abort

    BitVecDeinit(&source);
    DefaultAllocatorDeinit(&alloc);
    return false;
}

// BitVecPrefixMatch: `ValidateBitVec(bv);` (1888:cxx_remove_void_call) is the
// ONLY validation of `bv` when the pattern list is empty (the loop body that
// re-validates via BitVecStartsWith never executes). Removing it lets a NULL bv
// return SIZE_MAX instead of aborting. Deadend: a NULL bv must abort.
bool test_prefix_match_null_bv_empty_patterns(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVecPrefixMatch(NULL, empty) - should fatal\n");

    BitVecs patterns = VecInitWithDeepCopy(NULL, BitVecDeinit, ALLOCATOR_OF(&alloc));

    // Empty patterns: with validation present this aborts on the NULL bv;
    // with validation removed the empty loop falls through to SIZE_MAX.
    BitVecPrefixMatch(NULL, &patterns);

    VecDeinit(&patterns);
    DefaultAllocatorDeinit(&alloc);
    return true;
}

// BitVecSuffixMatch: `ValidateBitVec(bv);` (1903:cxx_remove_void_call) is the
// ONLY validation of `bv` when the pattern list is empty (the loop body that
// re-validates via BitVecEndsWith never executes). Removing it lets a NULL bv
// return SIZE_MAX instead of aborting. Deadend: a NULL bv must abort.
bool test_suffix_match_null_bv_empty_patterns(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    WriteFmt("Testing BitVecSuffixMatch(NULL, empty) - should fatal\n");

    BitVecs patterns = VecInitWithDeepCopy(NULL, BitVecDeinit, ALLOCATOR_OF(&alloc));

    // Empty patterns: with validation present this aborts on the NULL bv;
    // with validation removed the empty loop falls through to SIZE_MAX.
    BitVecSuffixMatch(NULL, &patterns);

    VecDeinit(&patterns);
    DefaultAllocatorDeinit(&alloc);
    return true;
}

// Main function that runs all deadend tests
int main(void) {
    WriteFmt("[INFO] Starting BitVec.Pattern.Deadend tests\n\n");

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
        test_bitvec_suffix_match_null_patterns,
        test_find_last_null_bv_aborts,
        test_find_all_pattern_vec_null_bv_aborts,
        test_find_all_pattern_vec_null_pattern_aborts,
        test_count_pattern_null_source,
        test_count_pattern_null_pattern,
        test_rfind_pattern_null_source,
        test_rfind_pattern_null_pattern,
        test_replace_null_new_pattern_not_found,
        test_replaceall_null_bv,
        test_replaceall_empty_null_old,
        test_replaceall_empty_null_new,
        test_matches_null_pattern_aborts,
        test_matches_null_wildcard_aborts,
        test_fuzzy_null_source_aborts,
        test_fuzzy_null_pattern_aborts,
        test_prefix_match_null_bv_empty_patterns,
        test_suffix_match_null_bv_empty_patterns
    };

    int total_deadend_tests = sizeof(deadend_tests) / sizeof(deadend_tests[0]);

    // Run all deadend tests using the centralized test driver
    return run_test_suite(NULL, 0, deadend_tests, total_deadend_tests, "BitVec.Pattern.Deadend");
}
