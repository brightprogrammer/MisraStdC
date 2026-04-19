/// file      : std/container/int/convert.h
/// author    : Generated following Misra project patterns
/// This is free and unencumbered software released into the public domain.
///
/// Conversion helpers for Int.

#ifndef MISRA_STD_CONTAINER_INT_CONVERT_H
#define MISRA_STD_CONTAINER_INT_CONVERT_H

#include "Private.h"
#include <Misra/Std/Container/Str.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef __cplusplus
#    define MISRA_INT_FROM_DISPATCH(value)                                                                             \
        _Generic(                                                                                                      \
            (value),                                                                                                   \
            unsigned char: MISRA_PRIV_IntFromU64,                                                                      \
            unsigned short: MISRA_PRIV_IntFromU64,                                                                     \
            unsigned int: MISRA_PRIV_IntFromU64,                                                                       \
            unsigned long: MISRA_PRIV_IntFromU64,                                                                      \
            unsigned long long: MISRA_PRIV_IntFromU64,                                                                 \
            signed char: MISRA_PRIV_IntFromI64,                                                                        \
            signed short: MISRA_PRIV_IntFromI64,                                                                       \
            signed int: MISRA_PRIV_IntFromI64,                                                                         \
            signed long: MISRA_PRIV_IntFromI64,                                                                        \
            signed long long: MISRA_PRIV_IntFromI64                                                                    \
        )

///
/// Convert a native integer into an arbitrary-precision integer.
/// Dispatches on the type of `value`.
///
/// value[in] : Signed or unsigned integer source value
///
/// RETURNS: Integer holding the same non-negative value.
///
/// WARN: Aborts when a signed input is negative.
///
/// USAGE:
///   Int value = IntFrom(1234u);
///
/// TAGS: Int, Convert, Import, Generic
///
#    define IntFrom(value) MISRA_INT_FROM_DISPATCH(value)(value)
#endif

///
/// Convert an integer to `u64`.
///
/// value[in] : Integer to convert
///
/// RETURNS: The numeric value as `u64`.
///
/// WARN: Aborts if the integer does not fit in 64 bits.
///
/// USAGE:
///   u64 small = IntToU64(&value);
///
/// TAGS: Int, Convert, U64, Export
///
u64 IntToU64(Int *value);

///
/// Create an integer from little-endian bytes.
///
/// bytes[in] : Source byte buffer
/// len[in]   : Number of bytes to read
///
/// RETURNS: Integer decoded from the byte sequence.
///
/// USAGE:
///   Int value = IntFromBytesLE(buffer, buffer_len);
///
/// TAGS: Int, Convert, Bytes, LittleEndian, Import
///
Int IntFromBytesLE(const u8 *bytes, u64 len);

///
/// Export an integer into little-endian bytes.
///
/// value[in]    : Integer to export
/// bytes[out]   : Destination buffer
/// max_len[in]  : Maximum bytes to write
///
/// RETURNS: Number of bytes written. Large values are truncated to `max_len` bytes.
///
/// USAGE:
///   u64 written = IntToBytesLE(&value, buffer, sizeof(buffer));
///
/// TAGS: Int, Convert, Bytes, LittleEndian, Export
///
u64 IntToBytesLE(Int *value, u8 *bytes, u64 max_len);

///
/// Create an integer from big-endian bytes.
///
/// bytes[in] : Source byte buffer
/// len[in]   : Number of bytes to read
///
/// RETURNS: Integer decoded from the byte sequence.
///
/// USAGE:
///   Int value = IntFromBytesBE(buffer, buffer_len);
///
/// TAGS: Int, Convert, Bytes, BigEndian, Import
///
Int IntFromBytesBE(const u8 *bytes, u64 len);

///
/// Export an integer into big-endian bytes.
///
/// value[in]    : Integer to export
/// bytes[out]   : Destination buffer
/// max_len[in]  : Maximum bytes to write
///
/// RETURNS: Number of bytes written. Large values are truncated to the least-significant `max_len` bytes.
///
/// USAGE:
///   u64 written = IntToBytesBE(&value, buffer, sizeof(buffer));
///
/// TAGS: Int, Convert, Bytes, BigEndian, Export
///
u64 IntToBytesBE(Int *value, u8 *bytes, u64 max_len);

