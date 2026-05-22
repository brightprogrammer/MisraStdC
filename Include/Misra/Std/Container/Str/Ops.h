/// file      : std/container/str/ops.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// String comparison, find, and modification operations for Str

#ifndef MISRA_STD_CONTAINER_STR_OPS_H
#define MISRA_STD_CONTAINER_STR_OPS_H

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
    /// Check if string contains another Str.
    ///
    /// s[in]   : Str object to search in.
    /// key[in] : Str object to search for.
    ///
    /// SUCCESS : Returns `true` when a match exists.
    /// FAILURE : Returns `false`.
    ///
    /// TAGS: Str, Contains, Search
    ///
    bool StrContains(const Str *s, const Str *key);

    ///
    /// Find the index of first occurrence of a null-terminated string.
    ///
    /// s[in]   : Str object to search in.
    /// key[in] : Null-terminated string to search for.
    ///
    /// SUCCESS : Returns the zero-based index of the first match. The string is not modified.
    /// FAILURE : Returns `SIZE_MAX` when no match is found. The string is not modified.
    ///
    /// TAGS: Str, IndexOf, Search
    ///
    size StrIndexOfZstr(const Str *s, Zstr key);

    ///
    /// Find the index of first occurrence of a fixed-length string.
    ///
    /// s[in]       : Str object to search in.
    /// key[in]     : String to search for.
    /// key_len[in] : Length of searched string.
    ///
    /// SUCCESS : Returns the zero-based index of the first match. The string is not modified.
    /// FAILURE : Returns `SIZE_MAX` when no match is found. The string is not modified.
    ///
    /// TAGS: Str, IndexOf, Search
    ///
    size StrIndexOfCstr(const Str *s, Zstr key, size key_len);

    ///
    /// Find the index of first occurrence of another Str.
    ///
    /// s[in]   : Str object to search in.
    /// key[in] : Str object to search for.
    ///
    /// SUCCESS : Returns the zero-based index of the first match. The string is not modified.
    /// FAILURE : Returns `SIZE_MAX` when no match is found. The string is not modified.
    ///
    /// TAGS: Str, IndexOf, Search
    ///
    size StrIndexOf(const Str *s, const Str *key);

    ///
    /// Check if string contains a null-terminated string.
    ///
    /// s[in]   : Str object to search in.
    /// key[in] : Null-terminated string to search for.
    ///
    /// SUCCESS : Returns `true` when a match exists.
    /// FAILURE : Returns `false`.
    ///
    /// TAGS: Str, Contains, Search
    ///
    bool StrContainsZstr(const Str *s, Zstr key);

    ///
    /// Check if string contains a fixed-length string.
    ///
    /// s[in]       : Str object to search in.
    /// key[in]     : String to search for.
    /// key_len[in] : Length of searched string.
    ///
    /// SUCCESS : Returns `true` when a match exists.
    /// FAILURE : Returns `false`.
    ///
    /// TAGS: Str, Contains, Search
    ///
    bool StrContainsCstr(const Str *s, Zstr key, size key_len);

    //
    // Prefix/Suffix Operations
    //

    ///
    /// Check if string starts with a null-terminated string (Zstr).
    ///
    /// s[in]     : Str to check.
    /// prefix[in]: Null-terminated prefix string.
    ///
    /// SUCCESS : Returns `true` when `s` starts with `prefix`.
    /// FAILURE : Returns `false`. The string is not modified.
    ///
    bool StrStartsWithZstr(const Str *s, Zstr prefix);

    ///
    /// Check if string ends with a null-terminated string (Zstr).
    ///
    /// s[in]     : Str to check.
    /// suffix[in]: Null-terminated suffix string.
    ///
    /// SUCCESS : Returns `true` when `s` ends with `suffix`.
    /// FAILURE : Returns `false`. The string is not modified.
    ///
    bool StrEndsWithZstr(const Str *s, Zstr suffix);

    ///
    /// Check if string starts with a fixed-length C-style string (Cstr).
    ///
    /// s[in]         : Str to check.
    /// prefix[in]    : Pointer to prefix character array.
    /// prefix_len[in]: Length of prefix.
    ///
    /// SUCCESS : Returns `true` when `s` starts with `prefix`.
    /// FAILURE : Returns `false`. The string is not modified.
    ///
    bool StrStartsWithCstr(const Str *s, Zstr prefix, size prefix_len);

    ///
    /// Check if string ends with a fixed-length C-style string (Cstr).
    ///
    /// s[in]         : Str to check.
    /// suffix[in]    : Pointer to suffix character array.
    /// suffix_len[in]: Length of suffix.
    ///
    /// SUCCESS : Returns `true` when `s` ends with `suffix`.
    /// FAILURE : Returns `false`. The string is not modified.
    ///
    bool StrEndsWithCstr(const Str *s, Zstr suffix, size suffix_len);

    ///
    /// Check if string starts with another Str object.
    ///
    /// s[in]     : Str to check.
    /// prefix[in]: Str to check as prefix.
    ///
    /// SUCCESS : Returns `true` when `s` starts with `prefix`.
    /// FAILURE : Returns `false`. The string is not modified.
    ///
    bool StrStartsWith(const Str *s, const Str *prefix);

    ///
    /// Check if string ends with another Str object.
    ///
    /// s[in]     : Str to check.
    /// suffix[in]: Str to check as suffix.
    ///
    /// SUCCESS : Returns `true` when `s` ends with `suffix`.
    /// FAILURE : Returns `false`. The string is not modified.
    ///
    bool StrEndsWith(const Str *s, const Str *suffix);

    //
    // Replace Operations
    //

    ///
    /// Replace occurrences of a null-terminated string (Zstr) in string.
    ///
    /// s[in,out]      : Str to modify.
    /// match[in]      : Null-terminated match string.
    /// replacement[in]: Null-terminated replacement string.
    /// count[in]      : Maximum number of replacements. -1 means replace all occurences.
    ///
    /// SUCCESS : Modifies `s` in place.
    /// FAILURE : No replacement if `match` not found.
    ///
    void StrReplaceZstr(Str *s, Zstr match, Zstr replacement, size count);

    ///
    /// Replace occurrences of a fixed-length string (Cstr) in string.
    ///
    /// s[in,out]         : Str to modify.
    /// match[in]         : Match string pointer.
    /// match_len[in]     : Length of match string.
    /// replacement[in]   : Replacement string pointer.
    /// replacement_len[in]: Length of replacement string.
    /// count[in]         : Maximum number of replacements. -1 means replace all occurences.
    ///
    /// SUCCESS : Modifies `s` in place.
    /// FAILURE : No replacement if `match` not found.
    ///
    void StrReplaceCstr(Str *s, Zstr match, size match_len, Zstr replacement, size replacement_len, size count);

    ///
    /// Replace occurrences of a Str in string with another Str.
    ///
    /// s[in,out]     : Str to modify.
    /// match[in]     : Str to match.
    /// replacement[in]: Str to replace with.
    /// count[in]     : Maximum number of replacements. -1 means replace all occurences.
    ///
    /// SUCCESS : Modifies `s` in place.
    /// FAILURE : No replacement if `match` not found.
    ///
    void StrReplace(Str *s, const Str *match, const Str *replacement, size count);

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
    _Generic((key), Str *: str_split_to_iters_str, const Str *: str_split_to_iters_str, char *: str_split_to_iters_zstr, const char *: str_split_to_iters_zstr)( \
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
    _Generic((key), Str *: str_split_str, const Str *: str_split_str, char *: str_split_zstr, const char *: str_split_zstr)( \
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
