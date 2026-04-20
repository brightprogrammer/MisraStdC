/// file      : std/container/map/memory.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Memory management helpers for Map.

#ifndef MISRA_STD_CONTAINER_MAP_MEMORY_H
#define MISRA_STD_CONTAINER_MAP_MEMORY_H

#include "Type.h"
#include "Private.h"

///
/// Clear all entries but retain allocated storage.
///
/// m[in,out] : Hash map.
///
#define MapClear(m)                                                                                                \
    clear_map(                                                                                                     \
        GENERIC_MAP(m),                                                                                            \
        sizeof(MAP_ENTRY_TYPE(m)),                                                                                 \
        offsetof(MAP_ENTRY_TYPE(m), key),                                                                          \
        sizeof(MAP_KEY_TYPE(m)),                                                                                   \
        offsetof(MAP_ENTRY_TYPE(m), value),                                                                        \
        sizeof(MAP_VALUE_TYPE(m)),                                                                                 \
        offsetof(MAP_ENTRY_TYPE(m), hash)                                                                          \
    )

///
/// Reserve enough probe slots for at least `n` entries.
///
/// m[in,out] : Hash map.
/// n[in]     : Minimum number of entries expected.
///
#define MapReserve(m, n)                                                                                           \
    reserve_map(                                                                                                   \
        GENERIC_MAP(m),                                                                                            \
        sizeof(MAP_ENTRY_TYPE(m)),                                                                                 \
        offsetof(MAP_ENTRY_TYPE(m), key),                                                                          \
        sizeof(MAP_KEY_TYPE(m)),                                                                                   \
        offsetof(MAP_ENTRY_TYPE(m), value),                                                                        \
        sizeof(MAP_VALUE_TYPE(m)),                                                                                 \
        offsetof(MAP_ENTRY_TYPE(m), hash),                                                                         \
        (n)                                                                                                            \
    )

///
/// Remap to current policy and at least `n` expected entries.
///
/// m[in,out] : Hash map.
/// n[in]     : Minimum number of entries expected after rehash.
///
#define MapRehash(m, n)                                                                                            \
    rehash_map(                                                                                                    \
        GENERIC_MAP(m),                                                                                            \
        sizeof(MAP_ENTRY_TYPE(m)),                                                                                 \
        offsetof(MAP_ENTRY_TYPE(m), key),                                                                          \
        sizeof(MAP_KEY_TYPE(m)),                                                                                   \
        offsetof(MAP_ENTRY_TYPE(m), value),                                                                        \
        sizeof(MAP_VALUE_TYPE(m)),                                                                                 \
        offsetof(MAP_ENTRY_TYPE(m), hash),                                                                         \
        (n),                                                                                                           \
        (m)->policy                                                                                                    \
    )

///
/// Remap using a specific probing policy.
///
/// m[in,out] : Hash map.
/// n[in]     : Minimum number of entries expected after rehash.
/// policy[in]: New probing policy copied into this map.
///
#define MapRehashWithPolicy(m, n, policy_value)                                                                    \
    rehash_map(                                                                                                    \
        GENERIC_MAP(m),                                                                                            \
        sizeof(MAP_ENTRY_TYPE(m)),                                                                                 \
        offsetof(MAP_ENTRY_TYPE(m), key),                                                                          \
        sizeof(MAP_KEY_TYPE(m)),                                                                                   \
        offsetof(MAP_ENTRY_TYPE(m), value),                                                                        \
        sizeof(MAP_VALUE_TYPE(m)),                                                                                 \
        offsetof(MAP_ENTRY_TYPE(m), hash),                                                                         \
        (n),                                                                                                           \
        (policy_value)                                                                                                 \
    )

#endif // MISRA_STD_CONTAINER_MAP_MEMORY_H
