/// file      : std/container/map/foreach.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Iteration helpers for Map.

#ifndef MISRA_STD_CONTAINER_MAP_FOREACH_H
#define MISRA_STD_CONTAINER_MAP_FOREACH_H

#include "Access.h"

///
/// Iterate over all stored key/value pairs with pointers.
///
/// TAGS: Map, Foreach, Pair, Pointer
///
#define MapForeachPairPtr(m, key_ptr, value_ptr)                                                                       \
    for (TYPE_OF(m) UNPL(pm) = (m); UNPL(pm); UNPL(pm) = NULL)                                                         \
        if ((ValidateMap(UNPL(pm)), 1) && UNPL(pm)->length > 0)                                                        \
            for (size UNPL(slot) = 0; UNPL(slot) < UNPL(pm)->capacity; UNPL(slot)++)                                   \
                if (map_slot_occupied(UNPL(pm), UNPL(slot)))                                                  \
                    for (bool UNPL(_once) = true; UNPL(_once); UNPL(_once) = false)                                    \
                        for (MAP_KEY_TYPE(UNPL(pm)) *key_ptr = map_key_ptr_at(UNPL(pm), UNPL(slot));           \
                             UNPL(_once);                                                                              \
                             UNPL(_once) = false)                                                                      \
                            for (MAP_VALUE_TYPE(UNPL(pm)) *value_ptr = map_value_ptr_at(UNPL(pm), UNPL(slot)); \
                                 value_ptr;                                                                            \
                                 value_ptr = NULL)

///
/// Iterate over all stored key/value pairs by value.
///
/// TAGS: Map, Foreach, Pair
///
#define MapForeachPair(m, key_var, value_var)                                                                          \
    for (TYPE_OF(m) UNPL(pm) = (m); UNPL(pm); UNPL(pm) = NULL)                                                         \
        if ((ValidateMap(UNPL(pm)), 1) && UNPL(pm)->length > 0)                                                        \
            for (size UNPL(slot) = 0; UNPL(slot) < UNPL(pm)->capacity; UNPL(slot)++)                                   \
                if (map_slot_occupied(UNPL(pm), UNPL(slot)))                                                  \
                    for (bool UNPL(_once) = true; UNPL(_once); UNPL(_once) = false)                                    \
                        for (MAP_KEY_TYPE(UNPL(pm)) key_var = *map_key_ptr_at(UNPL(pm), UNPL(slot));           \
                             UNPL(_once);                                                                              \
                             UNPL(_once) = false)                                                                      \
                            for (MAP_VALUE_TYPE(UNPL(pm)) value_var = *map_value_ptr_at(UNPL(pm), UNPL(slot)); \
                                 UNPL(_once);                                                                          \
                                 UNPL(_once) = false)

///
/// Iterate once per unique key stored in the multimap.
///
/// TAGS: Map, Foreach, Key
///
#define MapForeachKey(m, key_var)                                                                                      \
    for (TYPE_OF(m) UNPL(pm) = (m); UNPL(pm); UNPL(pm) = NULL)                                                         \
        if ((ValidateMap(UNPL(pm)), 1) && UNPL(pm)->length > 0)                                                        \
            for (size UNPL(slot) = 0; UNPL(slot) < UNPL(pm)->capacity; UNPL(slot)++)                                   \
                if (map_slot_occupied(UNPL(pm), UNPL(slot)) &&                                                \
                    (map_find_index(                                                                                   \
                         GENERIC_MAP(UNPL(pm)),                                                                        \
                         map_key_ptr_at(UNPL(pm), UNPL(slot)),                                                 \
                         sizeof(MAP_ENTRY_TYPE(UNPL(pm))),                                                             \
                         offsetof(MAP_ENTRY_TYPE(UNPL(pm)), key),                                                      \
                         sizeof(MAP_KEY_TYPE(UNPL(pm))),                                                               \
                         offsetof(MAP_ENTRY_TYPE(UNPL(pm)), hash)                                                      \
                     ) == UNPL(slot)))                                                                                 \
                    for (bool UNPL(_once) = true; UNPL(_once); UNPL(_once) = false)                                    \
                        for (MAP_KEY_TYPE(UNPL(pm)) key_var = *map_key_ptr_at(UNPL(pm), UNPL(slot));           \
                             UNPL(_once);                                                                              \
                             UNPL(_once) = false)

