/// file      : misra/types.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Common type definitions, macro definitions and other misc utilities

#ifndef MISRA_TYPES_H
#define MISRA_TYPES_H

#include <stdarg.h>
#include <string.h> // For strncmp, memcpy, etc.
#include <stdlib.h> // For malloc
#include <limits.h> // For INT_MIN, INT_MAX, etc.

// take an 8 character string and
#define MISRA_MAKE_NEW_MAGIC_VALUE(s)                                                                                  \
    ((((u64)(s[0]) << 56) ^ ((u64)(s[1]) << 48) ^ ((u64)(s[2]) << 40) ^ ((u64)(s[3]) << 32) ^ ((u64)(s[4]) << 24) ^    \
      ((u64)(s[5]) << 16) ^ ((u64)(s[6]) << 8) ^ ((u64)(s[7]))))

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

#if defined(_MSC_VER) || defined(__MSC_VER)
#    if defined(_WIN64)
typedef unsigned long long size;
#    else
typedef unsigned long size;
#    endif
#else
typedef unsigned long size;
#endif

// Integer limits
#ifndef INT8_MIN
#    define INT8_MIN (-128)
#endif
#ifndef INT8_MAX
#    define INT8_MAX 127
#endif
#ifndef UINT8_MAX
#    define UINT8_MAX 255
#endif

#ifndef INT16_MIN
#    define INT16_MIN (-32768)
#endif
#ifndef INT16_MAX
#    define INT16_MAX 32767
#endif
#ifndef UINT16_MAX
#    define UINT16_MAX 65535
#endif

#ifndef INT32_MIN
#    define INT32_MIN (-2147483647 - 1)
#endif
#ifndef INT32_MAX
#    define INT32_MAX 2147483647
#endif
#ifndef UINT32_MAX
#    define UINT32_MAX 4294967295U
#endif

#ifndef INT64_MIN
#    define INT64_MIN (-9223372036854775807LL - 1)
#endif
#ifndef INT64_MAX
#    define INT64_MAX 9223372036854775807LL
#endif
#ifndef UINT64_MAX
#    define UINT64_MAX 18446744073709551615ULL
#endif

// Size type limits
#ifndef SIZE_MAX
#    if defined(_MSC_VER) || defined(__MSC_VER)
#        if defined(_WIN64)
#            define SIZE_MAX UINT64_MAX
#        else
#            define SIZE_MAX UINT32_MAX
#        endif
#    else
#        if ULONG_MAX == UINT64_MAX
#            define SIZE_MAX UINT64_MAX
#        else
#            define SIZE_MAX UINT32_MAX
#        endif
#    endif
#endif

// bool is already defined in C++ and C23
#ifndef __cplusplus
#    if !defined(__STDC_VERSION__) || __STDC_VERSION__ < 201710L
#        ifndef bool
typedef i8 bool;
#        endif

#        ifndef true
#            define true 1
#        endif

#        ifndef false
#            define false 0
#        endif
#    endif

#    ifndef NULL
#        define NULL 0
#    endif
#endif


// Why decltype() yelds a pointer to reference? : https://stackoverflow.com/a/45980559
// What expressions yield a reference type when decltype is applied to them? : https://stackoverflow.com/a/17242295
#if defined(__cplusplus)
#    include <type_traits>
#    define TYPE_OF(x) std::remove_reference<decltype((x))>::type
#else
#    define TYPE_OF(x) __typeof__((x))
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
/// Checks whether a given character 'c' is printable ascii or not.
///
/// NOTE: Only characters in range 0x20 to 0x7e and whitespace characters
///       are considered printable.
///
/// c[in] : Character to check.
///
/// SUCCESS: Returns true or false based on whether given character is printable or not
/// FAILURE: Cannot fail, always return boolean result
///
/// TAGS: Character, ASCII, Printable
#define IS_PRINTABLE(c) (IN_RANGE(c, 0x20, 0x7e) || IS_SPACE(c))

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
/// Checks if the given character `c` is a whitespace character.
///
/// c[in] : The character to check.
///
/// SUCCESS: Returns true for space, tab, newline, carriage return, vertical tab, form feed.
/// FAILURE: Function cannot fail - always returns boolean result.
///
/// TAGS: Character, Validation, Whitespace
#define IS_SPACE(c) ((c) == ' ' || (c) == '\t' || (c) == '\n' || (c) == '\r' || (c) == '\v' || (c) == '\f')

