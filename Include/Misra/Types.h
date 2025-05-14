/// file      : misra/types.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// copyright : Copyright (c) 2024, Siddharth Mishra, Anvie Labs, All rights reserved.
///
/// Common type definitions, macro definitions and other misc utilities

#ifndef MISRA_TYPES_H
#define MISRA_TYPES_H

typedef signed char      i8;
typedef signed short     i16;
typedef signed int       i32;
typedef signed long long i64;

typedef unsigned char      u8;
typedef unsigned short     u16;
typedef unsigned int       u32;
typedef unsigned long long u64;

typedef float       f32;
typedef double      f64;
typedef long double fl;

typedef i8 bool;

typedef unsigned long size;

#define MIN2(x, y) ((x) < (y) ? (x) : (y))
#define MAX2(x, y) ((x) > (y) ? (x) : (y))

///
/// Clamp the value of `x` in between `lo` and `hi`
///
/// x[in]  : Value to be clamped
/// lo[in] : Range low value
/// hi[in] : Range high value
///
/// RETURN : clamp value
///
#define CLAMP(x, lo, hi) MIN2(MAX2(lo, x), hi)

// for any general alignment value (13, 8, 17, 144, etc...)
#define ALIGN_UP(value, alignment)                                                                                     \
    ((alignment) > 1 ? (((value) + (alignment) - 1) / (alignment) * (alignment)) : (value))

#define ALIGN_DOWN(value, alignment) ((alignment) > 1 ? ((value) / (alignment) * (alignment)) : (value))

// for alignment value that is power of two, (2, 4, 8, 16, 32, ...)
#define ALIGN_UP_POW2(value, alignment) ((alignment) > 1 ? (((value) + (alignment) - 1) & ~((alignment) - 1)) : (value))

#define ALIGN_DOWN_POW2(value, alignment) ((alignment) > 1 ? ((value) & ~((alignment) - 1)) : (value))

///
/// Is `x` in inclusive range of `hi` and `lo`
///
/// x[in]  : Value to check in range for
/// lo[in] : Range low value
/// hi[in] : Range high value
///
/// RETURN : true/false depending on whether value is in range
///
#define IN_RANGE(x, lo, hi) ((x) <= (hi) && (x) >= (lo))

///
/// Is given character a capital ASCII alphabet?
///
/// c[in] : Character to check for
///
/// RETURN : true/false depending on whether `c` is a capital alphabet or not
///
#define IS_CAPS_ALPHA(c) IN_RANGE(c, 'A', 'Z')

///
/// Is given character an ASCII alphabet?
///
/// c[in] : Character to check for
///
/// RETURN : true/false depending on whether `c` is an alphabet or not
///
#define IS_ALPHA(c) (IN_RANGE(c, 'a', 'z') || IN_RANGE(c, 'A', 'Z'))

///
/// Is given character an ASCII digit?
///
/// c[in] : Character to check for
///
/// RETURN : true/false depending on whether `c` is a digit or not
///
#define IS_DIGIT(c) IN_RANGE(c, '0', '9')

///
/// Is given character an octal digit?
///
/// c[in] : Character to check for
///
/// RETURN : true/false depending on whether `c` is an octal digit or not
///
#define IS_OCT(c) IN_RANGE(c, '0', '7')

///
/// Is given character a hex digit?
///
/// c[in] : Character to check for
///
/// RETURN : true/false depending on whether `c` is a hex digit or not
///
#define IS_HEX(c) (IN_RANGE(c, '0', '9') || IN_RANGE(c, 'a', 'f') || IN_RANGE(c, 'A', 'F'))

///
/// Is given character either an alphabet or a digit?
///
/// c[in] : Character to check for
///
/// RETURN : true/false depending on whether `c` is an alphabet or not
///
#define IS_ALPHA_NUMERIC(c) (IS_ALPHA(c) || IS_DIGIT(c))

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
