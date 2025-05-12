/// file      : std/container/str.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// copyright : Copyright (c) 2025, Siddharth Mishra, All rights reserved.
///
/// Str class

#ifndef MISRA_STD_CONTAINER_STRING_H
#define MISRA_STD_CONTAINER_STRING_H

#include <string.h>

// ct
#include <Misra/Std/Container/Vec.h>
#include <Misra/Std/Utility/StrIter.h>
#include <Misra/Types.h>

typedef Vec(char) Str;
typedef Vec(Str) Strs;

#ifdef _MSC_VER
static inline char* strndup(const char* s, size n) {
    size  len     = strnlen(s, n); // Only up to n
    char* new_str = (char*)malloc(len + 1);
    if (!new_str)
        return NULL;

    memcpy(new_str, s, len);
    new_str[len] = '\0'; // Null-terminate
    return new_str;
}
#endif

#define StrInitFromCstr(cstr, len)                                                                                     \
    ((Str) {.data        = strndup((char*)(cstr), (len)),                                                              \
            .length      = (len),                                                                                      \
            .capacity    = (len),                                                                                      \
            .copy_init   = NULL,                                                                                       \
            .copy_deinit = NULL,                                                                                       \
            .alignment   = 1})

#define StrInitFromZstr(zstr) StrInitFromCstr((zstr), strlen(zstr))

///
/// Init the string using the given format string and arguments.
/// Current contents of string will be cleared out
///
/// str[in,out] : Str to be inited with format string.
/// fmt[in]     : Format string, with variadic arguments following.
///
/// SUCCESS : `str`
/// FAILURE : NULL
///
Str* StrPrintf(Str* str, const char* fmt, ...) FORMAT_STRING(2, 3);

///
/// Initialize given string.
///
/// str : Pointer to string memory that needs to be initialized.
///
/// SUCCESS : `str`
/// FAILURE : NULL
///
#define StrInit() ((Str)VecInit());

///
/// Initialize given string but use memory from stack.
/// Such strings cannot be dynamically resized!!
///
/// str[in] : Pointer to string memory that needs to be initialized.
/// ne[in]  : Number of elements to allocate stack memory for.
///
/// SUCCESS : `str`
/// FAILURE : NULL
///
#define StrInitStack(str, ne, scoped_body) VecInitStack(str, ne, scoped_body)

///
/// Deinit str by freeing all allocations.
///
/// str : Pointer to string to be deinited
///
void StrDeinit(Str* str);

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

///
/// Print and append into given string object with given format.
///
/// str[in,out] : Str to print into.
/// fmt[in] : Format string, followed by variadic arguments.
///
/// SUCCESS : `str`
/// FAILURE : NULL
///
Str* StrAppendf(Str* str, const char* fmt, ...) FORMAT_STRING(2, 3);

///
/// Insert char into string of it's type.
/// Insertion index must not exceed string length.
///
/// str[in] : Str to insert char into
/// chr[in] : Character to be inserted
/// idx[in] : Index to insert char at.
///
/// SUCCESS : Returns `str` the string itself on success.
/// FAILURE : Returns `NULL` otherwise.
///
#define StrInsertCharAt(str, chr, idx) VecInsert((str), (chr), (str))

///
/// Insert a string of given length into given Str at given index.
///
/// str[in,out] : Str object to insert into.
/// zstr[in]    : Zero-terminated string to be inserted.
/// idx[in]     : Index to insert the string at.
/// len[in]     : Length of string or number of bytes to insert.
///
/// SUCCESS : return
/// FAILURE : Does not return
///
#define StrInsertCstr(str, cstr, idx, len) VecInsertRange((str), (cstr), (idx), (len))

///
/// Insert a zero-terminated string into given Str at given index.
///
/// str[in,out] : Str object to insert into.
/// zstr[in]    : Zero-terminated string to be inserted.
/// idx[in]     : Index to insert the string at.
///
/// SUCCESS : return
/// FAILURE : Does not return
///
#define StrInsertZstr(str, zstr, idx) StrInsertCstr((str), (zstr), (idx), strlen(cstr))

