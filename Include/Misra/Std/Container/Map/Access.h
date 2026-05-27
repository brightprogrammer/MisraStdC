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
/// TAGS: Map, Length, Count, Pair
///
#define MapPairCount(m) ((void)0, (m)->length)

///
/// Probe-table slot count: the size of the underlying open-addressed
/// hash table, including occupied slots, tombstones, and empty slots.
/// Grows with the policy's `next_capacity`. Always `>= MapPairCount(m)`.
///
/// m[in] : Map.
///
/// TAGS: Map, Access, Capacity
///
#define MapCapacity(m) ((void)0, (m)->capacity)

///
/// Tombstone count: slots that previously held a key but are now
/// reserved for probe-chain continuity. Reset to zero by rehash.
///
/// m[in] : Map.
///
/// TAGS: Map, Access, Tombstones
///
#define MapTombstones(m) ((void)0, (m)->tombstones)

///
/// Allocator backing the map's storage.
///
/// m[in] : Map.
///
/// TAGS: Map, Access, Allocator
///
#define MapAllocator(m) ((void)0, (m)->allocator)

///
/// Deep-copy `init` callback wired for map keys, or `NULL` if the map
/// was initialised without key deep-copy semantics.
///
/// m[in] : Map.
///
/// TAGS: Map, Access, DeepCopy, Key
///
#define MapKeyCopyInit(m) ((void)0, (m)->key_copy_init)

///
/// Deep-copy `deinit` callback wired for map keys, or `NULL` if the map
/// was initialised without key deep-copy semantics.
///
/// m[in] : Map.
///
/// TAGS: Map, Access, DeepCopy, Key
///
#define MapKeyCopyDeinit(m) ((void)0, (m)->key_copy_deinit)

///
/// Deep-copy `init` callback wired for map values, or `NULL` if the map
/// was initialised without value deep-copy semantics.
///
/// m[in] : Map.
///
/// TAGS: Map, Access, DeepCopy, Value
///
#define MapValueCopyInit(m) ((void)0, (m)->value_copy_init)

///
/// Deep-copy `deinit` callback wired for map values, or `NULL` if the map
/// was initialised without value deep-copy semantics.
///
/// m[in] : Map.
///
/// TAGS: Map, Access, DeepCopy, Value
///
#define MapValueCopyDeinit(m) ((void)0, (m)->value_copy_deinit)

///
/// Raw pointer to the entries array. For diagnostic inspection only -
/// the layout is `MAP_ENTRY_TYPE(m)[MapCapacity(m)]` interpreted via
/// `MAP_ENTRY_TYPE(m)` and gated by the state array. Use the value /
/// cursor APIs above for the supported lookup path.
///
/// m[in] : Map.
///
/// TAGS: Map, Access, Internal
///
#define MapEntries(m) ((void)0, (m)->entries)

///
/// Per-slot state array (occupied / empty / tombstone). One byte per
/// slot, `MapCapacity(m)` entries long. Diagnostic inspection only.
///
/// m[in] : Map.
///
/// TAGS: Map, Access, Internal
///
#define MapStates(m) ((void)0, (m)->states)

