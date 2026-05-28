/// file      : std/container/int/convert.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
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
///
/// Convert a native integer into an arbitrary-precision integer.
/// Dispatches on the type of `value`.
///
/// value[in] : Signed or unsigned integer source value
///
/// SUCCESS : Returns Integer holding the same non-negative value.
/// FAILURE : `LOG_FATAL` when a signed `value` is negative (the
///           non-negative `IntFrom` contract is violated). Allocator
///           failures on the underlying construction abort through
///           the allocator's standard failure path.
///
/// WARN: Aborts when a signed input is negative.
///
/// USAGE:
///   Int value = IntFrom(1234u);
///
/// TAGS: Int, Convert, Import, Generic
///
#    define IntFrom(value, alloc)                                                                                      \
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
        )((value), ALLOCATOR_OF(alloc))
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
    /// TAGS: Int, Convert, U64
    ///
    bool IntTryToU64(const Int *value, u64 *out);

    ///
    /// Convert an integer to `u64`.
    ///
    /// value[in]  : Integer to convert
    /// error[out] : Optional pointer set to `true` on failure and `false` on success
    ///
    /// SUCCESS : Returns The numeric value as `u64`, or `0` on failure.
    /// FAILURE : Returns `0` when the value exceeds 64 bits. `*error` (if
    ///           non-NULL) is set to `true`.
    ///
    /// TAGS: Int, Convert, U64
    ///
    u64 IntToU64WithError(const Int *value, bool *error);

    ///
    /// Create an integer from little-endian bytes.
    ///
    /// bytes[in] : Source byte buffer holding the magnitude in
    ///             little-endian order (least significant byte first).
    ///             May be `NULL` when `len == 0`.
    /// len[in]   : Number of bytes to read from `bytes`.
    /// alloc[in] : Allocator to bind to the returned integer.
    ///
    /// SUCCESS : Returns a normalised `Int` holding the unsigned
    ///           magnitude packed in `bytes`. `bytes` is not modified.
    /// FAILURE : Returns a zero-initialised `Int` bound to `alloc` when
    ///           the underlying bit-vector allocation fails. The caller
    ///           cannot distinguish that from a true zero result.
    ///           `LOG_FATAL` when `bytes` is `NULL` and `len != 0`.
    ///
    /// TAGS: Int, Convert, Bytes, LE
    ///
    Int int_from_bytes_le(const u8 *bytes, u64 len, Allocator *alloc);
#define IntFromBytesLE(...)                 OVERLOAD(IntFromBytesLE, __VA_ARGS__)
#define IntFromBytesLE_2(bytes, len)        int_from_bytes_le((bytes), (len), MisraScope)
#define IntFromBytesLE_3(bytes, len, alloc) int_from_bytes_le((bytes), (len), ALLOCATOR_OF(alloc))

    ///
    /// Export an integer into little-endian bytes.
    ///
    /// value[in]   : Integer to export.
    /// bytes[out]  : Destination byte buffer. Must be non-NULL and at
    ///               least `max_len` bytes long.
    /// max_len[in] : Maximum bytes to write. Must be non-zero.
    ///
    /// SUCCESS : Returns the number of bytes written, which is
    ///           `min(IntByteLength(value), max_len)`. The first byte
    ///           written holds the least significant 8 bits.
    ///           `value` is not modified.
    /// FAILURE : Returns `0` when `value` has zero bytes of
    ///           magnitude. `LOG_FATAL` when `bytes` is `NULL` or
    ///           `max_len` is `0`.
    ///
    /// TAGS: Int, Convert, Bytes, LE
    ///
    u64 IntToBytesLE(const Int *value, u8 *bytes, u64 max_len);

    ///
    /// Create an integer from big-endian bytes.
    ///
    /// bytes[in] : Source byte buffer holding the magnitude in
    ///             big-endian order (most significant byte first).
    ///             May be `NULL` when `len == 0`.
    /// len[in]   : Number of bytes to read from `bytes`.
    /// alloc[in] : Allocator to bind to the returned integer.
    ///
    /// SUCCESS : Returns a normalised `Int` holding the unsigned
    ///           magnitude packed in `bytes`. `bytes` is not modified.
    /// FAILURE : Returns a zero-initialised `Int` bound to `alloc` when
    ///           an intermediate shift or add allocation fails. The
    ///           caller cannot distinguish that from a true zero
    ///           result. `LOG_FATAL` when `bytes` is `NULL` and
    ///           `len != 0`.
    ///
    /// TAGS: Int, Convert, Bytes, BE
    ///
    Int int_from_bytes_be(const u8 *bytes, u64 len, Allocator *alloc);
