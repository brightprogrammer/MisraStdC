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
void (FloatAdd)(Float *result, Float *a, Float *b);
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
void (FloatSub)(Float *result, Float *a, Float *b);
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
void (FloatMul)(Float *result, Float *a, Float *b);
///
/// Divide one float by another.
/// The quotient is truncated after scaling by `10^precision`.
///
/// result[out]    : Destination for the quotient
/// a[in]          : Dividend
/// b[in]          : Divisor
/// precision[in]  : Number of decimal digits to retain before truncation
///
/// WARN: Aborts on division by zero.
///
/// USAGE:
///   FloatDiv(&quotient, &a, &b, 8);
///
/// TAGS: Float, Math, Divide, Precision
///
void (FloatDiv)(Float *result, Float *a, Float *b, u64 precision);
#ifndef __cplusplus
#    define MISRA_FLOAT_ADD_DISPATCH(rhs)                                                                              \
        _Generic(                                                                                                      \
            (rhs),                                                                                                     \
            Float: MISRA_PRIV_FloatAddValueFloat,                                                                      \
            Float *: FloatAdd,                                                                                         \
            const Float *: MISRA_PRIV_FloatAddConstFloat,                                                              \
            Int: MISRA_PRIV_FloatAddValueInt,                                                                          \
            Int *: MISRA_PRIV_FloatAddInt,                                                                             \
            const Int *: MISRA_PRIV_FloatAddConstInt,                                                                  \
            unsigned char: MISRA_PRIV_FloatAddU64,                                                                     \
            unsigned short: MISRA_PRIV_FloatAddU64,                                                                    \
            unsigned int: MISRA_PRIV_FloatAddU64,                                                                      \
            unsigned long: MISRA_PRIV_FloatAddU64,                                                                     \
            unsigned long long: MISRA_PRIV_FloatAddU64,                                                                \
            signed char: MISRA_PRIV_FloatAddI64,                                                                       \
            signed short: MISRA_PRIV_FloatAddI64,                                                                      \
            signed int: MISRA_PRIV_FloatAddI64,                                                                        \
            signed long: MISRA_PRIV_FloatAddI64,                                                                       \
            signed long long: MISRA_PRIV_FloatAddI64,                                                                  \
            float: MISRA_PRIV_FloatAddF32,                                                                             \
            double: MISRA_PRIV_FloatAddF64                                                                             \
        )

#    define MISRA_FLOAT_SUB_DISPATCH(rhs)                                                                              \
        _Generic(                                                                                                      \
            (rhs),                                                                                                     \
            Float: MISRA_PRIV_FloatSubValueFloat,                                                                      \
            Float *: FloatSub,                                                                                         \
            const Float *: MISRA_PRIV_FloatSubConstFloat,                                                              \
            Int: MISRA_PRIV_FloatSubValueInt,                                                                          \
            Int *: MISRA_PRIV_FloatSubInt,                                                                             \
            const Int *: MISRA_PRIV_FloatSubConstInt,                                                                  \
            unsigned char: MISRA_PRIV_FloatSubU64,                                                                     \
            unsigned short: MISRA_PRIV_FloatSubU64,                                                                    \
            unsigned int: MISRA_PRIV_FloatSubU64,                                                                      \
            unsigned long: MISRA_PRIV_FloatSubU64,                                                                     \
            unsigned long long: MISRA_PRIV_FloatSubU64,                                                                \
            signed char: MISRA_PRIV_FloatSubI64,                                                                       \
            signed short: MISRA_PRIV_FloatSubI64,                                                                      \
            signed int: MISRA_PRIV_FloatSubI64,                                                                        \
            signed long: MISRA_PRIV_FloatSubI64,                                                                       \
            signed long long: MISRA_PRIV_FloatSubI64,                                                                  \
            float: MISRA_PRIV_FloatSubF32,                                                                             \
            double: MISRA_PRIV_FloatSubF64                                                                             \
        )

#    define MISRA_FLOAT_MUL_DISPATCH(rhs)                                                                              \
        _Generic(                                                                                                      \
            (rhs),                                                                                                     \
            Float: MISRA_PRIV_FloatMulValueFloat,                                                                      \
            Float *: FloatMul,                                                                                         \
            const Float *: MISRA_PRIV_FloatMulConstFloat,                                                              \
            Int: MISRA_PRIV_FloatMulValueInt,                                                                          \
            Int *: MISRA_PRIV_FloatMulInt,                                                                             \
            const Int *: MISRA_PRIV_FloatMulConstInt,                                                                  \
            unsigned char: MISRA_PRIV_FloatMulU64,                                                                     \
            unsigned short: MISRA_PRIV_FloatMulU64,                                                                    \
            unsigned int: MISRA_PRIV_FloatMulU64,                                                                      \
            unsigned long: MISRA_PRIV_FloatMulU64,                                                                     \
            unsigned long long: MISRA_PRIV_FloatMulU64,                                                                \
            signed char: MISRA_PRIV_FloatMulI64,                                                                       \
            signed short: MISRA_PRIV_FloatMulI64,                                                                      \
            signed int: MISRA_PRIV_FloatMulI64,                                                                        \
            signed long: MISRA_PRIV_FloatMulI64,                                                                       \
            signed long long: MISRA_PRIV_FloatMulI64,                                                                  \
            float: MISRA_PRIV_FloatMulF32,                                                                             \
            double: MISRA_PRIV_FloatMulF64                                                                             \
        )

#    define MISRA_FLOAT_DIV_DISPATCH(rhs)                                                                              \
        _Generic(                                                                                                      \
            (rhs),                                                                                                     \
            Float: MISRA_PRIV_FloatDivValueFloat,                                                                      \
            Float *: FloatDiv,                                                                                         \
            const Float *: MISRA_PRIV_FloatDivConstFloat,                                                              \
            Int: MISRA_PRIV_FloatDivValueInt,                                                                          \
            Int *: MISRA_PRIV_FloatDivInt,                                                                             \
            const Int *: MISRA_PRIV_FloatDivConstInt,                                                                  \
            unsigned char: MISRA_PRIV_FloatDivU64,                                                                     \
            unsigned short: MISRA_PRIV_FloatDivU64,                                                                    \
            unsigned int: MISRA_PRIV_FloatDivU64,                                                                      \
            unsigned long: MISRA_PRIV_FloatDivU64,                                                                     \
            unsigned long long: MISRA_PRIV_FloatDivU64,                                                                \
            signed char: MISRA_PRIV_FloatDivI64,                                                                       \
            signed short: MISRA_PRIV_FloatDivI64,                                                                      \
            signed int: MISRA_PRIV_FloatDivI64,                                                                        \
            signed long: MISRA_PRIV_FloatDivI64,                                                                       \
            signed long long: MISRA_PRIV_FloatDivI64,                                                                  \
            float: MISRA_PRIV_FloatDivF32,                                                                             \
            double: MISRA_PRIV_FloatDivF64                                                                             \
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
#    define FloatAdd(result, a, b) MISRA_FLOAT_ADD_DISPATCH(b)((result), (a), (b))
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
#    define FloatSub(result, a, b) MISRA_FLOAT_SUB_DISPATCH(b)((result), (a), (b))
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
#    define FloatMul(result, a, b) MISRA_FLOAT_MUL_DISPATCH(b)((result), (a), (b))
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
#    define FloatDiv(result, a, b, precision) MISRA_FLOAT_DIV_DISPATCH(b)((result), (a), (b), (precision))
#endif

#ifdef __cplusplus
}
#endif

#endif // MISRA_STD_CONTAINER_FLOAT_MATH_H
