/// file      : std/container/map/private.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Internal runtime helpers for Map.

#ifndef MISRA_STD_CONTAINER_MAP_PRIVATE_H
#define MISRA_STD_CONTAINER_MAP_PRIVATE_H

#include "Type.h"

void   validate_map(const GenericMap *map);
void   deinit_map(
      GenericMap *map,
      size            entry_size,
      size            key_offset,
      size            key_size,
      size            value_offset,
      size            value_size,
      size            hash_offset
);
void   clear_map(
      GenericMap *map,
      size            entry_size,
      size            key_offset,
      size            key_size,
      size            value_offset,
      size            value_size,
      size            hash_offset
);
void   reserve_map(
      GenericMap *map,
      size            entry_size,
      size            key_offset,
      size            key_size,
      size            value_offset,
      size            value_size,
      size            hash_offset,
      size            n
);
void   rehash_map(
      GenericMap *map,
      size            entry_size,
      size            key_offset,
      size            key_size,
      size            value_offset,
      size            value_size,
      size            hash_offset,
      size            n,
      MapPolicy   policy
);
bool   map_contains(
      GenericMap *map,
      const void     *key,
      size            entry_size,
      size            key_offset,
      size            key_size,
      size            hash_offset
);
void  *map_get_value_ptr(
      GenericMap *map,
      const void     *key,
      size            entry_size,
      size            key_offset,
      size            key_size,
      size            value_offset,
      size            hash_offset
);
size   map_find_index(
      GenericMap *map,
      const void     *key,
      size            entry_size,
      size            key_offset,
      size            key_size,
      size            hash_offset
);
void   map_insert(
      GenericMap *map,
      const void     *key,
      const void     *value,
      size            entry_size,
      size            key_offset,
      size            key_size,
      size            value_offset,
      size            value_size,
      size            hash_offset,
      bool            replace_existing
);
bool   map_remove(
      GenericMap *map,
      const void     *key,
      void           *removed_key,
      void           *removed_value,
      size            entry_size,
      size            key_offset,
      size            key_size,
      size            value_offset,
      size            value_size,
      size            hash_offset
);

#endif // MISRA_STD_CONTAINER_MAP_PRIVATE_H
