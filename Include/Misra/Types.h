/// file      : misra/types.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Common type definitions, macro definitions and other misc utilities

#ifndef MISRA_TYPES_H
#define MISRA_TYPES_H

#include <stdarg.h>
#include <string.h>  // For strncmp, memcpy, etc.
#include <stdlib.h>  // For malloc

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

#ifdef _MSC_VER
#    if defined(_WIN64)
typedef unsigned long long size;
#    else
typedef unsigned long size;
#    endif
#else
typedef unsigned long size;
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

///
/// Fills a block of memory with a specified value.
///
/// dest[out] : Pointer to the memory block to fill.
/// val[in]   : Value to fill the memory with (converted to unsigned char).
/// n[in]     : Number of bytes to fill.
///
/// SUCCESS: Returns the pointer to the memory block.
/// FAILURE: Function cannot fail if dest is valid.
///
/// TAGS: Memory, Initialization, Safety
void* SetMemory(void* dest, int val, size n);

///
/// Defines text alignment options for formatted output.
///
/// ALIGN_LEFT   : Left-aligned text.
/// ALIGN_RIGHT  : Right-aligned text.
/// ALIGN_CENTER : Center-aligned text.
///
/// TAGS: Formatting, Alignment, Text
typedef enum {
    ALIGN_LEFT,
    ALIGN_RIGHT,
    ALIGN_CENTER
} Alignment;

///
/// Stores formatting information for text output.
///
/// align          : Text alignment (left, right, center).
/// width          : Minimum field width.
/// precision     : Number of decimal places for floating point.
/// has_precision : Whether precision was specified.
/// is_hex        : Format as hexadecimal.
/// is_binary     : Format as binary.
/// is_octal      : Format as octal.
/// is_debug      : Debug format mode.
/// is_scientific : Scientific notation for floats.
/// is_caps       : Use capital letters for hex/scientific.
///
/// TAGS: Formatting, Text, Configuration
typedef struct {
    Alignment align;
    size width;
    size precision;
    bool has_precision;
    bool is_hex;
    bool is_binary;
    bool is_octal;
    bool is_debug;
    bool is_scientific;
    bool is_caps;
} FmtInfo;

///
/// Compare memory regions.
///
/// p1[in]  : First memory region.
/// p2[in]  : Second memory region.
/// n[in]   : Number of bytes to compare.
///
/// SUCCESS: Returns 0 if equal, <0 if p1<p2, >0 if p1>p2.
/// FAILURE: Function cannot fail - always returns comparison result.
///
/// TAGS: Memory, Comparison, Safety
static inline i32 MemCompare(const void* p1, const void* p2, size n) {
    const u8* s1 = (const u8*)p1;
    const u8* s2 = (const u8*)p2;
    while (n--) {
        if (*s1 != *s2) {
            return *s1 - *s2;
        }
        s1++;
        s2++;
    }
    return 0;
}

///
/// Copy memory from source to destination.
///
/// dst[out] : Destination memory region.
/// src[in]  : Source memory region.
/// n[in]    : Number of bytes to copy.
///
/// SUCCESS: Returns destination pointer.
/// FAILURE: Function cannot fail if regions don't overlap.
///
/// TAGS: Memory, Copy, Safety
static inline void* MemCopy(void* dst, const void* src, size n) {
    u8* d = (u8*)dst;
    const u8* s = (const u8*)src;
    while (n--) {
        *d++ = *s++;
    }
    return dst;
}

///
/// Move memory from source to destination, handling overlapping regions.
///
/// dst[out] : Destination memory region.
/// src[in]  : Source memory region.
/// n[in]    : Number of bytes to move.
///
/// SUCCESS: Returns destination pointer.
/// FAILURE: Function cannot fail.
///
/// TAGS: Memory, Move, Safety
static inline void* MemMove(void* dst, const void* src, size n) {
    u8* d = (u8*)dst;
    const u8* s = (const u8*)src;
    if (d < s) {
        while (n--) {
            *d++ = *s++;
        }
    } else if (d > s) {
        d += n;
        s += n;
        while (n--) {
            *--d = *--s;
        }
    }
    return dst;
}

///
/// Set memory region to a value.
///
/// dst[out] : Memory region to set.
/// val[in]  : Value to set (converted to unsigned char).
/// n[in]    : Number of bytes to set.
///
/// SUCCESS: Returns destination pointer.
/// FAILURE: Function cannot fail.
///
/// TAGS: Memory, Set, Safety
static inline void* MemSet(void* dst, i32 val, size n) {
    u8* d = (u8*)dst;
    while (n--) {
        *d++ = (u8)val;
    }
    return dst;
}

