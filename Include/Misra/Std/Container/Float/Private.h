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

    Float MISRA_PRIV_FloatFromU64(u64 value);
    Float MISRA_PRIV_FloatFromI64(i64 value);
    Float MISRA_PRIV_FloatFromInt(Int *value);
    Float MISRA_PRIV_FloatFromF32(float value);
    Float MISRA_PRIV_FloatFromF64(double value);
    int  FloatCompare(Float *lhs, Float *rhs);
    void FloatAdd(Float *result, Float *a, Float *b);
    void FloatSub(Float *result, Float *a, Float *b);
    void FloatMul(Float *result, Float *a, Float *b);
    void FloatDiv(Float *result, Float *a, Float *b, u64 precision);
    int  MISRA_PRIV_FloatCompareInt(Float *lhs, Int *rhs);
    int  MISRA_PRIV_FloatCompareU64(Float *lhs, u64 rhs);
    int  MISRA_PRIV_FloatCompareI64(Float *lhs, i64 rhs);
    int  MISRA_PRIV_FloatCompareF32(Float *lhs, float rhs);
    int  MISRA_PRIV_FloatCompareF64(Float *lhs, double rhs);
    void MISRA_PRIV_FloatAddInt(Float *result, Float *a, Int *b);
    void MISRA_PRIV_FloatAddU64(Float *result, Float *a, u64 b);
    void MISRA_PRIV_FloatAddI64(Float *result, Float *a, i64 b);
    void MISRA_PRIV_FloatAddF32(Float *result, Float *a, float b);
    void MISRA_PRIV_FloatAddF64(Float *result, Float *a, double b);
    void MISRA_PRIV_FloatSubInt(Float *result, Float *a, Int *b);
    void MISRA_PRIV_FloatSubU64(Float *result, Float *a, u64 b);
    void MISRA_PRIV_FloatSubI64(Float *result, Float *a, i64 b);
    void MISRA_PRIV_FloatSubF32(Float *result, Float *a, float b);
    void MISRA_PRIV_FloatSubF64(Float *result, Float *a, double b);
    void MISRA_PRIV_FloatMulInt(Float *result, Float *a, Int *b);
    void MISRA_PRIV_FloatMulU64(Float *result, Float *a, u64 b);
    void MISRA_PRIV_FloatMulI64(Float *result, Float *a, i64 b);
    void MISRA_PRIV_FloatMulF32(Float *result, Float *a, float b);
    void MISRA_PRIV_FloatMulF64(Float *result, Float *a, double b);
    void MISRA_PRIV_FloatDivInt(Float *result, Float *a, Int *b, u64 precision);
    void MISRA_PRIV_FloatDivU64(Float *result, Float *a, u64 b, u64 precision);
    void MISRA_PRIV_FloatDivI64(Float *result, Float *a, i64 b, u64 precision);
    void MISRA_PRIV_FloatDivF32(Float *result, Float *a, float b, u64 precision);
    void MISRA_PRIV_FloatDivF64(Float *result, Float *a, double b, u64 precision);

#ifdef __cplusplus
}
#endif

#ifndef __cplusplus
static inline int MISRA_PRIV_FloatCompareConstFloat(Float *lhs, const Float *rhs) {
    return FloatCompare(lhs, (Float *)rhs);
}

static inline int MISRA_PRIV_FloatCompareValueFloat(Float *lhs, Float rhs) {
    return FloatCompare(lhs, &rhs);
}

static inline int MISRA_PRIV_FloatCompareConstInt(Float *lhs, const Int *rhs) {
    return MISRA_PRIV_FloatCompareInt(lhs, (Int *)rhs);
}

static inline int MISRA_PRIV_FloatCompareValueInt(Float *lhs, Int rhs) {
    return MISRA_PRIV_FloatCompareInt(lhs, &rhs);
}

static inline void MISRA_PRIV_FloatAddConstFloat(Float *result, Float *a, const Float *b) {
    FloatAdd(result, a, (Float *)b);
}

static inline void MISRA_PRIV_FloatAddValueFloat(Float *result, Float *a, Float b) {
    FloatAdd(result, a, &b);
}

static inline void MISRA_PRIV_FloatAddConstInt(Float *result, Float *a, const Int *b) {
    MISRA_PRIV_FloatAddInt(result, a, (Int *)b);
}

static inline void MISRA_PRIV_FloatAddValueInt(Float *result, Float *a, Int b) {
    MISRA_PRIV_FloatAddInt(result, a, &b);
}

static inline void MISRA_PRIV_FloatSubConstFloat(Float *result, Float *a, const Float *b) {
    FloatSub(result, a, (Float *)b);
}

static inline void MISRA_PRIV_FloatSubValueFloat(Float *result, Float *a, Float b) {
    FloatSub(result, a, &b);
}

static inline void MISRA_PRIV_FloatSubConstInt(Float *result, Float *a, const Int *b) {
    MISRA_PRIV_FloatSubInt(result, a, (Int *)b);
}

static inline void MISRA_PRIV_FloatSubValueInt(Float *result, Float *a, Int b) {
    MISRA_PRIV_FloatSubInt(result, a, &b);
}

static inline void MISRA_PRIV_FloatMulConstFloat(Float *result, Float *a, const Float *b) {
    FloatMul(result, a, (Float *)b);
}

static inline void MISRA_PRIV_FloatMulValueFloat(Float *result, Float *a, Float b) {
    FloatMul(result, a, &b);
}

static inline void MISRA_PRIV_FloatMulConstInt(Float *result, Float *a, const Int *b) {
    MISRA_PRIV_FloatMulInt(result, a, (Int *)b);
}

static inline void MISRA_PRIV_FloatMulValueInt(Float *result, Float *a, Int b) {
    MISRA_PRIV_FloatMulInt(result, a, &b);
}

static inline void MISRA_PRIV_FloatDivConstFloat(Float *result, Float *a, const Float *b, u64 precision) {
    FloatDiv(result, a, (Float *)b, precision);
}

static inline void MISRA_PRIV_FloatDivValueFloat(Float *result, Float *a, Float b, u64 precision) {
    FloatDiv(result, a, &b, precision);
}

static inline void MISRA_PRIV_FloatDivConstInt(Float *result, Float *a, const Int *b, u64 precision) {
    MISRA_PRIV_FloatDivInt(result, a, (Int *)b, precision);
}

static inline void MISRA_PRIV_FloatDivValueInt(Float *result, Float *a, Int b, u64 precision) {
    MISRA_PRIV_FloatDivInt(result, a, &b, precision);
}

static inline Float MISRA_PRIV_FloatFromValueInt(Int value) {
    return MISRA_PRIV_FloatFromInt(&value);
}

static inline Float MISRA_PRIV_FloatFromConstInt(const Int *value) {
    return MISRA_PRIV_FloatFromInt((Int *)value);
}
#endif

#endif // MISRA_STD_CONTAINER_FLOAT_PRIVATE_H
