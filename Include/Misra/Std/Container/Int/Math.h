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
    /// SUCCESS : Returns `true`. `*value` has been shifted in place.
    /// FAILURE : Returns `false` on allocator OOM while growing the
    ///           significand. `*value` is left untouched.
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
    /// SUCCESS : Returns `true`. `*value` has been shifted in place.
    /// FAILURE : Returns `false` on allocator OOM while trimming the
    ///           significand. `*value` is left untouched.
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
    /// SUCCESS : Returns `true`. `*result` holds `a + b`.
    /// FAILURE : Returns `false` on allocator OOM while growing
    ///           `result`. `*result` is left untouched.
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
    /// SUCCESS : Returns `true`. `*result` holds `a * b`.
    /// FAILURE : Returns `false` on allocator OOM while growing
    ///           `result`. `*result` is left untouched.
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
    /// SUCCESS : Returns `true`. `*result` holds `value * value`.
    /// FAILURE : Returns `false` on allocator OOM while growing
    ///           `result`. `*result` is left untouched.
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
    /// SUCCESS : Returns `true`. `*result` holds `base^exponent`.
    /// FAILURE : Returns `false` on negative `exponent` or allocator
    ///           OOM. `*result` is left untouched.
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
    /// SUCCESS : Returns `true`. `*result` holds `gcd(a, b)`.
    /// FAILURE : Returns `false` on allocator OOM while growing
    ///           `result`. `*result` is left untouched.
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
    /// SUCCESS : Returns `true`. `*result` holds `lcm(a, b)`.
    /// FAILURE : Returns `false` on allocator OOM while growing
    ///           `result`. `*result` is left untouched.
    ///
    /// USAGE:
    ///   IntLCM(&lcm, &a, &b);
    ///
    /// TAGS: Int, Math, LCM, NumberTheory
    ///
    bool IntLCM(Int *result, Int *a, Int *b);
    ///
    /// Compute the integer `degree`-th root of a value (floor).
    ///
    /// result[out]  : Destination for the root
    /// value[in]    : Input value
    /// degree[in]   : Root degree
    ///
    /// SUCCESS : Returns `true`. `*result` holds `floor(value^(1/degree))`.
    /// FAILURE : Returns `false` on `degree == 0` or allocator OOM.
    ///           `*result` is left untouched.
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
    /// SUCCESS : Returns `true`. `*root` holds `floor(value^(1/degree))`,
    ///           `*remainder` holds `value - root^degree`.
    /// FAILURE : Returns `false` on `degree == 0` or allocator OOM.
    ///           `*root` / `*remainder` are left untouched.
    ///
    /// USAGE:
    ///   IntRootRem(&root, &rem, &value, 3);
    ///
    /// TAGS: Int, Math, Root, Remainder
    ///
    bool IntRootRem(Int *root, Int *remainder, Int *value, u64 degree);
    ///
    /// Compute the integer square root (floor).
    ///
    /// result[out] : Destination for the root
    /// value[in]   : Input value
    ///
    /// SUCCESS : Returns `true`. `*result` holds `floor(sqrt(value))`.
    /// FAILURE : Returns `false` on allocator OOM. `*result` is left untouched.
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
    /// SUCCESS : Returns `true`. `*root` holds `floor(sqrt(value))`,
    ///           `*remainder` holds `value - root^2`.
    /// FAILURE : Returns `false` on allocator OOM. `*root` / `*remainder`
    ///           are left untouched.
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
    /// out[out] : Destination for the Jacobi symbol; one of `-1`, `0`,
    ///            or `1` on success.
    /// a[in]    : Numerator
    /// n[in]    : Odd positive modulus
    ///
    /// SUCCESS : Returns `true` and writes the Jacobi symbol to
    ///           `*out`. Neither operand is modified.
    /// FAILURE : Returns `false` when `n` is even, zero, or otherwise
    ///           invalid for the Jacobi computation. `*out` is left
    ///           untouched.
    ///
    /// USAGE:
    ///   int symbol = 0;
    ///   bool ok = IntTryJacobi(&symbol, &a, &n);
    ///
    /// TAGS: Int, Math, Jacobi, NumberTheory
    ///
    bool IntTryJacobi(int *out, Int *a, Int *n);

    ///
    /// Compute the Jacobi symbol `(a/n)` with explicit failure channel.
    ///
    /// a[in]      : Numerator.
    /// n[in]      : Odd positive modulus.
    /// error[out] : Optional pointer set to `true` on failure (invalid `n`)
    ///              and `false` on success.
    ///
    /// SUCCESS : Returns `-1`, `0`, or `1` reflecting the Jacobi symbol.
    ///           Neither operand is modified. When `error` is non-NULL,
    ///           `*error` is set to `false`.
    /// FAILURE : Returns `0` when `n` is even, zero, or otherwise invalid
    ///           for the Jacobi computation. When `error` is non-NULL,
    ///           `*error` is set to `true` so the caller can distinguish
    ///           failure from a true `0` result.
    ///
    /// TAGS: Int, Math, Jacobi, NumberTheory
    ///
    int IntJacobiWithError(Int *a, Int *n, bool *error);
    ///
    /// Compute `(value^2) mod modulus`.
    ///
    /// result[out]   : Destination for the reduced square
    /// value[in]     : Value to square
    /// modulus[in]   : Modulus
    ///
    /// SUCCESS : Returns `true`. `*result` holds `(value * value) mod modulus`.
    /// FAILURE : Returns `false` on `modulus == 0` or allocator OOM.
    ///           `*result` is left untouched.
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
    /// SUCCESS : Returns `true`. `*result` holds `(a + b) mod modulus`.
    /// FAILURE : Returns `false` on `modulus == 0` or allocator OOM.
    ///           `*result` is left untouched.
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
    /// SUCCESS : Returns `true`. `*result` holds `(a - b) mod modulus`,
    ///           normalised into `[0, modulus)`.
    /// FAILURE : Returns `false` on `modulus == 0` or allocator OOM.
    ///           `*result` is left untouched.
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
    /// SUCCESS : Returns `true`. `*result` holds `(a * b) mod modulus`.
    /// FAILURE : Returns `false` on `modulus == 0` or allocator OOM.
    ///           `*result` is left untouched.
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
    /// SUCCESS : Returns `true`. `*result` holds `(base^exponent) mod modulus`.
    /// FAILURE : Returns `false` on `modulus == 0`, negative `exponent`,
    ///           or allocator OOM. `*result` is left untouched.
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
    /// SUCCESS : Returns `true`. `*result` holds the smallest probable
    ///           prime >= `*value`.
    /// FAILURE : Returns `false` on allocator OOM during the witness
    ///           loop, or if the primality oracle gives up. `*result`
    ///           is left untouched.
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

