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
/// body        : Body of this foreach loop.
///
#define StrForeachIdx(str, chr, idx, body) VecForeachIdx((str), (chr), idx, {body})

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
/// body        : Body of this foreach loop.
///
#define StrForeachReverseIdx(str, chr, idx, body) VecForeachReverseIdx((str), (chr), idx, {body})

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
/// body        : Body of this foreach loop.
///
#define StrForeachPtrIdx(str, chrptr, idx, body) VecForeachPtrIdx((str), (chrptr), idx, {body})

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
/// body        : Body of this foreach loop.
///
#define StrForeachReversePtrIdx(str, chrptr, idx, body) VecForeachPtrReverseIdx((str), (chrptr), idx, {body})

///
/// Iterate over each character `chr` of the given Str `str`.
/// This is a convenience macro that iterates forward using an internally managed index.
/// The variable `chr` is declared and defined by the underlying `VecForeach` macro.
///
/// str[in,out] : Str to iterate over.
/// chr[in]     : Name of the variable to be used which will contain the character of the
///               current element during iteration. The type of `chr` will likely be
///               the character type used by the `Str` implementation (e.g., `char`).
/// body        : The block of code to be executed for each character of the Str.
///
#define StrForeach(str, chr, body) VecForeach((str), (chr), {body})

///
/// Iterate over each character `chr` of the given Str `str` in reverse order.
/// This is a convenience macro that iterates backward using an internally managed index.
/// The variable `chr` is declared and defined by the underlying `VecForeachReverse` macro.
///
/// str[in,out] : Str to iterate over.
/// chr[in]     : Name of the variable to be used which will contain the character of the
///               current element during iteration. The type of `chr` will likely be
///               the character type used by the `Str` implementation (e.g., `char`).
/// body        : The block of code to be executed for each character of the Str.
///
#define StrForeachReverse(str, chr, body) VecForeachReverse((str), (chr), {body})

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
/// body        : The block of code to be executed for each character of the Str.
///
#define StrForeachPtr(str, chrptr, body) VecForeachPtr((str), (chrptr), {body})

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
/// body        : The block of code to be executed for each character of the Str.
///
#define StrForeachPtrReverse(str, chrptr, body) VecForeachPtrReverse((str), (chrptr), {body})

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
/// body         : Body of this foreach loop.
///
/// SUCCESS : The `body` is executed for each character of the Str `str` from the
///           `start` index to the `end-1` index.
/// FAILURE : If the Str `str` is NULL, its length is zero, or the range is invalid,
///           the loop body will not be executed. Any access to an invalid index will
///           result in a fatal log message and program termination.
///
#define StrForeachInRangeIdx(str, chr, idx, start, end, body)                                                          \
    VecForeachInRangeIdx((str), (chr), idx, (start), (end), {body})

///
/// Iterate over characters in a specific range of the given Str `str`.
/// This is a convenience macro that iterates over a range using an internally managed index.
/// The variable `chr` is declared and defined by the underlying `VecForeachInRange` macro.
///
/// str[in,out]  : Str to iterate over.
/// chr[in]      : Name of variable to be used which'll contain character of the current element.
/// start[in]    : Starting index (inclusive).
/// end[in]      : Ending index (exclusive).
/// body         : Body of this foreach loop.
///
/// SUCCESS : The `body` is executed for each character of the Str `str` from the
///           `start` index to the `end-1` index.
/// FAILURE : If the Str `str` is NULL, its length is zero, or the range is invalid,
///           the loop body will not be executed. Any failures within the `VecForeachInRangeIdx`
///           macro will result in a fatal log message and program termination.
///
#define StrForeachInRange(str, chr, start, end, body) VecForeachInRange((str), (chr), (start), (end), {body})

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
/// body         : Body of this foreach loop.
///
/// SUCCESS : The `body` is executed for each character of the Str `str` from the
///           `start` index to the `end-1` index, with `chrptr` pointing to each character.
/// FAILURE : If the Str `str` is NULL, its length is zero, or the range is invalid,
///           the loop body will not be executed. Any access to an invalid index will
///           result in a fatal log message and program termination.
///
#define StrForeachPtrInRangeIdx(str, chrptr, idx, start, end, body)                                                    \
    VecForeachPtrInRangeIdx((str), (chrptr), idx, (start), (end), {body})

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
/// body         : Body of this foreach loop.
///
/// SUCCESS : The `body` is executed for each character of the Str `str` from the
///           `start` index to the `end-1` index, with `chrptr` pointing to each character.
/// FAILURE : If the Str `str` is NULL, its length is zero, or the range is invalid,
///           the loop body will not be executed. Any failures within the `VecForeachPtrInRangeIdx`
///           macro will result in a fatal log message and program termination.
///
#define StrForeachPtrInRange(str, chrptr, start, end, body)                                                            \
    VecForeachPtrInRange((str), (chrptr), (start), (end), {body})

#ifdef __cplusplus
}
#endif

#endif // MISRA_STD_CONTAINER_STR_FOREACH_H
