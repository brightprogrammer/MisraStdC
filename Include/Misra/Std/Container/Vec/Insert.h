/// file      : std/container/vec/insert.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Insert items into vector in different ways.

#ifndef MISRA_STD_CONTAINER_VEC_INSERT_H
#define MISRA_STD_CONTAINER_VEC_INSERT_H

#include "Private.h"

#include <stdio.h>

void SysAbort(void);

#if defined(MISRA_ENFORCE_TYPE_SAFETY) && MISRA_ENFORCE_TYPE_SAFETY
#    define VEC_TYPECHECK_L(v, lval) ((void)sizeof(char[_Generic(&(lval), VEC_DATATYPE(v) * : 1, default : -1)]))
#    define VEC_TYPECHECK_R(v, rval) ((void)sizeof((VEC_DATATYPE(v)[]){(rval)}))
#    define VEC_TYPECHECK_RANGE_L(v, ptr) ((void)sizeof(char[_Generic((ptr), VEC_DATATYPE(v) * : 1, default : -1)]))
#    define VEC_TYPECHECK_RANGE_R(v, ptr)                                                                              \
        ((void)sizeof(char[_Generic((ptr), VEC_DATATYPE(v) * : 1, const VEC_DATATYPE(v) * : 1, default : -1)]))
#else
#    define VEC_TYPECHECK_L(v, lval) ((void)0)
#    define VEC_TYPECHECK_R(v, rval) ((void)0)
#    define VEC_TYPECHECK_RANGE_L(v, ptr) ((void)0)
#    define VEC_TYPECHECK_RANGE_R(v, ptr) ((void)0)
#endif

#define VEC_ABORT(message) vec_abort_insert_operation(__func__, __LINE__, (message))
#define VEC_MUST(operation, message)                                                                                   \
    do {                                                                                                               \
        if (!(operation)) {                                                                                            \
            VEC_ABORT(message);                                                                                        \
        }                                                                                                              \
    } while (0)

static inline void vec_abort_insert_operation(const char *function, int line, const char *message) {
    fprintf(stderr, "FATAL [%s:%d] %s\n", function, line, message);
    SysAbort();
}

static inline bool vec_insert_one_l_impl(
    GenericVec *vec, const void *item_copy, void *source, size item_size, size idx, bool preserve_order
) {
    bool success = preserve_order ? insert_range_into_vec(vec, item_copy, item_size, idx, 1)
                                  : insert_range_fast_into_vec(vec, item_copy, item_size, idx, 1);

    return vec_zero_source_on_success(vec, source, item_size, success);
}

static inline bool vec_insert_one_r_impl(
    GenericVec *vec, const void *item_copy, size item_size, size idx, bool preserve_order
) {
    return preserve_order ? insert_range_into_vec(vec, item_copy, item_size, idx, 1)
                          : insert_range_fast_into_vec(vec, item_copy, item_size, idx, 1);
}

static inline bool vec_insert_range_l_impl(
    GenericVec *vec, void *items, size item_size, size idx, size count, bool preserve_order
) {
    bool success;

    if (!items) {
        VEC_ABORT("Expected a valid pointer");
    }

    success = preserve_order ? insert_range_into_vec(vec, items, item_size, idx, count)
                             : insert_range_fast_into_vec(vec, items, item_size, idx, count);

    return vec_zero_source_on_success(vec, items, count * item_size, success);
}

static inline bool vec_insert_range_r_impl(
    GenericVec *vec, const void *items, size item_size, size idx, size count, bool preserve_order
) {
    if (!items) {
        VEC_ABORT("Expected a valid pointer");
    }

    return preserve_order ? insert_range_into_vec(vec, items, item_size, idx, count)
                          : insert_range_fast_into_vec(vec, items, item_size, idx, count);
}

static inline bool vec_merge_l_impl(GenericVec *dst, GenericVec *src, size item_size) {
    if (!src->data || !src->length) {
        return true;
    }

    return vec_release_merged_source_on_success(
        dst, src, item_size, vec_insert_range_l_impl(dst, src->data, item_size, dst->length, src->length, true)
    );
}

