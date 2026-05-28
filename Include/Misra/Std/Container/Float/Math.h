/// file      : std/container/float/math.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Arithmetic helpers for Float.

#ifndef MISRA_STD_CONTAINER_FLOAT_MATH_H
#define MISRA_STD_CONTAINER_FLOAT_MATH_H

#include "Private.h"

#ifdef __cplusplus
extern "C" {
#endif

    ///
    /// Negate a floating-point value in place.
    ///
    /// value[in,out] : Float to modify
    ///
    /// SUCCESS : Returns to the caller. The sign of `*value` is
    ///           flipped unless `*value` is zero, which stays
    ///           non-negative. Significand and exponent are
    ///           unchanged.
    /// FAILURE : Function cannot fail.
    ///
    /// INFO: Zero remains non-negative after normalization.
    ///
    /// USAGE:
    ///   FloatNegate(&value);
    ///
    /// TAGS: Float, Math, Negate, Sign
    ///
    void FloatNegate(Float *value);
    ///
    /// Replace a float with its absolute value.
    ///
    /// value[in,out] : Float to modify
    ///
    /// SUCCESS : Returns to the caller. `*value` is now non-negative;
    ///           significand and exponent are unchanged.
    /// FAILURE : Function cannot fail.
    ///
    /// USAGE:
    ///   FloatAbs(&value);
    ///
    /// TAGS: Float, Math, AbsoluteValue
    ///
    void FloatAbs(Float *value);
    ///
    /// Add two floats.
    ///
    /// result[out] : Destination for the sum
    /// a[in]       : Left operand
    /// b[in]       : Right operand
    ///
    /// SUCCESS : Returns `true`. `*result` holds the sum.
    /// FAILURE : Returns `false` on allocator OOM while growing
    ///           `result`'s significand. `*result` is left untouched.
    ///
    /// USAGE:
    ///   FloatAdd(&sum, &a, &b);
    ///
    /// TAGS: Float, Math, Add
    ///
    bool float_add(Float *result, const Float *a, const Float *b);
    ///
    /// Subtract one float from another.
    ///
    /// result[out] : Destination for the difference
    /// a[in]       : Minuend
    /// b[in]       : Subtrahend
    ///
    /// SUCCESS : Returns `true`. `*result` holds `a - b`.
    /// FAILURE : Returns `false` on allocator OOM while growing
    ///           `result`'s significand. `*result` is left untouched.
    ///
    /// USAGE:
    ///   FloatSub(&diff, &a, &b);
    ///
    /// TAGS: Float, Math, Subtract
    ///
    bool float_sub(Float *result, const Float *a, const Float *b);
    ///
    /// Multiply two floats.
    ///
    /// result[out] : Destination for the product
    /// a[in]       : Left operand
    /// b[in]       : Right operand
    ///
    /// SUCCESS : Returns `true`. `*result` holds the product.
    /// FAILURE : Returns `false` on allocator OOM while growing
    ///           `result`'s significand. `*result` is left untouched.
    ///
    /// USAGE:
    ///   FloatMul(&product, &a, &b);
    ///
    /// TAGS: Float, Math, Multiply
    ///
    bool float_mul(Float *result, const Float *a, const Float *b);
    ///
    /// Divide one float by another.
    /// The quotient is truncated after scaling by `10^precision`.
    ///
    /// result[out]    : Destination for the quotient
    /// a[in]          : Dividend
    /// b[in]          : Divisor
    /// precision[in]  : Number of decimal digits to retain before truncation
    ///
    /// SUCCESS : Returns `true`. The result has been computed and the
    ///           destination object updated.
    /// FAILURE : Returns `false` when the divisor is zero. The destination is left
    ///           untouched.
    ///
    /// USAGE:
    ///   FloatDiv(&quotient, &a, &b, 8);
    ///
    /// TAGS: Float, Math, Divide, Precision
    ///
    bool float_div(Float *result, const Float *a, const Float *b, u64 precision);
#ifndef __cplusplus
///
/// Generic addition convenience macro for `Float`.
///
/// result[out] : Destination for the sum
/// a[in]       : Left operand
/// b[in]       : Right operand selected through generic dispatch
///
/// USAGE:
///   FloatAdd(&sum, &value, 1.25);
///
/// SUCCESS : Returns `true`; `*result` holds `a + b` with shared
///           exponent and normalized significand.
/// FAILURE : Returns `false` if an intermediate allocation
///           (clone / rescale / add) fails; `*result` is unchanged.
///
/// TAGS: Float, Math, Add, Generic
///
#    define FloatAdd(result, a, b)                                                                                     \
        _Generic(                                                                                                      \
            (b),                                                                                                       \
            Float *: float_add,                                                                                        \
            Int *: float_add_int,                                                                                      \
            unsigned char: float_add_u64,                                                                              \
            unsigned short: float_add_u64,                                                                             \
            unsigned int: float_add_u64,                                                                               \
            unsigned long: float_add_u64,                                                                              \
            unsigned long long: float_add_u64,                                                                         \
            signed char: float_add_i64,                                                                                \
            signed short: float_add_i64,                                                                               \
            signed int: float_add_i64,                                                                                 \
            signed long: float_add_i64,                                                                                \
            signed long long: float_add_i64,                                                                           \
            float: float_add_f32,                                                                                      \
            double: float_add_f64                                                                                      \
        )((result), (a), (b))
///
/// Generic subtraction convenience macro for `Float`.
///
/// result[out] : Destination for the difference
/// a[in]       : Left operand
/// b[in]       : Right operand selected through generic dispatch
///
/// USAGE:
///   FloatSub(&diff, &value, 2u);
///
/// SUCCESS : Returns `true`; `*result` holds `a - b` with shared
///           exponent and normalized significand.
/// FAILURE : Returns `false` if an intermediate allocation
///           (clone / rescale / subtract) fails; `*result` is unchanged.
///
/// TAGS: Float, Math, Subtract, Generic
///
#    define FloatSub(result, a, b)                                                                                     \
        _Generic(                                                                                                      \
            (b),                                                                                                       \
            Float *: float_sub,                                                                                        \
            Int *: float_sub_int,                                                                                      \
            unsigned char: float_sub_u64,                                                                              \
            unsigned short: float_sub_u64,                                                                             \
            unsigned int: float_sub_u64,                                                                               \
            unsigned long: float_sub_u64,                                                                              \
            unsigned long long: float_sub_u64,                                                                         \
            signed char: float_sub_i64,                                                                                \
            signed short: float_sub_i64,                                                                               \
            signed int: float_sub_i64,                                                                                 \
            signed long: float_sub_i64,                                                                                \
            signed long long: float_sub_i64,                                                                           \
            float: float_sub_f32,                                                                                      \
            double: float_sub_f64                                                                                      \
        )((result), (a), (b))
///
/// Generic multiplication convenience macro for `Float`.
///
/// result[out] : Destination for the product
/// a[in]       : Left operand
/// b[in]       : Right operand selected through generic dispatch
///
/// USAGE:
///   FloatMul(&product, &value, 0.5f);
///
/// SUCCESS : Returns `true`; `*result` holds `a * b` with the
///           exponents summed and the significand normalized.
/// FAILURE : Returns `false` if an intermediate allocation
///           (significand multiply / normalize) fails; `*result` is
///           unchanged.
///
/// TAGS: Float, Math, Multiply, Generic
///
#    define FloatMul(result, a, b)                                                                                     \
        _Generic(                                                                                                      \
            (b),                                                                                                       \
            Float *: float_mul,                                                                                        \
            Int *: float_mul_int,                                                                                      \
            unsigned char: float_mul_u64,                                                                              \
            unsigned short: float_mul_u64,                                                                             \
            unsigned int: float_mul_u64,                                                                               \
            unsigned long: float_mul_u64,                                                                              \
            unsigned long long: float_mul_u64,                                                                         \
            signed char: float_mul_i64,                                                                                \
            signed short: float_mul_i64,                                                                               \
            signed int: float_mul_i64,                                                                                 \
            signed long: float_mul_i64,                                                                                \
            signed long long: float_mul_i64,                                                                           \
            float: float_mul_f32,                                                                                      \
            double: float_mul_f64                                                                                      \
        )((result), (a), (b))
///
/// Generic division convenience macro for `Float`.
///
/// result[out]    : Destination for the quotient
/// a[in]          : Dividend
/// b[in]          : Divisor selected through generic dispatch
/// precision[in]  : Decimal precision to retain
///
/// USAGE:
///   FloatDiv(&quotient, &value, 3.0, 8);
///
/// SUCCESS : Returns `true`; `*result` holds `a / b` with `precision`
///           decimal digits retained. When `a` is zero `*result` is
///           set to zero without further work.
/// FAILURE : Returns `false` when `b` is zero (logged) or when any
///           intermediate allocation (scale / multiply / divide) fails;
///           `*result` is unchanged.
///
/// TAGS: Float, Math, Divide, Generic
///
#    define FloatDiv(result, a, b, precision)                                                                          \
        _Generic(                                                                                                      \
            (b),                                                                                                       \
            Float *: float_div,                                                                                        \
            Int *: float_div_int,                                                                                      \
            unsigned char: float_div_u64,                                                                              \
            unsigned short: float_div_u64,                                                                             \
            unsigned int: float_div_u64,                                                                               \
            unsigned long: float_div_u64,                                                                              \
            unsigned long long: float_div_u64,                                                                         \
            signed char: float_div_i64,                                                                                \
            signed short: float_div_i64,                                                                               \
            signed int: float_div_i64,                                                                                 \
            signed long: float_div_i64,                                                                                \
            signed long long: float_div_i64,                                                                           \
            float: float_div_f32,                                                                                      \
            double: float_div_f64                                                                                      \
        )((result), (a), (b), (precision))
#endif

#ifdef __cplusplus
}
#endif

#endif // MISRA_STD_CONTAINER_FLOAT_MATH_H