#define IntFromBytesBE(...)                 OVERLOAD(IntFromBytesBE, __VA_ARGS__)
#define IntFromBytesBE_2(bytes, len)        int_from_bytes_be((bytes), (len), MisraScope)
#define IntFromBytesBE_3(bytes, len, alloc) int_from_bytes_be((bytes), (len), ALLOCATOR_OF(alloc))

    ///
    /// Export an integer into big-endian bytes.
    ///
    /// value[in]   : Integer to export.
    /// bytes[out]  : Destination byte buffer. Must be non-NULL and at
    ///               least `max_len` bytes long.
    /// max_len[in] : Maximum bytes to write. Must be non-zero.
    ///
    /// SUCCESS : Returns the number of bytes written, which is
    ///           `min(IntByteLength(value), max_len)`. The first byte
    ///           written holds the most significant 8 bits.
    ///           `value` is not modified.
    /// FAILURE : Returns `0` when `value` has zero bytes of
    ///           magnitude. `LOG_FATAL` when `bytes` is `NULL` or
    ///           `max_len` is `0`.
    ///
    /// TAGS: Int, Convert, Bytes, BE
    ///
    u64 IntToBytesBE(const Int *value, u8 *bytes, u64 max_len);

    ///
    /// Parse digits in the given radix into an integer.
    /// Supports radices from 2 through 36 and ignores underscore separators.
    /// An optional leading `+` sign is consumed by callers; this entry
    /// point itself does not accept a sign.
    ///
    /// out[in,out] : Int to overwrite with the parsed value. Must already
    ///               be a valid initialised Int; its allocator is reused
    ///               for the parsed result.
    /// digits[in]  : Null-terminated digit string (no base prefix).
    /// radix[in]   : Output radix in the range `2..36`.
    ///
    /// SUCCESS : Returns `true`. `*out` is deinitialised and replaced with
    ///           the normalised parsed value.
    /// FAILURE : Returns `false` on an invalid radix, an invalid digit
    ///           for the radix, an empty digit run, or an allocation
    ///           failure during accumulation. `*out` retains its
    ///           pre-call value.
    ///
    /// TAGS: Int, Convert, Parse, Radix
    ///
    bool int_try_from_str_radix_zstr(Int *out, Zstr digits, u8 radix);
    bool int_try_from_str_radix_str(Int *out, const Str *digits, u8 radix);
#define IntTryFromStrRadix(out, digits, radix)                                                                                                                                      \
    _Generic((digits), Str *: int_try_from_str_radix_str, char *: int_try_from_str_radix_zstr, Zstr : int_try_from_str_radix_zstr)( \
        (out),                                                                                                                                                                      \
        (digits),                                                                                                                                                                   \
        (radix)                                                                                                                                                                     \
    )

    ///
    /// Compatibility wrapper for `IntTryFromStrRadix(...)`.
    ///
    /// SUCCESS : Returns Parsed integer value, or zero on failure.
    /// FAILURE : Returns a zero-initialised `Int` on an invalid radix, an
    ///           invalid digit, an empty digit run, or an allocation
    ///           failure. Use `IntTryFromStrRadix(...)` when explicit
    ///           failure propagation is required.
    ///
    /// TAGS: Int, Convert, Radix, String
    ///
    Int int_from_str_radix_zstr(Zstr digits, u8 radix, Allocator *alloc);
    Int int_from_str_radix_str(const Str *digits, u8 radix, Allocator *alloc);
#define IntFromStrRadix(...) OVERLOAD(IntFromStrRadix, __VA_ARGS__)
#define IntFromStrRadix_2(digits, radix)                                                                                                                            \
    _Generic((digits), Str *: int_from_str_radix_str, char *: int_from_str_radix_zstr, Zstr : int_from_str_radix_zstr)( \
        (digits),                                                                                                                                                   \
        (radix),                                                                                                                                                    \
        MisraScope                                                                                                                                                  \
    )
