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
#define MapInit(hash_fn, compare_fn)                                                                                   \
    MapInitWithDeepCopyAndValueCompareAndPolicy(                                                                       \
        (hash_fn),                                                                                                     \
        (compare_fn),                                                                                                  \
        NULL,                                                                                                          \
        NULL,                                                                                                          \
        NULL,                                                                                                          \
        NULL,                                                                                                          \
        NULL,                                                                                                          \
        MisraMapPolicyLinear                                                                                           \
    )

///
/// Initialize map with required key hash, key compare, and value compare callbacks.
/// Uses linear probing by default.
///
/// hash_fn[in]          : Hash callback for keys.
/// compare_fn[in]       : Key comparator.
/// value_compare_fn[in] : Value comparator used by pair-level APIs.
///
/// TAGS: Map, Init, Compare, Linear
///
#define MapInitWithValueCompare(hash_fn, compare_fn, value_compare_fn)                                                 \
    MapInitWithDeepCopyAndValueCompareAndPolicy(                                                                       \
        (hash_fn),                                                                                                     \
        (compare_fn),                                                                                                  \
        (value_compare_fn),                                                                                            \
        NULL,                                                                                                          \
        NULL,                                                                                                          \
        NULL,                                                                                                          \
        NULL,                                                                                                          \
        MisraMapPolicyLinear                                                                                           \
    )

///
/// Initialize map with required key hash and compare callbacks.
///
/// hash_fn[in]    : Hash callback for keys.
/// compare_fn[in] : Key comparator. Equality is `compare_fn(a, b) == 0`.
/// policy[in]     : Probing policy for this map.
///
/// TAGS: Map, Init, Policy
///
#define MapInitWithPolicy(hash_fn, compare_fn, policy_value)                                                           \
    MapInitWithDeepCopyAndValueCompareAndPolicy((hash_fn), (compare_fn), NULL, NULL, NULL, NULL, NULL, (policy_value))

///
/// Initialize map with key/value comparators and an explicit probing policy.
///
/// hash_fn[in]          : Hash callback for keys.
/// compare_fn[in]       : Key comparator.
/// value_compare_fn[in] : Value comparator used by pair-level APIs.
/// policy[in]           : Probing policy for this map.
///
/// TAGS: Map, Init, Compare, Policy
///
#define MapInitWithValueCompareAndPolicy(hash_fn, compare_fn, value_compare_fn, policy_value)                          \
    MapInitWithDeepCopyAndValueCompareAndPolicy(                                                                       \
        (hash_fn),                                                                                                     \
        (compare_fn),                                                                                                  \
        (value_compare_fn),                                                                                            \
        NULL,                                                                                                          \
        NULL,                                                                                                          \
        NULL,                                                                                                          \
        NULL,                                                                                                          \
        (policy_value)                                                                                                 \
    )

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
#define MapInitWithDeepCopy(hash_fn, compare_fn, key_ci, key_cd, value_ci, value_cd)                                   \
    MapInitWithDeepCopyAndValueCompareAndPolicy(                                                                       \
        (hash_fn),                                                                                                     \
        (compare_fn),                                                                                                  \
        NULL,                                                                                                          \
        (key_ci),                                                                                                      \
        (key_cd),                                                                                                      \
        (value_ci),                                                                                                    \
        (value_cd),                                                                                                    \
        MisraMapPolicyLinear                                                                                           \
    )

