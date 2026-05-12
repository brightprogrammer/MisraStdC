/// file      : Insert.h
/// author    : Generated following Misra project patterns
/// This is free and unencumbered software released into the public domain.
///
/// List insertion helpers.
///

#ifndef MISRA_STD_CONTAINER_LIST_INSERT_H
#define MISRA_STD_CONTAINER_LIST_INSERT_H

#include "Private.h"

#include <stdio.h>

void SysAbort(void);

#if defined(MISRA_ENFORCE_TYPE_SAFETY) && MISRA_ENFORCE_TYPE_SAFETY
#    define LIST_TYPECHECK_L(l, lval) ((void)sizeof(char[_Generic(&(lval), LIST_DATA_TYPE(l) * : 1, default : -1)]))
#    define LIST_TYPECHECK_R(l, rval) ((void)sizeof((LIST_DATA_TYPE(l)[]){(rval)}))
#    define LIST_TYPECHECK_RANGE_L(l, ptr) ((void)sizeof(char[_Generic((ptr), LIST_DATA_TYPE(l) * : 1, default : -1)]))
#    define LIST_TYPECHECK_RANGE_R(l, ptr)                                                                             \
        ((void)sizeof(char[_Generic((ptr), LIST_DATA_TYPE(l) * : 1, const LIST_DATA_TYPE(l) * : 1, default : -1)]))
#    define LIST_TYPECHECK_LIST(l, l2) ((void)sizeof(char[_Generic((l2)->head->data, LIST_DATA_TYPE(l) * : 1, default : -1)]))
#else
#    define LIST_TYPECHECK_L(l, lval) ((void)0)
#    define LIST_TYPECHECK_R(l, rval) ((void)0)
#    define LIST_TYPECHECK_RANGE_L(l, ptr) ((void)0)
#    define LIST_TYPECHECK_RANGE_R(l, ptr) ((void)0)
#    define LIST_TYPECHECK_LIST(l, l2) ((void)0)
#endif

#define LIST_ABORT(message) list_abort_insert_operation(__func__, __LINE__, (message))
#define LIST_MUST(operation, message)                                                                                  \
    do {                                                                                                               \
        if (!(operation)) {                                                                                            \
            LIST_ABORT(message);                                                                                       \
        }                                                                                                              \
    } while (0)

static inline void list_abort_insert_operation(const char *function, int line, const char *message) {
    fprintf(stderr, "FATAL [%s:%d] %s\n", function, line, message);
    SysAbort();
}

static inline bool list_insert_one_l_impl(
    GenericList *list, const void *item_copy, void *source, u64 item_size, u64 idx
) {
    return list_zero_source_on_success(list, source, item_size, insert_into_list(list, item_copy, item_size, idx));
}

static inline bool list_insert_one_r_impl(GenericList *list, const void *item_copy, u64 item_size, u64 idx) {
    return insert_into_list(list, item_copy, item_size, idx);
}

static inline bool list_insert_range_l_impl(GenericList *list, void *items, u64 item_size, u64 count) {
    if (!count) {
        return true;
    }

    if (!items) {
        LIST_ABORT("Expected a valid pointer");
    }

    return list_zero_source_on_success(list, items, item_size * count, push_arr_list(list, item_size, items, count));
}

static inline bool list_insert_range_r_impl(GenericList *list, const void *items, u64 item_size, u64 count) {
    if (!count) {
        return true;
    }

    if (!items) {
        LIST_ABORT("Expected a valid pointer");
    }

    return push_arr_list(list, item_size, items, count);
}

static inline bool list_merge_l_impl(GenericList *dst, GenericList *src, u64 item_size) {
    if (!src->length) {
        return true;
    }

    return list_release_merged_source_on_success(dst, src, item_size, merge_list(dst, item_size, src));
}

static inline bool list_merge_r_impl(GenericList *dst, GenericList *src, u64 item_size) {
    if (!src->length) {
        return true;
    }

    return merge_list(dst, item_size, src);
}

#define ListInsertL(l, lval, idx)                                                                                      \
    (ValidateList(l),                                                                                                  \
     LIST_TYPECHECK_L((l), (lval)),                                                                                    \
     list_insert_one_l_impl(                                                                                           \
         GENERIC_LIST(l),                                                                                              \
         &LVAL((LIST_DATA_TYPE(l))(lval)),                                                                             \
         &(lval),                                                                                                      \
         sizeof(LIST_DATA_TYPE(l)),                                                                                    \
         (idx)                                                                                                         \
     ))

