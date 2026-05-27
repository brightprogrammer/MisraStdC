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
/// Walk each character of `str` forward, binding `chr` to the character
/// value at index `idx`. Both `chr` and `idx` are stamped by the macro
/// and are visible inside the loop body.
/// See `VecForeachIdx` for the full SUCCESS/FAILURE contract.
///
/// TAGS: Str, Foreach, Iterate
///
#define StrForeachIdx(str, chr, idx) VecForeachIdx((str), (chr), idx)

///
/// Walk each character of `str` backward, binding `chr` to the character
/// value at index `idx` from `length - 1` down to `0`. Both `chr` and
/// `idx` are stamped by the macro.
/// See `VecForeachReverseIdx` for the full SUCCESS/FAILURE contract.
///
/// TAGS: Str, Foreach, Iterate, Reverse
///
#define StrForeachReverseIdx(str, chr, idx) VecForeachReverseIdx((str), (chr), idx)

///
/// Walk each character of `str` forward, binding `chrptr` to a pointer
/// to the character at index `idx`. Use this form when the body needs
/// to mutate characters in place. Both `chrptr` and `idx` are stamped
/// by the macro.
/// See `VecForeachPtrIdx` for the full SUCCESS/FAILURE contract.
///
/// TAGS: Str, Foreach, Iterate
///
#define StrForeachPtrIdx(str, chrptr, idx) VecForeachPtrIdx((str), (chrptr), idx)

///
/// Walk each character of `str` backward, binding `chrptr` to a pointer
/// to the character at index `idx` from `length - 1` down to `0`. Both
/// `chrptr` and `idx` are stamped by the macro.
/// See `VecForeachPtrReverseIdx` for the full SUCCESS/FAILURE contract.
///
/// TAGS: Str, Foreach, Iterate, Reverse
///
#define StrForeachReversePtrIdx(str, chrptr, idx) VecForeachPtrReverseIdx((str), (chrptr), idx)

///
/// Walk each character of `str` forward, binding `chr` to the current
/// character value. Convenience wrapper around `StrForeachIdx` with an
/// internally-managed index.
/// See `VecForeach` for the full SUCCESS/FAILURE contract.
///
/// TAGS: Str, Foreach, Iterate
///
#define StrForeach(str, chr) VecForeach((str), (chr))

///
/// Walk each character of `str` backward, binding `chr` to the current
/// character value. Convenience wrapper around `StrForeachReverseIdx`.
/// See `VecForeachReverse` for the full SUCCESS/FAILURE contract.
///
/// TAGS: Str, Foreach, Iterate, Reverse
///
#define StrForeachReverse(str, chr) VecForeachReverse((str), (chr))

///
/// Walk each character of `str` forward, binding `chrptr` to a pointer
/// to the current character. Use when the body needs to mutate
/// characters in place.
/// See `VecForeachPtr` for the full SUCCESS/FAILURE contract.
///
/// TAGS: Str, Foreach, Iterate
///
#define StrForeachPtr(str, chrptr) VecForeachPtr((str), (chrptr))

///
/// Walk each character of `str` backward, binding `chrptr` to a pointer
/// to the current character. Use when the body needs to mutate
/// characters in place.
/// See `VecForeachPtrReverse` for the full SUCCESS/FAILURE contract.
///
/// TAGS: Str, Foreach, Iterate, Reverse
///
#define StrForeachPtrReverse(str, chrptr) VecForeachPtrReverse((str), (chrptr))

///
/// Walk characters of `str` in the half-open range `[start, end)`,
/// binding `chr` to the character value at index `idx`. Both `chr` and
/// `idx` are stamped by the macro.
/// See `VecForeachInRangeIdx` for the full SUCCESS/FAILURE contract.
///
/// TAGS: Str, Foreach, Iterate, Range
///
#define StrForeachInRangeIdx(str, chr, idx, start, end) VecForeachInRangeIdx((str), (chr), idx, (start), (end))

///
/// Walk characters of `str` in the half-open range `[start, end)`,
/// binding `chr` to the current character value. Convenience wrapper
/// around `StrForeachInRangeIdx` with an internally-managed index.
/// See `VecForeachInRange` for the full SUCCESS/FAILURE contract.
///
/// TAGS: Str, Foreach, Iterate, Range
///
#define StrForeachInRange(str, chr, start, end) VecForeachInRange((str), (chr), (start), (end))

///
/// Walk characters of `str` in the half-open range `[start, end)`,
/// binding `chrptr` to a pointer to the character at index `idx`. Both
/// `chrptr` and `idx` are stamped by the macro.
/// See `VecForeachPtrInRangeIdx` for the full SUCCESS/FAILURE contract.
///
/// TAGS: Str, Foreach, Iterate, Range
///
#define StrForeachPtrInRangeIdx(str, chrptr, idx, start, end)                                                          \
    VecForeachPtrInRangeIdx((str), (chrptr), idx, (start), (end))

///
/// Walk characters of `str` in the half-open range `[start, end)`,
/// binding `chrptr` to a pointer to the current character. Convenience
/// wrapper around `StrForeachPtrInRangeIdx`.
/// See `VecForeachPtrInRange` for the full SUCCESS/FAILURE contract.
///
/// TAGS: Str, Foreach, Iterate, Range
///
#define StrForeachPtrInRange(str, chrptr, start, end) VecForeachPtrInRange((str), (chrptr), (start), (end))

#ifdef __cplusplus
}
#endif

#endif // MISRA_STD_CONTAINER_STR_FOREACH_H
