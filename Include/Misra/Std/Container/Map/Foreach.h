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
/// m[in,out]     : Map to iterate over.
/// key_ptr[in]   : Name of pointer variable bound to the key of each pair.
/// value_ptr[in] : Name of pointer variable bound to the value of each pair.
///
/// SUCCESS : The loop body runs once per occupied slot with `key_ptr`
///           bound to the in-slot key address and `value_ptr` bound to
///           the in-slot value address. The body is skipped when `m`
///           is empty. Use this form when the body needs to mutate the
///           value (or read the key) through the pointer. The map is
///           not modified by the macro itself.
/// FAILURE : The macro itself does not fail. `LOG_FATAL` via
///           `ValidateMap(m)` when `m` is uninitialised or corrupted.
///
/// TAGS: Map, Foreach, Pair, Pointer
///
#define MapForeachPairPtr(m, key_ptr, value_ptr)                                                                                   \
    for (TYPE_OF(m) UNPL(pm) = (m); UNPL(pm); UNPL(pm) = NULL)                                                                     \
        if ((ValidateMap(UNPL(pm)), 1) && UNPL(pm)->length > 0)                                                                    \
            for (size UNPL(slot) = 0; UNPL(slot) < UNPL(pm)->capacity; UNPL(slot)++)                                               \
                if (map_slot_occupied(UNPL(pm), UNPL(slot)))                                                                       \
                    for (bool UNPL(_once) = true; UNPL(_once); UNPL(_once) = false)                                                \
                        for (MAP_KEY_TYPE(UNPL(pm)) *key_ptr = map_key_ptr_at(UNPL(pm), UNPL(slot)); UNPL(_once);                  \
                             UNPL(_once)                     = false)                                                              \
                            for (MAP_VALUE_TYPE(UNPL(pm)) *value_ptr = map_value_ptr_at(UNPL(pm), UNPL(slot)); \
                                 value_ptr;                                                                    \
                                 value_ptr = NULL)

///
/// Iterate over all stored key/value pairs by value.
///
/// m[in,out]     : Map to iterate over.
/// key_var[in]   : Name of variable bound to a copy of each pair's key.
/// value_var[in] : Name of variable bound to a copy of each pair's value.
///
/// SUCCESS : The loop body runs once per occupied slot with `key_var`
///           and `value_var` bound to copies of the stored key and
///           value. The body is skipped when `m` is empty. Mutating
///           the locals does not write back into the map.
/// FAILURE : The macro itself does not fail. `LOG_FATAL` via
///           `ValidateMap(m)` when `m` is uninitialised or corrupted.
///
/// TAGS: Map, Foreach, Pair
///
#define MapForeachPair(m, key_var, value_var)                                                                                     \
    for (TYPE_OF(m) UNPL(pm) = (m); UNPL(pm); UNPL(pm) = NULL)                                                                    \
        if ((ValidateMap(UNPL(pm)), 1) && UNPL(pm)->length > 0)                                                                   \
            for (size UNPL(slot) = 0; UNPL(slot) < UNPL(pm)->capacity; UNPL(slot)++)                                              \
                if (map_slot_occupied(UNPL(pm), UNPL(slot)))                                                                      \
                    for (bool UNPL(_once) = true; UNPL(_once); UNPL(_once) = false)                                               \
                        for (MAP_KEY_TYPE(UNPL(pm)) key_var = *map_key_ptr_at(UNPL(pm), UNPL(slot)); UNPL(_once);                 \
                             UNPL(_once)                    = false)                                                              \
                            for (MAP_VALUE_TYPE(UNPL(pm)) value_var = *map_value_ptr_at(UNPL(pm), UNPL(slot)); \
                                 UNPL(_once);                                                                  \
                                 UNPL(_once) = false)

