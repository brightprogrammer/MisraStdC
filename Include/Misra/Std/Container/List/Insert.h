/// file      : Insert.h
/// author    : Generated following Misra project patterns
/// This is free and unencumbered software released into the public domain.
///
/// List insertion helpers.
///

#ifndef MISRA_STD_CONTAINER_LIST_INSERT_H
#define MISRA_STD_CONTAINER_LIST_INSERT_H

#include "Private.h"


///
/// Insert a single element at the given index in the list, preserving order.
/// L-value form: takes ownership of `lval` on success when the list has no
/// `copy_init` handler (source is zeroed). On failure the source is left
/// untouched.
///
/// l[in,out] : List handle.
/// lval[in]  : Addressable element to insert. Must match the list's element type.
/// idx[in]   : Position in [0, length].
///
/// SUCCESS : Returns `true`. A new node holding `lval`'s payload is linked at
///           position `idx`; the list length grows by one. When the list has
///           no `copy_init` handler, `lval` has been zeroed (moved-from);
///           otherwise `lval` is unchanged.
/// FAILURE : Returns `false` on allocation failure (either the node header
///           or the payload buffer). The list and `lval` are both unchanged.
///
/// TAGS: List, Insert, LValue, Ownership
///
#define ListInsertL(l, lval, idx)                                                                                      \
    (ValidateList(l),                                                                                                  \
     CHECK_TYPE_EQUIVALENCE(TYPE_OF(lval), LIST_DATA_TYPE(l)),                                                         \
     list_insert_one_l(GENERIC_LIST(l), &LVAL_AS(LIST_DATA_TYPE(l), lval), &(lval), sizeof(LIST_DATA_TYPE(l)), (idx)))

///
/// Insert a single element at the given index. R-value form: source is treated
/// as a temporary value and is never zeroed.
///
/// l[in,out] : List handle.
/// rval[in]  : Value to insert.
/// idx[in]   : Position in [0, length].
///
/// SUCCESS : Returns `true`. A new node holding a copy of `rval` is linked at
///           position `idx`; the list length grows by one. The source
///           expression is untouched.
/// FAILURE : Returns `false` on allocation failure. The list is unchanged.
///
/// TAGS: List, Insert, RValue
///
#define ListInsertR(l, rval, idx)                                                                                      \
    (ValidateList(l),                                                                                                  \
     CHECK_TYPE_CONVERTIBLE(LIST_DATA_TYPE(l), rval),                                                                  \
     list_insert_one_r(GENERIC_LIST(l), &LVAL_AS(LIST_DATA_TYPE(l), rval), sizeof(LIST_DATA_TYPE(l)), (idx)))

///
/// Default insertion alias for `ListInsertL`.
///
#define ListInsert(l, lval, idx) ListInsertL((l), (lval), (idx))

///
/// Prepend an element at the head of the list. L-value ownership form.
///
/// SUCCESS : Returns `true`. A new node holding `lval`'s payload is linked
///           as the new head; the list length grows by one. When the list
///           has no `copy_init` handler, `lval` has been zeroed.
/// FAILURE : Returns `false` on allocation failure. The list and `lval`
///           are unchanged.
///
/// TAGS: List, PushFront, LValue
///
#define ListPushFrontL(l, lval) ListInsertL((l), (lval), 0)

///
/// Prepend an element at the head of the list. R-value form.
///
/// SUCCESS : Returns `true`. A new node holding a copy of `rval` is linked
///           as the new head; the list length grows by one.
/// FAILURE : Returns `false` on allocation failure. The list is unchanged.
///
/// TAGS: List, PushFront, RValue
///
#define ListPushFrontR(l, rval) ListInsertR((l), (rval), 0)

///
/// Default head-push alias for `ListPushFrontL`.
///
#define ListPushFront(l, lval) ListPushFrontL((l), (lval))

///
/// Append an element at the tail of the list. L-value ownership form.
///
/// SUCCESS : Returns `true`. A new node holding `lval`'s payload is linked
///           as the new tail; the list length grows by one. When the list
///           has no `copy_init` handler, `lval` has been zeroed.
/// FAILURE : Returns `false` on allocation failure. The list and `lval`
///           are unchanged.
///
/// TAGS: List, PushBack, LValue
///
#define ListPushBackL(l, lval) ListInsertL((l), (lval), (l)->length)

