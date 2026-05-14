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
    bool IntAdd(Int *result, Int *a, Int *b);
    bool IntSub(Int *result, Int *a, Int *b);
    bool IntMul(Int *result, Int *a, Int *b);
    bool IntPow(Int *result, Int *base, Int *exponent);
    bool IntDiv(Int *result, Int *dividend, Int *divisor);
    bool IntDivExact(Int *result, Int *dividend, Int *divisor);
    bool IntMod(Int *result, Int *dividend, Int *divisor);
    bool IntDivMod(Int *quotient, Int *remainder, Int *dividend, Int *divisor);
    bool IntPowMod(Int *result, Int *base, Int *exponent, Int *modulus);
    int  IntCompareU64(Int *lhs, u64 rhs);
    int  IntCompareI64(Int *lhs, i64 rhs);
    bool IntAddU64(Int *result, Int *value, u64 addend);
    bool IntAddI64(Int *result, Int *value, i64 addend);
    bool IntSubU64(Int *result, Int *value, u64 subtrahend);
    bool IntSubI64(Int *result, Int *value, i64 subtrahend);
    bool IntMulU64(Int *result, Int *value, u64 factor);
    bool IntMulI64(Int *result, Int *value, i64 factor);
    bool IntPowU64(Int *result, Int *base, u64 exponent);
    bool IntPowI64(Int *result, Int *base, i64 exponent);
    bool IntPowU64Mod(Int *result, Int *base, u64 exponent, Int *modulus);
    bool IntPowI64Mod(Int *result, Int *base, i64 exponent, Int *modulus);
    bool IntDivU64(Int *result, Int *dividend, u64 divisor);
    bool IntDivI64(Int *result, Int *dividend, i64 divisor);
    bool IntDivExactU64(Int *result, Int *dividend, u64 divisor);
    bool IntDivExactI64(Int *result, Int *dividend, i64 divisor);
    bool IntModU64Into(Int *result, Int *dividend, u64 divisor);
    bool IntModI64Into(Int *result, Int *dividend, i64 divisor);
    bool IntDivModU64(Int *quotient, Int *remainder, Int *dividend, u64 divisor);
    bool IntDivModI64(Int *quotient, Int *remainder, Int *dividend, i64 divisor);
    u64  IntDivU64Rem(Int *quotient, Int *dividend, u64 divisor);
    u64  IntModU64(Int *value, u64 modulus);

#ifdef __cplusplus
}
#endif

#endif // MISRA_STD_CONTAINER_INT_PRIVATE_H