static inline bool vec_merge_r_impl(GenericVec *dst, const GenericVec *src, size item_size) {
    if (!src->data || !src->length) {
        return true;
    }

    return vec_insert_range_r_impl(dst, src->data, item_size, dst->length, src->length, true);
}

#define VecInsertL(v, lval, idx)                                                                                       \
    (ValidateVec(v),                                                                                                   \
     VEC_TYPECHECK_L((v), (lval)),                                                                                     \
     vec_insert_one_l_impl(                                                                                            \
         GENERIC_VEC(v),                                                                                               \
         &LVAL((VEC_DATATYPE(v))(lval)),                                                                               \
         &(lval),                                                                                                      \
         sizeof(VEC_DATATYPE(v)),                                                                                      \
         (idx),                                                                                                        \
         true                                                                                                          \
     ))

#define VecInsertR(v, rval, idx)                                                                                       \
    (ValidateVec(v),                                                                                                   \
     VEC_TYPECHECK_R((v), (rval)),                                                                                     \
     vec_insert_one_r_impl(GENERIC_VEC(v), &LVAL((VEC_DATATYPE(v))(rval)), sizeof(VEC_DATATYPE(v)), (idx), true))

#define VecInsert(v, lval, idx) VecInsertL((v), (lval), (idx))

#define VecInsertFastL(v, lval, idx)                                                                                   \
    (ValidateVec(v),                                                                                                   \
     VEC_TYPECHECK_L((v), (lval)),                                                                                     \
     vec_insert_one_l_impl(                                                                                            \
         GENERIC_VEC(v),                                                                                               \
         &LVAL((VEC_DATATYPE(v))(lval)),                                                                               \
         &(lval),                                                                                                      \
         sizeof(VEC_DATATYPE(v)),                                                                                      \
         (idx),                                                                                                        \
         false                                                                                                         \
     ))

#define VecInsertFastR(v, rval, idx)                                                                                   \
    (ValidateVec(v),                                                                                                   \
     VEC_TYPECHECK_R((v), (rval)),                                                                                     \
     vec_insert_one_r_impl(GENERIC_VEC(v), &LVAL((VEC_DATATYPE(v))(rval)), sizeof(VEC_DATATYPE(v)), (idx), false))

#define VecInsertFast(v, lval, idx) VecInsertFastL((v), (lval), (idx))

#define VecInsertRangeL(v, varr, idx, count)                                                                           \
    (ValidateVec(v),                                                                                                   \
     VEC_TYPECHECK_RANGE_L((v), (varr)),                                                                               \
     vec_insert_range_l_impl(GENERIC_VEC(v), (void *)(varr), sizeof(VEC_DATATYPE(v)), (idx), (count), true))

#define VecInsertRangeR(v, varr, idx, count)                                                                           \
    (ValidateVec(v),                                                                                                   \
     VEC_TYPECHECK_RANGE_R((v), (varr)),                                                                               \
     vec_insert_range_r_impl(GENERIC_VEC(v), (const void *)(varr), sizeof(VEC_DATATYPE(v)), (idx), (count), true))

#define VecInsertRange(v, varr, idx, count) VecInsertRangeL((v), (varr), (idx), (count))

#define VecInsertRangeFastL(v, varr, idx, count)                                                                       \
    (ValidateVec(v),                                                                                                   \
     VEC_TYPECHECK_RANGE_L((v), (varr)),                                                                               \
     vec_insert_range_l_impl(GENERIC_VEC(v), (void *)(varr), sizeof(VEC_DATATYPE(v)), (idx), (count), false))

#define VecInsertRangeFastR(v, varr, idx, count)                                                                       \
    (ValidateVec(v),                                                                                                   \
     VEC_TYPECHECK_RANGE_R((v), (varr)),                                                                               \
     vec_insert_range_r_impl(GENERIC_VEC(v), (const void *)(varr), sizeof(VEC_DATATYPE(v)), (idx), (count), false))

