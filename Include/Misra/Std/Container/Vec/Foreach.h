/// file      : std/container/vec/access.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)

#ifndef MISRA_STD_CONTAINER_VEC_FOREACH_H
#define MISRA_STD_CONTAINER_VEC_FOREACH_H

#include "Type.h"
#include "Private.h"
#include <Misra/Std/Log.h>

///
/// Iterate over each element `var` of given vector `v` at each index `idx` into the vector.
/// The variables `var` and `idx` declared and defined by this macro.
///
/// `idx` will start from 0 and will go till v->length - 1
///
/// v[in,out] : Vector to iterate over.
/// var[in]   : Name of variable to be used which'll contain value at iterated index `idx`
/// idx[in]   : Name of variable to be used for iterating over indices.
/// body      : Body of this foreach loop
///
/// SUCCESS : The `body` is executed for each element of the vector `v` from the
///           beginning to the end.
/// FAILURE : If the vector `v` is NULL or its length is zero, the loop body will not
///           be executed. Any failures within the `VecForeachIdx` macro (like invalid
///           index access) will result in a fatal log message and program termination.
///
#define VecForeachIdx(v, var, idx, body)                                                                               \
    do {                                                                                                               \
        size idx            = 0;                                                                                       \
        VEC_DATATYPE(v) var = {0};                                                                                     \
        if ((v) != NULL && (v)->length > 0) {                                                                          \
            for ((idx) = 0; (idx) < (v)->length; ++(idx)) {                                                            \
                var = VecAt(v, idx);                                                                                   \
                { body }                                                                                               \
                if ((idx) >= (v)->length) {                                                                            \
                    LOG_FATAL("Vector range overflow : Invalid index reached during Foreach iteration.");              \
                }                                                                                                      \
            }                                                                                                          \
        }                                                                                                              \
    } while (0)

///
/// Iterate over each element `var` of given vector `v` at each index `idx` into the vector.
/// The variables `var` and `idx` declared and defined by this macro.
///
/// `idx` will start from v->length - 1 and will go till 0
///
/// v[in,out] : Vector to iterate over.
/// var[in]   : Name of variable to be used which'll contain value at iterated index `idx`
/// idx[in]   : Name of variable to be used for iterating over indices.
/// body      : Body of this foreach loop
///
#define VecForeachReverseIdx(v, var, idx, body)                                                                        \
    do {                                                                                                               \
        size idx            = 0;                                                                                       \
        VEC_DATATYPE(v) var = {0};                                                                                     \
        if ((v) != NULL && (v)->length > 0) {                                                                          \
            for ((idx) = (v)->length - 1; (idx) < (v)->length; --(idx)) {                                              \
                var = VecAt(v, idx);                                                                                   \
                { body }                                                                                               \
                if ((idx) >= (v)->length) {                                                                            \
                    LOG_FATAL("Vector range overflow : Invalid index reached during Foreach reverse iteration.");      \
                }                                                                                                      \
                if (idx == 0)                                                                                          \
                    break; /* Stop after processing index 0 */                                                         \
            }                                                                                                          \
        }                                                                                                              \
    } while (0)

///
/// Iterate over each element `var` of given vector `v` at each index `idx` into the vector.
/// The variables `var` and `idx` declared and defined by this macro.
///
/// `idx` will start from 0 and will go till v->length - 1
///
/// v[in,out] : Vector to iterate over.
/// var[in]   : Name of variable to be used which'll contain value at iterated index `idx`
/// idx[in]   : Name of variable to be used for iterating over indices.
/// body      : Body of this foreach loop
///
#define VecForeachPtrIdx(v, var, idx, body)                                                                            \
    do {                                                                                                               \
        size idx             = 0;                                                                                      \
        VEC_DATATYPE(v) *var = NULL;                                                                                   \
        if ((v) != NULL && (v)->length > 0) {                                                                          \
            for ((idx) = 0; (idx) < (v)->length; ++(idx)) {                                                            \
                var = VecPtrAt(v, idx);                                                                                \
                body if ((idx) >= (v)->length) {                                                                       \
                    LOG_FATAL("Vector range overflow : Invalid index reached during Foreach iteration.");              \
                }                                                                                                      \
            }                                                                                                          \
        }                                                                                                              \
    } while (0)

