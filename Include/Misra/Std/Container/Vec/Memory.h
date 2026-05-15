/// file      : std/container/vec/memory.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Memory operations on vector

#ifndef MISRA_STD_CONTAINER_VEC_MEMORY_H
#define MISRA_STD_CONTAINER_VEC_MEMORY_H

#include "Type.h"
#include "Private.h"

///
/// Try to shrink the allocated capacity of the vector back to its current
/// length. Use when previous growth left a large unused tail.
///
/// v[in,out] : Vector handle.
///
/// SUCCESS : Returns `true`. The vector's capacity now equals its current
///           length; any over-allocated tail bytes have been returned to the
///           allocator.
/// FAILURE : Returns `false` on allocation failure during the shrink
///           reallocation. The vector is unchanged (length, capacity, and
///           data pointer are all preserved).
///
/// TAGS: Vec, Memory, ReduceSpace
///
#define VecTryReduceSpace(v) (reduce_space_vec(GENERIC_VEC(v), sizeof(VEC_DATATYPE(v))))

///
/// Aborting variant of `VecTryReduceSpace`.
///
/// SUCCESS : Returns to the caller.
/// FAILURE : Does not return - aborts via `LOG_FATAL` / `Abort`.
///
/// TAGS: Vec, Memory, ReduceSpace, Must, Abort
///
#define VecMustTryReduceSpace(v)                                                                                       \
    do {                                                                                                               \
        if (!VecTryReduceSpace((v))) {                                                                                 \
            LOG_FATAL("VecTryReduceSpace failed");                                                                     \
        }                                                                                                              \
    } while (0)

///
/// Resize the vector to exactly `len` elements. Truncates when shrinking and
/// allocates when growing. New elements (when growing) are zero-initialized.
///
/// v[in,out] : Vector handle.
/// len[in]   : New length.
///
/// SUCCESS : Returns `true`. The vector length is now exactly `len`. When
///           shrinking, the elements beyond `len` were deinitialized via the
///           configured `copy_deinit` (if any) and dropped. When growing,
///           the new slots in [old_length, len) are zero-initialized; the
///           allocated capacity is at least `len`.
/// FAILURE : Returns `false` on allocation failure when growth is needed.
///           The vector is unchanged (length, capacity, and elements all
///           preserved). Shrinking does not allocate and therefore cannot
///           fail.
///
/// TAGS: Vec, Memory, Resize
///
#define VecResize(v, len) (resize_vec(GENERIC_VEC(v), sizeof(VEC_DATATYPE(v)), (len)))

///
/// Aborting variant of `VecResize`.
///
/// SUCCESS : Returns to the caller.
/// FAILURE : Does not return - aborts via `LOG_FATAL` / `Abort`.
///
/// TAGS: Vec, Memory, Resize, Must, Abort
///
#define VecMustResize(v, len)                                                                                          \
    do {                                                                                                               \
        if (!VecResize((v), (len))) {                                                                                  \
            LOG_FATAL("VecResize failed");                                                                             \
        }                                                                                                              \
    } while (0)

///
/// Reserve enough capacity to fit at least `n` elements without further
/// allocation. Does not change the vector length.
///
/// v[in,out] : Vector handle.
/// n[in]     : Minimum capacity in elements.
///
/// SUCCESS : Returns `true`. The vector's allocated capacity is now at least
///           `n` elements. The vector length and the values of all existing
///           elements are unchanged.
/// FAILURE : Returns `false` on allocation failure. The vector is unchanged.
///
/// TAGS: Vec, Memory, Reserve
///
#define VecReserve(v, n) (reserve_vec(GENERIC_VEC(v), sizeof(VEC_DATATYPE(v)), (n)))

///
/// Aborting variant of `VecReserve`.
///
/// SUCCESS : Returns to the caller.
/// FAILURE : Does not return - aborts via `LOG_FATAL` / `Abort`.
///
/// TAGS: Vec, Memory, Reserve, Must, Abort
///
#define VecMustReserve(v, n)                                                                                           \
    do {                                                                                                               \
        if (!VecReserve((v), (n))) {                                                                                   \
            LOG_FATAL("VecReserve failed");                                                                            \
        }                                                                                                              \
    } while (0)

///
/// Set the vector length to 0 while keeping the allocated capacity.
/// Element payloads are deinitialized via the configured `copy_deinit` handler
/// when present.
///
/// v[in,out] : Vector handle.
///
/// SUCCESS : Returns to the caller. The vector length is now 0; the
///           allocated capacity and data buffer are preserved. When
///           `copy_deinit` is configured it has been invoked on each
///           previously-stored element.
/// FAILURE : Function cannot fail.
///
/// TAGS: Vec, Memory, Clear
///
#define VecClear(v) (clear_vec(GENERIC_VEC(v), sizeof(VEC_DATATYPE(v))))

#endif // MISRA_STD_CONTAINER_VEC_MEMORY_H
