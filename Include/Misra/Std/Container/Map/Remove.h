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
/// Remove entry matching given key.
///
/// m[in,out]          : Hash map.
/// key[in]            : Key to remove.
/// removed_key[out]   : Optional storage for removed key.
/// removed_value[out] : Optional storage for removed value.
///
/// SUCCESS : `true` if key existed and was removed.
/// FAILURE : `false`
///
#define MapRemove(m, lookup_key, removed_key_ptr, removed_value_ptr)                                              \
    map_remove(                                                                                                   \
        GENERIC_MAP(m),                                                                                           \
        &((MAP_KEY_TYPE(m)) {(lookup_key)}),                                                                      \
        (removed_key_ptr),                                                                                            \
        (removed_value_ptr),                                                                                          \
        sizeof(MAP_ENTRY_TYPE(m)),                                                                                \
        offsetof(MAP_ENTRY_TYPE(m), key),                                                                         \
        sizeof(MAP_KEY_TYPE(m)),                                                                                  \
        offsetof(MAP_ENTRY_TYPE(m), value),                                                                       \
        sizeof(MAP_VALUE_TYPE(m)),                                                                                \
        offsetof(MAP_ENTRY_TYPE(m), hash)                                                                         \
    )

///
/// Delete entry matching given key.
///
/// m[in,out] : Hash map.
/// key[in]   : Key to delete.
///
/// SUCCESS : `true` if key existed and was deleted.
/// FAILURE : `false`
///
#define MapDelete(m, lookup_key) MapRemove((m), (lookup_key), NULL, NULL)

#endif // MISRA_STD_CONTAINER_MAP_REMOVE_H