#define IntFromStrRadix_3(digits, radix, alloc)                                                                                                                     \
    _Generic((digits), Str *: int_from_str_radix_str, char *: int_from_str_radix_zstr, Zstr : int_from_str_radix_zstr)( \
        (digits),                                                                                                                                                   \
        (radix),                                                                                                                                                    \
        ALLOCATOR_OF(alloc)                                                                                                                                         \
    )

    ///
    /// Convert an integer to text in the given radix using an explicit allocator.
    ///
    /// out[out]      : Destination string.
    /// value[in]     : Integer to convert.
    /// radix[in]     : Output radix in the range `2..36`.
    /// uppercase[in] : Use uppercase letters for digits above `9`.
    /// alloc[in]     : Allocator to bind to the produced string.
    ///
    /// SUCCESS : Returns `true`; `*out` holds the textual form of
    ///           `value` in base `radix` (letters uppercased when
    ///           `uppercase` is `true`).
    /// FAILURE : Returns `false` when `radix` is outside `2..36` or
    ///           when an intermediate allocation fails. `*out` is left
    ///           initialised as an empty `Str` bound to `alloc`.
    ///
    /// TAGS: Int, Convert, String, Radix, Allocator
    ///
    bool int_try_to_str_radix(Str *out, const Int *value, u8 radix, bool uppercase, Allocator *alloc);
    Str  int_to_str_radix(const Int *value, u8 radix, bool uppercase, Allocator *alloc);

    ///
    /// Parse a decimal string into an integer.
    /// An optional leading `+` is accepted; underscore separators between
    /// digits are silently skipped.
    ///
    /// out[in,out] : Int to overwrite with the parsed value. Must already
    ///               be a valid initialised Int; its allocator is reused
    ///               for the parsed result.
    /// decimal[in] : Null-terminated decimal string, optionally prefixed
    ///               with `+`.
    ///
    /// SUCCESS : Returns `true`. `*out` is deinitialised and replaced with
    ///           the normalised parsed value.
    /// FAILURE : Returns `false` on a non-decimal character, an empty
    ///           digit run, or an allocation failure during accumulation.
    ///           `*out` retains its pre-call value.
    ///
    /// TAGS: Int, Convert, Parse, Decimal
    ///
    bool int_try_from_str_zstr(Int *out, Zstr decimal);
    bool int_try_from_str_str(Int *out, const Str *decimal);
#define IntTryFromStr(out, decimal)                                                                                                                          \
    _Generic((decimal), Str *: int_try_from_str_str, char *: int_try_from_str_zstr, Zstr : int_try_from_str_zstr)( \
        (out),                                                                                                                                               \
        (decimal)                                                                                                                                            \
    )

    ///
    /// Compatibility wrapper for `IntTryFromStr(...)`.
    ///
    /// SUCCESS : Returns Parsed integer value, or zero on failure.
    /// FAILURE : Returns a zero-initialised `Int` on a non-decimal
    ///           character, an empty digit run, or an allocation failure.
    ///           Use `IntTryFromStr(...)` when explicit failure
    ///           propagation is required.
    ///
    /// TAGS: Int, Convert, String
    ///
    Int int_from_str_zstr(Zstr decimal, Allocator *alloc);
    Int int_from_str_str(const Str *decimal, Allocator *alloc);
#define IntFromStr(...) OVERLOAD(IntFromStr, __VA_ARGS__)
#define IntFromStr_1(decimal)                                                                                                                \
    _Generic((decimal), Str *: int_from_str_str, char *: int_from_str_zstr, Zstr : int_from_str_zstr)( \
        (decimal),                                                                                                                           \
        MisraScope                                                                                                                           \
    )
#define IntFromStr_2(decimal, alloc)                                                                                                         \
    _Generic((decimal), Str *: int_from_str_str, char *: int_from_str_zstr, Zstr : int_from_str_zstr)( \
        (decimal),                                                                                                                           \
        ALLOCATOR_OF(alloc)                                                                                                                  \
    )

    ///
    /// Convert an integer to a decimal string using an explicit allocator.
    ///
    /// out[out]  : Destination string.
    /// value[in] : Integer to convert.
    /// alloc[in] : Allocator to bind to the produced string.
    ///
    /// SUCCESS : Returns `true`; `*out` holds the base-10 textual form
    ///           of `value` (leading `-` when negative, `"0"` for
    ///           zero).
    /// FAILURE : Returns `false` on allocation failure during the
    ///           radix conversion. `*out` is left initialised as an
    ///           empty `Str` bound to `alloc`.
    ///
    /// TAGS: Int, Convert, String, Decimal, Allocator
    ///
    bool int_try_to_str(Str *out, const Int *value, Allocator *alloc);
    Str  int_to_str(const Int *value, Allocator *alloc);

    ///
    /// Parse a binary string into an integer.
    /// Accepts an optional `0b` or `0B` prefix; underscore separators
    /// between digits are silently skipped.
    ///
    /// out[in,out] : Int to overwrite with the parsed value. Must already
    ///               be a valid initialised Int; its allocator is reused
    ///               for the parsed result.
    /// binary[in]  : Null-terminated binary string, optionally prefixed
    ///               with `0b` / `0B`.
    ///
    /// SUCCESS : Returns `true`. `*out` is deinitialised and replaced with
    ///           the normalised parsed value.
    /// FAILURE : Returns `false` on a non-binary character, an empty digit
    ///           run, or an allocation failure during accumulation.
    ///           `*out` retains its pre-call value.
    ///
    /// TAGS: Int, Convert, Parse, Binary
    ///
    bool int_try_from_binary_zstr(Int *out, Zstr binary);
    bool int_try_from_binary_str(Int *out, const Str *binary);
