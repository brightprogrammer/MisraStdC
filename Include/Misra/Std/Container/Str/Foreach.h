/// file      : std/container/str/foreach.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Iteration macros for Str

#ifndef MISRA_STD_CONTAINER_STR_FOREACH_H
#define MISRA_STD_CONTAINER_STR_FOREACH_H

#include "Type.h"

#ifdef __cplusplus
extern "C" {
#endif

///
/// Iterate over each character `chr` of the given Str `str` at each index `idx`.
/// This macro is a direct alias for `VecForeachIdx` specialized for Str.
/// The variables `chr` and `idx` are declared and defined by the underlying macro.
///
/// str[in,out] : Str to iterate over.
/// chr[in]     : Name of the variable to be used which will contain the character
///               at the iterated index `idx`. The type of `chr` will likely be
///               the character type used by the `Str` implementation (e.g., `char`).
/// idx[in]     : Name of the variable to be used for iterating over indices (i64).
///
#define StrForeachIdx(str, chr, idx) VecForeachIdx((str), (chr), idx)

///
/// Iterate over each character `chr` of the given Str `str` in reverse order at each index `idx`.
/// This macro is a direct alias for `VecForeachReverseIdx` specialized for Str.
/// The variables `chr` and `idx` are declared and defined by the underlying macro.
///
/// str[in,out] : Str to iterate over.
/// chr[in]     : Name of the variable to be used which will contain the character
///               at the iterated index `idx`. The type of `chr` will likely be
///               the character type used by the `Str` implementation (e.g., `char`).
/// idx[in]     : Name of the variable to be used for iterating over indices (i64).
///
#define StrForeachReverseIdx(str, chr, idx) VecForeachReverseIdx((str), (chr), idx)

///
/// Iterate over each character pointer `chrptr` of the given Str `str` at each index `idx`.
/// This macro is a direct alias for `VecForeachPtrIdx` specialized for Str.
/// The variables `chrptr` and `idx` are declared and defined by the underlying macro.
///
/// str[in,out] : Str to iterate over.
/// chrptr[in]  : Name of the pointer variable to be used which will point to the
///               character at the iterated index `idx`. The type of `chrptr` will
///               likely be a pointer to the character type used by the `Str`
///               implementation (e.g., `char*`).
/// idx[in]     : Name of the variable to be used for iterating over indices (i64).
///
#define StrForeachPtrIdx(str, chrptr, idx) VecForeachPtrIdx((str), (chrptr), idx)

///
/// Iterate over each character pointer `chrptr` of the given Str `str` in reverse order at each index `idx`.
/// This macro is a direct alias for `VecForeachPtrReverseIdx` specialized for Str.
/// The variables `chrptr` and `idx` are declared and defined by the underlying macro.
///
/// str[in,out] : Str to iterate over.
/// chrptr[in]  : Name of the pointer variable to be used which will point to the
///               character at the iterated index `idx`. The type of `chrptr` will
///               likely be a pointer to the character type used by the `Str`
///               implementation (e.g., `char*`).
/// idx[in]     : Name of the variable to be used for iterating over indices (i64).
///
#define StrForeachReversePtrIdx(str, chrptr, idx) VecForeachPtrReverseIdx((str), (chrptr), idx)

///
/// Iterate over each character `chr` of the given Str `str`.
/// This is a convenience macro that iterates forward using an internally managed index.
/// The variable `chr` is declared and defined by the underlying `VecForeach` macro.
///
/// str[in,out] : Str to iterate over.
/// chr[in]     : Name of the variable to be used which will contain the character of the
///               current element during iteration. The type of `chr` will likely be
///               the character type used by the `Str` implementation (e.g., `char`).
///
#define StrForeach(str, chr) VecForeach((str), (chr))

///
/// Iterate over each character `chr` of the given Str `str` in reverse order.
/// This is a convenience macro that iterates backward using an internally managed index.
/// The variable `chr` is declared and defined by the underlying `VecForeachReverse` macro.
///
/// str[in,out] : Str to iterate over.
/// chr[in]     : Name of the variable to be used which will contain the character of the
///               current element during iteration. The type of `chr` will likely be
///               the character type used by the `Str` implementation (e.g., `char`).
///
#define StrForeachReverse(str, chr) VecForeachReverse((str), (chr))

///
/// Iterate over each character pointer `chrptr` of the given Str `str`.
/// This is a convenience macro that iterates forward using an internally managed index
/// and provides a pointer to each character. The variable `chrptr` is declared and
/// defined by the underlying `VecForeachPtr` macro as a pointer to the character type.
///
/// str[in,out] : Str to iterate over.
/// chrptr[in]  : Name of the pointer variable to be used which will point to the
///               current character during iteration. The type of `chrptr` will
///               likely be a pointer to the character type used by the `Str`
///               implementation (e.g., `char*`).
///
#define StrForeachPtr(str, chrptr) VecForeachPtr((str), (chrptr))

///
/// Iterate over each character pointer `chrptr` of the given Str `str` in reverse order.
/// This is a convenience macro that iterates backward using an internally managed index
/// and provides a pointer to each character. The variable `chrptr` is declared and
/// defined by the underlying `VecForeachPtrReverse` macro as a pointer to the character type.
///
/// str[in,out] : Str to iterate over.
/// chrptr[in]  : Name of the pointer variable to be used which will point to the
///               current character during iteration. The type of `chrptr` will
///               likely be a pointer to the character type used by the `Str`
///               implementation (e.g., `char*`).
///
#define StrForeachPtrReverse(str, chrptr) VecForeachPtrReverse((str), (chrptr))

///
/// Iterate over characters in a specific range of the given Str `str` at each index `idx`.
/// This macro is a direct alias for `VecForeachInRangeIdx` specialized for Str.
/// The variables `chr` and `idx` are declared and defined by the underlying macro.
///
/// str[in,out]  : Str to iterate over.
/// chr[in]      : Name of variable to be used which'll contain character at iterated index `idx`.
/// idx[in]      : Name of variable to be used for iterating over indices.
/// start[in]    : Starting index (inclusive).
/// end[in]      : Ending index (exclusive).
///
#define StrForeachInRangeIdx(str, chr, idx, start, end) VecForeachInRangeIdx((str), (chr), idx, (start), (end))

///
/// Iterate over characters in a specific range of the given Str `str`.
/// This is a convenience macro that iterates over a range using an internally managed index.
/// The variable `chr` is declared and defined by the underlying `VecForeachInRange` macro.
///
/// str[in,out]  : Str to iterate over.
/// chr[in]      : Name of variable to be used which'll contain character of the current element.
/// start[in]    : Starting index (inclusive).
/// end[in]      : Ending index (exclusive).
///
#define StrForeachInRange(str, chr, start, end) VecForeachInRange((str), (chr), (start), (end))

///
/// Iterate over characters in a specific range of the given Str `str` at each index `idx` (as pointers).
/// This macro is a direct alias for `VecForeachPtrInRangeIdx` specialized for Str.
/// The variables `chrptr` and `idx` are declared and defined by the underlying macro.
///
/// str[in,out]  : Str to iterate over.
/// chrptr[in]   : Name of pointer variable to be used which'll point to character at iterated index `idx`.
/// idx[in]      : Name of variable to be used for iterating over indices.
/// start[in]    : Starting index (inclusive).
/// end[in]      : Ending index (exclusive).
///
#define StrForeachPtrInRangeIdx(str, chrptr, idx, start, end)                                                          \
    VecForeachPtrInRangeIdx((str), (chrptr), idx, (start), (end))

///
/// Iterate over characters in a specific range of the given Str `str` (as pointers).
/// This is a convenience macro that iterates over a range using an internally managed index
/// and provides a pointer to each character. The variable `chrptr` is declared and defined
/// by the underlying `VecForeachPtrInRange` macro as a pointer to the character type.
///
/// str[in,out]  : Str to iterate over.
/// chrptr[in]   : Name of pointer variable to be used which'll point to the current character.
/// start[in]    : Starting index (inclusive).
/// end[in]      : Ending index (exclusive).
///
#define StrForeachPtrInRange(str, chrptr, start, end) VecForeachPtrInRange((str), (chrptr), (start), (end))

#ifdef __cplusplus
}
#endif

#endif // MISRA_STD_CONTAINER_STR_FOREACH_H
