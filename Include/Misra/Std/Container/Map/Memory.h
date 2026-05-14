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
/// Entries are deinitialized via the configured `key_copy_deinit` and
/// `value_copy_deinit` handlers when present.
///
/// m[in,out] : Hash map.
///
/// TAGS: Map, Memory, Clear
///
#define MapClear(m)                                                                                                    \
    clear_map(                                                                                                         \
        GENERIC_MAP(m),                                                                                                \
        sizeof(MAP_ENTRY_TYPE(m)),                                                                                     \
        offsetof(MAP_ENTRY_TYPE(m), key),                                                                              \
        sizeof(MAP_KEY_TYPE(m)),                                                                                       \
        offsetof(MAP_ENTRY_TYPE(m), value),                                                                            \
        sizeof(MAP_VALUE_TYPE(m)),                                                                                     \
        offsetof(MAP_ENTRY_TYPE(m), hash)                                                                              \
    )

///
/// Reserve enough probe slots for at least `n` entries.
///
/// m[in,out] : Hash map.
/// n[in]     : Minimum number of entries expected.
///
/// SUCCESS : `true`.
/// FAILURE : `false` on allocation failure. The map is unchanged.
///
/// TAGS: Map, Memory, Reserve
///
#define MapReserve(m, n)                                                                                               \
    reserve_map(                                                                                                       \
        GENERIC_MAP(m),                                                                                                \
        sizeof(MAP_ENTRY_TYPE(m)),                                                                                     \
        offsetof(MAP_ENTRY_TYPE(m), key),                                                                              \
        sizeof(MAP_KEY_TYPE(m)),                                                                                       \
        offsetof(MAP_ENTRY_TYPE(m), value),                                                                            \
        sizeof(MAP_VALUE_TYPE(m)),                                                                                     \
        offsetof(MAP_ENTRY_TYPE(m), hash),                                                                             \
        (n)                                                                                                            \
    )
///
/// Aborting variant of `MapReserve`. Calls `LOG_FATAL` on allocation failure.
///
/// SUCCESS : Returns to the caller.
/// FAILURE : Does not return - aborts via `LOG_FATAL` / `SysAbort`.
///
/// TAGS: Map, Memory, Reserve, Must, Abort
///
#define MapMustReserve(m, n)                                                                                           \
    do {                                                                                                               \
        if (!MapReserve((m), (n))) {                                                                                   \
            LOG_FATAL("MapMustReserve failed");                                                                        \
        }                                                                                                              \
    } while (0)

///
/// Rebuild the map using the current policy and current pair count.
/// This removes tombstones and re-packs the probe table.
///
/// m[in,out] : Map.
///
/// SUCCESS : `true`.
/// FAILURE : `false` on allocation failure for the new probe table.
///
/// TAGS: Map, Memory, Rehash, Compact
///
#define MapCompact(m)                                                                                                  \
    rehash_map(                                                                                                        \
        GENERIC_MAP(m),                                                                                                \
        sizeof(MAP_ENTRY_TYPE(m)),                                                                                     \
        offsetof(MAP_ENTRY_TYPE(m), key),                                                                              \
        sizeof(MAP_KEY_TYPE(m)),                                                                                       \
        offsetof(MAP_ENTRY_TYPE(m), value),                                                                            \
        sizeof(MAP_VALUE_TYPE(m)),                                                                                     \
        offsetof(MAP_ENTRY_TYPE(m), hash),                                                                             \
        (size)((m)->length),                                                                                           \
        (m)->policy                                                                                                    \
    )
///
/// Aborting variant of `MapCompact`. Calls `LOG_FATAL` on allocation failure.
///
/// SUCCESS : Returns to the caller.
/// FAILURE : Does not return - aborts via `LOG_FATAL` / `SysAbort`.
///
/// TAGS: Map, Memory, Rehash, Compact, Must, Abort
///
#define MapMustCompact(m)                                                                                              \
    do {                                                                                                               \
        if (!MapCompact((m))) {                                                                                        \
            LOG_FATAL("MapMustCompact failed");                                                                        \
        }                                                                                                              \
    } while (0)

///
/// Remap using a specific probing policy.
///
/// m[in,out]    : Hash map.
/// n[in]        : Minimum number of entries expected after rehash.
/// policy_value : New probing policy copied into this map.
///
/// SUCCESS : `true`.
/// FAILURE : `false` on allocation failure during the new probe table build.
///
/// TAGS: Map, Memory, Rehash, Policy
///
#define MapRehashWithPolicy(m, n, policy_value)                                                                        \
    rehash_map(                                                                                                        \
        GENERIC_MAP(m),                                                                                                \
        sizeof(MAP_ENTRY_TYPE(m)),                                                                                     \
        offsetof(MAP_ENTRY_TYPE(m), key),                                                                              \
        sizeof(MAP_KEY_TYPE(m)),                                                                                       \
        offsetof(MAP_ENTRY_TYPE(m), value),                                                                            \
        sizeof(MAP_VALUE_TYPE(m)),                                                                                     \
        offsetof(MAP_ENTRY_TYPE(m), hash),                                                                             \
        (n),                                                                                                           \
        (policy_value)                                                                                                 \
    )
///
/// Aborting variant of `MapRehashWithPolicy`. Calls `LOG_FATAL` on allocation
/// failure.
///
/// SUCCESS : Returns to the caller.
/// FAILURE : Does not return - aborts via `LOG_FATAL` / `SysAbort`.
///
/// TAGS: Map, Memory, Rehash, Policy, Must, Abort
///
#define MapMustRehashWithPolicy(m, n, policy_value)                                                                    \
    do {                                                                                                               \
        if (!MapRehashWithPolicy((m), (n), (policy_value))) {                                                          \
            LOG_FATAL("MapMustRehashWithPolicy failed");                                                               \
        }                                                                                                              \
    } while (0)

#endif // MISRA_STD_CONTAINER_MAP_MEMORY_H