///
/// Iterate over each element `var` of given vector `v` at each index `idx` into the vector.
/// The variables `var` and `idx` declared and defined by this macro.
///
/// `idx` will start from v->length - 1 and will go till 0
///
/// v[in,out] : Vector to iterate over.
/// var[in]   : Name of variable to be used which'll contain value at iterated index `idx`
/// idx[in]   : Name of variable to be used for iterating over indices.
/// body      : Body of this foreach loop
///
#define VecForeachPtrReverseIdx(v, var, idx, body)                                                                     \
    do {                                                                                                               \
        size idx             = 0;                                                                                      \
        VEC_DATATYPE(v) *var = {0};                                                                                    \
        if ((v) != NULL && (v)->length > 0) {                                                                          \
            for ((idx) = (v)->length - 1; (idx) < (v)->length; --(idx)) {                                              \
                var = VecPtrAt(v, idx);                                                                                \
                { body }                                                                                               \
                if ((idx) >= (v)->length) {                                                                            \
                    LOG_FATAL("Vector range overflow : Invalid index reached during Foreach reverse iteration.");      \
                }                                                                                                      \
                if (idx == 0)                                                                                          \
                    break; /* Stop after processing index 0 */                                                         \
            }                                                                                                          \
        }                                                                                                              \
    } while (0)

///
/// Iterate over each element `var` of the given vector `v`.
/// This is a convenience macro that iterates forward using an internally managed index.
/// The variable `var` is declared and defined by this macro.
///
/// v[in,out] : Vector to iterate over.
/// var[in]   : Name of the variable to be used which will contain the value of the
///             current element during iteration. The type of `var` will be the
///             data type of the vector elements (obtained via `VEC_DATATYPE(v)`).
/// body      : The block of code to be executed for each element of the vector.
///
/// SUCCESS : The `body` is executed for each element of the vector `v` from the
///           beginning to the end.
/// FAILURE : If the vector `v` is NULL or its length is zero, the loop body will not
///           be executed. Any failures within the `VecForeachIdx` macro (like invalid
///           index access) will result in a fatal log message and program termination.
///
#define VecForeach(v, var, body) VecForeachIdx((v), (var), (____iter___), {body})

///
/// Iterate over each element `var` of the given vector `v` in reverse order.
/// This is a convenience macro that iterates backward using an internally managed index.
/// The variable `var` is declared and defined by this macro.
///
/// v[in,out] : Vector to iterate over.
/// var[in]   : Name of the variable to be used which will contain the value of the
///             current element during iteration. The type of `var` will be the
///             data type of the vector elements (obtained via `VEC_DATATYPE(v)`).
/// body      : The block of code to be executed for each element of the vector.
///
/// SUCCESS : The `body` is executed for each element of the vector `v` from the
///           end to the beginning.
/// FAILURE : If the vector `v` is NULL or its length is zero, the loop body will not
///           be executed. Any failures within the `VecForeachReverseIdx` macro (like
///           invalid index access) will result in a fatal log message and program termination.
///
#define VecForeachReverse(v, var, body) VecForeachReverseIdx((v), (var), (____iter___), {body})

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
/// body      : The block of code to be executed for each element of the vector.
///
/// SUCCESS : The `body` is executed for each element of the vector `v` (with `var`
///           pointing to the current element) from the beginning to the end.
/// FAILURE : If the vector `v` is NULL or its length is zero, the loop body will not
///           be executed. Any failures within the `VecForeachPtrIdx` macro (like invalid
///           index access) will result in a fatal log message and program termination.
///
#define VecForeachPtr(v, var, body) VecForeachPtrIdx((v), (var), (____iter___), {body})

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
/// body      : The block of code to be executed for each element of the vector.
///
/// SUCCESS : The `body` is executed for each element of the vector `v` (with `var`
///           pointing to the current element) from the end to the beginning.
/// FAILURE : If the vector `v` is NULL or its length is zero, the loop body will not
///           be executed. Any failures within the `VecForeachPtrReverseIdx` macro (like
///           invalid index access) will result in a fatal log message and program termination.
///
#define VecForeachPtrReverse(v, var, body) VecForeachPtrReverseIdx((v), (var), (____iter___), {body})

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
/// body         : Body of this foreach loop.
///
/// SUCCESS : The `body` is executed for each element of the vector `v` from the
///           `start` index to the `end-1` index.
/// FAILURE : If the vector `v` is NULL, its length is zero, or the range is invalid,
///           the loop body will not be executed. Any access to an invalid index will
///           result in a fatal log message and program termination.
///
#define VecForeachInRangeIdx(v, var, idx, start, end, body)                                                            \
    do {                                                                                                               \
        size idx            = 0;                                                                                       \
        VEC_DATATYPE(v) var = {0};                                                                                     \
        if ((v) != NULL && (v)->length > 0) {                                                                          \
            if ((end) > (v)->length) {                                                                                 \
                LOG_FATAL(                                                                                             \
                    "Vector range overflow: End index %zu exceeds vector length %zu. "                                 \
                    "If you intended to iterate over all items, use VecForeach instead.",                              \
                    (end),                                                                                             \
                    (v)->length                                                                                        \
                );                                                                                                     \
            }                                                                                                          \
            if ((start) >= (v)->length) {                                                                              \
                LOG_FATAL(                                                                                             \
                    "Vector range overflow: Start index %zu exceeds or equals vector length %zu.",                     \
                    (start),                                                                                           \
                    (v)->length                                                                                        \
                );                                                                                                     \
            }                                                                                                          \
            if ((start) > (end)) {                                                                                     \
                LOG_FATAL(                                                                                             \
                    "Invalid range: Start index %zu must be less than or equal to end index %zu.",                     \
                    (start),                                                                                           \
                    (end)                                                                                              \
                );                                                                                                     \
            }                                                                                                          \
            for ((idx) = (start); (idx) < (end); ++(idx)) {                                                            \
                if ((idx) >= (v)->length) {                                                                            \
                    LOG_FATAL(                                                                                         \
                        "Vector range overflow: Index %zu exceeds vector length %zu during iteration.",                \
                        (idx),                                                                                         \
                        (v)->length                                                                                    \
                    );                                                                                                 \
                }                                                                                                      \
                var = VecAt(v, idx);                                                                                   \
                { body }                                                                                               \
            }                                                                                                          \
        }                                                                                                              \
    } while (0)