///
/// Insert contents of `str2` into `str` at given index.
///
/// str[in,out] : Str object to insert into.
/// str2[in]    : Str object to be inserted.
/// idx[in]     : Index to insert at.
///
/// SUCCESS : return
/// FAILURE : Does not return
///
#define StrInsert(str, str2, idx) StrInsertCstr((str), (str2)->data, (dx), (str2)->length)

///
/// Push char into string.
///
/// str[in] : Str to push char into
/// chr[in] : Pointer to value to be pushed
///
/// SUCCESS : Returns `str` the string itself on success.
/// FAILURE : Returns `NULL` otherwise.
///
#define StrPushBack(str, chr) VecPushBack((str), (chr))

///
/// Pop char from string back.
///
/// str[in,out] : Str to pop char from.
/// val[out]    : Popped char will be stored here. Make sure this has sufficient memory
///              to store memcopied data. If no pointer is provided, then it's equivalent
///              to deleting char from last position.
///
/// SUCCESS : Returns `str` on success
/// FAILURE : Returns NULL otherwise.
///
#define StrPopBack(str, chr) VecPopBack((str), (chr))

///
/// Push char into string front.
///
/// str[in] : Str to push char into
/// chr[in] : Pointer to value to be pushed
///
/// SUCCESS : Returns `str` the string itself on success.
/// FAILURE : Returns `NULL` otherwise.
///
#define StrPushFront(str, chr) VecPushFront((str), (chr))

///
/// Pop char from string front.
///
/// str[in,out] : Str to pop char from.
/// val[out]    : Popped char will be stored here. Make sure this has sufficient memory
///              to store memcopied data. If no pointer is provided, then it's equivalent
///              to deleting char from last position.
///
/// SUCCESS : Returns `str` on success
/// FAILURE : Returns NULL otherwise.
///
#define StrPopFront(str, chr) VecPopFront((str), (chr))

///
/// Remove char from string at given index and store in given pointer.
///
/// str[in,out] : Str to remove char from.
/// val[out]  : Where removed char will be stored. If not provided then it's equivalent to
///             deleting the char at specified index.
/// idx[in]   : Index in string to remove char from.
///
/// SUCCESS : Returns `str` on success.
/// FAILURE : Returns NULL otherwise.
///
#define StrRemove(str, chr, idx) VecRemove((str), (chr), (idx))

///
/// Remove data from string in given range [start, start + count)
///
/// str[in,out] : Str to remove char from.
/// rd[out]   : Where removed data will be stored. If not provided then it's equivalent to
///             deleting the chars in specified range.
/// start[in] : Index in string to removing chars from.
/// count[in] : Number of chars from starting index.
///
/// SUCCESS : return
/// FAILURE : Does not return
///
#define StrRemoveRange(str, rd, start, count) VecRemoveRange((str), (rd), (start), (count))

///
/// Delete last char from vec
///
/// SUCCESS : return
/// FAILURE : Does not return
///
#define StrDeleteLastChar(str) VecDeleteLast(str)

///
/// Delete char at given index
///
/// SUCCESS : return
/// FAILURE : Does not return
///
#define StrDelete(str, idx) VecDelete((str), (idx))

///
/// Delete chars in given range [start, start + count)
///
/// str[in,out] : Str to delete a sequence of characters from.
/// start[in]   : Starting index to start deleting from.
/// count[in]   : Number of characters to be deleted (including the starting index).
///
/// SUCCESS : return
/// FAILURE : Does not return
///
#define StrDeleteRange(str, start, count) VecDeleteRange((str), (start), (count))

///
/// Try reducing memory footprint of string.
/// This is to be used when we know actual allocated memory for vec is large,
/// and we won't need it in future, so we can reduce it to whatever's required at
/// the moment.
///
/// str[in,out] : Str
///
/// SUCCESS : return
/// FAILURE : Does not return
///
#define StrTryReduceSpace(str) VecTryReduceSpace(str)

