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
int (FloatCompare)(Float *lhs, Float *rhs);
#ifndef __cplusplus
#    define MISRA_FLOAT_COMPARE_DISPATCH(rhs)                                                                          \
        _Generic(                                                                                                      \
            (rhs),                                                                                                     \
            Float: MISRA_PRIV_FloatCompareValueFloat,                                                                  \
            Float *: FloatCompare,                                                                                     \
            const Float *: MISRA_PRIV_FloatCompareConstFloat,                                                          \
            Int: MISRA_PRIV_FloatCompareValueInt,                                                                      \
            Int *: MISRA_PRIV_FloatCompareInt,                                                                         \
            const Int *: MISRA_PRIV_FloatCompareConstInt,                                                              \
            unsigned char: MISRA_PRIV_FloatCompareU64,                                                                 \
            unsigned short: MISRA_PRIV_FloatCompareU64,                                                                \
            unsigned int: MISRA_PRIV_FloatCompareU64,                                                                  \
            unsigned long: MISRA_PRIV_FloatCompareU64,                                                                 \
            unsigned long long: MISRA_PRIV_FloatCompareU64,                                                            \
            signed char: MISRA_PRIV_FloatCompareI64,                                                                   \
            signed short: MISRA_PRIV_FloatCompareI64,                                                                  \
            signed int: MISRA_PRIV_FloatCompareI64,                                                                    \
            signed long: MISRA_PRIV_FloatCompareI64,                                                                   \
            signed long long: MISRA_PRIV_FloatCompareI64,                                                              \
            float: MISRA_PRIV_FloatCompareF32,                                                                         \
            double: MISRA_PRIV_FloatCompareF64                                                                         \
        )

///
/// Compare a float against another numeric value.
/// Dispatches on the type of `rhs` to the matching internal handler.
///
/// lhs[in] : Left-hand float
/// rhs[in] : Right-hand operand (`Float`, `Int`, pointer, integer, or native float type)
///
/// RETURNS: `-1` if `lhs < rhs`, `0` if equal, `1` if `lhs > rhs`.
///
/// USAGE:
///   int cmp = FloatCompare(&value, 1.5);
///
/// TAGS: Float, Compare, Ordering, Generic
///
#    define FloatCompare(lhs, rhs) MISRA_FLOAT_COMPARE_DISPATCH(rhs)((lhs), (rhs))

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
#    define FloatEQ(lhs, rhs) (MISRA_FLOAT_COMPARE_DISPATCH(rhs)((lhs), (rhs)) == 0)
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
#    define FloatLT(lhs, rhs) (MISRA_FLOAT_COMPARE_DISPATCH(rhs)((lhs), (rhs)) < 0)
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
#    define FloatLE(lhs, rhs) (MISRA_FLOAT_COMPARE_DISPATCH(rhs)((lhs), (rhs)) <= 0)
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
#    define FloatGT(lhs, rhs) (MISRA_FLOAT_COMPARE_DISPATCH(rhs)((lhs), (rhs)) > 0)
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
#    define FloatGE(lhs, rhs) (MISRA_FLOAT_COMPARE_DISPATCH(rhs)((lhs), (rhs)) >= 0)
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
#    define FloatNE(lhs, rhs) (MISRA_FLOAT_COMPARE_DISPATCH(rhs)((lhs), (rhs)) != 0)
#endif

#ifdef __cplusplus
}
#endif

#endif // MISRA_STD_CONTAINER_FLOAT_COMPARE_H
