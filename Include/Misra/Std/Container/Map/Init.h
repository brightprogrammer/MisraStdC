/// file      : std/container/map/init.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Initializers for Map.

#ifndef MISRA_STD_CONTAINER_MAP_INIT_H
#define MISRA_STD_CONTAINER_MAP_INIT_H

#include "Private.h"
#include "Type.h"

///
/// Initialize a map with required key hash and compare callbacks plus a
/// user-owned allocator. Uses linear probing.
///
#define MapInit(hash_fn, compare_fn, typed_alloc_ptr)                                                                  \
    MapInitFull((hash_fn), (compare_fn), NULL, NULL, NULL, NULL, NULL, MisraMapPolicyLinear, typed_alloc_ptr)

///
/// Initialize a map with key/value comparators.
///
#define MapInitWithValueCompare(hash_fn, compare_fn, value_compare_fn, typed_alloc_ptr)                                \
    MapInitFull(                                                                                                       \
        (hash_fn),                                                                                                     \
        (compare_fn),                                                                                                  \
        (value_compare_fn),                                                                                            \
        NULL,                                                                                                          \
        NULL,                                                                                                          \
        NULL,                                                                                                          \
        NULL,                                                                                                          \
        MisraMapPolicyLinear,                                                                                          \
        typed_alloc_ptr)

///
/// Initialize a map with key callbacks and an explicit probing policy.
///
#define MapInitWithPolicy(hash_fn, compare_fn, policy_value, typed_alloc_ptr)                                          \
    MapInitFull((hash_fn), (compare_fn), NULL, NULL, NULL, NULL, NULL, (policy_value), typed_alloc_ptr)

///
/// Initialize a map with key/value comparators and an explicit probing policy.
///
#define MapInitWithValueCompareAndPolicy(hash_fn, compare_fn, value_compare_fn, policy_value, typed_alloc_ptr)         \
    MapInitFull(                                                                                                       \
        (hash_fn),                                                                                                     \
        (compare_fn),                                                                                                  \
        (value_compare_fn),                                                                                            \
        NULL,                                                                                                          \
        NULL,                                                                                                          \
        NULL,                                                                                                          \
        NULL,                                                                                                          \
        (policy_value),                                                                                                \
        typed_alloc_ptr)

///
/// Initialize a map with deep-copy callbacks for keys and values.
///
#define MapInitWithDeepCopy(hash_fn, compare_fn, key_ci, key_cd, value_ci, value_cd, typed_alloc_ptr)                  \
    MapInitFull(                                                                                                       \
        (hash_fn),                                                                                                     \
        (compare_fn),                                                                                                  \
        NULL,                                                                                                          \
        (key_ci),                                                                                                      \
        (key_cd),                                                                                                      \
        (value_ci),                                                                                                    \
        (value_cd),                                                                                                    \
        MisraMapPolicyLinear,                                                                                          \
        typed_alloc_ptr)

///
/// Full initializer with every knob exposed.
///
#define MapInitFull(                                                                                                   \
    hash_fn,                                                                                                           \
    compare_fn,                                                                                                        \
    value_compare_fn,                                                                                                  \
    key_ci,                                                                                                            \
    key_cd,                                                                                                            \
    value_ci,                                                                                                          \
    value_cd,                                                                                                          \
    policy_value,                                                                                                      \
    typed_alloc_ptr)                                                                                                   \
    {.length            = 0,                                                                                           \
     .capacity          = 0,                                                                                           \
     .tombstones        = 0,                                                                                           \
     .key_copy_init     = (GenericCopyInit)(key_ci),                                                                   \
     .key_copy_deinit   = (GenericCopyDeinit)(key_cd),                                                                 \
     .value_copy_init   = (GenericCopyInit)(value_ci),                                                                 \
     .value_copy_deinit = (GenericCopyDeinit)(value_cd),                                                               \
     .key_compare       = (GenericCompare)(compare_fn),                                                                \
     .value_compare     = (GenericCompare)(value_compare_fn),                                                          \
     .key_hash          = (GenericHash)(hash_fn),                                                                      \
     .entries           = NULL,                                                                                        \
     .states            = NULL,                                                                                        \
     .policy            = validate_map_policy_copy((policy_value)),                                                    \
     .allocator         = ALLOCATOR_OF(typed_alloc_ptr),                                                               \
     .__magic           = MISRA_MAP_MAGIC}

#ifdef __cplusplus
#    define MapInitT(m, hash_fn, compare_fn, typed_alloc_ptr) (TYPE_OF(m) MapInit((hash_fn), (compare_fn), typed_alloc_ptr))
#    define MapInitWithDeepCopyT(m, hash_fn, compare_fn, key_ci, key_cd, value_ci, value_cd, typed_alloc_ptr)          \
        (TYPE_OF(m)                                                                                                    \
             MapInitWithDeepCopy((hash_fn), (compare_fn), (key_ci), (key_cd), (value_ci), (value_cd), typed_alloc_ptr))
#else
#    define MapInitT(m, hash_fn, compare_fn, typed_alloc_ptr)                                                          \
        ((TYPE_OF(m))MapInit((hash_fn), (compare_fn), typed_alloc_ptr))
#    define MapInitWithDeepCopyT(m, hash_fn, compare_fn, key_ci, key_cd, value_ci, value_cd, typed_alloc_ptr)          \
        ((TYPE_OF(m))                                                                                                  \
             MapInitWithDeepCopy((hash_fn), (compare_fn), (key_ci), (key_cd), (value_ci), (value_cd), typed_alloc_ptr))
#endif

#define MapDeinit(m)                                                                                                   \
    deinit_map(                                                                                                        \
        GENERIC_MAP(m),                                                                                                \
        sizeof(MAP_ENTRY_TYPE(m)),                                                                                     \
        offsetof(MAP_ENTRY_TYPE(m), key),                                                                              \
        sizeof(MAP_KEY_TYPE(m)),                                                                                       \
        offsetof(MAP_ENTRY_TYPE(m), value),                                                                            \
        sizeof(MAP_VALUE_TYPE(m)),                                                                                     \
        offsetof(MAP_ENTRY_TYPE(m), hash))

#endif // MISRA_STD_CONTAINER_MAP_INIT_H