///
/// Initialize map with deep-copy callbacks and a stored value comparator.
/// Uses linear probing by default.
///
/// hash_fn[in]          : Hash callback for keys.
/// compare_fn[in]       : Key comparator.
/// value_compare_fn[in] : Value comparator used by pair-level APIs.
/// key_ci[in]           : Optional key deep-copy callback.
/// key_cd[in]           : Optional key deinit callback.
/// value_ci[in]         : Optional value deep-copy callback.
/// value_cd[in]         : Optional value deinit callback.
///
/// TAGS: Map, Init, DeepCopy, Compare, Linear
///
#define MapInitWithDeepCopyAndValueCompare(hash_fn, compare_fn, value_compare_fn, key_ci, key_cd, value_ci, value_cd)  \
    MapInitWithDeepCopyAndValueCompareAndPolicy(                                                                       \
        (hash_fn),                                                                                                     \
        (compare_fn),                                                                                                  \
        (value_compare_fn),                                                                                            \
        (key_ci),                                                                                                      \
        (key_cd),                                                                                                      \
        (value_ci),                                                                                                    \
        (value_cd),                                                                                                    \
        MisraMapPolicyLinear                                                                                           \
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
#define MapInitWithDeepCopyAndPolicy(hash_fn, compare_fn, key_ci, key_cd, value_ci, value_cd, policy_value)            \
    MapInitWithDeepCopyAndValueCompareAndPolicy(                                                                       \
        (hash_fn),                                                                                                     \
        (compare_fn),                                                                                                  \
        NULL,                                                                                                          \
        (key_ci),                                                                                                      \
        (key_cd),                                                                                                      \
        (value_ci),                                                                                                    \
        (value_cd),                                                                                                    \
        (policy_value)                                                                                                 \
    )

///
/// Initialize map with deep-copy callbacks, optional stored value comparator,
/// and explicit probing policy.
///
/// hash_fn[in]          : Hash callback for keys.
/// compare_fn[in]       : Key comparator.
/// value_compare_fn[in] : Value comparator used by pair-level APIs.
/// key_ci[in]           : Optional key deep-copy callback.
/// key_cd[in]           : Optional key deinit callback.
/// value_ci[in]         : Optional value deep-copy callback.
/// value_cd[in]         : Optional value deinit callback.
/// policy_value[in]     : Probing policy copied into this map.
///
/// TAGS: Map, Init, DeepCopy, Compare, Policy
///
#define MapInitWithDeepCopyAndValueCompareAndPolicy(                                                                   \
    hash_fn,                                                                                                           \
    compare_fn,                                                                                                        \
    value_compare_fn,                                                                                                  \
    key_ci,                                                                                                            \
    key_cd,                                                                                                            \
    value_ci,                                                                                                          \
    value_cd,                                                                                                          \
    policy_value                                                                                                       \
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
     .allocator         = AllocatorBind(DefaultAllocator()),                                                           \
     .__magic           = MISRA_MAP_MAGIC}

#ifdef __cplusplus
///
/// Typed initializer for a `Map(K, V)` typedef. Casts the inner anonymous
/// struct produced by `MapInit` to the caller's declared type.
///
/// m[in]          : Variable whose type tag is recovered via `TYPE_OF(m)`.
/// hash_fn[in]    : Hash callback for keys.
/// compare_fn[in] : Key comparator.
///
/// SUCCESS : Returns a value of type `TYPE_OF(m)` initialized as an empty
///           map (length 0, no probe table, linear policy bound). No heap
///           allocation is performed.
/// FAILURE : Function cannot fail.
///
/// TAGS: Map, Init, Typed, Linear
///
#    define MapInitT(m, hash_fn, compare_fn) (TYPE_OF(m) MapInit((hash_fn), (compare_fn)))

///
/// Typed initializer with stored value comparator.
///
/// SUCCESS : Returns a value of type `TYPE_OF(m)` initialized with the
///           given key/value comparators and linear probing.
/// FAILURE : Function cannot fail.
///
/// TAGS: Map, Init, Typed, Compare
///
#    define MapInitWithValueCompareT(m, hash_fn, compare_fn, value_compare_fn)                                         \
        (TYPE_OF(m) MapInitWithValueCompare((hash_fn), (compare_fn), (value_compare_fn)))

///
/// Typed initializer with explicit probing policy.
///
/// SUCCESS : Returns a value of type `TYPE_OF(m)` initialized with the
///           given key callbacks and explicit probing policy.
/// FAILURE : Function cannot fail.
///
/// TAGS: Map, Init, Typed, Policy
///
#    define MapInitWithPolicyT(m, hash_fn, compare_fn, policy_value)                                                   \
        (TYPE_OF(m) MapInitWithPolicy((hash_fn), (compare_fn), (policy_value)))

