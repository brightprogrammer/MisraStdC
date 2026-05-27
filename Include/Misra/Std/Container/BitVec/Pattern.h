/// file      : std/container/bitvec/pattern.h
/// author    : Generated following Misra project patterns
/// This is free and unencumbered software released into the public domain.
///
/// Pattern matching and search operations for bitvectors.

#ifndef MISRA_STD_CONTAINER_BITVEC_PATTERN_H
#define MISRA_STD_CONTAINER_BITVEC_PATTERN_H

#include "Type.h"
#include <Misra/Std/Container/Str/Type.h>
#include <Misra/Std/Zstr.h>
#include <Misra/Types.h>

#ifdef __cplusplus
extern "C" {
#endif

    ///
    /// Check if bitvector starts with the given bit pattern.
    ///
    /// bv[in]     : Bitvector to check
    /// prefix[in] : Pattern to match at the beginning
    ///
    /// SUCCESS : true if bitvector starts with the pattern
    ///
    /// USAGE:
    ///   bool starts = BitVecStartsWith(&flags, &pattern);
    ///
    /// TAGS: BitVec, Pattern, StartsWith, Match
    ///
    bool BitVecStartsWith(BitVec *bv, BitVec *prefix);

    ///
    /// Check if bitvector ends with the given bit pattern.
    ///
    /// bv[in]     : Bitvector to check
    /// suffix[in] : Pattern to match at the end
    ///
    /// SUCCESS : true if bitvector ends with the pattern
    ///
    /// USAGE:
    ///   bool ends = BitVecEndsWith(&flags, &pattern);
    ///
    /// TAGS: BitVec, Pattern, EndsWith, Match
    ///
    bool BitVecEndsWith(BitVec *bv, BitVec *suffix);

    ///
    /// Check if bitvector contains the given pattern at a specific position.
    ///
    /// bv[in]      : Bitvector to check
    /// pattern[in] : Pattern to match
    /// idx[in]     : Position to check at
    ///
    /// SUCCESS : true if pattern matches at the given position
    ///
    /// USAGE:
    ///   bool at_pos = BitVecContainsAt(&flags, &pattern, 5);
    ///
    /// TAGS: BitVec, Pattern, ContainsAt, Position
    ///
    bool BitVecContainsAt(BitVec *bv, BitVec *pattern, u64 idx);

    ///
    /// Find first occurrence of a bit pattern in the bitvector.
    ///
    /// bv[in]      : Bitvector to search in
    /// pattern[in] : Pattern to search for
    ///
    /// SUCCESS : Index of first occurrence, or SIZE_MAX if not found
    ///
    /// USAGE:
    ///   u64 index = BitVecFindPattern(&flags, &pattern);
    ///   if (index != SIZE_MAX) { /* found at index */ }
    ///
    /// TAGS: BitVec, Pattern, Find, Search
    ///
    u64 BitVecFindPattern(BitVec *bv, BitVec *pattern);

    ///
    /// Find last occurrence of a bit pattern in the bitvector.
    ///
    /// bv[in]      : Bitvector to search in
    /// pattern[in] : Pattern to search for
    ///
    /// SUCCESS : Index of last occurrence, or SIZE_MAX if not found
    ///
    /// USAGE:
    ///   u64 index = BitVecFindLastPattern(&flags, &pattern);
    ///
    /// TAGS: BitVec, Pattern, FindLast, Search
    ///
    u64 BitVecFindLastPattern(BitVec *bv, BitVec *pattern);

    ///
    /// Find all occurrences of a bit pattern in the bitvector. Two
    /// call shapes, dispatched by argument count.
    ///
    /// Raw form (caller-sized buffer):
    ///   u64 indices[10];
    ///   u64 count = BitVecFindAllPattern(&flags, &pattern, indices, 10);
    ///   // Returns count of matches written; silently truncates if
    ///   // there are more matches than `max_results`.
    ///
    /// Vec form (grows as matches are found):
    ///   BitVecMatchIndices matches = VecInitT(matches, alloc);
    ///   bool ok = BitVecFindAllPattern(&flags, &pattern, &matches);
    ///   // `matches.length` is the total count; no truncation.
    ///
    /// bv[in]              : Bitvector to search in
    /// pattern[in]         : Pattern to search for
    /// results[out]        : (raw) Array to store found indices
    /// max_results[in]     : (raw) Maximum number of results to store
    /// out[out]            : (vec) Vec to push indices into
    ///
    /// SUCCESS : (raw) Number of occurrences written into `results`.
    ///           (vec) `true`; `out` holds every match.
    /// FAILURE : (vec) `false` on allocator OOM during the walk.
    ///
    /// TAGS: BitVec, Pattern, FindAll, Search
    ///
    u64  bitvec_find_all_pattern_raw(BitVec *bv, BitVec *pattern, size *results, u64 max_results);
    bool bitvec_find_all_pattern_vec(BitVec *bv, BitVec *pattern, BitVecMatchIndices *out);

#define BitVecFindAllPattern(...)                         MISRA_OVERLOAD(BitVecFindAllPattern, __VA_ARGS__)
#define BitVecFindAllPattern_4(bv, pattern, results, max) bitvec_find_all_pattern_raw((bv), (pattern), (results), (max))
#define BitVecFindAllPattern_3(bv, pattern, out_vec)      bitvec_find_all_pattern_vec((bv), (pattern), (out_vec))

    ///
    /// Count total occurrences of a bit pattern in the bitvector.
    ///
    /// bv[in]      : Bitvector to search in
    /// pattern[in] : Pattern to count
    ///
    /// SUCCESS : Number of occurrences found
    ///
    /// USAGE:
    ///   u64 count = BitVecCountPattern(&flags, &pattern);
    ///
    /// TAGS: BitVec, Pattern, Count, Search
    ///
    u64 BitVecCountPattern(BitVec *bv, BitVec *pattern);

    ///
    /// Search for a pattern starting from a specific position (reverse search).
    ///
    /// bv[in]      : Bitvector to search in
    /// pattern[in] : Pattern to search for
    /// start[in]   : Position to start reverse search from
    ///
    /// SUCCESS : Index of occurrence, or SIZE_MAX if not found
    ///
    /// USAGE:
    ///   u64 index = BitVecRFindPattern(&flags, &pattern, 20);
    ///
    /// TAGS: BitVec, Pattern, RFind, Reverse
    ///
    u64 BitVecRFindPattern(BitVec *bv, BitVec *pattern, u64 start);

    ///
    /// Replace first occurrence of old pattern with new pattern.
    ///
    /// bv[in]          : Bitvector to modify
    /// old_pattern[in] : Pattern to find and replace
    /// new_pattern[in] : Pattern to replace with
    ///
    /// SUCCESS : true if replacement was made, false if old pattern not found
    ///
    /// USAGE:
    ///   bool replaced = BitVecReplace(&flags, &old_pat, &new_pat);
    ///
    /// TAGS: BitVec, Pattern, Replace, Modify
    ///
    bool BitVecReplace(BitVec *bv, BitVec *old_pattern, BitVec *new_pattern);

    ///
    /// Replace all occurrences of old pattern with new pattern.
    ///
    /// bv[in]          : Bitvector to modify
    /// old_pattern[in] : Pattern to find and replace
    /// new_pattern[in] : Pattern to replace with
    ///
    /// SUCCESS : Number of replacements made
    ///
    /// USAGE:
    ///   u64 count = BitVecReplaceAll(&flags, &old_pat, &new_pat);
    ///
    /// TAGS: BitVec, Pattern, ReplaceAll, Modify
    ///
    u64 BitVecReplaceAll(BitVec *bv, BitVec *old_pattern, BitVec *new_pattern);

    ///
    /// Match bitvector against pattern with wildcards.
    /// Wildcards allow flexible pattern matching where some positions can be "any bit".
    ///
    /// bv[in]       : Bitvector to match against
    /// pattern[in]  : Pattern bitvector to match
    /// wildcard[in] : Wildcard bitvector (1 = wildcard position, 0 = exact match required)
    ///
    /// SUCCESS : true if pattern matches with wildcards
    ///
    /// USAGE:
    ///   bool matches = BitVecMatches(&data, &pattern, &wildcard);
    ///
    /// TAGS: BitVec, Pattern, Match, Wildcard
    ///
    bool BitVecMatches(BitVec *bv, BitVec *pattern, BitVec *wildcard);

    ///
    /// Fuzzy pattern matching allowing up to N mismatches.
    /// Useful for approximate pattern matching with error tolerance.
    ///
    /// bv[in]         : Bitvector to search in
    /// pattern[in]    : Pattern to search for
    /// max_errors[in] : Maximum number of mismatches allowed
    ///
    /// SUCCESS : Index of first fuzzy match, or SIZE_MAX if not found
    ///
    /// USAGE:
    ///   u64 index = BitVecFuzzyMatch(&data, &pattern, 2);  // Allow 2 errors
    ///
    /// TAGS: BitVec, Pattern, Fuzzy, Approximate
    ///
    u64 BitVecFuzzyMatch(BitVec *bv, BitVec *pattern, u64 max_errors);

    ///
    /// Simple regex-style pattern matching for bitvectors.
    /// Supports basic patterns: '*' (any sequence), '?' (0 or 1), '[01]' (literal).
    ///
    /// bv[in]      : Bitvector to match against
    /// pattern[in] : Pattern string using regex syntax
    ///
    /// SUCCESS : true if bitvector matches the regex pattern
    ///
    /// USAGE:
    ///   bool matches = BitVecRegexMatch(&data, "10*01");  // 10 followed by any bits, then 01
    ///
    /// TAGS: BitVec, Pattern, Regex, Match
    ///
    bool bitvec_regex_match_zstr(BitVec *bv, Zstr pattern);
    bool bitvec_regex_match_str(BitVec *bv, const Str *pattern);
#define BitVecRegexMatch(bv, pattern)                                                                                                                                \
    _Generic((pattern), Str *: bitvec_regex_match_str, char *: bitvec_regex_match_zstr, Zstr : bitvec_regex_match_zstr)( \
        (bv),                                                                                                                                                        \
        (pattern)                                                                                                                                                    \
    )

    ///
    /// Match bitvector against an array of prefix patterns.
    /// Returns the index of the first matching prefix.
    ///
    /// bv[in]         : BitVec to check
    /// patterns[in]   : Vec(BitVec) to check prefices against
    ///
    /// SUCCESS : Index of matching pattern, or SIZE_MAX if no match
    ///
    /// USAGE:
    ///   BitVec prefixes[3] = { pattern1, pattern2, pattern3 };
    ///   u64 match = BitVecPrefixMatch(&data, prefixes, 3);
    ///
    /// TAGS: BitVec, Pattern, Prefix, Multiple
    ///
    u64 BitVecPrefixMatch(BitVec *bv, BitVecs *patterns);

    ///
    /// Match bitvector against an array of suffix patterns.
    /// Returns the index of the first matching suffix.
    ///
    /// bv[in]         : Bitvector to check
    /// patterns[in]   : Vec(BitVec) to check suffices against
    ///
    /// SUCCESS : Index of matching pattern, or SIZE_MAX if no match
    ///
    /// USAGE:
    ///   BitVec suffixes[3] = { pattern1, pattern2, pattern3 };
    ///   u64 match = BitVecSuffixMatch(&data, suffixes, 3);
    ///
    /// TAGS: BitVec, Pattern, Suffix, Multiple
    ///
    u64 BitVecSuffixMatch(BitVec *bv, BitVecs *patterns);

#ifdef __cplusplus
}
#endif

#endif // MISRA_STD_CONTAINER_BITVEC_PATTERN_H
