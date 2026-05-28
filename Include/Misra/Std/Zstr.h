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
/// libc spells `const char *`. Zstr is the canonical C-string type
/// in the codebase for declarations, parameters, return types, and
/// fields. `-Wwrite-strings` (gcc/clang) types string literals as
/// `const char *` (= `Zstr`) and rejects `char *p = "literal"`.
///
/// `_Generic` dispatch has one carve-out: every arm matching `Zstr`
/// also has a `char *` synonym arm with the same body. MSVC's C
/// `_Generic` follows the C standard, which types string literals
/// as `char[N]` decaying to `char *`; `/Zc:strictStrings` is a
/// C++-only flag with no effect in C mode. Inlining both arms at
/// every dispatch site is what keeps the codebase portable to
/// MSVC. See CODING-CONVENTIONS.md for the canonical shape.
///
/// `Cstr` is not a type but a naming-suffix for the `(Zstr, size)`
/// form -- a non-NUL-terminated view, or a NUL-terminated string
/// truncated at an explicit length. See `StrStartsWith` for the
/// canonical Cstr / Zstr / unsuffixed-Str overload family.
typedef const char *Zstr;
typedef Vec(Zstr) Zstrs;

///
/// Get length of a null-terminated string.
///
/// str[in] : Null-terminated string.
///
/// SUCCESS: Returns number of characters before null terminator.
/// FAILURE: Aborts via LOG_FATAL when `str` is NULL.
///
/// TAGS: Zstr, Length
size ZstrLen(Zstr str);

///
/// Compare two strings lexicographically (case-sensitive).
///
/// s1[in] : First string.
/// s2[in] : Second string.
///
/// SUCCESS: Returns 0 if equal, <0 if s1<s2, >0 if s1>s2.
/// FAILURE: Aborts via LOG_FATAL when either string pointer is NULL.
///
/// TAGS: Zstr, Comparison
i32 ZstrCompare(Zstr s1, Zstr s2);

///
/// Hash a `Zstr` for use as a generic map key. FNV-1a over the bytes
/// from `*key` up to (not including) the NUL terminator.
///
/// Typed signature; cast to `GenericHash` at the Map callback site.
///
/// key[in]  : Pointer to a `Zstr`.
/// size[in] : Ignored. Included for `GenericHash` callback compatibility
///            (the canvas-size of the key slot inside the Map).
///
/// SUCCESS : Returns a stable hash of the C-string's bytes. The string
///           and the pointer cell are not modified.
/// FAILURE : Function cannot fail when `key` and `*key` are valid.
///
/// TAGS: Zstr, Hash, Ops
///
u64 zstr_hash(const Zstr *key, u32 size);

///
/// Three-way lexicographic comparison of two `Zstr` values. Wraps
/// `ZstrCompare` and normalises the result to `-1` / `0` / `1`.
///
/// Typed signature; cast to `GenericCompare` at the Map callback site.
///
/// a[in]    : Pointer to the left `Zstr`.
/// b[in]    : Pointer to the right `Zstr`.
/// size[in] : Ignored. Included for `GenericCompare` callback
///            compatibility (the canvas-size of the key slot inside
///            the Map).
///
/// SUCCESS : Returns `0` when equal, `-1` when `*a < *b`, `1` when
///           `*a > *b`. Neither string is modified.
/// FAILURE : Function cannot fail when `a` / `b` and the strings they
///           point to are valid.
///
/// TAGS: Zstr, Compare, Ops
///
i32 zstr_compare(const Zstr *a, const Zstr *b, u32 size);

///
/// Compare two strings lexicographically up to n characters
/// (case-sensitive).
///
/// SUCCESS: Returns 0 if equal, <0 if s1<s2, >0 if s1>s2.
/// FAILURE: Aborts via LOG_FATAL when either string pointer is NULL.
///
/// TAGS: Zstr, Comparison
i32 ZstrCompareN(Zstr s1, Zstr s2, size n);

///
/// Compare two strings lexicographically, ignoring ASCII case.
/// Non-ASCII bytes are compared as-is (no Unicode case folding).
///
/// SUCCESS: Returns 0 if equal under ASCII-case folding, <0 if
///          lowered s1 < lowered s2, >0 otherwise.
/// FAILURE: Aborts via LOG_FATAL when either string pointer is NULL.
///
/// TAGS: Zstr, Comparison, IgnoreCase
i32 ZstrCompareIgnoreCase(Zstr s1, Zstr s2);

