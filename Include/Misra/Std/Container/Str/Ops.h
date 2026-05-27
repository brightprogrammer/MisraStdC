/// file      : std/container/str/ops.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// String comparison, find, and modification operations for Str

#ifndef MISRA_STD_CONTAINER_STR_OPS_H
#define MISRA_STD_CONTAINER_STR_OPS_H

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
    /// SUCCESS : Returns a stable hash of the string's bytes.
    ///
    u64 str_hash(const void *data, u32 size);

    ///
    /// Compare two `Str` values lexicographically. Shape matches
    /// `GenericCompare` so it can drop into map / vec compare slots.
    ///
    /// lhs[in] : Pointer to the left `Str`.
    /// rhs[in] : Pointer to the right `Str`.
    ///
    /// SUCCESS : Returns `0` when equal, `<0` when `lhs < rhs`, `>0` when
    ///           `lhs > rhs`.
    ///
    i32 str_compare(const void *lhs, const void *rhs);

//
// Comparison Operations
//

///
/// Compare two Str objects lexicographically, length-aware.
///
/// Thin alias for `str_compare` above (the generic-callback shape).
/// Both arms are read through `StrBegin` / `StrLen`, so a Str with an
/// embedded NUL still compares correctly -- unlike a `Zstr`-based
/// compare that would stop at the first NUL byte.
///
/// str[in]  : First string.
/// ostr[in] : Other string.
///
/// SUCCESS : Returns `0` when equal, `<0` when `str < ostr`, `>0` when
///           `str > ostr`. Neither string is modified.
///
/// TAGS: Str, Compare
///
#define StrCmp(str, ostr) str_compare((str), (ostr))

///
/// Compare string with another const char* of specified length
///
/// str[in]      : Pointer to Str object to compare with.
/// cstr[in]     : String to compare with.
/// cstr_len[in] : Length of the C string to compare.
///
/// RETURN : +ve or -ve depending on above or below in lexical ordering
/// RETURN : 0 if both are equal
///
#define StrCmpCstr(str, cstr, cstr_len) ZstrCompareN((str)->data, cstr, cstr_len)

///
/// Compare string with a null-terminated const char* string
///
/// str[in]  : Pointer to Str object to compare with.
/// zstr[in] : Null-terminated string to compare with.
///
/// RETURN : +ve or -ve depending on above or below in lexical ordering
/// RETURN : 0 if both are equal
///
#define StrCmpZstr(str, zstr) ZstrCompare((str)->data, (zstr))

///
/// Case-insensitive (ASCII) variants of the StrCmp family. Non-ASCII
/// bytes are compared verbatim; there is no Unicode case folding.
///
#define StrCmpIgnoreCase(str, ostr)               ZstrCompareIgnoreCase((str)->data, (ostr)->data)
#define StrCmpZstrIgnoreCase(str, zstr)           ZstrCompareIgnoreCase((str)->data, (zstr))
#define StrCmpCstrIgnoreCase(str, cstr, cstr_len) ZstrCompareNIgnoreCase((str)->data, (cstr), (cstr_len))

//
// Find Operations
//

///
/// Find a key in a Str object.
///
/// str[in] : Str object to find str into.
/// key[in] : Str object to find in `str`
///
/// SUCCESS : char* providing position of found string. Pointer is inside `str`
/// FAILURE : Returns `NULL`.
///
#define StrFindStr(str, key) ZstrFindSubstring((str)->data, (key)->data)

///
/// Find a key in a Str object.
///
/// str[in] : Str object to find str into.
/// key[in] : const char* string to look for
///
/// SUCCESS : char* providing position of found string. Pointer is inside `str`.
/// FAILURE : Returns `NULL`.
///
#define StrFindZstr(str, key) ZstrFindSubstring((str)->data, (key))

