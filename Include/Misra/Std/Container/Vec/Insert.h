/// file      : std/container/vec/insert.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Insert items into vector in different ways.

#ifndef MISRA_STD_CONTAINER_VEC_INSERT_H
#define MISRA_STD_CONTAINER_VEC_INSERT_H

#include "Private.h"

#if defined(MISRA_ENFORCE_TYPE_SAFETY) && MISRA_ENFORCE_TYPE_SAFETY
#    define VEC_TYPECHECK_L(v, lval)      ((void)sizeof(char[_Generic(&(lval), VEC_DATATYPE(v) *: 1, default: -1)]))
#    define VEC_TYPECHECK_R(v, rval)      ((void)sizeof((VEC_DATATYPE(v)[]) {(rval)}))
#    define VEC_TYPECHECK_RANGE_L(v, ptr) ((void)sizeof(char[_Generic((ptr), VEC_DATATYPE(v) *: 1, default: -1)]))
#    define VEC_TYPECHECK_RANGE_R(v, ptr)                                                                              \
        ((void)sizeof(char[_Generic((ptr), VEC_DATATYPE(v) *: 1, const VEC_DATATYPE(v) *: 1, default: -1)]))
#else
#    define VEC_TYPECHECK_L(v, lval)      ((void)0)
#    define VEC_TYPECHECK_R(v, rval)      ((void)0)
#    define VEC_TYPECHECK_RANGE_L(v, ptr) ((void)0)
#    define VEC_TYPECHECK_RANGE_R(v, ptr) ((void)0)
#endif

///
/// Insert a single element at the given index, preserving order of trailing
/// elements. L-value form: takes ownership of `lval` on success when the vector
/// has no `copy_init` handler configured (source is zeroed). On failure the
/// source is left untouched.
///
/// v[in,out] : Vector handle.
/// lval[in]  : Addressable element to insert. Must match the vector's element type.
/// idx[in]   : Position in [0, length]. Existing elements at and after this index
///             shift one slot to the right.
///
/// SUCCESS : Returns `true`. The element value of `lval` is written at `idx`,
///           the vector length grows by one, and trailing elements have
///           shifted one slot right. When the vector has no `copy_init`
///           handler, `lval` has been zeroed (moved-from); otherwise `lval`
///           is unchanged.
/// FAILURE : Returns `false` on allocation failure. The vector and `lval`
///           are both unchanged; the caller may retry or propagate the failure.
///
/// USAGE:
///   typedef Vec(int) IntVec;
///   IntVec v = VecInit();
///   int x = 42;
///   if (!VecInsertL(&v, x, 0)) { /* recover */ }
///
/// TAGS: Vec, Insert, LValue, Ownership
///
#define VecInsertL(v, lval, idx)                                                                                       \
    (ValidateVec(v),                                                                                                   \
     VEC_TYPECHECK_L((v), (lval)),                                                                                     \
     vec_insert_one_l(GENERIC_VEC(v), &LVAL((VEC_DATATYPE(v))(lval)), &(lval), sizeof(VEC_DATATYPE(v)), (idx), true))

///
/// Insert a single element at the given index, preserving order of trailing
/// elements. R-value form: the source is treated as a temporary value and is
/// never zeroed.
///
/// v[in,out] : Vector handle.
/// rval[in]  : Value to insert. Must be convertible to the vector's element type.
/// idx[in]   : Position in [0, length].
///
/// SUCCESS : Returns `true`. The value of `rval` is written at `idx`, the
///           vector length grows by one, and trailing elements have shifted
///           one slot right.
/// FAILURE : Returns `false` on allocation failure. The vector is unchanged.
///
/// USAGE:
///   if (!VecInsertR(&v, 42, 0)) { /* recover */ }
///
/// TAGS: Vec, Insert, RValue
///
#define VecInsertR(v, rval, idx)                                                                                       \
    (ValidateVec(v),                                                                                                   \
     VEC_TYPECHECK_R((v), (rval)),                                                                                     \
     vec_insert_one_r(GENERIC_VEC(v), &LVAL((VEC_DATATYPE(v))(rval)), sizeof(VEC_DATATYPE(v)), (idx), true))

