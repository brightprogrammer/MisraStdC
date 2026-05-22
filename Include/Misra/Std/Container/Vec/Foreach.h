/// file      : std/container/vec/access.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)

#ifndef MISRA_STD_CONTAINER_VEC_FOREACH_H
#define MISRA_STD_CONTAINER_VEC_FOREACH_H

#include "Type.h"
#include "Private.h"

///
/// Iterate over each element `var` of given vector `v` at each index `idx` into the vector.
/// The variables `var` and `idx` declared and defined by this macro.
///
/// `idx` will start from 0 and will go till v->length - 1
///
/// v[in,out] : Vector to iterate over.
/// var[in]   : Name of variable to be used which'll contain value at iterated index `idx`
/// idx[in]   : Name of variable to be used for iterating over indices.
///
/// TAGS: Foreach, Vec, Iteration, Loop
///
#define VecForeachIdx(v, var, idx)                                                                                     \
    for (TYPE_OF(v) UNPL(pv) = (v); UNPL(pv); UNPL(pv) = NULL)                                                         \
        if ((ValidateVec(UNPL(pv)), 1) && UNPL(pv)->length > 0)                                                        \
            for (u64 idx = 0, UNPL(d) = 1; UNPL(d); UNPL(d)--)                                                         \
                for (VEC_DATATYPE(UNPL(pv)) var = {0}; idx < UNPL(pv)->length && (var = VecAt(UNPL(pv), idx), 1); idx++)

///
/// Iterate over each element `var` of given vector `v` at each index `idx` into the vector.
/// The variables `var` and `idx` declared and defined by this macro.
///
/// `idx` will start from v->length - 1 and will go till 0
///
/// v[in,out] : Vector to iterate over.
/// var[in]   : Name of variable to be used which'll contain value at iterated index `idx`
/// idx[in]   : Name of variable to be used for iterating over indices.
///
/// TAGS: Foreach, Vec, Iteration, Loop, Reverse
///
#define VecForeachReverseIdx(v, var, idx)                                                                              \
    for (TYPE_OF(v) UNPL(pv) = (v); UNPL(pv); UNPL(pv) = NULL)                                                         \
        if ((ValidateVec(UNPL(pv)), 1) && UNPL(pv)->length > 0)                                                        \
            for (u64 idx = UNPL(pv)->length; idx-- > 0 && idx < UNPL(pv)->length;)                                     \
                for (u8 UNPL(run_once) = 1; UNPL(run_once); UNPL(run_once) = 0)                                        \
                    for (VEC_DATATYPE(UNPL(pv)) var = VecAt(UNPL(pv), idx); UNPL(run_once); UNPL(run_once) = 0)

///
/// Iterate over each element `var` of given vector `v` at each index `idx` into the vector.
/// The variables `var` and `idx` declared and defined by this macro.
///
/// `idx` will start from 0 and will go till v->length - 1
///
/// v[in,out] : Vector to iterate over.
/// var[in]   : Name of variable to be used which'll contain pointer to value at iterated index `idx`
/// idx[in]   : Name of variable to be used for iterating over indices.
///
/// TAGS: Foreach, Vec, Iteration, Loop, Pointer
///
#define VecForeachPtrIdx(v, var, idx)                                                                                  \
    for (TYPE_OF(v) UNPL(pv) = (v); UNPL(pv); UNPL(pv) = NULL)                                                         \
        if ((ValidateVec(UNPL(pv)), 1) && UNPL(pv)->length > 0)                                                        \
            for (u64 idx = 0, UNPL(d) = 1; UNPL(d); UNPL(d)--)                                                         \
                for (VEC_DATATYPE(UNPL(pv)) *var = NULL; idx < UNPL(pv)->length && (var = VecPtrAt(UNPL(pv), idx), 1); \
                     idx++)