///
/// Find a fixed-length substring in a Str object.
///
/// str[in]     : Str object to find str into.
/// key[in]     : Substring to look for.
/// key_len[in] : Length of the substring to look for.
///
/// SUCCESS : char* providing position of found string. Pointer is inside `str`.
/// FAILURE : Returns `NULL`.
///
#define StrFindCstr(str, key, key_len) ZstrFindSubstringN((str)->data, (key), (key_len))

///
/// Find the index of the first occurrence of a key inside a `Str`.
///
/// Three call shapes via `MISRA_OVERLOAD` + `_Generic` on `key`:
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
#define StrIndexOf(...) MISRA_OVERLOAD(StrIndexOf, __VA_ARGS__)
#define StrIndexOf_2(s, key)                                                                                           \
    _Generic(                                                                                                          \
        (key),                                                                                                         \
        Str *: str_index_of_str ((s), (const Str *)(key)),                                                             \
        Zstr:  str_index_of_zstr((s), (Zstr)(key))                                                                     \
    )
#define StrIndexOf_3(s, key, key_len) str_index_of_cstr((s), (Zstr)(key), (key_len))

///
/// Check whether a `Str` contains a key.
///
/// Three call shapes via `MISRA_OVERLOAD` + `_Generic` on `key`:
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
#define StrContains(...) MISRA_OVERLOAD(StrContains, __VA_ARGS__)
#define StrContains_2(s, key)                                                                                          \
    _Generic(                                                                                                          \
        (key),                                                                                                         \
        Str *: str_contains_str ((s), (const Str *)(key)),                                                             \
        Zstr:  str_contains_zstr((s), (Zstr)(key))                                                                     \
    )
#define StrContains_3(s, key, key_len) str_contains_cstr((s), (Zstr)(key), (key_len))

//
// Prefix/Suffix Operations
//

///
/// Check whether a `Str` starts with a prefix.
///
/// Three call shapes via `MISRA_OVERLOAD` + `_Generic` on `prefix`:
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
#define StrStartsWith(...) MISRA_OVERLOAD(StrStartsWith, __VA_ARGS__)
#define StrStartsWith_2(s, prefix)                                                                                     \
    _Generic(                                                                                                          \
        (prefix),                                                                                                      \
        Str *: str_starts_with_str ((s), (const Str *)(prefix)),                                                       \
        Zstr:  str_starts_with_zstr((s), (Zstr)(prefix))                                                               \
    )
#define StrStartsWith_3(s, prefix, prefix_len) str_starts_with_cstr((s), (Zstr)(prefix), (prefix_len))

///
/// Check whether a `Str` ends with a suffix.
///
/// Three call shapes via `MISRA_OVERLOAD` + `_Generic` on `suffix`:
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
#define StrEndsWith(...) MISRA_OVERLOAD(StrEndsWith, __VA_ARGS__)
#define StrEndsWith_2(s, suffix)                                                                                       \
    _Generic(                                                                                                          \
        (suffix),                                                                                                      \
        Str *: str_ends_with_str ((s), (const Str *)(suffix)),                                                         \
        Zstr:  str_ends_with_zstr((s), (Zstr)(suffix))                                                                 \
    )
#define StrEndsWith_3(s, suffix, suffix_len) str_ends_with_cstr((s), (Zstr)(suffix), (suffix_len))

//
// Replace Operations
//

///
/// Replace occurrences of `match` in `s` with `replacement`.
///
/// Three call shapes via `MISRA_OVERLOAD` + `_Generic` on `match`:
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
#define StrReplace(...) MISRA_OVERLOAD(StrReplace, __VA_ARGS__)
#define StrReplace_4(s, match, replacement, count)                                                                     \
    _Generic(                                                                                                          \
        (match),                                                                                                       \
        Str *: str_replace_str ((s), (const Str *)(match), (const Str *)(replacement), (count)),                       \
        Zstr:  str_replace_zstr((s), (Zstr)(match), (Zstr)(replacement), (count))                                      \
    )
