/// file      : std/container/int/access.h
/// author    : Generated following Misra project patterns
/// This is free and unencumbered software released into the public domain.
///
/// Numeric state inspection helpers for Int.

#ifndef MISRA_STD_CONTAINER_INT_ACCESS_H
#define MISRA_STD_CONTAINER_INT_ACCESS_H

#include "Type.h"

#ifdef __cplusplus
extern "C" {
#endif

    u64 IntBitLength(Int *value);
    u64 IntByteLength(Int *value);

    ///
    /// Try to compute `floor(log2(value))`.
    ///
    /// SUCCESS : Returns `true`. The result has been computed and the
    ///           destination object updated.
    /// FAILURE : Returns `false` when `value` is zero. The destination is left
    ///           untouched.
    ///
    bool IntTryLog2(Int *value, u64 *out);

    ///
    /// Compute `floor(log2(value))`.
    ///
    /// value[in]  : Integer to inspect
    /// error[out] : Optional pointer set to `true` on failure and `false` on success
    ///
    /// SUCCESS : Returns Index of the highest set bit, or `0` on failure.
    ///
    u64 IntLog2WithError(Int *value, bool *error);

    u64  IntTrailingZeroCount(Int *value);
    bool IntIsZero(Int *value);
    bool IntIsOne(Int *value);
    bool IntIsEven(Int *value);
    bool IntIsOdd(Int *value);
    bool IntFitsU64(Int *value);
    bool IntIsPowerOfTwo(Int *value);

#ifdef __cplusplus
}
#endif

static inline u64 int_log2_no_error(Int *value) {
    return IntLog2WithError(value, NULL);
}

#define INT_LOG2_SELECT(_1, _2, NAME, ...) NAME
#define IntLog2(...)                       INT_LOG2_SELECT(__VA_ARGS__, IntLog2WithError, int_log2_no_error)(__VA_ARGS__)

#endif // MISRA_STD_CONTAINER_INT_ACCESS_H