///
/// Append an element at the tail of the list. R-value form.
///
/// SUCCESS : Returns `true`. A new node holding a copy of `rval` is linked
///           as the new tail; the list length grows by one.
/// FAILURE : Returns `false` on allocation failure. The list is unchanged.
///
/// TAGS: List, PushBack, RValue
///
#define ListPushBackR(l, rval) ListInsertR((l), (rval), (l)->length)

///
/// Default tail-push alias for `ListPushBackL`.
///
#define ListPushBack(l, lval) ListPushBackL((l), (lval))

///
/// Append a contiguous range of elements to the end of the list.
/// L-value form: takes ownership of the source range on success when the list
/// has no `copy_init` handler.
///
/// l[in,out] : List handle.
/// arr[in]   : Pointer to source array. Must be non-NULL when `count > 0`.
/// count[in] : Number of elements to append.
///
/// SUCCESS : Returns `true`. `count` new nodes are linked at the tail of the
///           list; list length grows by `count`. When the list has no
///           `copy_init` handler, the `count * sizeof(element)` source bytes
///           have been zeroed. If a node allocation fails partway through,
///           the already-inserted nodes stay linked (caller may treat the
///           operation as partially complete).
/// FAILURE : Returns `false` on allocation failure during the first node
///           allocation. The list and source are unchanged.
///
/// TAGS: List, PushBack, Range, LValue
///
#define ListPushArrL(l, arr, count)                                                                                    \
    (ValidateList(l),                                                                                                  \
     CHECK_TYPE_EQUIVALENCE(TYPE_OF(*(arr)), LIST_DATA_TYPE(l)),                                                       \
     list_insert_range_l(GENERIC_LIST(l), (void *)(arr), sizeof(LIST_DATA_TYPE(l)), (count)))

///
/// Append a contiguous range of elements to the end of the list. R-value form.
///
/// SUCCESS : Returns `true`. `count` new nodes holding copies of the source
///           elements are linked at the tail; list length grows by `count`.
///           The source range is untouched.
/// FAILURE : Returns `false` on allocation failure during the first node
///           allocation. The list and source are unchanged.
///
/// TAGS: List, PushBack, Range, RValue
///
#define ListPushArrR(l, arr, count)                                                                                    \
    (ValidateList(l),                                                                                                  \
     CHECK_TYPE_CONVERTIBLE(const LIST_DATA_TYPE(l) *, arr),                                                           \
     list_insert_range_r(GENERIC_LIST(l), (const void *)(arr), sizeof(LIST_DATA_TYPE(l)), (count)))

///
/// Default range-push alias for `ListPushArrL`.
///
#define ListPushArr(l, arr, count) ListPushArrL((l), (arr), (count))

///
/// Append all elements of `l2` to the end of `l`.
/// L-value form: when the destination has no `copy_init` handler, ownership of
/// `l2`'s storage transfers and `l2` is left empty on success.
///
/// l[in,out]  : Destination list.
/// l2[in,out] : Source list. May be emptied on success.
///
/// SUCCESS : Returns `true`. `l->length` grows by the previous `l2->length`;
///           the appended nodes form the new tail of `l`. When `l` has no
///           `copy_init` handler, the nodes from `l2` have been relinked
///           into `l` and `l2` is left empty (head/tail NULL, length 0).
///           With a deep-copy handler, `l2` is unchanged.
/// FAILURE : Returns `false` on allocation failure. Both `l` and `l2` are
///           unchanged.
///
/// TAGS: List, Merge, LValue, Ownership
///
#define ListMergeL(l, l2)                                                                                              \
    (ValidateList(l),                                                                                                  \
     ValidateList(l2),                                                                                                 \
     CHECK_TYPE_EQUIVALENCE(LIST_DATA_TYPE(l2), LIST_DATA_TYPE(l)),                                                    \
     list_merge_l(GENERIC_LIST(l), GENERIC_LIST(l2), sizeof(LIST_DATA_TYPE(l))))

