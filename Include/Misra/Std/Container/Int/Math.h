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
    bool IntShiftLeft(Int *value, u64 positions);
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
    bool IntShiftRight(Int *value, u64 positions);
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
    bool int_add(Int *result, Int *a, Int *b);
    ///
    /// Subtract one integer from another.
    ///
    /// result[out] : Destination for the difference
    /// a[in]       : Minuend
    /// b[in]       : Subtrahend
    ///
    /// SUCCESS : Returns `true`. The result has been computed and the
    ///           destination object updated.
    /// FAILURE : Returns `false` when `a < b`. The destination is left
    ///           untouched.
    ///
    /// USAGE:
    ///   if (!IntSub(&diff, &a, &b)) { /* negative result not representable */ }
    ///
    /// TAGS: Int, Math, Subtract
    ///
    bool int_sub(Int *result, Int *a, Int *b);
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
    bool int_mul(Int *result, Int *a, Int *b);
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
    bool IntSquare(Int *result, Int *value);
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
    bool int_pow(Int *result, Int *base, Int *exponent);
    ///
    /// Divide one integer by another using floor division.
    ///
    /// result[out]    : Destination for the quotient
    /// dividend[in]   : Dividend
    /// divisor[in]    : Divisor
    ///
    /// SUCCESS : Returns `true`. The result has been computed and the
    ///           destination object updated.
    /// FAILURE : Returns `false` when the divisor is zero. The destination is left
    ///           untouched.
    ///
    /// USAGE:
    ///   IntDiv(&quotient, &a, &b);
    ///
    /// TAGS: Int, Math, Divide, Quotient
    ///
    bool int_div(Int *result, Int *dividend, Int *divisor);
    ///
    /// Divide one integer by another only when the division is exact.
    ///
    /// result[out]    : Destination for the quotient
    /// dividend[in]   : Dividend
    /// divisor[in]    : Divisor
    ///
    /// SUCCESS : Returns `true` when `divisor` divides `dividend` exactly.
    /// FAILURE : Returns `false` otherwise.
    ///
    /// USAGE:
    ///   bool exact = IntDivExact(&quotient, &a, &b);
    ///
    /// TAGS: Int, Math, DivideExact
    ///
    bool int_div_exact(Int *result, Int *dividend, Int *divisor);
    ///
    /// Compute `dividend mod divisor`.
    ///
    /// result[out]    : Destination for the remainder
    /// dividend[in]   : Dividend
    /// divisor[in]    : Divisor
    ///
    /// SUCCESS : Returns `true`. The result has been computed and the
    ///           destination object updated.
    /// FAILURE : Returns `false` when the divisor is zero. The destination is left
    ///           untouched.
    ///
    /// USAGE:
    ///   IntMod(&remainder, &a, &b);
    ///
    /// TAGS: Int, Math, Modulo
    ///
    bool int_mod(Int *result, Int *dividend, Int *divisor);
    ///
    /// Compute quotient and remainder in one call.
    ///
    /// quotient[out]   : Destination for the quotient
    /// remainder[out]  : Destination for the remainder
    /// dividend[in]    : Dividend
    /// divisor[in]     : Divisor
    ///
    /// SUCCESS : Returns `true`. The result has been computed and the
    ///           destination object updated.
    /// FAILURE : Returns `false` when the divisor is zero. The destination is left
    ///           untouched.
    ///
    /// USAGE:
    ///   IntDivMod(&q, &r, &a, &b);
    ///
    /// TAGS: Int, Math, Divide, Modulo
    ///
    bool int_div_mod(Int *quotient, Int *remainder, Int *dividend, Int *divisor);
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
    bool IntGCD(Int *result, Int *a, Int *b);
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
    bool IntLCM(Int *result, Int *a, Int *b);
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
    bool IntRoot(Int *result, Int *value, u64 degree);
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
    bool IntRootRem(Int *root, Int *remainder, Int *value, u64 degree);
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
    bool IntSqrt(Int *result, Int *value);
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
    bool IntSqrtRem(Int *root, Int *remainder, Int *value);
    ///
    /// Test whether a value is a perfect square.
    ///
    /// value[in] : Value to test
    ///
    /// SUCCESS : Returns `true` when `value = n^2` for some integer `n`.
    /// FAILURE : Returns `false` otherwise.
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
    /// SUCCESS : Returns `true` when the value can be written as `a^b` with `b > 1`.
    /// FAILURE : Returns `false` otherwise.
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
    /// error[out] : Optional pointer set to `true` on failure and `false` on success
    ///
    /// SUCCESS : Returns `-1`, `0`, or `1`, or `0` on failure.
    ///
    /// USAGE:
    ///   int symbol = IntJacobi(&a, &n);
    ///
    /// TAGS: Int, Math, Jacobi, NumberTheory
    ///
    bool IntTryJacobi(int *out, Int *a, Int *n);
    int  IntJacobiWithError(Int *a, Int *n, bool *error);
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
    bool IntSquareMod(Int *result, Int *value, Int *modulus);
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
    bool IntModAdd(Int *result, Int *a, Int *b, Int *modulus);
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
    bool IntModSub(Int *result, Int *a, Int *b, Int *modulus);
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
    bool IntModMul(Int *result, Int *a, Int *b, Int *modulus);
    ///
    /// Compute modular division `a / b (mod modulus)`.
    ///
    /// result[out]   : Destination for the reduced quotient
    /// a[in]         : Numerator
    /// b[in]         : Denominator
    /// modulus[in]   : Modulus
    ///
    /// SUCCESS : Returns `true` when a modular inverse for `b` exists.
    /// FAILURE : Returns `false` otherwise.
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
    bool int_pow_mod(Int *result, Int *base, Int *exponent, Int *modulus);
    ///
    /// Compute the multiplicative inverse of a value modulo `modulus`.
    ///
    /// result[out]   : Destination for the inverse
    /// value[in]     : Value to invert
    /// modulus[in]   : Modulus
    ///
    /// SUCCESS : Returns `true` when the inverse exists.
    /// FAILURE : Returns `false` otherwise.
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
    /// SUCCESS : Returns `true` when a modular square root exists.
    /// FAILURE : Returns `false` otherwise.
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
    /// error[out] : Optional error flag set to `true` when the test cannot be completed.
    ///
    /// SUCCESS : Returns `true` when the value is probably prime.
    /// FAILURE : Returns `false` otherwise.
    ///
    /// INFO: This is a probable-prime test, not a proof of primality.
    ///
    /// USAGE:
    ///   bool prime = IntIsProbablePrime(&value);
    ///
    /// TAGS: Int, Math, Prime, Predicate
    ///
    bool IntIsProbablePrimeWithError(Int *value, bool *error);
    ///
    /// Find the next probable prime greater than or equal to a value.
    ///
    /// result[out] : Destination for the prime
    /// value[in]   : Starting point
    ///
    /// USAGE:
    ///   bool ok = IntNextPrime(&prime, &value);
    ///
    /// TAGS: Int, Math, Prime, Search
    ///
    bool IntNextPrime(Int *result, Int *value);

    static inline bool int_is_probable_prime_no_error(Int *value) {
        return IntIsProbablePrimeWithError(value, NULL);
    }

