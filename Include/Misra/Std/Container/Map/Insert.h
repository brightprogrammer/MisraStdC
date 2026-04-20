/// file      : std/container/map/insert.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Insert and update helpers for Map.

#ifndef MISRA_STD_CONTAINER_MAP_INSERT_H
#define MISRA_STD_CONTAINER_MAP_INSERT_H

#include "Type.h"
#include "Private.h"

///
/// Insert a new key/value pair using l-value semantics.
/// Duplicate keys are allowed and append another value for the same key.
///
/// NOTE: Ownership of key and value is transferred to the map if the corresponding
///       copy-init callbacks are not set.
///
/// m[in,out] : Hash map.
/// key[in]   : Key to insert.
/// value[in] : Value to insert.
///
/// TAGS: Map, Insert, LValue, Ownership
///
#define MapInsertL(m, in_key, in_value)                                                                                \
    do {                                                                                                               \
        ValidateMap(m);                                                                                                \
        MAP_KEY_TYPE(m) *__hm_key_ptr_##__LINE__     = &(in_key);                                                      \
        MAP_VALUE_TYPE(m) *__hm_value_ptr_##__LINE__ = &(in_value);                                                    \
        MAP_KEY_TYPE(m) __hm_key_tmp_##__LINE__      = (in_key);                                                       \
        MAP_VALUE_TYPE(m) __hm_value_tmp_##__LINE__  = (in_value);                                                     \
        map_insert(                                                                                                    \
            GENERIC_MAP(m),                                                                                            \
            &__hm_key_tmp_##__LINE__,                                                                                  \
            &__hm_value_tmp_##__LINE__,                                                                                \
            sizeof(MAP_ENTRY_TYPE(m)),                                                                                 \
            offsetof(MAP_ENTRY_TYPE(m), key),                                                                          \
            sizeof(MAP_KEY_TYPE(m)),                                                                                   \
            offsetof(MAP_ENTRY_TYPE(m), value),                                                                        \
            sizeof(MAP_VALUE_TYPE(m)),                                                                                 \
            offsetof(MAP_ENTRY_TYPE(m), hash)                                                                          \
        );                                                                                                             \
        if (!(m)->key_copy_init) {                                                                                     \
            memset(__hm_key_ptr_##__LINE__, 0, sizeof(MAP_KEY_TYPE(m)));                                               \
        }                                                                                                              \
        if (!(m)->value_copy_init) {                                                                                   \
            memset(__hm_value_ptr_##__LINE__, 0, sizeof(MAP_VALUE_TYPE(m)));                                           \
        }                                                                                                              \
    } while (0)

///
/// Insert a new key/value pair using r-value semantics.
/// Duplicate keys are allowed and append another value for the same key.
///
/// m[in,out] : Hash map.
/// key[in]   : Key to insert.
/// value[in] : Value to insert.
///
/// TAGS: Map, Insert, RValue
///
#define MapInsertR(m, in_key, in_value)                                                                                \
    do {                                                                                                               \
        ValidateMap(m);                                                                                                \
        MAP_KEY_TYPE(m) __hm_key_tmp_##__LINE__     = (in_key);                                                        \
        MAP_VALUE_TYPE(m) __hm_value_tmp_##__LINE__ = (in_value);                                                      \
        map_insert(                                                                                                    \
            GENERIC_MAP(m),                                                                                            \
            &__hm_key_tmp_##__LINE__,                                                                                  \
            &__hm_value_tmp_##__LINE__,                                                                                \
            sizeof(MAP_ENTRY_TYPE(m)),                                                                                 \
            offsetof(MAP_ENTRY_TYPE(m), key),                                                                          \
            sizeof(MAP_KEY_TYPE(m)),                                                                                   \
            offsetof(MAP_ENTRY_TYPE(m), value),                                                                        \
            sizeof(MAP_VALUE_TYPE(m)),                                                                                 \
            offsetof(MAP_ENTRY_TYPE(m), hash)                                                                          \
        );                                                                                                             \
    } while (0)

///
/// Insert by default behaves like `MapInsertL`.
///
#define MapInsert(m, in_key, in_value) MapInsertL((m), (in_key), (in_value))

///
/// Replace the first value stored for a key using l-value semantics.
///
/// NOTE: This updates only the first matching value for the key and preserves all
///       remaining values for that key.
///
/// m[in,out] : Hash map.
/// key[in]   : Key to search for.
/// value[in] : Replacement value.
///
/// TAGS: Map, Set, LValue, Ownership
///
#define MapSetFirstL(m, in_key, in_value)                                                                              \
    do {                                                                                                               \
        ValidateMap(m);                                                                                                \
        MAP_VALUE_TYPE(m) *__hm_value_ptr_##__LINE__ = &(in_value);                                                    \
        MAP_KEY_TYPE(m) __hm_key_tmp_##__LINE__      = (in_key);                                                       \
        MAP_VALUE_TYPE(m) __hm_value_tmp_##__LINE__  = (in_value);                                                     \
        bool __hm_replaced_##__LINE__                = map_set_first(                                                  \
            GENERIC_MAP(m),                                                                             \
            &__hm_key_tmp_##__LINE__,                                                                   \
            &__hm_value_tmp_##__LINE__,                                                                 \
            sizeof(MAP_ENTRY_TYPE(m)),                                                                  \
            offsetof(MAP_ENTRY_TYPE(m), key),                                                           \
            sizeof(MAP_KEY_TYPE(m)),                                                                    \
            offsetof(MAP_ENTRY_TYPE(m), value),                                                         \
            sizeof(MAP_VALUE_TYPE(m)),                                                                  \
            offsetof(MAP_ENTRY_TYPE(m), hash)                                                           \
        );                                                                                              \
        if (__hm_replaced_##__LINE__ && !(m)->value_copy_init) {                                                       \
            memset(__hm_value_ptr_##__LINE__, 0, sizeof(MAP_VALUE_TYPE(m)));                                           \
        }                                                                                                              \
    } while (0)

///
/// Replace the first value stored for a key using r-value semantics.
///
/// m[in,out] : Hash map.
/// key[in]   : Key to search for.
/// value[in] : Replacement value.
///
/// TAGS: Map, Set, RValue
///
#define MapSetFirstR(m, in_key, in_value)                                                                              \
    do {                                                                                                               \
        ValidateMap(m);                                                                                                \
        MAP_KEY_TYPE(m) __hm_key_tmp_##__LINE__     = (in_key);                                                        \
        MAP_VALUE_TYPE(m) __hm_value_tmp_##__LINE__ = (in_value);                                                      \
        (void)map_set_first(                                                                                           \
            GENERIC_MAP(m),                                                                                            \
            &__hm_key_tmp_##__LINE__,                                                                                  \
            &__hm_value_tmp_##__LINE__,                                                                                \
            sizeof(MAP_ENTRY_TYPE(m)),                                                                                 \
            offsetof(MAP_ENTRY_TYPE(m), key),                                                                          \
            sizeof(MAP_KEY_TYPE(m)),                                                                                   \
            offsetof(MAP_ENTRY_TYPE(m), value),                                                                        \
            sizeof(MAP_VALUE_TYPE(m)),                                                                                 \
            offsetof(MAP_ENTRY_TYPE(m), hash)                                                                          \
        );                                                                                                             \
    } while (0)

///
/// Replace all values for a key with exactly one key/value pair using l-value semantics.
///
/// NOTE: Existing values for the key are removed before the new value is inserted.
///
/// m[in,out] : Hash map.
/// key[in]   : Key to insert or replace.
/// value[in] : Value to insert or replace.
///
/// TAGS: Map, Set, Only, LValue, Ownership
///
#define MapSetOnlyL(m, in_key, in_value)                                                                               \
    do {                                                                                                               \
        ValidateMap(m);                                                                                                \
        MAP_KEY_TYPE(m) *__hm_key_ptr_##__LINE__     = &(in_key);                                                      \
        MAP_VALUE_TYPE(m) *__hm_value_ptr_##__LINE__ = &(in_value);                                                    \
        MAP_KEY_TYPE(m) __hm_key_tmp_##__LINE__      = (in_key);                                                       \
        MAP_VALUE_TYPE(m) __hm_value_tmp_##__LINE__  = (in_value);                                                     \
        map_remove_all(                                                                                                \
            GENERIC_MAP(m),                                                                                            \
            &__hm_key_tmp_##__LINE__,                                                                                  \
            sizeof(MAP_ENTRY_TYPE(m)),                                                                                 \
            offsetof(MAP_ENTRY_TYPE(m), key),                                                                          \
            sizeof(MAP_KEY_TYPE(m)),                                                                                   \
            offsetof(MAP_ENTRY_TYPE(m), value),                                                                        \
            sizeof(MAP_VALUE_TYPE(m)),                                                                                 \
            offsetof(MAP_ENTRY_TYPE(m), hash)                                                                          \
        );                                                                                                             \
        map_insert(                                                                                                    \
            GENERIC_MAP(m),                                                                                            \
            &__hm_key_tmp_##__LINE__,                                                                                  \
            &__hm_value_tmp_##__LINE__,                                                                                \
            sizeof(MAP_ENTRY_TYPE(m)),                                                                                 \
            offsetof(MAP_ENTRY_TYPE(m), key),                                                                          \
            sizeof(MAP_KEY_TYPE(m)),                                                                                   \
            offsetof(MAP_ENTRY_TYPE(m), value),                                                                        \
            sizeof(MAP_VALUE_TYPE(m)),                                                                                 \
            offsetof(MAP_ENTRY_TYPE(m), hash)                                                                          \
        );                                                                                                             \
        if (!(m)->key_copy_init) {                                                                                     \
            memset(__hm_key_ptr_##__LINE__, 0, sizeof(MAP_KEY_TYPE(m)));                                               \
        }                                                                                                              \
        if (!(m)->value_copy_init) {                                                                                   \
            memset(__hm_value_ptr_##__LINE__, 0, sizeof(MAP_VALUE_TYPE(m)));                                           \
        }                                                                                                              \
    } while (0)

///
/// Replace all values for a key with exactly one key/value pair using r-value semantics.
///
/// m[in,out] : Hash map.
/// key[in]   : Key to insert or replace.
/// value[in] : Value to insert or replace.
///
/// TAGS: Map, Set, Only, RValue
///
#define MapSetOnlyR(m, in_key, in_value)                                                                               \
    do {                                                                                                               \
        ValidateMap(m);                                                                                                \
        MAP_KEY_TYPE(m) __hm_key_tmp_##__LINE__     = (in_key);                                                        \
        MAP_VALUE_TYPE(m) __hm_value_tmp_##__LINE__ = (in_value);                                                      \
        map_remove_all(                                                                                                \
            GENERIC_MAP(m),                                                                                            \
            &__hm_key_tmp_##__LINE__,                                                                                  \
            sizeof(MAP_ENTRY_TYPE(m)),                                                                                 \
            offsetof(MAP_ENTRY_TYPE(m), key),                                                                          \
            sizeof(MAP_KEY_TYPE(m)),                                                                                   \
            offsetof(MAP_ENTRY_TYPE(m), value),                                                                        \
            sizeof(MAP_VALUE_TYPE(m)),                                                                                 \
            offsetof(MAP_ENTRY_TYPE(m), hash)                                                                          \
        );                                                                                                             \
        map_insert(                                                                                                    \
            GENERIC_MAP(m),                                                                                            \
            &__hm_key_tmp_##__LINE__,                                                                                  \
            &__hm_value_tmp_##__LINE__,                                                                                \
            sizeof(MAP_ENTRY_TYPE(m)),                                                                                 \
            offsetof(MAP_ENTRY_TYPE(m), key),                                                                          \
            sizeof(MAP_KEY_TYPE(m)),                                                                                   \
            offsetof(MAP_ENTRY_TYPE(m), value),                                                                        \
            sizeof(MAP_VALUE_TYPE(m)),                                                                                 \
            offsetof(MAP_ENTRY_TYPE(m), hash)                                                                          \
        );                                                                                                             \
    } while (0)

///
/// Default set behaviour is same as `MapSetOnlyL`.
///
#define MapSet(m, in_key, in_value) MapSetOnlyL((m), (in_key), (in_value))

///
/// Ensure a key has at least one value and return a pointer to the first value.
/// If the key does not exist, `default_value` is inserted using r-value semantics.
///
/// m[in,out]           : Map.
/// key[in]             : Key to search or insert.
/// default_value[in]   : Value to insert if key does not exist.
///
/// SUCCESS : Pointer to the first value stored for the key.
///
#define MapEnsurePtr(m, lookup_key, default_value)                                                                     \
    ((MAP_VALUE_TYPE(m) *)map_ensure_value_ptr(                                                                        \
        GENERIC_MAP(m),                                                                                                \
        &((MAP_KEY_TYPE(m)) {(lookup_key)}),                                                                           \
        &((MAP_VALUE_TYPE(m)) {(default_value)}),                                                                      \
        sizeof(MAP_ENTRY_TYPE(m)),                                                                                     \
        offsetof(MAP_ENTRY_TYPE(m), key),                                                                              \
        sizeof(MAP_KEY_TYPE(m)),                                                                                       \
        offsetof(MAP_ENTRY_TYPE(m), value),                                                                            \
        sizeof(MAP_VALUE_TYPE(m)),                                                                                     \
        offsetof(MAP_ENTRY_TYPE(m), hash)                                                                              \
    ))

///
/// Alias for `MapEnsurePtr`.
///
#define MapGetOrInsertPtr(m, lookup_key, default_value) MapEnsurePtr((m), (lookup_key), (default_value))

#endif // MISRA_STD_CONTAINER_MAP_INSERT_H
