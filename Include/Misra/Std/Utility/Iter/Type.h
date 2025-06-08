/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.

#ifndef MISRA_STD_UTILITY_ITER_TYPE_H
#define MISRA_STD_UTILITY_ITER_TYPE_H

#include <Misra/Types.h>
#include <Misra/Std/Log.h>

typedef struct GenericIter {
    void *data;
    size  length;
    size  pos;
    size  alignment;
    i8    dir;
} GenericIter;

#define GENERIC_ITER(x) ((GenericIter *)(void *)(x))

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
        DTYPE *data;                                                                                                   \
        size   length;                                                                                                 \
        size   pos;                                                                                                    \
        size   alignment;                                                                                              \
        i8     dir;                                                                                                    \
    }

///
/// Validate whether a given `Iter` object is valid. Not foolproof but will work most of the time.
/// Aborts if provided `Iter` is not valid.
///
/// i[in] : Pointer to `Iter` object to validate.
///
/// SUCCESS : Continue execution, meaning given `Iter` object is most probably a valid `Iter`.
/// FAILURE : `abort`
///
#define ValidateIter(i)                                                                                                \
    do {                                                                                                               \
        if (((i)->dir != -1 && (i)->dir != 1) || !(i)->alignment || !(i)->length || (i)->pos >= (i)->length) {         \
            LOG_FATAL("Invalid iter object.");                                                                         \
        }                                                                                                              \
        (void)(*(char *)(void *)((i)->data));                                                                          \
    } while (0)

///
/// Get data type of `Iter` elements
///
/// mi[in] : `Iter` object
///
/// TAGS: Utility, TypeSafety, Iter
///
#define ITER_DATA_TYPE(mi) TYPE_OF((mi)->data[0])

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
#define NULL_ITER(mi) (TYPE_OF((mi)))0

///
/// Type specific NULL for data type Iter object is iterating over.
/// Use this instead of NULL when comparing for nullity of Iter objects of same type.
///
/// Null value for `Iter` element pointers
///
/// mi[in] : Type reference
///
/// TAGS: Utility, NullValue, Iter
#define NULL_ITER_DATA(mi) (ITER_DATA_TYPE(mi) *)0

#endif // MISRA_STD_UTILITY_ITER_TYPE_H

