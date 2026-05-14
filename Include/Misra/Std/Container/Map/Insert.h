/// file      : std/container/map/insert.h
/// author    : Generated following Misra project patterns
/// This is free and unencumbered software released into the public domain.
///
/// Insert and update helpers for Map.

#ifndef MISRA_STD_CONTAINER_MAP_INSERT_H
#define MISRA_STD_CONTAINER_MAP_INSERT_H

#include "Type.h"
#include "Private.h"

#include <Misra/Std/Memory.h>

#if defined(MISRA_ENFORCE_TYPE_SAFETY) && MISRA_ENFORCE_TYPE_SAFETY
#    define MAP_TYPECHECK_KEY_L(m, key) ((void)sizeof(char[_Generic(&(key), MAP_KEY_TYPE(m) *: 1, default: -1)]))
#    define MAP_TYPECHECK_KEY_R(m, key) ((void)sizeof((MAP_KEY_TYPE(m)[]) {(key)}))
#    define MAP_TYPECHECK_VALUE_L(m, value)                                                                            \
        ((void)sizeof(char[_Generic(&(value), MAP_VALUE_TYPE(m) *: 1, default: -1)]))
#    define MAP_TYPECHECK_VALUE_R(m, value) ((void)sizeof((MAP_VALUE_TYPE(m)[]) {(value)}))
#else
#    define MAP_TYPECHECK_KEY_L(m, key)     ((void)0)
#    define MAP_TYPECHECK_KEY_R(m, key)     ((void)0)
#    define MAP_TYPECHECK_VALUE_L(m, value) ((void)0)
#    define MAP_TYPECHECK_VALUE_R(m, value) ((void)0)
#endif

static inline bool map_zero_insert_sources_on_success(
    GenericMap *map,
    void       *key_src,
    size        key_size,
    void       *value_src,
    size        value_size,
    bool        success
) {
    if (!success) {
        return false;
    }

    if (!map->key_copy_init) {
        MemSet(key_src, 0, key_size);
    }

    if (!map->value_copy_init) {
        MemSet(value_src, 0, value_size);
    }

    return true;
}

static inline bool map_zero_value_source_on_success(GenericMap *map, void *value_src, size value_size, bool success) {
    if (success && !map->value_copy_init) {
        MemSet(value_src, 0, value_size);
    }

    return success;
}

static inline bool map_insert_l_impl(
    GenericMap *map,
    void       *key_src,
    size        key_size,
    void       *value_src,
    size        value_size,
    size        entry_size,
    size        key_offset,
    size        value_offset,
    size        hash_offset
) {
    return map_zero_insert_sources_on_success(
        map,
        key_src,
        key_size,
        value_src,
        value_size,
        map_insert(map, key_src, value_src, entry_size, key_offset, key_size, value_offset, value_size, hash_offset)
    );
}

static inline bool map_insert_r_impl(
    GenericMap *map,
    const void *key_src,
    size        key_size,
    const void *value_src,
    size        value_size,
    size        entry_size,
    size        key_offset,
    size        value_offset,
    size        hash_offset
) {
    return map_insert(map, key_src, value_src, entry_size, key_offset, key_size, value_offset, value_size, hash_offset);
}

static inline bool map_set_first_l_impl(
    GenericMap *map,
    const void *key_src,
    void       *value_src,
    size        key_size,
    size        value_size,
    size        entry_size,
    size        key_offset,
    size        value_offset,
    size        hash_offset
) {
    return map_zero_value_source_on_success(
        map,
        value_src,
        value_size,
        map_set_first(map, key_src, value_src, entry_size, key_offset, key_size, value_offset, value_size, hash_offset)
    );
}

static inline bool map_set_first_r_impl(
    GenericMap *map,
    const void *key_src,
    const void *value_src,
    size        key_size,
    size        value_size,
    size        entry_size,
    size        key_offset,
    size        value_offset,
    size        hash_offset
) {
    return map_set_first(
        map,
        key_src,
        value_src,
        entry_size,
        key_offset,
        key_size,
        value_offset,
        value_size,
        hash_offset
    );
}

static inline bool map_set_only_l_impl(
    GenericMap *map,
    void       *key_src,
    size        key_size,
    void       *value_src,
    size        value_size,
    size        entry_size,
    size        key_offset,
    size        value_offset,
    size        hash_offset
) {
    return map_zero_insert_sources_on_success(
        map,
        key_src,
        key_size,
        value_src,
        value_size,
        map_set_only(map, key_src, value_src, entry_size, key_offset, key_size, value_offset, value_size, hash_offset)
    );
}

static inline bool map_set_only_r_impl(
    GenericMap *map,
    const void *key_src,
    size        key_size,
    const void *value_src,
    size        value_size,
    size        entry_size,
    size        key_offset,
    size        value_offset,
    size        hash_offset
) {
    return map_set_only(
        map,
        key_src,
        value_src,
        entry_size,
        key_offset,
        key_size,
        value_offset,
        value_size,
        hash_offset
    );
}