///
/// Append a copy of all elements of `l2` to the end of `l`. R-value form: the
/// source list is never emptied; its contents are read-only.
///
/// SUCCESS : Returns `true`. `l->length` grows by `l2->length`; copies of
///           every `l2` node form the new tail of `l`. `l2` is untouched.
/// FAILURE : Returns `false` on allocation failure. `l` is unchanged.
///
/// TAGS: List, Merge, RValue
///
#define ListMergeR(l, l2)                                                                                              \
    (ValidateList(l),                                                                                                  \
     ValidateList(l2),                                                                                                 \
     CHECK_TYPE_EQUIVALENCE(LIST_DATA_TYPE(l2), LIST_DATA_TYPE(l)),                                                    \
     list_merge_r(GENERIC_LIST(l), GENERIC_LIST(l2), sizeof(LIST_DATA_TYPE(l))))

///
/// Default merge alias for `ListMergeL`.
///
#define ListMerge(l, l2) ListMergeL((l), (l2))

///
/// Aborting (`Must*`) variants of every fallible insertion macro above.
///
/// Each `ListMustXxx(...)` is the statement-style do-while wrapper around the
/// matching `ListXxx(...)` expression: it calls the underlying fallible form
/// and triggers `LOG_FATAL(...)` if the call returns `false`. Use these at
/// API boundaries where allocation failure is not recoverable for the caller.
/// Otherwise prefer the propagating forms.
///
/// SUCCESS : Returns to the caller.
/// FAILURE : Does not return - aborts via `LOG_FATAL` / `SysAbort`.
///
/// TAGS: List, Insert, Must, Abort
///
#define ListMustInsertL(l, lval, idx)                                                                                  \
    do {                                                                                                               \
        if (!ListInsertL((l), (lval), (idx))) {                                                                        \
            LOG_FATAL("ListMustInsertL failed");                                                                       \
        }                                                                                                              \
    } while (0)
///
/// Aborting variant of `ListInsertR`. See that macro for parameter
/// semantics and success-state effects.
///
/// SUCCESS : Returns to the caller. The underlying `ListInsertR` call
///           succeeded; see `ListInsertR` for the post-state.
/// FAILURE : Does not return - aborts via `LOG_FATAL` / `SysAbort` when
///           the underlying `ListInsertR` call returns `false`.
///
/// TAGS: List, Must, Abort
///
#define ListMustInsertR(l, rval, idx)                                                                                  \
    do {                                                                                                               \
        if (!ListInsertR((l), (rval), (idx))) {                                                                        \
            LOG_FATAL("ListMustInsertR failed");                                                                       \
        }                                                                                                              \
    } while (0)
///
/// Aborting variant of `ListInsert`. See that macro for parameter
/// semantics and success-state effects.
///
/// SUCCESS : Returns to the caller. The underlying `ListInsert` call
///           succeeded; see `ListInsert` for the post-state.
/// FAILURE : Does not return - aborts via `LOG_FATAL` / `SysAbort` when
///           the underlying `ListInsert` call returns `false`.
///
/// TAGS: List, Must, Abort
///
#define ListMustInsert(l, lval, idx)                                                                                   \
    do {                                                                                                               \
        if (!ListInsert((l), (lval), (idx))) {                                                                         \
            LOG_FATAL("ListMustInsert failed");                                                                        \
        }                                                                                                              \
    } while (0)

///
/// Aborting variant of `ListPushFrontL`. See that macro for parameter
/// semantics and success-state effects.
///
/// SUCCESS : Returns to the caller. The underlying `ListPushFrontL` call
///           succeeded; see `ListPushFrontL` for the post-state.
/// FAILURE : Does not return - aborts via `LOG_FATAL` / `SysAbort` when
///           the underlying `ListPushFrontL` call returns `false`.
///
/// TAGS: List, Must, Abort
///
#define ListMustPushFrontL(l, lval)                                                                                    \
    do {                                                                                                               \
        if (!ListPushFrontL((l), (lval))) {                                                                            \
            LOG_FATAL("ListMustPushFrontL failed");                                                                    \
        }                                                                                                              \
    } while (0)
