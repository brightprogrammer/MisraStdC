///
/// file      : std/memory.c
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Memory manipulation functions

#ifndef MISRA_STD_MEMORY_H
#define MISRA_STD_MEMORY_H

#include <Misra/Std/Container/Common.h>
#include <Misra/Std/Container/Vec/Type.h>
#include <Misra/Types.h>

typedef Vec(const char *) Zstrs;

///
/// Compare memory regions.
/// A zero byte count returns 0 without reading either pointer.
///
/// p1[in]  : First memory region.
/// p2[in]  : Second memory region.
/// n[in]   : Number of bytes to compare.
///
/// SUCCESS: Returns 0 if equal, <0 if p1<p2, >0 if p1>p2.
/// FAILURE: Function cannot fail - always returns comparison result.
///
/// TAGS: Memory, Comparison, Safety
i32 MemCompare(const void *p1, const void *p2, size n);

///
/// Copy memory from source to destination.
/// A zero byte count returns `dst` without reading either pointer.
///
/// dst[out] : Destination memory region.
/// src[in]  : Source memory region.
/// n[in]    : Number of bytes to copy.
///
/// SUCCESS: Returns destination pointer.
/// FAILURE: Function cannot fail if regions don't overlap.
///
/// TAGS: Memory, Copy, Safety
void *MemCopy(void *dst, const void *src, size n);

///
/// Move memory from source to destination, handling overlapping regions.
/// A zero byte count returns `dst` without reading either pointer.
///
/// dst[out] : Destination memory region.
/// src[in]  : Source memory region.
/// n[in]    : Number of bytes to move.
///
/// SUCCESS: Returns destination pointer.
/// FAILURE: Function cannot fail.
///
/// TAGS: Memory, Move, Safety
void *MemMove(void *dst, const void *src, size n);

///
/// Set memory region to a value.
/// A zero byte count returns `dst` without writing to it.
///
/// dst[out] : Memory region to set.
/// val[in]  : Value to set (converted to unsigned char).
/// n[in]    : Number of bytes to set.
///
/// SUCCESS: Returns destination pointer.
/// FAILURE: Function cannot fail.
///
/// TAGS: Memory, Set, Safety
void *MemSet(void *dst, i32 val, size n);

///
/// Generic in-place sort over a flat array of fixed-size items.
/// Quicksort with median-of-three pivot, insertion-sort fallback for
/// small partitions, tail-iteration on the larger side to bound stack
/// depth at O(log n). Stable across compilers / platforms because we
/// don't depend on libc's `qsort`.
///
/// base[in,out] : Pointer to the first element of the array.
/// n_items[in]  : Number of items in the array.
/// item_size[in]: Size of each item in bytes.
/// cmp[in]      : Comparator returning <0, 0, >0 like `strcmp`.
///
/// SUCCESS: Returns; the array is sorted in ascending order per `cmp`.
/// FAILURE: No failure mode. If `n_items < 2` or `cmp` is NULL the
///          call is a no-op.
///
/// TAGS: Memory, Sort, Generic
///
void MemSort(void *base, size n_items, size item_size, GenericCompare cmp);

///
/// Get length of a null-terminated string.
///
/// str[in] : Null-terminated string.
///
/// SUCCESS: Returns number of characters before null terminator.
/// FAILURE: Function cannot fail if str is valid.
///
/// TAGS: String, Length, Safety
size ZstrLen(const char *str);

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
i32 ZstrCompare(const char *s1, const char *s2);

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
i32 ZstrCompareN(const char *s1, const char *s2, size n);

///
/// Find the first occurrence of a character in a null-terminated string.
///
/// str[in] : String to search in.
/// ch[in]  : Character to search for.
///
/// SUCCESS: Returns pointer to first occurrence or NULL if not found.
/// FAILURE: Function cannot fail if `str` is valid.
///
/// TAGS: String, Search, Safety
char *ZstrFindChar(const char *str, char ch);

