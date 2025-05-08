#ifndef MISRA_STD_UTILITY_STR_ITER_H
#define MISRA_STD_UTILITY_STR_ITER_H

#include <Misra/Std/Utility/Iter.h>
#include <Misra/Types.h>

typedef Iter(char) StrIter;

#define StrIterMove(si, n) IterMove((si), (n))
#define StrIterNext(si)    IterNext((si))
#define StrIterPrev(si)    IterPrev((si))

///
/// Get a StrIter from given Str object.
///
#define StrIterFromStr(s) ((StrIter) {.data = (s).data, .length = (s).length, .read_pos = 0})

///
/// Get a StrIter from given Str object.
///
#define StrIterFromZstr(s) ((StrIter) {.data = (s), .length = strlen((s)), .read_pos = 0})

///
/// Get a StrIter from given Str object.
///
#define StrIterFromCstr(s, n) ((StrIter) {.data = (s), .length = n, .read_pos = 0})

///
/// Get total size of this StrIter object
///
/// SUCCESS : If provided StrIter object is not NULL_ITER(mi) then returns size in bytes of memory region
///           this StrIter is iterating over.
/// FAILURE : If provided StrIter is NULL_ITER(mi) then returns 0
#define StrIterSize(mi) IterSize(mi)

///
/// Get remaining size left to read this memory iterator.
///
/// SUCCESS : If provided StrIter object is not NULL_ITER(mi) then remaining size left to read in
///           memory region is returned.
/// FAILURE : If provided StrIter is NULL_ITER(mi) then returns 0
///
#define StrIterRemainingSize(mi) IterRemainingSize(mi)

///
/// Get total length of this StrIter object
///
/// SUCCESS : If provided StrIter object is not NULL_ITER(mi) then returns size in bytes of memory region
///           this StrIter is iterating over.
/// FAILURE : If provided StrIter is NULL_ITER(mi) then returns 0
#define StrIterLength(mi) IterLength(mi)

///
/// Get remaining length left to read this memory iterator.
///
/// SUCCESS : If provided StrIter object is not NULL_ITER(mi) then remaining size left to read in
///           memory region is returned.
/// FAILURE : If provided StrIter is NULL_ITER(mi) then returns 0
///
#define StrIterRemainingLength(mi) IterRemainingLength(mi)

///
/// If there's space left to read in memory region we're iterating over,
/// then return a pointer to current read position.
///
/// SUCCESS : If provided StrIter is not NULL_ITER_DATA(mi), and we have space left to read,
///           then return pointer to memory to start/resume reading from.
/// FAILURE : NULL_ITER_DATA(mi) othewise
///
#define StrIterPos(mi) IterPos(mi)

///
/// Read object from memory iter, given that
/// - Provided StrIter object is not NULL_ITER(mi).
/// - There's space left to read.
/// - Length of object data is being read into is an integral multiple of size of data type
///   this memory iter is iterating over.
///
/// SUCCESS : Data is copied from current read position to provided `dst`, and `mi` is returned
/// FAILURE : NULL_ITER(mi) returned
///
#define StrIterRead(mi, dst) IterRead(mi)

///
/// Peek (not read) object from memory iter, given that
/// - Provided StrIter object is not NULL_ITER(mi).
/// - There's space left to read.
/// - Length of object data is being read into is an integral multiple of size of data type
///   this memory iter is iterating over.
///
/// This is different from reading because it does not change current read position.
/// This is good for making some decisions over data without changing the read position.
///
/// SUCCESS : Data copied over to `dst` from current read position and `mi` is returned.
/// FAILURE : ITER_DATA_TYPE(mi){0} returned.
///
#define StrIterPeek(mi) IterPeek(mi)

#endif // MISRA_STD_UTILITY_STR_ITER_H