///
/// Default insertion alias for `VecInsertL`. Use when ownership transfer of an
/// l-value is the intended behaviour.
///
#define VecInsert(v, lval, idx) VecInsertL((v), (lval), (idx))

///
/// Insert a single element using fast (order-not-preserving) placement: the
/// element previously occupying `idx` is moved to the tail before `lval` is
/// written into the slot. L-value form takes ownership of `lval` on success.
///
/// Use when iteration order is not meaningful (sets, work queues with no
/// ordering requirement, etc.). Faster than `VecInsertL` for non-tail inserts
/// because no range shift is performed.
///
/// v[in,out] : Vector handle.
/// lval[in]  : Addressable element to insert.
/// idx[in]   : Position in [0, length].
///
/// SUCCESS : Returns `true`. The vector length grows by one. The element that
///           previously sat at `idx` now sits at the new tail; `lval`'s value
///           occupies `idx`. When the vector has no `copy_init` handler,
///           `lval` has been zeroed (moved-from); otherwise `lval` is
///           unchanged.
/// FAILURE : Returns `false` on allocation failure. Both vector and `lval`
///           are unchanged.
///
/// TAGS: Vec, Insert, LValue, Fast, Unordered
///
#define VecInsertFastL(v, lval, idx)                                                                                   \
    (ValidateVec(v),                                                                                                   \
     VEC_TYPECHECK_L((v), (lval)),                                                                                     \
     vec_insert_one_l(GENERIC_VEC(v), &LVAL((VEC_DATATYPE(v))(lval)), &(lval), sizeof(VEC_DATATYPE(v)), (idx), false))

///
/// Insert a single element using fast (order-not-preserving) placement.
/// R-value form: the source is treated as a temporary value.
///
/// v[in,out] : Vector handle.
/// rval[in]  : Value to insert.
/// idx[in]   : Position in [0, length].
///
/// SUCCESS : Returns `true`. The vector length grows by one. The element that
///           previously sat at `idx` now sits at the new tail; `rval`'s value
///           occupies `idx`.
/// FAILURE : Returns `false` on allocation failure. The vector is unchanged.
///
/// TAGS: Vec, Insert, RValue, Fast, Unordered
///
#define VecInsertFastR(v, rval, idx)                                                                                   \
    (ValidateVec(v),                                                                                                   \
     VEC_TYPECHECK_R((v), (rval)),                                                                                     \
     vec_insert_one_r(GENERIC_VEC(v), &LVAL((VEC_DATATYPE(v))(rval)), sizeof(VEC_DATATYPE(v)), (idx), false))

///
/// Default fast-insertion alias for `VecInsertFastL`.
///
#define VecInsertFast(v, lval, idx) VecInsertFastL((v), (lval), (idx))

///
/// Insert a contiguous range of elements at the given index, preserving order.
/// L-value form: takes ownership of the source range on success when the vector
/// has no `copy_init` handler (source bytes are zeroed). On failure the source
/// is left untouched.
///
/// v[in,out] : Vector handle.
/// varr[in]  : Pointer to the source array. Must be non-NULL when `count > 0`.
/// idx[in]   : Position in [0, length].
/// count[in] : Number of elements to insert.
///
/// SUCCESS : Returns `true`. The vector length grows by `count`. Source bytes
///           now occupy indices [idx, idx + count); previous elements at and
///           after `idx` have shifted right by `count`. When the vector has
///           no `copy_init` handler, the `count * sizeof(element)` source
///           bytes have been zeroed.
/// FAILURE : Returns `false` on allocation failure. Both vector and source are
///           unchanged.
///
/// USAGE:
///   int items[] = { 1, 2, 3 };
///   if (!VecInsertRangeL(&v, items, 0, 3)) { /* recover */ }
///
/// TAGS: Vec, Insert, Range, LValue
///
#define VecInsertRangeL(v, varr, idx, count)                                                                           \
    (ValidateVec(v),                                                                                                   \
     VEC_TYPECHECK_RANGE_L((v), (varr)),                                                                               \
     vec_insert_range_l(GENERIC_VEC(v), (void *)(varr), sizeof(VEC_DATATYPE(v)), (idx), (count), true))