#define ListInsertR(l, rval, idx)                                                                                      \
    (ValidateList(l),                                                                                                  \
     LIST_TYPECHECK_R((l), (rval)),                                                                                    \
     list_insert_one_r_impl(GENERIC_LIST(l), &LVAL((LIST_DATA_TYPE(l))(rval)), sizeof(LIST_DATA_TYPE(l)), (idx)))

#define ListInsert(l, lval, idx) ListInsertL((l), (lval), (idx))

#define ListPushFrontL(l, lval) ListInsertL((l), (lval), 0)
#define ListPushFrontR(l, rval) ListInsertR((l), (rval), 0)
#define ListPushFront(l, lval) ListPushFrontL((l), (lval))

#define ListPushBackL(l, lval) ListInsertL((l), (lval), (l)->length)
#define ListPushBackR(l, rval) ListInsertR((l), (rval), (l)->length)
#define ListPushBack(l, lval) ListPushBackL((l), (lval))

#define ListPushArrL(l, arr, count)                                                                                    \
    (ValidateList(l),                                                                                                  \
     LIST_TYPECHECK_RANGE_L((l), (arr)),                                                                               \
     list_insert_range_l_impl(GENERIC_LIST(l), (void *)(arr), sizeof(LIST_DATA_TYPE(l)), (count)))

#define ListPushArrR(l, arr, count)                                                                                    \
    (ValidateList(l),                                                                                                  \
     LIST_TYPECHECK_RANGE_R((l), (arr)),                                                                               \
     push_arr_list(GENERIC_LIST(l), sizeof(LIST_DATA_TYPE(l)), (const void *)(arr), (count)))

#define ListPushArr(l, arr, count) ListPushArrL((l), (arr), (count))

#define ListMergeL(l, l2)                                                                                              \
    (ValidateList(l),                                                                                                  \
     ValidateList(l2),                                                                                                 \
     LIST_TYPECHECK_LIST((l), (l2)),                                                                                   \
     list_merge_l_impl(GENERIC_LIST(l), GENERIC_LIST(l2), sizeof(LIST_DATA_TYPE(l))))

#define ListMergeR(l, l2)                                                                                              \
    (ValidateList(l),                                                                                                  \
     ValidateList(l2),                                                                                                 \
     LIST_TYPECHECK_LIST((l), (l2)),                                                                                   \
     list_merge_r_impl(GENERIC_LIST(l), GENERIC_LIST(l2), sizeof(LIST_DATA_TYPE(l))))

#define ListMerge(l, l2) ListMergeL((l), (l2))

#define ListMustInsertL(l, lval, idx) LIST_MUST(ListInsertL((l), (lval), (idx)), "ListMustInsertL failed")
#define ListMustInsertR(l, rval, idx) LIST_MUST(ListInsertR((l), (rval), (idx)), "ListMustInsertR failed")
#define ListMustInsert(l, lval, idx) LIST_MUST(ListInsert((l), (lval), (idx)), "ListMustInsert failed")

#define ListMustPushFrontL(l, lval) LIST_MUST(ListPushFrontL((l), (lval)), "ListMustPushFrontL failed")
#define ListMustPushFrontR(l, rval) LIST_MUST(ListPushFrontR((l), (rval)), "ListMustPushFrontR failed")
#define ListMustPushFront(l, lval) LIST_MUST(ListPushFront((l), (lval)), "ListMustPushFront failed")

#define ListMustPushBackL(l, lval) LIST_MUST(ListPushBackL((l), (lval)), "ListMustPushBackL failed")
#define ListMustPushBackR(l, rval) LIST_MUST(ListPushBackR((l), (rval)), "ListMustPushBackR failed")
#define ListMustPushBack(l, lval) LIST_MUST(ListPushBack((l), (lval)), "ListMustPushBack failed")

#define ListMustPushArrL(l, arr, count) LIST_MUST(ListPushArrL((l), (arr), (count)), "ListMustPushArrL failed")
#define ListMustPushArrR(l, arr, count) LIST_MUST(ListPushArrR((l), (arr), (count)), "ListMustPushArrR failed")
#define ListMustPushArr(l, arr, count) LIST_MUST(ListPushArr((l), (arr), (count)), "ListMustPushArr failed")

#define ListMustMergeL(l, l2) LIST_MUST(ListMergeL((l), (l2)), "ListMustMergeL failed")
#define ListMustMergeR(l, l2) LIST_MUST(ListMergeR((l), (l2)), "ListMustMergeR failed")
#define ListMustMerge(l, l2) LIST_MUST(ListMerge((l), (l2)), "ListMustMerge failed")

#endif // MISRA_STD_CONTAINER_LIST_INSERT_H
