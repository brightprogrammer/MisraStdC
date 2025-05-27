/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// copyright : Copyright (c) 2025, Siddharth Mishra, All rights reserved.

#ifndef MISRA_STD_UTILITY_ITER_ACCESS_H
#define MISRA_STD_UTILITY_ITER_ACCESS_H

#include "Type.h"
#include "Private.h"

///
/// Get total length of this Iter object
///
/// SUCCESS : If provided Iter object is not NULL_ITER(mi) then returns size in bytes of memory region
///           this Iter is iterating over.
/// FAILURE : If provided Iter is NULL_ITER(mi) then returns 0
///
/// TAGS: Memory, Length, Iter
///
#define IterLength(mi) ((mi)->length)
///
/// Get remaining length left to read this memory iterator.
///
/// SUCCESS : If provided Iter object is not NULL_ITER(mi) then remaining size left to read in
///           memory region is returned.
/// FAILURE : If provided Iter is NULL_ITER(mi) then returns 0
///
/// TAGS: Memory, Iter, Length
///
#define IterRemainingLength(mi) remaining_length_iter(GENERIC_ITER(mi))

///
/// Get total size of this Iter object
///
/// SUCCESS : If provided Iter object is not NULL_ITER(mi) then returns size in bytes of memory region
///           this Iter is iterating over.
/// FAILURE : If provided Iter is NULL_ITER(mi) then returns 0
///
/// TAGS: Memory, Size, Iter
///
#define IterSize(mi) IterLength(mi) * ALIGN_UP(sizeof(ITER_DATA_TYPE(mi)), (mi)->alignment)

///
/// Get remaining size left to read this memory iterator.
///
/// SUCCESS : If provided Iter object is not NULL_ITER(mi) then remaining size left to read in
///           memory region is returned.
/// FAILURE : If provided Iter is NULL_ITER(mi) then returns 0
///
/// TAGS: Memory, Iter, Size
///
#define IterRemainingSize(mi) IterRemainingLength(mi) * ALIGN_UP(sizeof(ITER_DATA_TYPE(mi)), (mi)->alignment)

///
/// If there's space left to read in memory region we're iterating over,
/// then return a pointer to current read position.
///
/// SUCCESS : If provided Iter is not NULL_ITER_DATA(mi), and we have space left to read,
///           then return pointer to memory to start/resume reading from.
/// FAILURE : NULL_ITER_DATA(mi) othewise
///
/// TAGS: Iter, Memory, Position
///
#define IterPos(mi)                                                                                                    \
    (IterRemainingLength(mi) ?                                                                                         \
         (ITER_DATA_TYPE(mi) *)(((u64)(mi)->data) +                                                                    \
                                (mi)->pos * ALIGN_UP(sizeof(ITER_DATA_TYPE(mi)), (mi)->alignment)) :                   \
         NULL_ITER_DATA(mi))

///
/// Read object from memory iter, given that
/// - Provided Iter object is not NULL_ITER(mi).
/// - There's space left to read.
/// - Length of object data is being read into is an integral multiple of size of data type
///   this memory iter is iterating over.
///
/// SUCCESS : Data is copied from current read position to provided `dst`, and `mi` is returned
/// FAILURE : NULL_ITER(mi) returned
///
/// TAGS: Memory, Iter, Read
///
#define IterRead(mi)                                                                                                   \
    (IterRemainingLength(mi) ? (((mi)->pos = (mi)->pos + (mi)->dir), (mi)->data[(mi)->pos - (mi)->dir]) :              \
                               (ITER_DATA_TYPE(mi)) {0})

///
/// Peek (not read) object from memory iter, given that
/// - Provided Iter object is not NULL_ITER(mi).
/// - There's space left to read.
/// - Length of object data is being read into is an integral multiple of size of data type
///   this memory iter is iterating over.
///
/// This is different from reading because it does not change current read position.
/// This is good for making some decisions over data without changing the read position.
///
/// SUCCESS : Data copied over to `dst` from current read position and `mi` is returned.
/// FAILURE : NULL_ITER(mi) returned.
///
/// TAGS: Memory, Peek, Iter
///
#define IterPeek(mi) (IterRemainingLength(mi) ? ((mi)->data[(mi)->pos]) : (ITER_DATA_TYPE(mi)) {0})

#endif // MISRA_STD_UTILITY_ITER_ACCESS_H