///
/// Insert a new (key, value) pair into the map. If a multi-valued map is
/// supported, this adds another entry for an existing key rather than replacing.
/// L-value form: takes ownership of both `in_key` and `in_value` on success
/// when the corresponding `copy_init` handler is not configured (the sources
/// are zeroed).
///
/// m[in,out]    : Map handle.
/// in_key[in]   : Addressable key. Must match the map's key type.
/// in_value[in] : Addressable value. Must match the map's value type.
///
/// SUCCESS : `true`.
/// FAILURE : `false` on allocation failure or policy violation. Map and
///           sources are unchanged.
///
/// TAGS: Map, Insert, LValue, Ownership
///
#define MapInsertL(m, in_key, in_value)                                                                                \
    (ValidateMap(m),                                                                                                   \
     MAP_TYPECHECK_KEY_L((m), (in_key)),                                                                               \
     MAP_TYPECHECK_VALUE_L((m), (in_value)),                                                                           \
     map_insert_l_impl(                                                                                                \
         GENERIC_MAP(m),                                                                                               \
         &(in_key),                                                                                                    \
         sizeof(MAP_KEY_TYPE(m)),                                                                                      \
         &(in_value),                                                                                                  \
         sizeof(MAP_VALUE_TYPE(m)),                                                                                    \
         sizeof(MAP_ENTRY_TYPE(m)),                                                                                    \
         offsetof(MAP_ENTRY_TYPE(m), key),                                                                             \
         offsetof(MAP_ENTRY_TYPE(m), value),                                                                           \
         offsetof(MAP_ENTRY_TYPE(m), hash)                                                                             \
     ))

///
/// Insert a new (key, value) pair. R-value form: key and value are treated as
/// temporary values and are never zeroed on the caller side.
///
/// m[in,out]    : Map handle.
/// in_key[in]   : Key expression.
/// in_value[in] : Value expression.
///
/// SUCCESS : `true`.
/// FAILURE : `false` on allocation failure or policy violation.
///
/// TAGS: Map, Insert, RValue
///
#define MapInsertR(m, in_key, in_value)                                                                                \
    (ValidateMap(m),                                                                                                   \
     MAP_TYPECHECK_KEY_R((m), (in_key)),                                                                               \
     MAP_TYPECHECK_VALUE_R((m), (in_value)),                                                                           \
     map_insert_r_impl(                                                                                                \
         GENERIC_MAP(m),                                                                                               \
         &LVAL((MAP_KEY_TYPE(m))(in_key)),                                                                             \
         sizeof(MAP_KEY_TYPE(m)),                                                                                      \
         &LVAL((MAP_VALUE_TYPE(m))(in_value)),                                                                         \
         sizeof(MAP_VALUE_TYPE(m)),                                                                                    \
         sizeof(MAP_ENTRY_TYPE(m)),                                                                                    \
         offsetof(MAP_ENTRY_TYPE(m), key),                                                                             \
         offsetof(MAP_ENTRY_TYPE(m), value),                                                                           \
         offsetof(MAP_ENTRY_TYPE(m), hash)                                                                             \
     ))

///
/// Default insertion alias for `MapInsertL`.
///
#define MapInsert(m, in_key, in_value) MapInsertL((m), (in_key), (in_value))

///
/// Update the value of the first existing entry that matches `in_key`, or
/// insert a new entry if no match exists. L-value form takes ownership of
/// `in_value` on success when the value type has no `copy_init` handler.
/// The key is always treated as an r-value lookup.
///
/// m[in,out]    : Map handle.
/// in_key[in]   : Lookup key (treated as r-value).
/// in_value[in] : Addressable replacement value.
///
/// SUCCESS : `true`.
/// FAILURE : `false` on allocation failure during a new-entry insert path.
///
/// TAGS: Map, SetFirst, LValue, Update
///
#define MapSetFirstL(m, in_key, in_value)                                                                              \
    (ValidateMap(m),                                                                                                   \
     MAP_TYPECHECK_KEY_R((m), (in_key)),                                                                               \
     MAP_TYPECHECK_VALUE_L((m), (in_value)),                                                                           \
     map_set_first_l_impl(                                                                                             \
         GENERIC_MAP(m),                                                                                               \
         &LVAL((MAP_KEY_TYPE(m))(in_key)),                                                                             \
         &(in_value),                                                                                                  \
         sizeof(MAP_KEY_TYPE(m)),                                                                                      \
         sizeof(MAP_VALUE_TYPE(m)),                                                                                    \
         sizeof(MAP_ENTRY_TYPE(m)),                                                                                    \
         offsetof(MAP_ENTRY_TYPE(m), key),                                                                             \
         offsetof(MAP_ENTRY_TYPE(m), value),                                                                           \
         offsetof(MAP_ENTRY_TYPE(m), hash)                                                                             \
     ))

