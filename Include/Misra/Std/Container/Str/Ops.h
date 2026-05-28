/// file      : std/container/str/ops.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// String comparison, find, and modification operations for Str

#ifndef MISRA_STD_CONTAINER_STR_OPS_H
#define MISRA_STD_CONTAINER_STR_OPS_H

#include "Access.h"
#include "Private.h"
#include "Type.h"
#include <Misra/Std/Utility/StrIter.h>
#include <Misra/Std/Zstr.h>

#ifdef __cplusplus
extern "C" {
#endif

    //
    // Generic-callback helpers (used as Map / Vec callback slots).
    //

    ///
    /// Hash a `Str` for use as a generic map key. FNV-1a over the byte view.
    ///
    /// data[in] : Pointer to a `Str`.
    /// size[in] : Ignored. Included for `GenericHash` callback compatibility.
    ///
    /// SUCCESS : Returns a stable hash of the string's bytes. `data`
    ///           is not modified.
    /// FAILURE : Function cannot fail.
    ///
    /// TAGS: Str, Hash, Ops
    ///
    u64 str_hash(const void *data, u32 size);

    ///
    /// Compare two `Str` values lexicographically. Shape matches
    /// `GenericCompare` so it can drop into map / vec compare slots.
    ///
    /// lhs[in] : Pointer to the left `Str`.
    /// rhs[in] : Pointer to the right `Str`.
    ///
    /// SUCCESS : Returns `0` when equal, `<0` when `lhs < rhs`, `>0`
    ///           when `lhs > rhs`. Neither operand is modified.
    /// FAILURE : Function cannot fail.
    ///
    /// TAGS: Str, Compare, Ops
    ///
    i32 str_compare(const void *lhs, const void *rhs);

//
// Comparison Operations
//

///
/// Compare a `Str` against another string lexicographically.
///
/// Three call shapes via `OVERLOAD` + `_Generic` on `other`:
///   `StrCmp(s, other)`              -- `other` is `Str *` or `Zstr`.
///   `StrCmp(s, other, other_len)`   -- `other` is a fixed-length view
///                                      (`Zstr`, `size`).
/// The `Str *` overload is length-aware on both sides, so a string with
/// an embedded NUL still compares correctly.
///
/// s[in]         : Str to compare.
/// other[in]     : Other string (`Str *` / `Zstr`).
/// other_len[in] : Length of `other` for the 3-arg fixed-length form.
///
/// SUCCESS : Returns `0` when equal, `<0` when `s < other`, `>0` when
///           `s > other`. Neither string is modified.
/// FAILURE : Function cannot fail; the return value carries the order.
///
/// TAGS: Str, Compare
///
#define StrCmp(...) OVERLOAD(StrCmp, __VA_ARGS__)
#define StrCmp_2(s, other)                                                                                             \
    _Generic(                                                                                                          \
        (other),                                                                                                       \
        Str *: str_cmp_str ((s), (const Str *)(other)),                                                                \
        Zstr:  str_cmp_zstr((s), (Zstr)(other)),                                                                        \
        char *: str_cmp_zstr((s), (Zstr)(other))                                                                        \
    )
#define StrCmp_3(s, other, other_len) str_cmp_cstr((s), (Zstr)(other), (other_len))

///
/// Case-insensitive (ASCII) comparison of a `Str` against another string.
/// Non-ASCII bytes are compared verbatim; there is no Unicode case folding.
///
/// Three call shapes via `OVERLOAD` + `_Generic` on `other`:
///   `StrCmpIgnoreCase(s, other)`              -- `other` is `Str *` or
///                                                `Zstr`.
///   `StrCmpIgnoreCase(s, other, other_len)`   -- `other` is a
///                                                fixed-length view
///                                                (`Zstr`, `size`).
///
/// s[in]         : Str to compare.
/// other[in]     : Other string (`Str *` / `Zstr`).
/// other_len[in] : Length of `other` for the 3-arg fixed-length form.
///
/// SUCCESS : Returns `0` when equal, `<0` when `s < other`, `>0` when
///           `s > other` (ASCII case-folded). Neither string is modified.
/// FAILURE : Function cannot fail; the return value carries the order.
///
/// TAGS: Str, Compare, IgnoreCase
///
#define StrCmpIgnoreCase(...) OVERLOAD(StrCmpIgnoreCase, __VA_ARGS__)
#define StrCmpIgnoreCase_2(s, other)                                                                                   \
    _Generic(                                                                                                          \
        (other),                                                                                                       \
        Str *: str_cmp_str_ignore_case ((s), (const Str *)(other)),                                                    \
        Zstr:  str_cmp_zstr_ignore_case((s), (Zstr)(other)),                                                            \
        char *: str_cmp_zstr_ignore_case((s), (Zstr)(other))                                                            \
    )
#define StrCmpIgnoreCase_3(s, other, other_len) str_cmp_cstr_ignore_case((s), (Zstr)(other), (other_len))

//
// Find Operations
//

///
/// Find the first occurrence of `key` inside `s` and return a pointer
/// into `s`'s buffer.
///
/// Three call shapes via `OVERLOAD` + `_Generic` on `key`:
///   `StrFind(s, key)`              -- `key` is `Str *` or `Zstr`.
///   `StrFind(s, key, key_len)`     -- `key` is a fixed-length view
///                                     (`Zstr`, `size`).
/// Differs from `StrIndexOf` (which returns a `size` index); this
/// returns a `Zstr` pointer into `s`'s storage at the match.
///
/// s[in]       : Str to search in.
/// key[in]     : Substring to search for (`Str *` / `Zstr`).
/// key_len[in] : Length of `key` when using the 3-arg fixed-length form.
///
/// SUCCESS : Returns a `Zstr` pointing at the first match inside `s`.
///           The string is not modified.
/// FAILURE : Returns `NULL` when no match is found. The string is not
///           modified.
///
/// TAGS: Str, Find, Search
///
#define StrFind(...) OVERLOAD(StrFind, __VA_ARGS__)
#define StrFind_2(s, key)                                                                                              \
    _Generic(                                                                                                          \
        (key),                                                                                                         \
        Str *: str_find_str ((s), (const Str *)(key)),                                                                 \
        Zstr:  str_find_zstr((s), (Zstr)(key)),                                                                         \
        char *: str_find_zstr((s), (Zstr)(key))                                                                         \
    )
#define StrFind_3(s, key, key_len) str_find_cstr((s), (Zstr)(key), (key_len))

///
/// Find the index of the first occurrence of a key inside a `Str`.
///
/// Three call shapes via `OVERLOAD` + `_Generic` on `key`:
///   `StrIndexOf(s, key)`              -- `key` is `Str *` or `Zstr`.
///   `StrIndexOf(s, key, key_len)`     -- `key` is a fixed-length view
///                                        (`Zstr`, `size`).
/// An empty key matches at position 0.
///
/// s[in]       : Str object to search in.
/// key[in]     : Substring to search for (`Str *` / `Zstr`).
/// key_len[in] : Length of `key` when using the 3-arg fixed-length form.
///
/// SUCCESS : Returns the zero-based index of the first match. The
///           string is not modified.
/// FAILURE : Returns `SIZE_MAX` when no match is found. The string is
///           not modified.
///
/// TAGS: Str, IndexOf, Search
///
#define StrIndexOf(...) OVERLOAD(StrIndexOf, __VA_ARGS__)
#define StrIndexOf_2(s, key)                                                                                           \
    _Generic(                                                                                                          \
        (key),                                                                                                         \
        Str *: str_index_of_str ((s), (const Str *)(key)),                                                             \
        Zstr:  str_index_of_zstr((s), (Zstr)(key)),                                                                     \
        char *: str_index_of_zstr((s), (Zstr)(key))                                                                     \
    )
#define StrIndexOf_3(s, key, key_len) str_index_of_cstr((s), (Zstr)(key), (key_len))

///
/// Check whether a `Str` contains a key.
///
/// Three call shapes via `OVERLOAD` + `_Generic` on `key`:
///   `StrContains(s, key)`              -- `key` is `Str *` or `Zstr`.
///   `StrContains(s, key, key_len)`     -- `key` is a fixed-length view
///                                         (`Zstr`, `size`).
/// An empty key trivially matches.
///
/// s[in]       : Str object to search in.
/// key[in]     : Substring to search for (`Str *` / `Zstr`).
/// key_len[in] : Length of `key` when using the 3-arg fixed-length form.
///
/// SUCCESS : Returns `true` when a match exists. The string is not
///           modified.
/// FAILURE : Returns `false`. The string is not modified.
///
/// TAGS: Str, Contains, Search
///
#define StrContains(...) OVERLOAD(StrContains, __VA_ARGS__)
#define StrContains_2(s, key)                                                                                          \
    _Generic(                                                                                                          \
        (key),                                                                                                         \
        Str *: str_contains_str ((s), (const Str *)(key)),                                                             \
        Zstr:  str_contains_zstr((s), (Zstr)(key)),                                                                     \
        char *: str_contains_zstr((s), (Zstr)(key))                                                                     \
    )
#define StrContains_3(s, key, key_len) str_contains_cstr((s), (Zstr)(key), (key_len))

//
// Prefix/Suffix Operations
//

///
/// Check whether a `Str` starts with a prefix.
///
/// Three call shapes via `OVERLOAD` + `_Generic` on `prefix`:
///   `StrStartsWith(s, prefix)`                -- `prefix` is `Str *`
///                                                or `Zstr`.
///   `StrStartsWith(s, prefix, prefix_len)`    -- `prefix` is a
///                                                fixed-length view
///                                                (`Zstr`, `size`).
///
/// s[in]          : Str to check.
/// prefix[in]     : Candidate prefix (`Str *` / `Zstr`).
/// prefix_len[in] : Length of `prefix` when using the 3-arg
///                  fixed-length form.
///
/// SUCCESS : Returns `true` when `s` starts with `prefix`. The string
///           is not modified.
/// FAILURE : Returns `false`. The string is not modified.
///
/// TAGS: Str, StartsWith, Prefix
///
#define StrStartsWith(...) OVERLOAD(StrStartsWith, __VA_ARGS__)
#define StrStartsWith_2(s, prefix)                                                                                     \
    _Generic(                                                                                                          \
        (prefix),                                                                                                      \
        Str *: str_starts_with_str ((s), (const Str *)(prefix)),                                                       \
        Zstr:  str_starts_with_zstr((s), (Zstr)(prefix)),                                                               \
        char *: str_starts_with_zstr((s), (Zstr)(prefix))                                                               \
    )
#define StrStartsWith_3(s, prefix, prefix_len) str_starts_with_cstr((s), (Zstr)(prefix), (prefix_len))

///
/// Check whether a `Str` ends with a suffix.
///
/// Three call shapes via `OVERLOAD` + `_Generic` on `suffix`:
///   `StrEndsWith(s, suffix)`                -- `suffix` is `Str *`
///                                              or `Zstr`.
///   `StrEndsWith(s, suffix, suffix_len)`    -- `suffix` is a
///                                              fixed-length view
///                                              (`Zstr`, `size`).
///
/// s[in]          : Str to check.
/// suffix[in]     : Candidate suffix (`Str *` / `Zstr`).
/// suffix_len[in] : Length of `suffix` when using the 3-arg
///                  fixed-length form.
///
/// SUCCESS : Returns `true` when `s` ends with `suffix`. The string is
///           not modified.
/// FAILURE : Returns `false`. The string is not modified.
///
/// TAGS: Str, EndsWith, Suffix
///
#define StrEndsWith(...) OVERLOAD(StrEndsWith, __VA_ARGS__)
#define StrEndsWith_2(s, suffix)                                                                                       \
    _Generic(                                                                                                          \
        (suffix),                                                                                                      \
        Str *: str_ends_with_str ((s), (const Str *)(suffix)),                                                         \
        Zstr:  str_ends_with_zstr((s), (Zstr)(suffix)),                                                                 \
        char *: str_ends_with_zstr((s), (Zstr)(suffix))                                                                 \
    )
#define StrEndsWith_3(s, suffix, suffix_len) str_ends_with_cstr((s), (Zstr)(suffix), (suffix_len))

//
// Replace Operations
//

///
/// Replace occurrences of `match` in `s` with `replacement`.
///
/// Three call shapes via `OVERLOAD` + `_Generic` on `match`:
///   `StrReplace(s, match, replacement, count)`
///       -- `match` and `replacement` are both `Str *` or both `Zstr`.
///   `StrReplace(s, match, match_len, replacement, replacement_len, count)`
///       -- `match` / `replacement` are fixed-length views
///          (`Zstr`, `size`).
/// `count = -1` requests replace-all; otherwise at most `count`
/// occurrences are replaced.
///
/// s[in,out]           : Str to modify in place.
/// match[in]           : Pattern to find (`Str *` / `Zstr`).
/// match_len[in]       : Length of `match` for the 6-arg form.
/// replacement[in]     : Replacement bytes (`Str *` / `Zstr`).
/// replacement_len[in] : Length of `replacement` for the 6-arg form.
/// count[in]           : Maximum number of replacements; `-1` for all.
///
/// SUCCESS : Modifies `s` in place. Returns no value.
/// FAILURE : `s` is left unchanged when `match` does not occur in `s`.
///
/// TAGS: Str, Replace
///
#define StrReplace(...) OVERLOAD(StrReplace, __VA_ARGS__)
#define StrReplace_4(s, match, replacement, count)                                                                     \
    _Generic(                                                                                                          \
        (match),                                                                                                       \
        Str *: str_replace_str ((s), (const Str *)(match), (const Str *)(replacement), (count)),                       \
        Zstr:  str_replace_zstr((s), (Zstr)(match), (Zstr)(replacement), (count)),                                      \
        char *: str_replace_zstr((s), (Zstr)(match), (Zstr)(replacement), (count))                                      \
    )
#define StrReplace_6(s, match, match_len, replacement, replacement_len, count)                                         \
    str_replace_cstr((s), (Zstr)(match), (match_len), (Zstr)(replacement), (replacement_len), (count))

//
// Split Operations
//

///
/// Split `s` into a vector of `StrIter` views over the original `s`
/// buffer, delimited by `key`. No new strings are allocated -- each
/// iterator borrows a slice of `s`. Best used when the caller only
/// needs to iterate the pieces without mutating them.
///
/// Two call shapes via `_Generic` on `key`:
///   `StrSplitToIters(s, key)`     -- `key` is `Str *` or `Zstr`.
///
/// s[in]   : Str to split.
/// key[in] : Delimiter (`Str *` / `Zstr`).
///
/// SUCCESS : Returns a non-empty `StrIters` vector whose entries view
///           slices of `s`. Caller owns the vector and must `VecDeinit`
///           it. `s` is not modified.
/// FAILURE : Returns a zero-length `StrIters` vector. `s` is not
///           modified.
///
/// TAGS: Str, Split, Iter
///
#define StrSplitToIters(s, key)                                                                                        \
    _Generic(                                                                                                          \
        (key),                                                                                                         \
        Str *: str_split_to_iters_str ((s), (const Str *)(key)),                                                       \
        Zstr:  str_split_to_iters_zstr((s), (Zstr)(key)),                                                               \
        char *: str_split_to_iters_zstr((s), (Zstr)(key))                                                               \
    )

///
/// Split `s` into a vector of new `Str` objects delimited by `key`.
/// Each returned `Str` owns its own storage and is independently
/// modifiable. Use when callers will further mutate the pieces.
///
/// Two call shapes via `_Generic` on `key`:
///   `StrSplit(s, key)`     -- `key` is `Str *` or `Zstr`.
///
/// s[in]   : Str to split.
/// key[in] : Delimiter (`Str *` / `Zstr`).
///
/// SUCCESS : Returns a non-empty `Strs` vector. Caller owns the vector
///           and must `VecDeinit` it; that releases each contained
///           `Str` as well. `s` is not modified.
/// FAILURE : Returns a zero-length `Strs` vector. `s` is not modified.
///
/// TAGS: Str, Split
///
#define StrSplit(s, key)                                                                                               \
    _Generic(                                                                                                          \
        (key),                                                                                                         \
        Str *: str_split_str ((s), (const Str *)(key)),                                                                \
        Zstr:  str_split_zstr((s), (Zstr)(key)),                                                                        \
        char *: str_split_zstr((s), (Zstr)(key))                                                                        \
    )

//
// Strip Operations
//

///
/// Strip leading and trailing characters from `s`, returning a new
/// `Str`. The original `s` is unmodified. The returned `Str` owns its
/// storage and must be deinited after use.
///
/// s[in]              : Str to strip.
/// chars_to_strip[in] : Optional `Zstr` listing characters to strip.
///                      `NULL` strips standard ASCII whitespace.
///
/// SUCCESS : Returns a new `Str` with surrounding characters removed.
///           `s` is not modified.
/// FAILURE : Returns a zero-length `Str` when the whole input strips
///           away or on allocator OOM.
///
/// TAGS: Str, Strip
///
#define StrStrip(s, chars_to_strip) strip_str((s), (chars_to_strip), 0)

///
/// Strip only leading characters from `s`, returning a new `Str`.
/// The original `s` is unmodified.
///
/// s[in]              : Str to strip.
/// chars_to_strip[in] : Optional `Zstr` listing characters to strip.
///                      `NULL` strips standard ASCII whitespace.
///
/// SUCCESS : Returns a new `Str` with leading characters removed.
///           `s` is not modified.
/// FAILURE : Returns a zero-length `Str` when the whole input strips
///           away or on allocator OOM.
///
/// TAGS: Str, Strip, LStrip
///
#define StrLStrip(s, chars_to_strip) strip_str((s), (chars_to_strip), -1)

///
/// Strip only trailing characters from `s`, returning a new `Str`.
/// The original `s` is unmodified.
///
/// s[in]              : Str to strip.
/// chars_to_strip[in] : Optional `Zstr` listing characters to strip.
///                      `NULL` strips standard ASCII whitespace.
///
/// SUCCESS : Returns a new `Str` with trailing characters removed.
///           `s` is not modified.
/// FAILURE : Returns a zero-length `Str` when the whole input strips
///           away or on allocator OOM.
///
/// TAGS: Str, Strip, RStrip
///
#define StrRStrip(s, chars_to_strip) strip_str((s), (chars_to_strip), 1)

#ifdef __cplusplus
}
#endif

#endif // MISRA_STD_CONTAINER_STR_OPS_H
