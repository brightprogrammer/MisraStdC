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

typedef unsigned long size;

// bool is already defined in C++
#ifndef __cplusplus
#    ifndef bool
typedef i8 bool;
#    endif

#    ifndef true
#        define true 1
#    endif

#    ifndef false
#        define false 0
#    endif

#    ifndef NULL
#        define NULL 0
#    endif
#endif

// Why decltype() yelds a pointer to reference? : https://stackoverflow.com/a/45980559
// What expressions yield a reference type when decltype is applied to them? : https://stackoverflow.com/a/17242295
#if defined(__cplusplus)
#    include <type_traits>
#    define TYPE_OF(x) std::remove_reference<decltype ((x))>::type
#else
#    define TYPE_OF(x) __typeof__ ((x))
#endif

//
/// Returns the smaller of two values `x` and `y`.
///
/// x[in] : First value for comparison.
/// y[in] : Second value for comparison.
///
/// SUCCESS: Returns the smaller of `x` and `y`.
/// FAILURE: Function cannot fail - always returns a value.
///
/// TAGS: Comparison, Min, Math, Utility
#define MIN2(x, y) ((x) < (y) ? (x) : (y))

///
/// Returns the larger of two values `x` and `y`.
///
/// x[in] : First value for comparison.
/// y[in] : Second value for comparison.
///
/// SUCCESS: Returns the larger of `x` and `y`.
/// FAILURE: Function cannot fail - always returns a value.
///
/// TAGS: Comparison, Max, Math, Utility
#define MAX2(x, y) ((x) > (y) ? (x) : (y))

///
/// Creates a temporary, addressable l-value from a given expression `x`.
///
/// x[in] : The expression whose value needs to be addressable.
///
/// SUCCESS: Returns addressable version of `x` as first element of temporary array.
/// FAILURE: Function cannot fail - creates temporary storage unconditionally.
///
/// TAGS: Memory, Utility, TypeConversion
#define LVAL(x) ((TYPE_OF(x)[]) {(x)})[0]

///
/// Clamps the value of `x` to be within the inclusive range [`lo`, `hi`].
///
/// x[in]  : The value to be clamped.
/// lo[in] : The lower bound of the clamping range.
/// hi[in] : The upper bound of the clamping range.
///
/// SUCCESS: Returns `lo` if `x < lo`, `hi` if `x > hi`, otherwise `x`.
/// FAILURE: Function cannot fail - always returns a clamped value.
///
/// TAGS: Math, Utility, Range
#define CLAMP(x, lo, hi) MIN2(MAX2(lo, x), hi)

///
/// Aligns the given `value` up to the nearest multiple of `alignment`.
///
/// value[in]     : The value to be aligned up.
/// alignment[in] : The alignment boundary.
///
/// SUCCESS: Returns aligned-up value when alignment > 1, original value otherwise.
/// FAILURE: Function cannot fail - always returns a valid value.
///
/// TAGS: Memory, Alignment, Math
#define ALIGN_UP(value, alignment)                                                                                     \
    ((alignment) > 1 ? (((value) + (alignment) - 1) / (alignment) * (alignment)) : (value))

///
/// Aligns the given `value` down to the nearest multiple of `alignment`.
///
/// value[in]     : The value to be aligned down.
/// alignment[in] : The alignment boundary.
///
/// SUCCESS: Returns aligned-down value when alignment > 1, original value otherwise.
/// FAILURE: Function cannot fail - always returns a valid value.
///
/// TAGS: Memory, Alignment, Math
#define ALIGN_DOWN(value, alignment) ((alignment) > 1 ? ((value) / (alignment) * (alignment)) : (value))

///
/// Aligns the given `value` up to the nearest power-of-two multiple.
///
/// value[in]     : The value to be aligned up.
/// alignment[in] : The power-of-two alignment boundary.
///
/// SUCCESS: Returns power-of-two aligned-up value when alignment > 1.
/// FAILURE: Function cannot fail - returns original value if alignment <= 1.
///
/// TAGS: Memory, Alignment, PowerOfTwo
#define ALIGN_UP_POW2(value, alignment) ((alignment) > 1 ? (((value) + (alignment) - 1) & ~((alignment) - 1)) : (value))

///
/// Aligns the given `value` down to the nearest power-of-two multiple.
///
/// value[in]     : The value to be aligned down.
/// alignment[in] : The power-of-two alignment boundary.
///
/// SUCCESS: Returns power-of-two aligned-down value when alignment > 1.
/// FAILURE: Function cannot fail - returns original value if alignment <= 1.
///
/// TAGS: Memory, Alignment, PowerOfTwo
#define ALIGN_DOWN_POW2(value, alignment) ((alignment) > 1 ? ((value) & ~((alignment) - 1)) : (value))

