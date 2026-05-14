/// file      : Insert.h
/// author    : Generated following Misra project patterns
/// This is free and unencumbered software released into the public domain.
///
/// List insertion helpers.
///

#ifndef MISRA_STD_CONTAINER_LIST_INSERT_H
#define MISRA_STD_CONTAINER_LIST_INSERT_H

#include "Private.h"

#if defined(MISRA_ENFORCE_TYPE_SAFETY) && MISRA_ENFORCE_TYPE_SAFETY
#    define LIST_TYPECHECK_L(l, lval)      ((void)sizeof(char[_Generic(&(lval), LIST_DATA_TYPE(l) *: 1, default: -1)]))
#    define LIST_TYPECHECK_R(l, rval)      ((void)sizeof((LIST_DATA_TYPE(l)[]) {(rval)}))
#    define LIST_TYPECHECK_RANGE_L(l, ptr) ((void)sizeof(char[_Generic((ptr), LIST_DATA_TYPE(l) *: 1, default: -1)]))
#    define LIST_TYPECHECK_RANGE_R(l, ptr)                                                                             \
        ((void)sizeof(char[_Generic((ptr), LIST_DATA_TYPE(l) *: 1, const LIST_DATA_TYPE(l) *: 1, default: -1)]))
#    define LIST_TYPECHECK_LIST(l, l2)                                                                                 \
        ((void)sizeof(char[_Generic((l2)->head->data, LIST_DATA_TYPE(l) *: 1, default: -1)]))
#else
#    define LIST_TYPECHECK_L(l, lval)      ((void)0)
#    define LIST_TYPECHECK_R(l, rval)      ((void)0)
#    define LIST_TYPECHECK_RANGE_L(l, ptr) ((void)0)
#    define LIST_TYPECHECK_RANGE_R(l, ptr) ((void)0)
#    define LIST_TYPECHECK_LIST(l, l2)     ((void)0)
#endif

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
/// SUCCESS : `true`.
/// FAILURE : `false` on allocation failure. The list and `lval` are unchanged.
///
/// TAGS: List, Insert, LValue, Ownership
///
#define ListInsertL(l, lval, idx)                                                                                      \
    (ValidateList(l),                                                                                                  \
     LIST_TYPECHECK_L((l), (lval)),                                                                                    \
     list_insert_one_l(GENERIC_LIST(l), &LVAL((LIST_DATA_TYPE(l))(lval)), &(lval), sizeof(LIST_DATA_TYPE(l)), (idx)))

///
/// Insert a single element at the given index. R-value form: source is treated
/// as a temporary value and is never zeroed.
///
/// l[in,out] : List handle.
/// rval[in]  : Value to insert.
/// idx[in]   : Position in [0, length].
///
/// SUCCESS : `true`.
/// FAILURE : `false` on allocation failure.
///
/// TAGS: List, Insert, RValue
///
#define ListInsertR(l, rval, idx)                                                                                      \
    (ValidateList(l),                                                                                                  \
     LIST_TYPECHECK_R((l), (rval)),                                                                                    \
     list_insert_one_r(GENERIC_LIST(l), &LVAL((LIST_DATA_TYPE(l))(rval)), sizeof(LIST_DATA_TYPE(l)), (idx)))

///
/// Default insertion alias for `ListInsertL`.
///
#define ListInsert(l, lval, idx) ListInsertL((l), (lval), (idx))

///
/// Prepend an element at the head of the list. L-value ownership form.
///
/// TAGS: List, PushFront, LValue
///
#define ListPushFrontL(l, lval) ListInsertL((l), (lval), 0)

///
/// Prepend an element at the head of the list. R-value form.
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
/// TAGS: List, PushBack, LValue
///
#define ListPushBackL(l, lval) ListInsertL((l), (lval), (l)->length)

///
/// Append an element at the tail of the list. R-value form.
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
/// SUCCESS : `true`.
/// FAILURE : `false` on allocation failure.
///
/// TAGS: List, PushBack, Range, LValue
///
#define ListPushArrL(l, arr, count)                                                                                    \
    (ValidateList(l),                                                                                                  \
     LIST_TYPECHECK_RANGE_L((l), (arr)),                                                                               \
     list_insert_range_l(GENERIC_LIST(l), (void *)(arr), sizeof(LIST_DATA_TYPE(l)), (count)))