///
/// Iterate over each element `var` of given vector `v` at each index `idx` into the vector.
/// The variables `var` and `idx` declared and defined by this macro.
///
/// `idx` will start from v->length - 1 and will go till 0
///
/// v[in,out] : Vector to iterate over.
/// var[in]   : Name of variable to be used which'll contain pointer to value at iterated index `idx`
/// idx[in]   : Name of variable to be used for iterating over indices.
///
/// TAGS: Foreach, Vec, Iteration, Loop, Reverse, Pointer
///
#define VecForeachPtrReverseIdx(v, var, idx)                                                                           \
    for (TYPE_OF(v) UNPL(pv) = (v); UNPL(pv); UNPL(pv) = NULL)                                                         \
        if ((ValidateVec(UNPL(pv)), 1) && UNPL(pv)->length > 0)                                                        \
            for (u64 idx = UNPL(pv)->length; idx-- > 0 && idx < UNPL(pv)->length;)                                     \
                for (u8 UNPL(run_once) = 1; UNPL(run_once); UNPL(run_once) = 0)                                        \
                    for (VEC_DATATYPE(UNPL(pv)) *var = VecPtrAt(UNPL(pv), idx); UNPL(run_once); UNPL(run_once) = 0)

///
/// Iterate over each element `var` of the given vector `v`.
/// This is a convenience macro that iterates forward using an internally managed index.
/// The variable `var` is declared and defined by this macro.
///
/// v[in,out] : Vector to iterate over.
/// var[in]   : Name of the variable to be used which will contain the value of the
///             current element during iteration. The type of `var` will be the
///             data type of the vector elements (obtained via `VEC_DATATYPE(v)`).
///
/// TAGS: Foreach, Vec, Iteration, Loop
///
#define VecForeach(v, var) VecForeachIdx((v), (var), UNPL(iter))

///
/// Iterate over each element `var` of the given vector `v` in reverse order.
/// This is a convenience macro that iterates backward using an internally managed index.
/// The variable `var` is declared and defined by this macro.
///
/// v[in,out] : Vector to iterate over.
/// var[in]   : Name of the variable to be used which will contain the value of the
///             current element during iteration. The type of `var` will be the
///             data type of the vector elements (obtained via `VEC_DATATYPE(v)`).
///
/// TAGS: Foreach, Vec, Iteration, Loop, Reverse
///
#define VecForeachReverse(v, var) VecForeachReverseIdx((v), (var), UNPL(iter))

///
/// Iterate over each element `var` of the given vector `v` (as a pointer).
/// This is a convenience macro that iterates forward using an internally managed index
/// and provides a pointer to each element. The variable `var` is declared and defined
/// by this macro as a pointer to the vector's data type.
///
/// v[in,out] : Vector to iterate over.
/// var[in]   : Name of the pointer variable to be used which will point to the
///             current element during iteration. The type of `var` will be a pointer
///             to the data type of the vector elements (obtained via
///             `VEC_DATATYPE(v) *`).
///
/// TAGS: Foreach, Vec, Iteration, Loop, Pointer
///
#define VecForeachPtr(v, var) VecForeachPtrIdx((v), (var), UNPL(iter))

///
/// Iterate over each element `var` (as a pointer) of the given vector `v` in reverse order.
/// This is a convenience macro that iterates backward using an internally managed index
/// and provides a pointer to each element. The variable `var` is declared and defined
/// by this macro as a pointer to the vector's data type.
///
/// v[in,out] : Vector to iterate over.
/// var[in]   : Name of the pointer variable to be used which will point to the
///             current element during iteration. The type of `var` will be a pointer
///             to the data type of the vector elements (obtained via
///             `VEC_DATATYPE(v) *`).
///
/// TAGS: Foreach, Vec, Iteration, Loop, Reverse, Pointer
///
#define VecForeachPtrReverse(v, var) VecForeachPtrReverseIdx((v), (var), UNPL(iter))

