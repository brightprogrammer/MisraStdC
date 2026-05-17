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
#    define INT_FROM_DISPATCH(value)                                                                                   \
        _Generic(                                                                                                      \
            (value),                                                                                                   \
            unsigned char: int_from_u64,                                                                               \
            unsigned short: int_from_u64,                                                                              \
            unsigned int: int_from_u64,                                                                                \
            unsigned long: int_from_u64,                                                                               \
            unsigned long long: int_from_u64,                                                                          \
            signed char: int_from_i64,                                                                                 \
            signed short: int_from_i64,                                                                                \
            signed int: int_from_i64,                                                                                  \
            signed long: int_from_i64,                                                                                 \
            signed long long: int_from_i64                                                                             \
        )

///
/// Convert a native integer into an arbitrary-precision integer.
/// Dispatches on the type of `value`.
///
/// value[in] : Signed or unsigned integer source value
///
/// SUCCESS : Returns Integer holding the same non-negative value.
///
/// WARN: Aborts when a signed input is negative.
///
/// USAGE:
///   Int value = IntFrom(1234u);
///
/// TAGS: Int, Convert, Import, Generic
///
#    define IntFrom(value, alloc) INT_FROM_DISPATCH(value)((value), (alloc))
#endif

    ///
    /// Try to convert an integer to `u64`.
    ///
    /// value[in] : Integer to convert
    /// out[out]  : Destination for the converted value
    ///
    /// SUCCESS : Returns `true`. The result has been computed and the
    ///           destination object updated.
    /// FAILURE : Returns `false` when the value does not fit in 64 bits. The destination is left
    ///           untouched.
    ///
    bool IntTryToU64(Int *value, u64 *out);

    ///
    /// Convert an integer to `u64`.
    ///
    /// value[in]  : Integer to convert
    /// error[out] : Optional pointer set to `true` on failure and `false` on success
    ///
    /// SUCCESS : Returns The numeric value as `u64`, or `0` on failure.
    ///
    u64 IntToU64WithError(Int *value, bool *error);

    ///
    /// Create an integer from little-endian bytes.
    ///
    Int int_from_bytes_le(const u8 *bytes, u64 len, Allocator *alloc);
#define IntFromBytesLE(...)                MISRA_OVERLOAD(IntFromBytesLE, __VA_ARGS__)
#define IntFromBytesLE_2(bytes, len)       int_from_bytes_le((bytes), (len), MisraScope)
#define IntFromBytesLE_3(bytes, len, alloc) int_from_bytes_le((bytes), (len), ALLOCATOR_OF(alloc))

    ///
    /// Export an integer into little-endian bytes.
    ///
    u64 IntToBytesLE(Int *value, u8 *bytes, u64 max_len);

    ///
    /// Create an integer from big-endian bytes.
    ///
    Int int_from_bytes_be(const u8 *bytes, u64 len, Allocator *alloc);
#define IntFromBytesBE(...)                 MISRA_OVERLOAD(IntFromBytesBE, __VA_ARGS__)
#define IntFromBytesBE_2(bytes, len)        int_from_bytes_be((bytes), (len), MisraScope)
#define IntFromBytesBE_3(bytes, len, alloc) int_from_bytes_be((bytes), (len), ALLOCATOR_OF(alloc))

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
    /// SUCCESS : Returns Parsed integer value, or zero on failure.
    ///
    Int int_from_str_radix(const char *digits, u8 radix, Allocator *alloc);
#define IntFromStrRadix(...)                  MISRA_OVERLOAD(IntFromStrRadix, __VA_ARGS__)
#define IntFromStrRadix_2(digits, radix)        int_from_str_radix((digits), (radix), MisraScope)
#define IntFromStrRadix_3(digits, radix, alloc) int_from_str_radix((digits), (radix), ALLOCATOR_OF(alloc))

    ///
    /// Convert an integer to text in the given radix using an explicit allocator.
    ///
    /// out[out]      : Destination string.
    /// value[in]     : Integer to convert.
    /// radix[in]     : Output radix in the range `2..36`.
    /// uppercase[in] : Use uppercase letters for digits above `9`.
    /// alloc[in]     : Allocator to bind to the produced string.
    ///
    /// SUCCESS : Returns `true` on success, `false` on allocation or validation failure.
    ///
    /// TAGS: Int, Convert, String, Radix, Allocator
    ///
    bool int_try_to_str_radix(Str *out, Int *value, u8 radix, bool uppercase, Allocator *alloc);
    Str  int_to_str_radix(Int *value, u8 radix, bool uppercase, Allocator *alloc);

    ///
    /// Parse a decimal string into an integer.
    /// An optional leading `+` is accepted.
    ///
    bool IntTryFromStr(Int *out, const char *decimal);

    ///
    /// Compatibility wrapper for `IntTryFromStr(...)`.
    ///
    /// SUCCESS : Returns Parsed integer value, or zero on failure.
    ///
    Int int_from_str(const char *decimal, Allocator *alloc);
#define IntFromStr(...)             MISRA_OVERLOAD(IntFromStr, __VA_ARGS__)
#define IntFromStr_1(decimal)       int_from_str((decimal), MisraScope)
#define IntFromStr_2(decimal, alloc) int_from_str((decimal), ALLOCATOR_OF(alloc))

    ///
    /// Convert an integer to a decimal string using an explicit allocator.
    ///
    /// out[out]  : Destination string.
    /// value[in] : Integer to convert.
    /// alloc[in] : Allocator to bind to the produced string.
    ///
    /// SUCCESS : Returns `true` on success, `false` on allocation failure.
    ///
    /// TAGS: Int, Convert, String, Decimal, Allocator
    ///
    bool int_try_to_str(Str *out, Int *value, Allocator *alloc);
    Str  int_to_str(Int *value, Allocator *alloc);

    ///
    /// Parse a binary string into an integer.
    /// Accepts an optional `0b` or `0B` prefix.
    ///
    bool IntTryFromBinary(Int *out, const char *binary);

    ///
    /// Compatibility wrapper for `IntTryFromBinary(...)`.
    ///
    /// SUCCESS : Returns Parsed integer value, or zero on failure.
    ///
    Int int_from_binary(const char *binary, Allocator *alloc);