#define IntTryFromBinary(out, binary)                                                                                                                                   \
    _Generic((binary), Str *: int_try_from_binary_str, char *: int_try_from_binary_zstr, Zstr : int_try_from_binary_zstr)( \
        (out),                                                                                                                                                          \
        (binary)                                                                                                                                                        \
    )

    ///
    /// Compatibility wrapper for `IntTryFromBinary(...)`.
    ///
    /// SUCCESS : Returns Parsed integer value, or zero on failure.
    /// FAILURE : Returns a zero-initialised `Int` on a non-binary
    ///           character, an empty digit run, or an allocation failure.
    ///           Use `IntTryFromBinary(...)` when explicit failure
    ///           propagation is required.
    ///
    /// TAGS: Int, Convert, Binary
    ///
    Int int_from_binary_zstr(Zstr binary, Allocator *alloc);
    Int int_from_binary_str(const Str *binary, Allocator *alloc);
#define IntFromBinary(...) OVERLOAD(IntFromBinary, __VA_ARGS__)
#define IntFromBinary_1(binary)                                                                                                                         \
    _Generic((binary), Str *: int_from_binary_str, char *: int_from_binary_zstr, Zstr : int_from_binary_zstr)( \
        (binary),                                                                                                                                       \
        MisraScope                                                                                                                                      \
    )
#define IntFromBinary_2(binary, alloc)                                                                                                                  \
    _Generic((binary), Str *: int_from_binary_str, char *: int_from_binary_zstr, Zstr : int_from_binary_zstr)( \
        (binary),                                                                                                                                       \
        ALLOCATOR_OF(alloc)                                                                                                                             \
    )

    ///
    /// Convert an integer to a binary string.
    ///
    /// value[in] : Integer to convert.
    ///
    /// SUCCESS : Returns a `Str` holding the base-2 textual form of
    ///           `value`, bound to `value`'s allocator. `value` is not
    ///           modified.
    /// FAILURE : Returns an empty `Str` bound to `value`'s allocator
    ///           when the underlying `IntToStrRadix` fails
    ///           (intermediate allocation failure). The caller cannot
    ///           distinguish that from a true empty result; use
    ///           `IntTryToStrRadix` directly when failure detection is
    ///           required.
    ///
    /// TAGS: Int, Convert, Binary
    ///
    Str IntToBinary(const Int *value);

    ///
    /// Parse an octal string into an integer.
    /// Accepts an optional `0o` or `0O` prefix; underscore separators
    /// between digits are silently skipped.
    ///
    /// out[in,out] : Int to overwrite with the parsed value. Must already
    ///               be a valid initialised Int; its allocator is reused
    ///               for the parsed result.
    /// octal[in]   : Null-terminated octal string, optionally prefixed
    ///               with `0o` / `0O`.
    ///
    /// SUCCESS : Returns `true`. `*out` is deinitialised and replaced with
    ///           the normalised parsed value.
    /// FAILURE : Returns `false` on a non-octal character, an empty digit
    ///           run, or an allocation failure during accumulation.
    ///           `*out` retains its pre-call value.
    ///
    /// TAGS: Int, Convert, Parse, Octal
    ///
    bool int_try_from_oct_str_zstr(Int *out, Zstr octal);
    bool int_try_from_oct_str_str(Int *out, const Str *octal);
