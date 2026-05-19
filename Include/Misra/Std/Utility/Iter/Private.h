/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.

#ifndef MISRA_STD_UTILITY_ITER_PRIVATE_H
#define MISRA_STD_UTILITY_ITER_PRIVATE_H

#include "Type.h"

size remaining_length_iter(GenericIter *it);

///
/// Validate that an in-range index can be read at offset `n` (signed)
/// from the current position. Returns the absolute index on success,
/// or `(size)-1` if the offset would land outside `[0, length)`.
///
/// Used by `IterPeekAt` so the bounds math is centralized rather than
/// re-derived in a macro that has to dance around unsigned wraparound.
///
size iter_peek_index(GenericIter *it, i64 n);

///
/// Advance the iterator by `n` positions in the iteration direction.
/// `n` may be negative to step backward. Succeeds when the new
/// position is in `[0, length]` (forward) or at the past-start
/// sentinel `(size)-1` / `[0, length-1]` (reverse); fails otherwise.
///
bool iter_try_move(GenericIter *it, i64 n);

///
/// Runtime contract check for an `Iter` instance. Called via the
/// `ValidateIter` macro at the public surface. Aborts when the iterator is
/// malformed; on success returns control with no state change.
///
void validate_iter(GenericIter *mi);

#endif // MISRA_STD_UTILITY_ITER_PRIVATE_H
