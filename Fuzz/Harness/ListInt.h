/// file      : Fuzz/Harness/ListInt.h
/// author    : Generated for MisraStdC fuzzing
/// This is free and unencumbered software released into the public domain.
///
/// List(i32) specific fuzzing harness

#ifndef FUZZ_LIST_INT_H
#define FUZZ_LIST_INT_H

#include <Misra/Std/Container/List.h>
#include <Misra/Std/Allocator/Default.h>
#include <Misra/Types.h>

// List(i32) typedef
typedef List(i32) IntList;

// List(i32) function enumeration
typedef enum {
    LIST_INT_PUSH_BACK = 0,
    LIST_INT_PUSH_FRONT,
    LIST_INT_POP_BACK,
    LIST_INT_POP_FRONT,
    LIST_INT_INSERT,
    LIST_INT_REMOVE,
    LIST_INT_DELETE,
    LIST_INT_AT,
    LIST_INT_LEN,
    LIST_INT_FIRST,
    LIST_INT_LAST,

    // Memory operations
    LIST_INT_CLEAR,

    // Advanced operations
    LIST_INT_REVERSE,
    LIST_INT_SWAP_ITEMS,

    // Range operations
    LIST_INT_REMOVE_RANGE,
    LIST_INT_DELETE_RANGE,

    // Array operations
    LIST_INT_PUSH_ARR,

    // Advanced functions
    LIST_INT_SORT,
    LIST_INT_PTR_AT,
    LIST_INT_MERGE,

    // Foreach operations
    LIST_INT_FOREACH,
    LIST_INT_FOREACH_IDX,
    LIST_INT_FOREACH_PTR,
    LIST_INT_FOREACH_PTR_IDX,
    LIST_INT_FOREACH_REVERSE,
    LIST_INT_FOREACH_REVERSE_IDX,
    LIST_INT_FOREACH_PTR_REVERSE,
    LIST_INT_FOREACH_PTR_REVERSE_IDX,
    LIST_INT_FOREACH_IN_RANGE,
    LIST_INT_FOREACH_PTR_IN_RANGE,
    LIST_INT_FOREACH_REVERSE_IN_RANGE,
    LIST_INT_FOREACH_PTR_REVERSE_IN_RANGE,

    LIST_INT_COUNT
} ListIntFunction;

// Function prototypes
void init_int_list(IntList *list, DefaultAllocator *alloc);
void deinit_int_list(IntList *list);
void fuzz_int_list(
    IntList          *list,
    ListIntFunction   func,
    const uint8_t    *data,
    size_t           *offset,
    size_t            size,
    DefaultAllocator *alloc
);

#endif // FUZZ_LIST_INT_H
