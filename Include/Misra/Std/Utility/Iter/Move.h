/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.

#ifndef MISRA_STD_UTILITY_ITER_MOVE_H
#define MISRA_STD_UTILITY_ITER_MOVE_H

#include "Access.h"
#include "Private.h"

///
/// Propagating move by `n` positions in the iteration direction. `n`
/// may be negative to step backward. The new position must land in
/// `[0, length]` for forward iteration, or in `[-1, length)` for
/// reverse iteration (where `-1` is the past-start sentinel).
///
/// SUCCESS : Position is updated, returns `true`.
/// FAILURE : The new position would be out of range. Position is
///           unchanged, returns `false`.
///
/// TAGS: Memory, Iter, Position
///
#define IterMove(mi, n) iter_try_move(GENERIC_ITER(mi), (i64)(n))

///
/// Aborting variant of `IterMove`. See that macro for parameter
/// semantics and success-state effects.
///
/// SUCCESS : Returns to the caller; the underlying `IterMove` succeeded.
/// FAILURE : Does not return - aborts via `LOG_FATAL` when the new
///           position would be out of range.
///
/// TAGS: Iter, Move, Must, Abort
///
#define IterMustMove(mi, n)                                                                                            \
    do {                                                                                                               \
        if (!IterMove((mi), (n))) {                                                                                    \
            LOG_FATAL("IterMustMove: target position out of range");                                                   \
        }                                                                                                              \
    } while (0)

///
/// Propagating advance by one position in the iteration direction.
///
/// SUCCESS : Position is updated, returns `true`.
/// FAILURE : Iterator is already exhausted, returns `false`.
///
/// TAGS: Iter, Memory, Position
///
#define IterNext(mi) IterMove((mi), 1)

///
/// Aborting variant of `IterNext`. See that macro for parameter
/// semantics and success-state effects.
///
/// SUCCESS : Returns to the caller; the underlying `IterNext` succeeded.
/// FAILURE : Does not return - aborts via `LOG_FATAL` when the iterator
///           is already exhausted.
///
/// TAGS: Iter, Move, Must, Abort
///
#define IterMustNext(mi) IterMustMove((mi), 1)

///
/// Propagating step back by one position in the iteration direction.
///
/// SUCCESS : Position is updated, returns `true`.
/// FAILURE : Stepping back would leave the valid range, returns `false`.
///
/// TAGS: Iter, Memory, Position
///
#define IterPrev(mi) IterMove((mi), -1)

///
/// Aborting variant of `IterPrev`. See that macro for parameter
/// semantics and success-state effects.
///
/// SUCCESS : Returns to the caller; the underlying `IterPrev` succeeded.
/// FAILURE : Does not return - aborts via `LOG_FATAL` when stepping back
///           would leave the valid range.
///
/// TAGS: Iter, Move, Must, Abort
///
#define IterMustPrev(mi) IterMustMove((mi), -1)

#endif // MISRA_STD_UTILITY_ITER_MOVE_H