#define IntTryFromOctStr(out, octal)                                                                                                                                       \
    _Generic((octal), Str *: int_try_from_oct_str_str, char *: int_try_from_oct_str_zstr, Zstr : int_try_from_oct_str_zstr)( \
        (out),                                                                                                                                                             \
        (octal)                                                                                                                                                            \
    )

    ///
    /// Compatibility wrapper for `IntTryFromOctStr(...)`.
    ///
    /// SUCCESS : Returns Parsed integer value, or zero on failure.
    /// FAILURE : Returns a zero-initialised `Int` on a non-octal
    ///           character, an empty digit run, or an allocation failure.
    ///           Use `IntTryFromOctStr(...)` when explicit failure
    ///           propagation is required.
    ///
    /// TAGS: Int, Convert, Oct
    ///
    Int int_from_oct_str_zstr(Zstr octal, Allocator *alloc);
    Int int_from_oct_str_str(const Str *octal, Allocator *alloc);
#define IntFromOctStr(...) OVERLOAD(IntFromOctStr, __VA_ARGS__)
#define IntFromOctStr_1(octal)                                                                                                                             \
    _Generic((octal), Str *: int_from_oct_str_str, char *: int_from_oct_str_zstr, Zstr : int_from_oct_str_zstr)( \
        (octal),                                                                                                                                           \
        MisraScope                                                                                                                                         \
    )
#define IntFromOctStr_2(octal, alloc)                                                                                                                      \
    _Generic((octal), Str *: int_from_oct_str_str, char *: int_from_oct_str_zstr, Zstr : int_from_oct_str_zstr)( \
        (octal),                                                                                                                                           \
        ALLOCATOR_OF(alloc)                                                                                                                                \
    )

    ///
    /// Convert an integer to an octal string.
    ///
    /// value[in] : Integer to convert.
    ///
    /// SUCCESS : Returns a `Str` holding the base-8 textual form of
    ///           `value`, bound to `value`'s allocator. `value` is not
    ///           modified.
    /// FAILURE : Returns an empty `Str` bound to `value`'s allocator
    ///           when the underlying `IntToStrRadix` fails
    ///           (intermediate allocation failure). The caller cannot
    ///           distinguish that from a true empty result; use
    ///           `IntTryToStrRadix` directly when failure detection is
    ///           required.
    ///
    /// TAGS: Int, Convert, Oct
    ///
    Str IntToOctStr(const Int *value);

    ///
    /// Parse a hexadecimal string into an integer.
    /// This parser expects hexadecimal digits only and does not accept a
    /// `0x` prefix. Underscore separators are NOT skipped (radix 16 path
    /// does not allow underscores).
    ///
    /// out[in,out] : Int to overwrite with the parsed value. Must already
    ///               be a valid initialised Int; its allocator is reused
    ///               for the parsed result.
    /// hex[in]     : Null-terminated string of hexadecimal digits
    ///               (`0-9`, `a-f`, `A-F`), with no base prefix.
    ///
    /// SUCCESS : Returns `true`. `*out` is deinitialised and replaced with
    ///           the normalised parsed value.
    /// FAILURE : Returns `false` on a non-hex character (including
    ///           underscore or a `0x` prefix), an empty digit run, or an
    ///           allocation failure during accumulation. `*out` retains
    ///           its pre-call value.
    ///
    /// TAGS: Int, Convert, Parse, Hex
    ///
    bool int_try_from_hex_str_zstr(Int *out, Zstr hex);
    bool int_try_from_hex_str_str(Int *out, const Str *hex);
#define IntTryFromHexStr(out, hex)                                                                                                                                       \
    _Generic((hex), Str *: int_try_from_hex_str_str, char *: int_try_from_hex_str_zstr, Zstr : int_try_from_hex_str_zstr)( \
        (out),                                                                                                                                                           \
        (hex)                                                                                                                                                            \
    )

    ///
    /// Compatibility wrapper for `IntTryFromHexStr(...)`.
    ///
    /// SUCCESS : Returns Parsed integer value, or zero on failure.
    /// FAILURE : Returns a zero-initialised `Int` on a non-hex character
    ///           (including underscore or a `0x` prefix), an empty digit
    ///           run, or an allocation failure. Use `IntTryFromHexStr(...)`
    ///           when explicit failure propagation is required.
    ///
    /// TAGS: Int, Convert, Hex
    ///
    Int int_from_hex_str_zstr(Zstr hex, Allocator *alloc);
    Int int_from_hex_str_str(const Str *hex, Allocator *alloc);
