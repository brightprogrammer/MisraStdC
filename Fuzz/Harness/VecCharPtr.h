/// file      : fuzz/VecCharPtrHarness.h
/// author    : Generated for MisraStdC fuzzing
/// This is free and unencumbered software released into the public domain.
///
/// Vec(char*) specific fuzzing harness

#ifndef FUZZ_VEC_CHAR_PTR_H
#define FUZZ_VEC_CHAR_PTR_H

#include <Misra/Std/Container/Vec.h>
#include <Misra/Types.h>

// Vec(char*) typedef
typedef Vec(char *) CharPtrVec;

// Vec(char*) function enumeration
typedef enum {
    VEC_CHAR_PTR_PUSH_BACK = 0,
    VEC_CHAR_PTR_PUSH_FRONT,
    VEC_CHAR_PTR_POP_BACK,
    VEC_CHAR_PTR_POP_FRONT,
    VEC_CHAR_PTR_INSERT,
    VEC_CHAR_PTR_REMOVE,
    VEC_CHAR_PTR_DELETE,
    VEC_CHAR_PTR_AT,
    VEC_CHAR_PTR_LEN,
    VEC_CHAR_PTR_FIRST,
    VEC_CHAR_PTR_LAST,

    // Memory operations
    VEC_CHAR_PTR_CLEAR,
    VEC_CHAR_PTR_RESIZE,
    VEC_CHAR_PTR_RESERVE,
    VEC_CHAR_PTR_TRY_REDUCE_SPACE,
    VEC_CHAR_PTR_SIZE,

    // Advanced operations
    VEC_CHAR_PTR_REVERSE,
    VEC_CHAR_PTR_SWAP_ITEMS,

    // Range operations
    VEC_CHAR_PTR_INSERT_RANGE,
    VEC_CHAR_PTR_REMOVE_RANGE,
    VEC_CHAR_PTR_DELETE_RANGE,
    VEC_CHAR_PTR_INSERT_FAST,
    VEC_CHAR_PTR_REMOVE_FAST,
    VEC_CHAR_PTR_REMOVE_RANGE_FAST,
    VEC_CHAR_PTR_DELETE_RANGE_FAST,

    // Array operations
    VEC_CHAR_PTR_PUSH_BACK_ARRAY,
    VEC_CHAR_PTR_PUSH_FRONT_ARRAY,
    VEC_CHAR_PTR_PUSH_FRONT_ARRAY_FAST,

    // Advanced functions
    VEC_CHAR_PTR_SORT,
    VEC_CHAR_PTR_BEGIN,
    VEC_CHAR_PTR_END,
    VEC_CHAR_PTR_PTR_AT,
    VEC_CHAR_PTR_MERGE,
    VEC_CHAR_PTR_INSERT_RANGE_FAST,
    VEC_CHAR_PTR_ALIGNED_OFFSET_AT,
    VEC_CHAR_PTR_DELETE_LAST,
    VEC_CHAR_PTR_DELETE_FAST,
    VEC_CHAR_PTR_INIT_CLONE,

    // Foreach operations
    VEC_CHAR_PTR_FOREACH,
    VEC_CHAR_PTR_FOREACH_IDX,
    VEC_CHAR_PTR_FOREACH_PTR,
    VEC_CHAR_PTR_FOREACH_PTR_IDX,
    VEC_CHAR_PTR_FOREACH_REVERSE,
    VEC_CHAR_PTR_FOREACH_REVERSE_IDX,
    VEC_CHAR_PTR_FOREACH_PTR_REVERSE,
    VEC_CHAR_PTR_FOREACH_PTR_REVERSE_IDX,
    VEC_CHAR_PTR_FOREACH_IN_RANGE,
    VEC_CHAR_PTR_FOREACH_IN_RANGE_IDX,
    VEC_CHAR_PTR_FOREACH_PTR_IN_RANGE,
    VEC_CHAR_PTR_FOREACH_PTR_IN_RANGE_IDX,

    VEC_CHAR_PTR_COUNT
} VecCharPtrFunction;

// Function prototypes
void init_char_ptr_vec(CharPtrVec *vec);
void deinit_char_ptr_vec(CharPtrVec *vec);
void fuzz_char_ptr_vec(CharPtrVec *vec, VecCharPtrFunction func, const uint8_t *data, size_t *offset, size_t size);

#endif // FUZZ_VEC_CHAR_PTR_H