///
/// Update the value of the first existing entry that matches `in_key`, or
/// insert a new entry if no match exists. R-value form.
///
/// TAGS: Map, SetFirst, RValue, Update
///
#define MapSetFirstR(m, in_key, in_value)                                                                              \
    (ValidateMap(m),                                                                                                   \
     MAP_TYPECHECK_KEY_R((m), (in_key)),                                                                               \
     MAP_TYPECHECK_VALUE_R((m), (in_value)),                                                                           \
     map_set_first_r_impl(                                                                                             \
         GENERIC_MAP(m),                                                                                               \
         &LVAL((MAP_KEY_TYPE(m))(in_key)),                                                                             \
         &LVAL((MAP_VALUE_TYPE(m))(in_value)),                                                                         \
         sizeof(MAP_KEY_TYPE(m)),                                                                                      \
         sizeof(MAP_VALUE_TYPE(m)),                                                                                    \
         sizeof(MAP_ENTRY_TYPE(m)),                                                                                    \
         offsetof(MAP_ENTRY_TYPE(m), key),                                                                             \
         offsetof(MAP_ENTRY_TYPE(m), value),                                                                           \
         offsetof(MAP_ENTRY_TYPE(m), hash)                                                                             \
     ))

///
/// Set the value for `in_key`, replacing any and all existing entries for that
/// key (collapsing multi-valued entries to a single mapping). L-value form
/// takes ownership of both key and value on success when the corresponding
/// `copy_init` handler is absent.
///
/// m[in,out]    : Map handle.
/// in_key[in]   : Addressable key.
/// in_value[in] : Addressable value.
///
/// SUCCESS : `true`.
/// FAILURE : `false` on allocation failure.
///
/// TAGS: Map, Set, LValue, Replace
///
#define MapSetOnlyL(m, in_key, in_value)                                                                               \
    (ValidateMap(m),                                                                                                   \
     MAP_TYPECHECK_KEY_L((m), (in_key)),                                                                               \
     MAP_TYPECHECK_VALUE_L((m), (in_value)),                                                                           \
     map_set_only_l_impl(                                                                                              \
         GENERIC_MAP(m),                                                                                               \
         &(in_key),                                                                                                    \
         sizeof(MAP_KEY_TYPE(m)),                                                                                      \
         &(in_value),                                                                                                  \
         sizeof(MAP_VALUE_TYPE(m)),                                                                                    \
         sizeof(MAP_ENTRY_TYPE(m)),                                                                                    \
         offsetof(MAP_ENTRY_TYPE(m), key),                                                                             \
         offsetof(MAP_ENTRY_TYPE(m), value),                                                                           \
         offsetof(MAP_ENTRY_TYPE(m), hash)                                                                             \
     ))

///
/// Set the value for `in_key`, replacing any existing entries. R-value form.
///
/// TAGS: Map, Set, RValue, Replace
///
#define MapSetOnlyR(m, in_key, in_value)                                                                               \
    (ValidateMap(m),                                                                                                   \
     MAP_TYPECHECK_KEY_R((m), (in_key)),                                                                               \
     MAP_TYPECHECK_VALUE_R((m), (in_value)),                                                                           \
     map_set_only_r_impl(                                                                                              \
         GENERIC_MAP(m),                                                                                               \
         &LVAL((MAP_KEY_TYPE(m))(in_key)),                                                                             \
         sizeof(MAP_KEY_TYPE(m)),                                                                                      \
         &LVAL((MAP_VALUE_TYPE(m))(in_value)),                                                                         \
         sizeof(MAP_VALUE_TYPE(m)),                                                                                    \
         sizeof(MAP_ENTRY_TYPE(m)),                                                                                    \
         offsetof(MAP_ENTRY_TYPE(m), key),                                                                             \
         offsetof(MAP_ENTRY_TYPE(m), value),                                                                           \
         offsetof(MAP_ENTRY_TYPE(m), hash)                                                                             \
     ))

///
/// Default replace-set alias for `MapSetOnlyL`.
///
#define MapSet(m, in_key, in_value) MapSetOnlyL((m), (in_key), (in_value))

