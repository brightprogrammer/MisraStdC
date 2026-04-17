/// file      : std/container/int/compare.h
/// author    : Generated following Misra project patterns
/// This is free and unencumbered software released into the public domain.
///
/// Comparison helpers for Int.

#ifndef MISRA_STD_CONTAINER_INT_COMPARE_H
#define MISRA_STD_CONTAINER_INT_COMPARE_H

#include "Type.h"

#ifdef __cplusplus
extern "C" {
#endif

int  IntCompare(Int *lhs, Int *rhs);
int  IntCompareU64(Int *lhs, u64 rhs);
int  IntCompareI64(Int *lhs, i64 rhs);

static inline int IntCompareConst(Int *lhs, const Int *rhs) {
    return IntCompare(lhs, (Int *)rhs);
}

static inline int IntCompareValue(Int *lhs, Int rhs) {
    return IntCompare(lhs, &rhs);
}

#ifndef __cplusplus
#    define MISRA_INT_COMPARE_DISPATCH(rhs)                                                                            \
        _Generic(                                                                                                      \
            (rhs),                                                                                                     \
            Int: IntCompareValue,                                                                                      \
            Int *: IntCompare,                                                                                         \
            const Int *: IntCompareConst,                                                                              \
            unsigned char: IntCompareU64,                                                                              \
            unsigned short: IntCompareU64,                                                                             \
            unsigned int: IntCompareU64,                                                                               \
            unsigned long: IntCompareU64,                                                                              \
            unsigned long long: IntCompareU64,                                                                         \
            signed char: IntCompareI64,                                                                                \
            signed short: IntCompareI64,                                                                               \
            signed int: IntCompareI64,                                                                                 \
            signed long: IntCompareI64,                                                                                \
            signed long long: IntCompareI64                                                                            \
        )

#    define IntEQ(lhs, rhs) (MISRA_INT_COMPARE_DISPATCH(rhs)((lhs), (rhs)) == 0)
#    define IntLT(lhs, rhs) (MISRA_INT_COMPARE_DISPATCH(rhs)((lhs), (rhs)) < 0)
#    define IntLE(lhs, rhs) (MISRA_INT_COMPARE_DISPATCH(rhs)((lhs), (rhs)) <= 0)
#    define IntGT(lhs, rhs) (MISRA_INT_COMPARE_DISPATCH(rhs)((lhs), (rhs)) > 0)
#    define IntGE(lhs, rhs) (MISRA_INT_COMPARE_DISPATCH(rhs)((lhs), (rhs)) >= 0)
#    define IntNE(lhs, rhs) (MISRA_INT_COMPARE_DISPATCH(rhs)((lhs), (rhs)) != 0)
#endif

#ifdef __cplusplus
}
#endif

#endif // MISRA_STD_CONTAINER_INT_COMPARE_H