#define IntFromBinary(...)            MISRA_OVERLOAD(IntFromBinary, __VA_ARGS__)
#define IntFromBinary_1(binary)       int_from_binary((binary), MisraScope)
#define IntFromBinary_2(binary, alloc) int_from_binary((binary), ALLOCATOR_OF(alloc))

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
    /// SUCCESS : Returns Parsed integer value, or zero on failure.
    ///
    Int int_from_oct_str(const char *octal, Allocator *alloc);
#define IntFromOctStr(...)           MISRA_OVERLOAD(IntFromOctStr, __VA_ARGS__)
#define IntFromOctStr_1(octal)       int_from_oct_str((octal), MisraScope)
#define IntFromOctStr_2(octal, alloc) int_from_oct_str((octal), ALLOCATOR_OF(alloc))

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
    /// SUCCESS : Returns Parsed integer value, or zero on failure.
    ///
    Int int_from_hex_str(const char *hex, Allocator *alloc);
#define IntFromHexStr(...)         MISRA_OVERLOAD(IntFromHexStr, __VA_ARGS__)
#define IntFromHexStr_1(hex)       int_from_hex_str((hex), MisraScope)
#define IntFromHexStr_2(hex, alloc) int_from_hex_str((hex), ALLOCATOR_OF(alloc))

    ///
    /// Convert an integer to a hexadecimal string.
    ///
    Str IntToHexStr(Int *value);

#ifdef __cplusplus
}
#endif

static inline u64 int_to_u64_no_error(Int *value) {
    return IntToU64WithError(value, NULL);
}

#define INT_TO_U64_SELECT(_1, _2, NAME, ...) NAME

///
/// Convert an integer to `u64`.
///
/// This public macro supports both forms:
///
/// - `IntToU64(value)`               - returns the result, no error channel.
/// - `IntToU64(value, error)`        - writes the error flag through `error`.
///
/// value[in]  : Integer to convert.
/// error[out] : Optional pointer set to `true` on failure and `false` on success.
///
/// SUCCESS : Returns the numeric value as a `u64`. The integer is not
///           modified.
/// FAILURE : Returns `0` when the value does not fit in 64 bits. With the
///           two-argument form `*error` is set to `true`; with the
///           one-argument form the caller cannot distinguish overflow from
///           a true zero result.
///
/// USAGE:
///   u64 v = IntToU64(&big);
///
/// TAGS: Int, Convert, U64, Macro
///
#define IntToU64(...) INT_TO_U64_SELECT(__VA_ARGS__, IntToU64WithError, int_to_u64_no_error)(__VA_ARGS__)

///
/// Convert an integer to a decimal string. Two forms via argument count:
///
/// - `IntTryToStr(out, value)`        - uses `value`'s allocator.
/// - `IntTryToStr(out, value, alloc)` - uses the explicit allocator.
///
#define IntTryToStr(...)                 MISRA_OVERLOAD(IntTryToStr, __VA_ARGS__)
#define IntTryToStr_2(out, value)        int_try_to_str((out), (value), (value)->bits.allocator)
#define IntTryToStr_3(out, value, alloc) int_try_to_str((out), (value), (alloc))

///
/// Convert an integer to a decimal string. Two forms via argument count:
///
/// - `IntToStr(value)`        - uses `value`'s allocator.
/// - `IntToStr(value, alloc)` - uses the explicit allocator.
///
#define IntToStr(...)            MISRA_OVERLOAD(IntToStr, __VA_ARGS__)
#define IntToStr_1(value)        int_to_str((value), (value)->bits.allocator)
#define IntToStr_2(value, alloc) int_to_str((value), (alloc))

///
/// Convert an integer to text in the given radix. Two forms via
/// argument count:
///
/// - `IntTryToStrRadix(out, value, radix, uppercase)`
///       uses `value`'s allocator.
/// - `IntTryToStrRadix(out, value, radix, uppercase, alloc)`
///       uses the explicit allocator.
///
#define IntTryToStrRadix(...) MISRA_OVERLOAD(IntTryToStrRadix, __VA_ARGS__)
#define IntTryToStrRadix_4(out, value, radix, uppercase)                                                               \
    int_try_to_str_radix((out), (value), (radix), (uppercase), (value)->bits.allocator)
#define IntTryToStrRadix_5(out, value, radix, uppercase, alloc)                                                        \
    int_try_to_str_radix((out), (value), (radix), (uppercase), (alloc))

///
/// Convert an integer to text in the given radix. Two forms via
/// argument count:
///
/// - `IntToStrRadix(value, radix, uppercase)`        - `value`'s allocator.
/// - `IntToStrRadix(value, radix, uppercase, alloc)` - explicit allocator.
///
#define IntToStrRadix(...) MISRA_OVERLOAD(IntToStrRadix, __VA_ARGS__)
#define IntToStrRadix_3(value, radix, uppercase)                                                                       \
    int_to_str_radix((value), (radix), (uppercase), (value)->bits.allocator)
#define IntToStrRadix_4(value, radix, uppercase, alloc) int_to_str_radix((value), (radix), (uppercase), (alloc))

#endif // MISRA_STD_CONTAINER_INT_CONVERT_H
