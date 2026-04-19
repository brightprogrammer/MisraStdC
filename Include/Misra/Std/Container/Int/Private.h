/// file      : std/container/int/private.h
/// author    : Generated following Misra project patterns
/// This is free and unencumbered software released into the public domain.
///
/// Internal Int helpers used to implement the public generic API.

#ifndef MISRA_STD_CONTAINER_INT_PRIVATE_H
#define MISRA_STD_CONTAINER_INT_PRIVATE_H

#include "Type.h"

#ifdef __cplusplus
extern "C" {
#endif

    Int  MISRA_PRIV_IntFromU64(u64 value);
    Int  MISRA_PRIV_IntFromI64(i64 value);
    int  IntCompare(Int *lhs, Int *rhs);
    void IntAdd(Int *result, Int *a, Int *b);
    bool IntSub(Int *result, Int *a, Int *b);
    void IntMul(Int *result, Int *a, Int *b);
    void IntPow(Int *result, Int *base, Int *exponent);
    void IntDiv(Int *result, Int *dividend, Int *divisor);
    bool IntDivExact(Int *result, Int *dividend, Int *divisor);
    void IntMod(Int *result, Int *dividend, Int *divisor);
    void IntDivMod(Int *quotient, Int *remainder, Int *dividend, Int *divisor);
    void IntPowMod(Int *result, Int *base, Int *exponent, Int *modulus);
    int  MISRA_PRIV_IntCompareU64(Int *lhs, u64 rhs);
    int  MISRA_PRIV_IntCompareI64(Int *lhs, i64 rhs);
    void MISRA_PRIV_IntAddU64(Int *result, Int *value, u64 addend);
    void MISRA_PRIV_IntAddI64(Int *result, Int *value, i64 addend);
    bool MISRA_PRIV_IntSubU64(Int *result, Int *value, u64 subtrahend);
    bool MISRA_PRIV_IntSubI64(Int *result, Int *value, i64 subtrahend);
    void MISRA_PRIV_IntMulU64(Int *result, Int *value, u64 factor);
    void MISRA_PRIV_IntMulI64(Int *result, Int *value, i64 factor);
    void MISRA_PRIV_IntPowU64(Int *result, Int *base, u64 exponent);
    void MISRA_PRIV_IntPowI64(Int *result, Int *base, i64 exponent);
    void MISRA_PRIV_IntPowU64Mod(Int *result, Int *base, u64 exponent, Int *modulus);
    void MISRA_PRIV_IntPowI64Mod(Int *result, Int *base, i64 exponent, Int *modulus);
    void MISRA_PRIV_IntDivU64(Int *result, Int *dividend, u64 divisor);
    void MISRA_PRIV_IntDivI64(Int *result, Int *dividend, i64 divisor);
    bool MISRA_PRIV_IntDivExactU64(Int *result, Int *dividend, u64 divisor);
    bool MISRA_PRIV_IntDivExactI64(Int *result, Int *dividend, i64 divisor);
    void MISRA_PRIV_IntModU64Into(Int *result, Int *dividend, u64 divisor);
    void MISRA_PRIV_IntModI64Into(Int *result, Int *dividend, i64 divisor);
    void MISRA_PRIV_IntDivModU64(Int *quotient, Int *remainder, Int *dividend, u64 divisor);
    void MISRA_PRIV_IntDivModI64(Int *quotient, Int *remainder, Int *dividend, i64 divisor);
    u64  MISRA_PRIV_IntDivU64Rem(Int *quotient, Int *dividend, u64 divisor);
    u64  MISRA_PRIV_IntModU64(Int *value, u64 modulus);

#ifdef __cplusplus
}
#endif

#ifndef __cplusplus
static inline int MISRA_PRIV_IntCompareConst(Int *lhs, const Int *rhs) {
    return IntCompare(lhs, (Int *)rhs);
}

static inline int MISRA_PRIV_IntCompareValue(Int *lhs, Int rhs) {
    return IntCompare(lhs, &rhs);
}

static inline void MISRA_PRIV_IntAddConst(Int *result, Int *a, const Int *b) {
    IntAdd(result, a, (Int *)b);
}

static inline void MISRA_PRIV_IntAddValue(Int *result, Int *a, Int b) {
    IntAdd(result, a, &b);
}

static inline bool MISRA_PRIV_IntSubConst(Int *result, Int *a, const Int *b) {
    return IntSub(result, a, (Int *)b);
}

static inline bool MISRA_PRIV_IntSubValue(Int *result, Int *a, Int b) {
    return IntSub(result, a, &b);
}

static inline void MISRA_PRIV_IntMulConst(Int *result, Int *a, const Int *b) {
    IntMul(result, a, (Int *)b);
}

static inline void MISRA_PRIV_IntMulValue(Int *result, Int *a, Int b) {
    IntMul(result, a, &b);
}

static inline void MISRA_PRIV_IntPowConst(Int *result, Int *base, const Int *exponent) {
    IntPow(result, base, (Int *)exponent);
}

static inline void MISRA_PRIV_IntPowValue(Int *result, Int *base, Int exponent) {
    IntPow(result, base, &exponent);
}

static inline void MISRA_PRIV_IntDivConst(Int *result, Int *dividend, const Int *divisor) {
    IntDiv(result, dividend, (Int *)divisor);
}

static inline void MISRA_PRIV_IntDivValue(Int *result, Int *dividend, Int divisor) {
    IntDiv(result, dividend, &divisor);
}

static inline bool MISRA_PRIV_IntDivExactConst(Int *result, Int *dividend, const Int *divisor) {
    return IntDivExact(result, dividend, (Int *)divisor);
}

static inline bool MISRA_PRIV_IntDivExactValue(Int *result, Int *dividend, Int divisor) {
    return IntDivExact(result, dividend, &divisor);
}

static inline void MISRA_PRIV_IntModConst(Int *result, Int *dividend, const Int *divisor) {
    IntMod(result, dividend, (Int *)divisor);
}

static inline void MISRA_PRIV_IntModValue(Int *result, Int *dividend, Int divisor) {
    IntMod(result, dividend, &divisor);
}

static inline void MISRA_PRIV_IntDivModConst(
    Int *quotient, Int *remainder, Int *dividend, const Int *divisor
) {
    IntDivMod(quotient, remainder, dividend, (Int *)divisor);
}

static inline void MISRA_PRIV_IntDivModValue(Int *quotient, Int *remainder, Int *dividend, Int divisor) {
    IntDivMod(quotient, remainder, dividend, &divisor);
}

static inline void MISRA_PRIV_IntPowModConst(
    Int *result, Int *base, const Int *exponent, Int *modulus
) {
    IntPowMod(result, base, (Int *)exponent, modulus);
}

static inline void MISRA_PRIV_IntPowModValue(Int *result, Int *base, Int exponent, Int *modulus) {
    IntPowMod(result, base, &exponent, modulus);
}
#endif

#endif // MISRA_STD_CONTAINER_INT_PRIVATE_H
