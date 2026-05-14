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
/// m[in,out]      : Map.
/// lookup_key[in] : Key to remove.
///
/// SUCCESS : Returns `true`. The first matching entry has been removed; its
///           slot is now a tombstone, and `key_copy_deinit` /
///           `value_copy_deinit` (if configured) have been invoked on the
///           removed key and value. Map length shrinks by one.
/// FAILURE : Returns `false` when no entry exists for the key. The map is
///           not modified.
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
/// m[in,out]        : Map.
/// lookup_key[in]   : Key to remove.
/// lookup_value[in] : Value to remove. Uses `value_compare` for equality.
///
/// SUCCESS : Returns `true`. The first matching (key, value) entry has been
///           removed; its slot is now a tombstone, and `key_copy_deinit` /
///           `value_copy_deinit` (if configured) have been invoked. Map
///           length shrinks by one.
/// FAILURE : Returns `false` when no matching pair exists. The map is not
///           modified. A NULL `value_compare` is a caller bug and aborts
///           via `LOG_FATAL`.
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
/// m[in,out]      : Map.
/// lookup_key[in] : Key to delete.
///
/// SUCCESS : Returns the count of removed entries (> 0). Every matching
///           slot is now a tombstone; `key_copy_deinit` / `value_copy_deinit`
///           (if configured) have been invoked on each removed entry.
///           Map length shrinks by the returned count.
/// FAILURE : Returns `0` when no entry matches. The map is not modified.
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
/// ctx_ptr[in,out]   : Optional user context passed to the predicate.
///
/// SUCCESS : Returns the count of removed entries (may be 0). Every entry
///           for which the predicate returned `true` has been removed,
///           its slot turned into a tombstone, and `key_copy_deinit` /
///           `value_copy_deinit` (if configured) invoked. Map length
///           shrinks by the returned count.
/// FAILURE : Function cannot fail. A NULL `predicate_fn` is a caller bug
///           and aborts via `LOG_FATAL`.
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
