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
            unsigned char: IntFromU64,                                                                                 \
            unsigned short: IntFromU64,                                                                                \
            unsigned int: IntFromU64,                                                                                  \
            unsigned long: IntFromU64,                                                                                 \
            unsigned long long: IntFromU64,                                                                            \
            signed char: IntFromI64,                                                                                   \
            signed short: IntFromI64,                                                                                  \
            signed int: IntFromI64,                                                                                    \
            signed long: IntFromI64,                                                                                   \
            signed long long: IntFromI64                                                                               \
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
/// Try to convert an integer to `u64`.
///
/// value[in] : Integer to convert
/// out[out]  : Destination for the converted value
///
/// RETURNS: `true` on success, `false` when the value does not fit in 64 bits.
///
bool IntTryToU64(Int *value, u64 *out);

///
/// Convert an integer to `u64`.
///
/// value[in]  : Integer to convert
/// error[out] : Optional pointer set to `true` on failure and `false` on success
///
/// RETURNS: The numeric value as `u64`, or `0` on failure.
///
u64 IntToU64WithError(Int *value, bool *error);

///
/// Create an integer from little-endian bytes.
///
Int IntFromBytesLE(const u8 *bytes, u64 len);

///
/// Export an integer into little-endian bytes.
///
u64 IntToBytesLE(Int *value, u8 *bytes, u64 max_len);

///
/// Create an integer from big-endian bytes.
///
Int IntFromBytesBE(const u8 *bytes, u64 len);

///
/// Export an integer into big-endian bytes.
///
u64 IntToBytesBE(Int *value, u8 *bytes, u64 max_len);

///
/// Parse digits in the given radix into an integer.
/// Supports radices from 2 through 36 and ignores underscore separators.
///
bool IntTryFromStrRadix(Int *out, const char *digits, u8 radix);

///
/// Compatibility wrapper for `IntTryFromStrRadix(...)`.
///
/// RETURNS: Parsed integer value, or zero on failure.
///
Int IntFromStrRadix(const char *digits, u8 radix);

///
/// Convert an integer to text in the given radix.
///
Str IntToStrRadix(Int *value, u8 radix, bool uppercase);

///
/// Parse a decimal string into an integer.
/// An optional leading `+` is accepted.
///
bool IntTryFromStr(Int *out, const char *decimal);

///
/// Compatibility wrapper for `IntTryFromStr(...)`.
///
/// RETURNS: Parsed integer value, or zero on failure.
///
Int IntFromStr(const char *decimal);

///
/// Convert an integer to a decimal string.
///
Str IntToStr(Int *value);

///
/// Parse a binary string into an integer.
/// Accepts an optional `0b` or `0B` prefix.
///
bool IntTryFromBinary(Int *out, const char *binary);

///
/// Compatibility wrapper for `IntTryFromBinary(...)`.
///
/// RETURNS: Parsed integer value, or zero on failure.
///
Int IntFromBinary(const char *binary);

///
/// Convert an integer to a binary string.
///
Str IntToBinary(Int *value);

///
/// Parse an octal string into an integer.
/// Accepts an optional `0o` or `0O` prefix.
///
bool IntTryFromOctStr(Int *out, const char *octal);

///
/// Compatibility wrapper for `IntTryFromOctStr(...)`.
///
/// RETURNS: Parsed integer value, or zero on failure.
///
Int IntFromOctStr(const char *octal);

///
/// Convert an integer to an octal string.
///
Str IntToOctStr(Int *value);

///
/// Parse a hexadecimal string into an integer.
/// This parser expects hexadecimal digits only and does not accept a `0x` prefix.
///
bool IntTryFromHexStr(Int *out, const char *hex);

///
/// Compatibility wrapper for `IntTryFromHexStr(...)`.
///
/// RETURNS: Parsed integer value, or zero on failure.
///
Int IntFromHexStr(const char *hex);

///
/// Convert an integer to a hexadecimal string.
///
Str IntToHexStr(Int *value);

#ifdef __cplusplus
}
#endif

static inline u64 MISRA_PRIV_IntToU64NoError(Int *value) {
    return IntToU64WithError(value, NULL);
}

#define MISRA_PRIV_INT_TO_U64_SELECT(_1, _2, NAME, ...) NAME
#define IntToU64(...) MISRA_PRIV_INT_TO_U64_SELECT(__VA_ARGS__, IntToU64WithError, MISRA_PRIV_IntToU64NoError)(__VA_ARGS__)

#endif // MISRA_STD_CONTAINER_INT_CONVERT_H
