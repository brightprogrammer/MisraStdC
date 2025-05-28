/// file      : std/container/str/modify.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// copyright : Copyright (c) 2025, Siddharth Mishra, All rights reserved.
///
/// String modification functions for Str

#ifndef MISRA_STD_CONTAINER_STR_MODIFY_H
#define MISRA_STD_CONTAINER_STR_MODIFY_H

#include "Type.h"

#ifdef __cplusplus
extern "C" {
#endif

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

#endif // MISRA_STD_CONTAINER_STR_MODIFY_H 