///
/// Look up the value for `lookup_key`. If not present, insert an entry mapping
/// `lookup_key` to `default_value` first. Returns a pointer to the value slot
/// in either case so the caller may inspect or mutate it in place.
///
/// m[in,out]         : Map handle.
/// lookup_key[in]    : Key to find or insert (r-value).
/// default_value[in] : Initial value to install if no entry exists (r-value).
///
/// SUCCESS : Returns a pointer to the value of type `MAP_VALUE_TYPE(m) *`.
/// FAILURE : Returns `NULL` on allocation failure during the insert path.
///
/// USAGE:
///   int *counter = MapEnsurePtr(&counts, key, 0);
///   if (counter) { (*counter)++; }
///
/// TAGS: Map, Lookup, Insert, Ensure
///
#define MapEnsurePtr(m, lookup_key, default_value)                                                                     \
    (ValidateMap(m),                                                                                                   \
     MAP_TYPECHECK_KEY_R((m), (lookup_key)),                                                                           \
     MAP_TYPECHECK_VALUE_R((m), (default_value)),                                                                      \
     (MAP_VALUE_TYPE(m) *)map_ensure_value_ptr(                                                                        \
         GENERIC_MAP(m),                                                                                               \
         &LVAL((MAP_KEY_TYPE(m))(lookup_key)),                                                                         \
         &LVAL((MAP_VALUE_TYPE(m))(default_value)),                                                                    \
         sizeof(MAP_ENTRY_TYPE(m)),                                                                                    \
         offsetof(MAP_ENTRY_TYPE(m), key),                                                                             \
         sizeof(MAP_KEY_TYPE(m)),                                                                                      \
         offsetof(MAP_ENTRY_TYPE(m), value),                                                                           \
         sizeof(MAP_VALUE_TYPE(m)),                                                                                    \
         offsetof(MAP_ENTRY_TYPE(m), hash)                                                                             \
     ))

///
/// Alias for `MapEnsurePtr` matching the get-or-insert idiom from other
/// associative container APIs.
///
#define MapGetOrInsertPtr(m, lookup_key, default_value) MapEnsurePtr((m), (lookup_key), (default_value))

///
/// Aborting (`Must*`) variants of the fallible Map insertion macros above.
///
/// Each `MapMustXxx(...)` is the statement-style do-while wrapper around the
/// matching `MapXxx(...)` expression: it calls the underlying fallible form
/// and triggers `LOG_FATAL(...)` if the call returns `false`. Use these at
/// API boundaries where allocation failure is not recoverable for the caller.
/// Otherwise prefer the propagating forms.
///
/// SUCCESS : Returns to the caller.
/// FAILURE : Does not return - aborts via `LOG_FATAL` / `SysAbort`.
///
/// TAGS: Map, Insert, Must, Abort
///
#define MapMustInsertL(m, in_key, in_value)                                                                            \
    do {                                                                                                               \
        if (!MapInsertL((m), (in_key), (in_value))) {                                                                  \
            LOG_FATAL("MapMustInsertL failed");                                                                        \
        }                                                                                                              \
    } while (0)
#define MapMustInsertR(m, in_key, in_value)                                                                            \
    do {                                                                                                               \
        if (!MapInsertR((m), (in_key), (in_value))) {                                                                  \
            LOG_FATAL("MapMustInsertR failed");                                                                        \
        }                                                                                                              \
    } while (0)
#define MapMustInsert(m, in_key, in_value)                                                                             \
    do {                                                                                                               \
        if (!MapInsert((m), (in_key), (in_value))) {                                                                   \
            LOG_FATAL("MapMustInsert failed");                                                                         \
        }                                                                                                              \
    } while (0)

#define MapMustSetFirstL(m, in_key, in_value)                                                                          \
    do {                                                                                                               \
        if (!MapSetFirstL((m), (in_key), (in_value))) {                                                                \
            LOG_FATAL("MapMustSetFirstL failed");                                                                      \
        }                                                                                                              \
    } while (0)
#define MapMustSetFirstR(m, in_key, in_value)                                                                          \
    do {                                                                                                               \
        if (!MapSetFirstR((m), (in_key), (in_value))) {                                                                \
            LOG_FATAL("MapMustSetFirstR failed");                                                                      \
        }                                                                                                              \
    } while (0)

#define MapMustSetOnlyL(m, in_key, in_value)                                                                           \
    do {                                                                                                               \
        if (!MapSetOnlyL((m), (in_key), (in_value))) {                                                                 \
            LOG_FATAL("MapMustSetOnlyL failed");                                                                       \
        }                                                                                                              \
    } while (0)
#define MapMustSetOnlyR(m, in_key, in_value)                                                                           \
    do {                                                                                                               \
        if (!MapSetOnlyR((m), (in_key), (in_value))) {                                                                 \
            LOG_FATAL("MapMustSetOnlyR failed");                                                                       \
        }                                                                                                              \
    } while (0)
#define MapMustSet(m, in_key, in_value)                                                                                \
    do {                                                                                                               \
        if (!MapSet((m), (in_key), (in_value))) {                                                                      \
            LOG_FATAL("MapMustSet failed");                                                                            \
        }                                                                                                              \
    } while (0)

#endif // MISRA_STD_CONTAINER_MAP_INSERT_H
