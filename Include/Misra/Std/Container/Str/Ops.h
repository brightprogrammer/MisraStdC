/// file      : std/container/str/ops.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// copyright : Copyright (c) 2025, Siddharth Mishra, All rights reserved.
///
/// String comparison, find, and modification operations for Str

#ifndef MISRA_STD_CONTAINER_STR_OPS_H
#define MISRA_STD_CONTAINER_STR_OPS_H

#include "Type.h"

#ifdef __cplusplus
extern "C" {
#endif

//
// Comparison Operations
//

///
/// Compare two Str objects
///
/// str[in]  : First string
/// ostr[in] : Other string
///
/// RETURN : +ve or -ve depending on above or below in lexical ordering
/// RETURN : 0 if both are equal
///
#define StrCmp(str, ostr) memcmp((str)->data, (ostr)->data, MIN2((str)->length, (ostr)->length))

///
/// Compare string with another const char*
///
/// str[in]  : Pointer to Str object to compare with.
/// cstr[in] : String to compare with.
///
/// RETURN : +ve or -ve depending on above or below in lexical ordering
/// RETURN : 0 if both are equal
///
#define StrCmpCstr(str, cstr) strncmp((str)->data, cstr, (str)->length)

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
/// FAILURE : NULL
///
#define StrFindStr(str, key) ZstrFindSubstring((str)->data, (key)->data)

///
/// Find a key in a Str object.
///
/// str[in] : Str object to find str into.
/// key[in] : const char* string to look for
///
/// SUCCESS : char* providing position of found string. Pointer is inside `str`.
/// FAILURE : NULL
///
#define StrFindZstr(str, key) ZstrFindSubstring((str)->data, (key))

//
// Prefix/Suffix Operations
//

    ///
    /// Check if string starts with a null-terminated string (Zstr).
    ///
    /// s[in]     : Str to check.
    /// prefix[in]: Null-terminated prefix string.
    ///
    /// SUCCESS : Returns true if `s` starts with `prefix`.
    /// FAILURE : Returns false.
    ///
    bool StrStartsWithZstr(const Str* s, const char* prefix);

    ///
    /// Check if string ends with a null-terminated string (Zstr).
    ///
    /// s[in]     : Str to check.
    /// suffix[in]: Null-terminated suffix string.
    ///
    /// SUCCESS : Returns true if `s` ends with `suffix`.
    /// FAILURE : Returns false.
    ///
    bool StrEndsWithZstr(const Str* s, const char* suffix);

    ///
    /// Check if string starts with a fixed-length C-style string (Cstr).
    ///
    /// s[in]         : Str to check.
    /// prefix[in]    : Pointer to prefix character array.
    /// prefix_len[in]: Length of prefix.
    ///
    /// SUCCESS : Returns true if `s` starts with `prefix`.
    /// FAILURE : Returns false.
    ///
    bool StrStartsWithCstr(const Str* s, const char* prefix, size prefix_len);

    ///
    /// Check if string ends with a fixed-length C-style string (Cstr).
    ///
    /// s[in]         : Str to check.
    /// suffix[in]    : Pointer to suffix character array.
    /// suffix_len[in]: Length of suffix.
    ///
    /// SUCCESS : Returns true if `s` ends with `suffix`.
    /// FAILURE : Returns false.
    ///
    bool StrEndsWithCstr(const Str* s, const char* suffix, size suffix_len);

    ///
    /// Check if string starts with another Str object.
    ///
    /// s[in]     : Str to check.
    /// prefix[in]: Str to check as prefix.
    ///
    /// SUCCESS : Returns true if `s` starts with `prefix`.
    /// FAILURE : Returns false.
    ///
    bool StrStartsWith(const Str* s, const Str* prefix);

    ///
    /// Check if string ends with another Str object.
    ///
    /// s[in]     : Str to check.
    /// suffix[in]: Str to check as suffix.
    ///
    /// SUCCESS : Returns true if `s` ends with `suffix`.
    /// FAILURE : Returns false.
    ///
    bool StrEndsWith(const Str* s, const Str* suffix);

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
    void StrReplaceZstr(Str* s, const char* match, const char* replacement, size count);

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
    void StrReplaceCstr(
        Str*        s,
        const char* match,
        size        match_len,
        const char* replacement,
        size        replacement_len,
        size        count
    );

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
    void StrReplace(Str* s, const Str* match, const Str* replacement, size count);

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
    StrIters StrSplitToIters(Str* s, const char* key);

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
    Strs StrSplit(Str* s, const char* key);

//
// Strip Operations
//

    ///
    /// Internal implementation for strip functions.
    /// Used by StrStrip, StrLStrip, and StrRStrip macros.
    ///
    Str strip_str(Str* s, const char* key, int split_direction);

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