///
/// Insert a contiguous range of elements at the given index, preserving order.
/// R-value form: the source range is treated as read-only input and is not
/// zeroed.
///
/// v[in,out] : Vector handle.
/// varr[in]  : Pointer to the source array. Must be non-NULL when `count > 0`.
/// idx[in]   : Position in [0, length].
/// count[in] : Number of elements to insert.
///
/// SUCCESS : Returns `true`. The vector length grows by `count`; copies of the
///           source elements now occupy indices [idx, idx + count); previous
///           elements at and after `idx` have shifted right by `count`. The
///           source range is untouched.
/// FAILURE : Returns `false` on allocation failure. The vector is unchanged.
///
/// TAGS: Vec, Insert, Range, RValue
///
#define VecInsertRangeR(v, varr, idx, count)                                                                           \
    (ValidateVec(v),                                                                                                   \
     VEC_TYPECHECK_RANGE_R((v), (varr)),                                                                               \
     vec_insert_range_r(GENERIC_VEC(v), (const void *)(varr), sizeof(VEC_DATATYPE(v)), (idx), (count), true))

///
/// Default range-insert alias for `VecInsertRangeL`.
///
#define VecInsertRange(v, varr, idx, count) VecInsertRangeL((v), (varr), (idx), (count))

///
/// Insert a range using fast (order-not-preserving) placement. L-value form.
/// The original tail-`count` elements of the vector are moved past the inserted
/// region instead of having every element after `idx` shifted; iteration order
/// is no longer meaningful.
///
/// SUCCESS : Returns `true`. The vector length grows by `count`; the inserted
///           elements occupy [idx, idx + count), and the displaced elements
///           sit somewhere in the new tail (no defined relative order). When
///           the vector has no `copy_init` handler, the `count *
///           sizeof(element)` source bytes have been zeroed.
/// FAILURE : Returns `false` on allocation failure. Both vector and source are
///           unchanged.
///
/// TAGS: Vec, Insert, Range, LValue, Fast, Unordered
///
#define VecInsertRangeFastL(v, varr, idx, count)                                                                       \
    (ValidateVec(v),                                                                                                   \
     VEC_TYPECHECK_RANGE_L((v), (varr)),                                                                               \
     vec_insert_range_l(GENERIC_VEC(v), (void *)(varr), sizeof(VEC_DATATYPE(v)), (idx), (count), false))

///
/// Insert a range using fast (order-not-preserving) placement. R-value form.
///
/// SUCCESS : Returns `true`. Same state effects as `VecInsertRangeFastL` minus
///           the source-zeroing step; the source range is left untouched.
/// FAILURE : Returns `false` on allocation failure. The vector is unchanged.
///
/// TAGS: Vec, Insert, Range, RValue, Fast, Unordered
///
#define VecInsertRangeFastR(v, varr, idx, count)                                                                       \
    (ValidateVec(v),                                                                                                   \
     VEC_TYPECHECK_RANGE_R((v), (varr)),                                                                               \
     vec_insert_range_r(GENERIC_VEC(v), (const void *)(varr), sizeof(VEC_DATATYPE(v)), (idx), (count), false))

///
/// Default fast range-insert alias for `VecInsertRangeFastL`.
///
#define VecInsertRangeFast(v, varr, idx, count) VecInsertRangeFastL((v), (varr), (idx), (count))

