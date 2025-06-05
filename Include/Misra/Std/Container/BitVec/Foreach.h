/// file      : std/container/bitvec/foreach.h
/// author    : Generated following Misra project patterns
/// This is free and unencumbered software released into the public domain.
///
/// Iteration and foreach operations for bitvectors.

#ifndef MISRA_STD_CONTAINER_BITVEC_FOREACH_H
#define MISRA_STD_CONTAINER_BITVEC_FOREACH_H

#include "Type.h"
#include <Misra/Types.h>
#include "Access.h"
#include <Misra/Std/Log.h>

///
/// Iterate over each bit `var` of given bitvector `bv` at each index `idx` into the bitvector.
/// The variables `var` and `idx` declared and defined by this macro.
///
/// `idx` will start from 0 and will go till bv->length - 1
///
/// bv[in,out] : Bitvector to iterate over.
/// var[in]    : Name of variable to be used which'll contain bit value at iterated index `idx`
/// idx[in]    : Name of variable to be used for iterating over indices.
/// body       : Body of this foreach loop
///
/// SUCCESS : The `body` is executed for each bit of the bitvector `bv` from the
///           beginning to the end.
/// FAILURE : If the bitvector `bv` is NULL or its length is zero, the loop body will not
///           be executed. Any failures within the `BitVecForeachIdx` macro (like invalid
///           index access) will result in a fatal log message and program termination.
///
#define BitVecForeachIdx(bv, var, idx, body)                                                                           \
    do {                                                                                                               \
        ValidateBitVec(bv);                                                                                            \
        u64 idx = 0;                                                                                                   \
        if ((bv)->length > 0) {                                                                                        \
            for ((idx) = 0; (idx) < (bv)->length; ++(idx)) {                                                           \
                bool var = BitVecGet(bv, idx);                                                                         \
                { body }                                                                                               \
                if ((idx) >= (bv)->length) {                                                                           \
                    LOG_FATAL("BitVec range overflow : Invalid index reached during Foreach iteration.");              \
                }                                                                                                      \
            }                                                                                                          \
        }                                                                                                              \
    } while (0)

///
/// Iterate over each bit `var` of given bitvector `bv` at each index `idx` into the bitvector.
/// The variables `var` and `idx` declared and defined by this macro.
///
/// `idx` will start from bv->length - 1 and will go till 0
///
/// bv[in,out] : Bitvector to iterate over.
/// var[in]    : Name of variable to be used which'll contain bit value at iterated index `idx`
/// idx[in]    : Name of variable to be used for iterating over indices.
/// body       : Body of this foreach loop
///
#define BitVecForeachReverseIdx(bv, var, idx, body)                                                                    \
    do {                                                                                                               \
        ValidateBitVec(bv);                                                                                            \
        u64 idx = 0;                                                                                                   \
        if ((bv)->length > 0) {                                                                                        \
            for (idx = (bv)->length - 1; (idx) < (bv)->length; --(idx)) {                                              \
                bool var = BitVecGet(bv, idx);                                                                         \
                { body }                                                                                               \
                if ((idx) >= (bv)->length) {                                                                           \
                    LOG_FATAL("BitVec range overflow : Invalid index reached during Foreach reverse iteration.");      \
                }                                                                                                      \
                if (idx == 0)                                                                                          \
                    break; /* Stop after processing index 0 */                                                         \
            }                                                                                                          \
        }                                                                                                              \
    } while (0)

///
/// Iterate over each bit `var` of the given bitvector `bv`.
/// This is a convenience macro that iterates forward using an internally managed index.
/// The variable `var` is declared and defined by this macro.
///
/// bv[in,out] : Bitvector to iterate over.
/// var[in]    : Name of the variable to be used which will contain the value of the
///              current bit during iteration. The type of `var` will be `bool`.
/// body       : The block of code to be executed for each bit of the bitvector.
///
/// SUCCESS : The `body` is executed for each bit of the bitvector `bv` from the
///           beginning to the end.
/// FAILURE : If the bitvector `bv` is NULL or its length is zero, the loop body will not
///           be executed. Any failures within the `BitVecForeachIdx` macro (like invalid
///           index access) will result in a fatal log message and program termination.
///
#define BitVecForeach(bv, var, body) BitVecForeachIdx((bv), var, ____iter___, {body})

///
/// Iterate over each bit `var` of the given bitvector `bv` in reverse order.
/// This is a convenience macro that iterates backward using an internally managed index.
/// The variable `var` is declared and defined by this macro.
///
/// bv[in,out] : Bitvector to iterate over.
/// var[in]    : Name of the variable to be used which will contain the value of the
///              current bit during iteration. The type of `var` will be `bool`.
/// body       : The block of code to be executed for each bit of the bitvector.
///
/// SUCCESS : The `body` is executed for each bit of the bitvector `bv` from the
///           end to the beginning.
/// FAILURE : If the bitvector `bv` is NULL or its length is zero, the loop body will not
///           be executed. Any failures within the `BitVecForeachReverseIdx` macro (like
///           invalid index access) will result in a fatal log message and program termination.
///
#define BitVecForeachReverse(bv, var, body) BitVecForeachReverseIdx((bv), (var), (____iter___), {body})

