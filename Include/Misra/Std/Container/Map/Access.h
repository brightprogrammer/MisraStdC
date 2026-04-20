/// file      : std/container/map/access.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Access helpers for Map.

#ifndef MISRA_STD_CONTAINER_MAP_ACCESS_H
#define MISRA_STD_CONTAINER_MAP_ACCESS_H

#include "Type.h"
#include "Private.h"

///
/// Number of stored key/value pairs in the multimap.
///
/// m[in] : Map.
///
#define MapPairCount(m) ((m)->length)

///
/// Check if the map stores at least one value for a key.
///
/// m[in]   : Map.
/// key[in] : Key to search for.
///
/// SUCCESS : `true` when the key exists.
/// FAILURE : `false`
///
#define MapContainsKey(m, lookup_key)                                                                                  \
    map_contains(                                                                                                      \
        GENERIC_MAP(m),                                                                                                \
        &((MAP_KEY_TYPE(m)) {(lookup_key)}),                                                                           \
        sizeof(MAP_ENTRY_TYPE(m)),                                                                                     \
        offsetof(MAP_ENTRY_TYPE(m), key),                                                                              \
        sizeof(MAP_KEY_TYPE(m)),                                                                                       \
        offsetof(MAP_ENTRY_TYPE(m), hash)                                                                              \
    )

///
/// Count how many values are stored for a key.
///
/// m[in]   : Map.
/// key[in] : Key to search for.
///
/// SUCCESS : Number of values stored for the key.
/// FAILURE : `0` if key does not exist.
///
#define MapValueCountForKey(m, lookup_key)                                                                             \
    map_value_count(                                                                                                   \
        GENERIC_MAP(m),                                                                                                \
        &((MAP_KEY_TYPE(m)) {(lookup_key)}),                                                                           \
        sizeof(MAP_ENTRY_TYPE(m)),                                                                                     \
        offsetof(MAP_ENTRY_TYPE(m), key),                                                                              \
        sizeof(MAP_KEY_TYPE(m)),                                                                                       \
        offsetof(MAP_ENTRY_TYPE(m), hash)                                                                              \
    )

///
/// Get pointer to the first value stored for a key.
///
/// m[in,out] : Map.
/// key[in]   : Key to search for.
///
/// SUCCESS : Pointer to the first value stored for the key.
/// FAILURE : `NULL`
///
#define MapGetFirstPtr(m, lookup_key)                                                                                  \
    ((MAP_VALUE_TYPE(m) *)map_get_value_ptr(                                                                           \
        GENERIC_MAP(m),                                                                                                \
        &((MAP_KEY_TYPE(m)) {(lookup_key)}),                                                                           \
        sizeof(MAP_ENTRY_TYPE(m)),                                                                                     \
        offsetof(MAP_ENTRY_TYPE(m), key),                                                                              \
        sizeof(MAP_KEY_TYPE(m)),                                                                                       \
        offsetof(MAP_ENTRY_TYPE(m), value),                                                                            \
        offsetof(MAP_ENTRY_TYPE(m), hash)                                                                              \
    ))

#endif // MISRA_STD_CONTAINER_MAP_ACCESS_H