///
/// Checks if the given character `c` is a hexadecimal digit.
///
/// c[in] : The character to check.
///
/// SUCCESS: Returns true for 0-9, a-f, A-F, false otherwise.
/// FAILURE: Function cannot fail - always returns boolean result.
///
/// TAGS: Character, Validation, Hexadecimal
#define IS_XDIGIT(c) (IS_DIGIT(c) || IN_RANGE(c, 'a', 'f') || IN_RANGE(c, 'A', 'F'))

///
/// Converts a character to lowercase.
///
/// c[in] : The character to convert.
///
/// SUCCESS: Returns lowercase version if uppercase, otherwise returns unchanged.
/// FAILURE: Function cannot fail - always returns a character.
///
/// TAGS: Character, Conversion, Case
#define TO_LOWER(c) (IS_CAPS_ALPHA(c) ? ((c) + ('a' - 'A')) : (c))

///
/// Converts a character to uppercase.
///
/// c[in] : The character to convert.
///
/// SUCCESS: Returns uppercase version if lowercase, otherwise returns unchanged.
/// FAILURE: Function cannot fail - always returns a character.
///
/// TAGS: Character, Conversion, Case
#define TO_UPPER(c) (IN_RANGE(c, 'a', 'z') ? ((c) - ('a' - 'A')) : (c))

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
#define INVERT_ENDIANNESS2(x) ((((u16)x) >> 8) & 0xff) | ((((u16)x) & 0xff) << 8)

///
/// Inverts endianness of 32-bit (4-byte) value.
///
/// x[in] : 32-bit value to swap.
///
/// SUCCESS: Returns byte-swapped 32-bit value.
/// FAILURE: Function cannot fail - pure bitwise operation.
///
/// TAGS: Endianness, Bitwise, Conversion
#define INVERT_ENDIANNESS4(x)                                                                                          \
    (INVERT_ENDIANNESS2(((u32)x) & 0xffff) << 16) | INVERT_ENDIANNESS2((((u32)x) >> 16) & 0xffff)

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
    (INVERT_ENDIANNESS4(((u64)x) & 0xffffffff) << 32) | INVERT_ENDIANNESS4((((u64)x) >> 32) & 0xffffffff)

///
/// Compile-time endianness detection.
///
/// SUCCESS: Evaluates to 1 for little-endian systems, 0 for big-endian.
/// FAILURE: Function cannot fail - evaluated at compile time.
///
/// TAGS: Endianness, Platform, Detection
#define IS_LITTLE_ENDIAN() (*(const u8 *)&(const u16) {1})

///
/// Conditionally converts value from big-endian to native byte order.
/// On little-endian systems, this inverts the byte order.
/// On big-endian systems, this is a no-op.
///
/// x[in] : 16-bit value in big-endian format
///
/// SUCCESS: Returns value in native byte order.
/// FAILURE: Function cannot fail - pure bitwise operation.
///
/// TAGS: Endianness, Conversion, 16-bit
#define FROM_BIG_ENDIAN2(x) (IS_LITTLE_ENDIAN() ? INVERT_ENDIANNESS2(x) : (x))

///
/// Conditionally converts value from big-endian to native byte order.
/// On little-endian systems, this inverts the byte order.
/// On big-endian systems, this is a no-op.
///
/// x[in] : 32-bit value in big-endian format
///
/// SUCCESS: Returns value in native byte order.
/// FAILURE: Function cannot fail - pure bitwise operation.
///
/// TAGS: Endianness, Conversion, 32-bit
#define FROM_BIG_ENDIAN4(x) (IS_LITTLE_ENDIAN() ? INVERT_ENDIANNESS4(x) : (x))

///
/// Conditionally converts value from big-endian to native byte order.
/// On little-endian systems, this inverts the byte order.
/// On big-endian systems, this is a no-op.
///
/// x[in] : 64-bit value in big-endian format
///
/// SUCCESS: Returns value in native byte order.
/// FAILURE: Function cannot fail - pure bitwise operation.
///
/// TAGS: Endianness, Conversion, 64-bit
#define FROM_BIG_ENDIAN8(x) (IS_LITTLE_ENDIAN() ? INVERT_ENDIANNESS8(x) : (x))

