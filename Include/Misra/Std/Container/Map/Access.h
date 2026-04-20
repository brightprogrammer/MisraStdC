/// file      : std/container/map/access.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Access helpers for Map.

#ifndef MISRA_STD_CONTAINER_MAP_ACCESS_H
#define MISRA_STD_CONTAINER_MAP_ACCESS_H

#include "Type.h"
#include "Private.h"

#define MISRA_MAP_SLOT_EMPTY     0u
#define MISRA_MAP_SLOT_OCCUPIED  1u
#define MISRA_MAP_SLOT_TOMBSTONE 2u

///
/// Number of occupied entries in the map.
///
/// m[in] : Hash map.
///
#define MapLen(m) ((m)->length)

///
/// Number of allocated probe slots in the map.
///
/// m[in] : Hash map.
///
#define MapCapacity(m) ((m)->capacity)

///
/// Active probing policy of the map.
///
/// m[in] : Hash map.
///
#define MapPolicyGet(m) ((m)->policy)

///
/// Name of the active probing policy.
///
/// m[in] : Hash map.
///
#define MapPolicyName(m) ((m)->policy.name)

///
/// Check whether a slot is occupied.
///
/// m[in]   : Hash map.
/// idx[in] : Probe slot index.
///
#define MapSlotOccupied(m, idx) ((m)->states && (m)->states[(idx)] == MISRA_MAP_SLOT_OCCUPIED)

///
/// Pointer to entry at given slot.
/// Slot must be occupied.
///
/// m[in]   : Hash map.
/// idx[in] : Probe slot index.
///
#define MapEntryPtrAt(m, idx) (&((m)->entries[(idx)]))

///
/// Pointer to key at given slot.
/// Slot must be occupied.
///
/// m[in]   : Hash map.
/// idx[in] : Probe slot index.
///
#define MapKeyPtrAt(m, idx) (&((m)->entries[(idx)].key))

///
/// Pointer to value at given slot.
/// Slot must be occupied.
///
/// m[in]   : Hash map.
/// idx[in] : Probe slot index.
///
#define MapValuePtrAt(m, idx) (&((m)->entries[(idx)].value))

///
/// Cached key hash at given slot.
///
/// m[in]   : Hash map.
/// idx[in] : Probe slot index.
///
#define MapHashAt(m, idx) ((m)->entries[(idx)].hash)

///
/// Returns current load factor of the map.
///
/// m[in] : Hash map.
///
#define MapLoadFactor(m) ((m)->capacity ? ((double)(m)->length / (double)(m)->capacity) : 0.0)

///
/// Check if a key exists in the map.
///
/// m[in]   : Hash map.
/// key[in] : Key to search for.
///
/// SUCCESS : `true` when key exists.
/// FAILURE : `false`
///
#define MapContains(m, lookup_key)                                                                                  \
    map_contains(                                                                                                   \
        GENERIC_MAP(m),                                                                                             \
        &((MAP_KEY_TYPE(m)) {(lookup_key)}),                                                                        \
        sizeof(MAP_ENTRY_TYPE(m)),                                                                                  \
        offsetof(MAP_ENTRY_TYPE(m), key),                                                                           \
        sizeof(MAP_KEY_TYPE(m)),                                                                                    \
        offsetof(MAP_ENTRY_TYPE(m), hash)                                                                           \
    )

///
/// Find probe-slot index of a key.
///
/// m[in]   : Hash map.
/// key[in] : Key to search for.
///
/// SUCCESS : Slot index if key exists.
/// FAILURE : `MapCapacity(m)` if key does not exist.
///
#define MapFindIndex(m, lookup_key)                                                                                \
    map_find_index(                                                                                                \
        GENERIC_MAP(m),                                                                                            \
        &((MAP_KEY_TYPE(m)) {(lookup_key)}),                                                                       \
        sizeof(MAP_ENTRY_TYPE(m)),                                                                                 \
        offsetof(MAP_ENTRY_TYPE(m), key),                                                                          \
        sizeof(MAP_KEY_TYPE(m)),                                                                                   \
        offsetof(MAP_ENTRY_TYPE(m), hash)                                                                          \
    )

///
/// Get pointer to value for given key.
///
/// m[in,out] : Hash map.
/// key[in]   : Key to search for.
///
/// SUCCESS : Pointer to value stored in map.
/// FAILURE : `NULL`
///
#define MapGetPtr(m, lookup_key)                                                                                   \
    ((MAP_VALUE_TYPE(m) *)map_get_value_ptr(                                                                   \
        GENERIC_MAP(m),                                                                                            \
        &((MAP_KEY_TYPE(m)) {(lookup_key)}),                                                                       \
        sizeof(MAP_ENTRY_TYPE(m)),                                                                                 \
        offsetof(MAP_ENTRY_TYPE(m), key),                                                                          \
        sizeof(MAP_KEY_TYPE(m)),                                                                                   \
        offsetof(MAP_ENTRY_TYPE(m), value),                                                                        \
        offsetof(MAP_ENTRY_TYPE(m), hash)                                                                          \
    ))

#endif // MISRA_STD_CONTAINER_MAP_ACCESS_H
