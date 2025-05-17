/// file      : misra/types.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// copyright : Copyright (c) 2024, Siddharth Mishra, Anvie Labs, All rights reserved.
///
/// Common type definitions, macro definitions and other misc utilities

#ifndef MISRA_TYPES_H
#define MISRA_TYPES_H

// signed types
typedef signed char      i8;
typedef signed short     i16;
typedef signed int       i32;
typedef signed long long i64;

// unsigned types
typedef unsigned char      u8;
typedef unsigned short     u16;
typedef unsigned int       u32;
typedef unsigned long long u64;

typedef float  f32;
typedef double f64;

typedef i8 bool;

typedef unsigned long size;

///
/// Returns the smaller of two values `x` and `y`.
///
/// x[in] : First value for comparison.
/// y[in] : Second value for comparison.
///
/// RETURN : The smaller of `x` and `y`.
///
#define MIN2(x, y) ((x) < (y) ? (x) : (y))

///
/// Returns the larger of two values `x` and `y`.
///
/// x[in] : First value for comparison.
/// y[in] : Second value for comparison.
///
/// RETURN : The larger of `x` and `y`.
///
#define MAX2(x, y) ((x) > (y) ? (x) : (y))

///
/// Creates a temporary, addressable l-value from a given expression `x`.
/// This allows taking the address of literals or other r-values as if they were
/// variables.
///
/// x[in] : The expression whose value needs to be addressable.
///
/// RETURN : The value of `x` as the first element of a temporary array. This allows
///          using the address-of operator (`&`) on the result (e.g., `&LVAL(123)`).
///
#define LVAL(x) ((__typeof__(x)[]) {(x)})[0]

///
/// Clamps the value of `x` to be within the inclusive range [`lo`, `hi`].
/// If `x` is less than `lo`, it returns `lo`. If `x` is greater than `hi`, it returns `hi`.
/// Otherwise, it returns `x`.
///
/// x[in]  : The value to be clamped.
/// lo[in] : The lower bound of the clamping range.
/// hi[in] : The upper bound of the clamping range.
///
/// RETURN : The clamped value of `x`.
///
#define CLAMP(x, lo, hi) MIN2(MAX2(lo, x), hi)

///
/// Aligns the given `value` up to the nearest multiple of `alignment`.
/// If `alignment` is 1 or less, the original `value` is returned.
/// This works for any general alignment value.
///
/// value[in]     : The value to be aligned up.
/// alignment[in] : The alignment boundary (must be greater than 0 for alignment to occur).
///
/// RETURN : The smallest multiple of `alignment` that is greater than or equal to `value`.
///          Returns `value` if `alignment` is not greater than 1.
///
#define ALIGN_UP(value, alignment)                                                                                     \
    ((alignment) > 1 ? (((value) + (alignment) - 1) / (alignment) * (alignment)) : (value))

///
/// Aligns the given `value` down to the nearest multiple of `alignment`.
/// If `alignment` is 1 or less, the original `value` is returned.
/// This works for any general alignment value.
///
/// value[in]     : The value to be aligned down.
/// alignment[in] : The alignment boundary (must be greater than 0 for alignment to occur).
///
/// RETURN : The largest multiple of `alignment` that is less than or equal to `value`.
///          Returns `value` if `alignment` is not greater than 1.
///
#define ALIGN_DOWN(value, alignment) ((alignment) > 1 ? ((value) / (alignment) * (alignment)) : (value))

///
/// Aligns the given `value` up to the nearest power-of-two multiple of `alignment`.
/// It is assumed that `alignment` is a power of two. If `alignment` is 1 or less,
/// the original `value` is returned.
///
/// value[in]     : The value to be aligned up.
/// alignment[in] : The power-of-two alignment boundary (must be greater than 0 for
///                 alignment to occur and should be a power of two).
///
/// RETURN : The smallest power-of-two multiple of `alignment` that is greater than
///          or equal to `value`. Returns `value` if `alignment` is not greater than 1.
///
#define ALIGN_UP_POW2(value, alignment) ((alignment) > 1 ? (((value) + (alignment) - 1) & ~((alignment) - 1)) : (value))

///
/// Aligns the given `value` down to the nearest power-of-two multiple of `alignment`.
/// It is assumed that `alignment` is a power of two. If `alignment` is 1 or less,
/// the original `value` is returned.
///
/// value[in]     : The value to be aligned down.
/// alignment[in] : The power-of-two alignment boundary (must be greater than 0 for
///                 alignment to occur and should be a power of two).
///
/// RETURN : The largest power-of-two multiple of `alignment` that is less than or
///          equal to `value`. Returns `value` if `alignment` is not greater than 1.
///
#define ALIGN_DOWN_POW2(value, alignment) ((alignment) > 1 ? ((value) & ~((alignment) - 1)) : (value))

///
/// Checks if the value `x` is within the inclusive range [`lo`, `hi`].
///
/// x[in]  : The value to check.
/// lo[in] : The lower bound of the range.
/// hi[in] : The upper bound of the range.
///
/// RETURN : `true` (non-zero) if `x` is greater than or equal to `lo` and less than
///          or equal to `hi`. `false` (zero) otherwise.
///
#define IN_RANGE(x, lo, hi) ((x) >= (lo) && (x) <= (hi))

///
/// Checks if the given character `c` is an uppercase ASCII alphabet (A-Z).
///
/// c[in] : The character to check.
///
/// RETURN : `true` (non-zero) if `c` is an uppercase ASCII alphabet. `false` (zero) otherwise.
///
#define IS_CAPS_ALPHA(c) IN_RANGE(c, 'A', 'Z')

///
/// Checks if the given character `c` is an ASCII alphabet (a-z or A-Z).
///
/// c[in] : The character to check.
///
/// RETURN : `true` (non-zero) if `c` is a lowercase or uppercase ASCII alphabet.
///          `false` (zero) otherwise.
///
#define IS_ALPHA(c) (IN_RANGE(c, 'a', 'z') || IN_RANGE(c, 'A', 'Z'))

///
/// Checks if the given character `c` is an ASCII digit (0-9).
///
/// c[in] : The character to check.
///
/// RETURN : `true` (non-zero) if `c` is an ASCII digit. `false` (zero) otherwise.
///
#define IS_DIGIT(c) IN_RANGE(c, '0', '9')

#ifndef true
#    define true 1
#endif

#ifndef false
#    define false 0
#endif

#ifndef NULL
#    define NULL 0
#endif

#define NEW(tname) calloc(1, sizeof(tname))
#define FREE(x)    (free((void *)(x)), (x) = NULL)

#define INVERT_ENDIANNESS2(x) (((x) >> 8) & 0xff) | (((x) & 0xff) << 8)
#define INVERT_ENDIANNESS4(x) (INVERT_ENDIANNESS2((x) & 0xffff) << 16) | INVERT_ENDIANNESS2(((x) >> 16) & 0xffff)
#define INVERT_ENDIANNESS8(x)                                                                                          \
    (INVERT_ENDIANNESS4((x) & 0xffffffff) << 32) | INVERT_ENDIANNESS4(((x) >> 32) & 0xffffffff)

/// Compatibility macro between MSVC and GCC/Clang
#if defined(_MSC_VER)
#    define FORMAT_STRING(fmt_pos, va_arg_pos)
#else
#    define FORMAT_STRING(fmt_pos, va_arg_pos) __attribute((format(printf, fmt_pos, va_arg_pos)))
#endif

#endif // MISRA_TYPES_H