///
/// Iterate over elements in a specific range of the given vector `v`.
/// This is a convenience macro that iterates over a range using an internally managed index.
/// The variable `var` is declared and defined by this macro.
///
/// v[in,out]    : Vector to iterate over.
/// var[in]      : Name of variable to be used which'll contain value of the current element.
/// start[in]    : Starting index (inclusive).
/// end[in]      : Ending index (exclusive).
/// body         : Body of this foreach loop.
///
/// SUCCESS : The `body` is executed for each element of the vector `v` from the
///           `start` index to the `end-1` index.
/// FAILURE : If the vector `v` is NULL, its length is zero, or the range is invalid,
///           the loop body will not be executed. Any failures within the `VecForeachInRangeIdx`
///           macro will result in a fatal log message and program termination.
///
#define VecForeachInRange(v, var, start, end, body)                                                                    \
    VecForeachInRangeIdx((v), (var), (____iter___), (start), (end), {body})

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
/// body         : Body of this foreach loop.
///
/// SUCCESS : The `body` is executed for each element of the vector `v` from the
///           `start` index to the `end-1` index, with `var` pointing to each element.
/// FAILURE : If the vector `v` is NULL, its length is zero, or the range is invalid,
///           the loop body will not be executed. Any access to an invalid index will
///           result in a fatal log message and program termination.
///
#define VecForeachPtrInRangeIdx(v, var, idx, start, end, body)                                                         \
    do {                                                                                                               \
        size idx             = 0;                                                                                      \
        VEC_DATATYPE(v) *var = NULL;                                                                                   \
        if ((v) != NULL && (v)->length > 0) {                                                                          \
            if ((end) > (v)->length) {                                                                                 \
                LOG_FATAL(                                                                                             \
                    "Vector range overflow: End index %zu exceeds vector length %zu. "                                 \
                    "If you intended to iterate over all items, use VecForeach instead.",                              \
                    (end),                                                                                             \
                    (v)->length                                                                                        \
                );                                                                                                     \
            }                                                                                                          \
            if ((start) >= (v)->length) {                                                                              \
                LOG_FATAL(                                                                                             \
                    "Vector range overflow: Start index %zu exceeds or equals vector length %zu.",                     \
                    (start),                                                                                           \
                    (v)->length                                                                                        \
                );                                                                                                     \
            }                                                                                                          \
            if ((start) > (end)) {                                                                                     \
                LOG_FATAL(                                                                                             \
                    "Invalid range: Start index %zu must be less than or equal to end index %zu.",                     \
                    (start),                                                                                           \
                    (end)                                                                                              \
                );                                                                                                     \
            }                                                                                                          \
            for ((idx) = (start); (idx) < (end); ++(idx)) {                                                            \
                if ((idx) >= (v)->length) {                                                                            \
                    LOG_FATAL(                                                                                         \
                        "Vector range overflow: Index %zu exceeds vector length %zu during iteration.",                \
                        (idx),                                                                                         \
                        (v)->length                                                                                    \
                    );                                                                                                 \
                }                                                                                                      \
                var = VecPtrAt(v, idx);                                                                                \
                { body }                                                                                               \
            }                                                                                                          \
        }                                                                                                              \
    } while (0)

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
/// body         : Body of this foreach loop.
///
/// SUCCESS : The `body` is executed for each element of the vector `v` from the
///           `start` index to the `end-1` index, with `var` pointing to each element.
/// FAILURE : If the vector `v` is NULL, its length is zero, or the range is invalid,
///           the loop body will not be executed. Any failures within the `VecForeachPtrInRangeIdx`
///           macro will result in a fatal log message and program termination.
///
#define VecForeachPtrInRange(v, var, start, end, body)                                                                 \
    VecForeachPtrInRangeIdx((v), (var), (____iter___), (start), (end), {body})

#endif // MISRA_STD_CONTAINER_VEC_FOREACH_H