///
/// Iterate once per unique key stored in the multimap.
///
/// m[in,out]   : Map to iterate over.
/// key_var[in] : Name of variable bound to a copy of each unique key.
///
/// SUCCESS : The loop body runs once for each distinct key (duplicates
///           in the multimap are visited only at their canonical
///           probe-anchor slot) with `key_var` bound to a copy of that
///           key. The body is skipped when `m` is empty.
/// FAILURE : The macro itself does not fail. `LOG_FATAL` via
///           `ValidateMap(m)` when `m` is uninitialised or corrupted.
///
/// TAGS: Map, Foreach, Key
///
#define MapForeachKey(m, key_var)                                                                                      \
    for (TYPE_OF(m) UNPL(pm) = (m); UNPL(pm); UNPL(pm) = NULL)                                                         \
        if ((ValidateMap(UNPL(pm)), 1) && UNPL(pm)->length > 0)                                                        \
            for (size UNPL(slot) = 0; UNPL(slot) < UNPL(pm)->capacity; UNPL(slot)++)                                   \
                if (map_slot_occupied(UNPL(pm), UNPL(slot)) && (map_find_index(                                        \
                                                                    GENERIC_MAP(UNPL(pm)),                             \
                                                                    map_key_ptr_at(UNPL(pm), UNPL(slot)),              \
                                                                    sizeof(MAP_ENTRY_TYPE(UNPL(pm))),                  \
                                                                    offsetof(MAP_ENTRY_TYPE(UNPL(pm)), key),           \
                                                                    sizeof(MAP_KEY_TYPE(UNPL(pm))),                    \
                                                                    offsetof(MAP_ENTRY_TYPE(UNPL(pm)), hash)           \
                                                                ) == UNPL(slot)))                                      \
                    for (bool UNPL(_once) = true; UNPL(_once); UNPL(_once) = false)                                    \
                        for (MAP_KEY_TYPE(UNPL(pm)) key_var = *map_key_ptr_at(UNPL(pm), UNPL(slot)); UNPL(_once);      \
                             UNPL(_once)                    = false)

///
/// Iterate over all stored values by value.
///
/// m[in,out]     : Map to iterate over.
/// value_var[in] : Name of variable bound to a copy of each stored value.
///
/// SUCCESS : The loop body runs once per occupied slot with `value_var`
///           bound to a copy of the stored value. The body is skipped
///           when `m` is empty. Mutating the local does not write back
///           into the map.
/// FAILURE : The macro itself does not fail. `LOG_FATAL` via
///           `ValidateMap(m)` when `m` is uninitialised or corrupted.
///
/// TAGS: Map, Foreach, Value
///
#define MapForeachValue(m, value_var)                                                                                  \
    for (TYPE_OF(m) UNPL(pm) = (m); UNPL(pm); UNPL(pm) = NULL)                                                         \
        if ((ValidateMap(UNPL(pm)), 1) && UNPL(pm)->length > 0)                                                        \
            for (size UNPL(slot) = 0; UNPL(slot) < UNPL(pm)->capacity; UNPL(slot)++)                                   \
                if (map_slot_occupied(UNPL(pm), UNPL(slot)))                                                           \
                    for (bool UNPL(_once) = true; UNPL(_once); UNPL(_once) = false)                                    \
                        for (MAP_VALUE_TYPE(UNPL(pm)) value_var = *map_value_ptr_at(UNPL(pm), UNPL(slot));             \
                             UNPL(_once);                                                                              \
                             UNPL(_once) = false)

///
/// Iterate over all stored values with pointers.
///
/// m[in,out]     : Map to iterate over.
/// value_ptr[in] : Name of pointer variable bound to each stored value.
///
/// SUCCESS : The loop body runs once per occupied slot with `value_ptr`
///           bound to the in-slot value address. Use this form when
///           the body mutates the stored value in place. The body is
///           skipped when `m` is empty.
/// FAILURE : The macro itself does not fail. `LOG_FATAL` via
///           `ValidateMap(m)` when `m` is uninitialised or corrupted.
///
/// TAGS: Map, Foreach, Value, Pointer
///
#define MapForeachValuePtr(m, value_ptr)                                                                               \
    for (TYPE_OF(m) UNPL(pm) = (m); UNPL(pm); UNPL(pm) = NULL)                                                         \
        if ((ValidateMap(UNPL(pm)), 1) && UNPL(pm)->length > 0)                                                        \
            for (size UNPL(slot) = 0; UNPL(slot) < UNPL(pm)->capacity; UNPL(slot)++)                                   \
                if (map_slot_occupied(UNPL(pm), UNPL(slot)))                                                           \
                    for (bool UNPL(_once) = true; UNPL(_once); UNPL(_once) = false)                                    \
                        for (MAP_VALUE_TYPE(UNPL(pm)) *value_ptr = map_value_ptr_at(UNPL(pm), UNPL(slot));             \
                             UNPL(_once);                                                                              \
                             UNPL(_once) = false)