#define VecInsertRangeFast(v, varr, idx, count) VecInsertRangeFastL((v), (varr), (idx), (count))

#define VecPushBackArrL(v, arr, count) VecInsertRangeL((v), (arr), (v)->length, (count))
#define VecPushBackArrR(v, arr, count) VecInsertRangeR((v), (arr), (v)->length, (count))
#define VecPushBackArr(v, arr, count) VecPushBackArrL((v), (arr), (count))

#define VecPushFrontArrL(v, arr, count) VecInsertRangeL((v), (arr), 0, (count))
#define VecPushFrontArrR(v, arr, count) VecInsertRangeR((v), (arr), 0, (count))
#define VecPushFrontArr(v, arr, count) VecPushFrontArrL((v), (arr), (count))

#define VecPushFrontArrFastL(v, arr, count) VecInsertRangeFastL((v), (arr), 0, (count))
#define VecPushFrontArrFastR(v, arr, count) VecInsertRangeFastR((v), (arr), 0, (count))
#define VecPushFrontArrFast(v, arr, count) VecPushFrontArrFastL((v), (arr), (count))

#define VecMergeL(v, v2)                                                                                               \
    (ValidateVec(v), ValidateVec(v2), vec_merge_l_impl(GENERIC_VEC(v), GENERIC_VEC(v2), sizeof(VEC_DATATYPE(v))))

#define VecMergeR(v, v2)                                                                                               \
    (ValidateVec(v), ValidateVec(v2), vec_merge_r_impl(GENERIC_VEC(v), GENERIC_VEC(v2), sizeof(VEC_DATATYPE(v))))

#define VecMerge(v, v2) VecMergeL((v), (v2))

#define VecPushBackL(v, val) VecInsertL((v), (val), (v)->length)
#define VecPushBackR(v, val) VecInsertR((v), (val), (v)->length)
#define VecPushBack(v, val) VecInsert((v), (val), (v)->length)

#define VecPushFrontL(v, val) VecInsertL((v), (val), 0)
#define VecPushFrontR(v, val) VecInsertR((v), (val), 0)
#define VecPushFront(v, val) VecPushFrontL((v), (val))

#define VecInitClone(vd, vs)                                                                                            \
    (ValidateVec(vd),                                                                                                   \
     ValidateVec(vs),                                                                                                   \
     VecDeinit(vd),                                                                                                     \
     *(vd) = (TYPE_OF(*(vd)))VEC_INIT_ALIGNED_WITH_DEEP_COPY_VALUE(                                                        \
         (vs)->copy_init,                                                                                               \
         (vs)->copy_deinit,                                                                                             \
         (vs)->alignment,                                                                                               \
         (vs)->allocator                                                                                                 \
     ),                                                                                                                 \
     clone_vec(GENERIC_VEC(vd), GENERIC_VEC(vs), sizeof(VEC_DATATYPE(vd))))

#define VecMustInsertL(v, lval, idx) VEC_MUST(VecInsertL((v), (lval), (idx)), "VecMustInsertL failed")
#define VecMustInsertR(v, rval, idx) VEC_MUST(VecInsertR((v), (rval), (idx)), "VecMustInsertR failed")
#define VecMustInsert(v, lval, idx) VEC_MUST(VecInsert((v), (lval), (idx)), "VecMustInsert failed")

#define VecMustInsertFastL(v, lval, idx) VEC_MUST(VecInsertFastL((v), (lval), (idx)), "VecMustInsertFastL failed")
#define VecMustInsertFastR(v, rval, idx) VEC_MUST(VecInsertFastR((v), (rval), (idx)), "VecMustInsertFastR failed")
#define VecMustInsertFast(v, lval, idx) VEC_MUST(VecInsertFast((v), (lval), (idx)), "VecMustInsertFast failed")

#define VecMustInsertRangeL(v, varr, idx, count)                                                                      \
    VEC_MUST(VecInsertRangeL((v), (varr), (idx), (count)), "VecMustInsertRangeL failed")
