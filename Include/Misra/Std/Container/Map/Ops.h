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
/// SUCCESS : Returns `true` when the map length is 0. The map is not modified.
/// FAILURE : Returns `false` when the map contains at least one entry.
///
#define MapEmpty(m) (MapPairCount(m) == 0)

///
/// Swap two maps of the same type.
///
/// lhs[in,out] : First map.
/// rhs[in,out] : Second map.
///
/// SUCCESS : Returns to the caller. Storage, callbacks, policy, and
///           allocator of `lhs` and `rhs` have been exchanged byte-for-byte;
///           neither map's contents are touched.
/// FAILURE : Function cannot fail. Validation of either argument is a
///           caller bug and aborts via `LOG_FATAL`.
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
/// ctx_ptr[in,out]   : Optional user context passed to the predicate.
///
/// SUCCESS : Returns the count of removed entries (may be 0). Every entry
///           for which the predicate returned `false` has been removed,
///           its slot turned into a tombstone, and `key_copy_deinit` /
///           `value_copy_deinit` (if configured) invoked. Map length
///           shrinks by the returned count.
/// FAILURE : Function cannot fail. A NULL `predicate_fn` is a caller bug
///           and aborts via `LOG_FATAL`.
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
