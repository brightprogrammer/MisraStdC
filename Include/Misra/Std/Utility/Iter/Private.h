/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.

#ifndef MISRA_STD_UTILITY_ITER_PRIVATE_H
#define MISRA_STD_UTILITY_ITER_PRIVATE_H

#include "Type.h"

size remaining_length_iter(GenericIter *it);

///
/// Runtime contract check for an `Iter` instance. Called via the
/// `ValidateIter` macro at the public surface. Aborts when the iterator is
/// malformed; on success returns control with no state change.
///
void validate_iter(GenericIter *mi);

#endif // MISRA_STD_UTILITY_ITER_PRIVATE_H
