/// file      : std/container/map/foreach.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Iteration helpers for Map.

#ifndef MISRA_STD_CONTAINER_MAP_FOREACH_H
#define MISRA_STD_CONTAINER_MAP_FOREACH_H

#include "Access.h"

///
/// Iterate over occupied key/value pairs with pointers.
///
/// TAGS: Map, Foreach, Pointer
///
#define MapForeachPtr(m, key_ptr, value_ptr)                                                                                               \
    for (TYPE_OF(m) UNPL(pm) = (m); UNPL(pm); UNPL(pm) = NULL)                                                                             \
        if ((ValidateMap(UNPL(pm)), 1) && UNPL(pm)->length > 0)                                                                            \
            for (size UNPL(slot) = 0; UNPL(slot) < UNPL(pm)->capacity; UNPL(slot)++)                                                       \
                if (MapSlotOccupied(UNPL(pm), UNPL(slot)))                                                                                 \
                    for (bool UNPL(_once) = true; UNPL(_once); UNPL(_once) = false)                                                        \
                        for (MAP_KEY_TYPE(UNPL(pm)) *key_ptr = MapKeyPtrAt(UNPL(pm), UNPL(slot)); UNPL(_once);                             \
                             UNPL(_once)                     = false)                                                                      \
                            for (MAP_VALUE_TYPE(UNPL(pm)) *value_ptr = MapValuePtrAt(UNPL(pm), UNPL(slot)); value_ptr; \
                                 value_ptr                           = NULL)

///
/// Iterate over occupied key/value pairs by value.
///
/// TAGS: Map, Foreach
///
#define MapForeach(m, key_var, value_var)                                                                                      \
    for (TYPE_OF(m) UNPL(pm) = (m); UNPL(pm); UNPL(pm) = NULL)                                                                 \
        if ((ValidateMap(UNPL(pm)), 1) && UNPL(pm)->length > 0)                                                                \
            for (size UNPL(slot) = 0; UNPL(slot) < UNPL(pm)->capacity; UNPL(slot)++)                                           \
                if (MapSlotOccupied(UNPL(pm), UNPL(slot)))                                                                     \
                    for (bool UNPL(_once) = true; UNPL(_once); UNPL(_once) = false)                                            \
                        for (MAP_KEY_TYPE(UNPL(pm)) key_var = *MapKeyPtrAt(UNPL(pm), UNPL(slot)); UNPL(_once);                 \
                             UNPL(_once)                    = false)                                                           \
                            for (MAP_VALUE_TYPE(UNPL(pm)) value_var = *MapValuePtrAt(UNPL(pm), UNPL(slot)); \
                                 UNPL(_once);                                                               \
                                 UNPL(_once) = false)

#endif // MISRA_STD_CONTAINER_MAP_FOREACH_H