#define INT_IS_PROBABLE_PRIME_SELECT(_1, _2, NAME, ...) NAME
#define IntIsProbablePrime(...)                                                                                        \
    INT_IS_PROBABLE_PRIME_SELECT(__VA_ARGS__, IntIsProbablePrimeWithError, int_is_probable_prime_no_error)(__VA_ARGS__)

#ifndef __cplusplus
#    define INT_ADD_DISPATCH(rhs)                                                                                      \
        _Generic(                                                                                                      \
            (rhs),                                                                                                     \
            Int *: int_add,                                                                                            \
            unsigned char: int_add_u64,                                                                                \
            unsigned short: int_add_u64,                                                                               \
            unsigned int: int_add_u64,                                                                                 \
            unsigned long: int_add_u64,                                                                                \
            unsigned long long: int_add_u64,                                                                           \
            signed char: int_add_i64,                                                                                  \
            signed short: int_add_i64,                                                                                 \
            signed int: int_add_i64,                                                                                   \
            signed long: int_add_i64,                                                                                  \
            signed long long: int_add_i64                                                                              \
        )

#    define INT_SUB_DISPATCH(rhs)                                                                                      \
        _Generic(                                                                                                      \
            (rhs),                                                                                                     \
            Int *: int_sub,                                                                                            \
            unsigned char: int_sub_u64,                                                                                \
            unsigned short: int_sub_u64,                                                                               \
            unsigned int: int_sub_u64,                                                                                 \
            unsigned long: int_sub_u64,                                                                                \
            unsigned long long: int_sub_u64,                                                                           \
            signed char: int_sub_i64,                                                                                  \
            signed short: int_sub_i64,                                                                                 \
            signed int: int_sub_i64,                                                                                   \
            signed long: int_sub_i64,                                                                                  \
            signed long long: int_sub_i64                                                                              \
        )

