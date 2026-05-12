/// file      : std/container/map/memory.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Memory management helpers for Map.

#ifndef MISRA_STD_CONTAINER_MAP_MEMORY_H
#define MISRA_STD_CONTAINER_MAP_MEMORY_H

#include "Type.h"
#include "Private.h"

#include <stdio.h>

void SysAbort(void);

static inline void map_abort_memory_operation(const char *function, int line, const char *message) {
    fprintf(stderr, "FATAL [%s:%d] %s\n", function, line, message);
    SysAbort();
}

///
/// Clear all entries but retain allocated storage.
///
/// m[in,out] : Hash map.
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
#define MapMustReserve(m, n)                                                                                            \
    do {                                                                                                               \
        if (!MapReserve((m), (n))) {                                                                                   \
            map_abort_memory_operation(__func__, __LINE__, "MapMustReserve failed");                                   \
        }                                                                                                              \
    } while (0)

///
/// Rebuild the map using the current policy and current pair count.
/// This removes tombstones and re-packs the probe table.
///
/// m[in,out] : Map.
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
#define MapMustCompact(m)                                                                                                \
    do {                                                                                                               \
        if (!MapCompact((m))) {                                                                                        \
            map_abort_memory_operation(__func__, __LINE__, "MapMustCompact failed");                                   \
        }                                                                                                              \
    } while (0)

/// Remap using a specific probing policy.
///
/// m[in,out] : Hash map.
/// n[in]     : Minimum number of entries expected after rehash.
/// policy[in]: New probing policy copied into this map.
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
#define MapMustRehashWithPolicy(m, n, policy_value)                                                                     \
    do {                                                                                                               \
        if (!MapRehashWithPolicy((m), (n), (policy_value))) {                                                          \
            map_abort_memory_operation(__func__, __LINE__, "MapMustRehashWithPolicy failed");                          \
        }                                                                                                              \
    } while (0)

#endif // MISRA_STD_CONTAINER_MAP_MEMORY_H