///
/// Append a contiguous range of elements to the end of the vector.
/// L-value form takes ownership of the source on success when the vector has
/// no `copy_init` handler.
///
/// v[in,out] : Vector handle.
/// arr[in]   : Pointer to the source array.
/// count[in] : Number of elements to append.
///
/// SUCCESS : Returns `true`. The vector length grows by `count`; the appended
///           elements occupy [old_length, new_length). When the vector has no
///           `copy_init` handler, the `count * sizeof(element)` source bytes
///           have been zeroed.
/// FAILURE : Returns `false` on allocation failure. Both vector and source are
///           unchanged.
///
/// TAGS: Vec, PushBack, Range, LValue
///
#define VecPushBackArrL(v, arr, count) VecInsertRangeL((v), (arr), (v)->length, (count))

///
/// Append a contiguous range of elements to the end of the vector. R-value form.
///
/// SUCCESS : Returns `true`. The vector length grows by `count`; copies of the
///           source elements occupy [old_length, new_length). The source range
///           is untouched.
/// FAILURE : Returns `false` on allocation failure. The vector is unchanged.
///
/// TAGS: Vec, PushBack, Range, RValue
///
#define VecPushBackArrR(v, arr, count) VecInsertRangeR((v), (arr), (v)->length, (count))

///
/// Default tail-append alias for `VecPushBackArrL`.
///
#define VecPushBackArr(v, arr, count) VecPushBackArrL((v), (arr), (count))

///
/// Prepend a contiguous range of elements at the front of the vector, preserving
/// order of existing elements. L-value form takes ownership of the source on
/// success when the vector has no `copy_init` handler.
///
/// v[in,out] : Vector handle.
/// arr[in]   : Pointer to the source array.
/// count[in] : Number of elements to prepend.
///
/// SUCCESS : Returns `true`. The vector length grows by `count`; the prepended
///           elements occupy [0, count); all previous elements have shifted
///           right by `count`. When the vector has no `copy_init` handler,
///           the source bytes have been zeroed.
/// FAILURE : Returns `false` on allocation failure. Both vector and source are
///           unchanged.
///
/// TAGS: Vec, PushFront, Range, LValue
///
#define VecPushFrontArrL(v, arr, count) VecInsertRangeL((v), (arr), 0, (count))

///
/// Prepend a contiguous range at the front of the vector. R-value form.
///
/// SUCCESS : Returns `true`. The vector length grows by `count`; copies of the
///           source elements occupy [0, count); all previous elements have
///           shifted right by `count`. The source range is untouched.
/// FAILURE : Returns `false` on allocation failure. The vector is unchanged.
///
/// TAGS: Vec, PushFront, Range, RValue
///
#define VecPushFrontArrR(v, arr, count) VecInsertRangeR((v), (arr), 0, (count))

///
/// Default front-prepend alias for `VecPushFrontArrL`.
///
#define VecPushFrontArr(v, arr, count) VecPushFrontArrL((v), (arr), (count))

///
/// Prepend a range at the front using fast (order-not-preserving) placement.
/// L-value form.
///
/// SUCCESS : Returns `true`. The vector length grows by `count`; the prepended
///           elements occupy [0, count); previously-front elements are now
///           somewhere in the tail (no defined relative order). When the
///           vector has no `copy_init` handler, the source bytes have been
///           zeroed.
/// FAILURE : Returns `false` on allocation failure. Both vector and source are
///           unchanged.
///
/// TAGS: Vec, PushFront, Range, LValue, Fast, Unordered
///
#define VecPushFrontArrFastL(v, arr, count) VecInsertRangeFastL((v), (arr), 0, (count))

///
/// Prepend a range at the front using fast (order-not-preserving) placement.
/// R-value form.
///
/// SUCCESS : Returns `true`. Same state effects as `VecPushFrontArrFastL`
///           minus the source-zeroing step; the source range is left
///           untouched.
/// FAILURE : Returns `false` on allocation failure. The vector is unchanged.
///
/// TAGS: Vec, PushFront, Range, RValue, Fast, Unordered
///
#define VecPushFrontArrFastR(v, arr, count) VecInsertRangeFastR((v), (arr), 0, (count))

