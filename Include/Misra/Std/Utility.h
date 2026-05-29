/// file      : std/utility.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Utility umbrella header. Pulls in the always-present `Pair` and
/// `StrIter` headers; `Iter(T)` rides in when `FEATURE_ITER` is on.

#ifndef MISRA_STD_UTILITY_H
#define MISRA_STD_UTILITY_H

#include <Misra/Types.h>

// Foundation: header-only utilities.
#include <Misra/Std/Utility/Pair.h>
#include <Misra/Std/Utility/StrIter.h>

#if FEATURE_ITER
#    include <Misra/Std/Utility/Iter.h>
#endif

#endif // MISRA_STD_UTILITY_H