///
/// Number of distinct keys stored in the multimap. Walks the slot
/// table once to count canonical probe-anchor slots, so this is `O(capacity)`
/// -- prefer `MapPairCount` when total pair count is enough.
///
/// m[in] : Map.
///
/// SUCCESS : Returns the count of distinct keys present. The map is
///           not modified.
/// FAILURE : Cannot fail. `LOG_FATAL` via `ValidateMap(m)` when `m`
///           is uninitialised or corrupted.
///
/// TAGS: Map, Count, Key, Access
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
/// TAGS: Map, Contains, Key, Access
///
#define MapContainsKey(m, lookup_key)                                                                                  \
    map_contains(                                                                                                      \
        GENERIC_MAP(m),                                                                                                \
        &LVAL_AS(MAP_KEY_TYPE(m), lookup_key),                                                                         \
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
/// TAGS: Map, Contains, Pair, Access
///
#define MapContainsPair(m, lookup_key, lookup_value)                                                                   \
    map_contains_pair(                                                                                                 \
        GENERIC_MAP(m),                                                                                                \
        &LVAL_AS(MAP_KEY_TYPE(m), lookup_key),                                                                         \
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
/// TAGS: Map, Count, Value, Key
///
#define MapValueCountForKey(m, lookup_key)                                                                             \
    map_value_count(                                                                                                   \
        GENERIC_MAP(m),                                                                                                \
        &LVAL_AS(MAP_KEY_TYPE(m), lookup_key),                                                                         \
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
/// TAGS: Map, Get, First, Access
///
#define MapGetFirstPtr(m, lookup_key)                                                                                  \
    ((MAP_VALUE_TYPE(m) *)map_get_value_ptr(                                                                           \
        GENERIC_MAP(m),                                                                                                \
        &LVAL_AS(MAP_KEY_TYPE(m), lookup_key),                                                                         \
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
/// TAGS: Map, Get, Try, Access
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
/// TAGS: Map, Get, Default, Access
///
#define MapGetOrDefault(m, lookup_key, default_value)                                                                  \
    (*(MAP_VALUE_TYPE(m) *)map_get_value_or_default(                                                                   \
        GENERIC_MAP(m),                                                                                                \
        &LVAL_AS(MAP_KEY_TYPE(m), lookup_key),                                                                         \
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
/// Sentinel cursor representing past-the-end / iteration exhausted.
/// Returned by `MapFindFirstForKey` and `MapFindNextForKey` when no
/// (further) entries match the queried key.
///
/// SUCCESS : Returns a `MapValueCursor` value that compares unequal to
///           every cursor that points to an occupied entry. Suitable
///           as the loop terminator for per-key cursor iteration. No
///           map is touched.
/// FAILURE : Macro cannot fail. Has no map argument and no observable
///           failure mode.
///
/// TAGS: Map, Cursor, Value, Access
///
#define MapValueCursorInvalid() ((MapValueCursor) {.__index = (size) - 1})

///
/// Check whether a cursor still points to a value for its key (i.e. the
/// per-key iteration has not yet been exhausted).
///
/// cursor[in] : Cursor returned by `MapFindFirstForKey` or `MapFindNextForKey`.
///
/// SUCCESS : Returns `true` when `cursor` refers to an in-range entry
///           (i.e. it is not the past-the-end sentinel). No map is
///           touched.
/// FAILURE : Returns `false` when `cursor` equals `MapValueCursorInvalid()`,
///           meaning the per-key iteration is done. No map is touched.
///
/// TAGS: Map, Cursor, Value, Access
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
/// TAGS: Map, Find, First, Key
///
#define MapFindFirstForKey(m, lookup_key)                                                                              \
    map_find_first_cursor(                                                                                             \
        GENERIC_MAP(m),                                                                                                \
        &LVAL_AS(MAP_KEY_TYPE(m), lookup_key),                                                                         \
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
/// TAGS: Map, Find, Next, Key
///
#define MapFindNextForKey(m, lookup_key, cursor)                                                                       \
    map_find_next_cursor(                                                                                              \
        GENERIC_MAP(m),                                                                                                \
        &LVAL_AS(MAP_KEY_TYPE(m), lookup_key),                                                                         \
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
/// TAGS: Map, Cursor, Value, Access
///
#define MapValuePtrFromCursor(m, cursor)                                                                               \
    ((MAP_VALUE_TYPE(m) *)map_value_ptr_from_cursor(                                                                   \
        GENERIC_MAP(m),                                                                                                \
        (cursor),                                                                                                      \
        sizeof(MAP_ENTRY_TYPE(m)),                                                                                     \
        offsetof(MAP_ENTRY_TYPE(m), value)                                                                             \
    ))

#endif // MISRA_STD_CONTAINER_MAP_ACCESS_H
