/// file      : std/container/str/insert.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Insertion functions for Str

#ifndef MISRA_STD_CONTAINER_STR_INSERT_H
#define MISRA_STD_CONTAINER_STR_INSERT_H

#include "Type.h"
#include <Misra/Std/Container/Vec/Insert.h>
#include <Misra/Std/Zstr.h>

#ifdef __cplusplus
extern "C" {
#endif

///
/// Insert a single character into `str` at position `idx`, shifting
/// trailing characters right. Three shapes:
///
///   `StrInsertL(str, chr, idx)`  - L-form. `chr` must be a writeable
///                                  `char` lvalue (`VecInsertL` zeroes
///                                  it on success). Character literals
///                                  (`'x'` -- type `int`) cannot be
///                                  used here; bind to a `char` first.
///   `StrInsertR(str, chr, idx)`  - R-form. `chr` may be a `char` lvalue
///                                  or any value implicitly convertible
///                                  to `char` (in particular, `int`
///                                  character literals like `'x'`).
///   `StrInsert(str, chr, idx)`   - Unsuffixed default = L-form.
///
/// Multi-character insertion is `StrInsertMany` (Zstr / Cstr), below.
/// Merging another `Str` is `StrMergeL` / `StrMergeR`.
///
/// SUCCESS : Returns `true`; one character inserted; trailing
///           characters shifted right by one.
/// FAILURE : Returns `false` on allocation failure. `str` unchanged.
///
/// TAGS: Str, Insert, Char
///
#define StrInsertL(str, lval, idx) VecInsertL((str), (lval), (idx))
#define StrInsertR(str, rval, idx) VecInsertR((str), (rval), (idx))
#define StrInsert(str, val, idx)   StrInsertL((str), (val), (idx))

///
/// Aborting variant of the single-char insert family. Same shapes as
/// `StrInsert` / `StrInsertL` / `StrInsertR`.
///
/// SUCCESS : Returns `true`.
/// FAILURE : Does not return; `LOG_FATAL` aborts the process.
///
/// TAGS: Str, Insert, Char, Must, Abort
///
#define StrMustInsertL(str, lval, idx) VecMustInsertL((str), (lval), (idx))
#define StrMustInsertR(str, rval, idx) VecMustInsertR((str), (rval), (idx))
#define StrMustInsert(str, val, idx)   StrMustInsertL((str), (val), (idx))

///
/// Insert MANY characters into `str` at position `idx`. R-form (copy);
/// the source bytes are read-only / borrowed. Two arities via
/// `OVERLOAD`:
///
///   `StrInsertMany(str, zstr, idx)`            - 3 args; `zstr` is
///                                                NUL-terminated; length
///                                                derived via `ZstrLen`.
///   `StrInsertMany(str, cstr, cstr_len, idx)`  - 4 args; counted view.
///                                                The `(Zstr, size)`
///                                                pair sits adjacent.
///
/// For `Str *` → `Str` use `StrMergeL` / `StrMergeR`.
///
/// No L-form: const source bytes (Zstr / Cstr) cannot carry ownership-
/// transfer semantics. For writeable-source range insert (with L-form
/// ownership transfer) use `StrInsertRangeL`.
///
/// SUCCESS : Returns `true`; bytes copied at `idx`; trailing characters
///           shifted right by the inserted length.
/// FAILURE : Returns `false` on allocation failure. `str` unchanged.
///
/// TAGS: Str, Insert, Range, Zstr, Cstr
///
#define StrInsertMany(...) OVERLOAD(StrInsertMany, __VA_ARGS__)
#define StrInsertMany_3(str, zstr, idx)                                                                            \
    _Generic((zstr), Zstr: VecInsertRangeR((str), (Zstr)(zstr), (idx), ZstrLen((Zstr)(zstr))), char *: VecInsertRangeR((str), (Zstr)(zstr), (idx), ZstrLen((Zstr)(zstr))))
#define StrInsertMany_4(str, cstr, cstr_len, idx)                                                                  \
    _Generic((cstr), Zstr: VecInsertRangeR((str), (Zstr)(cstr), (idx), (cstr_len)), char *: VecInsertRangeR((str), (Zstr)(cstr), (idx), (cstr_len)))

///
/// Aborting variant of `StrInsertMany`. Same shapes; calls `LOG_FATAL`
/// on allocation failure instead of returning `false`.
///
/// SUCCESS : Returns `true`.
/// FAILURE : Does not return; `LOG_FATAL` aborts the process.
///
/// TAGS: Str, Insert, Range, Zstr, Cstr, Must, Abort
///
#define StrMustInsertMany(...) OVERLOAD(StrMustInsertMany, __VA_ARGS__)
#define StrMustInsertMany_3(str, zstr, idx)                                                                        \
    _Generic((zstr), Zstr: VecMustInsertRangeR((str), (Zstr)(zstr), (idx), ZstrLen((Zstr)(zstr))), char *: VecMustInsertRangeR((str), (Zstr)(zstr), (idx), ZstrLen((Zstr)(zstr))))
#define StrMustInsertMany_4(str, cstr, cstr_len, idx)                                                              \
    _Generic((cstr), Zstr: VecMustInsertRangeR((str), (Zstr)(cstr), (idx), (cstr_len)), char *: VecMustInsertRangeR((str), (Zstr)(cstr), (idx), (cstr_len)))

///
/// Fast (order-not-preserving) variant of `StrInsertMany`. Same shapes
/// as `StrInsertMany`; the trailing-character preservation is
/// sacrificed for an O(1) at-position write. Use when the relative
/// order of trailing characters after the insertion is irrelevant.
///
/// SUCCESS : Returns `true`; bytes copied at `idx`; characters at and
///           after `idx` are moved to the new tail (order not preserved).
/// FAILURE : Returns `false` on allocation failure. `str` unchanged.
///
/// TAGS: Str, Insert, Range, Zstr, Cstr, Fast, Unordered
///
#define StrInsertManyFast(...) OVERLOAD(StrInsertManyFast, __VA_ARGS__)
#define StrInsertManyFast_3(str, zstr, idx)                                                                        \
    _Generic((zstr), Zstr: VecInsertRangeFastR((str), (Zstr)(zstr), (idx), ZstrLen((Zstr)(zstr))), char *: VecInsertRangeFastR((str), (Zstr)(zstr), (idx), ZstrLen((Zstr)(zstr))))
#define StrInsertManyFast_4(str, cstr, cstr_len, idx)                                                              \
    _Generic((cstr), Zstr: VecInsertRangeFastR((str), (Zstr)(cstr), (idx), (cstr_len)), char *: VecInsertRangeFastR((str), (Zstr)(cstr), (idx), (cstr_len)))

///
/// Aborting variant of `StrInsertManyFast`.
///
/// SUCCESS : Returns `true`.
/// FAILURE : Does not return; `LOG_FATAL` aborts the process.
///
/// TAGS: Str, Insert, Range, Zstr, Cstr, Fast, Unordered, Must, Abort
///
#define StrMustInsertManyFast(...) OVERLOAD(StrMustInsertManyFast, __VA_ARGS__)
#define StrMustInsertManyFast_3(str, zstr, idx)                                                                    \
    _Generic((zstr), Zstr: VecMustInsertRangeFastR((str), (Zstr)(zstr), (idx), ZstrLen((Zstr)(zstr))), char *: VecMustInsertRangeFastR((str), (Zstr)(zstr), (idx), ZstrLen((Zstr)(zstr))))
#define StrMustInsertManyFast_4(str, cstr, cstr_len, idx)                                                          \
    _Generic((cstr), Zstr: VecMustInsertRangeFastR((str), (Zstr)(cstr), (idx), (cstr_len)), char *: VecMustInsertRangeFastR((str), (Zstr)(cstr), (idx), (cstr_len)))

// ---------------------------------------------------------------------------
// PushBack -- append to the tail.
// ---------------------------------------------------------------------------

///
/// Append a single character to the end of `str`. L-form (ownership-
/// transfer); on success the source `char` lvalue is zeroed.
///
/// SUCCESS : Returns `true`; the character is appended at the tail.
/// FAILURE : Returns `false` on allocation failure. `str` unchanged.
///
/// TAGS: Str, PushBack, Char, LValue
///
#define StrPushBackL(str, lval) VecPushBackL((str), (lval))

///
/// R-form sibling of `StrPushBackL`: appends a single character by
/// copy. The source is not modified. Accepts any value implicitly
/// convertible to `char` (in particular, `int` character literals
/// like `'x'`).
///
/// SUCCESS : Returns `true`; the character is appended at the tail.
/// FAILURE : Returns `false` on allocation failure. `str` unchanged.
///
/// TAGS: Str, PushBack, Char, RValue
///
#define StrPushBackR(str, rval) VecPushBackR((str), (rval))

///
/// Unsuffixed default -- alias for `StrPushBackL`.
///
/// SUCCESS : Returns `true`; the character is appended at the tail; the
///           source `char` lvalue is zeroed-on-take per the L-form invariant.
/// FAILURE : Returns `false` on allocation failure. `str` unchanged.
///
/// TAGS: Str, PushBack, Char
///
#define StrPushBack(str, val) StrPushBackL((str), (val))

///
/// Aborting variants of the single-char PushBack family.
///
/// SUCCESS : Returns `true`.
/// FAILURE : Does not return; `LOG_FATAL` aborts the process.
///
/// TAGS: Str, PushBack, Char, Must, Abort
///
#define StrMustPushBackL(str, lval) VecMustPushBackL((str), (lval))
#define StrMustPushBackR(str, rval) VecMustPushBackR((str), (rval))
#define StrMustPushBack(str, val)   StrMustPushBackL((str), (val))

///
/// Append MANY characters to the end of `str`. R-form (the source bytes
/// are read-only / borrowed). Two arities via `OVERLOAD`:
///
///   `StrPushBackMany(str, zstr)`            - 2 args; `zstr` is
///                                             NUL-terminated.
///   `StrPushBackMany(str, cstr, cstr_len)`  - 3 args; counted view.
///                                             `(Zstr, size)` adjacent.
///
/// No Fast variant: tail append is naturally O(1) amortised; the
/// "Fast" semantics only matter for operations that would otherwise
/// shift elements.
///
/// SUCCESS : Returns `true`; bytes copied at the tail.
/// FAILURE : Returns `false` on allocation failure. `str` unchanged.
///
/// TAGS: Str, PushBack, Range, Zstr, Cstr
///
#define StrPushBackMany(...) OVERLOAD(StrPushBackMany, __VA_ARGS__)
#define StrPushBackMany_2(str, zstr)                                                                               \
    _Generic((zstr), Zstr: VecPushBackArrR((str), (Zstr)(zstr), ZstrLen((Zstr)(zstr))), char *: VecPushBackArrR((str), (Zstr)(zstr), ZstrLen((Zstr)(zstr))))
#define StrPushBackMany_3(str, cstr, cstr_len)                                                                     \
    _Generic((cstr), Zstr: VecPushBackArrR((str), (Zstr)(cstr), (cstr_len)), char *: VecPushBackArrR((str), (Zstr)(cstr), (cstr_len)))

///
/// Aborting variant of `StrPushBackMany`.
///
/// SUCCESS : Returns `true`.
/// FAILURE : Does not return; `LOG_FATAL` aborts the process.
///
/// TAGS: Str, PushBack, Range, Zstr, Cstr, Must, Abort
///
#define StrMustPushBackMany(...) OVERLOAD(StrMustPushBackMany, __VA_ARGS__)
#define StrMustPushBackMany_2(str, zstr)                                                                           \
    _Generic((zstr), Zstr: VecMustPushBackArrR((str), (Zstr)(zstr), ZstrLen((Zstr)(zstr))), char *: VecMustPushBackArrR((str), (Zstr)(zstr), ZstrLen((Zstr)(zstr))))
#define StrMustPushBackMany_3(str, cstr, cstr_len)                                                                 \
    _Generic((cstr), Zstr: VecMustPushBackArrR((str), (Zstr)(cstr), (cstr_len)), char *: VecMustPushBackArrR((str), (Zstr)(cstr), (cstr_len)))

// ---------------------------------------------------------------------------
// PushFront -- prepend at the head.
// ---------------------------------------------------------------------------

///
/// Prepend a single character at the head of `str`. L-form (ownership-
/// transfer); on success the source `char` lvalue is zeroed.
///
/// SUCCESS : Returns `true`; the character is prepended at the head.
/// FAILURE : Returns `false` on allocation failure. `str` unchanged.
///
/// TAGS: Str, PushFront, Char, LValue
///
#define StrPushFrontL(str, lval) VecPushFrontL((str), (lval))

///
/// R-form sibling of `StrPushFrontL`.
///
/// SUCCESS : Returns `true`; the character is prepended at the head.
/// FAILURE : Returns `false` on allocation failure. `str` unchanged.
///
/// TAGS: Str, PushFront, Char, RValue
///
#define StrPushFrontR(str, rval) VecPushFrontR((str), (rval))

///
/// Unsuffixed default -- alias for `StrPushFrontL`.
///
/// SUCCESS : Returns `true`; the character is prepended at the head; the
///           source `char` lvalue is zeroed-on-take per the L-form invariant.
/// FAILURE : Returns `false` on allocation failure. `str` unchanged.
///
/// TAGS: Str, PushFront, Char
///
#define StrPushFront(str, val) StrPushFrontL((str), (val))

///
/// Aborting variants of the single-char PushFront family.
///
/// SUCCESS : Returns `true`.
/// FAILURE : Does not return; `LOG_FATAL` aborts the process.
///
/// TAGS: Str, PushFront, Char, Must, Abort
///
#define StrMustPushFrontL(str, lval) VecMustPushFrontL((str), (lval))
#define StrMustPushFrontR(str, rval) VecMustPushFrontR((str), (rval))
#define StrMustPushFront(str, val)   StrMustPushFrontL((str), (val))

///
/// Prepend MANY characters at the head of `str`. R-form. Two arities:
///
///   `StrPushFrontMany(str, zstr)`            - 2 args; NUL-terminated.
///   `StrPushFrontMany(str, cstr, cstr_len)`  - 3 args; counted view.
///
/// SUCCESS : Returns `true`; bytes copied at the head; existing
///           characters shifted right by the prepended length.
/// FAILURE : Returns `false` on allocation failure. `str` unchanged.
///
/// TAGS: Str, PushFront, Range, Zstr, Cstr
///
#define StrPushFrontMany(...) OVERLOAD(StrPushFrontMany, __VA_ARGS__)
#define StrPushFrontMany_2(str, zstr)                                                                              \
    _Generic((zstr), Zstr: VecPushFrontArrR((str), (Zstr)(zstr), ZstrLen((Zstr)(zstr))), char *: VecPushFrontArrR((str), (Zstr)(zstr), ZstrLen((Zstr)(zstr))))
#define StrPushFrontMany_3(str, cstr, cstr_len)                                                                    \
    _Generic((cstr), Zstr: VecPushFrontArrR((str), (Zstr)(cstr), (cstr_len)), char *: VecPushFrontArrR((str), (Zstr)(cstr), (cstr_len)))

///
/// Aborting variant of `StrPushFrontMany`.
///
/// SUCCESS : Returns `true`.
/// FAILURE : Does not return; `LOG_FATAL` aborts the process.
///
/// TAGS: Str, PushFront, Range, Zstr, Cstr, Must, Abort
///
#define StrMustPushFrontMany(...) OVERLOAD(StrMustPushFrontMany, __VA_ARGS__)
#define StrMustPushFrontMany_2(str, zstr)                                                                          \
    _Generic((zstr), Zstr: VecMustPushFrontArrR((str), (Zstr)(zstr), ZstrLen((Zstr)(zstr))), char *: VecMustPushFrontArrR((str), (Zstr)(zstr), ZstrLen((Zstr)(zstr))))
#define StrMustPushFrontMany_3(str, cstr, cstr_len)                                                                \
    _Generic((cstr), Zstr: VecMustPushFrontArrR((str), (Zstr)(cstr), (cstr_len)), char *: VecMustPushFrontArrR((str), (Zstr)(cstr), (cstr_len)))

///
/// Fast (order-not-preserving) variant of `StrPushFrontMany`. The
/// existing head characters are moved to the tail; the new bytes
/// occupy the head. O(1) when the prepended length is small.
///
/// SUCCESS : Returns `true`; bytes copied at the head; previous
///           head characters relocated to the tail (order not
///           preserved).
/// FAILURE : Returns `false` on allocation failure. `str` unchanged.
///
/// TAGS: Str, PushFront, Range, Zstr, Cstr, Fast, Unordered
///
#define StrPushFrontManyFast(...) OVERLOAD(StrPushFrontManyFast, __VA_ARGS__)
#define StrPushFrontManyFast_2(str, zstr)                                                                          \
    _Generic((zstr), Zstr: VecPushFrontArrFastR((str), (Zstr)(zstr), ZstrLen((Zstr)(zstr))), char *: VecPushFrontArrFastR((str), (Zstr)(zstr), ZstrLen((Zstr)(zstr))))
#define StrPushFrontManyFast_3(str, cstr, cstr_len)                                                                \
    _Generic((cstr), Zstr: VecPushFrontArrFastR((str), (Zstr)(cstr), (cstr_len)), char *: VecPushFrontArrFastR((str), (Zstr)(cstr), (cstr_len)))

///
/// Aborting variant of `StrPushFrontManyFast`.
///
/// SUCCESS : Returns `true`.
/// FAILURE : Does not return; `LOG_FATAL` aborts the process.
///
/// TAGS: Str, PushFront, Range, Zstr, Cstr, Fast, Unordered, Must, Abort
///
#define StrMustPushFrontManyFast(...) OVERLOAD(StrMustPushFrontManyFast, __VA_ARGS__)
#define StrMustPushFrontManyFast_2(str, zstr)                                                                      \
    _Generic((zstr), Zstr: VecMustPushFrontArrFastR((str), (Zstr)(zstr), ZstrLen((Zstr)(zstr))), char *: VecMustPushFrontArrFastR((str), (Zstr)(zstr), ZstrLen((Zstr)(zstr))))
#define StrMustPushFrontManyFast_3(str, cstr, cstr_len)                                                            \
    _Generic((cstr), Zstr: VecMustPushFrontArrFastR((str), (Zstr)(cstr), (cstr_len)), char *: VecMustPushFrontArrFastR((str), (Zstr)(cstr), (cstr_len)))

///
/// Merge `str2` into the end of `str` with L-value (ownership-transfer)
/// semantics. On success `str2`'s storage is taken into `str` and `str2`
/// is left zeroed.
///
/// str[in,out]  : Destination Str.
/// str2[in,out] : Source Str. Zeroed on success.
///
/// SUCCESS : Returns `true`; `str` grew by `str2->length`; `str2` is zeroed.
/// FAILURE : Returns `false` on allocation failure; `str2` was already
///           zeroed-on-take per the L-form invariant, `str` is unchanged.
///
/// TAGS: Str, Merge, LValue, Ownership
///
#define StrMergeL(str, str2) VecMergeL((str), (str2))

///
/// Aborting variant of `StrMergeL`. Calls `LOG_FATAL` on allocation
/// failure instead of returning `false`.
///
/// SUCCESS : Returns to the caller; `str2` is empty (ownership taken).
/// FAILURE : Does not return; `LOG_FATAL` aborts the process. `str2`
///           is left in its zero-on-take state per the L-form invariant.
///
/// TAGS: Str, Merge, LValue, Must, Abort
///
#define StrMustMergeL(str, str2) VecMustMergeL((str), (str2))

///
/// Merge a copy of `str2` into the end of `str` with R-value (read-only-
/// source) semantics. The source is never emptied.
///
/// SUCCESS : Returns `true`; `str` grows by `str2->length` characters;
///           `str2` is untouched.
/// FAILURE : Returns `false` on allocation failure. Both strings unchanged.
///
/// TAGS: Str, Merge, RValue
///
#define StrMergeR(str, str2) VecMergeR((str), (str2))

///
/// Aborting variant of `StrMergeR`.
///
/// SUCCESS : Returns to the caller; `str` grew by `str2->length`;
///           `str2` is untouched.
/// FAILURE : Does not return; `LOG_FATAL` aborts the process.
///
/// TAGS: Str, Merge, RValue, Must, Abort
///
#define StrMustMergeR(str, str2) VecMustMergeR((str), (str2))

///
/// Unsuffixed merge -- alias for `StrMergeL` (L-form), per the project
/// convention that an unsuffixed name lands on the L-form. Callers
/// that need to preserve the source must spell out `StrMergeR`.
///
/// SUCCESS : Returns `true`; `str` grew by `str2->length` characters;
///           `str2` is in its zero-on-take state (ownership taken).
/// FAILURE : Returns `false` on allocation failure; `str2` was already
///           zeroed-on-take, `str` is unchanged.
///
/// TAGS: Str, Merge
///
#define StrMerge(str, str2) StrMergeL((str), (str2))

///
/// Aborting variant of `StrMerge`.
///
/// SUCCESS : Returns to the caller; `str2` is empty.
/// FAILURE : Does not return; `LOG_FATAL` aborts the process.
///
/// TAGS: Str, Merge, Must, Abort
///
#define StrMustMerge(str, str2) StrMustMergeL((str), (str2))

//
// Character-specialised aliases for the underlying container operations.
// Use the `Str*` form throughout; the `Vec` machinery is an implementation
// detail.
//

///
/// Insert a single character at `idx` using fast (order-not-preserving)
/// placement: the element previously at `idx` is moved to the new tail
/// before the new character occupies `idx`. L-form: the source `char`
/// lvalue is zeroed on success per the L-form invariant.
///
/// SUCCESS : Returns `true`; the new character occupies `idx`; the
///           character that previously occupied `idx` is at the new tail.
/// FAILURE : Returns `false` on allocation failure. `str` unchanged.
///
/// TAGS: Str, Insert, Char, LValue, Fast, Unordered
///
#define StrInsertFastL(str, lval, idx) VecInsertFastL((str), (lval), (idx))

///
/// Aborting variant of `StrInsertFastL`.
///
/// SUCCESS : Returns to the caller; new character at `idx`.
/// FAILURE : Does not return; `LOG_FATAL` aborts the process.
///
/// TAGS: Str, Insert, Char, LValue, Fast, Must, Abort
///
#define StrMustInsertFastL(str, lval, idx) VecMustInsertFastL((str), (lval), (idx))

///
/// R-form sibling of `StrInsertFastL`: same fast (order-not-preserving)
/// insertion at `idx`, but the source is copied by value (not zeroed).
///
/// SUCCESS : Returns `true`; the new character occupies `idx`; the
///           previous character at `idx` is now at the tail.
/// FAILURE : Returns `false` on allocation failure. `str` unchanged.
///
/// TAGS: Str, Insert, Char, RValue, Fast, Unordered
///
#define StrInsertFastR(str, rval, idx) VecInsertFastR((str), (rval), (idx))

///
/// Aborting variant of `StrInsertFastR`.
///
/// SUCCESS : Returns to the caller.
/// FAILURE : Does not return; `LOG_FATAL` aborts the process.
///
/// TAGS: Str, Insert, Char, RValue, Fast, Must, Abort
///
#define StrMustInsertFastR(str, rval, idx) VecMustInsertFastR((str), (rval), (idx))

///
/// Unsuffixed default fast insert -- alias for `StrInsertFastL`.
///
/// SUCCESS : Returns `true`; new character at `idx`; previous occupant
///           relocated to the tail.
/// FAILURE : Returns `false` on allocation failure. `str` unchanged.
///
/// TAGS: Str, Insert, Char, Fast, Unordered
///
#define StrInsertFast(str, lval, idx) StrInsertFastL((str), (lval), (idx))

///
/// Aborting variant of `StrInsertFast`.
///
/// SUCCESS : Returns to the caller.
/// FAILURE : Does not return; `LOG_FATAL` aborts the process.
///
/// TAGS: Str, Insert, Char, Fast, Must, Abort
///
#define StrMustInsertFast(str, lval, idx) StrMustInsertFastL((str), (lval), (idx))

///
/// Insert a contiguous range of characters at `idx`, preserving the
/// order of trailing characters. L-form (ownership-transfer); source
/// range zeroed-on-take per the L-form invariant.
///
/// For Zstr/Cstr (const) source ranges use `StrInsertMany`.
///
/// SUCCESS : Returns `true`; `str` grew by `count` characters at `idx`;
///           trailing characters shifted right by `count`; source range
///           zeroed-on-take per the L-form invariant.
/// FAILURE : Returns `false` on allocation failure; source range was
///           already zeroed-on-take, `str` is unchanged.
///
/// TAGS: Str, Insert, Range, LValue
///
#define StrInsertRangeL(str, varr, idx, count) VecInsertRangeL((str), (varr), (idx), (count))

///
/// Aborting variant of `StrInsertRangeL`.
///
/// SUCCESS : Returns to the caller; `str` grew by `count`; source range
///           zeroed-on-take.
/// FAILURE : Does not return; `LOG_FATAL` aborts the process.
///
/// TAGS: Str, Insert, Range, LValue, Must, Abort
///
#define StrMustInsertRangeL(str, varr, idx, count) VecMustInsertRangeL((str), (varr), (idx), (count))

///
/// R-form sibling of `StrInsertRangeL`: range insert at `idx` with
/// source bytes copied (not zeroed).
///
/// SUCCESS : Returns `true`; `str` grew by `count`; trailing characters
///           shifted right by `count`. Source untouched.
/// FAILURE : Returns `false` on allocation failure. `str` unchanged.
///
/// TAGS: Str, Insert, Range, RValue
///
#define StrInsertRangeR(str, varr, idx, count) VecInsertRangeR((str), (varr), (idx), (count))

///
/// Aborting variant of `StrInsertRangeR`.
///
/// SUCCESS : Returns to the caller.
/// FAILURE : Does not return; `LOG_FATAL` aborts the process.
///
/// TAGS: Str, Insert, Range, RValue, Must, Abort
///
#define StrMustInsertRangeR(str, varr, idx, count) VecMustInsertRangeR((str), (varr), (idx), (count))

///
/// Unsuffixed default range insert -- alias for `StrInsertRangeL`.
///
/// SUCCESS : Returns `true`; `str` grew by `count`; trailing characters
///           shifted right; source zeroed-on-take.
/// FAILURE : Returns `false` on allocation failure. `str` and source
///           unchanged.
///
/// TAGS: Str, Insert, Range
///
#define StrInsertRange(str, varr, idx, count) StrInsertRangeL((str), (varr), (idx), (count))

///
/// Aborting variant of `StrInsertRange`.
///
/// SUCCESS : Returns to the caller.
/// FAILURE : Does not return; `LOG_FATAL` aborts the process.
///
/// TAGS: Str, Insert, Range, Must, Abort
///
#define StrMustInsertRange(str, varr, idx, count) StrMustInsertRangeL((str), (varr), (idx), (count))

///
/// Fast (order-not-preserving) range insert at `idx`. L-form. The
/// existing characters at and after `idx` are relocated to the new
/// tail (order not preserved) before the new range occupies `idx`.
/// Source range zeroed-on-take.
///
/// SUCCESS : Returns `true`; new range at `idx`; previous occupants
///           relocated to the new tail (unordered).
/// FAILURE : Returns `false` on allocation failure. `str` and source
///           unchanged.
///
/// TAGS: Str, Insert, Range, LValue, Fast, Unordered
///
#define StrInsertRangeFastL(str, varr, idx, count) VecInsertRangeFastL((str), (varr), (idx), (count))

///
/// Aborting variant of `StrInsertRangeFastL`.
///
/// SUCCESS : Returns to the caller.
/// FAILURE : Does not return; `LOG_FATAL` aborts the process.
///
/// TAGS: Str, Insert, Range, LValue, Fast, Must, Abort
///
#define StrMustInsertRangeFastL(str, varr, idx, count) VecMustInsertRangeFastL((str), (varr), (idx), (count))

///
/// R-form sibling of `StrInsertRangeFastL`: fast unordered range
/// insert with source copied (not zeroed).
///
/// SUCCESS : Returns `true`; new range at `idx`; previous occupants
///           relocated to the new tail.
/// FAILURE : Returns `false` on allocation failure. `str` unchanged.
///
/// TAGS: Str, Insert, Range, RValue, Fast, Unordered
///
#define StrInsertRangeFastR(str, varr, idx, count) VecInsertRangeFastR((str), (varr), (idx), (count))

///
/// Aborting variant of `StrInsertRangeFastR`.
///
/// SUCCESS : Returns to the caller.
/// FAILURE : Does not return; `LOG_FATAL` aborts the process.
///
/// TAGS: Str, Insert, Range, RValue, Fast, Must, Abort
///
#define StrMustInsertRangeFastR(str, varr, idx, count) VecMustInsertRangeFastR((str), (varr), (idx), (count))

///
/// Unsuffixed default fast range insert -- alias for `StrInsertRangeFastL`.
///
/// SUCCESS : Returns `true`; new range at `idx`; previous occupants
///           relocated to the new tail (unordered); source zeroed-on-take.
/// FAILURE : Returns `false` on allocation failure. `str` and source
///           unchanged.
///
/// TAGS: Str, Insert, Range, Fast, Unordered
///
#define StrInsertRangeFast(str, varr, idx, count) StrInsertRangeFastL((str), (varr), (idx), (count))

///
/// Aborting variant of `StrInsertRangeFast`.
///
/// SUCCESS : Returns to the caller.
/// FAILURE : Does not return; `LOG_FATAL` aborts the process.
///
/// TAGS: Str, Insert, Range, Fast, Must, Abort
///
#define StrMustInsertRangeFast(str, varr, idx, count) StrMustInsertRangeFastL((str), (varr), (idx), (count))

///
/// Append a contiguous range of characters to the tail of `str`. L-form
/// (ownership-transfer); source range zeroed-on-take.
///
/// SUCCESS : Returns `true`; `str` grew by `count` at the tail; source
///           range zeroed-on-take per the L-form invariant.
/// FAILURE : Returns `false` on allocation failure. `str` and source
///           unchanged.
///
/// TAGS: Str, PushBack, Range, LValue
///
#define StrPushBackArrL(str, arr, count) VecPushBackArrL((str), (arr), (count))

///
/// Aborting variant of `StrPushBackArrL`.
///
/// SUCCESS : Returns to the caller; source range zeroed-on-take.
/// FAILURE : Does not return; `LOG_FATAL` aborts the process.
///
/// TAGS: Str, PushBack, Range, LValue, Must, Abort
///
#define StrMustPushBackArrL(str, arr, count) VecMustPushBackArrL((str), (arr), (count))

///
/// R-form sibling: append range with source copied (not zeroed).
///
/// SUCCESS : Returns `true`; `str` grew by `count` at the tail.
/// FAILURE : Returns `false` on allocation failure. `str` unchanged.
///
/// TAGS: Str, PushBack, Range, RValue
///
#define StrPushBackArrR(str, arr, count) VecPushBackArrR((str), (arr), (count))

///
/// Aborting variant of `StrPushBackArrR`.
///
/// SUCCESS : Returns to the caller.
/// FAILURE : Does not return; `LOG_FATAL` aborts the process.
///
/// TAGS: Str, PushBack, Range, RValue, Must, Abort
///
#define StrMustPushBackArrR(str, arr, count) VecMustPushBackArrR((str), (arr), (count))

///
/// Unsuffixed default range append -- alias for `StrPushBackArrL`.
///
/// SUCCESS : Returns `true`; `str` grew by `count` at the tail; source
///           zeroed-on-take.
/// FAILURE : Returns `false` on allocation failure. `str` and source
///           unchanged.
///
/// TAGS: Str, PushBack, Range
///
#define StrPushBackArr(str, arr, count) StrPushBackArrL((str), (arr), (count))

///
/// Aborting variant of `StrPushBackArr`.
///
/// SUCCESS : Returns to the caller.
/// FAILURE : Does not return; `LOG_FATAL` aborts the process.
///
/// TAGS: Str, PushBack, Range, Must, Abort
///
#define StrMustPushBackArr(str, arr, count) StrMustPushBackArrL((str), (arr), (count))

///
/// Prepend a contiguous range of characters at the head of `str`. L-form
/// (ownership-transfer); source range zeroed-on-take.
///
/// SUCCESS : Returns `true`; `str` grew by `count` at the head; existing
///           characters shifted right; source range zeroed-on-take.
/// FAILURE : Returns `false` on allocation failure. `str` and source
///           unchanged.
///
/// TAGS: Str, PushFront, Range, LValue
///
#define StrPushFrontArrL(str, arr, count) VecPushFrontArrL((str), (arr), (count))

///
/// Aborting variant of `StrPushFrontArrL`.
///
/// SUCCESS : Returns to the caller; source range zeroed-on-take.
/// FAILURE : Does not return; `LOG_FATAL` aborts the process.
///
/// TAGS: Str, PushFront, Range, LValue, Must, Abort
///
#define StrMustPushFrontArrL(str, arr, count) VecMustPushFrontArrL((str), (arr), (count))

///
/// R-form sibling: prepend range with source copied (not zeroed).
///
/// SUCCESS : Returns `true`; `str` grew by `count` at the head; existing
///           characters shifted right.
/// FAILURE : Returns `false` on allocation failure. `str` unchanged.
///
/// TAGS: Str, PushFront, Range, RValue
///
#define StrPushFrontArrR(str, arr, count) VecPushFrontArrR((str), (arr), (count))

///
/// Aborting variant of `StrPushFrontArrR`.
///
/// SUCCESS : Returns to the caller.
/// FAILURE : Does not return; `LOG_FATAL` aborts the process.
///
/// TAGS: Str, PushFront, Range, RValue, Must, Abort
///
#define StrMustPushFrontArrR(str, arr, count) VecMustPushFrontArrR((str), (arr), (count))

///
/// Unsuffixed default range prepend -- alias for `StrPushFrontArrL`.
///
/// SUCCESS : Returns `true`; `str` grew by `count` at the head; existing
///           characters shifted right; source zeroed-on-take.
/// FAILURE : Returns `false` on allocation failure. `str` and source
///           unchanged.
///
/// TAGS: Str, PushFront, Range
///
#define StrPushFrontArr(str, arr, count) StrPushFrontArrL((str), (arr), (count))

///
/// Aborting variant of `StrPushFrontArr`.
///
/// SUCCESS : Returns to the caller.
/// FAILURE : Does not return; `LOG_FATAL` aborts the process.
///
/// TAGS: Str, PushFront, Range, Must, Abort
///
#define StrMustPushFrontArr(str, arr, count) StrMustPushFrontArrL((str), (arr), (count))

///
/// Fast (order-not-preserving) range prepend. L-form. The existing head
/// characters are moved to the new tail (order not preserved) before
/// the new range occupies the head. Source range zeroed-on-take.
///
/// SUCCESS : Returns `true`; new range at the head; existing head
///           characters relocated to the tail (unordered); source
///           zeroed-on-take.
/// FAILURE : Returns `false` on allocation failure. `str` and source
///           unchanged.
///
/// TAGS: Str, PushFront, Range, LValue, Fast, Unordered
///
#define StrPushFrontArrFastL(str, arr, count) VecPushFrontArrFastL((str), (arr), (count))

///
/// Aborting variant of `StrPushFrontArrFastL`.
///
/// SUCCESS : Returns to the caller.
/// FAILURE : Does not return; `LOG_FATAL` aborts the process.
///
/// TAGS: Str, PushFront, Range, LValue, Fast, Must, Abort
///
#define StrMustPushFrontArrFastL(str, arr, count) VecMustPushFrontArrFastL((str), (arr), (count))

///
/// R-form sibling: fast unordered range prepend with source copied.
///
/// SUCCESS : Returns `true`; new range at the head; existing head
///           characters relocated to the tail.
/// FAILURE : Returns `false` on allocation failure. `str` unchanged.
///
/// TAGS: Str, PushFront, Range, RValue, Fast, Unordered
///
#define StrPushFrontArrFastR(str, arr, count) VecPushFrontArrFastR((str), (arr), (count))

///
/// Aborting variant of `StrPushFrontArrFastR`.
///
/// SUCCESS : Returns to the caller.
/// FAILURE : Does not return; `LOG_FATAL` aborts the process.
///
/// TAGS: Str, PushFront, Range, RValue, Fast, Must, Abort
///
#define StrMustPushFrontArrFastR(str, arr, count) VecMustPushFrontArrFastR((str), (arr), (count))

///
/// Unsuffixed default fast range prepend -- alias for `StrPushFrontArrFastL`.
///
/// SUCCESS : Returns `true`; new range at the head; existing head
///           characters relocated to the tail (unordered); source
///           zeroed-on-take.
/// FAILURE : Returns `false` on allocation failure. `str` and source
///           unchanged.
///
/// TAGS: Str, PushFront, Range, Fast, Unordered
///
#define StrPushFrontArrFast(str, arr, count) StrPushFrontArrFastL((str), (arr), (count))

///
/// Aborting variant of `StrPushFrontArrFast`.
///
/// SUCCESS : Returns to the caller.
/// FAILURE : Does not return; `LOG_FATAL` aborts the process.
///
/// TAGS: Str, PushFront, Range, Fast, Must, Abort
///
#define StrMustPushFrontArrFast(str, arr, count) StrMustPushFrontArrFastL((str), (arr), (count))

///
/// Reinitialise `strd` as an independent copy of `strs`: same characters in
/// the same order, with its own freshly allocated character buffer, inheriting
/// the source string's alignment and allocator configuration. The destination
/// is deinit'd first, so it must already be a constructed Str.
///
/// strd[out] : Destination Str handle (current contents are deinit'd first).
/// strs[in]  : Source Str handle.
///
/// SUCCESS : Returns `true`; `strd` now holds a deep copy of every character
///           in `strs` and inherits its copy/alignment/allocator configuration.
/// FAILURE : Returns `false` on allocation failure; `strd` is left in a valid
///           but partially-populated state - call StrDeinit before reuse.
///
/// See `VecInitClone` for the full SUCCESS/FAILURE contract.
///
/// TAGS: Str, Clone, Init, DeepCopy
///
#define StrInitClone(strd, strs) VecInitClone((strd), (strs))

///
/// Aborting variant of `StrInitClone`.
///
/// SUCCESS : Returns to the caller; `strd` is a deep copy of `strs`.
/// FAILURE : Does not return; `LOG_FATAL` aborts the process.
///
/// TAGS: Str, Clone, Init, DeepCopy, Must, Abort
///
#define StrMustInitClone(strd, strs) VecMustInitClone((strd), (strs))

#ifdef __cplusplus
}
#endif

#endif // MISRA_STD_CONTAINER_STR_INSERT_H
