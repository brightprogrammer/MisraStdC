/// file      : std/container/map/ops.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Misc helpers for Map.

#ifndef MISRA_STD_CONTAINER_MAP_OPS_H
#define MISRA_STD_CONTAINER_MAP_OPS_H

#include "Access.h"

///
/// Check whether the map has no entries.
///
/// m[in] : Hash map.
///
#define MapEmpty(m) (MapPairCount(m) == 0)

#endif // MISRA_STD_CONTAINER_MAP_OPS_H
