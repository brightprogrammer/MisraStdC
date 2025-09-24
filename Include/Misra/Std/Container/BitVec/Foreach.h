/// file      : std/container/bitvec/foreach.h
/// author    : Generated following Misra project patterns
/// This is free and unencumbered software released into the public domain.
///
/// Iteration and foreach operations for bitvectors.

#ifndef MISRA_STD_CONTAINER_BITVEC_FOREACH_H
#define MISRA_STD_CONTAINER_BITVEC_FOREACH_H

#include "Type.h"
#include "Access.h"

///
/// Iterate over each bit `var` of given bitvector `bv` at each index `idx` into the bitvector.
/// The variables `var` and `idx` declared and defined by this macro.
///
/// `idx` will start from 0 and will go till bv->length - 1
///
/// bv[in,out] : Bitvector to iterate over.
/// var[in]    : Name of variable to be used which'll contain bit value at iterated index `idx`
/// idx[in]    : Name of variable to be used for iterating over indices.
///
#define BitVecForeachIdx(bv, var, idx)                                                                                 \
    for (TYPE_OF(bv) UNPL(pbv) = (bv); UNPL(pbv); UNPL(pbv) = NULL)                                                    \
        if ((ValidateBitVec(UNPL(pbv)), 1) && UNPL(pbv)->length > 0)                                                   \
            for (u64 idx = 0, UNPL(d) = 1; UNPL(d); UNPL(d)--)                                                         \
                for (bool var = 0; idx < UNPL(pbv)->length && (var = BitVecGet(UNPL(pbv), idx), 1); idx++)

///
/// Iterate over each bit `var` of given bitvector `bv` at each index `idx` into the bitvector.
/// The variables `var` and `idx` declared and defined by this macro.
///
/// `idx` will start from bv->length - 1 and will go till 0
///
/// bv[in,out] : Bitvector to iterate over.
/// var[in]    : Name of variable to be used which'll contain bit value at iterated index `idx`
/// idx[in]    : Name of variable to be used for iterating over indices.
///
#define BitVecForeachReverseIdx(bv, var, idx)                                                                          \
    for (TYPE_OF(bv) UNPL(pbv) = (bv); UNPL(pbv); UNPL(pbv) = NULL)                                                    \
        if ((ValidateBitVec(UNPL(pbv)), 1) && UNPL(pbv)->length > 0)                                                   \
            for (u64 idx = UNPL(pbv)->length; idx-- > 0 && idx < UNPL(pbv)->length;)                                   \
                for (u8 UNPL(run_once) = 1; UNPL(run_once); UNPL(run_once) = 0)                                        \
                    for (bool var = BitVecGet(UNPL(pbv), idx); UNPL(run_once); UNPL(run_once) = 0)

///
/// Iterate over each bit `var` of the given bitvector `bv`.
/// This is a convenience macro that iterates forward using an internally managed index.
/// The variable `var` is declared and defined by this macro.
///
/// bv[in,out] : Bitvector to iterate over.
/// var[in]    : Name of the variable to be used which will contain the value of the
///              current bit during iteration. The type of `var` will be `bool`.
///
#define BitVecForeach(bv, var) BitVecForeachIdx((bv), (var), UNPL(iter))

///
/// Iterate over each bit `var` of the given bitvector `bv` in reverse order.
/// This is a convenience macro that iterates backward using an internally managed index.
/// The variable `var` is declared and defined by this macro.
///
/// bv[in,out] : Bitvector to iterate over.
/// var[in]    : Name of the variable to be used which will contain the value of the
///              current bit during iteration. The type of `var` will be `bool`.
///
#define BitVecForeachReverse(bv, var) BitVecForeachReverseIdx((bv), (var), UNPL(iter))

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
///
#define BitVecForeachInRangeIdx(bv, var, idx, start, end)                                                              \
    for (TYPE_OF(bv) UNPL(pbv) = (bv); UNPL(pbv); UNPL(pbv) = NULL)                                                    \
        if ((ValidateBitVec(UNPL(pbv)), 1) && UNPL(pbv)->length > 0)                                                   \
            for (u64 UNPL(s) = (start), UNPL(e) = (end), idx = UNPL(s), UNPL(d) = 1;                                   \
                 UNPL(s) <= idx && idx < UNPL(e) && idx < UNPL(pbv)->length && UNPL(s) <= UNPL(e);                     \
                 ++idx, UNPL(d) = 1)                                                                                   \
                for (bool var = BitVecGet(UNPL(pbv), idx); UNPL(d); UNPL(d) = 0)


///
/// Iterate over bits in a specific range of the given bitvector `bv`.
/// This is a convenience macro that iterates over a range using an internally managed index.
/// The variable `var` is declared and defined by this macro.
///
/// bv[in,out]   : Bitvector to iterate over.
/// var[in]      : Name of variable to be used which'll contain bit value of the current bit.
/// start[in]    : Starting index (inclusive).
/// end[in]      : Ending index (exclusive).
///
#define BitVecForeachInRange(bv, var, start, end) BitVecForeachInRangeIdx((bv), (var), UNPL(iter), (start), (end))

#endif // MISRA_STD_CONTAINER_BITVEC_FOREACH_H
