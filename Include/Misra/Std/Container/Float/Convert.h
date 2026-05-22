/// file      : std/container/float/convert.h
/// author    : Generated following Misra project patterns
/// This is free and unencumbered software released into the public domain.
///
/// Conversion helpers for Float.

#ifndef MISRA_STD_CONTAINER_FLOAT_CONVERT_H
#define MISRA_STD_CONTAINER_FLOAT_CONVERT_H

#include "Private.h"
#include <Misra/Std/Container/Str.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef __cplusplus
#    define FLOAT_FROM_DISPATCH(value)                                                                                 \
        _Generic(                                                                                                      \
            (value),                                                                                                   \
            Int *: float_from_int,                                                                                     \
            unsigned char: float_from_u64,                                                                             \
            unsigned short: float_from_u64,                                                                            \
            unsigned int: float_from_u64,                                                                              \
            unsigned long: float_from_u64,                                                                             \
            unsigned long long: float_from_u64,                                                                        \
            signed char: float_from_i64,                                                                               \
            signed short: float_from_i64,                                                                              \
            signed int: float_from_i64,                                                                                \
            signed long: float_from_i64,                                                                               \
            signed long long: float_from_i64,                                                                          \
            float: float_from_f32,                                                                                     \
            double: float_from_f64                                                                                     \
        )

///
/// Convert a numeric value into an arbitrary-precision float.
/// Dispatches on the type of `value`. The allocator argument may be a
/// typed allocator pointer (`&heap`) or a raw `Allocator *`.
///
/// value[in]     : Integer, `Int`, `float`, or `double` source value
/// allocator_ptr : Allocator that owns the returned Float's storage.
///
/// SUCCESS : Returns Float representing the same numeric value.
///
/// USAGE:
///   DefaultAllocator a = DefaultAllocatorInit();
///   Float value = FloatFrom(42, &a);
///
/// TAGS: Float, Convert, Import, Generic
///
#    define FloatFrom(value, allocator_ptr) FLOAT_FROM_DISPATCH(value)((value), ALLOCATOR_OF(allocator_ptr))
#endif

    ///
    /// Convert a float to an arbitrary-precision integer (truncation toward
    /// zero - the fractional portion is discarded).
    ///
    /// result[out] : Destination integer.
    /// value[in]   : Float to convert.
    ///
    /// SUCCESS : Returns `true`. `*result` holds the integer part of
    ///           `value` (sign preserved). The source float is unchanged.
    /// FAILURE : Returns `false` on allocation failure during the
    ///           magnitude transfer. `*result` is left in a valid but
    ///           unspecified state.
    ///
    /// TAGS: Float, Convert, Int, Truncate
    ///
    bool FloatToInt(Int *result, Float *value);

    ///
    /// Parse a decimal string into a float.
    /// Supports an optional sign, decimal point, and scientific exponent
    /// (`[+-]?digits(.digits)?([eE][+-]?digits)?`).
    ///
    /// out[in,out] : Float to overwrite with the parsed value. The
    ///               existing storage is reused; `*out` must already be
    ///               a valid initialised Float bound to an allocator.
    /// text[in]    : Null-terminated decimal string.
    ///
    /// SUCCESS : Returns `true`. `*out` is deinitialised and replaced
    ///           with the normalised parsed value (sign, significand,
    ///           and exponent set).
    /// FAILURE : Returns `false` on a malformed string, an exponent
    ///           overflow, or an allocation failure. `*out` retains its
    ///           pre-call value.
    ///
    /// TAGS: Float, Convert, Parse, Decimal
    ///
    bool float_try_from_str_zstr(Float *out, Zstr text);
    bool float_try_from_str_str(Float *out, const Str *text);
#define FloatTryFromStr(out, text)                                                                                                                                \
    _Generic((text), Str *: float_try_from_str_str, const Str *: float_try_from_str_str, char *: float_try_from_str_zstr, const char *: float_try_from_str_zstr)( \
        (out),                                                                                                                                                    \
        (text)                                                                                                                                                    \
    )

    ///
    /// Compatibility wrapper for `FloatTryFromStr(...)`.
    ///
    /// SUCCESS : Returns Parsed floating-point value, or zero on failure.
    ///
    Float float_from_str_zstr(Zstr text, Allocator *alloc);
    Float float_from_str_str(const Str *text, Allocator *alloc);
#define FloatFromStr(...) MISRA_OVERLOAD(FloatFromStr, __VA_ARGS__)
#define FloatFromStr_1(text)                                                                                                                      \
    _Generic((text), Str *: float_from_str_str, const Str *: float_from_str_str, char *: float_from_str_zstr, const char *: float_from_str_zstr)( \
        (text),                                                                                                                                   \
        MisraScope                                                                                                                                \
    )
#define FloatFromStr_2(text, alloc)                                                                                                               \
    _Generic((text), Str *: float_from_str_str, const Str *: float_from_str_str, char *: float_from_str_zstr, const char *: float_from_str_zstr)( \
        (text),                                                                                                                                   \
        ALLOCATOR_OF(alloc)                                                                                                                       \
    )

    /// Snake_case runtime helpers. User code calls the PascalCase macros
    /// below, which dispatch to these via MISRA_OVERLOAD.
    bool float_try_to_str(Str *out, Float *value, Allocator *alloc);
    Str  float_to_str(Float *value, Allocator *alloc);

#ifdef __cplusplus
}
#endif

///
/// Convert a float to a decimal string. Two forms via argument count:
///
/// - `FloatTryToStr(out, value)`        - uses `value`'s allocator.
/// - `FloatTryToStr(out, value, alloc)` - uses the explicit allocator.
///
/// SUCCESS : Returns `true`. `*out` holds the decimal representation.
/// FAILURE : Returns `false` on allocation failure. `*out` is left in a
///           valid but unspecified state.
///
/// TAGS: Float, Convert, String, Decimal
///
#define FloatTryToStr(...)                 MISRA_OVERLOAD(FloatTryToStr, __VA_ARGS__)
#define FloatTryToStr_2(out, value)        float_try_to_str((out), (value), (value)->significand.bits.allocator)
#define FloatTryToStr_3(out, value, alloc) float_try_to_str((out), (value), (alloc))

///
/// Convert a float to a decimal string. Two forms via argument count:
///
/// - `FloatToStr(value)`        - uses `value`'s allocator.
/// - `FloatToStr(value, alloc)` - uses the explicit allocator.
///
/// SUCCESS : Returns the freshly built `Str`.
/// FAILURE : Returns an empty `Str` on allocation failure. Use the
///           `FloatTryToStr` form when you need explicit failure
///           propagation.
///
/// TAGS: Float, Convert, String, Decimal
///
#define FloatToStr(...)            MISRA_OVERLOAD(FloatToStr, __VA_ARGS__)
#define FloatToStr_1(value)        float_to_str((value), (value)->significand.bits.allocator)
#define FloatToStr_2(value, alloc) float_to_str((value), (alloc))

#endif // MISRA_STD_CONTAINER_FLOAT_CONVERT_H