///
/// Aborting variant of `ListPushFrontR`. See that macro for parameter
/// semantics and success-state effects.
///
/// SUCCESS : Returns to the caller. The underlying `ListPushFrontR` call
///           succeeded; see `ListPushFrontR` for the post-state.
/// FAILURE : Does not return - aborts via `LOG_FATAL` / `SysAbort` when
///           the underlying `ListPushFrontR` call returns `false`.
///
/// TAGS: List, Must, Abort
///
#define ListMustPushFrontR(l, rval)                                                                                    \
    do {                                                                                                               \
        if (!ListPushFrontR((l), (rval))) {                                                                            \
            LOG_FATAL("ListMustPushFrontR failed");                                                                    \
        }                                                                                                              \
    } while (0)
///
/// Aborting variant of `ListPushFront`. See that macro for parameter
/// semantics and success-state effects.
///
/// SUCCESS : Returns to the caller. The underlying `ListPushFront` call
///           succeeded; see `ListPushFront` for the post-state.
/// FAILURE : Does not return - aborts via `LOG_FATAL` / `SysAbort` when
///           the underlying `ListPushFront` call returns `false`.
///
/// TAGS: List, Must, Abort
///
#define ListMustPushFront(l, lval)                                                                                     \
    do {                                                                                                               \
        if (!ListPushFront((l), (lval))) {                                                                             \
            LOG_FATAL("ListMustPushFront failed");                                                                     \
        }                                                                                                              \
    } while (0)

///
/// Aborting variant of `ListPushBackL`. See that macro for parameter
/// semantics and success-state effects.
///
/// SUCCESS : Returns to the caller. The underlying `ListPushBackL` call
///           succeeded; see `ListPushBackL` for the post-state.
/// FAILURE : Does not return - aborts via `LOG_FATAL` / `SysAbort` when
///           the underlying `ListPushBackL` call returns `false`.
///
/// TAGS: List, Must, Abort
///
#define ListMustPushBackL(l, lval)                                                                                     \
    do {                                                                                                               \
        if (!ListPushBackL((l), (lval))) {                                                                             \
            LOG_FATAL("ListMustPushBackL failed");                                                                     \
        }                                                                                                              \
    } while (0)
///
/// Aborting variant of `ListPushBackR`. See that macro for parameter
/// semantics and success-state effects.
///
/// SUCCESS : Returns to the caller. The underlying `ListPushBackR` call
///           succeeded; see `ListPushBackR` for the post-state.
/// FAILURE : Does not return - aborts via `LOG_FATAL` / `SysAbort` when
///           the underlying `ListPushBackR` call returns `false`.
///
/// TAGS: List, Must, Abort
///
#define ListMustPushBackR(l, rval)                                                                                     \
    do {                                                                                                               \
        if (!ListPushBackR((l), (rval))) {                                                                             \
            LOG_FATAL("ListMustPushBackR failed");                                                                     \
        }                                                                                                              \
    } while (0)
///
/// Aborting variant of `ListPushBack`. See that macro for parameter
/// semantics and success-state effects.
///
/// SUCCESS : Returns to the caller. The underlying `ListPushBack` call
///           succeeded; see `ListPushBack` for the post-state.
/// FAILURE : Does not return - aborts via `LOG_FATAL` / `SysAbort` when
///           the underlying `ListPushBack` call returns `false`.
///
/// TAGS: List, Must, Abort
///
#define ListMustPushBack(l, lval)                                                                                      \
    do {                                                                                                               \
        if (!ListPushBack((l), (lval))) {                                                                              \
            LOG_FATAL("ListMustPushBack failed");                                                                      \
        }                                                                                                              \
    } while (0)

