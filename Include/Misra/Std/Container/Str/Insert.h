/// file      : std/container/str/insert.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Insertion functions for Str

#ifndef MISRA_STD_CONTAINER_STR_INSERT_H
#define MISRA_STD_CONTAINER_STR_INSERT_H

#include "Type.h"

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
/// Insert a counted byte range from a C buffer at the given position.
///
/// str[in,out] : Str handle.
/// cstr[in]    : Source byte buffer. Must be non-NULL when `len > 0`.
/// idx[in]     : Position in [0, length].
/// len[in]     : Number of bytes to insert.
///
/// SUCCESS : Returns `true`.
/// FAILURE : Returns `false` on allocation failure.
///
/// TAGS: Str, Insert, Cstr, Range
///
#define StrInsertCstr(str, cstr, idx, len) VecInsertRangeR((str), (cstr), (idx), (len))

///
/// Aborting variant of `StrInsertCstr`.
///
/// TAGS: Str, Insert, Cstr, Range, Must, Abort
///
#define StrMustInsertCstr(str, cstr, idx, len) VecMustInsertRangeR((str), (cstr), (idx), (len))

///
/// Insert a null-terminated C string at the given position. Length is derived
/// from `ZstrLen(zstr)`.
///
/// str[in,out] : Str handle.
/// zstr[in]    : Null-terminated source string.
/// idx[in]     : Position in [0, length].
///
/// SUCCESS : Returns `true`.
/// FAILURE : Returns `false` on allocation failure.
///
/// TAGS: Str, Insert, Zstr
///
#define StrInsertZstr(str, zstr, idx) StrInsertCstr((str), (zstr), (idx), ZstrLen(zstr))

///
/// Aborting variant of `StrInsertZstr`.
///
/// TAGS: Str, Insert, Zstr, Must, Abort
///
#define StrMustInsertZstr(str, zstr, idx) StrMustInsertCstr((str), (zstr), (idx), ZstrLen(zstr))

///
/// Insert the contents of `str2` into `str` at the given position.
///
/// str[in,out] : Str handle to insert into.
/// str2[in]    : Source Str (read-only).
/// idx[in]     : Position in [0, length].
///
/// SUCCESS : Returns `true`.
/// FAILURE : Returns `false` on allocation failure.
///
/// TAGS: Str, Insert, Str
///
#define StrInsert(str, str2, idx) StrInsertCstr((str), (str2)->data, (idx), (str2)->length)

///
/// Aborting variant of `StrInsert`.
///
/// TAGS: Str, Insert, Str, Must, Abort
///
#define StrMustInsert(str, str2, idx) StrMustInsertCstr((str), (str2)->data, (idx), (str2)->length)

///
/// Push a counted byte range into the string at an arbitrary position.
/// Equivalent to `StrInsertCstr` with the argument order suited for streaming
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

#ifdef __cplusplus
}
#endif

#endif // MISRA_STD_CONTAINER_STR_INSERT_H
