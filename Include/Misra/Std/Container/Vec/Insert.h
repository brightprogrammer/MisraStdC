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