///
/// Iterate over elements in a specific range of the given vector `v` at each index `idx`.
/// The variables `var` and `idx` are declared and defined by this macro.
///
/// `idx` will start from `start` and will go till `end - 1`
///
/// v[in,out]    : Vector to iterate over.
/// var[in]      : Name of variable to be used which'll contain value at iterated index `idx`.
/// idx[in]      : Name of variable to be used for iterating over indices.
/// start[in]    : Starting index (inclusive).
/// end[in]      : Ending index (exclusive).
///
/// TAGS: Foreach, Vec, Iteration, Loop, Range
///
#define VecForeachInRangeIdx(v, var, idx, start, end)                                                                  \
    for (TYPE_OF(v) UNPL(pv) = (v); UNPL(pv); UNPL(pv) = NULL)                                                         \
        if ((ValidateVec(UNPL(pv)), 1) && UNPL(pv)->length > 0)                                                        \
            for (u64 UNPL(s) = (start), UNPL(e) = (end), idx = UNPL(s), UNPL(d) = 1;                                   \
                 UNPL(s) <= idx && idx < UNPL(e) && idx < UNPL(pv)->length && UNPL(s) <= UNPL(e);                      \
                 ++idx, UNPL(d) = 1)                                                                                   \
                for (VEC_DATATYPE(UNPL(pv)) var = VecAt(UNPL(pv), idx); UNPL(d); UNPL(d) = 0)

///
/// Iterate over elements in a specific range of the given vector `v`.
/// This is a convenience macro that iterates over a range using an internally managed index.
/// The variable `var` is declared and defined by this macro.
///
/// v[in,out]    : Vector to iterate over.
/// var[in]      : Name of variable to be used which'll contain value of the current element.
/// start[in]    : Starting index (inclusive).
/// end[in]      : Ending index (exclusive).
///
/// TAGS: Foreach, Vec, Iteration, Loop, Range
///
#define VecForeachInRange(v, var, start, end) VecForeachInRangeIdx((v), (var), UNPL(iter), (start), (end))

///
/// Iterate over elements in a specific range of the given vector `v` at each index `idx` (as pointers).
/// The variables `var` and `idx` are declared and defined by this macro.
///
/// `idx` will start from `start` and will go till `end - 1`
///
/// v[in,out]    : Vector to iterate over.
/// var[in]      : Name of pointer variable to be used which'll point to value at iterated index `idx`.
/// idx[in]      : Name of variable to be used for iterating over indices.
/// start[in]    : Starting index (inclusive).
/// end[in]      : Ending index (exclusive).
///
/// TAGS: Foreach, Vec, Iteration, Loop, Range, Pointer
///
#define VecForeachPtrInRangeIdx(v, var, idx, start, end)                                                               \
    for (TYPE_OF(v) UNPL(pv) = (v); UNPL(pv); UNPL(pv) = NULL)                                                         \
        if ((ValidateVec(UNPL(pv)), 1) && UNPL(pv)->length > 0)                                                        \
            for (u64 UNPL(s) = (start), UNPL(e) = (end), idx = UNPL(s), UNPL(d) = 1;                                   \
                 idx >= UNPL(s) && idx < UNPL(e) && idx < UNPL(pv)->length && UNPL(s) <= UNPL(e);                      \
                 ++idx, UNPL(d) = 1)                                                                                   \
                for (VEC_DATATYPE(UNPL(pv)) *var = VecPtrAt(UNPL(pv), idx); UNPL(d); UNPL(d) = 0)

///
/// Iterate over elements in a specific range of the given vector `v` (as pointers).
/// This is a convenience macro that iterates over a range using an internally managed index
/// and provides a pointer to each element. The variable `var` is declared and defined
/// by this macro as a pointer to the vector's data type.
///
/// v[in,out]    : Vector to iterate over.
/// var[in]      : Name of pointer variable to be used which'll point to the current element.
/// start[in]    : Starting index (inclusive).
/// end[in]      : Ending index (exclusive).
///
/// TAGS: Foreach, Vec, Iteration, Loop, Range, Pointer
///
#define VecForeachPtrInRange(v, var, start, end) VecForeachPtrInRangeIdx((v), (var), UNPL(iter), (start), (end))

#endif // MISRA_STD_CONTAINER_VEC_FOREACH_H
