/// file      : std/container/float/private.h
/// author    : Generated following Misra project patterns
/// This is free and unencumbered software released into the public domain.
///
/// Internal Float helpers used to implement the public generic API.

#ifndef MISRA_STD_CONTAINER_FLOAT_PRIVATE_H
#define MISRA_STD_CONTAINER_FLOAT_PRIVATE_H

#include "Type.h"

#ifdef __cplusplus
extern "C" {
#endif

    Float FloatFromU64(u64 value);
    Float FloatFromI64(i64 value);
    Float FloatFromInt(Int *value);
    Float FloatFromF32(float value);
    Float FloatFromF64(double value);
    int  FloatCompare(Float *lhs, Float *rhs);
    bool FloatAdd(Float *result, Float *a, Float *b);
    bool FloatSub(Float *result, Float *a, Float *b);
    bool FloatMul(Float *result, Float *a, Float *b);
    bool FloatDiv(Float *result, Float *a, Float *b, u64 precision);
    int  FloatCompareInt(Float *lhs, Int *rhs);
    int  FloatCompareU64(Float *lhs, u64 rhs);
    int  FloatCompareI64(Float *lhs, i64 rhs);
    int  FloatCompareF32(Float *lhs, float rhs);
    int  FloatCompareF64(Float *lhs, double rhs);
    bool FloatAddInt(Float *result, Float *a, Int *b);
    bool FloatAddU64(Float *result, Float *a, u64 b);
    bool FloatAddI64(Float *result, Float *a, i64 b);
    bool FloatAddF32(Float *result, Float *a, float b);
    bool FloatAddF64(Float *result, Float *a, double b);
    bool FloatSubInt(Float *result, Float *a, Int *b);
    bool FloatSubU64(Float *result, Float *a, u64 b);
    bool FloatSubI64(Float *result, Float *a, i64 b);
    bool FloatSubF32(Float *result, Float *a, float b);
    bool FloatSubF64(Float *result, Float *a, double b);
    bool FloatMulInt(Float *result, Float *a, Int *b);
    bool FloatMulU64(Float *result, Float *a, u64 b);
    bool FloatMulI64(Float *result, Float *a, i64 b);
    bool FloatMulF32(Float *result, Float *a, float b);
    bool FloatMulF64(Float *result, Float *a, double b);
    bool FloatDivInt(Float *result, Float *a, Int *b, u64 precision);
    bool FloatDivU64(Float *result, Float *a, u64 b, u64 precision);
    bool FloatDivI64(Float *result, Float *a, i64 b, u64 precision);
    bool FloatDivF32(Float *result, Float *a, float b, u64 precision);
    bool FloatDivF64(Float *result, Float *a, double b, u64 precision);

#ifdef __cplusplus
}
#endif

#endif // MISRA_STD_CONTAINER_FLOAT_PRIVATE_H