#    define INT_MUL_DISPATCH(rhs)                                                                                      \
        _Generic(                                                                                                      \
            (rhs),                                                                                                     \
            Int *: int_mul,                                                                                            \
            unsigned char: int_mul_u64,                                                                                \
            unsigned short: int_mul_u64,                                                                               \
            unsigned int: int_mul_u64,                                                                                 \
            unsigned long: int_mul_u64,                                                                                \
            unsigned long long: int_mul_u64,                                                                           \
            signed char: int_mul_i64,                                                                                  \
            signed short: int_mul_i64,                                                                                 \
            signed int: int_mul_i64,                                                                                   \
            signed long: int_mul_i64,                                                                                  \
            signed long long: int_mul_i64                                                                              \
        )

#    define INT_POW_DISPATCH(exponent)                                                                                 \
        _Generic(                                                                                                      \
            (exponent),                                                                                                \
            Int *: int_pow,                                                                                            \
            unsigned char: int_pow_u64,                                                                                \
            unsigned short: int_pow_u64,                                                                               \
            unsigned int: int_pow_u64,                                                                                 \
            unsigned long: int_pow_u64,                                                                                \
            unsigned long long: int_pow_u64,                                                                           \
            signed char: int_pow_i64,                                                                                  \
            signed short: int_pow_i64,                                                                                 \
            signed int: int_pow_i64,                                                                                   \
            signed long: int_pow_i64,                                                                                  \
            signed long long: int_pow_i64                                                                              \
        )

#    define INT_DIV_DISPATCH(divisor)                                                                                  \
        _Generic(                                                                                                      \
            (divisor),                                                                                                 \
            Int *: int_div,                                                                                            \
            unsigned char: int_div_u64,                                                                                \
            unsigned short: int_div_u64,                                                                               \
            unsigned int: int_div_u64,                                                                                 \
            unsigned long: int_div_u64,                                                                                \
            unsigned long long: int_div_u64,                                                                           \
            signed char: int_div_i64,                                                                                  \
            signed short: int_div_i64,                                                                                 \
            signed int: int_div_i64,                                                                                   \
            signed long: int_div_i64,                                                                                  \
            signed long long: int_div_i64                                                                              \
        )

#    define INT_DIV_EXACT_DISPATCH(divisor)                                                                            \
        _Generic(                                                                                                      \
            (divisor),                                                                                                 \
            Int *: int_div_exact,                                                                                      \
            unsigned char: int_div_exact_u64,                                                                          \
            unsigned short: int_div_exact_u64,                                                                         \
            unsigned int: int_div_exact_u64,                                                                           \
            unsigned long: int_div_exact_u64,                                                                          \
            unsigned long long: int_div_exact_u64,                                                                     \
            signed char: int_div_exact_i64,                                                                            \
            signed short: int_div_exact_i64,                                                                           \
            signed int: int_div_exact_i64,                                                                             \
            signed long: int_div_exact_i64,                                                                            \
            signed long long: int_div_exact_i64                                                                        \
        )

#    define INT_MOD_DISPATCH(divisor)                                                                                  \
        _Generic(                                                                                                      \
            (divisor),                                                                                                 \
            Int *: int_mod,                                                                                            \
            unsigned char: int_mod_u64_into,                                                                           \
            unsigned short: int_mod_u64_into,                                                                          \
            unsigned int: int_mod_u64_into,                                                                            \
            unsigned long: int_mod_u64_into,                                                                           \
            unsigned long long: int_mod_u64_into,                                                                      \
            signed char: int_mod_i64_into,                                                                             \
            signed short: int_mod_i64_into,                                                                            \
            signed int: int_mod_i64_into,                                                                              \
            signed long: int_mod_i64_into,                                                                             \
            signed long long: int_mod_i64_into                                                                         \
        )

