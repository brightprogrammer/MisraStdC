/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// copyright : Copyright (c) 2025, Siddharth Mishra, All rights reserved.

#ifndef MISRA_STD_UTILITY_ITER_TYPE_H
#define MISRA_STD_UTILITY_ITER_TYPE_H

#include <Misra/Types.h>

typedef struct GenericIter {
    void* data;
    i64   length;
    i64   pos;
    size  alignment;
    i8    dir;
} GenericIter;

#define GENERIC_ITER(x) ((GenericIter*)(void*)(x))

///
/// Memory iterators are there to allow reading regions of memory by remembering current
/// read position and the size limit. With proper checking we can guarantee that we can never
/// overflow or underflow when reading a memory region
///
/// This also means that Iter objects are created for use with only one reading operation,
/// and one object in their lifetime.
///
/// The designed API does not allow modifications to the data Iter is iterating over
///
/// FIELDS:
/// - data   : Pointer to memory we're iterating over
/// - length : Number of objects in memory.
/// - pos    : Current iterating position.
/// - size   : Alignment requirements (if-any), must be at-least 1
/// - dir    : Iteration direction, -1 or 1
///
/// TAGS: Memory, Iterator, Safety
///
#define Iter(DTYPE)                                                                                                    \
    struct {                                                                                                           \
        DTYPE* data;                                                                                                   \
        i64    length;                                                                                                 \
        i64    pos;                                                                                                    \
        size   alignment;                                                                                              \
        i8     dir;                                                                                                    \
    }

///
/// Get data type of `Iter` elements
///
/// mi[in] : `Iter` object
///
/// TAGS: Utility, TypeSafety, Iter
///
#define ITER_DATA_TYPE(mi) __typeof__((mi)->data[0])

///
/// Type specific NULL for given Iter object.
/// Use this instead of NULL when comparing for nullity of Iter objects of same type.
///
/// Null value for `Iter` objects
///
/// mi[in] : Type reference
///
/// TAGS: Utility, NullValue, Iter
///
#define NULL_ITER(mi) (__typeof__((mi)))0

///
/// Type specific NULL for data type Iter object is iterating over.
/// Use this instead of NULL when comparing for nullity of Iter objects of same type.
///
/// Null value for `Iter` element pointers
///
/// mi[in] : Type reference
///
/// TAGS: Utility, NullValue, Iter
#define NULL_ITER_DATA(mi) (ITER_DATA_TYPE(mi)*)0

#endif // MISRA_STD_UTILITY_ITER_TYPE_H
