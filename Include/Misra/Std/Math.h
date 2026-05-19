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

#if defined(_MSC_VER) && !defined(__clang__)
    // MSVC: no `__builtin_inf` etc.; HUGE_VAL / NAN come from
    // `<float.h>` macros, but those are also libc territory. The
    // explicit double literal `1e+308 * 10.0` overflows to +Inf at
    // compile time on every conforming compiler; we lean on that.
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
    static inline bool F64IsNan(f64 x) {
#if defined(__GNUC__) || defined(__clang__)
        return (bool)__builtin_isnan(x);
#else
    return x != x;
#endif
    }

    static inline bool F32IsNan(f32 x) {
#if defined(__GNUC__) || defined(__clang__)
        return (bool)__builtin_isnan(x);
#else
    return x != x;
#endif
    }

    /// True iff `x` is `+inf` or `-inf`.
    static inline bool F64IsInf(f64 x) {
#if defined(__GNUC__) || defined(__clang__)
        return (bool)__builtin_isinf(x);
#else
    return x == F64_INFINITY || x == -F64_INFINITY;
#endif
    }

    static inline bool F32IsInf(f32 x) {
#if defined(__GNUC__) || defined(__clang__)
        return (bool)__builtin_isinf(x);
#else
    return x == F32_INFINITY || x == -F32_INFINITY;
#endif
    }

    /// Absolute value. Plain arithmetic; no libm call.
    static inline f64 F64Abs(f64 x) {
        return x < 0.0 ? -x : x;
    }

    static inline f32 F32Abs(f32 x) {
        return x < 0.0f ? -x : x;
    }

    /// Integer power: x raised to a small signed exponent. Replaces
    /// the libm `pow(x, n)` idiom for cases where n is a tolerance
    /// scale factor (10^k). Loops, so don't use for hot paths.
    ///
    /// F64Pow(2.0, 10) == 1024.0
    /// F64Pow(10.0, -3) == 0.001
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
    static inline bool AddOverflow64(u64 a, u64 b, u64 *out) {
#if defined(__GNUC__) || defined(__clang__)
        return !__builtin_add_overflow(a, b, out);
#else
    *out = a + b;
    return *out >= a;
#endif
    }

    /// Checked-multiply for u64. Same shape as AddOverflow64.
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
