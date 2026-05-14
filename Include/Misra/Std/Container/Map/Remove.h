/// file      : std/container/map/remove.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Remove helpers for Map.

#ifndef MISRA_STD_CONTAINER_MAP_REMOVE_H
#define MISRA_STD_CONTAINER_MAP_REMOVE_H

#include "Type.h"
#include "Private.h"

///
/// Remove and destroy the first entry matching a key.
///
/// m[in,out] : Map.
/// key[in]   : Key to remove.
///
/// SUCCESS : `true` if a value for the key existed and was removed.
/// FAILURE : Returns `false`.
///
#define MapRemoveFirst(m, lookup_key)                                                                                  \
    map_remove(                                                                                                        \
        GENERIC_MAP(m),                                                                                                \
        &((MAP_KEY_TYPE(m)) {(lookup_key)}),                                                                           \
        sizeof(MAP_ENTRY_TYPE(m)),                                                                                     \
        offsetof(MAP_ENTRY_TYPE(m), key),                                                                              \
        sizeof(MAP_KEY_TYPE(m)),                                                                                       \
        offsetof(MAP_ENTRY_TYPE(m), value),                                                                            \
        sizeof(MAP_VALUE_TYPE(m)),                                                                                     \
        offsetof(MAP_ENTRY_TYPE(m), hash)                                                                              \
    )

///
/// Remove and destroy the first matching key/value pair.
///
/// m[in,out]               : Map.
/// key[in]                 : Key to remove.
/// value[in]               : Value to remove.
/// SUCCESS : `true` if the pair existed and was removed.
/// FAILURE : Returns `false`.
///
#define MapRemovePair(m, lookup_key, lookup_value)                                                                     \
    map_remove_pair(                                                                                                   \
        GENERIC_MAP(m),                                                                                                \
        &((MAP_KEY_TYPE(m)) {(lookup_key)}),                                                                           \
        &((MAP_VALUE_TYPE(m)) {(lookup_value)}),                                                                       \
        sizeof(MAP_ENTRY_TYPE(m)),                                                                                     \
        offsetof(MAP_ENTRY_TYPE(m), key),                                                                              \
        sizeof(MAP_KEY_TYPE(m)),                                                                                       \
        offsetof(MAP_ENTRY_TYPE(m), value),                                                                            \
        sizeof(MAP_VALUE_TYPE(m)),                                                                                     \
        offsetof(MAP_ENTRY_TYPE(m), hash)                                                                              \
    )

///
/// Remove all entries matching a key.
///
/// m[in,out] : Map.
/// key[in]   : Key to delete.
///
/// SUCCESS : Number of removed values.
/// FAILURE : Returns `0`.
///
#define MapRemoveAll(m, lookup_key)                                                                                    \
    map_remove_all(                                                                                                    \
        GENERIC_MAP(m),                                                                                                \
        &((MAP_KEY_TYPE(m)) {(lookup_key)}),                                                                           \
        sizeof(MAP_ENTRY_TYPE(m)),                                                                                     \
        offsetof(MAP_ENTRY_TYPE(m), key),                                                                              \
        sizeof(MAP_KEY_TYPE(m)),                                                                                       \
        offsetof(MAP_ENTRY_TYPE(m), value),                                                                            \
        sizeof(MAP_VALUE_TYPE(m)),                                                                                     \
        offsetof(MAP_ENTRY_TYPE(m), hash)                                                                              \
    )

///
/// Remove and destroy all entries that match a predicate.
///
/// m[in,out]         : Map.
/// predicate_fn[in]  : Callback returning `true` for entries to remove.
/// ctx[in,out]       : Optional user context passed to the predicate.
///
/// SUCCESS : Number of removed pairs.
///
#define MapRemoveIf(m, predicate_fn, ctx_ptr)                                                                          \
    map_remove_if(                                                                                                     \
        GENERIC_MAP(m),                                                                                                \
        (MapPredicateFn)(predicate_fn),                                                                                \
        (ctx_ptr),                                                                                                     \
        sizeof(MAP_ENTRY_TYPE(m)),                                                                                     \
        offsetof(MAP_ENTRY_TYPE(m), key),                                                                              \
        sizeof(MAP_KEY_TYPE(m)),                                                                                       \
        offsetof(MAP_ENTRY_TYPE(m), value),                                                                            \
        sizeof(MAP_VALUE_TYPE(m))                                                                                      \
    )

#endif // MISRA_STD_CONTAINER_MAP_REMOVE_H