#define VecMustInsertRangeR(v, varr, idx, count)                                                                      \
    VEC_MUST(VecInsertRangeR((v), (varr), (idx), (count)), "VecMustInsertRangeR failed")
#define VecMustInsertRange(v, varr, idx, count)                                                                       \
    VEC_MUST(VecInsertRange((v), (varr), (idx), (count)), "VecMustInsertRange failed")

#define VecMustInsertRangeFastL(v, varr, idx, count)                                                                  \
    VEC_MUST(VecInsertRangeFastL((v), (varr), (idx), (count)), "VecMustInsertRangeFastL failed")
#define VecMustInsertRangeFastR(v, varr, idx, count)                                                                  \
    VEC_MUST(VecInsertRangeFastR((v), (varr), (idx), (count)), "VecMustInsertRangeFastR failed")
#define VecMustInsertRangeFast(v, varr, idx, count)                                                                   \
    VEC_MUST(VecInsertRangeFast((v), (varr), (idx), (count)), "VecMustInsertRangeFast failed")

#define VecMustPushBackArrL(v, arr, count) VEC_MUST(VecPushBackArrL((v), (arr), (count)), "VecMustPushBackArrL failed")
#define VecMustPushBackArrR(v, arr, count) VEC_MUST(VecPushBackArrR((v), (arr), (count)), "VecMustPushBackArrR failed")
#define VecMustPushBackArr(v, arr, count) VEC_MUST(VecPushBackArr((v), (arr), (count)), "VecMustPushBackArr failed")

#define VecMustPushFrontArrL(v, arr, count)                                                                           \
    VEC_MUST(VecPushFrontArrL((v), (arr), (count)), "VecMustPushFrontArrL failed")
#define VecMustPushFrontArrR(v, arr, count)                                                                           \
    VEC_MUST(VecPushFrontArrR((v), (arr), (count)), "VecMustPushFrontArrR failed")
#define VecMustPushFrontArr(v, arr, count) VEC_MUST(VecPushFrontArr((v), (arr), (count)), "VecMustPushFrontArr failed")

#define VecMustPushFrontArrFastL(v, arr, count)                                                                       \
    VEC_MUST(VecPushFrontArrFastL((v), (arr), (count)), "VecMustPushFrontArrFastL failed")
#define VecMustPushFrontArrFastR(v, arr, count)                                                                       \
    VEC_MUST(VecPushFrontArrFastR((v), (arr), (count)), "VecMustPushFrontArrFastR failed")
#define VecMustPushFrontArrFast(v, arr, count)                                                                        \
    VEC_MUST(VecPushFrontArrFast((v), (arr), (count)), "VecMustPushFrontArrFast failed")

#define VecMustMergeL(v, v2) VEC_MUST(VecMergeL((v), (v2)), "VecMustMergeL failed")
#define VecMustMergeR(v, v2) VEC_MUST(VecMergeR((v), (v2)), "VecMustMergeR failed")
#define VecMustMerge(v, v2) VEC_MUST(VecMerge((v), (v2)), "VecMustMerge failed")

#define VecMustPushBackL(v, val) VEC_MUST(VecPushBackL((v), (val)), "VecMustPushBackL failed")
#define VecMustPushBackR(v, val) VEC_MUST(VecPushBackR((v), (val)), "VecMustPushBackR failed")
#define VecMustPushBack(v, val) VEC_MUST(VecPushBack((v), (val)), "VecMustPushBack failed")

#define VecMustPushFrontL(v, val) VEC_MUST(VecPushFrontL((v), (val)), "VecMustPushFrontL failed")
#define VecMustPushFrontR(v, val) VEC_MUST(VecPushFrontR((v), (val)), "VecMustPushFrontR failed")
#define VecMustPushFront(v, val) VEC_MUST(VecPushFront((v), (val)), "VecMustPushFront failed")

#define VecMustInitClone(vd, vs) VEC_MUST(VecInitClone((vd), (vs)), "VecMustInitClone failed")

#endif // MISRA_STD_CONTAINER_VEC_INSERT_H
