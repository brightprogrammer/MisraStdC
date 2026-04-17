/// file      : std/container/int/math.h
/// author    : Generated following Misra project patterns
/// This is free and unencumbered software released into the public domain.
///
/// Arithmetic and number-theoretic helpers for Int.

#ifndef MISRA_STD_CONTAINER_INT_MATH_H
#define MISRA_STD_CONTAINER_INT_MATH_H

#include "Type.h"

#ifdef __cplusplus
extern "C" {
#endif

void IntShiftLeft(Int *value, u64 positions);
void IntShiftRight(Int *value, u64 positions);
void (IntAdd)(Int *result, Int *a, Int *b);
void IntAddU64(Int *result, Int *value, u64 addend);
void IntAddI64(Int *result, Int *value, i64 addend);
bool (IntSub)(Int *result, Int *a, Int *b);
bool IntSubU64(Int *result, Int *value, u64 subtrahend);
bool IntSubI64(Int *result, Int *value, i64 subtrahend);
void IntMul(Int *result, Int *a, Int *b);
void IntMulU64(Int *result, Int *value, u64 factor);
void IntSquare(Int *result, Int *value);
void IntPowU64(Int *result, Int *base, u64 exponent);
void IntDiv(Int *result, Int *dividend, Int *divisor);
bool IntDivExact(Int *result, Int *dividend, Int *divisor);
u64  IntDivU64Rem(Int *quotient, Int *dividend, u64 divisor);
void IntMod(Int *result, Int *dividend, Int *divisor);
u64  IntModU64(Int *value, u64 modulus);
void IntDivMod(Int *quotient, Int *remainder, Int *dividend, Int *divisor);
void IntGCD(Int *result, Int *a, Int *b);
void IntLCM(Int *result, Int *a, Int *b);
void IntRoot(Int *result, Int *value, u64 degree);
void IntRootRem(Int *root, Int *remainder, Int *value, u64 degree);
void IntSqrt(Int *result, Int *value);
void IntSqrtRem(Int *root, Int *remainder, Int *value);
bool IntIsPerfectSquare(Int *value);
bool IntIsPerfectPower(Int *value);
int  IntJacobi(Int *a, Int *n);
void IntSquareMod(Int *result, Int *value, Int *modulus);
void IntModAdd(Int *result, Int *a, Int *b, Int *modulus);
void IntModSub(Int *result, Int *a, Int *b, Int *modulus);
void IntModMul(Int *result, Int *a, Int *b, Int *modulus);
bool IntModDiv(Int *result, Int *a, Int *b, Int *modulus);
void IntPowU64Mod(Int *result, Int *base, u64 exponent, Int *modulus);
void IntModPow(Int *result, Int *base, Int *exponent, Int *modulus);
bool IntModInv(Int *result, Int *value, Int *modulus);
bool IntModSqrt(Int *result, Int *value, Int *modulus);
bool IntIsProbablePrime(Int *value);
void IntNextPrime(Int *result, Int *value);

static inline void IntAddConst(Int *result, Int *a, const Int *b) {
    IntAdd(result, a, (Int *)b);
}

static inline void IntAddValue(Int *result, Int *a, Int b) {
    IntAdd(result, a, &b);
}

static inline bool IntSubConst(Int *result, Int *a, const Int *b) {
    return IntSub(result, a, (Int *)b);
}

static inline bool IntSubValue(Int *result, Int *a, Int b) {
    return IntSub(result, a, &b);
}

#ifndef __cplusplus
#    define MISRA_INT_ADD_DISPATCH(rhs)                                                                                \
        _Generic(                                                                                                      \
            (rhs),                                                                                                     \
            Int: IntAddValue,                                                                                          \
            Int *: IntAdd,                                                                                             \
            const Int *: IntAddConst,                                                                                  \
            unsigned char: IntAddU64,                                                                                  \
            unsigned short: IntAddU64,                                                                                 \
            unsigned int: IntAddU64,                                                                                   \
            unsigned long: IntAddU64,                                                                                  \
            unsigned long long: IntAddU64,                                                                             \
            signed char: IntAddI64,                                                                                    \
            signed short: IntAddI64,                                                                                   \
            signed int: IntAddI64,                                                                                     \
            signed long: IntAddI64,                                                                                    \
            signed long long: IntAddI64                                                                                \
        )

#    define MISRA_INT_SUB_DISPATCH(rhs)                                                                                \
        _Generic(                                                                                                      \
            (rhs),                                                                                                     \
            Int: IntSubValue,                                                                                          \
            Int *: IntSub,                                                                                             \
            const Int *: IntSubConst,                                                                                  \
            unsigned char: IntSubU64,                                                                                  \
            unsigned short: IntSubU64,                                                                                 \
            unsigned int: IntSubU64,                                                                                   \
            unsigned long: IntSubU64,                                                                                  \
            unsigned long long: IntSubU64,                                                                             \
            signed char: IntSubI64,                                                                                    \
            signed short: IntSubI64,                                                                                   \
            signed int: IntSubI64,                                                                                     \
            signed long: IntSubI64,                                                                                    \
            signed long long: IntSubI64                                                                                \
        )

#    define IntAdd(result, a, b) MISRA_INT_ADD_DISPATCH(b)((result), (a), (b))
#    define IntSub(result, a, b) MISRA_INT_SUB_DISPATCH(b)((result), (a), (b))
#endif

#ifdef __cplusplus
}
#endif

#endif // MISRA_STD_CONTAINER_INT_MATH_H