#define StrReplace_6(s, match, match_len, replacement, replacement_len, count)                                         \
    str_replace_cstr((s), (Zstr)(match), (match_len), (Zstr)(replacement), (replacement_len), (count))

    //
    // Split Operations
    //

    ///
    /// Split given string into multiple StrIter into the same string.
    /// This way the split operation can be performed without creating new strings,
    /// but instead just having an iterated view into the Str object.
    ///
    /// This is best used when user never needs to make modifications and save
    /// the modifications. In other words, best used when only need iteration
    /// over string with some delimiters.
    ///
    /// str[in] : Str object to split
    /// key[in] : Zero-terminated char pointer value to split based on
    ///
    /// SUCCESS : StrIters vector of non-zero length
    /// FAILURE : StrIters vector of zero-length
    ///
    StrIters str_split_to_iters_zstr(Str *s, Zstr key);
    StrIters str_split_to_iters_str(Str *s, const Str *key);
#define StrSplitToIters(s, key)                                                                                                                                  \
    _Generic((key), Str *: str_split_to_iters_str, Zstr: str_split_to_iters_zstr)( \
        (s),                                                                                                                                                     \
        (key)                                                                                                                                                    \
    )

    ///
    /// Split the given Str object into multiple Str objects stored in a vector
    /// of Str objects. Each Str object in returned vector is a new Str object
    /// and hence must be deinited after use. Calling `VecDeinit()` on the returned
    /// vector will do that for you automatically for all the objects.
    ///
    /// This is best used when iterating over a delimited data is not the only goal,
    /// but also other modifications like stripping over whitespaces from returned Str objects.
    ///
    /// str[in] : Str object to split
    /// key[in] : Zero-terminated char pointer value to split based on
    ///
    /// SUCCESS : Strs vector of non-zero length
    /// FAILURE : Strs vector of zero-length
    ///
    Strs str_split_zstr(Str *s, Zstr key);
    Strs str_split_str(Str *s, const Str *key);
#define StrSplit(s, key)                                                                                                     \
    _Generic((key), Str *: str_split_str, Zstr: str_split_zstr)( \
        (s),                                                                                                                 \
        (key)                                                                                                                \
    )

    //
    // Strip Operations
    //

    ///
    /// Internal implementation for the `StrStrip` / `StrLStrip` /
    /// `StrRStrip` public macros. Not part of the public API.
    ///
    Str strip_str(Str *s, Zstr key, int split_direction);

///
/// Strip leading and trailing whitespace (or optional custom characters) from
/// the given Str object. Returns a new Str object. Original is unmodified.
/// The returned Str must be deinited after use.
///
/// str[in]            : Str object to strip
/// chars_to_strip[in] : Optional zero-terminated char pointer specifying which characters to strip.
///                      If NULL, standard ASCII whitespace is stripped.
///
/// SUCCESS : A new Str object with surrounding characters removed
/// FAILURE : A zero-length Str object
///
#define StrStrip(str, chars_to_strip) strip_str(str, chars_to_strip, 0)

///
/// Strip only leading whitespace (or optional custom characters) from the
/// given Str object. Returns a new Str object. Original is unmodified.
///
/// str[in]            : Str object to strip
/// chars_to_strip[in] : Optional zero-terminated char pointer specifying which characters to strip.
///                      If NULL, standard ASCII whitespace is stripped.
///
/// SUCCESS : A new Str object with leading characters removed
/// FAILURE : A zero-length Str object
///
#define StrLStrip(str, chars_to_strip) strip_str(str, chars_to_strip, -1)

///
/// Strip only trailing whitespace (or optional custom characters) from the
/// given Str object. Returns a new Str object. Original is unmodified.
///
/// str[in]            : Str object to strip
/// chars_to_strip[in] : Optional zero-terminated char pointer specifying which characters to strip.
///                      If NULL, standard ASCII whitespace is stripped.
///
/// SUCCESS : A new Str object with trailing characters removed
/// FAILURE : A zero-length Str object
///
#define StrRStrip(str, chars_to_strip) strip_str(str, chars_to_strip, 1)

#ifdef __cplusplus
}
#endif

#endif // MISRA_STD_CONTAINER_STR_OPS_H
