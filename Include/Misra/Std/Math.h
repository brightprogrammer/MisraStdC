/// file      : std/math.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Floating-point primitives without `<math.h>` / libm. Routes to
/// compiler builtins where they exist (GCC, Clang, MSVC all expose
/// the relevant `__builtin_*` / intrinsic). The library does not
/// link libm.

#ifndef MISRA_STD_MATH_H
#define MISRA_STD_MATH_H

#include <Misra/Types.h>

#ifdef __cplusplus
extern "C" {
#endif

/// IEEE 754 positive infinity / quiet NaN constants -- the in-tree
/// equivalents of the platform's positive-infinity and quiet-NaN
/// double-precision values, kept free of any platform-header
/// dependency.
///
/// TAGS: Math, Float, Infinity, NaN, Constant
///
#if defined(_MSC_VER) && !defined(__clang__)
    // MSVC: no `__builtin_inf` / `__builtin_nan`. The explicit double
    // literal `1e308 * 10.0` overflows to +Inf at compile time on every
    // conforming compiler, and `+Inf - +Inf` is the canonical quiet
    // NaN; we lean on both.
#    define F64_INFINITY ((f64)(1e308 * 10.0))
#    define F64_NAN      ((f64)(F64_INFINITY - F64_INFINITY))
#else
#    define F64_INFINITY ((f64)__builtin_inf())
#    define F64_NAN      ((f64)__builtin_nan(""))
#endif

#define F32_INFINITY ((f32)F64_INFINITY)
#define F32_NAN      ((f32)F64_NAN)

    /// True iff `x` is a NaN. Uses the compiler builtin where available;
    /// otherwise the IEEE 754 `(x != x)` identity (NaN is the only
    /// value not equal to itself).
    ///
    /// SUCCESS : Returns `true` if `x` is a NaN, `false` otherwise.
    ///           Pure inspection -- never fails and never mutates state.
    /// FAILURE : None. The function has no failure mode.
    ///
    /// TAGS: Math, Float, Classify, NaN
    ///
    static inline bool F64IsNan(f64 x) {
#if defined(__GNUC__) || defined(__clang__)
        return (bool)__builtin_isnan(x);
#else
    return x != x;
#endif
    }

    /// True iff `x` is a NaN (f32 variant). Same semantics as
    /// `F64IsNan`, applied to a single-precision value.
    ///
    /// SUCCESS : Returns `true` if `x` is a NaN, `false` otherwise.
    ///           Pure inspection -- never fails and never mutates state.
    /// FAILURE : None. The function has no failure mode.
    ///
    /// TAGS: Math, Float, Classify, NaN
    ///
    static inline bool F32IsNan(f32 x) {
#if defined(__GNUC__) || defined(__clang__)
        return (bool)__builtin_isnan(x);
#else
    return x != x;
#endif
    }

    /// True iff `x` is `+inf` or `-inf`.
    ///
    /// SUCCESS : Returns `true` if `x` is `+inf` or `-inf`, `false`
    ///           otherwise (including for NaN and finite values).
    ///           Pure inspection -- never fails and never mutates state.
    /// FAILURE : None. The function has no failure mode.
    ///
    /// TAGS: Math, Float, Classify, Infinity
    ///
    static inline bool F64IsInf(f64 x) {
#if defined(__GNUC__) || defined(__clang__)
        return (bool)__builtin_isinf(x);
#else
    return x == F64_INFINITY || x == -F64_INFINITY;
#endif
    }

    /// True iff `x` is `+inf` or `-inf` (f32 variant). Same semantics
    /// as `F64IsInf`, applied to a single-precision value.
    ///
    /// SUCCESS : Returns `true` if `x` is `+inf` or `-inf`, `false`
    ///           otherwise (including for NaN and finite values).
    ///           Pure inspection -- never fails and never mutates state.
    /// FAILURE : None. The function has no failure mode.
    ///
    /// TAGS: Math, Float, Classify, Infinity
    ///
    static inline bool F32IsInf(f32 x) {
#if defined(__GNUC__) || defined(__clang__)
        return (bool)__builtin_isinf(x);
#else
    return x == F32_INFINITY || x == -F32_INFINITY;
#endif
    }

    /// Absolute value. Plain arithmetic; no libm call.
    ///
    /// SUCCESS : Returns `|x|` -- `x` if non-negative, `-x` otherwise.
    ///           Pure arithmetic -- never fails and never mutates state.
    /// FAILURE : None. The function has no failure mode.
    ///
    /// TAGS: Math, Float, Abs
    ///
    static inline f64 F64Abs(f64 x) {
        return x < 0.0 ? -x : x;
    }

    /// Absolute value (f32 variant). Same semantics as `F64Abs`,
    /// applied to a single-precision value.
    ///
    /// SUCCESS : Returns `|x|` -- `x` if non-negative, `-x` otherwise.
    ///           Pure arithmetic -- never fails and never mutates state.
    /// FAILURE : None. The function has no failure mode.
    ///
    /// TAGS: Math, Float, Abs
    ///
    static inline f32 F32Abs(f32 x) {
        return x < 0.0f ? -x : x;
    }

    /// Integer power: x raised to a small signed exponent. Fits the
    /// tolerance-scale-factor (10^k) shape used by the in-tree numeric
    /// parsers and printers. Loops, so don't use for hot paths.
    ///
    /// F64Pow(2.0, 10) == 1024.0
    /// F64Pow(10.0, -3) == 0.001
    ///
    /// SUCCESS : Returns `base` raised to `exp`. Negative exponents
    ///           produce the reciprocal of the positive-exponent
    ///           result. Pure arithmetic; no allocation.
    /// FAILURE : No explicit failure return. IEEE 754 edge cases
    ///           propagate through the underlying floating-point ops:
    ///           a sufficiently large positive `exp` overflows to
    ///           `+inf` (or `-inf` if `base` is negative); a negative
    ///           `exp` whose positive-exponent result overflows to
    ///           `+/-inf` yields `+/-0.0` after the reciprocal step;
    ///           `0` raised to a negative `exp` yields `+inf`; `NaN`
    ///           inputs propagate to a `NaN` return. Callers that
    ///           care should classify the result with `F64IsInf` /
    ///           `F64IsNan`.
    ///
    /// TAGS: Math, Float, Pow
    ///
    static inline f64 F64Pow(f64 base, i32 exp) {
        f64  result = 1.0;
        bool neg    = exp < 0;
        u32  n      = neg ? (u32)(-exp) : (u32)exp;
        while (n--) {
            result *= base;
        }
        return neg ? 1.0 / result : result;
    }

    /// Checked-add for u64. Computes `a + b`, stores the result in
    /// `*out`, returns `true` if the mathematical sum fits in u64,
    /// `false` if it would overflow. On overflow `*out` still holds
    /// the wrapped value (callers must not rely on it).
    ///
    /// Used by parsers to validate `offset + size` style arithmetic
    /// over attacker-controlled u64 fields, where the older
    /// "if (a + b > bound)" idiom wraps and silently passes.
    ///
    /// SUCCESS : Returns `true`; `*out` holds the exact sum `a + b`
    ///           because the mathematical result fits in 64 bits.
    /// FAILURE : Returns `false` when the sum would overflow u64;
    ///           `*out` holds the wrapped (mod 2^64) value and must
    ///           be treated as garbage by callers.
    ///
    /// TAGS: Math, Overflow, Checked, Arithmetic
    ///
    static inline bool AddOverflow64(u64 a, u64 b, u64 *out) {
#if defined(__GNUC__) || defined(__clang__)
        return !__builtin_add_overflow(a, b, out);
#else
    *out = a + b;
    return *out >= a;
#endif
    }

    /// Checked-multiply for u64. Same shape as AddOverflow64.
    ///
    /// SUCCESS : Returns `true`; `*out` holds the exact product `a * b`
    ///           because the mathematical result fits in 64 bits.
    /// FAILURE : Returns `false` when the product would overflow u64;
    ///           `*out` holds the wrapped (mod 2^64) value and must
    ///           be treated as garbage by callers.
    ///
    /// TAGS: Math, Overflow, Checked, Arithmetic
    ///
    static inline bool MulOverflow64(u64 a, u64 b, u64 *out) {
#if defined(__GNUC__) || defined(__clang__)
        return !__builtin_mul_overflow(a, b, out);
#else
    *out = a * b;
    return a == 0 || (*out / a) == b;
#endif
    }

#ifdef __cplusplus
}
#endif

#endif // MISRA_STD_MATH_H
