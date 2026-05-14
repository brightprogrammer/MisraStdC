/// file      : std/container/float/compare.h
/// author    : Generated following Misra project patterns
/// This is free and unencumbered software released into the public domain.
///
/// Comparison helpers for Float.

#ifndef MISRA_STD_CONTAINER_FLOAT_COMPARE_H
#define MISRA_STD_CONTAINER_FLOAT_COMPARE_H

#include "Private.h"

#ifdef __cplusplus
extern "C" {
#endif

    ///
    /// Compare two arbitrary-precision floating-point values.
    ///
    /// lhs[in] : Left-hand operand
    /// rhs[in] : Right-hand operand
    ///
    /// RETURNS: `-1` if `lhs < rhs`, `0` if equal, `1` if `lhs > rhs`.
    ///
    /// USAGE:
    ///   int cmp = FloatCompare(&a, &b);
    ///
    /// TAGS: Float, Compare, Ordering
    ///
    int float_compare_with_error(Float *lhs, Float *rhs, bool *error);
    int(float_compare)(Float *lhs, Float *rhs);
#ifndef __cplusplus
#    define FLOAT_COMPARE_DISPATCH(rhs)                                                                                \
        _Generic(                                                                                                      \
            (rhs),                                                                                                     \
            Float *: float_compare,                                                                                    \
            Int *: float_compare_int,                                                                                  \
            unsigned char: float_compare_u64,                                                                          \
            unsigned short: float_compare_u64,                                                                         \
            unsigned int: float_compare_u64,                                                                           \
            unsigned long: float_compare_u64,                                                                          \
            unsigned long long: float_compare_u64,                                                                     \
            signed char: float_compare_i64,                                                                            \
            signed short: float_compare_i64,                                                                           \
            signed int: float_compare_i64,                                                                             \
            signed long: float_compare_i64,                                                                            \
            signed long long: float_compare_i64,                                                                       \
            float: float_compare_f32,                                                                                  \
            double: float_compare_f64                                                                                  \
        )
#    define FLOAT_COMPARE_WITH_ERROR_DISPATCH(rhs)                                                                     \
        _Generic(                                                                                                      \
            (rhs),                                                                                                     \
            Float *: float_compare_with_error,                                                                         \
            Int *: float_compare_int_with_error,                                                                       \
            unsigned char: float_compare_u64_with_error,                                                               \
            unsigned short: float_compare_u64_with_error,                                                              \
            unsigned int: float_compare_u64_with_error,                                                                \
            unsigned long: float_compare_u64_with_error,                                                               \
            unsigned long long: float_compare_u64_with_error,                                                          \
            signed char: float_compare_i64_with_error,                                                                 \
            signed short: float_compare_i64_with_error,                                                                \
            signed int: float_compare_i64_with_error,                                                                  \
            signed long: float_compare_i64_with_error,                                                                 \
            signed long long: float_compare_i64_with_error,                                                            \
            float: float_compare_f32_with_error,                                                                       \
            double: float_compare_f64_with_error                                                                       \
        )
#    define FLOAT_COMPARE_SELECT(_1, _2, _3, NAME, ...) NAME

///
/// Compare a float against another numeric value.
/// Dispatches on the type of `rhs` to the matching internal handler.
///
/// lhs[in] : Left-hand float
/// rhs[in] : Right-hand operand (`Float`, `Int`, pointer, integer, or native float type)
/// error[out] : Optional error flag set to `true` on operational failure and
///              `false` otherwise.
///
/// RETURNS: `-1` if `lhs < rhs`, `0` if equal, `1` if `lhs > rhs`.
///
/// USAGE:
///   int cmp = FloatCompare(&value, 1.5);
///
/// TAGS: Float, Compare, Ordering, Generic
///
#    define FloatCompare(...)               FLOAT_COMPARE_SELECT(__VA_ARGS__, FloatCompare_3, FloatCompare_2)(__VA_ARGS__)
#    define FloatCompare_2(lhs, rhs)        FLOAT_COMPARE_DISPATCH(rhs)((lhs), (rhs))
#    define FloatCompare_3(lhs, rhs, error) FLOAT_COMPARE_WITH_ERROR_DISPATCH(rhs)((lhs), (rhs), (error))

///
/// Test whether two numeric values compare equal.
///
/// lhs[in] : Left-hand float
/// rhs[in] : Right-hand operand selected through generic dispatch
///
/// RETURNS: `true` when both values are equal.
///
/// USAGE:
///   if (FloatEQ(&value, 1.5)) { /* ... */ }
///
/// TAGS: Float, Compare, Equal, Generic
///
#    define FloatEQ(lhs, rhs) (FLOAT_COMPARE_DISPATCH(rhs)((lhs), (rhs)) == 0)
///
/// Test whether `lhs` is strictly less than `rhs`.
///
/// lhs[in] : Left-hand float
/// rhs[in] : Right-hand operand selected through generic dispatch
///
/// RETURNS: `true` when `lhs < rhs`.
///
/// USAGE:
///   bool smaller = FloatLT(&value, other);
///
/// TAGS: Float, Compare, LessThan, Generic
///
#    define FloatLT(lhs, rhs) (FLOAT_COMPARE_DISPATCH(rhs)((lhs), (rhs)) < 0)
///
/// Test whether `lhs` is less than or equal to `rhs`.
///
/// lhs[in] : Left-hand float
/// rhs[in] : Right-hand operand selected through generic dispatch
///
/// RETURNS: `true` when `lhs <= rhs`.
///
/// USAGE:
///   bool ok = FloatLE(&value, 0.0);
///
/// TAGS: Float, Compare, LessEqual, Generic
///
#    define FloatLE(lhs, rhs) (FLOAT_COMPARE_DISPATCH(rhs)((lhs), (rhs)) <= 0)
///
/// Test whether `lhs` is strictly greater than `rhs`.
///
/// lhs[in] : Left-hand float
/// rhs[in] : Right-hand operand selected through generic dispatch
///
/// RETURNS: `true` when `lhs > rhs`.
///
/// USAGE:
///   bool larger = FloatGT(&value, 1u);
///
/// TAGS: Float, Compare, GreaterThan, Generic
///
#    define FloatGT(lhs, rhs) (FLOAT_COMPARE_DISPATCH(rhs)((lhs), (rhs)) > 0)
///
/// Test whether `lhs` is greater than or equal to `rhs`.
///
/// lhs[in] : Left-hand float
/// rhs[in] : Right-hand operand selected through generic dispatch
///
/// RETURNS: `true` when `lhs >= rhs`.
///
/// USAGE:
///   bool at_least = FloatGE(&value, baseline);
///
/// TAGS: Float, Compare, GreaterEqual, Generic
///
#    define FloatGE(lhs, rhs) (FLOAT_COMPARE_DISPATCH(rhs)((lhs), (rhs)) >= 0)
///
/// Test whether two numeric values differ.
///
/// lhs[in] : Left-hand float
/// rhs[in] : Right-hand operand selected through generic dispatch
///
/// RETURNS: `true` when `lhs != rhs`.
///
/// USAGE:
///   bool changed = FloatNE(&value, 0.0f);
///
/// TAGS: Float, Compare, NotEqual, Generic
///
#    define FloatNE(lhs, rhs) (FLOAT_COMPARE_DISPATCH(rhs)((lhs), (rhs)) != 0)
#endif

#ifdef __cplusplus
}
#endif

#endif // MISRA_STD_CONTAINER_FLOAT_COMPARE_H
