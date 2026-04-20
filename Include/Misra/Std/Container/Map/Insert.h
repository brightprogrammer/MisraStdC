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
/// Insert new key/value pair using l-value semantics.
/// Aborts if key already exists.
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
#define MapInsertL(m, in_key, in_value)                                                                            \
    do {                                                                                                               \
        ValidateMap(m);                                                                                            \
        MAP_KEY_TYPE(m) *__hm_key_ptr_##__LINE__     = &(in_key);                                                  \
        MAP_VALUE_TYPE(m) *__hm_value_ptr_##__LINE__ = &(in_value);                                                \
        MAP_KEY_TYPE(m) __hm_key_tmp_##__LINE__      = (in_key);                                                   \
        MAP_VALUE_TYPE(m) __hm_value_tmp_##__LINE__  = (in_value);                                                 \
        map_insert(                                                                                                \
            GENERIC_MAP(m),                                                                                        \
            &__hm_key_tmp_##__LINE__,                                                                                  \
            &__hm_value_tmp_##__LINE__,                                                                                \
            sizeof(MAP_ENTRY_TYPE(m)),                                                                             \
            offsetof(MAP_ENTRY_TYPE(m), key),                                                                      \
            sizeof(MAP_KEY_TYPE(m)),                                                                               \
            offsetof(MAP_ENTRY_TYPE(m), value),                                                                    \
            sizeof(MAP_VALUE_TYPE(m)),                                                                             \
            offsetof(MAP_ENTRY_TYPE(m), hash),                                                                     \
            false                                                                                                      \
        );                                                                                                             \
        if (!(m)->key_copy_init) {                                                                                     \
            memset(__hm_key_ptr_##__LINE__, 0, sizeof(MAP_KEY_TYPE(m)));                                           \
        }                                                                                                              \
        if (!(m)->value_copy_init) {                                                                                   \
            memset(__hm_value_ptr_##__LINE__, 0, sizeof(MAP_VALUE_TYPE(m)));                                       \
        }                                                                                                              \
    } while (0)

///
/// Insert new key/value pair using r-value semantics.
/// Aborts if key already exists.
///
/// m[in,out] : Hash map.
/// key[in]   : Key to insert.
/// value[in] : Value to insert.
///
/// TAGS: Map, Insert, RValue
///
#define MapInsertR(m, in_key, in_value)                                                                            \
    do {                                                                                                               \
        ValidateMap(m);                                                                                            \
        MAP_KEY_TYPE(m) __hm_key_tmp_##__LINE__     = (in_key);                                                    \
        MAP_VALUE_TYPE(m) __hm_value_tmp_##__LINE__ = (in_value);                                                  \
        map_insert(                                                                                                \
            GENERIC_MAP(m),                                                                                        \
            &__hm_key_tmp_##__LINE__,                                                                                  \
            &__hm_value_tmp_##__LINE__,                                                                                \
            sizeof(MAP_ENTRY_TYPE(m)),                                                                             \
            offsetof(MAP_ENTRY_TYPE(m), key),                                                                      \
            sizeof(MAP_KEY_TYPE(m)),                                                                               \
            offsetof(MAP_ENTRY_TYPE(m), value),                                                                    \
            sizeof(MAP_VALUE_TYPE(m)),                                                                             \
            offsetof(MAP_ENTRY_TYPE(m), hash),                                                                     \
            false                                                                                                      \
        );                                                                                                             \
    } while (0)

///
/// Insert by default behaves like `MapInsertL`.
///
#define MapInsert(m, in_key, in_value) MapInsertL((m), (in_key), (in_value))

///
/// Insert or replace key/value pair using l-value semantics.
///
/// NOTE: If the key already exists then both key and value stored in the map are
///       replaced by the provided objects.
///
/// m[in,out] : Hash map.
/// key[in]   : Key to insert or replace.
/// value[in] : Value to insert or replace.
///
/// TAGS: Map, Set, LValue, Ownership
///
#define MapSetL(m, in_key, in_value)                                                                               \
    do {                                                                                                               \
        ValidateMap(m);                                                                                            \
        MAP_KEY_TYPE(m) *__hm_key_ptr_##__LINE__     = &(in_key);                                                  \
        MAP_VALUE_TYPE(m) *__hm_value_ptr_##__LINE__ = &(in_value);                                                \
        MAP_KEY_TYPE(m) __hm_key_tmp_##__LINE__      = (in_key);                                                   \
        MAP_VALUE_TYPE(m) __hm_value_tmp_##__LINE__  = (in_value);                                                 \
        map_insert(                                                                                                \
            GENERIC_MAP(m),                                                                                        \
            &__hm_key_tmp_##__LINE__,                                                                                  \
            &__hm_value_tmp_##__LINE__,                                                                                \
            sizeof(MAP_ENTRY_TYPE(m)),                                                                             \
            offsetof(MAP_ENTRY_TYPE(m), key),                                                                      \
            sizeof(MAP_KEY_TYPE(m)),                                                                               \
            offsetof(MAP_ENTRY_TYPE(m), value),                                                                    \
            sizeof(MAP_VALUE_TYPE(m)),                                                                             \
            offsetof(MAP_ENTRY_TYPE(m), hash),                                                                     \
            true                                                                                                       \
        );                                                                                                             \
        if (!(m)->key_copy_init) {                                                                                     \
            memset(__hm_key_ptr_##__LINE__, 0, sizeof(MAP_KEY_TYPE(m)));                                           \
        }                                                                                                              \
        if (!(m)->value_copy_init) {                                                                                   \
            memset(__hm_value_ptr_##__LINE__, 0, sizeof(MAP_VALUE_TYPE(m)));                                       \
        }                                                                                                              \
    } while (0)

///
/// Insert or replace key/value pair using r-value semantics.
///
/// m[in,out] : Hash map.
/// key[in]   : Key to insert or replace.
/// value[in] : Value to insert or replace.
///
/// TAGS: Map, Set, RValue
///
#define MapSetR(m, in_key, in_value)                                                                               \
    do {                                                                                                               \
        ValidateMap(m);                                                                                            \
        MAP_KEY_TYPE(m) __hm_key_tmp_##__LINE__     = (in_key);                                                    \
        MAP_VALUE_TYPE(m) __hm_value_tmp_##__LINE__ = (in_value);                                                  \
        map_insert(                                                                                                \
            GENERIC_MAP(m),                                                                                        \
            &__hm_key_tmp_##__LINE__,                                                                                  \
            &__hm_value_tmp_##__LINE__,                                                                                \
            sizeof(MAP_ENTRY_TYPE(m)),                                                                             \
            offsetof(MAP_ENTRY_TYPE(m), key),                                                                      \
            sizeof(MAP_KEY_TYPE(m)),                                                                               \
            offsetof(MAP_ENTRY_TYPE(m), value),                                                                    \
            sizeof(MAP_VALUE_TYPE(m)),                                                                             \
            offsetof(MAP_ENTRY_TYPE(m), hash),                                                                     \
            true                                                                                                       \
        );                                                                                                             \
    } while (0)

///
/// Set by default behaves like `MapSetL`.
///
#define MapSet(m, in_key, in_value) MapSetL((m), (in_key), (in_value))

#endif // MISRA_STD_CONTAINER_MAP_INSERT_H
