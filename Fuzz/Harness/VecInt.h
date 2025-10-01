/// file      : fuzz/VecIntHarness.h
/// author    : Generated for MisraStdC fuzzing
/// This is free and unencumbered software released into the public domain.
///
/// Vec(i32) specific fuzzing harness

#ifndef FUZZ_VEC_INT_H
#define FUZZ_VEC_INT_H

#include <Misra/Std/Container/Vec.h>
#include <Misra/Types.h>

// Vec(i32) typedef
typedef Vec(i32) IntVec;

// Vec(i32) function enumeration
typedef enum {
    VEC_INT_PUSH_BACK = 0,
    VEC_INT_PUSH_FRONT,
    VEC_INT_POP_BACK,
    VEC_INT_POP_FRONT,
    VEC_INT_INSERT,
    VEC_INT_REMOVE,
    VEC_INT_DELETE,
    VEC_INT_AT,
    VEC_INT_LEN,
    VEC_INT_FIRST,
    VEC_INT_LAST,

    // Memory operations
    VEC_INT_CLEAR,
    VEC_INT_RESIZE,
    VEC_INT_RESERVE,
    VEC_INT_TRY_REDUCE_SPACE,
    VEC_INT_SIZE,

    // Advanced operations
    VEC_INT_REVERSE,
    VEC_INT_SWAP_ITEMS,

    // Range operations
    VEC_INT_INSERT_RANGE,
    VEC_INT_REMOVE_RANGE,
    VEC_INT_DELETE_RANGE,
    VEC_INT_INSERT_FAST,
    VEC_INT_REMOVE_FAST,
    VEC_INT_REMOVE_RANGE_FAST,
    VEC_INT_DELETE_RANGE_FAST,

    // Array operations
    VEC_INT_PUSH_BACK_ARRAY,
    VEC_INT_PUSH_FRONT_ARRAY,
    VEC_INT_PUSH_FRONT_ARRAY_FAST,

    // Advanced functions
    VEC_INT_SORT,
    VEC_INT_BEGIN,
    VEC_INT_END,
    VEC_INT_PTR_AT,
    VEC_INT_MERGE,
    VEC_INT_INSERT_RANGE_FAST,
    VEC_INT_ALIGNED_OFFSET_AT,
    VEC_INT_DELETE_LAST,
    VEC_INT_DELETE_FAST,
    VEC_INT_INIT_CLONE,

    // Foreach operations
    VEC_INT_FOREACH,
    VEC_INT_FOREACH_IDX,
    VEC_INT_FOREACH_PTR,
    VEC_INT_FOREACH_PTR_IDX,
    VEC_INT_FOREACH_REVERSE,
    VEC_INT_FOREACH_REVERSE_IDX,
    VEC_INT_FOREACH_PTR_REVERSE,
    VEC_INT_FOREACH_PTR_REVERSE_IDX,
    VEC_INT_FOREACH_IN_RANGE,
    VEC_INT_FOREACH_IN_RANGE_IDX,
    VEC_INT_FOREACH_PTR_IN_RANGE,
    VEC_INT_FOREACH_PTR_IN_RANGE_IDX,

    VEC_INT_COUNT
} VecIntFunction;

// Function prototypes
void init_int_vec(IntVec *vec);
void deinit_int_vec(IntVec *vec);
void fuzz_int_vec(IntVec *vec, VecIntFunction func, const uint8_t *data, size_t *offset, size_t size);

#endif // FUZZ_VEC_INT_H
