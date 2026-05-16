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
/// Full initializer with every knob exposed. Builds the Map struct
/// literal directly; other `MapInit*` variants forward to this with
/// some arguments preset.
///
#define MapInitFull_9(                                                                                                 \
    hash_fn,                                                                                                           \
    compare_fn,                                                                                                        \
    value_compare_fn,                                                                                                  \
    key_ci,                                                                                                            \
    key_cd,                                                                                                            \
    value_ci,                                                                                                          \
    value_cd,                                                                                                          \
    policy_value,                                                                                                      \
    typed_alloc_ptr                                                                                                    \
)                                                                                                                      \
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
     .__magic           = MAP_MAGIC}

#define MapInitFull(...) MISRA_OVERLOAD(MapInitFull, __VA_ARGS__)
#define MapInitFull_8(hash_fn, compare_fn, vcmp, kci, kcd, vci, vcd, policy_value)                                     \
    MapInitFull_9(hash_fn, compare_fn, vcmp, kci, kcd, vci, vcd, policy_value, MisraScope)

///
/// Initialize a map with required key hash and compare callbacks plus a
/// user-owned allocator. Uses linear probing. Allocator argument is
/// optional inside a `Scope` block.
///
#define MapInit(...)                   MISRA_OVERLOAD(MapInit, __VA_ARGS__)
#define MapInit_2(hash_fn, compare_fn) MapInit_3(hash_fn, compare_fn, MisraScope)
#define MapInit_3(hash_fn, compare_fn, typed_alloc_ptr)                                                                \
    MapInitFull_9((hash_fn), (compare_fn), NULL, NULL, NULL, NULL, NULL, MapPolicyLinear, typed_alloc_ptr)

///
/// Initialize a map with key/value comparators.
///
#define MapInitWithValueCompare(...) MISRA_OVERLOAD(MapInitWithValueCompare, __VA_ARGS__)
#define MapInitWithValueCompare_3(hash_fn, compare_fn, value_compare_fn)                                               \
    MapInitWithValueCompare_4(hash_fn, compare_fn, value_compare_fn, MisraScope)
#define MapInitWithValueCompare_4(hash_fn, compare_fn, value_compare_fn, typed_alloc_ptr)                              \
    MapInitFull_9((hash_fn), (compare_fn), (value_compare_fn), NULL, NULL, NULL, NULL, MapPolicyLinear, typed_alloc_ptr)

///
/// Initialize a map with key callbacks and an explicit probing policy.
///
#define MapInitWithPolicy(...) MISRA_OVERLOAD(MapInitWithPolicy, __VA_ARGS__)
#define MapInitWithPolicy_3(hash_fn, compare_fn, policy_value)                                                         \
    MapInitWithPolicy_4(hash_fn, compare_fn, policy_value, MisraScope)
#define MapInitWithPolicy_4(hash_fn, compare_fn, policy_value, typed_alloc_ptr)                                        \
    MapInitFull_9((hash_fn), (compare_fn), NULL, NULL, NULL, NULL, NULL, (policy_value), typed_alloc_ptr)

///
/// Initialize a map with key/value comparators and an explicit probing policy.
///
#define MapInitWithValueCompareAndPolicy(...) MISRA_OVERLOAD(MapInitWithValueCompareAndPolicy, __VA_ARGS__)
#define MapInitWithValueCompareAndPolicy_4(hash_fn, compare_fn, value_compare_fn, policy_value)                        \
    MapInitWithValueCompareAndPolicy_5(hash_fn, compare_fn, value_compare_fn, policy_value, MisraScope)
#define MapInitWithValueCompareAndPolicy_5(hash_fn, compare_fn, value_compare_fn, policy_value, typed_alloc_ptr)       \
    MapInitFull_9((hash_fn), (compare_fn), (value_compare_fn), NULL, NULL, NULL, NULL, (policy_value), typed_alloc_ptr)

///
/// Initialize a map with deep-copy callbacks for keys and values.
///
#define MapInitWithDeepCopy(...) MISRA_OVERLOAD(MapInitWithDeepCopy, __VA_ARGS__)
#define MapInitWithDeepCopy_6(hash_fn, compare_fn, key_ci, key_cd, value_ci, value_cd)                                 \
    MapInitWithDeepCopy_7(hash_fn, compare_fn, key_ci, key_cd, value_ci, value_cd, MisraScope)
