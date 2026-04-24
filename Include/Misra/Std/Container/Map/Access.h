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
/// Number of distinct keys stored in the multimap.
///
/// m[in] : Map.
///
#define MapUniqueKeyCount(m)                                                                                           \
    map_unique_key_count(                                                                                              \
        GENERIC_MAP(m),                                                                                                \
        sizeof(MAP_ENTRY_TYPE(m)),                                                                                     \
        offsetof(MAP_ENTRY_TYPE(m), key),                                                                              \
        sizeof(MAP_KEY_TYPE(m)),                                                                                       \
        offsetof(MAP_ENTRY_TYPE(m), hash)                                                                              \
    )

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
/// Check if the map stores a specific key/value pair.
///
/// m[in]               : Map.
/// key[in]             : Key to search for.
/// value[in]           : Value to search for.
/// SUCCESS : `true` when the pair exists.
/// FAILURE : `false`
///
#define MapContainsPair(m, lookup_key, lookup_value)                                                                   \
    map_contains_pair(                                                                                                 \
        GENERIC_MAP(m),                                                                                                \
        &((MAP_KEY_TYPE(m)) {(lookup_key)}),                                                                           \
        &((MAP_VALUE_TYPE(m)) {(lookup_value)}),                                                                       \
        sizeof(MAP_ENTRY_TYPE(m)),                                                                                     \
        offsetof(MAP_ENTRY_TYPE(m), key),                                                                              \
        sizeof(MAP_KEY_TYPE(m)),                                                                                       \
        offsetof(MAP_ENTRY_TYPE(m), value),                                                                            \
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

///
/// Try to get pointer to the first value stored for a key.
///
/// This is an alias for `MapGetFirstPtr` with a more stdlib-style lookup name.
///
/// m[in,out] : Map.
/// key[in]   : Key to search for.
///
/// SUCCESS : Pointer to the first value stored for the key.
/// FAILURE : `NULL`
///
#define MapTryGetPtr(m, lookup_key) MapGetFirstPtr((m), (lookup_key))

///
/// Get the first value stored for a key, or return a fallback value copy.
///
/// NOTE: This returns a value copy, not a pointer. The fallback value is not inserted
///       into the map. If you want insertion-on-miss semantics, use `MapGetOrInsertPtr`.
///
/// m[in,out]             : Map.
/// lookup_key[in]        : Key to search for.
/// default_value[in]     : Value returned when key does not exist.
///
/// SUCCESS : First stored value for key, or `default_value` when absent.
/// FAILURE : Does not return on invalid arguments.
///
#define MapGetOrDefault(m, lookup_key, default_value)                                                                  \
    (*(MAP_VALUE_TYPE(m) *)map_get_value_or_default(                                                                   \
        GENERIC_MAP(m),                                                                                                \
        &((MAP_KEY_TYPE(m)) {(lookup_key)}),                                                                           \
        &((MAP_VALUE_TYPE(m)) {(default_value)}),                                                                      \
        sizeof(MAP_ENTRY_TYPE(m)),                                                                                     \
        offsetof(MAP_ENTRY_TYPE(m), key),                                                                              \
        sizeof(MAP_KEY_TYPE(m)),                                                                                       \
        offsetof(MAP_ENTRY_TYPE(m), value),                                                                            \
        sizeof(MAP_VALUE_TYPE(m)),                                                                                     \
        offsetof(MAP_ENTRY_TYPE(m), hash),                                                                             \
        &((MAP_VALUE_TYPE(m)) {0})                                                                                     \
    ))

///
/// Invalid cursor returned when a per-key query has no more values.
///
#define MapValueCursorInvalid() ((MapValueCursor) {.__index = (size) - 1})

///
/// Check whether a cursor currently points to a value.
///
/// cursor[in] : Cursor returned by `MapFindFirstForKey` or `MapFindNextForKey`.
///
#define MapValueCursorIsValid(cursor) ((cursor).__index != (size) - 1)

///
/// Find the first value stored for a key as a cursor.
///
/// m[in]   : Map.
/// key[in] : Key to search for.
///
/// SUCCESS : Cursor positioned at the first value stored for the key.
/// FAILURE : `MapValueCursorInvalid()` if the key does not exist.
///
#define MapFindFirstForKey(m, lookup_key)                                                                              \
    map_find_first_cursor(                                                                                             \
        GENERIC_MAP(m),                                                                                                \
        &((MAP_KEY_TYPE(m)) {(lookup_key)}),                                                                           \
        sizeof(MAP_ENTRY_TYPE(m)),                                                                                     \
        offsetof(MAP_ENTRY_TYPE(m), key),                                                                              \
        sizeof(MAP_KEY_TYPE(m)),                                                                                       \
        offsetof(MAP_ENTRY_TYPE(m), hash)                                                                              \
    )

///
/// Advance a per-key cursor to the next value for the same key.
///
/// m[in]      : Map.
/// key[in]    : Key being queried.
/// cursor[in] : Current cursor.
///
/// SUCCESS : Cursor positioned at the next value for the same key.
/// FAILURE : `MapValueCursorInvalid()` if there are no more values.
///
#define MapFindNextForKey(m, lookup_key, cursor)                                                                       \
    map_find_next_cursor(                                                                                              \
        GENERIC_MAP(m),                                                                                                \
        &((MAP_KEY_TYPE(m)) {(lookup_key)}),                                                                           \
        (cursor),                                                                                                      \
        sizeof(MAP_ENTRY_TYPE(m)),                                                                                     \
        offsetof(MAP_ENTRY_TYPE(m), key),                                                                              \
        sizeof(MAP_KEY_TYPE(m)),                                                                                       \
        offsetof(MAP_ENTRY_TYPE(m), hash)                                                                              \
    )

///
/// Get value pointer for a valid cursor.
///
/// m[in,out]      : Map.
/// cursor[in,out] : Valid cursor for this map.
///
/// SUCCESS : Pointer to the value referenced by the cursor.
/// FAILURE : `NULL` if the cursor is invalid or no longer points to an occupied entry.
///
#define MapValuePtrFromCursor(m, cursor)                                                                               \
    ((MAP_VALUE_TYPE(m) *)map_value_ptr_from_cursor(                                                                   \
        GENERIC_MAP(m),                                                                                                \
        (cursor),                                                                                                      \
        sizeof(MAP_ENTRY_TYPE(m)),                                                                                     \
        offsetof(MAP_ENTRY_TYPE(m), value)                                                                             \
    ))

#endif // MISRA_STD_CONTAINER_MAP_ACCESS_H
