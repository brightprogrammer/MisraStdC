/// file      : fuzz/VecStrHarness.h
/// author    : Generated for MisraStdC fuzzing
/// This is free and unencumbered software released into the public domain.
///
/// Vec(Str) specific fuzzing harness

#ifndef FUZZ_VEC_STR_H
#define FUZZ_VEC_STR_H

#include <Misra/Std/Container/Vec.h>
#include <Misra/Std/Container/Str.h>
#include <Misra/Types.h>

// Vec(Str) typedef
typedef Vec(Str) StrVec;

// Vec(Str) function enumeration
typedef enum {
    VEC_STR_PUSH_BACK = 0,
    VEC_STR_PUSH_FRONT,
    VEC_STR_POP_BACK,
    VEC_STR_POP_FRONT,
    VEC_STR_INSERT,
    VEC_STR_REMOVE,
    VEC_STR_DELETE,
    VEC_STR_AT,
    VEC_STR_LEN,
    VEC_STR_FIRST,
    VEC_STR_LAST,

    // Memory operations
    VEC_STR_CLEAR,
    VEC_STR_RESIZE,
    VEC_STR_RESERVE,
    VEC_STR_TRY_REDUCE_SPACE,
    VEC_STR_SIZE,

    // Advanced operations
    VEC_STR_REVERSE,
    VEC_STR_SWAP_ITEMS,

    // Range operations
    VEC_STR_INSERT_RANGE,
    VEC_STR_REMOVE_RANGE,
    VEC_STR_DELETE_RANGE,
    VEC_STR_INSERT_FAST,
    VEC_STR_REMOVE_FAST,
    VEC_STR_REMOVE_RANGE_FAST,
    VEC_STR_DELETE_RANGE_FAST,

    // Array operations
    VEC_STR_PUSH_BACK_ARRAY,
    VEC_STR_PUSH_FRONT_ARRAY,
    VEC_STR_PUSH_FRONT_ARRAY_FAST,

    // Advanced functions
    VEC_STR_SORT,
    VEC_STR_BEGIN,
    VEC_STR_END,
    VEC_STR_PTR_AT,
    VEC_STR_MERGE,
    VEC_STR_INSERT_RANGE_FAST,
    VEC_STR_ALIGNED_OFFSET_AT,
    VEC_STR_DELETE_LAST,
    VEC_STR_DELETE_FAST,
    VEC_STR_INIT_CLONE,

    // Foreach operations
    VEC_STR_FOREACH,
    VEC_STR_FOREACH_IDX,
    VEC_STR_FOREACH_PTR,
    VEC_STR_FOREACH_PTR_IDX,
    VEC_STR_FOREACH_REVERSE,
    VEC_STR_FOREACH_REVERSE_IDX,
    VEC_STR_FOREACH_PTR_REVERSE,
    VEC_STR_FOREACH_PTR_REVERSE_IDX,
    VEC_STR_FOREACH_IN_RANGE,
    VEC_STR_FOREACH_IN_RANGE_IDX,
    VEC_STR_FOREACH_PTR_IN_RANGE,
    VEC_STR_FOREACH_PTR_IN_RANGE_IDX,

    VEC_STR_COUNT
} VecStrFunction;

// Function prototypes
void init_str_vec(StrVec *vec);
void deinit_str_vec(StrVec *vec);
void fuzz_str_vec(StrVec *vec, VecStrFunction func, const uint8_t *data, size_t *offset, size_t size);

#endif // FUZZ_VEC_STR_H
