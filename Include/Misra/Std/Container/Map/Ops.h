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

///
/// Swap two maps of the same type.
///
/// lhs[in,out] : First map.
/// rhs[in,out] : Second map.
///
#define MapSwap(lhs, rhs)                                                                                              \
    do {                                                                                                               \
        ValidateMap(lhs);                                                                                              \
        ValidateMap(rhs);                                                                                              \
        TYPE_OF(*(lhs)) UNPL(tmp) = *(lhs);                                                                            \
        *(lhs)                    = *(rhs);                                                                            \
        *(rhs)                    = UNPL(tmp);                                                                         \
    } while (0)

///
/// Retain only entries that satisfy a predicate.
///
/// m[in,out]         : Map.
/// predicate_fn[in]  : Callback returning `true` for entries to keep.
/// ctx[in,out]       : Optional user context passed to the predicate.
///
/// SUCCESS : Number of removed pairs.
///
#define MapRetainIf(m, predicate_fn, ctx_ptr)                                                                          \
    map_retain_if(                                                                                                     \
        GENERIC_MAP(m),                                                                                                \
        (MapPredicateFn)(predicate_fn),                                                                                \
        (ctx_ptr),                                                                                                     \
        sizeof(MAP_ENTRY_TYPE(m)),                                                                                     \
        offsetof(MAP_ENTRY_TYPE(m), key),                                                                              \
        sizeof(MAP_KEY_TYPE(m)),                                                                                       \
        offsetof(MAP_ENTRY_TYPE(m), value),                                                                            \
        sizeof(MAP_VALUE_TYPE(m))                                                                                      \
    )

#endif // MISRA_STD_CONTAINER_MAP_OPS_H