///
/// Append a contiguous range of elements to the end of the list. R-value form.
///
/// TAGS: List, PushBack, Range, RValue
///
#define ListPushArrR(l, arr, count)                                                                                    \
    (ValidateList(l),                                                                                                  \
     LIST_TYPECHECK_RANGE_R((l), (arr)),                                                                               \
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
/// SUCCESS : `true`.
/// FAILURE : `false` on allocation failure. Both lists are unchanged.
///
/// TAGS: List, Merge, LValue, Ownership
///
#define ListMergeL(l, l2)                                                                                              \
    (ValidateList(l),                                                                                                  \
     ValidateList(l2),                                                                                                 \
     LIST_TYPECHECK_LIST((l), (l2)),                                                                                   \
     list_merge_l(GENERIC_LIST(l), GENERIC_LIST(l2), sizeof(LIST_DATA_TYPE(l))))

///
/// Append a copy of all elements of `l2` to the end of `l`. R-value form: the
/// source list is never emptied; its contents are read-only.
///
/// TAGS: List, Merge, RValue
///
#define ListMergeR(l, l2)                                                                                              \
    (ValidateList(l),                                                                                                  \
     ValidateList(l2),                                                                                                 \
     LIST_TYPECHECK_LIST((l), (l2)),                                                                                   \
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
#define ListMustInsertR(l, rval, idx)                                                                                  \
    do {                                                                                                               \
        if (!ListInsertR((l), (rval), (idx))) {                                                                        \
            LOG_FATAL("ListMustInsertR failed");                                                                       \
        }                                                                                                              \
    } while (0)
#define ListMustInsert(l, lval, idx)                                                                                   \
    do {                                                                                                               \
        if (!ListInsert((l), (lval), (idx))) {                                                                         \
            LOG_FATAL("ListMustInsert failed");                                                                        \
        }                                                                                                              \
    } while (0)

#define ListMustPushFrontL(l, lval)                                                                                    \
    do {                                                                                                               \
        if (!ListPushFrontL((l), (lval))) {                                                                            \
            LOG_FATAL("ListMustPushFrontL failed");                                                                    \
        }                                                                                                              \
    } while (0)
#define ListMustPushFrontR(l, rval)                                                                                    \
    do {                                                                                                               \
        if (!ListPushFrontR((l), (rval))) {                                                                            \
            LOG_FATAL("ListMustPushFrontR failed");                                                                    \
        }                                                                                                              \
    } while (0)
#define ListMustPushFront(l, lval)                                                                                     \
    do {                                                                                                               \
        if (!ListPushFront((l), (lval))) {                                                                             \
            LOG_FATAL("ListMustPushFront failed");                                                                     \
        }                                                                                                              \
    } while (0)

#define ListMustPushBackL(l, lval)                                                                                     \
    do {                                                                                                               \
        if (!ListPushBackL((l), (lval))) {                                                                             \
            LOG_FATAL("ListMustPushBackL failed");                                                                     \
        }                                                                                                              \
    } while (0)
#define ListMustPushBackR(l, rval)                                                                                     \
    do {                                                                                                               \
        if (!ListPushBackR((l), (rval))) {                                                                             \
            LOG_FATAL("ListMustPushBackR failed");                                                                     \
        }                                                                                                              \
    } while (0)
#define ListMustPushBack(l, lval)                                                                                      \
    do {                                                                                                               \
        if (!ListPushBack((l), (lval))) {                                                                              \
            LOG_FATAL("ListMustPushBack failed");                                                                      \
        }                                                                                                              \
    } while (0)

#define ListMustPushArrL(l, arr, count)                                                                                \
    do {                                                                                                               \
        if (!ListPushArrL((l), (arr), (count))) {                                                                      \
            LOG_FATAL("ListMustPushArrL failed");                                                                      \
        }                                                                                                              \
    } while (0)
#define ListMustPushArrR(l, arr, count)                                                                                \
    do {                                                                                                               \
        if (!ListPushArrR((l), (arr), (count))) {                                                                      \
            LOG_FATAL("ListMustPushArrR failed");                                                                      \
        }                                                                                                              \
    } while (0)
#define ListMustPushArr(l, arr, count)                                                                                 \
    do {                                                                                                               \
        if (!ListPushArr((l), (arr), (count))) {                                                                       \
            LOG_FATAL("ListMustPushArr failed");                                                                       \
        }                                                                                                              \
    } while (0)

#define ListMustMergeL(l, l2)                                                                                          \
    do {                                                                                                               \
        if (!ListMergeL((l), (l2))) {                                                                                  \
            LOG_FATAL("ListMustMergeL failed");                                                                        \
        }                                                                                                              \
    } while (0)
#define ListMustMergeR(l, l2)                                                                                          \
    do {                                                                                                               \
        if (!ListMergeR((l), (l2))) {                                                                                  \
            LOG_FATAL("ListMustMergeR failed");                                                                        \
        }                                                                                                              \
    } while (0)
#define ListMustMerge(l, l2)                                                                                           \
    do {                                                                                                               \
        if (!ListMerge((l), (l2))) {                                                                                   \
            LOG_FATAL("ListMustMerge failed");                                                                         \
        }                                                                                                              \
    } while (0)

#endif // MISRA_STD_CONTAINER_LIST_INSERT_H