///
/// Conditionally converts value from little-endian to native byte order.
/// On big-endian systems, this inverts the byte order.
/// On little-endian systems, this is a no-op.
///
/// x[in] : 16-bit value in little-endian format
///
/// SUCCESS: Returns value in native byte order.
/// FAILURE: Function cannot fail - pure bitwise operation.
///
/// TAGS: Endianness, Conversion, 16-bit
#define FROM_LITTLE_ENDIAN2(x) (IS_LITTLE_ENDIAN() ? (x) : INVERT_ENDIANNESS2(x))

///
/// Conditionally converts value from little-endian to native byte order.
/// On big-endian systems, this inverts the byte order.
/// On little-endian systems, this is a no-op.
///
/// x[in] : 32-bit value in little-endian format
///
/// SUCCESS: Returns value in native byte order.
/// FAILURE: Function cannot fail - pure bitwise operation.
///
/// TAGS: Endianness, Conversion, 32-bit
#define FROM_LITTLE_ENDIAN4(x) (IS_LITTLE_ENDIAN() ? (x) : INVERT_ENDIANNESS4(x))

///
/// Conditionally converts value from little-endian to native byte order.
/// On big-endian systems, this inverts the byte order.
/// On little-endian systems, this is a no-op.
///
/// x[in] : 64-bit value in little-endian format
///
/// SUCCESS: Returns value in native byte order.
/// FAILURE: Function cannot fail - pure bitwise operation.
///
/// TAGS: Endianness, Conversion, 64-bit
#define FROM_LITTLE_ENDIAN8(x) (IS_LITTLE_ENDIAN() ? (x) : INVERT_ENDIANNESS8(x))

///
/// Conditionally converts value from native byte order to little-endian.
/// On big-endian systems, this inverts the byte order.
/// On little-endian systems, this is a no-op.
///
/// x[in] : 16-bit value in native byte order
///
/// SUCCESS: Returns value in little-endian format.
/// FAILURE: Function cannot fail - pure bitwise operation.
///
/// TAGS: Endianness, Conversion, 16-bit
#define TO_LITTLE_ENDIAN2(x) (IS_LITTLE_ENDIAN() ? (x) : INVERT_ENDIANNESS2(x))

///
/// Conditionally converts value from native byte order to little-endian.
/// On big-endian systems, this inverts the byte order.
/// On little-endian systems, this is a no-op.
///
/// x[in] : 32-bit value in native byte order
///
/// SUCCESS: Returns value in little-endian format.
/// FAILURE: Function cannot fail - pure bitwise operation.
///
/// TAGS: Endianness, Conversion, 32-bit
#define TO_LITTLE_ENDIAN4(x) (IS_LITTLE_ENDIAN() ? (x) : INVERT_ENDIANNESS4(x))

///
/// Conditionally converts value from native byte order to little-endian.
/// On big-endian systems, this inverts the byte order.
/// On little-endian systems, this is a no-op.
///
/// x[in] : 64-bit value in native byte order
///
/// SUCCESS: Returns value in little-endian format.
/// FAILURE: Function cannot fail - pure bitwise operation.
///
/// TAGS: Endianness, Conversion, 64-bit
#define TO_LITTLE_ENDIAN8(x) (IS_LITTLE_ENDIAN() ? (x) : INVERT_ENDIANNESS8(x))

///
/// Conditionally converts value from native byte order to big-endian.
/// On little-endian systems, this inverts the byte order.
/// On big-endian systems, this is a no-op.
///
/// x[in] : 16-bit value in native byte order
///
/// SUCCESS: Returns value in big-endian format.
/// FAILURE: Function cannot fail - pure bitwise operation.
///
/// TAGS: Endianness, Conversion, 16-bit
#define TO_BIG_ENDIAN2(x) (IS_LITTLE_ENDIAN() ? INVERT_ENDIANNESS2(x) : (x))

///
/// Conditionally converts value from native byte order to big-endian.
/// On little-endian systems, this inverts the byte order.
/// On big-endian systems, this is a no-op.
///
/// x[in] : 32-bit value in native byte order
///
/// SUCCESS: Returns value in big-endian format.
/// FAILURE: Function cannot fail - pure bitwise operation.
///
/// TAGS: Endianness, Conversion, 32-bit
#define TO_BIG_ENDIAN4(x) (IS_LITTLE_ENDIAN() ? INVERT_ENDIANNESS4(x) : (x))

