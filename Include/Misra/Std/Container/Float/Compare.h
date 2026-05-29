/// file      : std/container/float/compare.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
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
    /// Compare two arbitrary-precision floating-point values. The
    /// no-error variant matches the `GenericCompare` shape so it drops
    /// straight into map / vec compare slots; the `*_with_error` variant
    /// keeps the typed signature plus an out-of-band failure flag.
    ///
    /// lhs[in]    : Left-hand operand (pointer to `Float`).
    /// rhs[in]    : Right-hand operand (pointer to `Float`).
    /// error[out] : (`*_with_error` only) Optional pointer set to
    ///              `true` on operational failure and `false` on
    ///              success.
    ///
    /// SUCCESS : Returns `-1` if `lhs < rhs`, `0` if equal, `1` if
    ///           `lhs > rhs`. Neither operand is modified.
    /// FAILURE : Returns `0` when an intermediate rescale allocation
    ///           fails. The `*_with_error` variant sets `*error` to
    ///           `true` so the caller can distinguish failure from a
    ///           true equality; the no-error variant cannot.
    ///
    /// USAGE:
    ///   int cmp = FloatCompare(&a, &b);
    ///
    /// TAGS: Float, Compare, Ordering, GenericCompare
    ///
    int float_compare_with_error(const Float *lhs, const Float *rhs, bool *error);
    i32 float_compare(const void *lhs, const void *rhs);

    ///
    /// Hash a `Float` for use as a map key. FNV-1a over the significand
    /// magnitude bytes, the exponent, and the sign so `+1.5e3`,
    /// `-1.5e3`, and `1.5e2` land in different buckets. Matches the
    /// `GenericHash` shape so it drops straight into map / vec hash
    /// slots.
    ///
    /// data[in] : Pointer to the `Float` to hash.
    /// size[in] : Ignored. Included for `GenericHash` callback
    ///            compatibility; the value's real length lives inside
    ///            the `Float` header itself.
    ///
    /// SUCCESS : Returns a stable hash of the float's representation.
    ///           `data` is not modified.
    /// FAILURE : Function cannot fail.
    ///
    /// USAGE:
    ///   Map(Float, u64) counts = MapInit(float_hash, float_compare, alloc);
    ///
    /// TAGS: Float, Hash, GenericHash
    ///
    u64 float_hash(const void *data, u32 size);

#ifndef __cplusplus
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
/// SUCCESS : Returns `-1` if `lhs < rhs`, `0` if equal, `1` if `lhs > rhs`.
/// FAILURE : Returns `0` when an intermediate conversion of `rhs` to
///           `Float` fails. With the two-argument form a failure result
///           is indistinguishable from a true equality; with the
///           three-argument form `*error` is set to `true`.
///
/// USAGE:
///   int cmp = FloatCompare(&value, 1.5);
///
/// TAGS: Float, Compare, Ordering, Generic
///
#    define FloatCompare(...) FLOAT_COMPARE_SELECT(__VA_ARGS__, FloatCompare_3, FloatCompare_2)(__VA_ARGS__)
#    define FloatCompare_2(lhs, rhs)                                                                                   \
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
        )((lhs), (rhs))
#    define FloatCompare_3(lhs, rhs, error)                                                                            \
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
        )((lhs), (rhs), (error))

///
/// Test whether two numeric values compare equal.
///
/// lhs[in] : Left-hand float
/// rhs[in] : Right-hand operand selected through generic dispatch
///
/// SUCCESS : Returns `true` when both values are equal.
/// FAILURE : Returns `false` otherwise. Neither operand is modified.
///
/// USAGE:
///   if (FloatEQ(&value, 1.5)) { /* ... */ }
///
/// TAGS: Float, Compare, Equal, Generic
///
#    define FloatEQ(lhs, rhs) (FloatCompare_2((lhs), (rhs)) == 0)
///
/// Test whether `lhs` is strictly less than `rhs`.
///
/// lhs[in] : Left-hand float
/// rhs[in] : Right-hand operand selected through generic dispatch
///
/// SUCCESS : Returns `true` when `lhs < rhs`.
/// FAILURE : Returns `false` otherwise. Neither operand is modified.
///
/// USAGE:
///   bool smaller = FloatLT(&value, other);
///
/// TAGS: Float, Compare, LessThan, Generic
///
#    define FloatLT(lhs, rhs) (FloatCompare_2((lhs), (rhs)) < 0)
///
/// Test whether `lhs` is less than or equal to `rhs`.
///
/// lhs[in] : Left-hand float
/// rhs[in] : Right-hand operand selected through generic dispatch
///
/// SUCCESS : Returns `true` when `lhs <= rhs`.
/// FAILURE : Returns `false` otherwise. Neither operand is modified.
///
/// USAGE:
///   bool ok = FloatLE(&value, 0.0);
///
/// TAGS: Float, Compare, LessEqual, Generic
///
#    define FloatLE(lhs, rhs) (FloatCompare_2((lhs), (rhs)) <= 0)
///
/// Test whether `lhs` is strictly greater than `rhs`.
///
/// lhs[in] : Left-hand float
/// rhs[in] : Right-hand operand selected through generic dispatch
///
/// SUCCESS : Returns `true` when `lhs > rhs`.
/// FAILURE : Returns `false` otherwise. Neither operand is modified.
///
/// USAGE:
///   bool larger = FloatGT(&value, 1u);
///
/// TAGS: Float, Compare, GreaterThan, Generic
///
#    define FloatGT(lhs, rhs) (FloatCompare_2((lhs), (rhs)) > 0)
///
/// Test whether `lhs` is greater than or equal to `rhs`.
///
/// lhs[in] : Left-hand float
/// rhs[in] : Right-hand operand selected through generic dispatch
///
/// SUCCESS : Returns `true` when `lhs >= rhs`.
/// FAILURE : Returns `false` otherwise. Neither operand is modified.
///
/// USAGE:
///   bool at_least = FloatGE(&value, baseline);
///
/// TAGS: Float, Compare, GreaterEqual, Generic
///
#    define FloatGE(lhs, rhs) (FloatCompare_2((lhs), (rhs)) >= 0)
///
/// Test whether two numeric values differ.
///
/// lhs[in] : Left-hand float
/// rhs[in] : Right-hand operand selected through generic dispatch
///
/// SUCCESS : Returns `true` when `lhs != rhs`.
/// FAILURE : Returns `false` otherwise. Neither operand is modified.
///
/// USAGE:
///   bool changed = FloatNE(&value, 0.0f);
///
/// TAGS: Float, Compare, NotEqual, Generic
///
#    define FloatNE(lhs, rhs) (FloatCompare_2((lhs), (rhs)) != 0)
#endif

#ifdef __cplusplus
}
#endif

#endif // MISRA_STD_CONTAINER_FLOAT_COMPARE_H
