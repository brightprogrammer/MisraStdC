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
/// Remove the first entry matching a key.
///
/// m[in,out]          : Hash map.
/// key[in]            : Key to remove.
/// removed_key[out]   : Optional storage for removed key.
/// removed_value[out] : Optional storage for removed value.
///
/// SUCCESS : `true` if a value for the key existed and was removed.
/// FAILURE : `false`
///
#define MapRemoveFirst(m, lookup_key, removed_key_ptr, removed_value_ptr)                                              \
    map_remove(                                                                                                        \
        GENERIC_MAP(m),                                                                                                \
        &((MAP_KEY_TYPE(m)) {(lookup_key)}),                                                                           \
        (removed_key_ptr),                                                                                             \
        (removed_value_ptr),                                                                                           \
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
/// FAILURE : `0`
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
/// Delete all entries matching a key.
///
/// m[in,out] : Map.
/// key[in]   : Key to delete.
///
/// SUCCESS : Number of removed values.
/// FAILURE : `0`
///
#endif // MISRA_STD_CONTAINER_MAP_REMOVE_H
