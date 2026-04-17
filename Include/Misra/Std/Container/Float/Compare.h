/// file      : std/container/float/compare.h
/// author    : Generated following Misra project patterns
/// This is free and unencumbered software released into the public domain.
///
/// Comparison helpers for Float.

#ifndef MISRA_STD_CONTAINER_FLOAT_COMPARE_H
#define MISRA_STD_CONTAINER_FLOAT_COMPARE_H

#include "Type.h"

#ifdef __cplusplus
extern "C" {
#endif

int  FloatCompare(Float *lhs, Float *rhs);
int  FloatCompareInt(Float *lhs, Int *rhs);
int  FloatCompareU64(Float *lhs, u64 rhs);
int  FloatCompareI64(Float *lhs, i64 rhs);
int  FloatCompareF32(Float *lhs, float rhs);
int  FloatCompareF64(Float *lhs, double rhs);

static inline int FloatCompareConstFloat(Float *lhs, const Float *rhs) {
    return FloatCompare(lhs, (Float *)rhs);
}

static inline int FloatCompareValueFloat(Float *lhs, Float rhs) {
    return FloatCompare(lhs, &rhs);
}

static inline int FloatCompareConstInt(Float *lhs, const Int *rhs) {
    return FloatCompareInt(lhs, (Int *)rhs);
}

static inline int FloatCompareValueInt(Float *lhs, Int rhs) {
    return FloatCompareInt(lhs, &rhs);
}

#ifndef __cplusplus
#    define MISRA_FLOAT_COMPARE_DISPATCH(rhs)                                                                          \
        _Generic(                                                                                                      \
            (rhs),                                                                                                     \
            Float: FloatCompareValueFloat,                                                                             \
            Float *: FloatCompare,                                                                                     \
            const Float *: FloatCompareConstFloat,                                                                     \
            Int: FloatCompareValueInt,                                                                                 \
            Int *: FloatCompareInt,                                                                                    \
            const Int *: FloatCompareConstInt,                                                                         \
            unsigned char: FloatCompareU64,                                                                            \
            unsigned short: FloatCompareU64,                                                                           \
            unsigned int: FloatCompareU64,                                                                             \
            unsigned long: FloatCompareU64,                                                                            \
            unsigned long long: FloatCompareU64,                                                                       \
            signed char: FloatCompareI64,                                                                              \
            signed short: FloatCompareI64,                                                                             \
            signed int: FloatCompareI64,                                                                               \
            signed long: FloatCompareI64,                                                                              \
            signed long long: FloatCompareI64,                                                                         \
            float: FloatCompareF32,                                                                                    \
            double: FloatCompareF64                                                                                    \
        )

#    define FloatEQ(lhs, rhs) (MISRA_FLOAT_COMPARE_DISPATCH(rhs)((lhs), (rhs)) == 0)
#    define FloatLT(lhs, rhs) (MISRA_FLOAT_COMPARE_DISPATCH(rhs)((lhs), (rhs)) < 0)
#    define FloatLE(lhs, rhs) (MISRA_FLOAT_COMPARE_DISPATCH(rhs)((lhs), (rhs)) <= 0)
#    define FloatGT(lhs, rhs) (MISRA_FLOAT_COMPARE_DISPATCH(rhs)((lhs), (rhs)) > 0)
#    define FloatGE(lhs, rhs) (MISRA_FLOAT_COMPARE_DISPATCH(rhs)((lhs), (rhs)) >= 0)
#    define FloatNE(lhs, rhs) (MISRA_FLOAT_COMPARE_DISPATCH(rhs)((lhs), (rhs)) != 0)
#endif

#ifdef __cplusplus
}
#endif

#endif // MISRA_STD_CONTAINER_FLOAT_COMPARE_H
