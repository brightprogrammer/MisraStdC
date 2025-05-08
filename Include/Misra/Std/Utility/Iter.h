/// file      : std/utility/iter.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// copyright : Copyright (c) 2025, Siddharth Mishra, All rights reserved.
///
/// Pairs up memory pointer with it's size

#ifndef MISRA_STD_UTILITY_ITER_H
#define MISRA_STD_UTILITY_ITER_H

#include <Misra/Types.h>

/// Memory iterators are there to allow reading regions of memory by remembering current
/// read position and the size limit. With proper checking we can guarantee that we can never
/// overflow or underflow when reading a memory region
///
/// This also means that Iter objects are created for use with only one reading operation,
/// and one object in their lifetime.
///
/// The designed API does not allow modifications to the data Iter is iterating over
#define Iter(DTYPE)                                                                                                    \
    struct {                                                                                                           \
        DTYPE* data;                                                                                                   \
        i64    length;                                                                                                 \
        i64    pos;                                                                                                    \
        size   alignment;                                                                                              \
    }

#define IterInit()           {.data = NULL, .length = 0, .pos = 0, .alignment = 1}
#define IterInitAligned(aln) {.data = NULL, .length = 0, .pos = 0, .alignment = (aln)}

#define IterInitFromVec(v) {.data = (v)->data, .length = (v)->length, .pos = 0, .alignment = (v)->alignment}

#define IterInitFromStr(s) IterInitFromVec(s)

///
/// Provides data type given Iter object is iterating over
///
#define ITER_DATA_TYPE(mi) __typeof__((mi)->data[0])

///
/// Type specific NULL for given Iter object.
/// Use this instead of NULL when comparing for nullity of Iter objects of same type.
///
#define NULL_ITER(mi) (__typeof__((mi)))0

///
/// Type specific NULL for data type Iter object is iterating over.
/// Use this instead of NULL when comparing for nullity of Iter objects of same type.
///
#define NULL_ITER_DATA(mi) (ITER_DATA_TYPE(mi)*)0

typedef Iter(u8) ByteIter;
typedef Iter(u16) WordIter;
typedef Iter(u32) DWordIter;
typedef Iter(u64) QWordIter;

///
/// Get total length of this Iter object
///
/// SUCCESS : If provided Iter object is not NULL_ITER(mi) then returns size in bytes of memory region
///           this Iter is iterating over.
/// FAILURE : If provided Iter is NULL_ITER(mi) then returns 0
#define IterLength(mi) ((mi) ? ((mi)->length) : (LOG_ERROR("Iter: Invalid memory iter pointer"), 0))

///
/// Get remaining length left to read this memory iterator.
///
/// SUCCESS : If provided Iter object is not NULL_ITER(mi) then remaining size left to read in
///           memory region is returned.
/// FAILURE : If provided Iter is NULL_ITER(mi) then returns 0
///
#define IterRemainingLength(mi)                                                                                        \
    ((mi) ? (((mi)->pos >= 0 && (mi)->pos < IterLength(mi)) ? (IterLength(mi) - (mi)->pos) : 0) :                      \
            (LOG_ERROR("Iter: Invalid memory pointer"), 0))

///
/// Get total size of this Iter object
///
/// SUCCESS : If provided Iter object is not NULL_ITER(mi) then returns size in bytes of memory region
///           this Iter is iterating over.
/// FAILURE : If provided Iter is NULL_ITER(mi) then returns 0
#define IterSize(mi) IterLength(mi) * ALIGN_UP(sizeof(ITER_DATA_TYPE(mi)), (mi)->alignment)

///
/// Get remaining size left to read this memory iterator.
///
/// SUCCESS : If provided Iter object is not NULL_ITER(mi) then remaining size left to read in
///           memory region is returned.
/// FAILURE : If provided Iter is NULL_ITER(mi) then returns 0
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
#define IterPos(mi)                                                                                                    \
    (IterRemainingLength(mi) ?                                                                                         \
         (ITER_DATA_TYPE(mi)*)(((u64)(mi)->data) + (mi)->pos * ALIGN_UP(sizeof(ITER_DATA_TYPE(mi)), (mi)->alignment)   \
         ) :                                                                                                           \
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
#define IterRead(mi) (IterRemainingLength(mi) ? ((mi)->data[(mi)->pos++]) : (ITER_DATA_TYPE(mi)) {0})

///
/// Move current reading position of Iterator.
///
/// SUCCESS : Data is copied from current read position to provided `dst`, and `mi` is returned
/// FAILURE : NULL_ITER(mi) returned
///
#define IterMove(mi, n)                                                                                                \
    do {                                                                                                               \
        if (((IterRemainingLength(mi) - (i64)(n) <= IterLength(mi)) && (IterRemainingLength(mi) - (i64)(n) >= 0)))     \
            (mi)->pos += (n);                                                                                          \
    } while (0)

#define IterNext(mi) IterMove(mi, 1)
#define IterPrev(mi) IterMove(mi, -1)

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
#define IterPeek(mi) (IterRemainingLength(mi) ? ((mi)->data[(mi)->pos]) : (ITER_DATA_TYPE(mi)) {0})

#endif // MISRA_STD_UTILITY_ITER_H