///
/// Aborting variant of `ListPushArrL`. See that macro for parameter
/// semantics and success-state effects.
///
/// SUCCESS : Returns to the caller. The underlying `ListPushArrL` call
///           succeeded; see `ListPushArrL` for the post-state.
/// FAILURE : Does not return - aborts via `LOG_FATAL` / `SysAbort` when
///           the underlying `ListPushArrL` call returns `false`.
///
/// TAGS: List, Must, Abort
///
#define ListMustPushArrL(l, arr, count)                                                                                \
    do {                                                                                                               \
        if (!ListPushArrL((l), (arr), (count))) {                                                                      \
            LOG_FATAL("ListMustPushArrL failed");                                                                      \
        }                                                                                                              \
    } while (0)
///
/// Aborting variant of `ListPushArrR`. See that macro for parameter
/// semantics and success-state effects.
///
/// SUCCESS : Returns to the caller. The underlying `ListPushArrR` call
///           succeeded; see `ListPushArrR` for the post-state.
/// FAILURE : Does not return - aborts via `LOG_FATAL` / `SysAbort` when
///           the underlying `ListPushArrR` call returns `false`.
///
/// TAGS: List, Must, Abort
///
#define ListMustPushArrR(l, arr, count)                                                                                \
    do {                                                                                                               \
        if (!ListPushArrR((l), (arr), (count))) {                                                                      \
            LOG_FATAL("ListMustPushArrR failed");                                                                      \
        }                                                                                                              \
    } while (0)
///
/// Aborting variant of `ListPushArr`. See that macro for parameter
/// semantics and success-state effects.
///
/// SUCCESS : Returns to the caller. The underlying `ListPushArr` call
///           succeeded; see `ListPushArr` for the post-state.
/// FAILURE : Does not return - aborts via `LOG_FATAL` / `SysAbort` when
///           the underlying `ListPushArr` call returns `false`.
///
/// TAGS: List, Must, Abort
///
#define ListMustPushArr(l, arr, count)                                                                                 \
    do {                                                                                                               \
        if (!ListPushArr((l), (arr), (count))) {                                                                       \
            LOG_FATAL("ListMustPushArr failed");                                                                       \
        }                                                                                                              \
    } while (0)

///
/// Aborting variant of `ListMergeL`. See that macro for parameter
/// semantics and success-state effects.
///
/// SUCCESS : Returns to the caller. The underlying `ListMergeL` call
///           succeeded; see `ListMergeL` for the post-state.
/// FAILURE : Does not return - aborts via `LOG_FATAL` / `SysAbort` when
///           the underlying `ListMergeL` call returns `false`.
///
/// TAGS: List, Must, Abort
///
#define ListMustMergeL(l, l2)                                                                                          \
    do {                                                                                                               \
        if (!ListMergeL((l), (l2))) {                                                                                  \
            LOG_FATAL("ListMustMergeL failed");                                                                        \
        }                                                                                                              \
    } while (0)
///
/// Aborting variant of `ListMergeR`. See that macro for parameter
/// semantics and success-state effects.
///
/// SUCCESS : Returns to the caller. The underlying `ListMergeR` call
///           succeeded; see `ListMergeR` for the post-state.
/// FAILURE : Does not return - aborts via `LOG_FATAL` / `SysAbort` when
///           the underlying `ListMergeR` call returns `false`.
///
/// TAGS: List, Must, Abort
///
#define ListMustMergeR(l, l2)                                                                                          \
    do {                                                                                                               \
        if (!ListMergeR((l), (l2))) {                                                                                  \
            LOG_FATAL("ListMustMergeR failed");                                                                        \
        }                                                                                                              \
    } while (0)
///
/// Aborting variant of `ListMerge`. See that macro for parameter
/// semantics and success-state effects.
///
/// SUCCESS : Returns to the caller. The underlying `ListMerge` call
///           succeeded; see `ListMerge` for the post-state.
/// FAILURE : Does not return - aborts via `LOG_FATAL` / `SysAbort` when
///           the underlying `ListMerge` call returns `false`.
///
/// TAGS: List, Must, Abort
///
#define ListMustMerge(l, l2)                                                                                           \
    do {                                                                                                               \
        if (!ListMerge((l), (l2))) {                                                                                   \
            LOG_FATAL("ListMustMerge failed");                                                                         \
        }                                                                                                              \
    } while (0)

#endif // MISRA_STD_CONTAINER_LIST_INSERT_H