///
/// Swap chars at given indices.
///
/// str[in,out] : Str to swap chars in.
/// idx1[in]  : Index/Position of first char.
/// idx1[in]  : Index/Position of second char.
///
/// SUCCESS : return
/// FAILURE : Does not return
///
#define StrSwapCharAt(str, idx1, idx2) VecSwapItems((str), (idx1), (idx2))

///
/// Resize string.
/// If length is smaller than current capacity, string length is shrinked.
/// If length is greater than current capacity, space is reserved and string is expanded.
///
/// vec[in,out] : Str to be resized.
/// len[in]     : New length of string.
///
/// SUCCESS : return
/// FAILURE : Does not return
///
#define StrResize(str, len) VecResize((str), (len))

///
/// Reserve space for string.
///
/// vec[in,out] : Str to be resized.
/// len[in]     : New capacity of string.
///
/// SUCCESS : return
/// FAILURE : Does not return
///
#define StrDeleteRange(str, start, count) VecDeleteRange((str), (start), (count))

///
/// Try reducing memory footprint of string.
/// This is to be used when we know actual allocated memory for vec is large,
/// and we won't need it in future, so we can reduce it to whatever's required at
/// the moment.
///
/// str[in,out] : Str
///
/// SUCCESS : return
/// FAILURE : Does not return
///
#define StrTryReduceSpace(str) VecTryReduceSpace(str)

///
/// Swap chars at given indices.
///
/// str[in,out] : Str to swap chars in.
/// idx1[in]  : Index/Position of first char.
/// idx1[in]  : Index/Position of second char.
///
/// SUCCESS : return
/// FAILURE : Does not return
///
#define StrSwapCharAt(str, idx1, idx2) VecSwapItems((str), (idx1), (idx2))

///
/// Resize string.
/// If length is smaller than current capacity, string length is shrinked.
/// If length is greater than current capacity, space is reserved and string is expanded.
///
/// vec[in,out] : Str to be resized.
/// len[in]     : New length of string.
///
/// SUCCESS : return
/// FAILURE : Does not return
///
#define StrReserve(str, n) VecReserve((str), (n))

///
/// Set string length to 0.
///
/// vec[in,out] : Str to be cleared.
///
/// SUCCESS :
/// FAILURE : NULL
///
#define StrClear(str) VecClear(str)

#define StrFirst(str)     VecFirst(str)
#define StrLast(str)      VecLast(str)
#define StrBegin(str)     VecBegin(str)
#define StrEnd(str)       VecEnd(str)
#define StrCharAt(str)    VecAt(str, idx)
#define StrCharPtrAt(str) VecPtrAt(str, idx)

///
/// Push a array of characters with given length into this string at the given
/// position.
///
/// str[in,out] : Str to insert array chars into.
/// cstr[in]    : array of characters with given length to be inserted.
/// len [in]    : Number of characters to be appended.
///
/// SUCCESS : `str`
/// FAILURE : NULL
///
#define StrPushCstr(str, cstr, len, pos) VecPushArr((str), (cstr), (count), (pos))

///
/// Push a null-terminated string to this string
/// at given position.
///
/// str[in,out] : Str to insert array chars into.
/// zstr[in]    : Null-terminated string to be appended.
///
/// SUCCESS : `str`
/// FAILURE : NULL
///
#define StrPushZStr(str, zstr, pos) StrPushCstr((str), (zstr), strlen(zstr), (pos))

///
/// Push an array of chars with given length to the back of this string.
///
/// str[in,out] : Str to insert array chars into.
/// cstr[in]    : array of characters with given length to be inserted.
/// len [in]    : Number of characters to be appended.
///
/// SUCCESS : `str`
/// FAILURE : NULL
///
#define StrPushBackCstr(str, cstr, len) VecPushBackArr((str), (cstr), (len))