///
/// Parse digits in the given radix into an integer.
/// Supports radices from 2 through 36 and ignores underscore separators.
///
/// digits[in] : Input digit string
/// radix[in]  : Radix to use
///
/// RETURNS: Parsed integer value.
///
/// WARN: Aborts on invalid digits or unsupported radix.
///
/// USAGE:
///   Int value = IntFromStrRadix("ff", 16);
///
/// TAGS: Int, Convert, String, Radix, Import
///
Int IntFromStrRadix(const char *digits, u8 radix);

///
/// Convert an integer to text in the given radix.
///
/// value[in]      : Integer to convert
/// radix[in]      : Radix to use
/// uppercase[in]  : Use uppercase alphabetic digits when `true`
///
/// RETURNS: String containing the formatted digits.
///
/// USAGE:
///   Str text = IntToStrRadix(&value, 16, true);
///
/// TAGS: Int, Convert, String, Radix, Export
///
Str IntToStrRadix(Int *value, u8 radix, bool uppercase);

///
/// Parse a decimal string into an integer.
/// An optional leading `+` is accepted.
///
/// decimal[in] : Decimal digit string
///
/// RETURNS: Parsed integer value.
///
/// USAGE:
///   Int value = IntFromStr("18446744073709551616");
///
/// TAGS: Int, Convert, Decimal, String, Import
///
Int IntFromStr(const char *decimal);

///
/// Convert an integer to a decimal string.
///
/// value[in] : Integer to convert
///
/// RETURNS: Decimal representation of the integer.
///
/// USAGE:
///   Str text = IntToStr(&value);
///
/// TAGS: Int, Convert, Decimal, String, Export
///
Str IntToStr(Int *value);

///
/// Parse a binary string into an integer.
/// Accepts an optional `0b` or `0B` prefix.
///
/// binary[in] : Binary digit string
///
/// RETURNS: Parsed integer value.
///
/// USAGE:
///   Int value = IntFromBinary("0b101101");
///
/// TAGS: Int, Convert, Binary, String, Import
///
Int IntFromBinary(const char *binary);

///
/// Convert an integer to a binary string.
///
/// value[in] : Integer to convert
///
/// RETURNS: Base-2 representation without a prefix.
///
/// USAGE:
///   Str bits = IntToBinary(&value);
///
/// TAGS: Int, Convert, Binary, String, Export
///
Str IntToBinary(Int *value);

///
/// Parse an octal string into an integer.
/// Accepts an optional `0o` or `0O` prefix.
///
/// octal[in] : Octal digit string
///
/// RETURNS: Parsed integer value.
///
/// USAGE:
///   Int value = IntFromOctStr("0o755");
///
/// TAGS: Int, Convert, Octal, String, Import
///
Int IntFromOctStr(const char *octal);

///
/// Convert an integer to an octal string.
///
/// value[in] : Integer to convert
///
/// RETURNS: Base-8 representation without a prefix.
///
/// USAGE:
///   Str text = IntToOctStr(&value);
///
/// TAGS: Int, Convert, Octal, String, Export
///
Str IntToOctStr(Int *value);

///
/// Parse a hexadecimal string into an integer.
/// This parser expects hexadecimal digits only and does not accept a `0x` prefix.
///
/// hex[in] : Hexadecimal digit string
///
/// RETURNS: Parsed integer value.
///
/// WARN: Aborts on invalid characters.
///
/// USAGE:
///   Int value = IntFromHexStr("deadbeef");
///
/// TAGS: Int, Convert, Hex, String, Import
///
Int IntFromHexStr(const char *hex);

///
/// Convert an integer to a hexadecimal string.
///
/// value[in] : Integer to convert
///
/// RETURNS: Lowercase base-16 representation without a prefix.
///
/// USAGE:
///   Str text = IntToHexStr(&value);
///
/// TAGS: Int, Convert, Hex, String, Export
///
Str IntToHexStr(Int *value);

#ifdef __cplusplus
}
#endif

#endif // MISRA_STD_CONTAINER_INT_CONVERT_H