///
/// Duplicates a string up to the specified length.
/// Creates a new null-terminated string by allocating memory and copying
/// at most n characters from the source string.
///
/// src[in]   : Source string to duplicate.
/// n[in]     : Maximum number of characters to copy.
/// alloc[in] : Allocator that owns the returned buffer.
///
/// SUCCESS : Returns a pointer to the newly allocated duplicate string.
/// FAILURE : Returns NULL if memory allocation fails.
///
/// TAGS: String, Memory, Allocation
///
char *ZstrDupN(const char *src, size n, Allocator *alloc);

///
/// Duplicates a string.
/// Creates a new null-terminated string by allocating memory and copying
/// the source string in full.
///
/// src[in]   : Source string to duplicate.
/// alloc[in] : Allocator that owns the returned buffer.
///
/// SUCCESS : Returns a pointer to the newly allocated duplicate string.
/// FAILURE : Returns NULL if memory allocation fails or if src is NULL.
///
/// TAGS: String, Memory, Allocation
///
char *ZstrDup(const char *src, Allocator *alloc);

///
/// Init clone method for zero-terminated strings.
///
/// NOTE: This is meant to be used as init method with `Zstrs` vector which is basically
///       a typedef of `Vec(const char*)`.
///
/// dst[out]  : Pointer to zero-terminated string to store cloned string pointer into.
/// src[in]   : Pointer to zero-terminated string to make clone of.
/// alloc[in] : Allocator that owns the cloned buffer.
///
/// SUCCESS: Returns true
/// FAILURE: May abort with a log message or may return false depending on severity of situation.
///
bool zstr_init_clone(void *dst, const void *src, const Allocator *alloc);

///
/// Deinit method for zero-terminated strings.
///
/// NOTE: This is meant to be used as deinit method with `Zstrs` vector which is basically
///       a typedef of `Vec(const char*)`.
///
/// zs[in]    : Pointer to zero-terminated string to be destroyed.
/// alloc[in] : Allocator that originally owned the string buffer (same one passed to clone).
///
/// SUCCESS: Returns.
/// FAILURE: Does not return.
///
void zstr_deinit(void *zs, const Allocator *alloc);

///
/// Parse a signed decimal integer from a null-terminated string.
/// Skips ASCII whitespace, accepts an optional leading sign, then
/// consumes the longest run of `0..9`. Drops the libc `strtoll` /
/// `strtol` dependency for callers that only need base-10.
///
/// s[in]       : Source string.
/// endptr[out] : If non-NULL, set to the first byte past the last
///               digit consumed (or to `s` if no digits were found).
///
/// SUCCESS: Returns the parsed value as i64. Overflow wraps in the
///          unsigned accumulator before sign application; callers
///          that care should validate the value externally.
/// FAILURE: Returns 0 when no digits are present.
///
/// TAGS: String, Parse, Integer
///
i64 ZstrToI64(const char *s, char **endptr);

///
/// Parse a decimal floating-point value from a null-terminated
/// string. Accepts `[+-]?digits(.digits)?([eE][+-]?digits)?`. Skips
/// ASCII whitespace before the sign. Not bit-exact: precision-loss
/// is possible on long mantissas. Adequate for JSON / KvConfig
/// numeric values; replaces libc `strtod`.
///
/// s[in]       : Source string.
/// endptr[out] : If non-NULL, set to the first byte past the last
///               character consumed (matches strtod's contract).
///
/// SUCCESS: Returns the parsed value as f64.
/// FAILURE: Returns 0.0 when no digits are present.
///
/// TAGS: String, Parse, Float
///
f64 ZstrToF64(const char *s, char **endptr);

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
char *ZstrFindSubstring(const char *haystack, const char *needle);

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
char *ZstrFindSubstringN(const char *haystack, const char *needle, size needle_len);

#endif // MISRA_STD_MEMORY_H