///
/// Push a null-terminated string to the back of string.
///
/// str[in,out] : Str to insert array chars into.
/// zstr[in]    : Null-terminated string to be appended.
///
/// SUCCESS : `str`
/// FAILURE : NULL
///
#define StrPushBackZStr(str, zstr) StrPushBackCstr((str), (zstr), strlen((zstr)))

///
/// Push a array of characters with given length to the front of this string
///
/// str[in,out] : Str to insert array chars into.
/// cstr[in]    : array of characters with given length to be inserted.
/// len [in]    : Number of characters to be appended.
///
/// SUCCESS : `str`
/// FAILURE : NULL
///
#define StrPushFrontCstr(str, cstr, len) VecPushFrontArr((str), (cstr), (len))

///
/// Push a null-terminated string to the front of this string.
///
/// str[in,out] : Str to insert array chars into.
/// zstr[in]    : Null-terminated string to be appended.
///
/// SUCCESS : `str`
/// FAILURE : NULL
///
#define StrPushFrontZStr(str, zstr) StrPushFrontCstr((str), (zstr), strlen((zstr)))

///
/// Merge two strings and store the result in first string.
///
/// str[in,out] : Str to insert array chars into.
/// str2[in]    : Str to be inserted.
///
/// SUCCESS : `str`
/// FAILURE : NULL
///
#define StrMerge(str, str2) VecMerge((str), (str2))

///
/// Reverse contents of this string.
///
/// str[in,out] : Str to be reversed.
///
/// SUCCESS : `str`
/// FAILURE : NULL
///
#define StrReverse(str) VecReverse((str))

///
/// Find a key in a Str object.
///
/// str[in] : Str object to find str into.
/// key[in] : Str object to find in `str`
///
/// SUCCESS : char* providing position of found string. Pointer is inside `str`
/// FAILURE : NULL
///
#define StrFindStr(str, key) strstr((str)->data, (key)->data)

///
/// Find a key in a Str object.
///
/// str[in] : Str object to find str into.
/// key[in] : const char* string to look for
///
/// SUCCESS : char* providing position of found string. Pointer is inside `str`.
/// FAILURE : NULL
///
#define StrFindZstr(str, key) strstr((str)->data, (key))

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
#define StrStrip(str, chars_to_strip) StrStripImpl(str, chars_to_strip, 0)

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
#define StrLStrip(str, chars_to_strip) StrStripImpl(str, chars_to_strip, -1)

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
#define StrRStrip(str, chars_to_strip) StrStripImpl(str, chars_to_strip, 1)

#define StrForeachIdx(str, chr, idx, body)              VecForeachIdx((str), (chr), idx, {body})
#define StrForeachReverseIdx(str, chr, idx, body)       VecForeachReverseIdx((str), (chr), idx, {body})
#define StrForeachPtrIdx(str, chrptr, idx, body)        VecForeachPtrIdx((str), (chrptr), idx, {body})
#define StrForeachReversePtrIdx(str, chrptr, idx, body) VecForeachPtrReverseIdx((str), (chrptr), idx, {body})

#define StrForeach(str, chr, body)              VecForeach((str), (chr), {body})
#define StrForeachReverse(str, chr, body)       VecForeachReverse((str), (chr), {body})
#define StrForeachPtr(str, chrptr, body)        VecForeachPtr((str), (chrptr), {body})
#define StrForeachPtrReverse(str, chrptr, body) VecForeachPtrReverse((str), (chrptr), {body})

///
/// Copy data from `src` to `dst`
///
/// dst[out] : Str object to copy into.
/// src[in]  : Str object to copy from.
///
/// SUCCESS : true
/// FAILURE : false
///
bool StrInitCopy(Str* dst, const Str* src);

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

Str strip_str(Str* s, const char* key, int split_direction);

#endif // MISRA_STD_CONTAINER_STRING_H