///
/// Default fast front-prepend alias for `VecPushFrontArrFastL`.
///
#define VecPushFrontArrFast(v, arr, count) VecPushFrontArrFastL((v), (arr), (count))

///
/// Append all elements of `v2` to the end of `v`.
/// L-value form: when the destination has no `copy_init` handler, ownership of
/// `v2`'s storage transfers and `v2` is left empty on success. With a deep-copy
/// handler, `v2` is unchanged.
///
/// v[in,out]  : Destination vector.
/// v2[in,out] : Source vector. May be emptied on success.
///
/// SUCCESS : Returns `true`. `v->length` grows by the previous `v2->length`;
///           the appended elements occupy the new tail of `v`. When `v` has
///           no `copy_init` handler, `v2->data` ownership has transferred
///           into `v` and `v2` is left empty (length 0, capacity 0, data
///           freed and pointer reset). With a deep-copy handler, `v2` is
///           unchanged.
/// FAILURE : Returns `false` on allocation failure. Both `v` and `v2` are
///           unchanged.
///
/// TAGS: Vec, Merge, LValue, Ownership
///
#define VecMergeL(v, v2)                                                                                               \
    (ValidateVec(v), ValidateVec(v2), vec_merge_l(GENERIC_VEC(v), GENERIC_VEC(v2), sizeof(VEC_DATATYPE(v))))

///
/// Append a copy of all elements of `v2` to the end of `v`. R-value form: the
/// source vector is never emptied; its contents are read-only.
///
/// v[in,out] : Destination vector.
/// v2[in]    : Source vector.
///
/// SUCCESS : Returns `true`. `v->length` grows by `v2->length`; copies of
///           every `v2` element occupy the new tail of `v`. `v2` is
///           untouched.
/// FAILURE : Returns `false` on allocation failure. `v` is unchanged.
///
/// TAGS: Vec, Merge, RValue
///
#define VecMergeR(v, v2)                                                                                               \
    (ValidateVec(v), ValidateVec(v2), vec_merge_r(GENERIC_VEC(v), GENERIC_VEC(v2), sizeof(VEC_DATATYPE(v))))

///
/// Default merge alias for `VecMergeL`.
///
#define VecMerge(v, v2) VecMergeL((v), (v2))

///
/// Append a single element to the end of the vector. L-value ownership form.
///
/// SUCCESS : Returns `true`. The vector length grows by one; `val` occupies
///           the new tail. When the vector has no `copy_init` handler, `val`
///           has been zeroed (moved-from); otherwise unchanged.
/// FAILURE : Returns `false` on allocation failure. Both vector and `val` are
///           unchanged.
///
/// TAGS: Vec, PushBack, LValue
///
#define VecPushBackL(v, val) VecInsertL((v), (val), (v)->length)

///
/// Append a single element to the end of the vector. R-value form.
///
/// SUCCESS : Returns `true`. The vector length grows by one; the value of
///           `val` occupies the new tail.
/// FAILURE : Returns `false` on allocation failure. The vector is unchanged.
///
/// TAGS: Vec, PushBack, RValue
///
#define VecPushBackR(v, val) VecInsertR((v), (val), (v)->length)

///
/// Default tail-push alias for `VecPushBackL`.
///
#define VecPushBack(v, val) VecInsert((v), (val), (v)->length)

///
/// Prepend a single element at the front of the vector. L-value ownership form.
///
/// SUCCESS : Returns `true`. The vector length grows by one; `val` occupies
///           index 0; previous elements have shifted right by one. When the
///           vector has no `copy_init` handler, `val` has been zeroed
///           (moved-from); otherwise unchanged.
/// FAILURE : Returns `false` on allocation failure. Both vector and `val` are
///           unchanged.
///
/// TAGS: Vec, PushFront, LValue
///
#define VecPushFrontL(v, val) VecInsertL((v), (val), 0)