///
/// Get length of a null-terminated string.
///
/// str[in] : Null-terminated string.
///
/// SUCCESS: Returns number of characters before null terminator.
/// FAILURE: Function cannot fail if str is valid.
///
/// TAGS: String, Length, Safety
static inline size ZstrLen(const char* str) {
    const char* s = str;
    while (*s) s++;
    return s - str;
}

///
/// Compare two strings lexicographically.
///
/// s1[in] : First string.
/// s2[in] : Second string.
///
/// SUCCESS: Returns 0 if equal, <0 if s1<s2, >0 if s1>s2.
/// FAILURE: Function cannot fail if strings are valid.
///
/// TAGS: String, Comparison, Safety
static inline i32 ZstrCompare(const char* s1, const char* s2) {
    while (*s1 && *s1 == *s2) {
        s1++;
        s2++;
    }
    return *(const u8*)s1 - *(const u8*)s2;
}

///
/// Compare two strings lexicographically up to n characters.
///
/// s1[in] : First string.
/// s2[in] : Second string.
/// n[in]  : Maximum number of characters to compare.
///
/// SUCCESS: Returns 0 if equal, <0 if s1<s2, >0 if s1>s2.
/// FAILURE: Function cannot fail if strings are valid.
///
/// TAGS: String, Comparison, Safety
static inline i32 ZstrCompareN(const char* s1, const char* s2, size n) {
    if (!s1 || !s2)
        return s1 == s2 ? 0 : (s1 ? 1 : -1);

    return strncmp(s1, s2, n);
}

///
/// Duplicates a string up to the specified length.
/// Creates a new null-terminated string by allocating memory and copying
/// at most n characters from the source string.
///
/// src[in] : Source string to duplicate.
/// n[in]   : Maximum number of characters to copy.
///
/// SUCCESS : Returns a pointer to the newly allocated duplicate string.
/// FAILURE : Returns NULL if memory allocation fails or if src is NULL.
///
/// TAGS: String, Memory, Allocation
///
static inline char* ZstrDupN(const char* src, size n) {
    if (!src)
        return NULL;

    size len = 0;
    while (len < n && src[len])
        len++;

    char* new_str = (char*)malloc(len + 1);
    if (!new_str)
        return NULL;

    MemCopy(new_str, src, len);
    new_str[len] = '\0'; // Null-terminate
    return new_str;
}

///
/// Find first occurrence of needle in haystack.
///
/// haystack[in] : String to search in.
/// needle[in]   : String to search for.
///
/// SUCCESS: Returns pointer to first occurrence or NULL if not found.
/// FAILURE: Returns NULL if either string is invalid.
///
/// TAGS: String, Search, Safety
static inline char* ZstrFindSubstring(const char* haystack, const char* needle) {
    if (!*needle) return (char*)haystack;
    
    const char* p2;
    const char* p1_advance = haystack;
    for (p2 = needle; *p2; p2++) {
        p1_advance++;  // increment ahead of time
    }
    p2 = needle;
    while (*p1_advance) {  // test the end of pattern
        p1_advance = haystack;
        while (1) {
            if (!*p2) return (char*)haystack;
            if (*p1_advance++ != *p2++) break;
        }
        p2 = needle;
        haystack++;
    }
    return NULL;
}

///
/// Find first occurrence of a substring of specified length in haystack.
///
/// haystack[in] : String to search in.
/// needle[in]   : Substring to search for.
/// needle_len[in]: Length of the substring to search for.
///
/// SUCCESS: Returns pointer to first occurrence or NULL if not found.
/// FAILURE: Returns NULL if haystack is invalid or needle is NULL.
///
/// TAGS: String, Search, Safety
static inline char* ZstrFindSubstringN(const char* haystack, const char* needle, size needle_len) {
    if (!haystack || !needle) return NULL;
    
    // Empty needle matches at the start of haystack
    if (needle_len == 0) return (char*)haystack;
    
    // First character to match
    char first_char = *needle;
    
    // Calculate haystack length
    size haystack_len = ZstrLen(haystack);
    
    // Search through haystack
    size pos = 0;
    while (pos <= haystack_len - needle_len) {
        // Find the first character
        if (haystack[pos] != first_char) {
            pos++;
            continue;
        }
        
        // Compare the substring
        if (MemCompare(haystack + pos, needle, needle_len) == 0) {
            return (char*)(haystack + pos);
        }
        
        pos++;
    }
    
    return NULL;
}

#endif // MISRA_TYPES_H