#define IntFromHexStr(...) OVERLOAD(IntFromHexStr, __VA_ARGS__)
#define IntFromHexStr_1(hex)                                                                                                                             \
    _Generic((hex), Str *: int_from_hex_str_str, char *: int_from_hex_str_zstr, Zstr : int_from_hex_str_zstr)( \
        (hex),                                                                                                                                           \
        MisraScope                                                                                                                                       \
    )
#define IntFromHexStr_2(hex, alloc)                                                                                                                      \
    _Generic((hex), Str *: int_from_hex_str_str, char *: int_from_hex_str_zstr, Zstr : int_from_hex_str_zstr)( \
        (hex),                                                                                                                                           \
        ALLOCATOR_OF(alloc)                                                                                                                              \
    )

    ///
    /// Convert an integer to a hexadecimal string.
    ///
    /// value[in] : Integer to convert.
    ///
    /// SUCCESS : Returns a `Str` holding the base-16 textual form of
    ///           `value` with lowercase letters, bound to `value`'s
    ///           allocator. `value` is not modified.
    /// FAILURE : Returns an empty `Str` bound to `value`'s allocator
    ///           when the underlying `IntToStrRadix` fails
    ///           (intermediate allocation failure). The caller cannot
    ///           distinguish that from a true empty result; use
    ///           `IntTryToStrRadix` directly when failure detection is
    ///           required.
    ///
    /// TAGS: Int, Convert, Hex
    ///
    Str IntToHexStr(const Int *value);

#ifdef __cplusplus
}
#endif

static inline u64 int_to_u64_no_error(const Int *value) {
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
/// SUCCESS : Returns `true`; `*out` holds the base-10 textual form of
///           `value` (leading `-` when negative, `"0"` for zero).
/// FAILURE : Returns `false` when an intermediate allocation fails;
///           `*out` is left initialized as an empty `Str` on `alloc`.
///
/// TAGS: Int, Convert, String
///
#define IntTryToStr(...)                 OVERLOAD(IntTryToStr, __VA_ARGS__)
#define IntTryToStr_2(out, value)        int_try_to_str((out), (value), (value)->bits.allocator)
#define IntTryToStr_3(out, value, alloc) int_try_to_str((out), (value), (alloc))

///
/// Convert an integer to a decimal string. Two forms via argument count:
///
/// - `IntToStr(value)`        - uses `value`'s allocator.
/// - `IntToStr(value, alloc)` - uses the explicit allocator.
///
/// SUCCESS : Returns a `Str` holding the base-10 textual form of
///           `value`.
/// FAILURE : Returns an empty `Str` bound to `alloc` when the
///           underlying `IntTryToStr` fails (intermediate allocation
///           failure); the caller cannot distinguish that from a true
///           empty result, so callers that need to detect failure
///           should use `IntTryToStr` directly.
///
/// TAGS: Int, Convert, String
///
#define IntToStr(...)            OVERLOAD(IntToStr, __VA_ARGS__)
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
/// SUCCESS : Returns `true`; `*out` holds the textual form of `value`
///           in base `radix`. Letters are uppercase when `uppercase`
///           is `true`, lowercase otherwise.
/// FAILURE : Returns `false` when `radix` is outside the supported
///           range or when an intermediate allocation fails; `*out` is
///           left initialized as an empty `Str` on `alloc`.
///
/// TAGS: Int, Convert, Radix, String
///
#define IntTryToStrRadix(...) OVERLOAD(IntTryToStrRadix, __VA_ARGS__)
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
/// SUCCESS : Returns a `Str` holding the textual form of `value` in
///           base `radix`.
/// FAILURE : Returns an empty `Str` bound to `alloc` when the
///           underlying `IntTryToStrRadix` fails (unsupported radix
///           or intermediate allocation failure); the caller cannot
///           distinguish that from a true empty result, so callers
///           that need to detect failure should use `IntTryToStrRadix`
///           directly.
///
/// TAGS: Int, Convert, Radix, String
///
#define IntToStrRadix(...) OVERLOAD(IntToStrRadix, __VA_ARGS__)
#define IntToStrRadix_3(value, radix, uppercase)                                                                       \
    int_to_str_radix((value), (radix), (uppercase), (value)->bits.allocator)
#define IntToStrRadix_4(value, radix, uppercase, alloc) int_to_str_radix((value), (radix), (uppercase), (alloc))

#endif // MISRA_STD_CONTAINER_INT_CONVERT_H
