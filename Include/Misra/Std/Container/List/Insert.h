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

#define ListInsertL(l, lval, idx)                                                                                      \
    (ValidateList(l),                                                                                                  \
     LIST_TYPECHECK_L((l), (lval)),                                                                                    \
     list_insert_one_l(GENERIC_LIST(l), &LVAL((LIST_DATA_TYPE(l))(lval)), &(lval), sizeof(LIST_DATA_TYPE(l)), (idx)))

#define ListInsertR(l, rval, idx)                                                                                      \
    (ValidateList(l),                                                                                                  \
     LIST_TYPECHECK_R((l), (rval)),                                                                                    \
     list_insert_one_r(GENERIC_LIST(l), &LVAL((LIST_DATA_TYPE(l))(rval)), sizeof(LIST_DATA_TYPE(l)), (idx)))

#define ListInsert(l, lval, idx) ListInsertL((l), (lval), (idx))

#define ListPushFrontL(l, lval) ListInsertL((l), (lval), 0)
#define ListPushFrontR(l, rval) ListInsertR((l), (rval), 0)
#define ListPushFront(l, lval)  ListPushFrontL((l), (lval))

#define ListPushBackL(l, lval) ListInsertL((l), (lval), (l)->length)
#define ListPushBackR(l, rval) ListInsertR((l), (rval), (l)->length)
#define ListPushBack(l, lval)  ListPushBackL((l), (lval))

#define ListPushArrL(l, arr, count)                                                                                    \
    (ValidateList(l),                                                                                                  \
     LIST_TYPECHECK_RANGE_L((l), (arr)),                                                                               \
     list_insert_range_l(GENERIC_LIST(l), (void *)(arr), sizeof(LIST_DATA_TYPE(l)), (count)))

#define ListPushArrR(l, arr, count)                                                                                    \
    (ValidateList(l),                                                                                                  \
     LIST_TYPECHECK_RANGE_R((l), (arr)),                                                                               \
     list_insert_range_r(GENERIC_LIST(l), (const void *)(arr), sizeof(LIST_DATA_TYPE(l)), (count)))

#define ListPushArr(l, arr, count) ListPushArrL((l), (arr), (count))

#define ListMergeL(l, l2)                                                                                              \
    (ValidateList(l),                                                                                                  \
     ValidateList(l2),                                                                                                 \
     LIST_TYPECHECK_LIST((l), (l2)),                                                                                   \
     list_merge_l(GENERIC_LIST(l), GENERIC_LIST(l2), sizeof(LIST_DATA_TYPE(l))))

#define ListMergeR(l, l2)                                                                                              \
    (ValidateList(l),                                                                                                  \
     ValidateList(l2),                                                                                                 \
     LIST_TYPECHECK_LIST((l), (l2)),                                                                                   \
     list_merge_r(GENERIC_LIST(l), GENERIC_LIST(l2), sizeof(LIST_DATA_TYPE(l))))

#define ListMerge(l, l2) ListMergeL((l), (l2))

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
