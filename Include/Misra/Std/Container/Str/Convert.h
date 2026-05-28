/// file      : std/container/str/convert.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// String conversion functions for Str

#ifndef MISRA_STD_CONTAINER_STR_CONVERT_H
#define MISRA_STD_CONTAINER_STR_CONVERT_H

#include "Type.h"

#ifdef __cplusplus
extern "C" {
#endif

    /// Configuration for integer formatting
    typedef struct {
        u8   base;       ///< Base (2-36)
        bool uppercase;  ///< Use uppercase letters for bases > 10
        bool use_prefix; ///< Add prefix (0x, 0b, 0o)
        bool pad_zeros;  ///< Pad with leading zeros
        u8   min_width;  ///< Minimum field width
    } StrIntFormat;

    /// Configuration for float formatting
    typedef struct {
        u8   precision;   ///< Decimal places (0-17)
        bool force_sci;   ///< Force scientific notation
        bool uppercase;   ///< Use uppercase E in scientific notation
        bool trim_zeros;  ///< Remove trailing zeros
        bool always_sign; ///< Always show + for positive numbers
    } StrFloatFormat;

    /// Configuration for parsing
    typedef struct {
        bool strict;     ///< Strict parsing (no trailing chars)
        bool trim_space; ///< Trim leading/trailing whitespace
        u8   base;       ///< Base for integers (0 = auto-detect)
    } StrParseConfig;

    /// Default configurations
    extern const StrIntFormat   STR_INT_DEFAULT;
    extern const StrFloatFormat STR_FLOAT_DEFAULT;
    extern const StrParseConfig STR_PARSE_DEFAULT;

    ///
    /// Convert an unsigned 64-bit integer to string
    ///
    /// str[out]    : String to store the result in
    /// value[in]   : Value to convert
    /// config[in]  : Formatting configuration. May be `NULL` (uses the
    ///               default: base 10, no prefix).
    ///
    /// SUCCESS : Returns `str`
    /// FAILURE : Returns `NULL` on allocator OOM while appending to `str`,
    ///           or when a non-NULL `config` has an invalid base. The
    ///           destination string is untouched.
    ///
    /// TAGS: Str, Convert, U64
    ///
    Str *StrFromU64(Str *str, u64 value, const StrIntFormat *config);

    ///
    /// Convert a signed 64-bit integer to string
    ///
    /// str[out]    : String to store the result in
    /// value[in]   : Value to convert
    /// config[in]  : Formatting configuration. May be `NULL` (uses the
    ///               default: base 10, no prefix).
    ///
    /// SUCCESS : Returns `str`
    /// FAILURE : Returns `NULL` on allocator OOM while appending to `str`,
    ///           or when a non-NULL `config` has an invalid base. The
    ///           destination string is untouched.
    ///
    /// TAGS: Str, Convert, I64
    ///
    Str *StrFromI64(Str *str, i64 value, const StrIntFormat *config);

    ///
    /// Convert a double to string
    ///
    /// str[out]     : String to store the result in
    /// value[in]    : Value to convert
    /// config[in]   : Formatting configuration. May be `NULL` (uses the
    ///                default: 6 decimal places).
    ///
    /// SUCCESS : Returns `str`
    /// FAILURE : Returns `NULL` on allocator OOM while appending to `str`,
    ///           or when a non-NULL `config` has an invalid precision
    ///           (> 17). The destination string is untouched.
    ///
    /// TAGS: Str, Convert, F64
    ///
    Str *StrFromF64(Str *str, f64 value, const StrFloatFormat *config);

    ///
    /// Convert string to unsigned 64-bit integer
    ///
    /// str[in]     : String to convert
    /// value[out]  : Where to store the result
    /// config[in]  : Parse configuration (NULL for auto-detect base)
    ///
    /// SUCCESS : Returns `true` and writes the parsed value into `*value`
    /// FAILURE : Returns `false` when the input cannot be parsed; `*value` is left unchanged
    ///
    /// TAGS: Str, Convert, U64
    ///
    bool StrToU64(const Str *str, u64 *value, const StrParseConfig *config);

    ///
    /// Convert string to signed 64-bit integer
    ///
    /// str[in]     : String to convert
    /// value[out]  : Where to store the result
    /// config[in]  : Parse configuration (NULL for auto-detect base)
    ///
    /// SUCCESS : Returns `true` and writes the parsed value into `*value`
    /// FAILURE : Returns `false` when the input cannot be parsed; `*value` is left unchanged
    ///
    /// TAGS: Str, Convert, I64
    ///
    bool StrToI64(const Str *str, i64 *value, const StrParseConfig *config);

    ///
    /// Convert string to double
    ///
    /// str[in]     : String to convert
    /// value[out]  : Where to store the result
    /// config[in]  : Parse configuration (NULL for default parsing)
    ///
    /// SUCCESS : Returns `true` and writes the parsed value into `*value`
    /// FAILURE : Returns `false` when the input cannot be parsed; `*value` is left unchanged
    ///
    /// TAGS: Str, Convert, F64
    ///
    bool StrToF64(const Str *str, f64 *value, const StrParseConfig *config);



#ifdef __cplusplus
}
#endif

#endif // MISRA_STD_CONTAINER_STR_CONVERT_H
