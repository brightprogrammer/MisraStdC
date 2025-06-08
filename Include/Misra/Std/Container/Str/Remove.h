/// file      : std/container/str/remove.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Removal functions for Str

#ifndef MISRA_STD_CONTAINER_STR_REMOVE_H
#define MISRA_STD_CONTAINER_STR_REMOVE_H

#include "Type.h"

#ifdef __cplusplus
extern "C" {
#endif

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

#ifdef __cplusplus
}
#endif

#endif // MISRA_STD_CONTAINER_STR_REMOVE_H

