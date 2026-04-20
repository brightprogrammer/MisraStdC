/// file      : std/container/map/foreach.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Iteration helpers for Map.

#ifndef MISRA_STD_CONTAINER_MAP_FOREACH_H
#define MISRA_STD_CONTAINER_MAP_FOREACH_H

#include "Access.h"

///
/// Iterate over occupied key/value pairs with pointers and explicit slot index.
///
/// TAGS: Map, Foreach, Pointer, Index
///
#define MapForeachPtrIdx(m, key_ptr, value_ptr, idx)                                                              \
    for (TYPE_OF(m) UNPL(pm) = (m); UNPL(pm); UNPL(pm) = NULL)                                                        \
        if ((ValidateMap(UNPL(pm)), 1) && UNPL(pm)->length > 0)                                                   \
            for (size idx = 0; idx < UNPL(pm)->capacity; idx++)                                                       \
                if (MapSlotOccupied(UNPL(pm), idx))                                                                \
                    for (bool UNPL(_once) = true; UNPL(_once); UNPL(_once) = false)                                   \
                        for (MAP_KEY_TYPE(UNPL(pm)) *key_ptr = MapKeyPtrAt(UNPL(pm), idx);                    \
                             UNPL(_once);                                                                              \
                             UNPL(_once) = false)                                                                      \
                            for (MAP_VALUE_TYPE(UNPL(pm)) *value_ptr = MapValuePtrAt(UNPL(pm), idx);          \
                                 value_ptr;                                                                            \
                                 value_ptr = NULL)

///
/// Iterate over occupied key/value pairs with pointers.
///
/// TAGS: Map, Foreach, Pointer
///
#define MapForeachPtr(m, key_ptr, value_ptr) MapForeachPtrIdx((m), (key_ptr), (value_ptr), UNPL(iter))

///
/// Iterate over occupied key/value pairs by value with explicit slot index.
///
/// TAGS: Map, Foreach, Index
///
#define MapForeachIdx(m, key_var, value_var, idx)                                                                 \
    MapForeachIdx_IMPL((m), (key_var), (value_var), idx, UNPL(map_once))

#define MapForeachIdx_IMPL(m, key_var, value_var, idx, once_var)                                                  \
    for (TYPE_OF(m) UNPL(pm) = (m); UNPL(pm); UNPL(pm) = NULL)                                                        \
        if ((ValidateMap(UNPL(pm)), 1) && UNPL(pm)->length > 0)                                                   \
            for (size idx = 0; idx < UNPL(pm)->capacity; idx++)                                                       \
                if (MapSlotOccupied(UNPL(pm), idx))                                                                \
                    for (bool once_var = true; once_var; once_var = false)                                             \
                        for (MAP_KEY_TYPE(UNPL(pm)) key_var = *MapKeyPtrAt(UNPL(pm), idx);                    \
                             once_var;                                                                                 \
                             once_var = false)                                                                         \
                            for (MAP_VALUE_TYPE(UNPL(pm)) value_var = *MapValuePtrAt(UNPL(pm), idx);          \
                                 once_var;                                                                             \
                                 once_var = false)

///
/// Iterate over occupied key/value pairs by value.
///
/// TAGS: Map, Foreach
///
#define MapForeach(m, key_var, value_var) MapForeachIdx((m), (key_var), (value_var), UNPL(iter))

#endif // MISRA_STD_CONTAINER_MAP_FOREACH_H
