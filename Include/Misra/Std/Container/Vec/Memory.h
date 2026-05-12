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
/// Try reducing memory footprint of vector.
/// This is to be used when we know actual allocated memory for vec is large,
/// and we won't need it in future, so we can reduce it to whatever's required at
/// the moment.
///
/// v[in,out] : Vector
///
/// SUCCESS : return
/// FAILURE : Does not return
///
#define VecTryReduceSpace(v) (reduce_space_vec(GENERIC_VEC(v), sizeof(VEC_DATATYPE(v))))
#define VecMustTryReduceSpace(v)                                                                                        \
    do {                                                                                                               \
        if (!VecTryReduceSpace((v))) {                                                                                 \
            LOG_FATAL("VecTryReduceSpace failed");                                                                     \
        }                                                                                                              \
    } while (0)

///
/// Resize vector.
/// If length is smaller than current capacity, vector length is shrinked.
/// If length is greater than current capacity, space is reserved and vector is expanded.
///
/// vec[in,out] : Vector to be resized.
/// len[in]     : New length of vector.
///
/// SUCCESS : return
/// FAILURE : Does not return
///
#define VecResize(v, len) (resize_vec(GENERIC_VEC(v), sizeof(VEC_DATATYPE(v)), (len)))
#define VecMustResize(v, len)                                                                                          \
    do {                                                                                                               \
        if (!VecResize((v), (len))) {                                                                                  \
            LOG_FATAL("VecResize failed");                                                                             \
        }                                                                                                              \
    } while (0)

///
/// Reserve space for vector.
///
/// vec[in,out] : Vector to be resized.
/// len[in]     : New capacity of vector.
///
/// SUCCESS : return
/// FAILURE : Does not return
///
#define VecReserve(v, n) (reserve_vec(GENERIC_VEC(v), sizeof(VEC_DATATYPE(v)), (n)))
#define VecMustReserve(v, n)                                                                                           \
    do {                                                                                                               \
        if (!VecReserve((v), (n))) {                                                                                   \
            LOG_FATAL("VecReserve failed");                                                                            \
        }                                                                                                              \
    } while (0)

///
/// Clear vec contents.
///
/// vec[in,out] : Vector to be cleared.
///
/// SUCCESS : return
/// FAILURE : Does not return
///
#define VecClear(v) (clear_vec(GENERIC_VEC(v), sizeof(VEC_DATATYPE(v))))

#endif // MISRA_STD_CONTAINER_VEC_MEMORY_H