///
/// Conditionally converts value from native byte order to big-endian.
/// On little-endian systems, this inverts the byte order.
/// On big-endian systems, this is a no-op.
///
/// x[in] : 64-bit value in native byte order
///
/// SUCCESS: Returns value in big-endian format.
/// FAILURE: Function cannot fail - pure bitwise operation.
///
/// TAGS: Endianness, Conversion, 64-bit
#define TO_BIG_ENDIAN8(x) (IS_LITTLE_ENDIAN() ? INVERT_ENDIANNESS8(x) : (x))

///
/// Maintains native endianness for 16-bit values.
/// This is a no-op that returns the value unchanged.
///
/// x[in] : 16-bit value in native byte order
///
/// SUCCESS: Returns value in native byte order (unchanged).
/// FAILURE: Function cannot fail - pure passthrough operation.
///
/// TAGS: Endianness, Native, 16-bit
#define TO_NATIVE_ENDIAN2(x) (x)

///
/// Maintains native endianness for 32-bit values.
/// This is a no-op that returns the value unchanged.
///
/// x[in] : 32-bit value in native byte order
///
/// SUCCESS: Returns value in native byte order (unchanged).
/// FAILURE: Function cannot fail - pure passthrough operation.
///
/// TAGS: Endianness, Native, 32-bit
#define TO_NATIVE_ENDIAN4(x) (x)

///
/// Maintains native endianness for 64-bit values.
/// This is a no-op that returns the value unchanged.
///
/// x[in] : 64-bit value in native byte order
///
/// SUCCESS: Returns value in native byte order (unchanged).
/// FAILURE: Function cannot fail - pure passthrough operation.
///
/// TAGS: Endianness, Native, 64-bit
#define TO_NATIVE_ENDIAN8(x) (x)

#if defined(_MSC_VER) || defined(__MSC_VER)
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

///
/// Part of the parenthesis trick to delay text generation
///
#define TRICK_PARENS ()

#define TRICK_EXPAND(...)  TRICK_EXPAND4(TRICK_EXPAND4(TRICK_EXPAND4(TRICK_EXPAND4(__VA_ARGS__))))
#define TRICK_EXPAND4(...) TRICK_EXPAND3(TRICK_EXPAND3(TRICK_EXPAND3(TRICK_EXPAND3(__VA_ARGS__))))
#define TRICK_EXPAND3(...) TRICK_EXPAND2(TRICK_EXPAND2(TRICK_EXPAND2(TRICK_EXPAND2(__VA_ARGS__))))
#define TRICK_EXPAND2(...) TRICK_EXPAND1(TRICK_EXPAND1(TRICK_EXPAND1(TRICK_EXPAND1(__VA_ARGS__))))
#define TRICK_EXPAND1(...) __VA_ARGS__

///
/// Expand variadic argument list for given macro for each element one by one.
///
/// This is a macro trick to apply a given macro on a variadic argument list
/// one by one. The only limitiation here is that we cannot expand more than
/// 256 times for the moment. To make it do more than that, we just need to add
/// one more line to the EXPAND macros
///
/// Also, if you have more than 20-30 arguments in your format strings, you should
/// seriously consider whether you're doing something wrong
///
/// Reference : https://www.scs.stanford.edu/~dm/blog/va-opt.html
///
#define APPLY_MACRO_FOREACH(macro, ...) __VA_OPT__(TRICK_EXPAND(APPLY_MACRO_FOREACH_HELPER(macro, __VA_ARGS__)))

///
/// Helper macro to apply given macro to the very first argument in list of variadic macro arguments
/// Then expands to applying the same macro to more arguments only if there are more
///
#define APPLY_MACRO_FOREACH_HELPER(macro, a1, ...)                                                                     \
    macro(a1) __VA_OPT__(APPLY_MACRO_FOREACH_AGAIN TRICK_PARENS(macro, __VA_ARGS__))

///
/// Helper macro to delay evaluation in text generation (pre-processing) phase of macro
///
#define APPLY_MACRO_FOREACH_AGAIN() APPLY_MACRO_FOREACH_HELPER


///
/// Macro helper to generate unique names
///
#define CONCAT_(a, b) a##b
#define CONCAT(a, b) CONCAT_(a, b)
#define UNIQUE_NAME(base) CONCAT(base, __COUNTER__)

///  Unique name per line
#define UNPL(base) CONCAT(base, __LINE__)


#endif // MISRA_TYPES_H
