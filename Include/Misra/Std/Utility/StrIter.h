#ifndef MISRA_STD_UTILITY_STR_ITER_H
#define MISRA_STD_UTILITY_STR_ITER_H

#include <Misra/Std/Container/Vec/Type.h>
#include <Misra/Std/Utility/Iter.h>
#include <Misra/Types.h>

typedef Iter(char) StrIter;
typedef Vec(StrIter) StrIters;

///
/// Validate whether a given `StrIter` object is valid.
/// Not foolproof but will work most of the time.
/// Aborts if provided `StrIter` is not valid.
///
/// i[in] : Pointer to `StrIter` object to validate.
///
/// SUCCESS : Continue execution, meaning given `StrIter` object is most probably a valid `StrIter`.
/// FAILURE : `abort`
///
#define ValidateStrIter(si) ValidateIter(si)

///
/// Validate whether a given `StrIters` object is valid.
/// Not foolproof but will work most of the time.
/// Aborts if provided `StrIters` is not valid.
///
/// i[in] : Pointer to `StrIters` object to validate.
///
/// SUCCESS : Continue execution, meaning given `StrIters` object is most probably a valid `StrIters`.
/// FAILURE : `abort`
///
#define ValidateStrIters(siv) ValidateVec(siv);

///
/// Move string iterator position by `n` elements
///
/// si[in,out] : `StrIter` to modify
/// n[in]      : Number of elements to move
///
/// SUCCESS: Updates position if within bounds
/// FAILURE: No change for invalid moves
///
/// TAGS: StrIter, Pos, Iter
#define StrIterMove(si, n) IterMove((si), (n))

///
/// Advance to next character in string iterator
///
/// si[in,out] : `StrIter` to modify
///
/// TAGS: StrIter, Pos, Iter
#define StrIterNext(si) IterNext((si))

///
/// Move to previous character in string iterator
///
/// si[in,out] : `StrIter` to modify
///
/// TAGS: StrIter, Pos, Iter
#define StrIterPrev(si) IterPrev((si))

///
/// Create string iterator from `Str` object
///
/// s[in] : `Str` object to iterate
///
/// TAGS: StrIter, Initialization, Str, Iter
#define StrIterFromStr(s) IterInitFromVec(s)

///
/// Create string iterator from null-terminated C string
///
/// s[in] : Null-terminated C string
///
/// TAGS: StrIter, Initialization, CString, Iter
#define StrIterFromZstr(s) ((StrIter) {.data = (s), .length = ZstrLen((s)), .pos = 0, .alignment = 1})

///
/// Create string iterator from C string with explicit length
///
/// s[in] : Character buffer
/// n[in] : Length of buffer
///
/// TAGS: StrIter, Initialization, CString, Iter
#define StrIterFromCstr(s, n) ((StrIter) {.data = (s), .length = n, .pos = 0, .alignment = 1})

///
/// Get total size of string data in bytes
///
/// mi[in] : `StrIter` object
///
/// SUCCESS: Returns byte size for valid iterators
/// FAILURE: Returns 0 for invalid iterators
///
/// TAGS: StrIter, Size, Query, Iter
#define StrIterSize(mi) IterSize(mi)

///
/// Get remaining bytes left to read
///
/// mi[in] : `StrIter` object
///
/// SUCCESS: Returns remaining byte count
/// FAILURE: Returns 0 for invalid iterators
///
/// TAGS: StrIter, Size, Query, Iter
#define StrIterRemainingSize(mi) IterRemainingSize(mi)

///
/// Get total length in elements
///
/// mi[in] : `StrIter` object
///
/// SUCCESS: Returns element count for valid iterators
/// FAILURE: Returns 0 for invalid iterators
///
/// TAGS: StrIter, Length, Query, Iter
#define StrIterLength(mi) IterLength(mi)

///
/// Get remaining elements left to read
///
/// mi[in] : `StrIter` object
///
/// SUCCESS: Returns remaining element count
/// FAILURE: Returns 0 for invalid iterators
///
/// TAGS: StrIter, Length, Query, Iter
#define StrIterRemainingLength(mi) IterRemainingLength(mi)

///
/// Get current read position pointer
///
/// mi[in] : `StrIter` object
///
/// SUCCESS: Returns valid char pointer
/// FAILURE: Returns `NULL_ITER_DATA` if exhausted/invalid
///
/// TAGS: StrIter, Pos, Pos, Iter
#define StrIterPos(mi) IterPos(mi)

///
/// Read character from iterator
///
/// mi[in,out] : `StrIter` object
///
/// SUCCESS: Returns character and advances position
/// FAILURE: Returns null character if exhausted
///
/// TAGS: StrIter, Read, Character, Iter
#define StrIterRead(mi, dst) IterRead(mi)

///
/// Peek current character without advancing
///
/// mi[in] : `StrIter` object
///
/// SUCCESS: Returns current character
/// FAILURE: Returns null character if exhausted
///
/// TAGS: StrIter, Peek, Character, Iter
#define StrIterPeek(mi) IterPeekAt((mi), 0)

///
/// Peek character at offset without advancing
///
/// mi[in] : `StrIter` object
/// n[in]  : Peek position
///
/// SUCCESS: Returns current character
/// FAILURE: Returns null character if exhausted
///
/// TAGS: StrIter, Peek, Character, Iter
#define StrIterPeekAt(mi, n) IterPeekAt((mi), (n))

#define StrIterPeekNext(mi) IterPeekAt((mi), 1)
#define StrIterPeekPrev(mi) IterPeekAt((mi), -1)

#endif // MISRA_STD_UTILITY_STR_ITER_H
