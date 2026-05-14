/// file      : std/container/float/math.h
/// author    : Generated following Misra project patterns
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
    /// value[in] : Float to modify
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
    /// value[in] : Float to modify
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
    /// USAGE:
    ///   FloatAdd(&sum, &a, &b);
    ///
    /// TAGS: Float, Math, Add
    ///
    bool(float_add)(Float *result, Float *a, Float *b);
    ///
    /// Subtract one float from another.
    ///
    /// result[out] : Destination for the difference
    /// a[in]       : Minuend
    /// b[in]       : Subtrahend
    ///
    /// USAGE:
    ///   FloatSub(&diff, &a, &b);
    ///
    /// TAGS: Float, Math, Subtract
    ///
    bool(float_sub)(Float *result, Float *a, Float *b);
    ///
    /// Multiply two floats.
    ///
    /// result[out] : Destination for the product
    /// a[in]       : Left operand
    /// b[in]       : Right operand
    ///
    /// USAGE:
    ///   FloatMul(&product, &a, &b);
    ///
    /// TAGS: Float, Math, Multiply
    ///
    bool(float_mul)(Float *result, Float *a, Float *b);
    ///
    /// Divide one float by another.
    /// The quotient is truncated after scaling by `10^precision`.
    ///
    /// result[out]    : Destination for the quotient
    /// a[in]          : Dividend
    /// b[in]          : Divisor
    /// precision[in]  : Number of decimal digits to retain before truncation
    ///
    /// RETURNS: `true` on success, `false` when the divisor is zero.
    ///
    /// USAGE:
    ///   FloatDiv(&quotient, &a, &b, 8);
    ///
    /// TAGS: Float, Math, Divide, Precision
    ///
    bool(float_div)(Float *result, Float *a, Float *b, u64 precision);
#ifndef __cplusplus
#    define FLOAT_ADD_DISPATCH(rhs)                                                                                    \
        _Generic(                                                                                                      \
            (rhs),                                                                                                     \
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
        )

#    define FLOAT_SUB_DISPATCH(rhs)                                                                                    \
        _Generic(                                                                                                      \
            (rhs),                                                                                                     \
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
        )

#    define FLOAT_MUL_DISPATCH(rhs)                                                                                    \
        _Generic(                                                                                                      \
            (rhs),                                                                                                     \
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
        )

#    define FLOAT_DIV_DISPATCH(rhs)                                                                                    \
        _Generic(                                                                                                      \
            (rhs),                                                                                                     \
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
        )

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
/// TAGS: Float, Math, Add, Generic
///
#    define FloatAdd(result, a, b) FLOAT_ADD_DISPATCH(b)((result), (a), (b))
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
/// TAGS: Float, Math, Subtract, Generic
///
#    define FloatSub(result, a, b) FLOAT_SUB_DISPATCH(b)((result), (a), (b))
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
/// TAGS: Float, Math, Multiply, Generic
///
#    define FloatMul(result, a, b) FLOAT_MUL_DISPATCH(b)((result), (a), (b))
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
/// TAGS: Float, Math, Divide, Generic
///
#    define FloatDiv(result, a, b, precision) FLOAT_DIV_DISPATCH(b)((result), (a), (b), (precision))
#endif

#ifdef __cplusplus
}
#endif

#endif // MISRA_STD_CONTAINER_FLOAT_MATH_H