///
/// Prepend a single element at the front of the vector. R-value form.
///
/// SUCCESS : Returns `true`. The vector length grows by one; the value of
///           `val` occupies index 0; previous elements have shifted right by
///           one.
/// FAILURE : Returns `false` on allocation failure. The vector is unchanged.
///
/// TAGS: Vec, PushFront, RValue
///
#define VecPushFrontR(v, val) VecInsertR((v), (val), 0)

///
/// Default front-push alias for `VecPushFrontL`.
///
#define VecPushFront(v, val) VecPushFrontL((v), (val))

///
/// Reinitialize `vd` as a deep clone of `vs`.
/// Any current contents of `vd` are first deinitialized. The destination adopts
/// `vs`'s `copy_init` / `copy_deinit` / alignment / allocator configuration,
/// then all elements are deep-copied.
///
/// vd[out] : Destination vector. Must be initialized; its current contents are
///           released before cloning.
/// vs[in]  : Source vector.
///
/// SUCCESS : Returns `true`. `vd` now holds a deep copy of every element of
///           `vs`, has the same length, and carries `vs`'s `copy_init` /
///           `copy_deinit` / alignment / allocator configuration. The prior
///           contents of `vd` were released before the clone began.
/// FAILURE : Returns `false` on allocation failure during the clone. `vd` is
///           left in a valid but partially-populated state (the prior
///           contents are gone). Callers should treat `vd` as opaque on
///           failure and call `VecDeinit(vd)` before reuse.
///
/// TAGS: Vec, Clone, Init, DeepCopy
///
#define VecInitClone(vd, vs)                                                                                           \
    (ValidateVec(vd),                                                                                                  \
     ValidateVec(vs),                                                                                                  \
     VecDeinit(vd),                                                                                                    \
     *(vd) = (TYPE_OF(*(vd)))                                                                                          \
         VEC_INIT_ALIGNED_WITH_DEEP_COPY_VALUE((vs)->copy_init, (vs)->copy_deinit, (vs)->alignment, (vs)->allocator),  \
     clone_vec(GENERIC_VEC(vd), GENERIC_VEC(vs), sizeof(VEC_DATATYPE(vd))))

///
/// Aborting (`Must*`) variants of every fallible insertion macro above.
///
/// Each `VecMustXxx(...)` is the statement-style do-while wrapper around the
/// matching `VecXxx(...)` expression: it calls the underlying fallible form
/// and triggers `LOG_FATAL(...)` if the call returns `false`. Use these at
/// API boundaries where allocation failure is not a recoverable condition
/// for the caller. Otherwise prefer the propagating forms.
///
/// SUCCESS : Returns to the caller.
/// FAILURE : Does not return - aborts via `LOG_FATAL` / `SysAbort`.
///
/// TAGS: Vec, Insert, Must, Abort
///
#define VecMustInsertL(v, lval, idx)                                                                                   \
    do {                                                                                                               \
        if (!VecInsertL((v), (lval), (idx))) {                                                                         \
            LOG_FATAL("VecMustInsertL failed");                                                                        \
        }                                                                                                              \
    } while (0)
#define VecMustInsertR(v, rval, idx)                                                                                   \
    do {                                                                                                               \
        if (!VecInsertR((v), (rval), (idx))) {                                                                         \
            LOG_FATAL("VecMustInsertR failed");                                                                        \
        }                                                                                                              \
    } while (0)
#define VecMustInsert(v, lval, idx)                                                                                    \
    do {                                                                                                               \
        if (!VecInsert((v), (lval), (idx))) {                                                                          \
            LOG_FATAL("VecMustInsert failed");                                                                         \
        }                                                                                                              \
    } while (0)

#define VecMustInsertFastL(v, lval, idx)                                                                               \
    do {                                                                                                               \
        if (!VecInsertFastL((v), (lval), (idx))) {                                                                     \
            LOG_FATAL("VecMustInsertFastL failed");                                                                    \
        }                                                                                                              \
    } while (0)
#define VecMustInsertFastR(v, rval, idx)                                                                               \
    do {                                                                                                               \
        if (!VecInsertFastR((v), (rval), (idx))) {                                                                     \
            LOG_FATAL("VecMustInsertFastR failed");                                                                    \
        }                                                                                                              \
    } while (0)
