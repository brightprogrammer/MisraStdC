/// file      : std/container/map/private.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Internal runtime helpers for Map.

#ifndef MISRA_STD_CONTAINER_MAP_PRIVATE_H
#define MISRA_STD_CONTAINER_MAP_PRIVATE_H

#include "Type.h"

#define MISRA_PRIV_MAP_SLOT_EMPTY     0u
#define MISRA_PRIV_MAP_SLOT_OCCUPIED  1u
#define MISRA_PRIV_MAP_SLOT_TOMBSTONE 2u

#define MISRA_PRIV_MapSlotOccupied(m, idx) ((m)->states && (m)->states[(idx)] == MISRA_PRIV_MAP_SLOT_OCCUPIED)
#define MISRA_PRIV_MapEntryPtrAt(m, idx)   (&((m)->entries[(idx)]))
#define MISRA_PRIV_MapKeyPtrAt(m, idx)     (&((m)->entries[(idx)].key))
#define MISRA_PRIV_MapValuePtrAt(m, idx)   (&((m)->entries[(idx)].value))
#define MISRA_PRIV_MapHashAt(m, idx)       ((m)->entries[(idx)].hash)

void      validate_map_policy(const MapPolicy *policy);
MapPolicy validate_map_policy_copy(MapPolicy policy);
void      validate_map(const GenericMap *map);
void      deinit_map(
         GenericMap *map,
         size        entry_size,
         size        key_offset,
         size        key_size,
         size        value_offset,
         size        value_size,
         size        hash_offset
     );
void clear_map(
    GenericMap *map,
    size        entry_size,
    size        key_offset,
    size        key_size,
    size        value_offset,
    size        value_size,
    size        hash_offset
);
void reserve_map(
    GenericMap *map,
    size        entry_size,
    size        key_offset,
    size        key_size,
    size        value_offset,
    size        value_size,
    size        hash_offset,
    size        n
);
void rehash_map(
    GenericMap *map,
    size        entry_size,
    size        key_offset,
    size        key_size,
    size        value_offset,
    size        value_size,
    size        hash_offset,
    size        n,
    MapPolicy   policy
);
bool map_contains(GenericMap *map, const void *key, size entry_size, size key_offset, size key_size, size hash_offset);
bool map_contains_pair(
    GenericMap *map,
    const void *key,
    const void *value,
    size        entry_size,
    size        key_offset,
    size        key_size,
    size        value_offset,
    size        hash_offset
);
size map_unique_key_count(GenericMap *map, size entry_size, size key_offset, size key_size, size hash_offset);
size map_value_count(
    GenericMap *map,
    const void *key,
    size        entry_size,
    size        key_offset,
    size        key_size,
    size        hash_offset
);
void *map_get_value_ptr(
    GenericMap *map,
    const void *key,
    size        entry_size,
    size        key_offset,
    size        key_size,
    size        value_offset,
    size        hash_offset
);
void *map_ensure_value_ptr(
    GenericMap *map,
    const void *key,
    const void *value,
    size        entry_size,
    size        key_offset,
    size        key_size,
    size        value_offset,
    size        value_size,
    size        hash_offset
);
MapValueCursor map_find_first_cursor(
    GenericMap *map,
    const void *key,
    size        entry_size,
    size        key_offset,
    size        key_size,
    size        hash_offset
);
MapValueCursor map_find_next_cursor(
    GenericMap    *map,
    const void    *key,
    MapValueCursor cursor,
    size           entry_size,
    size           key_offset,
    size           key_size,
    size           hash_offset
);
void *map_value_ptr_from_cursor(GenericMap *map, MapValueCursor cursor, size entry_size, size value_offset);
size  map_find_index(
     GenericMap *map,
     const void *key,
     size        entry_size,
     size        key_offset,
     size        key_size,
     size        hash_offset
 );
size map_find_next_index(
    GenericMap *map,
    const void *key,
    size        previous_index,
    size        entry_size,
    size        key_offset,
    size        key_size,
    size        hash_offset
);
void map_insert(
    GenericMap *map,
    const void *key,
    const void *value,
    size        entry_size,
    size        key_offset,
    size        key_size,
    size        value_offset,
    size        value_size,
    size        hash_offset
);
bool map_remove(
    GenericMap *map,
    const void *key,
    size        entry_size,
    size        key_offset,
    size        key_size,
    size        value_offset,
    size        value_size,
    size        hash_offset
);
bool map_remove_pair(
    GenericMap *map,
    const void *key,
    const void *value,
    size        entry_size,
    size        key_offset,
    size        key_size,
    size        value_offset,
    size        value_size,
    size        hash_offset
);
size map_remove_all(
    GenericMap *map,
    const void *key,
    size        entry_size,
    size        key_offset,
    size        key_size,
    size        value_offset,
    size        value_size,
    size        hash_offset
);
size map_remove_if(
    GenericMap    *map,
    MapPredicateFn predicate,
    void          *ctx,
    size           entry_size,
    size           key_offset,
    size           key_size,
    size           value_offset,
    size           value_size
);
size map_retain_if(
    GenericMap    *map,
    MapPredicateFn predicate,
    void          *ctx,
    size           entry_size,
    size           key_offset,
    size           key_size,
    size           value_offset,
    size           value_size
);

#endif // MISRA_STD_CONTAINER_MAP_PRIVATE_H
