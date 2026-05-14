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
/// m[in]          : Map.
/// lookup_key[in] : Key to search for.
///
/// SUCCESS : Returns `true` when at least one entry exists for the key.
///           The map is not modified.
/// FAILURE : Returns `false` when no entry exists for the key. The map is
///           not modified.
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
/// m[in]             : Map.
/// lookup_key[in]    : Key to search for.
/// lookup_value[in]  : Value to search for. Uses `value_compare` for equality.
///
/// SUCCESS : Returns `true` when at least one entry mapping `lookup_key` to
///           `lookup_value` exists. The map is not modified.
/// FAILURE : Returns `false` when no matching pair exists. The map is not
///           modified. A NULL `value_compare` is a caller bug and aborts
///           via `LOG_FATAL`.
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
/// m[in]          : Map.
/// lookup_key[in] : Key to search for.
///
/// SUCCESS : Returns the number of entries mapping `lookup_key` to any
///           value. The map is not modified.
/// FAILURE : Returns `0` when no entry exists for the key. The map is not
///           modified.
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
/// m[in,out]      : Map.
/// lookup_key[in] : Key to search for.
///
/// SUCCESS : Returns a pointer of type `MAP_VALUE_TYPE(m) *` to the value
///           slot of the first matching entry. The map is not modified.
///           The pointer is valid until the next rehash.
/// FAILURE : Returns `NULL` when no entry exists for the key. The map is
///           not modified.
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
/// m[in,out]      : Map.
/// lookup_key[in] : Key to search for.
///
/// SUCCESS : Returns a pointer of type `MAP_VALUE_TYPE(m) *` to the value
///           slot of the first matching entry. The map is not modified.
/// FAILURE : Returns `NULL` when no entry exists for the key.
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
/// SUCCESS : Returns a value of type `MAP_VALUE_TYPE(m)` - either a copy of
///           the first stored value for the key, or a copy of
///           `default_value` when the key is absent. The map is not
///           modified; `default_value` is not inserted.
/// FAILURE : Does not return on invalid arguments (caller bug); aborts via
///           `LOG_FATAL`.
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
/// m[in]          : Map.
/// lookup_key[in] : Key to search for.
///
/// SUCCESS : Returns a `MapValueCursor` positioned at the first entry that
///           matches the key. `MapValuePtrFromCursor` resolves it to a
///           value pointer. The map is not modified.
/// FAILURE : Returns `MapValueCursorInvalid()` when no entry exists for
///           the key. The map is not modified.
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
/// m[in]          : Map.
/// lookup_key[in] : Key being queried.
/// cursor[in]     : Current cursor returned by `MapFindFirstForKey` or a
///                  previous `MapFindNextForKey` call.
///
/// SUCCESS : Returns a `MapValueCursor` positioned at the next entry for
///           `lookup_key` (in iteration order). The map is not modified.
/// FAILURE : Returns `MapValueCursorInvalid()` when no more entries match.
///           The map is not modified.
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
/// SUCCESS : Returns a pointer of type `MAP_VALUE_TYPE(m) *` to the value
///           slot referenced by the cursor. The map is not modified. The
///           pointer is valid until the next rehash.
/// FAILURE : Returns `NULL` if the cursor is invalid or no longer points to
///           an occupied entry (e.g. after a rehash). The map is not
///           modified.
///
#define MapValuePtrFromCursor(m, cursor)                                                                               \
    ((MAP_VALUE_TYPE(m) *)map_value_ptr_from_cursor(                                                                   \
        GENERIC_MAP(m),                                                                                                \
        (cursor),                                                                                                      \
        sizeof(MAP_ENTRY_TYPE(m)),                                                                                     \
        offsetof(MAP_ENTRY_TYPE(m), value)                                                                             \
    ))

#endif // MISRA_STD_CONTAINER_MAP_ACCESS_H