///
/// Iterate over bits in a specific range of the given bitvector `bv` at each index `idx`.
/// The variables `var` and `idx` are declared and defined by this macro.
///
/// `idx` will start from `start` and will go till `end - 1`
///
/// bv[in,out]   : Bitvector to iterate over.
/// var[in]      : Name of variable to be used which'll contain bit value at iterated index `idx`.
/// idx[in]      : Name of variable to be used for iterating over indices.
/// start[in]    : Starting index (inclusive).
/// end[in]      : Ending index (exclusive).
/// body         : Body of this foreach loop.
///
/// SUCCESS : The `body` is executed for each bit of the bitvector `bv` from the
///           `start` index to the `end-1` index.
/// FAILURE : If the bitvector `bv` is NULL, its length is zero, or the range is invalid,
///           the loop body will not be executed. Any access to an invalid index will
///           result in a fatal log message and program termination.
///
#define BitVecForeachInRangeIdx(bv, var, idx, start, end, body)                                                        \
    do {                                                                                                               \
        ValidateBitVec(bv);                                                                                            \
        u64 idx = 0;                                                                                                   \
        if ((bv)->length > 0) {                                                                                        \
            if ((end) > (bv)->length) {                                                                                \
                LOG_FATAL(                                                                                             \
                    "BitVec range overflow: End index %zu exceeds bitvector length %zu. "                              \
                    "If you intended to iterate over all bits, use BitVecForeach instead.",                            \
                    (end),                                                                                             \
                    (bv)->length                                                                                       \
                );                                                                                                     \
            }                                                                                                          \
            if ((start) >= (bv)->length) {                                                                             \
                LOG_FATAL(                                                                                             \
                    "BitVec range overflow: Start index %zu exceeds or equals bitvector length %zu.",                  \
                    (start),                                                                                           \
                    (bv)->length                                                                                       \
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
                if ((idx) >= (bv)->length) {                                                                           \
                    LOG_FATAL(                                                                                         \
                        "BitVec range overflow: Index %zu exceeds bitvector length %zu during iteration.",             \
                        (idx),                                                                                         \
                        (bv)->length                                                                                   \
                    );                                                                                                 \
                }                                                                                                      \
                bool var = BitVecGet(bv, idx);                                                                         \
                { body }                                                                                               \
            }                                                                                                          \
        }                                                                                                              \
    } while (0)

///
/// Iterate over bits in a specific range of the given bitvector `bv`.
/// This is a convenience macro that iterates over a range using an internally managed index.
/// The variable `var` is declared and defined by this macro.
///
/// bv[in,out]   : Bitvector to iterate over.
/// var[in]      : Name of variable to be used which'll contain bit value of the current bit.
/// start[in]    : Starting index (inclusive).
/// end[in]      : Ending index (exclusive).
/// body         : Body of this foreach loop.
///
/// SUCCESS : The `body` is executed for each bit of the bitvector `bv` from the
///           `start` index to the `end-1` index.
/// FAILURE : If the bitvector `bv` is NULL, its length is zero, or the range is invalid,
///           the loop body will not be executed. Any failures within the `BitVecForeachInRangeIdx`
///           macro will result in a fatal log message and program termination.
///
#define BitVecForeachInRange(bv, var, start, end, body)                                                                \
    BitVecForeachInRangeIdx((bv), (var), (____iter___), (start), (end), {body})

#ifdef __cplusplus
extern "C" {
#endif



    ///
    /// Analyze run lengths in a bitvector.
    /// A run is a sequence of consecutive identical bits.
    /// Results array must be pre-allocated with sufficient space.
    ///
    /// bv[in]         : Bitvector to analyze
    /// runs[out]      : Array to store run lengths
    /// values[out]    : Array to store run values (true/false)
    /// max_runs[in]   : Maximum number of runs to store
    ///
    /// RETURNS: Number of runs found
    ///
    /// USAGE:
    ///   u64 run_lengths[50];
    ///   bool run_values[50];
    ///   u64 count = BitVecRunLengths(&flags, run_lengths, run_values, 50);
    ///
    /// TAGS: BitVec, RunLength, Analysis, Pattern
    ///
    u64 BitVecRunLengths(BitVec *bv, u64 *runs, bool *values, u64 max_runs);

#ifdef __cplusplus
}
#endif

#endif // MISRA_STD_CONTAINER_BITVEC_FOREACH_H