///
/// Checks if the value `x` is within the inclusive range [`lo`, `hi`].
///
/// x[in]  : The value to check.
/// lo[in] : The lower bound of the range.
/// hi[in] : The upper bound of the range.
///
/// SUCCESS: Returns true if `x` is in range, false otherwise.
/// FAILURE: Function cannot fail - always returns boolean result.
///
/// TAGS: Validation, Range, Utility
#define IN_RANGE(x, lo, hi) ((x) >= (lo) && (x) <= (hi))

///
/// Checks if the given character `c` is an uppercase ASCII alphabet.
///
/// c[in] : The character to check.
///
/// SUCCESS: Returns true for A-Z, false otherwise.
/// FAILURE: Function cannot fail - always returns boolean result.
///
/// TAGS: Character, Validation, ASCII
#define IS_CAPS_ALPHA(c) IN_RANGE(c, 'A', 'Z')

///
/// Checks if the given character `c` is an ASCII alphabet.
///
/// c[in] : The character to check.
///
/// SUCCESS: Returns true for a-z/A-Z, false otherwise.
/// FAILURE: Function cannot fail - always returns boolean result.
///
/// TAGS: Character, Validation, ASCII
#define IS_ALPHA(c) (IN_RANGE(c, 'a', 'z') || IN_RANGE(c, 'A', 'Z'))

///
/// Checks if the given character `c` is an ASCII digit.
///
/// c[in] : The character to check.
///
/// SUCCESS: Returns true for 0-9, false otherwise.
/// FAILURE: Function cannot fail - always returns boolean result.
///
/// TAGS: Character, Validation, Numeric
#define IS_DIGIT(c) IN_RANGE(c, '0', '9')

///
/// Allocates zero-initialized memory for a type.
///
/// tname[in] : Type name to allocate memory for.
///
/// SUCCESS: Returns pointer to zero-initialized memory block.
/// FAILURE: Returns NULL if memory allocation fails.
///
/// TAGS: Memory, Allocation, Initialization
#define NEW(tname) calloc(1, sizeof(tname))

///
/// Safely deallocates memory and nullifies pointer.
///
/// x[in,out] : Pointer variable to free.
///
/// SUCCESS: Memory is deallocated and pointer set to NULL.
/// FAILURE: Function cannot fail - safe to call with NULL.
///
/// TAGS: Memory, Deallocation, Safety
#define FREE(x) ((x) ? free((void *)(x)) : (void)1, (x) = NULL)

///
/// Inverts endianness of 16-bit (2-byte) value.
///
/// x[in] : 16-bit value to swap.
///
/// SUCCESS: Returns byte-swapped 16-bit value.
/// FAILURE: Function cannot fail - pure bitwise operation.
///
/// TAGS: Endianness, Bitwise, Conversion
#define INVERT_ENDIANNESS2(x) (((x) >> 8) & 0xff) | (((x) & 0xff) << 8)

///
/// Inverts endianness of 32-bit (4-byte) value.
///
/// x[in] : 32-bit value to swap.
///
/// SUCCESS: Returns byte-swapped 32-bit value.
/// FAILURE: Function cannot fail - pure bitwise operation.
///
/// TAGS: Endianness, Bitwise, Conversion
#define INVERT_ENDIANNESS4(x) (INVERT_ENDIANNESS2((x) & 0xffff) << 16) | INVERT_ENDIANNESS2(((x) >> 16) & 0xffff)

///
/// Inverts endianness of 64-bit (8-byte) value.
///
/// x[in] : 64-bit value to swap.
///
/// SUCCESS: Returns byte-swapped 64-bit value.
/// FAILURE: Function cannot fail - pure bitwise operation.
///
/// TAGS: Endianness, Bitwise, Conversion
#define INVERT_ENDIANNESS8(x)                                                                                          \
    (INVERT_ENDIANNESS4((x) & 0xffffffff) << 32) | INVERT_ENDIANNESS4(((x) >> 32) & 0xffffffff)


#if defined(_MSC_VER)
#    define FORMAT_STRING(fmt_pos, va_arg_pos)
#else
///
/// Enables printf-style format string validation for non-MSVC compilers.
///
/// fmt_pos[in]   : Position of format string parameter (1-based)
/// va_arg_pos[in]: Position of variadic arguments (1-based)
///
/// SUCCESS: Enables compiler format string validation (GCC/Clang).
/// FAILURE: No-op when using MSVC compiler.
///
/// TAGS: Compiler, Compatibility, Validation
#    define FORMAT_STRING(fmt_pos, va_arg_pos) __attribute((format(printf, fmt_pos, va_arg_pos)))
#endif


#endif // MISRA_TYPES_H