///
/// Iterate over all stored values by value.
///
/// TAGS: Map, Foreach, Value
///
#define MapForeachValue(m, value_var)                                                                                  \
    for (TYPE_OF(m) UNPL(pm) = (m); UNPL(pm); UNPL(pm) = NULL)                                                         \
        if ((ValidateMap(UNPL(pm)), 1) && UNPL(pm)->length > 0)                                                        \
            for (size UNPL(slot) = 0; UNPL(slot) < UNPL(pm)->capacity; UNPL(slot)++)                                   \
                if (map_slot_occupied(UNPL(pm), UNPL(slot)))                                                  \
                    for (bool UNPL(_once) = true; UNPL(_once); UNPL(_once) = false)                                    \
                        for (MAP_VALUE_TYPE(UNPL(pm)) value_var = *map_value_ptr_at(UNPL(pm), UNPL(slot));     \
                             UNPL(_once);                                                                              \
                             UNPL(_once) = false)

///
/// Iterate over all stored values with pointers.
///
/// TAGS: Map, Foreach, Value, Pointer
///
#define MapForeachValuePtr(m, value_ptr)                                                                               \
    for (TYPE_OF(m) UNPL(pm) = (m); UNPL(pm); UNPL(pm) = NULL)                                                         \
        if ((ValidateMap(UNPL(pm)), 1) && UNPL(pm)->length > 0)                                                        \
            for (size UNPL(slot) = 0; UNPL(slot) < UNPL(pm)->capacity; UNPL(slot)++)                                   \
                if (map_slot_occupied(UNPL(pm), UNPL(slot)))                                                  \
                    for (bool UNPL(_once) = true; UNPL(_once); UNPL(_once) = false)                                    \
                        for (MAP_VALUE_TYPE(UNPL(pm)) *value_ptr = map_value_ptr_at(UNPL(pm), UNPL(slot));     \
                             UNPL(_once);                                                                              \
                             UNPL(_once) = false)

///
/// Iterate over all values stored for a specific key.
///
/// TAGS: Map, Foreach, Value, Lookup
///
#define MapForeachValueForKey(m, lookup_key, value_var)                                                                \
    for (TYPE_OF(m) UNPL(pm) = (m); UNPL(pm); UNPL(pm) = NULL)                                                         \
        if ((ValidateMap(UNPL(pm)), 1) && UNPL(pm)->length > 0)                                                        \
            for (bool UNPL(_key_once) = true; UNPL(_key_once); UNPL(_key_once) = false)                                \
                for (MAP_KEY_TYPE(UNPL(pm)) UNPL(find_key) = (lookup_key); UNPL(_key_once); UNPL(_key_once) = false)   \
                    for (size UNPL(slot) = 0; UNPL(slot) < UNPL(pm)->capacity; UNPL(slot)++)                           \
                        if (map_slot_occupied(UNPL(pm), UNPL(slot)) &&                                        \
                            (UNPL(pm)->key_compare(map_key_ptr_at(UNPL(pm), UNPL(slot)), &UNPL(find_key)) ==   \
                             0))                                                                                       \
                            for (bool UNPL(_once) = true; UNPL(_once); UNPL(_once) = false)                            \
                                for (MAP_VALUE_TYPE(UNPL(pm))                                                          \
                                         value_var = *map_value_ptr_at(UNPL(pm), UNPL(slot));                  \
                                     UNPL(_once);                                                                      \
                                     UNPL(_once) = false)

///
/// Iterate over all values stored for a specific key with pointers.
///
/// TAGS: Map, Foreach, Value, Lookup, Pointer
///
#define MapForeachValuePtrForKey(m, lookup_key, value_ptr)                                                             \
    for (TYPE_OF(m) UNPL(pm) = (m); UNPL(pm); UNPL(pm) = NULL)                                                         \
        if ((ValidateMap(UNPL(pm)), 1) && UNPL(pm)->length > 0)                                                        \
            for (bool UNPL(_key_once) = true; UNPL(_key_once); UNPL(_key_once) = false)                                \
                for (MAP_KEY_TYPE(UNPL(pm)) UNPL(find_key) = (lookup_key); UNPL(_key_once); UNPL(_key_once) = false)   \
                    for (size UNPL(slot) = 0; UNPL(slot) < UNPL(pm)->capacity; UNPL(slot)++)                           \
                        if (map_slot_occupied(UNPL(pm), UNPL(slot)) &&                                        \
                            (UNPL(pm)->key_compare(map_key_ptr_at(UNPL(pm), UNPL(slot)), &UNPL(find_key)) ==   \
                             0))                                                                                       \
                            for (bool UNPL(_once) = true; UNPL(_once); UNPL(_once) = false)                            \
                                for (MAP_VALUE_TYPE(UNPL(pm)) *value_ptr =                                             \
                                         map_value_ptr_at(UNPL(pm), UNPL(slot));                               \
                                     UNPL(_once);                                                                      \
                                     UNPL(_once) = false)

#endif // MISRA_STD_CONTAINER_MAP_FOREACH_H
