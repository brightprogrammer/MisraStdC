/// file      : std/zstr.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Operations on NUL-terminated C strings (`Zstr`). For growable
/// owned strings see `Misra/Std/Container/Str.h`. For binary buffers
/// see `Misra/Std/Container/Buf.h`.

#ifndef MISRA_STD_ZSTR_H
#define MISRA_STD_ZSTR_H

#include <Misra/Std/Allocator.h>
#include <Misra/Std/Container/Vec/Type.h>
#include <Misra/Types.h>

/// Read-only NUL-terminated C string -- the project name for what
/// libc calls `const char *`. Public API surface uses this typedef so
/// the intent ("read a borrowed C-string here") is visible at a
/// glance; internal helpers may keep raw `const char *`.
typedef const char *Zstr;
typedef Vec(Zstr) Zstrs;

///
/// Get length of a null-terminated string.
///
/// str[in] : Null-terminated string.
///
/// SUCCESS: Returns number of characters before null terminator.
/// FAILURE: Function cannot fail if str is valid.
///
/// TAGS: Zstr, Length
size ZstrLen(const char *str);

///
/// Compare two strings lexicographically (case-sensitive).
///
/// s1[in] : First string.
/// s2[in] : Second string.
///
/// SUCCESS: Returns 0 if equal, <0 if s1<s2, >0 if s1>s2.
/// FAILURE: Function cannot fail if strings are valid.
///
/// TAGS: Zstr, Comparison
i32 ZstrCompare(const char *s1, const char *s2);

///
/// Compare two strings lexicographically up to n characters
/// (case-sensitive).
///
/// SUCCESS: Returns 0 if equal, <0 if s1<s2, >0 if s1>s2.
/// FAILURE: Function cannot fail if strings are valid.
///
/// TAGS: Zstr, Comparison
i32 ZstrCompareN(const char *s1, const char *s2, size n);

///
/// Compare two strings lexicographically, ignoring ASCII case.
/// Non-ASCII bytes are compared as-is (no Unicode case folding).
///
/// SUCCESS: Returns 0 if equal under ASCII-case folding, <0 if
///          lowered s1 < lowered s2, >0 otherwise.
/// FAILURE: Function cannot fail if strings are valid.
///
/// TAGS: Zstr, Comparison, IgnoreCase
i32 ZstrCompareIgnoreCase(const char *s1, const char *s2);

///
/// Compare two strings up to n characters, ignoring ASCII case.
///
/// SUCCESS: Returns 0 if equal under ASCII-case folding, <0 / >0
///          otherwise.
/// FAILURE: Function cannot fail if strings are valid.
///
/// TAGS: Zstr, Comparison, IgnoreCase
i32 ZstrCompareNIgnoreCase(const char *s1, const char *s2, size n);

///
/// Find the first occurrence of a character in a null-terminated string.
///
/// SUCCESS: Returns pointer to first occurrence or NULL if not found.
/// FAILURE: Function cannot fail if `str` is valid.
///
/// TAGS: Zstr, Search
char *ZstrFindChar(const char *str, char ch);

///
/// Find first occurrence of `needle` in `haystack`.
///
/// SUCCESS: Returns pointer to first occurrence or NULL if not found.
/// FAILURE: Returns NULL if either string is invalid.
///
/// TAGS: Zstr, Search
char *ZstrFindSubstring(const char *haystack, const char *needle);

///
/// Find first occurrence of a substring of specified length.
///
/// SUCCESS: Returns pointer to first occurrence or NULL if not found.
/// FAILURE: Returns NULL if haystack is invalid or needle is NULL.
///
/// TAGS: Zstr, Search
char *ZstrFindSubstringN(const char *haystack, const char *needle, size needle_len);

///
/// Duplicates a string up to the specified length. Allocates a new
/// buffer through `alloc` and copies at most `n` characters.
///
/// `alloc` is optional inside a `Scope` block (defaults to `MisraScope`).
///
/// SUCCESS : Returns a pointer to the newly allocated duplicate string.
/// FAILURE : Returns NULL if memory allocation fails.
///
/// TAGS: Zstr, Allocation
char *zstr_dup_n(const char *src, size n, Allocator *alloc);
#define ZstrDupN(...)             MISRA_OVERLOAD(ZstrDupN, __VA_ARGS__)
#define ZstrDupN_2(src, n)        zstr_dup_n((src), (n), MisraScope)
#define ZstrDupN_3(src, n, alloc) zstr_dup_n((src), (n), ALLOCATOR_OF(alloc))

///
/// Duplicates a string in full.
///
/// `alloc` is optional inside a `Scope` block (defaults to `MisraScope`).
///
/// SUCCESS : Returns a pointer to the newly allocated duplicate string.
/// FAILURE : Returns NULL if memory allocation fails or `src` is NULL.
///
/// TAGS: Zstr, Allocation
char *zstr_dup(const char *src, Allocator *alloc);
#define ZstrDup(...)          MISRA_OVERLOAD(ZstrDup, __VA_ARGS__)
#define ZstrDup_1(src)        zstr_dup((src), MisraScope)
#define ZstrDup_2(src, alloc) zstr_dup((src), ALLOCATOR_OF(alloc))

///
/// Clone callback for `Zstrs` (= `Vec(const char *)`).
///
/// NOTE: Meant to be installed as the init handler on a `Zstrs` vector
///       so that VecInitCopy / element-level operations duplicate the
///       underlying buffers.
///
bool zstr_init_clone(void *dst, const void *src, const Allocator *alloc);

///
/// Deinit callback for `Zstrs` (= `Vec(const char *)`). Releases the
/// buffer that `zstr_init_clone` allocated, then NULLs the slot.
///
void zstr_deinit(void *zs, const Allocator *alloc);

///
/// Parse a signed decimal integer from a null-terminated string.
/// Skips ASCII whitespace, accepts an optional leading sign, then
/// consumes the longest run of `0..9`. Drops the libc `strtoll`
/// dependency for callers that only need base-10.
///
/// SUCCESS: Returns the parsed value as i64. Overflow saturates to
///          `INT64_MAX` / `INT64_MIN`.
/// FAILURE: Returns 0 when no digits are present.
///
/// TAGS: Zstr, Parse, Integer
i64 ZstrToI64(const char *s, char **endptr);

///
/// Parse a decimal floating-point value. Accepts
/// `[+-]?digits(.digits)?([eE][+-]?digits)?`. Replaces libc `strtod`
/// for JSON / KvConfig numeric values. Not bit-exact on long mantissas.
///
/// SUCCESS: Returns the parsed value as f64.
/// FAILURE: Returns 0.0 when no digits are present.
///
/// TAGS: Zstr, Parse, Float
f64 ZstrToF64(const char *s, char **endptr);

#endif // MISRA_STD_ZSTR_H
