/// file      : misra/std/container.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Container umbrella. Pulls in foundation containers plus every
/// optional one the current build enabled.

#ifndef MISRA_STD_CONTAINER_H
#define MISRA_STD_CONTAINER_H

#include <Misra/Types.h>

// Foundation: always available.
#include <Misra/Std/Container/Str.h>
#include <Misra/Std/Container/Vec.h>

#if FEATURE_BITVEC
#    include <Misra/Std/Container/BitVec.h>
#endif
#if FEATURE_LIST
#    include <Misra/Std/Container/List.h>
#endif
#if FEATURE_MAP
#    include <Misra/Std/Container/Map.h>
#endif
#if FEATURE_GRAPH
#    include <Misra/Std/Container/Graph.h>
#endif
#if FEATURE_INT
#    include <Misra/Std/Container/Int.h>
#endif
#if FEATURE_FLOAT
#    include <Misra/Std/Container/Float.h>
#endif

#endif // MISRA_STD_CONTAINER_H
