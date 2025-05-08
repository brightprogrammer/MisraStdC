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

typedef float  f32;
typedef double f64;

typedef i8 bool;

typedef unsigned long size;

#define MIN2(x, y) ((x) < (y) ? (x) : (y))
#define MAX2(x, y) ((x) > (y) ? (x) : (y))

// for any general alignment value (13, 8, 17, 144, etc...)
#define ALIGN_UP(value, alignment)                                                                 \
    ((alignment) > 1 ? (((value) + (alignment) - 1) / (alignment) * (alignment)) : (value))

#define ALIGN_DOWN(value, alignment)                                                               \
    ((alignment) > 1 ? ((value) / (alignment) * (alignment)) : (value))

// for alignment value that is power of two, (2, 4, 8, 16, 32, ...)
#define ALIGN_UP_POW2(value, alignment)                                                            \
    ((alignment) > 1 ? (((value) + (alignment) - 1) & ~((alignment) - 1)) : (value))

#define ALIGN_DOWN_POW2(value, alignment)                                                          \
    ((alignment) > 1 ? ((value) & ~((alignment) - 1)) : (value))

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
#define INVERT_ENDIANNESS4(x)                                                                      \
    (INVERT_ENDIANNESS2((x) & 0xffff) << 16) | INVERT_ENDIANNESS2(((x) >> 16) & 0xffff)
#define INVERT_ENDIANNESS8(x)                                                                      \
    (INVERT_ENDIANNESS4((x) & 0xffffffff) << 32) | INVERT_ENDIANNESS4(((x) >> 32) & 0xffffffff)

/// Compatibility macro between MSVC and GCC/Clang
#if defined(_MSC_VER)
#    define FORMAT_STRING(fmt_pos, va_arg_pos)
#else
#    define FORMAT_STRING(fmt_pos, va_arg_pos) __attribute((format(printf, fmt_pos, va_arg_pos)))
#endif

#endif // MISRA_TYPES_H
