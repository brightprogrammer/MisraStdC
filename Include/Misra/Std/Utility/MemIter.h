/// file      : std/utility/memiter.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// copyright : Copyright (c) 2025, Siddharth Mishra, All rights reserved.
///
/// Pairs up memory pointer with it's size

#ifndef MISRA_STD_UTILITY_MEM_ITER_H
#define MISRA_STD_UTILITY_MEM_ITER_H

#include <Misra/Types.h>

/// Memory iterators are there to allow reading regions of memory by remembering current
/// read position and the size limit. With proper checking we can guarantee that we can never
/// overflow or underflow when reading a memory region
///
/// This also means that MemIter objects are created for use with only one reading operation,
/// and one object in their lifetime.
#define MemIter(DTYPE)                                                                             \
    struct {                                                                                       \
        DTYPE* data;                                                                               \
        u64    length;                                                                             \
        u64    read_pos;                                                                           \
    }

///
/// Provides data type given MemIter object is iterating over
///
#define MEM_ITER_DATA_TYPE(mi) __typeof__((mi)->data[0])

///
/// Type specific NULL for given MemIter object.
/// Use this instead of NULL when comparing for nullity of MemIter objects of same type.
///
#define NULL_MEM_ITER(mi) (__typeof__((mi)))0

///
/// Type specific NULL for data type MemIter object is iterating over.
/// Use this instead of NULL when comparing for nullity of MemIter objects of same type.
///
#define NULL_MEM_ITER_DATA(mi) (MEM_ITER_DATA_TYPE(mi)*)0

typedef MemIter(u8) ByteIter;
typedef MemIter(u16) WordIter;
typedef MemIter(u32) DWordIter;
typedef MemIter(u64) QWordIter;

///
/// Get total size of this MemIter object
///
/// SUCCESS : If provided MemIter object is not NULL_MEM_ITER(mi) then returns size in bytes of memory region
///           this MemIter is iterating over.
/// FAILURE : If provided MemIter is NULL_MEM_ITER(mi) then returns 0
#define MemIterSize(mi)                                                                            \
    ((mi) ? ((sizeof(MEM_ITER_DATA_TYPE(mi))) * (mi)->length) :                                    \
            (LOG_ERROR("Invalid memory iter pointer"), 0))

///
/// Get remaining size left to read this memory iterator.
///
/// SUCCESS : If provided MemIter object is not NULL_MEM_ITER(mi) then remaining size left to read in
///           memory region is returned.
/// FAILURE : If provided MemIter is NULL_MEM_ITER(mi) then returns 0
///
#define MemIterRemainingSize(mi)                                                                   \
    ((mi) ? ((MemIterSize(mi) - (mi)->read_pos) > 0 ? (MemIterSize(mi) - (mi)->read_pos) : 0) :    \
            (LOG_ERROR("Invalid memory iter pointer"), 0))

///
/// If there's space left to read in memory region we're iterating over,
/// then return a pointer to current read position.
///
/// SUCCESS : If provided MemIter is not NULL_MEM_ITER_DATA(mi), and we have space left to read,
///           then return pointer to memory to start/resume reading from.
/// FAILURE : NULL_MEM_ITER_DATA(mi) othewise
///
#define MemIterCurrentReadPtr(mi)                                                                  \
    ((mi) && MemIterRemainingSize(mi) ?                                                            \
         ((mi)->data + (mi)->read_pos) :                                                           \
         (LOG_ERROR("Invalid memory iter pointer"), NULL_MEM_ITER_DATA(mi)))

///
/// Read object from memory iter, given that
/// - Provided MemIter object is not NULL_MEM_ITER(mi).
/// - There's space left to read.
/// - Size of object data is being read into is an integral multiple of size of data type
///   this memory iter is iterating over.
///
/// SUCCESS : Data is copied from current read position to provided `dst`, and `mi` is returned
/// FAILURE : NULL_MEM_ITER(mi) returned
///
#define MemIterRead(mi, dst)                                                                       \
    ((mi) ? (!(sizeof(*(dst)) % sizeof(MEM_ITER_DATA_TYPE(mi))) ?                                  \
                 ((MemIterRemainingSize(mi) > sizeof(*(dst))) ?                                    \
                      (memcpy((dst), MemIterCurrentReadPtr(mi), sizeof(*(dst))),                   \
                       (mi)->read_pos += (sizeof(*(dst)) / sizeof(MEM_ITER_DATA_TYPE(mi))),        \
                       (mi)) :                                                                     \
                      (LOG_ERROR("Not enough space left to read in memory iter"),                  \
                       NULL_MEM_ITER(mi))) :                                                       \
                 (LOG_ERROR(                                                                       \
                      "Size of data being read is not integral multiple of type of data memory "   \
                      "iter is iterating over"                                                     \
                  ),                                                                               \
                  NULL_MEM_ITER(mi))) :                                                            \
            (LOG_ERROR("Invalid memory iter read pointer"), NULL_MEM_ITER(mi)))

///
/// Peek (not read) object from memory iter, given that
/// - Provided MemIter object is not NULL_MEM_ITER(mi).
/// - There's space left to read.
/// - Size of object data is being read into is an integral multiple of size of data type
///   this memory iter is iterating over.
///
/// This is different from reading because it does not change current read position.
/// This is good for making some decisions over data without changing the read position.
///
/// SUCCESS : Data copied over to `dst` from current read position and `mi` is returned.
/// FAILURE : NULL_MEM_ITER(mi) returned.
///
#define MemIterPeek(mi, dst)                                                                       \
    (__typeof__(mi))((mi) ?                                                                        \
                         (!(sizeof(*(dst)) % sizeof(MEM_ITER_DATA_TYPE(mi))) ?                     \
                              ((MemIterRemainingSize(mi) > sizeof(*(dst))) ?                       \
                                   (memcpy((dst), MemIterCurrentReadPtr(mi), sizeof(*(dst))),      \
                                    (mi)) :                                                        \
                                   (LOG_ERROR("Not enough space left to read in memory iter"),     \
                                    NULL_MEM_ITER(mi))) :                                          \
                              (LOG_ERROR(                                                          \
                                   "Size of data being read is not integral multiple of type of "  \
                                   "data memory "                                                  \
                                   "iter is iterating over"                                        \
                               ),                                                                  \
                               NULL_MEM_ITER(mi))) :                                               \
                         (LOG_ERROR("Invalid memory iter read pointer"), NULL_MEM_ITER(mi)))

#endif // MISRA_STD_UTILITY_MEM_ITER_H