#define MapInitWithDeepCopy_7(hash_fn, compare_fn, key_ci, key_cd, value_ci, value_cd, typed_alloc_ptr)                \
    MapInitFull_9(                                                                                                     \
        (hash_fn),                                                                                                     \
        (compare_fn),                                                                                                  \
        NULL,                                                                                                          \
        (key_ci),                                                                                                      \
        (key_cd),                                                                                                      \
        (value_ci),                                                                                                    \
        (value_cd),                                                                                                    \
        MapPolicyLinear,                                                                                               \
        typed_alloc_ptr                                                                                                \
    )

#define MapInitT(m, ...) MISRA_OVERLOAD(MapInitT, m, __VA_ARGS__)
#ifdef __cplusplus
#    define MapInitT_3(m, hash_fn, compare_fn) (TYPE_OF(m) MapInit_3((hash_fn), (compare_fn), MisraScope))
#    define MapInitT_4(m, hash_fn, compare_fn, typed_alloc_ptr)                                                        \
        (TYPE_OF(m) MapInit_3((hash_fn), (compare_fn), typed_alloc_ptr))
#else
#    define MapInitT_3(m, hash_fn, compare_fn) ((TYPE_OF(m))MapInit_3((hash_fn), (compare_fn), MisraScope))
#    define MapInitT_4(m, hash_fn, compare_fn, typed_alloc_ptr)                                                        \
        ((TYPE_OF(m))MapInit_3((hash_fn), (compare_fn), typed_alloc_ptr))
#endif

#define MapInitWithDeepCopyT(m, ...) MISRA_OVERLOAD(MapInitWithDeepCopyT, m, __VA_ARGS__)
#ifdef __cplusplus
#    define MapInitWithDeepCopyT_7(m, hash_fn, compare_fn, key_ci, key_cd, value_ci, value_cd)                         \
        (TYPE_OF(m)                                                                                                    \
             MapInitWithDeepCopy_7((hash_fn), (compare_fn), (key_ci), (key_cd), (value_ci), (value_cd), MisraScope))
#    define MapInitWithDeepCopyT_8(m, hash_fn, compare_fn, key_ci, key_cd, value_ci, value_cd, typed_alloc_ptr)        \
        (TYPE_OF(                                                                                                      \
            m                                                                                                          \
        ) MapInitWithDeepCopy_7((hash_fn), (compare_fn), (key_ci), (key_cd), (value_ci), (value_cd), typed_alloc_ptr))
#else
#    define MapInitWithDeepCopyT_7(m, hash_fn, compare_fn, key_ci, key_cd, value_ci, value_cd)                         \
        ((TYPE_OF(m))                                                                                                  \
             MapInitWithDeepCopy_7((hash_fn), (compare_fn), (key_ci), (key_cd), (value_ci), (value_cd), MisraScope))
#    define MapInitWithDeepCopyT_8(m, hash_fn, compare_fn, key_ci, key_cd, value_ci, value_cd, typed_alloc_ptr)        \
        ((                                                                                                             \
            TYPE_OF(m)                                                                                                 \
        )MapInitWithDeepCopy_7((hash_fn), (compare_fn), (key_ci), (key_cd), (value_ci), (value_cd), typed_alloc_ptr))
#endif

#define MapDeinit(m)                                                                                                   \
    deinit_map(                                                                                                        \
        GENERIC_MAP(m),                                                                                                \
        sizeof(MAP_ENTRY_TYPE(m)),                                                                                     \
        offsetof(MAP_ENTRY_TYPE(m), key),                                                                              \
        sizeof(MAP_KEY_TYPE(m)),                                                                                       \
        offsetof(MAP_ENTRY_TYPE(m), value),                                                                            \
        sizeof(MAP_VALUE_TYPE(m)),                                                                                     \
        offsetof(MAP_ENTRY_TYPE(m), hash)                                                                              \
    )

#endif // MISRA_STD_CONTAINER_MAP_INIT_H