#    define INT_DIVMOD_DISPATCH(divisor)                                                                               \
        _Generic(                                                                                                      \
            (divisor),                                                                                                 \
            Int *: int_div_mod,                                                                                        \
            unsigned char: int_div_mod_u64,                                                                            \
            unsigned short: int_div_mod_u64,                                                                           \
            unsigned int: int_div_mod_u64,                                                                             \
            unsigned long: int_div_mod_u64,                                                                            \
            unsigned long long: int_div_mod_u64,                                                                       \
            signed char: int_div_mod_i64,                                                                              \
            signed short: int_div_mod_i64,                                                                             \
            signed int: int_div_mod_i64,                                                                               \
            signed long: int_div_mod_i64,                                                                              \
            signed long long: int_div_mod_i64                                                                          \
        )

#    define INT_POWMOD_DISPATCH(exponent)                                                                              \
        _Generic(                                                                                                      \
            (exponent),                                                                                                \
            Int *: int_pow_mod,                                                                                        \
            unsigned char: int_pow_u64_mod,                                                                            \
            unsigned short: int_pow_u64_mod,                                                                           \
            unsigned int: int_pow_u64_mod,                                                                             \
            unsigned long: int_pow_u64_mod,                                                                            \
            unsigned long long: int_pow_u64_mod,                                                                       \
            signed char: int_pow_i64_mod,                                                                              \
            signed short: int_pow_i64_mod,                                                                             \
            signed int: int_pow_i64_mod,                                                                               \
            signed long: int_pow_i64_mod,                                                                              \
            signed long long: int_pow_i64_mod                                                                          \
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
#    define IntAdd(result, a, b) INT_ADD_DISPATCH(b)((result), (a), (b))
///
/// Generic subtraction convenience macro.
/// Dispatches on the type of `b` to the matching `IntSub*` overload.
///
/// result[out] : Destination for the difference
/// a[in]       : Left operand
/// b[in]       : Right operand (`Int`, pointer, `u64`, or `i64` compatible type)
///
/// SUCCESS : Returns Result of the selected subtraction overload.
///
/// USAGE:
///   bool ok = IntSub(&diff, &value, 1u);
///
/// TAGS: Int, Math, Subtract, Generic
///
#    define IntSub(result, a, b) INT_SUB_DISPATCH(b)((result), (a), (b))
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
#    define IntMul(result, a, b) INT_MUL_DISPATCH(b)((result), (a), (b))
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
#    define IntPow(result, base, exponent) INT_POW_DISPATCH(exponent)((result), (base), (exponent))
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
#    define IntDiv(result, dividend, divisor) INT_DIV_DISPATCH(divisor)((result), (dividend), (divisor))
///
/// Generic exact-division convenience macro.
///
/// result[out]    : Destination for the quotient
/// dividend[in]   : Dividend
/// divisor[in]    : Divisor selected through generic dispatch
///
/// SUCCESS : Returns `true` when the division is exact.
/// FAILURE : Returns `false` otherwise.
///
/// USAGE:
///   bool ok = IntDivExact(&quotient, &value, 5u);
///
/// TAGS: Int, Math, DivideExact, Generic
///
#    define IntDivExact(result, dividend, divisor) INT_DIV_EXACT_DISPATCH(divisor)((result), (dividend), (divisor))
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
#    define IntMod(result, dividend, divisor) INT_MOD_DISPATCH(divisor)((result), (dividend), (divisor))
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
#    define IntDivMod(quotient, remainder, dividend, divisor)                                                          \
        INT_DIVMOD_DISPATCH(divisor)((quotient), (remainder), (dividend), (divisor))
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
#    define IntPowMod(result, base, exponent, modulus)                                                                 \
        INT_POWMOD_DISPATCH(exponent)((result), (base), (exponent), (modulus))
#endif

    static inline int int_jacobi_no_error(Int *a, Int *n) {
        return IntJacobiWithError(a, n, NULL);
    }

#define INT_JACOBI_SELECT(_1, _2, _3, NAME, ...) NAME
#define IntJacobi(...)                           INT_JACOBI_SELECT(__VA_ARGS__, IntJacobiWithError, int_jacobi_no_error)(__VA_ARGS__)

#ifdef __cplusplus
}
#endif

#endif // MISRA_STD_CONTAINER_INT_MATH_H
