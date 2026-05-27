/// file      : std/container/str/insert.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Insertion functions for Str

#ifndef MISRA_STD_CONTAINER_STR_INSERT_H
#define MISRA_STD_CONTAINER_STR_INSERT_H

#include "Type.h"
#include <Misra/Std/Zstr.h>

#ifdef __cplusplus
extern "C" {
#endif

///
/// Insert a single character at `idx`, shifting trailing characters right.
///
/// str[in,out] : Str handle.
/// chr[in]     : Character to insert.
/// idx[in]     : Position in [0, length].
///
/// SUCCESS : Returns `true`.
/// FAILURE : Returns `false` on allocation failure. The string is unchanged.
///
/// TAGS: Str, Insert, Char
///
#define StrInsertCharAt(str, chr, idx) VecInsertR((str), (chr), (idx))

///
/// Aborting variant of `StrInsertCharAt`. Calls `LOG_FATAL` on allocation
/// failure.
///
/// TAGS: Str, Insert, Char, Must, Abort
///
#define StrMustInsertCharAt(str, chr, idx) VecMustInsertR((str), (chr), (idx))

///
/// Insert a contiguous run of characters into `str` at position `idx`,
/// shifting the trailing tail right. Three shapes are accepted via
/// arg-count + `_Generic` dispatch:
///
///   `StrInsert(str, src:Str *, idx)`           - copies all of `src`'s chars
///   `StrInsert(str, zstr:Zstr, idx)`           - copies up to the NUL terminator
///   `StrInsert(str, cstr:Zstr, idx, cstr_len)` - copies exactly `cstr_len` bytes
///
/// str[in,out]  : Destination Str.
/// src/zstr/cstr: Source bytes (Str / Zstr / raw counted view).
/// idx[in]      : Position in [0, length].
/// cstr_len[in] : Number of bytes (Cstr form only).
///
/// SUCCESS : Returns `true`; bytes are inserted, trailing characters are
///           shifted right by the inserted length.
/// FAILURE : Returns `false` on allocation failure. `str` is unchanged.
///
/// TAGS: Str, Insert, Range
///
#define StrInsert(...) MISRA_OVERLOAD(StrInsert, __VA_ARGS__)
#define StrInsert_3(str, src, idx)                                                                                     \
    _Generic(                                                                                                          \
        (src),                                                                                                         \
        Str *: VecInsertRangeR((str), (Zstr)StrBegin((Str *)(src)), (idx), StrLen((Str *)(src))),                      \
        Zstr:  VecInsertRangeR((str), (Zstr)(src), (idx), ZstrLen((Zstr)(src)))                                        \
    )
#define StrInsert_4(str, cstr, idx, cstr_len) VecInsertRangeR((str), (cstr), (idx), (cstr_len))

///
/// Aborting variant of `StrInsert`. Same shapes; calls `LOG_FATAL` on
/// allocation failure instead of returning `false`.
///
/// SUCCESS : Returns `true`.
/// FAILURE : Does not return; `LOG_FATAL` aborts the process.
///
/// TAGS: Str, Insert, Range, Must, Abort
///
#define StrMustInsert(...) MISRA_OVERLOAD(StrMustInsert, __VA_ARGS__)
#define StrMustInsert_3(str, src, idx)                                                                                 \
    _Generic(                                                                                                          \
        (src),                                                                                                         \
        Str *: VecMustInsertRangeR((str), (Zstr)StrBegin((Str *)(src)), (idx), StrLen((Str *)(src))),                  \
        Zstr:  VecMustInsertRangeR((str), (Zstr)(src), (idx), ZstrLen((Zstr)(src)))                                    \
    )
#define StrMustInsert_4(str, cstr, idx, cstr_len) VecMustInsertRangeR((str), (cstr), (idx), (cstr_len))

///
/// Push a counted byte range into the string at an arbitrary position.
/// Equivalent to `StrInsert` 4-arg (Cstr) form with the argument order suited for streaming
/// emitters that keep `(cstr, len, pos)` triples around.
///
/// SUCCESS : Returns `true`.
/// FAILURE : Returns `false` on allocation failure.
///
/// TAGS: Str, Push, Cstr, Range
///
#define StrPushCstr(str, cstr, len, pos) VecInsertRangeR((str), (cstr), (pos), (len))

///
/// Aborting variant of `StrPushCstr`.
///
/// TAGS: Str, Push, Cstr, Range, Must, Abort
///
#define StrMustPushCstr(str, cstr, len, pos) VecMustInsertRangeR((str), (cstr), (pos), (len))

///
/// Push a null-terminated string into the Str at the given position.
///
/// SUCCESS : Returns `true`.
/// FAILURE : Returns `false` on allocation failure.
///
/// TAGS: Str, Push, Zstr
///
#define StrPushZstr(str, zstr, pos) StrPushCstr((str), (zstr), ZstrLen(zstr), (pos))

///
/// Aborting variant of `StrPushZstr`.
///
/// TAGS: Str, Push, Zstr, Must, Abort
///
#define StrMustPushZstr(str, zstr, pos) StrMustPushCstr((str), (zstr), ZstrLen(zstr), (pos))

///
/// Append a counted byte range to the end of the Str.
///
/// SUCCESS : Returns `true`.
/// FAILURE : Returns `false` on allocation failure.
///
/// TAGS: Str, PushBack, Cstr, Range
///
#define StrPushBackCstr(str, cstr, len) VecPushBackArrR((str), (cstr), (len))

///
/// Aborting variant of `StrPushBackCstr`.
///
/// TAGS: Str, PushBack, Cstr, Range, Must, Abort
///
#define StrMustPushBackCstr(str, cstr, len) VecMustPushBackArrR((str), (cstr), (len))

///
/// Append a null-terminated string to the end of the Str.
///
/// SUCCESS : Returns `true`.
/// FAILURE : Returns `false` on allocation failure.
///
/// TAGS: Str, PushBack, Zstr
///
#define StrPushBackZstr(str, zstr) StrPushBackCstr((str), (zstr), ZstrLen((zstr)))

///
/// Aborting variant of `StrPushBackZstr`.
///
/// TAGS: Str, PushBack, Zstr, Must, Abort
///
#define StrMustPushBackZstr(str, zstr) StrMustPushBackCstr((str), (zstr), ZstrLen((zstr)))

///
/// Prepend a counted byte range at the front of the Str.
///
/// SUCCESS : Returns `true`.
/// FAILURE : Returns `false` on allocation failure.
///
/// TAGS: Str, PushFront, Cstr, Range
///
#define StrPushFrontCstr(str, cstr, len) VecPushFrontArrR((str), (cstr), (len))

///
/// Aborting variant of `StrPushFrontCstr`.
///
/// TAGS: Str, PushFront, Cstr, Range, Must, Abort
///
#define StrMustPushFrontCstr(str, cstr, len) VecMustPushFrontArrR((str), (cstr), (len))

///
/// Prepend a null-terminated string at the front of the Str.
///
/// SUCCESS : Returns `true`.
/// FAILURE : Returns `false` on allocation failure.
///
/// TAGS: Str, PushFront, Zstr
///
#define StrPushFrontZstr(str, zstr) StrPushFrontCstr((str), (zstr), ZstrLen((zstr)))

///
/// Aborting variant of `StrPushFrontZstr`.
///
/// TAGS: Str, PushFront, Zstr, Must, Abort
///
#define StrMustPushFrontZstr(str, zstr) StrMustPushFrontCstr((str), (zstr), ZstrLen((zstr)))

///
/// Append a single character to the end of the Str.
///
/// SUCCESS : Returns `true`.
/// FAILURE : Returns `false` on allocation failure.
///
/// TAGS: Str, PushBack, Char
///
#define StrPushBack(str, chr) VecPushBackR((str), (chr))

///
/// Aborting variant of `StrPushBack`.
///
/// TAGS: Str, PushBack, Char, Must, Abort
///
#define StrMustPushBack(str, chr) VecMustPushBackR((str), (chr))

///
/// Prepend a single character at the front of the Str.
///
/// SUCCESS : Returns `true`.
/// FAILURE : Returns `false` on allocation failure.
///
/// TAGS: Str, PushFront, Char
///
#define StrPushFront(str, chr) VecPushFrontR((str), (chr))

///
/// Aborting variant of `StrPushFront`.
///
/// TAGS: Str, PushFront, Char, Must, Abort
///
#define StrMustPushFront(str, chr) VecMustPushFrontR((str), (chr))

///
/// Merge `str2` into the end of `str` with L-value (ownership-transfer)
/// semantics. When `str` has no `copy_init` handler, `str2`'s storage is
/// adopted and `str2` is left in a clean empty state on success.
///
/// str[in,out]  : Destination Str.
/// str2[in,out] : Source Str. May be emptied on success.
///
/// SUCCESS : Returns `true`.
/// FAILURE : Returns `false` on allocation failure. Both strings are unchanged.
///
/// TAGS: Str, Merge, LValue, Ownership
///
#define StrMergeL(str, str2) VecMergeL((str), (str2))

///
/// Aborting variant of `StrMergeL`.
///
/// TAGS: Str, Merge, LValue, Must, Abort
///
#define StrMustMergeL(str, str2) VecMustMergeL((str), (str2))

///
/// Merge a copy of `str2` into the end of `str` with R-value (read-only-source)
/// semantics. The source is never emptied.
///
/// SUCCESS : Returns `true`.
/// FAILURE : Returns `false` on allocation failure.
///
/// TAGS: Str, Merge, RValue
///
#define StrMergeR(str, str2) VecMergeR((str), (str2))

///
/// Aborting variant of `StrMergeR`.
///
/// TAGS: Str, Merge, RValue, Must, Abort
///
#define StrMustMergeR(str, str2) VecMustMergeR((str), (str2))

///
/// Default merge alias for `StrMergeR` - preserves the source string.
/// Use `StrMergeL` when you want ownership transfer.
///
/// TAGS: Str, Merge
///
#define StrMerge(str, str2) StrMergeR((str), (str2))

///
/// Aborting variant of `StrMerge`.
///
/// TAGS: Str, Merge, Must, Abort
///
#define StrMustMerge(str, str2) StrMustMergeR((str), (str2))

//
// Generic Vec-shape passthrough aliases. These mirror every public VecInsert*
// / VecPushBack* / VecPushFront* / VecMerge* / VecInitClone macro under the
// Str namespace; the contracts are identical to the underlying Vec macros,
// just specialised to the char element type.
//

///
/// Insert a single character at the given index, L-value (ownership-transfer)
/// form. Same contract as VecInsertL, specialised for the char element.
///
/// str[in,out] : Str handle.
/// lval[in]    : Addressable char to insert.
/// idx[in]     : Position in [0, length].
///
/// SUCCESS : Returns `true`; the character was inserted and trailing characters
///           shifted right by one. When the string has no `copy_init` handler,
///           `lval` has been zeroed.
/// FAILURE : Returns `false` on allocation failure. Neither string nor `lval`
///           is modified.
///
/// TAGS: Str, Insert, Char, LValue
///
#define StrInsertL(str, lval, idx) VecInsertL((str), (lval), (idx))

///
/// Aborting variant of `StrInsertL`.
///
/// TAGS: Str, Insert, Char, LValue, Must, Abort
///
#define StrMustInsertL(str, lval, idx) VecMustInsertL((str), (lval), (idx))

///
/// Insert a single character at the given index, R-value form. Same contract
/// as VecInsertR, specialised for the char element.
///
/// SUCCESS : Returns `true`; the character was inserted, trailing characters
///           shifted right by one.
/// FAILURE : Returns `false` on allocation failure; the string is unchanged.
///
/// TAGS: Str, Insert, Char, RValue
///
#define StrInsertR(str, rval, idx) VecInsertR((str), (rval), (idx))

///
/// Aborting variant of `StrInsertR`.
///
/// TAGS: Str, Insert, Char, RValue, Must, Abort
///
#define StrMustInsertR(str, rval, idx) VecMustInsertR((str), (rval), (idx))

///
/// Default character-insert alias for `StrInsertL`. Same contract as VecInsert,
/// specialised for the char element.
///
/// TAGS: Str, Insert, Char
///
#define StrInsertChar(str, lval, idx) VecInsert((str), (lval), (idx))

///
/// Aborting variant of `StrInsertChar`.
///
/// TAGS: Str, Insert, Char, Must, Abort
///
#define StrMustInsertChar(str, lval, idx) VecMustInsert((str), (lval), (idx))

///
/// Insert a single character using fast (order-not-preserving) placement.
/// L-value form. Same contract as VecInsertFastL, specialised for the char
/// element.
///
/// TAGS: Str, Insert, Char, LValue, Fast, Unordered
///
#define StrInsertFastL(str, lval, idx) VecInsertFastL((str), (lval), (idx))

///
/// Aborting variant of `StrInsertFastL`.
///
/// TAGS: Str, Insert, Char, LValue, Fast, Must, Abort
///
#define StrMustInsertFastL(str, lval, idx) VecMustInsertFastL((str), (lval), (idx))

///
/// Insert a single character using fast (order-not-preserving) placement.
/// R-value form. Same contract as VecInsertFastR, specialised for the char
/// element.
///
/// TAGS: Str, Insert, Char, RValue, Fast, Unordered
///
#define StrInsertFastR(str, rval, idx) VecInsertFastR((str), (rval), (idx))

///
/// Aborting variant of `StrInsertFastR`.
///
/// TAGS: Str, Insert, Char, RValue, Fast, Must, Abort
///
#define StrMustInsertFastR(str, rval, idx) VecMustInsertFastR((str), (rval), (idx))

///
/// Default fast character-insert alias. Same contract as VecInsertFast,
/// specialised for the char element.
///
/// TAGS: Str, Insert, Char, Fast, Unordered
///
#define StrInsertFast(str, lval, idx) VecInsertFast((str), (lval), (idx))

///
/// Aborting variant of `StrInsertFast`.
///
/// TAGS: Str, Insert, Char, Fast, Must, Abort
///
#define StrMustInsertFast(str, lval, idx) VecMustInsertFast((str), (lval), (idx))

///
/// Insert a contiguous range of characters at the given index, preserving
/// order of trailing characters. L-value (ownership-transfer) form. Same
/// contract as VecInsertRangeL, specialised for the char element.
///
/// TAGS: Str, Insert, Range, LValue
///
#define StrInsertRangeL(str, varr, idx, count) VecInsertRangeL((str), (varr), (idx), (count))

///
/// Aborting variant of `StrInsertRangeL`.
///
/// TAGS: Str, Insert, Range, LValue, Must, Abort
///
#define StrMustInsertRangeL(str, varr, idx, count) VecMustInsertRangeL((str), (varr), (idx), (count))

///
/// Insert a contiguous range of characters at the given index. R-value form.
/// Same contract as VecInsertRangeR, specialised for the char element.
///
/// TAGS: Str, Insert, Range, RValue
///
#define StrInsertRangeR(str, varr, idx, count) VecInsertRangeR((str), (varr), (idx), (count))

///
/// Aborting variant of `StrInsertRangeR`.
///
/// TAGS: Str, Insert, Range, RValue, Must, Abort
///
#define StrMustInsertRangeR(str, varr, idx, count) VecMustInsertRangeR((str), (varr), (idx), (count))

///
/// Default character-range-insert alias. Same contract as VecInsertRange,
/// specialised for the char element.
///
/// TAGS: Str, Insert, Range
///
#define StrInsertRange(str, varr, idx, count) VecInsertRange((str), (varr), (idx), (count))

///
/// Aborting variant of `StrInsertRange`.
///
/// TAGS: Str, Insert, Range, Must, Abort
///
#define StrMustInsertRange(str, varr, idx, count) VecMustInsertRange((str), (varr), (idx), (count))

///
/// Insert a range of characters using fast (order-not-preserving) placement.
/// L-value form. Same contract as VecInsertRangeFastL, specialised for the
/// char element.
///
/// TAGS: Str, Insert, Range, LValue, Fast, Unordered
///
#define StrInsertRangeFastL(str, varr, idx, count) VecInsertRangeFastL((str), (varr), (idx), (count))

///
/// Aborting variant of `StrInsertRangeFastL`.
///
/// TAGS: Str, Insert, Range, LValue, Fast, Must, Abort
///
#define StrMustInsertRangeFastL(str, varr, idx, count) VecMustInsertRangeFastL((str), (varr), (idx), (count))

///
/// Insert a range of characters using fast placement. R-value form. Same
/// contract as VecInsertRangeFastR, specialised for the char element.
///
/// TAGS: Str, Insert, Range, RValue, Fast, Unordered
///
#define StrInsertRangeFastR(str, varr, idx, count) VecInsertRangeFastR((str), (varr), (idx), (count))

///
/// Aborting variant of `StrInsertRangeFastR`.
///
/// TAGS: Str, Insert, Range, RValue, Fast, Must, Abort
///
#define StrMustInsertRangeFastR(str, varr, idx, count) VecMustInsertRangeFastR((str), (varr), (idx), (count))

///
/// Default fast character-range-insert alias. Same contract as
/// VecInsertRangeFast, specialised for the char element.
///
/// TAGS: Str, Insert, Range, Fast, Unordered
///
#define StrInsertRangeFast(str, varr, idx, count) VecInsertRangeFast((str), (varr), (idx), (count))

///
/// Aborting variant of `StrInsertRangeFast`.
///
/// TAGS: Str, Insert, Range, Fast, Must, Abort
///
#define StrMustInsertRangeFast(str, varr, idx, count) VecMustInsertRangeFast((str), (varr), (idx), (count))

///
/// Append a contiguous range of characters to the end of the string. L-value
/// (ownership-transfer) form. Same contract as VecPushBackArrL, specialised
/// for the char element.
///
/// TAGS: Str, PushBack, Range, LValue
///
#define StrPushBackArrL(str, arr, count) VecPushBackArrL((str), (arr), (count))

///
/// Aborting variant of `StrPushBackArrL`.
///
/// TAGS: Str, PushBack, Range, LValue, Must, Abort
///
#define StrMustPushBackArrL(str, arr, count) VecMustPushBackArrL((str), (arr), (count))

///
/// Append a contiguous range of characters to the end of the string. R-value
/// form. Same contract as VecPushBackArrR, specialised for the char element.
///
/// TAGS: Str, PushBack, Range, RValue
///
#define StrPushBackArrR(str, arr, count) VecPushBackArrR((str), (arr), (count))

///
/// Aborting variant of `StrPushBackArrR`.
///
/// TAGS: Str, PushBack, Range, RValue, Must, Abort
///
#define StrMustPushBackArrR(str, arr, count) VecMustPushBackArrR((str), (arr), (count))

///
/// Default tail-append alias for `StrPushBackArrL`. Same contract as
/// VecPushBackArr, specialised for the char element.
///
/// TAGS: Str, PushBack, Range
///
#define StrPushBackArr(str, arr, count) VecPushBackArr((str), (arr), (count))

///
/// Aborting variant of `StrPushBackArr`.
///
/// TAGS: Str, PushBack, Range, Must, Abort
///
#define StrMustPushBackArr(str, arr, count) VecMustPushBackArr((str), (arr), (count))

///
/// Prepend a contiguous range of characters at the front of the string.
/// L-value (ownership-transfer) form. Same contract as VecPushFrontArrL,
/// specialised for the char element.
///
/// TAGS: Str, PushFront, Range, LValue
///
#define StrPushFrontArrL(str, arr, count) VecPushFrontArrL((str), (arr), (count))

///
/// Aborting variant of `StrPushFrontArrL`.
///
/// TAGS: Str, PushFront, Range, LValue, Must, Abort
///
#define StrMustPushFrontArrL(str, arr, count) VecMustPushFrontArrL((str), (arr), (count))

///
/// Prepend a contiguous range of characters at the front of the string.
/// R-value form. Same contract as VecPushFrontArrR, specialised for the char
/// element.
///
/// TAGS: Str, PushFront, Range, RValue
///
#define StrPushFrontArrR(str, arr, count) VecPushFrontArrR((str), (arr), (count))

///
/// Aborting variant of `StrPushFrontArrR`.
///
/// TAGS: Str, PushFront, Range, RValue, Must, Abort
///
#define StrMustPushFrontArrR(str, arr, count) VecMustPushFrontArrR((str), (arr), (count))

///
/// Default front-prepend alias for `StrPushFrontArrL`. Same contract as
/// VecPushFrontArr, specialised for the char element.
///
/// TAGS: Str, PushFront, Range
///
#define StrPushFrontArr(str, arr, count) VecPushFrontArr((str), (arr), (count))

///
/// Aborting variant of `StrPushFrontArr`.
///
/// TAGS: Str, PushFront, Range, Must, Abort
///
#define StrMustPushFrontArr(str, arr, count) VecMustPushFrontArr((str), (arr), (count))

///
/// Prepend a range using fast (order-not-preserving) placement. L-value form.
/// Same contract as VecPushFrontArrFastL, specialised for the char element.
///
/// TAGS: Str, PushFront, Range, LValue, Fast, Unordered
///
#define StrPushFrontArrFastL(str, arr, count) VecPushFrontArrFastL((str), (arr), (count))

///
/// Aborting variant of `StrPushFrontArrFastL`.
///
/// TAGS: Str, PushFront, Range, LValue, Fast, Must, Abort
///
#define StrMustPushFrontArrFastL(str, arr, count) VecMustPushFrontArrFastL((str), (arr), (count))

///
/// Prepend a range using fast (order-not-preserving) placement. R-value form.
/// Same contract as VecPushFrontArrFastR, specialised for the char element.
///
/// TAGS: Str, PushFront, Range, RValue, Fast, Unordered
///
#define StrPushFrontArrFastR(str, arr, count) VecPushFrontArrFastR((str), (arr), (count))

///
/// Aborting variant of `StrPushFrontArrFastR`.
///
/// TAGS: Str, PushFront, Range, RValue, Fast, Must, Abort
///
#define StrMustPushFrontArrFastR(str, arr, count) VecMustPushFrontArrFastR((str), (arr), (count))

///
/// Default fast front-prepend alias for `StrPushFrontArrFastL`. Same contract
/// as VecPushFrontArrFast, specialised for the char element.
///
/// TAGS: Str, PushFront, Range, Fast, Unordered
///
#define StrPushFrontArrFast(str, arr, count) VecPushFrontArrFast((str), (arr), (count))

///
/// Aborting variant of `StrPushFrontArrFast`.
///
/// TAGS: Str, PushFront, Range, Fast, Must, Abort
///
#define StrMustPushFrontArrFast(str, arr, count) VecMustPushFrontArrFast((str), (arr), (count))

///
/// Append a single character to the end of the string. L-value
/// (ownership-transfer) form. Same contract as VecPushBackL, specialised for
/// the char element.
///
/// TAGS: Str, PushBack, Char, LValue
///
#define StrPushBackL(str, val) VecPushBackL((str), (val))

///
/// Aborting variant of `StrPushBackL`.
///
/// TAGS: Str, PushBack, Char, LValue, Must, Abort
///
#define StrMustPushBackL(str, val) VecMustPushBackL((str), (val))

///
/// Append a single character to the end of the string. R-value form. Same
/// contract as VecPushBackR, specialised for the char element.
///
/// TAGS: Str, PushBack, Char, RValue
///
#define StrPushBackR(str, val) VecPushBackR((str), (val))

///
/// Aborting variant of `StrPushBackR`.
///
/// TAGS: Str, PushBack, Char, RValue, Must, Abort
///
#define StrMustPushBackR(str, val) VecMustPushBackR((str), (val))

///
/// Prepend a single character at the front of the string. L-value
/// (ownership-transfer) form. Same contract as VecPushFrontL, specialised for
/// the char element.
///
/// TAGS: Str, PushFront, Char, LValue
///
#define StrPushFrontL(str, val) VecPushFrontL((str), (val))

///
/// Aborting variant of `StrPushFrontL`.
///
/// TAGS: Str, PushFront, Char, LValue, Must, Abort
///
#define StrMustPushFrontL(str, val) VecMustPushFrontL((str), (val))

///
/// Prepend a single character at the front of the string. R-value form. Same
/// contract as VecPushFrontR, specialised for the char element.
///
/// TAGS: Str, PushFront, Char, RValue
///
#define StrPushFrontR(str, val) VecPushFrontR((str), (val))

///
/// Aborting variant of `StrPushFrontR`.
///
/// TAGS: Str, PushFront, Char, RValue, Must, Abort
///
#define StrMustPushFrontR(str, val) VecMustPushFrontR((str), (val))

///
/// Reinitialise `strd` as a deep clone of `strs`. Same contract as
/// VecInitClone, specialised for the char element.
///
/// strd[out] : Destination Str handle (current contents are deinit'd first).
/// strs[in]  : Source Str handle.
///
/// SUCCESS : Returns `true`; `strd` now holds a deep copy of every character
///           in `strs` and inherits its copy/alignment/allocator configuration.
/// FAILURE : Returns `false` on allocation failure; `strd` is left in a valid
///           but partially-populated state - call StrDeinit before reuse.
///
/// TAGS: Str, Clone, Init, DeepCopy
///
#define StrInitClone(strd, strs) VecInitClone((strd), (strs))

///
/// Aborting variant of `StrInitClone`.
///
/// TAGS: Str, Clone, Init, DeepCopy, Must, Abort
///
#define StrMustInitClone(strd, strs) VecMustInitClone((strd), (strs))

#ifdef __cplusplus
}
#endif

#endif // MISRA_STD_CONTAINER_STR_INSERT_H