///
/// Test whether the integer is probably prime using a Miller-Rabin style
/// primality check.
///
/// This public macro supports both forms:
///
/// - `IntIsProbablePrime(value)`         - returns the result, no error channel.
/// - `IntIsProbablePrime(value, error)`  - writes the error flag through `error`.
///
/// value[in]  : Integer to test.
/// error[out] : Optional pointer set to `true` on internal failure and
///              `false` on success.
///
/// SUCCESS : Returns `true` when the value is probably prime.
/// FAILURE : Returns `false` for composite values. With the two-argument
///           form, `*error` is set to `true` on internal allocation failure
///           during the witness loop and `false` otherwise.
///
/// TAGS: Int, Math, Prime, Predicate, Macro
///
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
/// SUCCESS : Returns `true`; `*result` holds `a + b`.
/// FAILURE : Returns `false` if an intermediate allocation fails;
///           `*result` is unchanged.
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
/// USAGE:
///   bool ok = IntSub(&diff, &value, 1u);
///
/// SUCCESS : Returns `true`; `*result` holds `a - b`.
/// FAILURE : Returns `false` if an intermediate allocation fails;
///           `*result` is unchanged.
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
/// SUCCESS : Returns `true`; `*result` holds `a * b`.
/// FAILURE : Returns `false` if an intermediate allocation fails;
///           `*result` is unchanged.
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
/// SUCCESS : Returns `true`; `*result` holds `base ** exponent`
///           computed via repeated-squaring.
/// FAILURE : Returns `false` if an intermediate allocation fails or
///           if `exponent` is negative; `*result` is unchanged.
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
/// SUCCESS : Returns `true`; `*result` holds the floor quotient of
///           `dividend / divisor`.
/// FAILURE : Returns `false` when `divisor` is zero (logged) or when an
///           intermediate allocation fails; `*result` is unchanged.
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
/// SUCCESS : Returns `true`; `*result` holds `dividend mod divisor`
///           in the range `[0, |divisor|)`.
/// FAILURE : Returns `false` when `divisor` is zero (logged) or when an
///           intermediate allocation fails; `*result` is unchanged.
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
/// SUCCESS : Returns `true`; `*quotient` and `*remainder` jointly
///           satisfy `dividend = quotient * divisor + remainder`.
/// FAILURE : `LOG_FATAL` if `quotient` and `remainder` alias the same
///           object. Returns `false` when `divisor` is zero (logged) or
///           when an intermediate allocation fails; `*quotient` and
///           `*remainder` are unchanged.
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
/// SUCCESS : Returns `true`; `*result` holds `base ** exponent mod
///           modulus` computed via repeated-squaring with intermediate
///           reduction.
/// FAILURE : Returns `false` when `modulus` is zero (logged), when
///           `exponent` is negative, or when an intermediate allocation
///           fails; `*result` is unchanged.
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

///
/// Compute the Jacobi symbol `(a/n)`.
///
/// This public macro supports both forms:
///
/// - `IntJacobi(a, n)`         - returns the result, no error channel.
/// - `IntJacobi(a, n, error)`  - writes the error flag through `error`.
///
/// a[in]      : Numerator.
/// n[in]      : Odd positive modulus.
/// error[out] : Optional pointer set to `true` on failure and `false` on success.
///
/// SUCCESS : Returns `-1`, `0`, or `1`. Neither operand is modified.
/// FAILURE : Returns `0` when `n` is even, zero, or otherwise invalid for
///           the Jacobi computation. With the two-argument form the caller
///           cannot distinguish a true `0` result from failure; use the
///           three-argument form with an `error` pointer to disambiguate.
///
/// TAGS: Int, Math, Jacobi, NumberTheory, Macro
///
#define IntJacobi(...) INT_JACOBI_SELECT(__VA_ARGS__, IntJacobiWithError, int_jacobi_no_error)(__VA_ARGS__)

#ifdef __cplusplus
}
#endif

#endif // MISRA_STD_CONTAINER_INT_MATH_H
