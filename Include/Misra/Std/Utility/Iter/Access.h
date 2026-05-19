/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.

#ifndef MISRA_STD_UTILITY_ITER_ACCESS_H
#define MISRA_STD_UTILITY_ITER_ACCESS_H

#include "Private.h"
#include "Type.h"

///
/// Total length (in elements) of the region the iterator covers.
///
/// TAGS: Memory, Length, Iter
///
#define IterLength(mi) ((mi)->length)

///
/// Elements remaining to read in the iteration direction. Returns `0`
/// once the iterator is past the end (forward) or past the start
/// (reverse).
///
/// TAGS: Memory, Iter, Length
///
#define IterRemainingLength(mi) remaining_length_iter(GENERIC_ITER(mi))

///
/// Total region size in bytes (length scaled by element stride).
///
/// TAGS: Memory, Size, Iter
///
#define IterSize(mi) IterLength(mi) * ALIGN_UP(sizeof(ITER_DATA_TYPE(mi)), (mi)->alignment)

///
/// Remaining region size in bytes.
///
/// TAGS: Memory, Iter, Size
///
#define IterRemainingSize(mi) IterRemainingLength(mi) * ALIGN_UP(sizeof(ITER_DATA_TYPE(mi)), (mi)->alignment)

///
/// Pointer to the current read position, or `NULL_ITER_DATA(mi)` when
/// the iterator is exhausted.
///
/// TAGS: Iter, Memory, Position
///
#define IterPos(mi)                                                                                                    \
    (IterRemainingLength(mi) ?                                                                                         \
         (ITER_DATA_TYPE(mi) *)(((u64)(mi)->data) +                                                                    \
                                (mi)->pos * ALIGN_UP(sizeof(ITER_DATA_TYPE(mi)), (mi)->alignment)) :                   \
         NULL_ITER_DATA(mi))

///
/// Propagating read. Writes the current element to `*out` and advances
/// the iterator. `out` must point at storage compatible with the
/// iterator's element type.
///
/// SUCCESS : `*out` is set, `pos` advances by `dir`, returns `true`.
/// FAILURE : Iterator is exhausted. `*out` is not written, `pos` is
///           unchanged, returns `false`.
///
/// TAGS: Memory, Iter, Read
///
#define IterRead(mi, out)                                                                                              \
    (IterRemainingLength(mi) ? (*(out) = (mi)->data[(mi)->pos], (mi)->pos = (mi)->pos + (mi)->dir, true) : false)

///
/// Aborting variant of `IterRead`. See that macro for parameter
/// semantics and success-state effects.
///
/// SUCCESS : Returns to the caller; the underlying `IterRead` succeeded.
/// FAILURE : Does not return - aborts via `LOG_FATAL` when the iterator
///           is exhausted.
///
/// TAGS: Iter, Read, Must, Abort
///
#define IterMustRead(mi, out)                                                                                          \
    do {                                                                                                               \
        if (!IterRead((mi), (out))) {                                                                                  \
            LOG_FATAL("IterMustRead: iterator exhausted");                                                             \
        }                                                                                                              \
    } while (0)

///
/// Propagating peek at signed offset `n` from the current position.
/// Does not advance the iterator. Writes `data[pos + n]` to `*out`.
///
/// SUCCESS : `*out` is set, returns `true`.
/// FAILURE : `pos + n` is outside `[0, length)`. `*out` is not written,
///           returns `false`.
///
/// TAGS: Memory, Peek, Iter
///
#define IterPeekAt(mi, n, out)                                                                                         \
    ((iter_peek_index(GENERIC_ITER(mi), (i64)(n)) != (size) - 1) ?                                                     \
         (*(out) = (mi)->data[iter_peek_index(GENERIC_ITER(mi), (i64)(n))], true) :                                    \
         false)

///
/// Aborting variant of `IterPeekAt`. See that macro for parameter
/// semantics and success-state effects.
///
/// SUCCESS : Returns to the caller; the underlying `IterPeekAt`
///           succeeded.
/// FAILURE : Does not return - aborts via `LOG_FATAL` when `pos + n`
///           is out of range.
///
/// TAGS: Iter, Peek, Must, Abort
///
#define IterMustPeekAt(mi, n, out)                                                                                     \
    do {                                                                                                               \
        if (!IterPeekAt((mi), (n), (out))) {                                                                           \
            LOG_FATAL("IterMustPeekAt: offset out of range");                                                          \
        }                                                                                                              \
    } while (0)

#endif // MISRA_STD_UTILITY_ITER_ACCESS_H
