/// file      : std/container/int/math.h
/// author    : Generated following Misra project patterns
/// This is free and unencumbered software released into the public domain.
///
/// Arithmetic and number-theoretic helpers for Int.

#ifndef MISRA_STD_CONTAINER_INT_MATH_H
#define MISRA_STD_CONTAINER_INT_MATH_H

#include "Private.h"

#ifdef __cplusplus
extern "C" {
#endif

///
/// Shift an integer left by the given number of bit positions.
///
/// value[in]      : Integer to modify
/// positions[in]  : Number of zero bits to append on the right
///
/// USAGE:
///   IntShiftLeft(&value, 8);
///
/// TAGS: Int, Math, ShiftLeft, Bits
///
void IntShiftLeft(Int *value, u64 positions);
///
/// Shift an integer right by the given number of bit positions.
///
/// value[in]      : Integer to modify
/// positions[in]  : Number of low bits to discard
///
/// USAGE:
///   IntShiftRight(&value, 1);
///
/// TAGS: Int, Math, ShiftRight, Bits
///
void IntShiftRight(Int *value, u64 positions);
///
/// Add two integers.
///
/// result[out] : Destination for the sum
/// a[in]       : Left operand
/// b[in]       : Right operand
///
/// USAGE:
///   IntAdd(&sum, &a, &b);
///
/// TAGS: Int, Math, Add
///
void (IntAdd)(Int *result, Int *a, Int *b);
///
/// Subtract one integer from another.
///
/// result[out] : Destination for the difference
/// a[in]       : Minuend
/// b[in]       : Subtrahend
///
/// RETURNS: `true` on success, `false` when `a < b`.
///
/// USAGE:
///   if (!IntSub(&diff, &a, &b)) { /* negative result not representable */ }
///
/// TAGS: Int, Math, Subtract
///
bool (IntSub)(Int *result, Int *a, Int *b);
///
/// Multiply two integers.
///
/// result[out] : Destination for the product
/// a[in]       : Left operand
/// b[in]       : Right operand
///
/// USAGE:
///   IntMul(&product, &a, &b);
///
/// TAGS: Int, Math, Multiply
///
void (IntMul)(Int *result, Int *a, Int *b);
///
/// Square an integer.
///
/// result[out] : Destination for the square
/// value[in]   : Value to square
///
/// USAGE:
///   IntSquare(&square, &value);
///
/// TAGS: Int, Math, Square
///
void IntSquare(Int *result, Int *value);
///
/// Raise an integer to an arbitrary-precision power.
///
/// result[out]   : Destination for the result
/// base[in]      : Base value
/// exponent[in]  : Non-negative exponent
///
/// USAGE:
///   IntPow(&power, &base, &exp);
///
/// TAGS: Int, Math, Power, Exponentiation
///
void (IntPow)(Int *result, Int *base, Int *exponent);
///
/// Divide one integer by another using floor division.
///
/// result[out]    : Destination for the quotient
/// dividend[in]   : Dividend
/// divisor[in]    : Divisor
///
/// WARN: Aborts on division by zero.
///
/// USAGE:
///   IntDiv(&quotient, &a, &b);
///
/// TAGS: Int, Math, Divide, Quotient
///
void (IntDiv)(Int *result, Int *dividend, Int *divisor);
///
/// Divide one integer by another only when the division is exact.
///
/// result[out]    : Destination for the quotient
/// dividend[in]   : Dividend
/// divisor[in]    : Divisor
///
/// RETURNS: `true` when `divisor` divides `dividend` exactly.
///
/// USAGE:
///   bool exact = IntDivExact(&quotient, &a, &b);
///
/// TAGS: Int, Math, DivideExact
///
bool (IntDivExact)(Int *result, Int *dividend, Int *divisor);
///
/// Compute `dividend mod divisor`.
///
/// result[out]    : Destination for the remainder
/// dividend[in]   : Dividend
/// divisor[in]    : Divisor
///
/// WARN: Aborts on division by zero.
///
/// USAGE:
///   IntMod(&remainder, &a, &b);
///
/// TAGS: Int, Math, Modulo
///
void (IntMod)(Int *result, Int *dividend, Int *divisor);
///
/// Compute quotient and remainder in one call.
///
/// quotient[out]   : Destination for the quotient
/// remainder[out]  : Destination for the remainder
/// dividend[in]    : Dividend
/// divisor[in]     : Divisor
///
/// WARN: Aborts on division by zero.
///
/// USAGE:
///   IntDivMod(&q, &r, &a, &b);
///
/// TAGS: Int, Math, Divide, Modulo
///
void (IntDivMod)(Int *quotient, Int *remainder, Int *dividend, Int *divisor);
///
/// Compute the greatest common divisor of two integers.
///
/// result[out] : Destination for the GCD
/// a[in]       : First operand
/// b[in]       : Second operand
///
/// USAGE:
///   IntGCD(&gcd, &a, &b);
///
/// TAGS: Int, Math, GCD, NumberTheory
///
void IntGCD(Int *result, Int *a, Int *b);
///
/// Compute the least common multiple of two integers.
///
/// result[out] : Destination for the LCM
/// a[in]       : First operand
/// b[in]       : Second operand
///
/// USAGE:
///   IntLCM(&lcm, &a, &b);
///
/// TAGS: Int, Math, LCM, NumberTheory
///
void IntLCM(Int *result, Int *a, Int *b);
///
/// Compute the integer `degree`-th root of a value.
///
/// result[out]  : Destination for the root
/// value[in]    : Input value
/// degree[in]   : Root degree
///
/// USAGE:
///   IntRoot(&root, &value, 3);
///
/// TAGS: Int, Math, Root, NumberTheory
///
void IntRoot(Int *result, Int *value, u64 degree);
///
/// Compute an integer root and the leftover remainder.
///
/// root[out]       : Destination for the root
/// remainder[out]  : Destination for the remainder
/// value[in]       : Input value
/// degree[in]      : Root degree
///
/// USAGE:
///   IntRootRem(&root, &rem, &value, 3);
///
/// TAGS: Int, Math, Root, Remainder
///
void IntRootRem(Int *root, Int *remainder, Int *value, u64 degree);
///
/// Compute the integer square root.
///
/// result[out] : Destination for the root
/// value[in]   : Input value
///
/// USAGE:
///   IntSqrt(&root, &value);
///
/// TAGS: Int, Math, Sqrt
///
void IntSqrt(Int *result, Int *value);
///
/// Compute the integer square root and remainder.
///
/// root[out]       : Destination for the root
/// remainder[out]  : Destination for the remainder
/// value[in]       : Input value
///
/// USAGE:
///   IntSqrtRem(&root, &rem, &value);
///
/// TAGS: Int, Math, Sqrt, Remainder
///
void IntSqrtRem(Int *root, Int *remainder, Int *value);
///
/// Test whether a value is a perfect square.
///
/// value[in] : Value to test
///
/// RETURNS: `true` when `value = n^2` for some integer `n`.
///
/// USAGE:
///   bool square = IntIsPerfectSquare(&value);
///
/// TAGS: Int, Math, PerfectSquare, Predicate
///
bool IntIsPerfectSquare(Int *value);
///
/// Test whether a value is a perfect power.
///
/// value[in] : Value to test
///
/// RETURNS: `true` when the value can be written as `a^b` with `b > 1`.
///
/// USAGE:
///   bool power = IntIsPerfectPower(&value);
///
/// TAGS: Int, Math, PerfectPower, Predicate
///
bool IntIsPerfectPower(Int *value);
///
/// Compute the Jacobi symbol `(a/n)`.
///
/// a[in] : Numerator
/// n[in] : Odd positive modulus
///
/// RETURNS: `-1`, `0`, or `1`.
///
/// WARN: Aborts when `n` is zero or even.
///
/// USAGE:
///   int symbol = IntJacobi(&a, &n);
///
/// TAGS: Int, Math, Jacobi, NumberTheory
///
int  IntJacobi(Int *a, Int *n);
///
/// Compute `(value^2) mod modulus`.
///
/// result[out]   : Destination for the reduced square
/// value[in]     : Value to square
/// modulus[in]   : Modulus
///
/// USAGE:
///   IntSquareMod(&result, &value, &modulus);
///
/// TAGS: Int, Math, Modular, Square
///
void IntSquareMod(Int *result, Int *value, Int *modulus);
///
/// Compute `(a + b) mod modulus`.
///
/// result[out]   : Destination for the reduced sum
/// a[in]         : Left operand
/// b[in]         : Right operand
/// modulus[in]   : Modulus
///
/// USAGE:
///   IntModAdd(&result, &a, &b, &modulus);
///
/// TAGS: Int, Math, Modular, Add
///
void IntModAdd(Int *result, Int *a, Int *b, Int *modulus);
///
/// Compute `(a - b) mod modulus`.
///
/// result[out]   : Destination for the reduced difference
/// a[in]         : Left operand
/// b[in]         : Right operand
/// modulus[in]   : Modulus
///
/// USAGE:
///   IntModSub(&result, &a, &b, &modulus);
///
/// TAGS: Int, Math, Modular, Subtract
///
void IntModSub(Int *result, Int *a, Int *b, Int *modulus);
///
/// Compute `(a * b) mod modulus`.
///
/// result[out]   : Destination for the reduced product
/// a[in]         : Left operand
/// b[in]         : Right operand
/// modulus[in]   : Modulus
///
/// USAGE:
///   IntModMul(&result, &a, &b, &modulus);
///
/// TAGS: Int, Math, Modular, Multiply
///
void IntModMul(Int *result, Int *a, Int *b, Int *modulus);
///
/// Compute modular division `a / b (mod modulus)`.
///
/// result[out]   : Destination for the reduced quotient
/// a[in]         : Numerator
/// b[in]         : Denominator
/// modulus[in]   : Modulus
///
/// RETURNS: `true` when a modular inverse for `b` exists.
///
/// USAGE:
///   bool ok = IntModDiv(&result, &a, &b, &modulus);
///
/// TAGS: Int, Math, Modular, Divide
///
bool IntModDiv(Int *result, Int *a, Int *b, Int *modulus);
///
/// Compute `(base^exponent) mod modulus`.
///
/// result[out]    : Destination for the reduced power
/// base[in]       : Base value
/// exponent[in]   : Exponent value
/// modulus[in]    : Modulus
///
/// USAGE:
///   IntPowMod(&result, &base, &exp, &modulus);
///
/// TAGS: Int, Math, Modular, Power
///
void (IntPowMod)(Int *result, Int *base, Int *exponent, Int *modulus);
///
/// Compute the multiplicative inverse of a value modulo `modulus`.
///
/// result[out]   : Destination for the inverse
/// value[in]     : Value to invert
/// modulus[in]   : Modulus
///
/// RETURNS: `true` when the inverse exists.
///
/// USAGE:
///   bool ok = IntModInv(&inverse, &value, &modulus);
///
/// TAGS: Int, Math, Modular, Inverse
///
bool IntModInv(Int *result, Int *value, Int *modulus);
///
/// Compute a modular square root.
///
/// result[out]   : Destination for the root
/// value[in]     : Value whose square root is requested
/// modulus[in]   : Modulus
///
/// RETURNS: `true` when a modular square root exists.
///
/// USAGE:
///   bool ok = IntModSqrt(&root, &value, &modulus);
///
/// TAGS: Int, Math, Modular, Sqrt
///
bool IntModSqrt(Int *result, Int *value, Int *modulus);
///
/// Perform a probabilistic primality test.
///
/// value[in] : Integer to test
///
/// RETURNS: `true` when the value is probably prime.
///
/// INFO: This is a probable-prime test, not a proof of primality.
///
/// USAGE:
///   bool prime = IntIsProbablePrime(&value);
///
/// TAGS: Int, Math, Prime, Predicate
///
bool IntIsProbablePrime(Int *value);
///
/// Find the next probable prime greater than or equal to a value.
///
/// result[out] : Destination for the prime
/// value[in]   : Starting point
///
/// USAGE:
///   IntNextPrime(&prime, &value);
///
/// TAGS: Int, Math, Prime, Search
///
void IntNextPrime(Int *result, Int *value);

#ifndef __cplusplus
#    define MISRA_INT_ADD_DISPATCH(rhs)                                                                                \
        _Generic(                                                                                                      \
            (rhs),                                                                                                     \
            Int: MISRA_PRIV_IntAddValue,                                                                               \
            Int *: IntAdd,                                                                                             \
            const Int *: MISRA_PRIV_IntAddConst,                                                                       \
            unsigned char: MISRA_PRIV_IntAddU64,                                                                       \
            unsigned short: MISRA_PRIV_IntAddU64,                                                                      \
            unsigned int: MISRA_PRIV_IntAddU64,                                                                        \
            unsigned long: MISRA_PRIV_IntAddU64,                                                                       \
            unsigned long long: MISRA_PRIV_IntAddU64,                                                                  \
            signed char: MISRA_PRIV_IntAddI64,                                                                         \
            signed short: MISRA_PRIV_IntAddI64,                                                                        \
            signed int: MISRA_PRIV_IntAddI64,                                                                          \
            signed long: MISRA_PRIV_IntAddI64,                                                                         \
            signed long long: MISRA_PRIV_IntAddI64                                                                     \
        )

#    define MISRA_INT_SUB_DISPATCH(rhs)                                                                                \
        _Generic(                                                                                                      \
            (rhs),                                                                                                     \
            Int: MISRA_PRIV_IntSubValue,                                                                               \
            Int *: IntSub,                                                                                             \
            const Int *: MISRA_PRIV_IntSubConst,                                                                       \
            unsigned char: MISRA_PRIV_IntSubU64,                                                                       \
            unsigned short: MISRA_PRIV_IntSubU64,                                                                      \
            unsigned int: MISRA_PRIV_IntSubU64,                                                                        \
            unsigned long: MISRA_PRIV_IntSubU64,                                                                       \
            unsigned long long: MISRA_PRIV_IntSubU64,                                                                  \
            signed char: MISRA_PRIV_IntSubI64,                                                                         \
            signed short: MISRA_PRIV_IntSubI64,                                                                        \
            signed int: MISRA_PRIV_IntSubI64,                                                                          \
            signed long: MISRA_PRIV_IntSubI64,                                                                         \
            signed long long: MISRA_PRIV_IntSubI64                                                                     \
        )

#    define MISRA_INT_MUL_DISPATCH(rhs)                                                                                \
        _Generic(                                                                                                      \
            (rhs),                                                                                                     \
            Int: MISRA_PRIV_IntMulValue,                                                                               \
            Int *: IntMul,                                                                                             \
            const Int *: MISRA_PRIV_IntMulConst,                                                                       \
            unsigned char: MISRA_PRIV_IntMulU64,                                                                       \
            unsigned short: MISRA_PRIV_IntMulU64,                                                                      \
            unsigned int: MISRA_PRIV_IntMulU64,                                                                        \
            unsigned long: MISRA_PRIV_IntMulU64,                                                                       \
            unsigned long long: MISRA_PRIV_IntMulU64,                                                                  \
            signed char: MISRA_PRIV_IntMulI64,                                                                         \
            signed short: MISRA_PRIV_IntMulI64,                                                                        \
            signed int: MISRA_PRIV_IntMulI64,                                                                          \
            signed long: MISRA_PRIV_IntMulI64,                                                                         \
            signed long long: MISRA_PRIV_IntMulI64                                                                     \
        )

#    define MISRA_INT_POW_DISPATCH(exponent)                                                                           \
        _Generic(                                                                                                      \
            (exponent),                                                                                                \
            Int: MISRA_PRIV_IntPowValue,                                                                               \
            Int *: IntPow,                                                                                             \
            const Int *: MISRA_PRIV_IntPowConst,                                                                       \
            unsigned char: MISRA_PRIV_IntPowU64,                                                                       \
            unsigned short: MISRA_PRIV_IntPowU64,                                                                      \
            unsigned int: MISRA_PRIV_IntPowU64,                                                                        \
            unsigned long: MISRA_PRIV_IntPowU64,                                                                       \
            unsigned long long: MISRA_PRIV_IntPowU64,                                                                  \
            signed char: MISRA_PRIV_IntPowI64,                                                                         \
            signed short: MISRA_PRIV_IntPowI64,                                                                        \
            signed int: MISRA_PRIV_IntPowI64,                                                                          \
            signed long: MISRA_PRIV_IntPowI64,                                                                         \
            signed long long: MISRA_PRIV_IntPowI64                                                                     \
        )

#    define MISRA_INT_DIV_DISPATCH(divisor)                                                                            \
        _Generic(                                                                                                      \
            (divisor),                                                                                                 \
            Int: MISRA_PRIV_IntDivValue,                                                                               \
            Int *: IntDiv,                                                                                             \
            const Int *: MISRA_PRIV_IntDivConst,                                                                       \
            unsigned char: MISRA_PRIV_IntDivU64,                                                                       \
            unsigned short: MISRA_PRIV_IntDivU64,                                                                      \
            unsigned int: MISRA_PRIV_IntDivU64,                                                                        \
            unsigned long: MISRA_PRIV_IntDivU64,                                                                       \
            unsigned long long: MISRA_PRIV_IntDivU64,                                                                  \
            signed char: MISRA_PRIV_IntDivI64,                                                                         \
            signed short: MISRA_PRIV_IntDivI64,                                                                        \
            signed int: MISRA_PRIV_IntDivI64,                                                                          \
            signed long: MISRA_PRIV_IntDivI64,                                                                         \
            signed long long: MISRA_PRIV_IntDivI64                                                                     \
        )

#    define MISRA_INT_DIV_EXACT_DISPATCH(divisor)                                                                      \
        _Generic(                                                                                                      \
            (divisor),                                                                                                 \
            Int: MISRA_PRIV_IntDivExactValue,                                                                          \
            Int *: IntDivExact,                                                                                        \
            const Int *: MISRA_PRIV_IntDivExactConst,                                                                  \
            unsigned char: MISRA_PRIV_IntDivExactU64,                                                                  \
            unsigned short: MISRA_PRIV_IntDivExactU64,                                                                 \
            unsigned int: MISRA_PRIV_IntDivExactU64,                                                                   \
            unsigned long: MISRA_PRIV_IntDivExactU64,                                                                  \
            unsigned long long: MISRA_PRIV_IntDivExactU64,                                                             \
            signed char: MISRA_PRIV_IntDivExactI64,                                                                    \
            signed short: MISRA_PRIV_IntDivExactI64,                                                                   \
            signed int: MISRA_PRIV_IntDivExactI64,                                                                     \
            signed long: MISRA_PRIV_IntDivExactI64,                                                                    \
            signed long long: MISRA_PRIV_IntDivExactI64                                                                \
        )

#    define MISRA_INT_MOD_DISPATCH(divisor)                                                                            \
        _Generic(                                                                                                      \
            (divisor),                                                                                                 \
            Int: MISRA_PRIV_IntModValue,                                                                               \
            Int *: IntMod,                                                                                             \
            const Int *: MISRA_PRIV_IntModConst,                                                                       \
            unsigned char: MISRA_PRIV_IntModU64Into,                                                                   \
            unsigned short: MISRA_PRIV_IntModU64Into,                                                                  \
            unsigned int: MISRA_PRIV_IntModU64Into,                                                                    \
            unsigned long: MISRA_PRIV_IntModU64Into,                                                                   \
            unsigned long long: MISRA_PRIV_IntModU64Into,                                                              \
            signed char: MISRA_PRIV_IntModI64Into,                                                                     \
            signed short: MISRA_PRIV_IntModI64Into,                                                                    \
            signed int: MISRA_PRIV_IntModI64Into,                                                                      \
            signed long: MISRA_PRIV_IntModI64Into,                                                                     \
            signed long long: MISRA_PRIV_IntModI64Into                                                                 \
        )

#    define MISRA_INT_DIVMOD_DISPATCH(divisor)                                                                         \
        _Generic(                                                                                                      \
            (divisor),                                                                                                 \
            Int: MISRA_PRIV_IntDivModValue,                                                                            \
            Int *: IntDivMod,                                                                                          \
            const Int *: MISRA_PRIV_IntDivModConst,                                                                    \
            unsigned char: MISRA_PRIV_IntDivModU64,                                                                    \
            unsigned short: MISRA_PRIV_IntDivModU64,                                                                   \
            unsigned int: MISRA_PRIV_IntDivModU64,                                                                     \
            unsigned long: MISRA_PRIV_IntDivModU64,                                                                    \
            unsigned long long: MISRA_PRIV_IntDivModU64,                                                               \
            signed char: MISRA_PRIV_IntDivModI64,                                                                      \
            signed short: MISRA_PRIV_IntDivModI64,                                                                     \
            signed int: MISRA_PRIV_IntDivModI64,                                                                       \
            signed long: MISRA_PRIV_IntDivModI64,                                                                      \
            signed long long: MISRA_PRIV_IntDivModI64                                                                  \
        )

#    define MISRA_INT_POWMOD_DISPATCH(exponent)                                                                        \
        _Generic(                                                                                                      \
            (exponent),                                                                                                \
            Int: MISRA_PRIV_IntPowModValue,                                                                            \
            Int *: IntPowMod,                                                                                          \
            const Int *: MISRA_PRIV_IntPowModConst,                                                                    \
            unsigned char: MISRA_PRIV_IntPowU64Mod,                                                                    \
            unsigned short: MISRA_PRIV_IntPowU64Mod,                                                                   \
            unsigned int: MISRA_PRIV_IntPowU64Mod,                                                                     \
            unsigned long: MISRA_PRIV_IntPowU64Mod,                                                                    \
            unsigned long long: MISRA_PRIV_IntPowU64Mod,                                                               \
            signed char: MISRA_PRIV_IntPowI64Mod,                                                                      \
            signed short: MISRA_PRIV_IntPowI64Mod,                                                                     \
            signed int: MISRA_PRIV_IntPowI64Mod,                                                                       \
            signed long: MISRA_PRIV_IntPowI64Mod,                                                                      \
            signed long long: MISRA_PRIV_IntPowI64Mod                                                                  \
        )

///
/// Generic addition convenience macro.
/// Dispatches on the type of `b` to the matching `IntAdd*` overload.
///
/// result[out] : Destination for the sum
/// a[in]       : Left operand
/// b[in]       : Right operand (`Int`, pointer, `u64`, or `i64` compatible type)
///
/// USAGE:
///   IntAdd(&sum, &value, 10u);
///
/// TAGS: Int, Math, Add, Generic
///
#    define IntAdd(result, a, b) MISRA_INT_ADD_DISPATCH(b)((result), (a), (b))
///
/// Generic subtraction convenience macro.
/// Dispatches on the type of `b` to the matching `IntSub*` overload.
///
/// result[out] : Destination for the difference
/// a[in]       : Left operand
/// b[in]       : Right operand (`Int`, pointer, `u64`, or `i64` compatible type)
///
/// RETURNS: Result of the selected subtraction overload.
///
/// USAGE:
///   bool ok = IntSub(&diff, &value, 1u);
///
/// TAGS: Int, Math, Subtract, Generic
///
#    define IntSub(result, a, b) MISRA_INT_SUB_DISPATCH(b)((result), (a), (b))
///
/// Generic multiplication convenience macro.
///
/// result[out] : Destination for the product
/// a[in]       : Left operand
/// b[in]       : Right operand selected through generic dispatch
///
/// USAGE:
///   IntMul(&product, &value, 10u);
///
/// TAGS: Int, Math, Multiply, Generic
///
#    define IntMul(result, a, b) MISRA_INT_MUL_DISPATCH(b)((result), (a), (b))
///
/// Generic exponentiation convenience macro.
///
/// result[out]   : Destination for the result
/// base[in]      : Base value
/// exponent[in]  : Exponent selected through generic dispatch
///
/// USAGE:
///   IntPow(&power, &base, 20u);
///
/// TAGS: Int, Math, Power, Generic
///
#    define IntPow(result, base, exponent) MISRA_INT_POW_DISPATCH(exponent)((result), (base), (exponent))
///
/// Generic division convenience macro.
///
/// result[out]    : Destination for the quotient
/// dividend[in]   : Dividend
/// divisor[in]    : Divisor selected through generic dispatch
///
/// USAGE:
///   IntDiv(&quotient, &value, 10u);
///
/// TAGS: Int, Math, Divide, Generic
///
#    define IntDiv(result, dividend, divisor) MISRA_INT_DIV_DISPATCH(divisor)((result), (dividend), (divisor))
///
/// Generic exact-division convenience macro.
///
/// result[out]    : Destination for the quotient
/// dividend[in]   : Dividend
/// divisor[in]    : Divisor selected through generic dispatch
///
/// RETURNS: `true` when the division is exact.
///
/// USAGE:
///   bool ok = IntDivExact(&quotient, &value, 5u);
///
/// TAGS: Int, Math, DivideExact, Generic
///
#    define IntDivExact(result, dividend, divisor)                                                                     \
        MISRA_INT_DIV_EXACT_DISPATCH(divisor)((result), (dividend), (divisor))
///
/// Generic modulo convenience macro.
///
/// result[out]    : Destination for the remainder
/// dividend[in]   : Dividend
/// divisor[in]    : Divisor selected through generic dispatch
///
/// USAGE:
///   IntMod(&remainder, &value, 97u);
///
/// TAGS: Int, Math, Modulo, Generic
///
#    define IntMod(result, dividend, divisor) MISRA_INT_MOD_DISPATCH(divisor)((result), (dividend), (divisor))
///
/// Generic quotient-and-remainder convenience macro.
///
/// quotient[out]   : Destination for the quotient
/// remainder[out]  : Destination for the remainder
/// dividend[in]    : Dividend
/// divisor[in]     : Divisor selected through generic dispatch
///
/// USAGE:
///   IntDivMod(&q, &r, &value, 97u);
///
/// TAGS: Int, Math, Divide, Modulo, Generic
///
#    define IntDivMod(quotient, remainder, dividend, divisor)                                                         \
        MISRA_INT_DIVMOD_DISPATCH(divisor)((quotient), (remainder), (dividend), (divisor))
///
/// Generic modular exponentiation convenience macro.
///
/// result[out]    : Destination for the reduced power
/// base[in]       : Base value
/// exponent[in]   : Exponent selected through generic dispatch
/// modulus[in]    : Modulus
///
/// USAGE:
///   IntPowMod(&result, &base, 65537u, &modulus);
///
/// TAGS: Int, Math, Modular, Power, Generic
///
#    define IntPowMod(result, base, exponent, modulus)                                                                \
        MISRA_INT_POWMOD_DISPATCH(exponent)((result), (base), (exponent), (modulus))
#endif

#ifdef __cplusplus
}
#endif

#endif // MISRA_STD_CONTAINER_INT_MATH_H
