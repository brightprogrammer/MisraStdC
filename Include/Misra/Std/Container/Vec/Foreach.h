/// file      : std/container/vec/access.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)

#ifndef MISRA_STD_CONTAINER_VEC_FOREACH_H
#define MISRA_STD_CONTAINER_VEC_FOREACH_H

// clang-format off
#include "Type.h"
#include "Private.h"
// clang-format on

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
#define VecForeachIdx(v, var, idx, body)                                                                               \
    do {                                                                                                               \
        i64 idx             = 0;                                                                                       \
        VEC_DATATYPE(v) var = {0};                                                                                     \
        if ((v) && (v)->length) {                                                                                      \
            for ((idx) = 0; (idx) < (v)->length; ++(idx)) {                                                            \
                if ((idx) < 0) {                                                                                       \
                    LOG_FATAL(                                                                                         \
                        "Vector range underflow : Invalid index reached "                                              \
                        "during Foreach iteration."                                                                    \
                    );                                                                                                 \
                }                                                                                                      \
                var = VecAt(v, idx);                                                                                   \
                { body }                                                                                               \
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
        i64 idx             = 0;                                                                                       \
        VEC_DATATYPE(v) var = {0};                                                                                     \
        if ((v) && (v)->length) {                                                                                      \
            for ((idx) = (v)->length - 1; (idx) >= 0; --(idx)) {                                                       \
                if ((idx) < 0) {                                                                                       \
                    LOG_FATAL(                                                                                         \
                        "Vector range underflow : Invalid index reached "                                              \
                        "during Foreach reverse "                                                                      \
                        "iteration."                                                                                   \
                    );                                                                                                 \
                }                                                                                                      \
                if ((idx) >= (v)->length) {                                                                            \
                    LOG_FATAL(                                                                                         \
                        "Vector range overflow : Invalid index reached "                                               \
                        "during Foreach reverse "                                                                      \
                        "iteration."                                                                                   \
                    );                                                                                                 \
                }                                                                                                      \
                var = VecAt(v, idx);                                                                                   \
                { body }                                                                                               \
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
        i64 idx              = 0;                                                                                      \
        VEC_DATATYPE(v) *var = {0};                                                                                    \
        if ((v) && (v)->length) {                                                                                      \
            for ((idx) = 0; (idx) < (v)->length; ++(idx)) {                                                            \
                if ((idx) < 0) {                                                                                       \
                    LOG_FATAL(                                                                                         \
                        "Vector range underflow : Invalid index reached "                                              \
                        "during Foreach iteration."                                                                    \
                    );                                                                                                 \
                }                                                                                                      \
                var = VecPtrAt(v, idx);                                                                                \
                { body }                                                                                               \
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
        i64 idx              = 0;                                                                                      \
        VEC_DATATYPE(v) *var = {0};                                                                                    \
        if ((v) && (v)->length) {                                                                                      \
            for ((idx) = (v)->length - 1; (idx) >= 0; --(idx)) {                                                       \
                if ((idx) < 0) {                                                                                       \
                    LOG_FATAL(                                                                                         \
                        "Vector range underflow : Invalid index reached "                                              \
                        "during Foreach reverse "                                                                      \
                        "iteration."                                                                                   \
                    );                                                                                                 \
                }                                                                                                      \
                if ((idx) >= (v)->length) {                                                                            \
                    LOG_FATAL(                                                                                         \
                        "Vector range overflow : Invalid index reached "                                               \
                        "during Foreach reverse "                                                                      \
                        "iteration."                                                                                   \
                    );                                                                                                 \
                }                                                                                                      \
                var = VecPtrAt(v, idx);                                                                                \
                { body }                                                                                               \
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
/// Iterate over each element `var` (as a pointer) of the given vector `v`.
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

#endif // MISRA_STD_CONTAINER_VEC_FOREACH_H
