/// file      : std/container/Bits/pattern.h
/// author    : Generated following Misra project patterns
/// This is free and unencumbered software released into the public domain.
///
/// Pattern matching and search operations for Bitstors.

#ifndef MISRA_STD_CONTAINER_Bits_PATTERN_H
#define MISRA_STD_CONTAINER_Bits_PATTERN_H

#include "Type.h"
#include <Misra/Types.h>
#include <Misra/Std/Container/Vec.h>

#ifdef __cplusplus
extern "C" {
#endif

    typedef Vec(u64) Indices;

    ///
    /// Check if Bitstor starts with the given bit pattern.
    ///
    /// bv[in]     : Bitstor to check
    /// prefix[in] : Pattern to match at the beginning
    ///
    /// RETURNS: true if Bitstor starts with the pattern
    ///
    /// USAGE:
    ///   bool starts = BitsStartsWith(&flags, &pattern);
    ///
    /// TAGS: Bits, Pattern, StartsWith, Match
    ///
    bool BitsStartsWith(Bits *bv, Bits *prefix);

    ///
    /// Check if Bitstor ends with the given bit pattern.
    ///
    /// bv[in]     : Bitstor to check
    /// suffix[in] : Pattern to match at the end
    ///
    /// RETURNS: true if Bitstor ends with the pattern
    ///
    /// USAGE:
    ///   bool ends = BitsEndsWith(&flags, &pattern);
    ///
    /// TAGS: Bits, Pattern, EndsWith, Match
    ///
    bool BitsEndsWith(Bits *bv, Bits *suffix);

    ///
    /// Check if Bitstor contains the given bit pattern anywhere.
    ///
    /// bv[in]      : Bitstor to search in
    /// pattern[in] : Pattern to search for
    ///
    /// RETURNS: true if pattern is found anywhere in the Bitstor
    ///
    /// USAGE:
    ///   bool contains = BitsContains(&flags, &pattern);
    ///
    /// TAGS: Bits, Pattern, Contains, Search
    ///
    bool BitsContains(Bits *bv, Bits *pattern);

    ///
    /// Check if Bitstor contains the given pattern at a specific position.
    ///
    /// bv[in]      : Bitstor to check
    /// pattern[in] : Pattern to match
    /// idx[in]     : Position to check at
    ///
    /// RETURNS: true if pattern matches at the given position
    ///
    /// USAGE:
    ///   bool at_pos = BitsContainsAt(&flags, &pattern, 5);
    ///
    /// TAGS: Bits, Pattern, ContainsAt, Position
    ///
    bool BitsContainsAfter(Bits *bv, Bits *pattern, u64 idx);

    ///
    /// Find first occurrence of a bit pattern in the Bitstor.
    ///
    /// bv[in]      : Bitstor to search in
    /// pattern[in] : Pattern to search for
    ///
    /// RETURNS: Index of first occurrence, or SIZE_MAX if not found
    ///
    /// USAGE:
    ///   u64 index = BitsFindPattern(&flags, &pattern);
    ///   if (index != SIZE_MAX) { /* found at index */ }
    ///
    /// TAGS: Bits, Pattern, Find, Search
    ///
    u64 BitsFindPattern(Bits *bv, Bits *pattern);

    ///
    /// Find last occurrence of a bit pattern in the Bitstor.
    ///
    /// bv[in]      : Bitstor to search in
    /// pattern[in] : Pattern to search for
    ///
    /// RETURNS: Index of last occurrence, or SIZE_MAX if not found
    ///
    /// USAGE:
    ///   u64 index = BitsFindLastPattern(&flags, &pattern);
    ///
    /// TAGS: Bits, Pattern, FindLast, Search
    ///
    u64 BitsFindLastPattern(Bits *bv, Bits *pattern);

    ///
    /// Find all occurrences of a bit pattern in the Bitstor.
    /// Results array must be pre-allocated with sufficient space.
    ///
    /// bv[in]       : Bitstor to search in
    /// pattern[in]  : Pattern to search for
    /// results[out] : Array to store found indices
    /// max_results[in]: Maximum number of results to store
    ///
    /// RETURNS: Number of occurrences found
    ///
    /// USAGE:
    ///   u64 indices[10];
    ///   u64 count = BitsFindAllPattern(&flags, &pattern, indices, 10);
    ///
    /// TAGS: Bits, Pattern, FindAll, Search
    ///
    Indices BitsFindAllPattern(Bits *bv, Bits *pattern);

    ///
    /// Count total occurrences of a bit pattern in the Bitstor.
    ///
    /// bv[in]      : Bitstor to search in
    /// pattern[in] : Pattern to count
    ///
    /// RETURNS: Number of occurrences found
    ///
    /// USAGE:
    ///   u64 count = BitsCountPattern(&flags, &pattern);
    ///
    /// TAGS: Bits, Pattern, Count, Search
    ///
    u64 BitsCountPattern(Bits *bv, Bits *pattern);

    ///
    /// Search for a pattern starting from a specific position (reverse search).
    ///
    /// bv[in]      : Bitstor to search in
    /// pattern[in] : Pattern to search for
    /// start[in]   : Position to start reverse search from
    ///
    /// RETURNS: Index of occurrence, or SIZE_MAX if not found
    ///
    /// USAGE:
    ///   u64 index = BitsRFindPattern(&flags, &pattern, 20);
    ///
    /// TAGS: Bits, Pattern, RFind, Reverse
    ///
    u64 BitsRFindPattern(Bits *bv, Bits *pattern, u64 start);

    ///
    /// Replace first occurrence of old pattern with new pattern.
    ///
    /// bv[in]          : Bitstor to modify
    /// old_pattern[in] : Pattern to find and replace
    /// new_pattern[in] : Pattern to replace with
    ///
    /// RETURNS: true if replacement was made, false if old pattern not found
    ///
    /// USAGE:
    ///   bool replaced = BitsReplace(&flags, &old_pat, &new_pat);
    ///
    /// TAGS: Bits, Pattern, Replace, Modify
    ///
    bool BitsReplace(Bits *bv, Bits *old_pattern, Bits *new_pattern);

    ///
    /// Replace all occurrences of old pattern with new pattern.
    ///
    /// bv[in]          : Bitstor to modify
    /// old_pattern[in] : Pattern to find and replace
    /// new_pattern[in] : Pattern to replace with
    ///
    /// RETURNS: Number of replacements made
    ///
    /// USAGE:
    ///   u64 count = BitsReplaceAll(&flags, &old_pat, &new_pat);
    ///
    /// TAGS: Bits, Pattern, ReplaceAll, Modify
    ///
    u64 BitsReplaceAll(Bits *bv, Bits *old_pattern, Bits *new_pattern);

    ///
    /// Match Bitstor against pattern with wildcards.
    /// Wildcards allow flexible pattern matching where some positions can be "any bit".
    ///
    /// bv[in]       : Bitstor to match against
    /// pattern[in]  : Pattern Bitstor to match
    /// wildcard[in] : Wildcard Bitstor (1 = wildcard position, 0 = exact match required)
    ///
    /// RETURNS: true if pattern matches with wildcards
    ///
    /// USAGE:
    ///   bool matches = BitsMatches(&data, &pattern, &wildcard);
    ///
    /// TAGS: Bits, Pattern, Match, Wildcard
    ///
    bool BitsMatches(Bits *bv, Bits *pattern, Bits *wildcard);

    ///
    /// Fuzzy pattern matching allowing up to N mismatches.
    /// Useful for approximate pattern matching with error tolerance.
    ///
    /// bv[in]         : Bitstor to search in
    /// pattern[in]    : Pattern to search for
    /// max_errors[in] : Maximum number of mismatches allowed
    ///
    /// RETURNS: Index of first fuzzy match, or SIZE_MAX if not found
    ///
    /// USAGE:
    ///   u64 index = BitsFuzzyMatch(&data, &pattern, 2);  // Allow 2 errors
    ///
    /// TAGS: Bits, Pattern, Fuzzy, Approximate
    ///
    u64 BitsFuzzyMatch(Bits *bv, Bits *pattern, u64 max_errors);

    ///
    /// Match Bitstor against an array of prefix patterns.
    /// Returns the index of the first matching prefix.
    ///
    /// bv[in]         : Bitstor to check
    /// patterns[in]   : Array of prefix patterns to match against
    /// num_patterns[in]: Number of patterns in the array
    ///
    /// RETURNS: Index of matching pattern, or SIZE_MAX if no match
    ///
    /// USAGE:
    ///   Bits prefixes[3] = { pattern1, pattern2, pattern3 };
    ///   u64 match = BitsPrefixMatch(&data, prefixes, 3);
    ///
    /// TAGS: Bits, Pattern, Prefix, Multiple
    ///
    u64 BitsPrefixMatch(Bits *bv, Bits *patterns, u64 num_patterns);

    ///
    /// Match Bitstor against an array of suffix patterns.
    /// Returns the index of the first matching suffix.
    ///
    /// bv[in]         : Bitstor to check
    /// patterns[in]   : Array of suffix patterns to match against
    /// num_patterns[in]: Number of patterns in the array
    ///
    /// RETURNS: Index of matching pattern, or SIZE_MAX if no match
    ///
    /// USAGE:
    ///   Bits suffixes[3] = { pattern1, pattern2, pattern3 };
    ///   u64 match = BitsSuffixMatch(&data, suffixes, 3);
    ///
    /// TAGS: Bits, Pattern, Suffix, Multiple
    ///
    u64 BitsSuffixMatch(Bits *bv, Bits *patterns, u64 num_patterns);

#ifdef __cplusplus
}
#endif

#endif // MISRA_STD_CONTAINER_Bits_PATTERN_H

