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

///
/// Get the number of significant bits in an integer.
///
/// value[in] : Integer to inspect
///
/// RETURNS: Number of significant bits, or `0` for zero.
///
/// USAGE:
///   u64 bits = IntBitLength(&value);
///
/// TAGS: Int, Access, Bits, Length
///
u64  IntBitLength(Int *value);

///
/// Get the minimum number of bytes needed to encode an integer.
///
/// value[in] : Integer to inspect
///
/// RETURNS: Number of bytes required to represent the value, or `0` for zero.
///
/// USAGE:
///   u64 bytes = IntByteLength(&value);
///
/// TAGS: Int, Access, Bytes, Length
///
u64  IntByteLength(Int *value);

///
/// Compute `floor(log2(value))`.
///
/// value[in] : Integer to inspect
///
/// RETURNS: Index of the highest set bit.
///
/// WARN: Undefined for zero. This function aborts when called on zero.
///
/// USAGE:
///   u64 msb = IntLog2(&value);
///
/// TAGS: Int, Access, Log2, Bits
///
u64  IntLog2(Int *value);

///
/// Count trailing zero bits in the integer representation.
///
/// value[in] : Integer to inspect
///
/// RETURNS: Number of consecutive zero bits starting at bit 0.
///
/// USAGE:
///   u64 tz = IntTrailingZeroCount(&value);
///
/// TAGS: Int, Access, TrailingZeroes, Bits
///
u64  IntTrailingZeroCount(Int *value);

///
/// Test whether the integer is zero.
///
/// value[in] : Integer to test
///
/// RETURNS: `true` when the integer is zero.
///
/// USAGE:
///   if (IntIsZero(&value)) { /* ... */ }
///
/// TAGS: Int, Access, Zero, Predicate
///
bool IntIsZero(Int *value);

///
/// Test whether the integer is exactly one.
///
/// value[in] : Integer to test
///
/// RETURNS: `true` when the integer equals `1`.
///
/// USAGE:
///   bool one = IntIsOne(&value);
///
/// TAGS: Int, Access, One, Predicate
///
bool IntIsOne(Int *value);

///
/// Test whether the integer is even.
///
/// value[in] : Integer to test
///
/// RETURNS: `true` when the least-significant bit is zero.
///
/// USAGE:
///   bool even = IntIsEven(&value);
///
/// TAGS: Int, Access, Even, Predicate
///
bool IntIsEven(Int *value);

///
/// Test whether the integer is odd.
///
/// value[in] : Integer to test
///
/// RETURNS: `true` when the least-significant bit is one.
///
/// USAGE:
///   bool odd = IntIsOdd(&value);
///
/// TAGS: Int, Access, Odd, Predicate
///
bool IntIsOdd(Int *value);

///
/// Test whether the integer can be losslessly converted to `u64`.
///
/// value[in] : Integer to test
///
/// RETURNS: `true` when the value fits in 64 unsigned bits.
///
/// USAGE:
///   if (IntFitsU64(&value)) { /* safe to call IntToU64 */ }
///
/// TAGS: Int, Access, Convert, Range
///
bool IntFitsU64(Int *value);

///
/// Test whether the integer is a power of two.
///
/// value[in] : Integer to test
///
/// RETURNS: `true` when exactly one bit is set and the value is non-zero.
///
/// USAGE:
///   bool pow2 = IntIsPowerOfTwo(&value);
///
/// TAGS: Int, Access, PowerOfTwo, Predicate
///
bool IntIsPowerOfTwo(Int *value);

#ifdef __cplusplus
}
#endif

#endif // MISRA_STD_CONTAINER_INT_ACCESS_H