#define VecMustInsertFast(v, lval, idx)                                                                                \
    do {                                                                                                               \
        if (!VecInsertFast((v), (lval), (idx))) {                                                                      \
            LOG_FATAL("VecMustInsertFast failed");                                                                     \
        }                                                                                                              \
    } while (0)

#define VecMustInsertRangeL(v, varr, idx, count)                                                                       \
    do {                                                                                                               \
        if (!VecInsertRangeL((v), (varr), (idx), (count))) {                                                           \
            LOG_FATAL("VecMustInsertRangeL failed");                                                                   \
        }                                                                                                              \
    } while (0)
#define VecMustInsertRangeR(v, varr, idx, count)                                                                       \
    do {                                                                                                               \
        if (!VecInsertRangeR((v), (varr), (idx), (count))) {                                                           \
            LOG_FATAL("VecMustInsertRangeR failed");                                                                   \
        }                                                                                                              \
    } while (0)
#define VecMustInsertRange(v, varr, idx, count)                                                                        \
    do {                                                                                                               \
        if (!VecInsertRange((v), (varr), (idx), (count))) {                                                            \
            LOG_FATAL("VecMustInsertRange failed");                                                                    \
        }                                                                                                              \
    } while (0)

#define VecMustInsertRangeFastL(v, varr, idx, count)                                                                   \
    do {                                                                                                               \
        if (!VecInsertRangeFastL((v), (varr), (idx), (count))) {                                                       \
            LOG_FATAL("VecMustInsertRangeFastL failed");                                                               \
        }                                                                                                              \
    } while (0)
#define VecMustInsertRangeFastR(v, varr, idx, count)                                                                   \
    do {                                                                                                               \
        if (!VecInsertRangeFastR((v), (varr), (idx), (count))) {                                                       \
            LOG_FATAL("VecMustInsertRangeFastR failed");                                                               \
        }                                                                                                              \
    } while (0)
#define VecMustInsertRangeFast(v, varr, idx, count)                                                                    \
    do {                                                                                                               \
        if (!VecInsertRangeFast((v), (varr), (idx), (count))) {                                                        \
            LOG_FATAL("VecMustInsertRangeFast failed");                                                                \
        }                                                                                                              \
    } while (0)

#define VecMustPushBackArrL(v, arr, count)                                                                             \
    do {                                                                                                               \
        if (!VecPushBackArrL((v), (arr), (count))) {                                                                   \
            LOG_FATAL("VecMustPushBackArrL failed");                                                                   \
        }                                                                                                              \
    } while (0)
#define VecMustPushBackArrR(v, arr, count)                                                                             \
    do {                                                                                                               \
        if (!VecPushBackArrR((v), (arr), (count))) {                                                                   \
            LOG_FATAL("VecMustPushBackArrR failed");                                                                   \
        }                                                                                                              \
    } while (0)
#define VecMustPushBackArr(v, arr, count)                                                                              \
    do {                                                                                                               \
        if (!VecPushBackArr((v), (arr), (count))) {                                                                    \
            LOG_FATAL("VecMustPushBackArr failed");                                                                    \
        }                                                                                                              \
    } while (0)

#define VecMustPushFrontArrL(v, arr, count)                                                                            \
    do {                                                                                                               \
        if (!VecPushFrontArrL((v), (arr), (count))) {                                                                  \
            LOG_FATAL("VecMustPushFrontArrL failed");                                                                  \
        }                                                                                                              \
    } while (0)
#define VecMustPushFrontArrR(v, arr, count)                                                                            \
    do {                                                                                                               \
        if (!VecPushFrontArrR((v), (arr), (count))) {                                                                  \
            LOG_FATAL("VecMustPushFrontArrR failed");                                                                  \
        }                                                                                                              \
    } while (0)