///
/// Compare two strings up to n characters, ignoring ASCII case.
///
/// SUCCESS: Returns 0 if equal under ASCII-case folding, <0 / >0
///          otherwise.
/// FAILURE: Aborts via LOG_FATAL when either string pointer is NULL.
///
/// TAGS: Zstr, Comparison, IgnoreCase
i32 ZstrCompareNIgnoreCase(Zstr s1, Zstr s2, size n);

///
/// Find the first occurrence of a character in a null-terminated string.
///
/// SUCCESS: Returns pointer to first occurrence or NULL if not found.
/// FAILURE: Aborts via LOG_FATAL when `str` is NULL.
///
/// TAGS: Zstr, Search
Zstr ZstrFindChar(Zstr str, char ch);

///
/// Find first occurrence of `needle` in `haystack`.
///
/// SUCCESS: Returns pointer to first occurrence or NULL if not found.
/// FAILURE: Aborts via LOG_FATAL when either pointer is NULL.
///
/// TAGS: Zstr, Search
Zstr ZstrFindSubstring(Zstr haystack, Zstr needle);

///
/// Find first occurrence of a substring of specified length.
///
/// SUCCESS: Returns pointer to first occurrence or NULL if not found.
/// FAILURE: Aborts via LOG_FATAL when either pointer is NULL.
///
/// TAGS: Zstr, Search
Zstr ZstrFindSubstringN(Zstr haystack, Zstr needle, size needle_len);

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
Zstr zstr_dup_n(Zstr src, size n, Allocator *alloc);
#define ZstrDupN(...)             OVERLOAD(ZstrDupN, __VA_ARGS__)
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
Zstr zstr_dup(Zstr src, Allocator *alloc);
#define ZstrDup(...)          OVERLOAD(ZstrDup, __VA_ARGS__)
#define ZstrDup_1(src)        zstr_dup((src), MisraScope)
#define ZstrDup_2(src, alloc) zstr_dup((src), ALLOCATOR_OF(alloc))

///
/// Clone callback for `Zstrs` (= `Vec(Zstr)`).
///
/// NOTE: Meant to be installed as the init handler on a `Zstrs` vector
///       so that VecInitCopy / element-level operations duplicate the
///       underlying buffers.
///
bool zstr_init_clone(void *dst, const void *src, const Allocator *alloc);

///
/// Deinit callback for `Zstrs` (= `Vec(Zstr)`). Releases the
/// buffer that `zstr_init_clone` allocated, then NULLs the slot.
///
void zstr_deinit(void *zs, const Allocator *alloc);

///
/// Convert a hex / decimal digit character to its numeric value.
/// Accepts `0`..`9`, `a`..`f`, `A`..`F` (case-insensitive for hex).
///
/// SUCCESS: Returns the digit value 0..15.
/// FAILURE: Returns -1 when `c` is not a valid hex / decimal digit.
///
/// TAGS: Zstr, Parse, Hex
i32 ZstrHexDigitValue(char c);

///
/// Decode one escape sequence starting at `*str`. `*str` must point at
/// the leading `\\`; on success the pointer is advanced past the
/// consumed escape. Handles single-character escapes
/// (`\\n`/`\\r`/`\\t`/`\\b`/`\\f`/`\\v`/`\\a`/`\\\\`/`\\"`/`\\'`/`\\0`),
/// hex escapes (`\\xNN`), and `\\u{...}` Unicode escapes that fit in a
/// single byte.
///
/// str[in,out] : Address of a `Zstr` cursor. Advanced past the
///               consumed escape on success.
///
/// SUCCESS: Returns the decoded byte; `*str` is advanced past the escape.
/// FAILURE: Returns 0; logs the malformed escape; `*str` may have been
///          partially advanced.
///
/// TAGS: Zstr, Parse, Escape
char ZstrProcessEscape(Zstr *str);

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
i64 ZstrToI64(Zstr s, Zstr *endptr);

///
/// Parse a decimal floating-point value. Accepts
/// `[+-]?digits(.digits)?([eE][+-]?digits)?`. Replaces libc `strtod`
/// for JSON / KvConfig numeric values. Not bit-exact on long mantissas.
///
/// SUCCESS: Returns the parsed value as f64.
/// FAILURE: Returns 0.0 when no digits are present.
///
/// TAGS: Zstr, Parse, Float
f64 ZstrToF64(Zstr s, Zstr *endptr);

#endif // MISRA_STD_ZSTR_H
