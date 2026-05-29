/// file      : std/container/int/compare.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Comparison helpers for Int.

#ifndef MISRA_STD_CONTAINER_INT_COMPARE_H
#define MISRA_STD_CONTAINER_INT_COMPARE_H

#include "Private.h"

#ifdef __cplusplus
extern "C" {
#endif

    ///
    /// Compare two arbitrary-precision integers. Matches the
    /// `GenericCompare` shape so it drops straight into map / vec
    /// compare slots.
    ///
    /// lhs[in] : Left-hand operand (pointer to `Int`).
    /// rhs[in] : Right-hand operand (pointer to `Int`).
    ///
    /// SUCCESS : Returns `-1` if `lhs < rhs`, `0` if equal, `1` if
    ///           `lhs > rhs`. Neither operand is modified.
    /// FAILURE : Function cannot fail. A `NULL` operand is a caller bug
    ///           and aborts via `LOG_FATAL` from `ValidateInt`.
    ///
    /// USAGE:
    ///   int cmp = IntCompare(&a, &b);
    ///
    /// TAGS: Int, Compare, Ordering, GenericCompare
    ///
    i32 int_compare(const void *lhs, const void *rhs);

    ///
    /// Hash an `Int` for use as a map key. FNV-1a over the magnitude
    /// bytes. Matches the `GenericHash` shape so it drops straight into
    /// map / vec hash slots.
    ///
    /// data[in] : Pointer to the `Int` to hash.
    /// size[in] : Ignored. Included for `GenericHash` callback
    ///            compatibility; the value's real length lives inside
    ///            the `Int` header itself.
    ///
    /// SUCCESS : Returns a stable hash of the integer's magnitude.
    ///           `data` is not modified.
    /// FAILURE : Function cannot fail.
    ///
    /// USAGE:
    ///   Map(Int, u64) counts = MapInit(int_hash, int_compare, alloc);
    ///
    /// TAGS: Int, Hash, GenericHash
    ///
    u64 int_hash(const void *data, u32 size);

#ifndef __cplusplus
///
/// Compare an integer against another integer-like value.
/// Dispatches on the type of `rhs` to the matching internal handler.
///
/// lhs[in] : Left-hand integer
/// rhs[in] : Right-hand operand (`Int`, pointer, `u64`, or `i64` compatible type)
///
/// SUCCESS : Returns `-1` if `lhs < rhs`, `0` if equal, `1` if `lhs > rhs`.
/// FAILURE : Pure comparison cannot fail. `LOG_FATAL` if either operand
///           pointer is NULL (propagated from the underlying handler's
///           `ValidateInt`).
///
/// USAGE:
///   int cmp = IntCompare(&value, 42);
///
/// TAGS: Int, Compare, Ordering, Generic
///
#    define IntCompare(lhs, rhs)                                                                                       \
        _Generic(                                                                                                      \
            (rhs),                                                                                                     \
            Int *: int_compare,                                                                                        \
            unsigned char: int_compare_u64,                                                                            \
            unsigned short: int_compare_u64,                                                                           \
            unsigned int: int_compare_u64,                                                                             \
            unsigned long: int_compare_u64,                                                                            \
            unsigned long long: int_compare_u64,                                                                       \
            signed char: int_compare_i64,                                                                              \
            signed short: int_compare_i64,                                                                             \
            signed int: int_compare_i64,                                                                               \
            signed long: int_compare_i64,                                                                              \
            signed long long: int_compare_i64                                                                          \
        )((lhs), (rhs))

///
/// Test whether two numeric values compare equal.
/// The right-hand operand may be an `Int`, `Int*`, `const Int*`, `u64`, or `i64` compatible type.
///
/// lhs[in] : Left-hand integer
/// rhs[in] : Right-hand operand selected through generic dispatch
///
/// SUCCESS : Returns `true` when both values are equal.
/// FAILURE : Returns `false` otherwise.
///
/// USAGE:
///   if (IntEQ(&value, 10u)) { /* ... */ }
///
/// TAGS: Int, Compare, Equal, Generic
///
#    define IntEQ(lhs, rhs) (IntCompare((lhs), (rhs)) == 0)
///
/// Test whether `lhs` is strictly less than `rhs`.
///
/// lhs[in] : Left-hand integer
/// rhs[in] : Right-hand operand selected through generic dispatch
///
/// SUCCESS : Returns `true` when `lhs < rhs`.
/// FAILURE : Returns `false` otherwise.
///
/// USAGE:
///   bool smaller = IntLT(&value, other);
///
/// TAGS: Int, Compare, LessThan, Generic
///
#    define IntLT(lhs, rhs) (IntCompare((lhs), (rhs)) < 0)
///
/// Test whether `lhs` is less than or equal to `rhs`.
///
/// lhs[in] : Left-hand integer
/// rhs[in] : Right-hand operand selected through generic dispatch
///
/// SUCCESS : Returns `true` when `lhs <= rhs`.
/// FAILURE : Returns `false` otherwise.
///
/// USAGE:
///   bool ok = IntLE(&value, 1024u);
///
/// TAGS: Int, Compare, LessEqual, Generic
///
#    define IntLE(lhs, rhs) (IntCompare((lhs), (rhs)) <= 0)
///
/// Test whether `lhs` is strictly greater than `rhs`.
///
/// lhs[in] : Left-hand integer
/// rhs[in] : Right-hand operand selected through generic dispatch
///
/// SUCCESS : Returns `true` when `lhs > rhs`.
/// FAILURE : Returns `false` otherwise.
///
/// USAGE:
///   bool larger = IntGT(&value, 0u);
///
/// TAGS: Int, Compare, GreaterThan, Generic
///
#    define IntGT(lhs, rhs) (IntCompare((lhs), (rhs)) > 0)
///
/// Test whether `lhs` is greater than or equal to `rhs`.
///
/// lhs[in] : Left-hand integer
/// rhs[in] : Right-hand operand selected through generic dispatch
///
/// SUCCESS : Returns `true` when `lhs >= rhs`.
/// FAILURE : Returns `false` otherwise.
///
/// USAGE:
///   bool at_least = IntGE(&value, threshold);
///
/// TAGS: Int, Compare, GreaterEqual, Generic
///
#    define IntGE(lhs, rhs) (IntCompare((lhs), (rhs)) >= 0)
///
/// Test whether two numeric values differ.
///
/// lhs[in] : Left-hand integer
/// rhs[in] : Right-hand operand selected through generic dispatch
///
/// SUCCESS : Returns `true` when `lhs != rhs`.
/// FAILURE : Returns `false` otherwise.
///
/// USAGE:
///   bool changed = IntNE(&value, expected);
///
/// TAGS: Int, Compare, NotEqual, Generic
///
#    define IntNE(lhs, rhs) (IntCompare((lhs), (rhs)) != 0)
#endif

#ifdef __cplusplus
}
#endif

#endif // MISRA_STD_CONTAINER_INT_COMPARE_H
