/// file      : fuzz/harness/str.h
/// author    : Generated for MisraStdC fuzzing
/// This is free and unencumbered software released into the public domain.
///
/// Str specific fuzzing harness header

#ifndef FUZZ_STR_H
#define FUZZ_STR_H

#include <Misra/Std/Container/Str.h>
#include <Misra/Std/Allocator/Default.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h> // For size_t

// Function enumeration for Str operations
typedef enum {
    STR_INIT = 0,
    STR_INIT_FROM_CSTR,
    STR_INIT_FROM_ZSTR,
    STR_INIT_FROM_STR,
    STR_DUP,
    STR_DEINIT,

    // Access operations
    STR_FIRST,
    STR_LAST,
    STR_BEGIN,
    STR_END,
    STR_CHAR_AT,
    STR_CHAR_PTR_AT,

    // Memory operations
    STR_CLEAR,
    STR_RESIZE,
    STR_RESERVE,
    STR_TRY_REDUCE_SPACE,
    STR_LEN,
    STR_SIZE,

    // String operations
    STR_CMP,
    STR_CMP_CSTR,
    STR_CMP_ZSTR,
    STR_FIND_STR,
    STR_FIND_ZSTR,
    STR_FIND_CSTR,

    // Insert operations
    STR_INSERT,
    STR_INSERT_CSTR,
    STR_INSERT_ZSTR,
    STR_INSERT_STR,
    STR_PUSH_BACK,
    STR_PUSH_FRONT,
    STR_APPEND,
    STR_APPEND_CSTR,
    STR_APPEND_ZSTR,
    STR_APPEND_STR,

    // Remove operations
    STR_POP_BACK,
    STR_POP_FRONT,
    STR_REMOVE,
    STR_REMOVE_RANGE,
    STR_DELETE,
    STR_DELETE_RANGE,

    // Advanced operations
    STR_REVERSE,
    STR_SWAP_ITEMS,
    STR_SORT,
    STR_MERGE,
    STR_MERGE_CSTR,
    STR_MERGE_ZSTR,
    STR_MERGE_STR,

    // Foreach operations
    STR_FOREACH,
    STR_FOREACH_IDX,
    STR_FOREACH_PTR,
    STR_FOREACH_PTR_IDX,
    STR_FOREACH_REVERSE,
    STR_FOREACH_REVERSE_IDX,
    STR_FOREACH_PTR_REVERSE,
    STR_FOREACH_PTR_REVERSE_IDX,
    STR_FOREACH_IN_RANGE,
    STR_FOREACH_IN_RANGE_IDX,
    STR_FOREACH_PTR_IN_RANGE,
    STR_FOREACH_PTR_IN_RANGE_IDX,

    // Utility operations
    STR_PRINTF,
    STR_VALIDATE,

    STR_COUNT
} StrFunction;

// Function prototypes
void init_str(Str *str, DefaultAllocator *alloc);
void deinit_str(Str *str);
void fuzz_str(
    Str              *str,
    StrFunction       func,
    const uint8_t    *data,
    size_t           *offset,
    size_t            size,
    DefaultAllocator *alloc
);

#endif // FUZZ_STR_H
