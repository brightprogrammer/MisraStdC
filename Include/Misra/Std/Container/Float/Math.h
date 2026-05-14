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
    bool(FloatAdd)(Float *result, Float *a, Float *b);
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
    bool(FloatSub)(Float *result, Float *a, Float *b);
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
    bool(FloatMul)(Float *result, Float *a, Float *b);
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
    bool(FloatDiv)(Float *result, Float *a, Float *b, u64 precision);
#ifndef __cplusplus
#    define FLOAT_ADD_DISPATCH(rhs)                                                                                    \
        _Generic(                                                                                                      \
            (rhs),                                                                                                     \
            Float *: FloatAdd,                                                                                         \
            Int *: FloatAddInt,                                                                                        \
            unsigned char: FloatAddU64,                                                                                \
            unsigned short: FloatAddU64,                                                                               \
            unsigned int: FloatAddU64,                                                                                 \
            unsigned long: FloatAddU64,                                                                                \
            unsigned long long: FloatAddU64,                                                                           \
            signed char: FloatAddI64,                                                                                  \
            signed short: FloatAddI64,                                                                                 \
            signed int: FloatAddI64,                                                                                   \
            signed long: FloatAddI64,                                                                                  \
            signed long long: FloatAddI64,                                                                             \
            float: FloatAddF32,                                                                                        \
            double: FloatAddF64                                                                                        \
        )

#    define FLOAT_SUB_DISPATCH(rhs)                                                                                    \
        _Generic(                                                                                                      \
            (rhs),                                                                                                     \
            Float *: FloatSub,                                                                                         \
            Int *: FloatSubInt,                                                                                        \
            unsigned char: FloatSubU64,                                                                                \
            unsigned short: FloatSubU64,                                                                               \
            unsigned int: FloatSubU64,                                                                                 \
            unsigned long: FloatSubU64,                                                                                \
            unsigned long long: FloatSubU64,                                                                           \
            signed char: FloatSubI64,                                                                                  \
            signed short: FloatSubI64,                                                                                 \
            signed int: FloatSubI64,                                                                                   \
            signed long: FloatSubI64,                                                                                  \
            signed long long: FloatSubI64,                                                                             \
            float: FloatSubF32,                                                                                        \
            double: FloatSubF64                                                                                        \
        )

#    define FLOAT_MUL_DISPATCH(rhs)                                                                                    \
        _Generic(                                                                                                      \
            (rhs),                                                                                                     \
            Float *: FloatMul,                                                                                         \
            Int *: FloatMulInt,                                                                                        \
            unsigned char: FloatMulU64,                                                                                \
            unsigned short: FloatMulU64,                                                                               \
            unsigned int: FloatMulU64,                                                                                 \
            unsigned long: FloatMulU64,                                                                                \
            unsigned long long: FloatMulU64,                                                                           \
            signed char: FloatMulI64,                                                                                  \
            signed short: FloatMulI64,                                                                                 \
            signed int: FloatMulI64,                                                                                   \
            signed long: FloatMulI64,                                                                                  \
            signed long long: FloatMulI64,                                                                             \
            float: FloatMulF32,                                                                                        \
            double: FloatMulF64                                                                                        \
        )

#    define FLOAT_DIV_DISPATCH(rhs)                                                                                    \
        _Generic(                                                                                                      \
            (rhs),                                                                                                     \
            Float *: FloatDiv,                                                                                         \
            Int *: FloatDivInt,                                                                                        \
            unsigned char: FloatDivU64,                                                                                \
            unsigned short: FloatDivU64,                                                                               \
            unsigned int: FloatDivU64,                                                                                 \
            unsigned long: FloatDivU64,                                                                                \
            unsigned long long: FloatDivU64,                                                                           \
            signed char: FloatDivI64,                                                                                  \
            signed short: FloatDivI64,                                                                                 \
            signed int: FloatDivI64,                                                                                   \
            signed long: FloatDivI64,                                                                                  \
            signed long long: FloatDivI64,                                                                             \
            float: FloatDivF32,                                                                                        \
            double: FloatDivF64                                                                                        \
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