///
/// Typed initializer with value comparator and explicit probing policy.
///
/// SUCCESS : Returns a value of type `TYPE_OF(m)` initialized with the
///           given key/value comparators and probing policy.
/// FAILURE : Function cannot fail.
///
/// TAGS: Map, Init, Typed, Compare, Policy
///
#    define MapInitWithValueCompareAndPolicyT(m, hash_fn, compare_fn, value_compare_fn, policy_value)                  \
        (TYPE_OF(m) MapInitWithValueCompareAndPolicy((hash_fn), (compare_fn), (value_compare_fn), (policy_value)))

///
/// Typed initializer with deep-copy callbacks for keys and values.
///
/// SUCCESS : Returns a value of type `TYPE_OF(m)` initialized with the
///           given key/value deep-copy callbacks and linear probing.
/// FAILURE : Function cannot fail.
///
/// TAGS: Map, Init, Typed, DeepCopy, Linear
///
#    define MapInitWithDeepCopyT(m, hash_fn, compare_fn, key_ci, key_cd, value_ci, value_cd)                           \
        (TYPE_OF(m) MapInitWithDeepCopy((hash_fn), (compare_fn), (key_ci), (key_cd), (value_ci), (value_cd)))

///
/// Typed initializer with deep-copy callbacks and stored value comparator.
///
/// SUCCESS : Returns a value of type `TYPE_OF(m)` initialized with the
///           given comparators and deep-copy callbacks.
/// FAILURE : Function cannot fail.
///
/// TAGS: Map, Init, Typed, DeepCopy, Compare
///
#    define MapInitWithDeepCopyAndValueCompareT(                                                                       \
        m,                                                                                                             \
        hash_fn,                                                                                                       \
        compare_fn,                                                                                                    \
        value_compare_fn,                                                                                              \
        key_ci,                                                                                                        \
        key_cd,                                                                                                        \
        value_ci,                                                                                                      \
        value_cd                                                                                                       \
    )                                                                                                                  \
        (TYPE_OF(m) MapInitWithDeepCopyAndValueCompare(                                                                \
            (hash_fn),                                                                                                 \
            (compare_fn),                                                                                              \
            (value_compare_fn),                                                                                        \
            (key_ci),                                                                                                  \
            (key_cd),                                                                                                  \
            (value_ci),                                                                                                \
            (value_cd)                                                                                                 \
        ))

///
/// Typed initializer with deep-copy callbacks and explicit probing policy.
///
/// SUCCESS : Returns a value of type `TYPE_OF(m)` initialized with the
///           given deep-copy callbacks and probing policy.
/// FAILURE : Function cannot fail.
///
/// TAGS: Map, Init, Typed, DeepCopy, Policy
///
#    define MapInitWithDeepCopyAndPolicyT(m, hash_fn, compare_fn, key_ci, key_cd, value_ci, value_cd, policy_value)    \
        (TYPE_OF(m) MapInitWithDeepCopyAndPolicy(                                                                      \
            (hash_fn),                                                                                                 \
            (compare_fn),                                                                                              \
            (key_ci),                                                                                                  \
            (key_cd),                                                                                                  \
            (value_ci),                                                                                                \
            (value_cd),                                                                                                \
            (policy_value)                                                                                             \
        ))

///
/// Typed initializer with deep-copy callbacks, value comparator, and
/// explicit probing policy.
///
/// SUCCESS : Returns a value of type `TYPE_OF(m)` initialized with the
///           full callback set and probing policy.
/// FAILURE : Function cannot fail.
///
/// TAGS: Map, Init, Typed, DeepCopy, Compare, Policy
///
#    define MapInitWithDeepCopyAndValueCompareAndPolicyT(                                                              \
        m,                                                                                                             \
        hash_fn,                                                                                                       \
        compare_fn,                                                                                                    \
        value_compare_fn,                                                                                              \
        key_ci,                                                                                                        \
        key_cd,                                                                                                        \
        value_ci,                                                                                                      \
        value_cd,                                                                                                      \
        policy_value                                                                                                   \
    )                                                                                                                  \
        (TYPE_OF(m) MapInitWithDeepCopyAndValueCompareAndPolicy(                                                       \
            (hash_fn),                                                                                                 \
            (compare_fn),                                                                                              \
            (value_compare_fn),                                                                                        \
            (key_ci),                                                                                                  \
            (key_cd),                                                                                                  \
            (value_ci),                                                                                                \
            (value_cd),                                                                                                \
            (policy_value)                                                                                             \
        ))
