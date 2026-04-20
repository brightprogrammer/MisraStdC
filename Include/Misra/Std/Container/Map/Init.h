/// file      : std/container/map/init.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Initializers for Map.

#ifndef MISRA_STD_CONTAINER_MAP_INIT_H
#define MISRA_STD_CONTAINER_MAP_INIT_H

#include "Type.h"
#include "Private.h"

///
/// Initialize map with required key hash and compare callbacks.
/// Uses linear probing by default.
///
/// hash_fn[in]    : Hash callback for keys.
/// compare_fn[in] : Key comparator. Equality is `compare_fn(a, b) == 0`.
///
/// USAGE:
///   typedef Map(int, Str) IntStrMap;
///   IntStrMap map = MapInit(IntHash, IntCompare);
///
/// TAGS: Map, Init, Policy, Linear
///
#define MapInit(hash_fn, compare_fn)                                                                               \
    MapInitWithDeepCopyAndPolicy((hash_fn), (compare_fn), NULL, NULL, NULL, NULL, MisraMapPolicyLinear)

///
/// Initialize map with required key hash and compare callbacks.
///
/// hash_fn[in]    : Hash callback for keys.
/// compare_fn[in] : Key comparator. Equality is `compare_fn(a, b) == 0`.
/// policy[in]     : Probing policy for this map.
///
/// TAGS: Map, Init, Policy
///
#define MapInitWithPolicy(hash_fn, compare_fn, policy_value)                                                       \
    MapInitWithDeepCopyAndPolicy((hash_fn), (compare_fn), NULL, NULL, NULL, NULL, (policy_value))

///
/// Initialize map with deep-copy callbacks for keys and values.
/// Uses linear probing by default.
///
/// hash_fn[in]          : Hash callback for keys.
/// compare_fn[in]       : Key comparator.
/// key_ci[in]           : Optional key deep-copy callback.
/// key_cd[in]           : Optional key deinit callback.
/// value_ci[in]         : Optional value deep-copy callback.
/// value_cd[in]         : Optional value deinit callback.
///
/// TAGS: Map, Init, DeepCopy, Linear
///
#define MapInitWithDeepCopy(hash_fn, compare_fn, key_ci, key_cd, value_ci, value_cd)                              \
    MapInitWithDeepCopyAndPolicy(                                                                                  \
        (hash_fn),                                                                                                     \
        (compare_fn),                                                                                                  \
        (key_ci),                                                                                                      \
        (key_cd),                                                                                                      \
        (value_ci),                                                                                                    \
        (value_cd),                                                                                                    \
        MisraMapPolicyLinear                                                                                        \
    )

///
/// Initialize map with deep-copy callbacks and explicit probing policy.
///
/// hash_fn[in]          : Hash callback for keys.
/// compare_fn[in]       : Key comparator.
/// key_ci[in]           : Optional key deep-copy callback.
/// key_cd[in]           : Optional key deinit callback.
/// value_ci[in]         : Optional value deep-copy callback.
/// value_cd[in]         : Optional value deinit callback.
/// policy_value[in]     : Probing policy copied into this map.
///
/// TAGS: Map, Init, DeepCopy, Policy
///
#define MapInitWithDeepCopyAndPolicy(hash_fn, compare_fn, key_ci, key_cd, value_ci, value_cd, policy_value)      \
    {.length            = 0,                                                                                           \
     .capacity          = 0,                                                                                           \
     .key_copy_init     = (GenericCopyInit)(key_ci),                                                                   \
     .key_copy_deinit   = (GenericCopyDeinit)(key_cd),                                                                 \
     .value_copy_init   = (GenericCopyInit)(value_ci),                                                                 \
     .value_copy_deinit = (GenericCopyDeinit)(value_cd),                                                               \
     .key_compare       = (GenericCompare)(compare_fn),                                                                \
     .key_hash          = (GenericHash)(hash_fn),                                                                      \
     .entries           = NULL,                                                                                        \
     .states            = NULL,                                                                                        \
     .policy            = (policy_value),                                                                              \
     .__magic           = MISRA_MAP_MAGIC}

#ifdef __cplusplus
#    define MapInitT(m, hash_fn, compare_fn)                                                                       \
        (TYPE_OF(m) MapInit((hash_fn), (compare_fn)))
#    define MapInitWithPolicyT(m, hash_fn, compare_fn, policy_value)                                               \
        (TYPE_OF(m) MapInitWithPolicy((hash_fn), (compare_fn), (policy_value)))
#    define MapInitWithDeepCopyT(m, hash_fn, compare_fn, key_ci, key_cd, value_ci, value_cd)                      \
        (TYPE_OF(m) MapInitWithDeepCopy((hash_fn), (compare_fn), (key_ci), (key_cd), (value_ci), (value_cd)))
#    define MapInitWithDeepCopyAndPolicyT(m, hash_fn, compare_fn, key_ci, key_cd, value_ci, value_cd, policy_value) \
        (TYPE_OF(m)                                                                                                    \
             MapInitWithDeepCopyAndPolicy(                                                                         \
                 (hash_fn),                                                                                            \
                 (compare_fn),                                                                                         \
                 (key_ci),                                                                                             \
                 (key_cd),                                                                                             \
                 (value_ci),                                                                                           \
                 (value_cd),                                                                                           \
                 (policy_value)                                                                                        \
             ))
#else
#    define MapInitT(m, hash_fn, compare_fn)                                                                       \
        ((TYPE_OF(m))MapInit((hash_fn), (compare_fn)))
#    define MapInitWithPolicyT(m, hash_fn, compare_fn, policy_value)                                               \
        ((TYPE_OF(m))MapInitWithPolicy((hash_fn), (compare_fn), (policy_value)))
#    define MapInitWithDeepCopyT(m, hash_fn, compare_fn, key_ci, key_cd, value_ci, value_cd)                      \
        ((TYPE_OF(m))MapInitWithDeepCopy((hash_fn), (compare_fn), (key_ci), (key_cd), (value_ci), (value_cd)))
#    define MapInitWithDeepCopyAndPolicyT(m, hash_fn, compare_fn, key_ci, key_cd, value_ci, value_cd, policy_value) \
        ((TYPE_OF(m))                                                                                                  \
             MapInitWithDeepCopyAndPolicy(                                                                         \
                 (hash_fn),                                                                                            \
                 (compare_fn),                                                                                         \
                 (key_ci),                                                                                             \
                 (key_cd),                                                                                             \
                 (value_ci),                                                                                           \
                 (value_cd),                                                                                           \
                 (policy_value)                                                                                        \
             ))
#endif

///
/// Deinitialize given map and all contained keys and values.
///
/// m[in,out] : Pointer to `Map` to deinitialize.
///
/// TAGS: Map, Deinit, Memory
///
#define MapDeinit(m)                                                                                               \
    deinit_map(                                                                                                    \
        GENERIC_MAP(m),                                                                                            \
        sizeof(MAP_ENTRY_TYPE(m)),                                                                                 \
        offsetof(MAP_ENTRY_TYPE(m), key),                                                                          \
        sizeof(MAP_KEY_TYPE(m)),                                                                                   \
        offsetof(MAP_ENTRY_TYPE(m), value),                                                                        \
        sizeof(MAP_VALUE_TYPE(m)),                                                                                 \
        offsetof(MAP_ENTRY_TYPE(m), hash)                                                                          \
    )

#endif // MISRA_STD_CONTAINER_MAP_INIT_H
