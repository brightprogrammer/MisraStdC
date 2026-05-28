/// file      : std/container/int/access.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Numeric state inspection helpers for Int.

#ifndef MISRA_STD_CONTAINER_INT_ACCESS_H
#define MISRA_STD_CONTAINER_INT_ACCESS_H

#include "Type.h"

#ifdef __cplusplus
extern "C" {
#endif

    ///
    /// Number of significant bits required to represent the magnitude of an
    /// integer.
    ///
    /// value[in] : Integer to inspect.
    ///
    /// SUCCESS : Returns the bit length as a `u64`. Returns `0` when
    ///           `value` is zero. The integer is not modified.
    /// FAILURE : Function cannot fail. An invalid `value` pointer is a
    ///           caller bug and aborts via `LOG_FATAL`.
    ///
    /// TAGS: Int, Access, BitLength
    ///
    u64 IntBitLength(const Int *value);

    ///
    /// Number of bytes required to store the integer (ceil of bit length over
    /// eight).
    ///
    /// value[in] : Integer to inspect.
    ///
    /// SUCCESS : Returns the byte length as a `u64`. Returns `0` when
    ///           `value` is zero. The integer is not modified.
    /// FAILURE : Function cannot fail.
    ///
    /// TAGS: Int, Access, ByteLength
    ///
    u64 IntByteLength(const Int *value);

    ///
    /// Try to compute `floor(log2(value))`.
    ///
    /// SUCCESS : Returns `true`. The result has been computed and the
    ///           destination object updated.
    /// FAILURE : Returns `false` when `value` is zero. The destination is left
    ///           untouched.
    ///
    /// TAGS: Int, Math, Log2, Access
    ///
    bool IntTryLog2(const Int *value, u64 *out);

    ///
    /// Compute `floor(log2(value))`.
    ///
    /// value[in]  : Integer to inspect
    /// error[out] : Optional pointer set to `true` on failure and `false` on success
    ///
    /// SUCCESS : Returns Index of the highest set bit, or `0` on failure.
    /// FAILURE : Returns `0` when `value` is zero. `*error` (if non-NULL)
    ///           is set to `true`.
    ///
    /// TAGS: Int, Math, Log2, Access
    ///
    u64 IntLog2WithError(const Int *value, bool *error);

    ///
    /// Number of trailing zero bits (equivalent to the largest power of two
    /// that divides the value).
    ///
    /// value[in] : Integer to inspect.
    ///
    /// SUCCESS : Returns the trailing-zero count as a `u64`. Returns `0`
    ///           when `value` is zero. The integer is not modified.
    /// FAILURE : Function cannot fail.
    ///
    /// TAGS: Int, Access, TrailingZeros
    ///
    u64 IntTrailingZeroCount(const Int *value);

    ///
    /// Test whether the integer equals zero.
    ///
    /// value[in] : Integer to inspect.
    ///
    /// SUCCESS : Returns `true` when the integer represents zero.
    /// FAILURE : Returns `false` for any non-zero value.
    ///
    /// TAGS: Int, Access, Predicate, IsZero
    ///
    bool IntIsZero(const Int *value);

    ///
    /// Test whether the integer equals one.
    ///
    /// value[in] : Integer to inspect.
    ///
    /// SUCCESS : Returns `true` when the integer equals one.
    /// FAILURE : Returns `false` otherwise.
    ///
    /// TAGS: Int, Access, Predicate, IsOne
    ///
    bool IntIsOne(const Int *value);

    ///
    /// Test whether the integer's value is even.
    ///
    /// value[in] : Integer to inspect.
    ///
    /// SUCCESS : Returns `true` when the integer is even (including zero).
    /// FAILURE : Returns `false` when the integer is odd.
    ///
    /// TAGS: Int, Access, Predicate, IsEven
    ///
    bool IntIsEven(const Int *value);

    ///
    /// Test whether the integer's value is odd.
    ///
    /// value[in] : Integer to inspect.
    ///
    /// SUCCESS : Returns `true` when the integer is odd.
    /// FAILURE : Returns `false` when the integer is even (including zero).
    ///
    /// TAGS: Int, Access, Predicate, IsOdd
    ///
    bool IntIsOdd(const Int *value);

    ///
    /// Test whether the integer's magnitude fits in a 64-bit unsigned value.
    ///
    /// value[in] : Integer to inspect.
    ///
    /// SUCCESS : Returns `true` when `value <= UINT64_MAX`.
    /// FAILURE : Returns `false` when the value exceeds 64 bits.
    ///
    /// TAGS: Int, Access, Predicate, FitsU64
    ///
    bool IntFitsU64(const Int *value);

    ///
    /// Test whether the integer is a power of two (1, 2, 4, 8, ...).
    ///
    /// value[in] : Integer to inspect.
    ///
    /// SUCCESS : Returns `true` when the integer is a power of two.
    /// FAILURE : Returns `false` for zero or non-power-of-two values.
    ///
    /// TAGS: Int, Access, Predicate, PowerOfTwo
    ///
    bool IntIsPowerOfTwo(const Int *value);

#ifdef __cplusplus
}
#endif

static inline u64 int_log2_no_error(const Int *value) {
    return IntLog2WithError(value, NULL);
}

#define INT_LOG2_SELECT(_1, _2, NAME, ...) NAME

///
/// Compute `floor(log2(value))`.
///
/// This public macro supports both forms:
///
/// - `IntLog2(value)`               - returns the result, no error channel.
/// - `IntLog2(value, error)`        - writes the error flag through `error`.
///
/// value[in]  : Integer to inspect.
/// error[out] : Optional pointer set to `true` on failure and `false` on success.
///
/// SUCCESS : Returns the zero-based index of the highest set bit as a `u64`.
///           The integer is not modified.
/// FAILURE : Returns `0` when `value` is zero. With the two-argument form
///           `*error` is set to `true`; with the one-argument form the
///           caller cannot distinguish zero-input from log2(1).
///
/// USAGE:
///   u64 hi = IntLog2(&value);
///
/// TAGS: Int, Access, Log2, Macro
///
#define IntLog2(...) INT_LOG2_SELECT(__VA_ARGS__, IntLog2WithError, int_log2_no_error)(__VA_ARGS__)

#endif // MISRA_STD_CONTAINER_INT_ACCESS_H