#else
#    define MapInitT(m, hash_fn, compare_fn) ((TYPE_OF(m))MapInit((hash_fn), (compare_fn)))
#    define MapInitWithValueCompareT(m, hash_fn, compare_fn, value_compare_fn)                                         \
        ((TYPE_OF(m))MapInitWithValueCompare((hash_fn), (compare_fn), (value_compare_fn)))
#    define MapInitWithPolicyT(m, hash_fn, compare_fn, policy_value)                                                   \
        ((TYPE_OF(m))MapInitWithPolicy((hash_fn), (compare_fn), (policy_value)))
#    define MapInitWithValueCompareAndPolicyT(m, hash_fn, compare_fn, value_compare_fn, policy_value)                  \
        ((TYPE_OF(m))MapInitWithValueCompareAndPolicy((hash_fn), (compare_fn), (value_compare_fn), (policy_value)))
#    define MapInitWithDeepCopyT(m, hash_fn, compare_fn, key_ci, key_cd, value_ci, value_cd)                           \
        ((TYPE_OF(m))MapInitWithDeepCopy((hash_fn), (compare_fn), (key_ci), (key_cd), (value_ci), (value_cd)))
#    define MapInitWithDeepCopyAndValueCompareT(                                                                       \
        m,                                                                                                             \
        hash_fn,                                                                                                       \
        compare_fn,                                                                                                    \
        value_compare_fn,                                                                                              \
        key_ci,                                                                                                        \
        key_cd,                                                                                                        \
        value_ci,                                                                                                      \
        value_cd                                                                                                       \
    )                                                                                                                  \
        ((TYPE_OF(m))MapInitWithDeepCopyAndValueCompare(                                                               \
            (hash_fn),                                                                                                 \
            (compare_fn),                                                                                              \
            (value_compare_fn),                                                                                        \
            (key_ci),                                                                                                  \
            (key_cd),                                                                                                  \
            (value_ci),                                                                                                \
            (value_cd)                                                                                                 \
        ))
#    define MapInitWithDeepCopyAndPolicyT(m, hash_fn, compare_fn, key_ci, key_cd, value_ci, value_cd, policy_value)    \
        ((TYPE_OF(m))MapInitWithDeepCopyAndPolicy(                                                                     \
            (hash_fn),                                                                                                 \
            (compare_fn),                                                                                              \
            (key_ci),                                                                                                  \
            (key_cd),                                                                                                  \
            (value_ci),                                                                                                \
            (value_cd),                                                                                                \
            (policy_value)                                                                                             \
        ))
#    define MapInitWithDeepCopyAndValueCompareAndPolicyT(                                                              \
        m,                                                                                                             \
        hash_fn,                                                                                                       \
        compare_fn,                                                                                                    \
        value_compare_fn,                                                                                              \
        key_ci,                                                                                                        \
        key_cd,                                                                                                        \
        value_ci,                                                                                                      \
        value_cd,                                                                                                      \
        policy_value                                                                                                   \
    )                                                                                                                  \
        ((TYPE_OF(m))MapInitWithDeepCopyAndValueCompareAndPolicy(                                                      \
            (hash_fn),                                                                                                 \
            (compare_fn),                                                                                              \
            (value_compare_fn),                                                                                        \
            (key_ci),                                                                                                  \
            (key_cd),                                                                                                  \
            (value_ci),                                                                                                \
            (value_cd),                                                                                                \
            (policy_value)                                                                                             \
        ))
#endif

///
/// Deinitialize given map and all contained keys and values.
///
/// m[in,out] : Pointer to `Map` to deinitialize.
///
/// SUCCESS : Returns to the caller. The entries table and states table
///           have been freed back to the map's allocator. `key_copy_deinit`
///           and `value_copy_deinit` (if configured) have been invoked on
///           every previously-occupied slot. Length, capacity, and
///           tombstone count are reset; allocator binding is unbound. The
///           map object can be safely re-initialized or discarded.
/// FAILURE : Function cannot fail. A NULL `m` or invalid magic is a
///           caller bug and aborts via `LOG_FATAL`.
///
/// TAGS: Map, Deinit, Memory
///
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