///
/// Iterate over all values stored for a specific key.
///
/// m[in,out]      : Map to iterate over.
/// lookup_key[in] : Key value to match against (compared via the map's
///                  `key_compare` callback).
/// value_var[in]  : Name of variable bound to a copy of each matching
///                  stored value.
///
/// SUCCESS : The loop body runs once for each value stored under
///           `lookup_key`, with `value_var` bound to a copy of that
///           value. The body is skipped when `m` is empty or when the
///           key is not present.
/// FAILURE : The macro itself does not fail. `LOG_FATAL` via
///           `ValidateMap(m)` when `m` is uninitialised or corrupted.
///
/// TAGS: Map, Foreach, Value, Lookup
///
#define MapForeachValueForKey(m, lookup_key, value_var)                                                                \
    for (TYPE_OF(m) UNPL(pm) = (m); UNPL(pm); UNPL(pm) = NULL)                                                         \
        if ((ValidateMap(UNPL(pm)), 1) && UNPL(pm)->length > 0)                                                        \
            for (bool UNPL(_key_once) = true; UNPL(_key_once); UNPL(_key_once) = false)                                \
                for (MAP_KEY_TYPE(UNPL(pm)) UNPL(find_key) = (lookup_key); UNPL(_key_once); UNPL(_key_once) = false)   \
                    for (size UNPL(slot) = 0; UNPL(slot) < UNPL(pm)->capacity; UNPL(slot)++)                           \
                        if (map_slot_occupied(UNPL(pm), UNPL(slot)) &&                                                 \
                            (UNPL(pm)->key_compare(map_key_ptr_at(UNPL(pm), UNPL(slot)), &UNPL(find_key)) == 0))       \
                            for (bool UNPL(_once) = true; UNPL(_once); UNPL(_once) = false)                            \
                                for (MAP_VALUE_TYPE(UNPL(pm)) value_var = *map_value_ptr_at(UNPL(pm), UNPL(slot));     \
                                     UNPL(_once);                                                                      \
                                     UNPL(_once) = false)

///
/// Iterate over all values stored for a specific key with pointers.
///
/// m[in,out]      : Map to iterate over.
/// lookup_key[in] : Key value to match against (compared via the map's
///                  `key_compare` callback).
/// value_ptr[in]  : Name of pointer variable bound to each matching
///                  in-slot value.
///
/// SUCCESS : The loop body runs once for each value stored under
///           `lookup_key`, with `value_ptr` bound to the in-slot value
///           address. Use this form when the body mutates the stored
///           value in place. The body is skipped when `m` is empty or
///           when the key is not present.
/// FAILURE : The macro itself does not fail. `LOG_FATAL` via
///           `ValidateMap(m)` when `m` is uninitialised or corrupted.
///
/// TAGS: Map, Foreach, Value, Lookup, Pointer
///
#define MapForeachValuePtrForKey(m, lookup_key, value_ptr)                                                             \
    for (TYPE_OF(m) UNPL(pm) = (m); UNPL(pm); UNPL(pm) = NULL)                                                         \
        if ((ValidateMap(UNPL(pm)), 1) && UNPL(pm)->length > 0)                                                        \
            for (bool UNPL(_key_once) = true; UNPL(_key_once); UNPL(_key_once) = false)                                \
                for (MAP_KEY_TYPE(UNPL(pm)) UNPL(find_key) = (lookup_key); UNPL(_key_once); UNPL(_key_once) = false)   \
                    for (size UNPL(slot) = 0; UNPL(slot) < UNPL(pm)->capacity; UNPL(slot)++)                           \
                        if (map_slot_occupied(UNPL(pm), UNPL(slot)) &&                                                 \
                            (UNPL(pm)->key_compare(map_key_ptr_at(UNPL(pm), UNPL(slot)), &UNPL(find_key)) == 0))       \
                            for (bool UNPL(_once) = true; UNPL(_once); UNPL(_once) = false)                            \
                                for (MAP_VALUE_TYPE(UNPL(pm)) *value_ptr = map_value_ptr_at(UNPL(pm), UNPL(slot));     \
                                     UNPL(_once);                                                                      \
                                     UNPL(_once) = false)

#endif // MISRA_STD_CONTAINER_MAP_FOREACH_H