#define VecMustPushFrontArr(v, arr, count)                                                                             \
    do {                                                                                                               \
        if (!VecPushFrontArr((v), (arr), (count))) {                                                                   \
            LOG_FATAL("VecMustPushFrontArr failed");                                                                   \
        }                                                                                                              \
    } while (0)

#define VecMustPushFrontArrFastL(v, arr, count)                                                                        \
    do {                                                                                                               \
        if (!VecPushFrontArrFastL((v), (arr), (count))) {                                                              \
            LOG_FATAL("VecMustPushFrontArrFastL failed");                                                              \
        }                                                                                                              \
    } while (0)
#define VecMustPushFrontArrFastR(v, arr, count)                                                                        \
    do {                                                                                                               \
        if (!VecPushFrontArrFastR((v), (arr), (count))) {                                                              \
            LOG_FATAL("VecMustPushFrontArrFastR failed");                                                              \
        }                                                                                                              \
    } while (0)
#define VecMustPushFrontArrFast(v, arr, count)                                                                         \
    do {                                                                                                               \
        if (!VecPushFrontArrFast((v), (arr), (count))) {                                                               \
            LOG_FATAL("VecMustPushFrontArrFast failed");                                                               \
        }                                                                                                              \
    } while (0)

#define VecMustMergeL(v, v2)                                                                                           \
    do {                                                                                                               \
        if (!VecMergeL((v), (v2))) {                                                                                   \
            LOG_FATAL("VecMustMergeL failed");                                                                         \
        }                                                                                                              \
    } while (0)
#define VecMustMergeR(v, v2)                                                                                           \
    do {                                                                                                               \
        if (!VecMergeR((v), (v2))) {                                                                                   \
            LOG_FATAL("VecMustMergeR failed");                                                                         \
        }                                                                                                              \
    } while (0)
#define VecMustMerge(v, v2)                                                                                            \
    do {                                                                                                               \
        if (!VecMerge((v), (v2))) {                                                                                    \
            LOG_FATAL("VecMustMerge failed");                                                                          \
        }                                                                                                              \
    } while (0)

#define VecMustPushBackL(v, val)                                                                                       \
    do {                                                                                                               \
        if (!VecPushBackL((v), (val))) {                                                                               \
            LOG_FATAL("VecMustPushBackL failed");                                                                      \
        }                                                                                                              \
    } while (0)
#define VecMustPushBackR(v, val)                                                                                       \
    do {                                                                                                               \
        if (!VecPushBackR((v), (val))) {                                                                               \
            LOG_FATAL("VecMustPushBackR failed");                                                                      \
        }                                                                                                              \
    } while (0)
#define VecMustPushBack(v, val)                                                                                        \
    do {                                                                                                               \
        if (!VecPushBack((v), (val))) {                                                                                \
            LOG_FATAL("VecMustPushBack failed");                                                                       \
        }                                                                                                              \
    } while (0)

#define VecMustPushFrontL(v, val)                                                                                      \
    do {                                                                                                               \
        if (!VecPushFrontL((v), (val))) {                                                                              \
            LOG_FATAL("VecMustPushFrontL failed");                                                                     \
        }                                                                                                              \
    } while (0)
#define VecMustPushFrontR(v, val)                                                                                      \
    do {                                                                                                               \
        if (!VecPushFrontR((v), (val))) {                                                                              \
            LOG_FATAL("VecMustPushFrontR failed");                                                                     \
        }                                                                                                              \
    } while (0)
#define VecMustPushFront(v, val)                                                                                       \
    do {                                                                                                               \
        if (!VecPushFront((v), (val))) {                                                                               \
            LOG_FATAL("VecMustPushFront failed");                                                                      \
        }                                                                                                              \
    } while (0)

#define VecMustInitClone(vd, vs)                                                                                       \
    do {                                                                                                               \
        if (!VecInitClone((vd), (vs))) {                                                                               \
            LOG_FATAL("VecMustInitClone failed");                                                                      \
        }                                                                                                              \
    } while (0)

#endif // MISRA_STD_CONTAINER_VEC_INSERT_H
