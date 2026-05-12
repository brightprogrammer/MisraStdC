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

    Int  IntFromU64(u64 value);
    Int  IntFromI64(i64 value);
    int  IntCompare(Int *lhs, Int *rhs);
    void IntAdd(Int *result, Int *a, Int *b);
    bool IntSub(Int *result, Int *a, Int *b);
    void IntMul(Int *result, Int *a, Int *b);
    bool IntPow(Int *result, Int *base, Int *exponent);
    bool IntDiv(Int *result, Int *dividend, Int *divisor);
    bool IntDivExact(Int *result, Int *dividend, Int *divisor);
    bool IntMod(Int *result, Int *dividend, Int *divisor);
    bool IntDivMod(Int *quotient, Int *remainder, Int *dividend, Int *divisor);
    bool IntPowMod(Int *result, Int *base, Int *exponent, Int *modulus);
    int  IntCompareU64(Int *lhs, u64 rhs);
    int  IntCompareI64(Int *lhs, i64 rhs);
    void IntAddU64(Int *result, Int *value, u64 addend);
    void IntAddI64(Int *result, Int *value, i64 addend);
    bool IntSubU64(Int *result, Int *value, u64 subtrahend);
    bool IntSubI64(Int *result, Int *value, i64 subtrahend);
    void IntMulU64(Int *result, Int *value, u64 factor);
    void IntMulI64(Int *result, Int *value, i64 factor);
    void IntPowU64(Int *result, Int *base, u64 exponent);
    void IntPowI64(Int *result, Int *base, i64 exponent);
    void IntPowU64Mod(Int *result, Int *base, u64 exponent, Int *modulus);
    void IntPowI64Mod(Int *result, Int *base, i64 exponent, Int *modulus);
    void IntDivU64(Int *result, Int *dividend, u64 divisor);
    void IntDivI64(Int *result, Int *dividend, i64 divisor);
    bool IntDivExactU64(Int *result, Int *dividend, u64 divisor);
    bool IntDivExactI64(Int *result, Int *dividend, i64 divisor);
    void IntModU64Into(Int *result, Int *dividend, u64 divisor);
    void IntModI64Into(Int *result, Int *dividend, i64 divisor);
    void IntDivModU64(Int *quotient, Int *remainder, Int *dividend, u64 divisor);
    void IntDivModI64(Int *quotient, Int *remainder, Int *dividend, i64 divisor);
    u64  MISRA_PRIV_IntDivU64Rem(Int *quotient, Int *dividend, u64 divisor);
    u64  MISRA_PRIV_IntModU64(Int *value, u64 modulus);

#ifdef __cplusplus
}
#endif

#endif // MISRA_STD_CONTAINER_INT_PRIVATE_H
