/// file      : std/container/vec/insert.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Insert items into vector in different ways.

#ifndef MISRA_STD_CONTAINER_VEC_INSERT_H
#define MISRA_STD_CONTAINER_VEC_INSERT_H

#include "Type.h"

///
/// Insert an `l-value` into vector of it's type.
/// Insertion index must not exceed vector length.
/// This preserves the ordering of elements. Best to be used with sorted vectors,
/// if the sorted property is to be preserved.
///
/// NOTE: Ownership of item is transferred to vector if no `copy_init` method is set.
///       This is to prevent multiple ownership of same object, once inserted into vector.
///       Object may not be usable after this call if `copy_init` is not set.
///
/// INFO: If `copy_init` is set, then vector will create it's own copy of items.
///
/// In worst case this would to to O(n)
///
/// v[in,out] : Vector to insert item into
/// lval[in]  : l-value to be inserted
/// idx[in]   : Index to insert item at.
///
/// USAGE:
///   // the data
///   int x = 10;
///   int y = 20;
///
///   // vector
///   Vec(int) integers = VecInit();
///
///   // insert items
///   VecInsertL(&integers, x, 0); // x inserted at position 0
///   VecInsertL(&integers, y, 0); // x shifted one position and y is inserted
///   VecInsertL(&integers, LVAL(101), 1); // x shifted one position and 101 is inserted at index 1
///
/// SUCCESS : return
/// FAILURE : Does not return
///
#define VecInsertL(v, lval, idx)                                                                                       \
    do {                                                                                                               \
        ValidateVec(v);                                                                                                \
        VEC_DATATYPE(v) *__ptr__val_##__LINE__ = &(lval);                                                              \
        VEC_DATATYPE(v) __tmp__val_##__LINE__  = (lval);                                                               \
        insert_range_into_vec(GENERIC_VEC(v), (char *)&__tmp__val_##__LINE__, sizeof(VEC_DATATYPE(v)), (idx), 1);      \
        if (!(v)->copy_init) {                                                                                         \
            memset(__ptr__val_##__LINE__, 0, sizeof(VEC_DATATYPE(v)));                                                 \
        }                                                                                                              \
    } while (0)

///
/// Insert an `r-value` into vector of it's type.
/// Insertion index must not exceed vector length.
/// This preserves the ordering of elements. Best to be used with sorted vectors,
/// if the sorted property is to be preserved.
///
/// In worst case this would to to O(n)
///
/// v[in,out] : Vector to insert item into
/// rval[in]  : r-value to be inserted
/// idx[in]   : Index to insert item at.
///
/// USAGE:
///   // the data
///   int x = 10;
///   int y = 20;
///
///   // vector
///   Vec(int) integers = VecInit();
///
///   // insert items
///   VecInsertR(&integers, x, 0); // x inserted at position 0
///   VecInsertR(&integers, y, 0); // x shifted one position and y is inserted
///   VecInsertR(&integers, 5, 1); // x shifted one position and 5 is inserted at index 1
///
/// SUCCESS : return
/// FAILURE : Does not return
///
#define VecInsertR(v, rval, idx)                                                                                       \
    do {                                                                                                               \
        ValidateVec(v);                                                                                                \
        VEC_DATATYPE(v) __tmp__val_##__LINE__ = (rval);                                                                \
        insert_range_into_vec(GENERIC_VEC(v), (char *)&__tmp__val_##__LINE__, sizeof(VEC_DATATYPE(v)), (idx), 1);      \
    } while (0)

///
/// Insert by default behaves like `VecInsertL`, which is to insert an l-value into
/// vector and then take ownership if vector does not have a copy-init method.
///
/// v[in,out] : Vector to insert item into
/// lval[in]   : l-value to be inserted
/// idx[in]   : Index to insert item at.
///
/// SUCCESS : return
/// FAILURE : Does not return
///
#define VecInsert(v, lval, idx) VecInsertL((v), (lval), (idx))

///
/// Quickly insert item into vector. Ordering of elements is not guaranteed
/// to be preserved. This call makes significant difference only for sufficiently
/// large vectors and when `idx` is quite less than `(v)->length`.
///
/// Insertion time is guaranteed to be constant for same data types.
///
/// Usage is exactly same as `VecInsert`, just the internal implementation is
/// different.
///
/// NOTE: Ownership of item is transferred to vector if no `copy_init` method is set.
///       This is to prevent multiple ownership of same object, once inserted into vector.
///       Object won't be usable after this call if `copy_init` is not set.
///
/// INFO: If `copy_init` is set, then vector will create it's own copy of items.
///
/// v[in,out] : Vector to insert item into
/// lval[in]  : l-value to be inserted
/// idx[in]   : Index to insert item at.
///
/// SUCCESS : return
/// FAILURE : Does not return
///
#define VecInsertFastL(v, val, idx)                                                                                    \
    do {                                                                                                               \
        ValidateVec(v);                                                                                                \
        VEC_DATATYPE(v) *__ptr_val_##__LINE__ = &(val);                                                                \
        {                                                                                                              \
            VEC_DATATYPE(v) __tmp_val_##__LINE__ = (val);                                                              \
            (void)__tmp_val_##__LINE__;                                                                                \
        }                                                                                                              \
        insert_range_fast_into_vec(GENERIC_VEC(v), (char *)__ptr_val_##__LINE__, sizeof(VEC_DATATYPE(v)), (idx), 1);   \
        if (!(v)->copy_init) {                                                                                         \
            memset(__ptr_val_##__LINE__, 0, sizeof(VEC_DATATYPE(v)));                                                  \
        }                                                                                                              \
    } while (0)

///
/// Quickly insert item into vector. Ordering of elements is not guaranteed
/// to be preserved. This call makes significant difference only for sufficiently
/// large vectors and when `idx` is quite less than `(v)->length`.
///
/// Insertion time is guaranteed to be constant for same data types.
///
/// Usage is exactly same as `VecInsert`, just the internal implementation is
/// different.
///
/// v[in,out] : Vector to insert item into
/// lval[in]  : r-value to be inserted
/// idx[in]   : Index to insert item at.
///
/// SUCCESS : return
/// FAILURE : Does not return
///
#define VecInsertFastR(v, val, idx)                                                                                    \
    do {                                                                                                               \
        ValidateVec(v);                                                                                                \
        VEC_DATATYPE(v) *__ptr_val_##__LINE__ = &(val);                                                                \
        {                                                                                                              \
            VEC_DATATYPE(v) __tmp_val_##__LINE__ = (val);                                                              \
            (void)__tmp_val_##__LINE__;                                                                                \
        }                                                                                                              \
        insert_range_fast_into_vec(GENERIC_VEC(v), (char *)__ptr_val_##__LINE__, sizeof(VEC_DATATYPE(v)), (idx), 1);   \
        if (!(v)->copy_init) {                                                                                         \
            memset(__ptr_val_##__LINE__, 0, sizeof(VEC_DATATYPE(v)));                                                  \
        }                                                                                                              \
    } while (0)

///
/// By default this behaves like inserting an l-value using `VecInsertFastL`
///
/// v[in,out] : Vector to insert item into
/// lval[in]  : l-value to be inserted
/// idx[in]   : Index to insert item at.
///
#define VecInsertFast(v, lval, idx) VecInsertFastR((v), (lval), (idx))

///
/// Insert array of items into vector, with L-value semantics.
/// Insertion index must not exceed vector length.
/// This preserves the ordering of elements. Best to be used with sorted vectors,
/// if the sorted property is to be preserved.
///
/// NOTE: Ownership of items in array is transferred to vector if no `copy_init` method is set.
///       This is to prevent multiple ownership of same object, once inserted into vector.
///       Object won't be usable after this call if `copy_init` is not set.
///
/// INFO: If `copy_init` is set, then vector will create it's own copy of items.
///
/// v[in,out] : Vector to insert item into
/// val[in]   : Array of items to be inserted
/// idx[in]   : Index to start inserting item at.
/// count[in] : Number of items to insert.
///
/// SUCCESS : return
/// FAILURE : Does not return
///
#define VecInsertRangeL(v, varr, idx, count)                                                                           \
    do {                                                                                                               \
        ValidateVec(v);                                                                                                \
        {                                                                                                              \
            if (varr == NULL) {                                                                                        \
                LOG_FATAL("Expected a valid pointer");                                                                 \
            }                                                                                                          \
            VEC_DATATYPE(v) __tmp_val_##__LINE__ = *(varr);                                                            \
            (void)__tmp_val_##__LINE__;                                                                                \
        }                                                                                                              \
        VEC_DATATYPE(v) *__tmp_ptr_##__LINE__ = (varr);                                                                \
        insert_range_into_vec(GENERIC_VEC(v), (char *)__tmp_ptr_##__LINE__, sizeof(VEC_DATATYPE(v)), (idx), (count));  \
        if (!(v)->copy_init) {                                                                                         \
            memset((void *)(__tmp_ptr_##__LINE__), 0, (count) * sizeof(VEC_DATATYPE(v)));                              \
        }                                                                                                              \
    } while (0)

///
/// Insert array of items into vector, with R-value semantics.
/// Insertion index must not exceed vector length.
/// This preserves the ordering of elements. Best to be used with sorted vectors,
/// if the sorted property is to be preserved.
///
/// NOTE: Unlike VecInsertRangeL, this does NOT zero out the source array after insertion
///       regardless of copy_init settings. Use this for temporary arrays or when you need
///       to maintain the source array.
///
/// INFO: If `copy_init` is set, then vector will create it's own copy of items.
///
/// v[in,out] : Vector to insert item into
/// val[in]   : Array of items to be inserted
/// idx[in]   : Index to start inserting item at.
/// count[in] : Number of items to insert.
///
/// SUCCESS : return
/// FAILURE : Does not return
///
#define VecInsertRangeR(v, varr, idx, count)                                                                           \
    do {                                                                                                               \
        ValidateVec(v);                                                                                                \
        {                                                                                                              \
            if (varr == NULL) {                                                                                        \
                LOG_FATAL("Expected a valid pointer");                                                                 \
            }                                                                                                          \
            VEC_DATATYPE(v) __tmp_val_##__LINE__ = *(varr);                                                            \
            (void)__tmp_val_##__LINE__;                                                                                \
        }                                                                                                              \
        const VEC_DATATYPE(v) *__tmp_ptr_##__LINE__ = (varr);                                                          \
        insert_range_into_vec(GENERIC_VEC(v), (char *)__tmp_ptr_##__LINE__, sizeof(VEC_DATATYPE(v)), (idx), (count));  \
    } while (0)

///
/// Insert array of items into vector of it's type.
/// By default, this uses L-value semantics (ownership transfer).
/// Insertion index must not exceed vector length.
/// This preserves the ordering of elements. Best to be used with sorted vectors,
/// if the sorted property is to be preserved.
///
/// NOTE: Ownership of items in array is transferred to vector if no `copy_init` method is set.
///       This is to prevent multiple ownership of same object, once inserted into vector.
///       Object won't be usable after this call if `copy_init` is not set.
///
/// INFO: If `copy_init` is set, then vector will create it's own copy of items.
///
/// v[in,out] : Vector to insert item into
/// val[in]   : Array of items to be inserted
/// idx[in]   : Index to start inserting item at.
/// count[in] : Number of items to insert.
///
/// SUCCESS : return
/// FAILURE : Does not return
///
#define VecInsertRange(v, varr, idx, count) VecInsertRangeL((v), (varr), (idx), (count))

///
/// Quickly insert array of items into vector, with L-value semantics.
/// Ordering of elements is not guaranteed to be preserved.
/// This call makes significant difference only for sufficiently
/// large vectors and when `idx` is quite less than `(v)->length`.
///
/// Insertion time is guaranteed to be constant for same data types.
///
/// NOTE: Ownership of items in array is transferred to vector if no `copy_init` method is set.
///       This is to prevent multiple ownership of same object, once inserted into vector.
///       Object won't be usable after this call if `copy_init` is not set.
///
/// INFO: If `copy_init` is set, then vector will create it's own copy of items.
///
/// v[in,out] : Vector to insert item into
/// val[in]   : Array of items to be inserted
/// idx[in]   : Index to insert item at.
/// count[in] : Number of items to insert.
///
/// SUCCESS : return
/// FAILURE : Does not return
///
#define VecInsertRangeFastL(v, varr, idx, count)                                                                       \
    do {                                                                                                               \
        ValidateVec(v);                                                                                                \
        {                                                                                                              \
            if (varr == NULL) {                                                                                        \
                LOG_FATAL("Expected a valid pointer");                                                                 \
            }                                                                                                          \
            VEC_DATATYPE(v) __tmp_val_##__LINE__ = *(varr);                                                            \
            (void)__tmp_val_##__LINE__;                                                                                \
        }                                                                                                              \
        const VEC_DATATYPE(v) *__tmp_ptr_##__LINE__ = (varr);                                                          \
        insert_range_fast_into_vec(                                                                                    \
            GENERIC_VEC(v),                                                                                            \
            (char *)__tmp_ptr_##__LINE__,                                                                              \
            sizeof(VEC_DATATYPE(v)),                                                                                   \
            (idx),                                                                                                     \
            (count)                                                                                                    \
        );                                                                                                             \
        if (!(v)->copy_init) {                                                                                         \
            memset((void *)__tmp_ptr_##__LINE__, 0, (count) * sizeof(VEC_DATATYPE(v)));                                \
        }                                                                                                              \
    } while (0)

///
/// Quickly insert array of items into vector, with R-value semantics.
/// Ordering of elements is not guaranteed to be preserved.
/// This call makes significant difference only for sufficiently
/// large vectors and when `idx` is quite less than `(v)->length`.
///
/// Insertion time is guaranteed to be constant for same data types.
///
/// NOTE: Unlike VecInsertRangeFastL, this does NOT zero out the source array after insertion
///       regardless of copy_init settings. Use this for temporary arrays or when you need
///       to maintain the source array.
///
/// INFO: If `copy_init` is set, then vector will create it's own copy of items.
///
/// v[in,out] : Vector to insert item into
/// val[in]   : Array of items to be inserted
/// idx[in]   : Index to insert item at.
/// count[in] : Number of items to insert.
///
/// SUCCESS : return
/// FAILURE : Does not return
///
#define VecInsertRangeFastR(v, varr, idx, count)                                                                       \
    do {                                                                                                               \
        ValidateVec(v);                                                                                                \
        {                                                                                                              \
            if (varr == NULL) {                                                                                        \
                LOG_FATAL("Expected a valid pointer");                                                                 \
            }                                                                                                          \
            VEC_DATATYPE(v) __tmp_val_##__LINE__ = *(varr);                                                            \
            (void)__tmp_val_##__LINE__;                                                                                \
        }                                                                                                              \
        const VEC_DATATYPE(v) *__tmp_ptr_##__LINE__ = (varr);                                                          \
        insert_range_fast_into_vec(                                                                                    \
            GENERIC_VEC(v),                                                                                            \
            (char *)__tmp_ptr_##__LINE__,                                                                              \
            sizeof(VEC_DATATYPE(v)),                                                                                   \
            (idx),                                                                                                     \
            (count)                                                                                                    \
        );                                                                                                             \
    } while (0)

///
/// Quickly insert array of items into vector.
/// By default, this uses L-value semantics (ownership transfer).
/// Ordering of elements is not guaranteed to be preserved.
/// This call makes significant difference only for sufficiently
/// large vectors and when `idx` is quite less than `(v)->length`.
///
/// Insertion time is guaranteed to be constant for same data types.
///
/// NOTE: Ownership of items in array is transferred to vector if no `copy_init` method is set.
///       This is to prevent multiple ownership of same object, once inserted into vector.
///       Object won't be usable after this call if `copy_init` is not set.
///
/// INFO: If `copy_init` is set, then vector will create it's own copy of items.
///
/// v[in,out] : Vector to insert item into
/// val[in]   : Array of items to be inserted
/// idx[in]   : Index to insert item at.
/// count[in] : Number of items to insert.
///
/// SUCCESS : return
/// FAILURE : Does not return
///
#define VecInsertRangeFast(v, varr, idx, count) VecInsertRangeFastL((v), (varr), (idx), (count))

///
/// Push a complete array into this vector, with L-value semantics.
///
/// NOTE: Ownership trasfer takes place if vector is not creating it's own copy of items.
///
/// v[in,out] : Vector to insert array items into.
/// arr[in]   : Array to be inserted.
/// count[in] : Number (non-zero) of items in array.
///
/// SUCCESS : `v`
/// FAILURE : Does not return on failure
///
#define VecPushBackArrL(v, arr, count) VecInsertRangeL((v), (arr), (v)->length, (count))

///
/// Push a complete array into this vector, with R-value semantics.
///
/// NOTE: Unlike VecPushBackArrL, this does NOT zero out the source array after insertion.
///
/// v[in,out] : Vector to insert array items into.
/// arr[in]   : Array to be inserted.
/// count[in] : Number (non-zero) of items in array.
///
/// SUCCESS : `v`
/// FAILURE : Does not return on failure
///
#define VecPushBackArrR(v, arr, count) VecInsertRangeR((v), (arr), (v)->length, (count))

///
/// Push a complete array into this vector.
/// By default, this uses L-value semantics (ownership transfer).
///
/// NOTE: Ownership trasfer takes place if vector is not creating it's own copy of items.
///
/// v[in,out] : Vector to insert array items into.
/// arr[in]   : Array to be inserted.
/// count[in] : Number (non-zero) of items in array.
///
/// SUCCESS : `v`
/// FAILURE : Does not return on failure
///
#define VecPushBackArr(v, arr, count) VecPushBackArrL((v), (arr), (count))

///
/// Push a complete array into this vector front, with L-value semantics.
///
/// NOTE: Ownership trasfer takes place if vector is not creating it's own copy of items.
///
/// v[in,out] : Vector to insert array items into.
/// arr[in]   : Array to be inserted.
/// count[in] : Number (non-zero) of items in array.
///
/// SUCCESS : `v`
/// FAILURE : Does not return on failure
///
#define VecPushFrontArrL(v, arr, count) VecInsertRangeL((v), (arr), 0, (count))

///
/// Push a complete array into this vector front, with R-value semantics.
///
/// NOTE: Unlike VecPushFrontArrL, this does NOT zero out the source array after insertion.
///
/// v[in,out] : Vector to insert array items into.
/// arr[in]   : Array to be inserted.
/// count[in] : Number (non-zero) of items in array.
///
/// SUCCESS : `v`
/// FAILURE : Does not return on failure
///
#define VecPushFrontArrR(v, arr, count) VecInsertRangeR((v), (arr), 0, (count))

///
/// Push a complete array into this vector front.
/// By default, this uses L-value semantics (ownership transfer).
///
/// NOTE: Ownership trasfer takes place if vector is not creating it's own copy of items.
///
/// v[in,out] : Vector to insert array items into.
/// arr[in]   : Array to be inserted.
/// count[in] : Number (non-zero) of items in array.
///
/// SUCCESS : `v`
/// FAILURE : Does not return on failure
///
#define VecPushFrontArr(v, arr, count) VecPushFrontArrL((v), (arr), (count))

///
/// Push a complete array into this vector front without preserving the order
/// of elements in vector, with L-value semantics.
///
/// NOTE: Ownership trasfer takes place if vector is not creating it's own copy of items.
///
/// v[in,out] : Vector to insert array items into.
/// arr[in]   : Array to be inserted.
/// count[in] : Number (non-zero) of items in array.
///
/// SUCCESS : `v`
/// FAILURE : Does not return on failure
///
#define VecPushFrontArrFastL(v, arr, count) VecInsertRangeFastL((v), (arr), 0, (count))

///
/// Push a complete array into this vector front without preserving the order
/// of elements in vector, with R-value semantics.
///
/// NOTE: Unlike VecPushFrontArrFastL, this does NOT zero out the source array after insertion.
///
/// v[in,out] : Vector to insert array items into.
/// arr[in]   : Array to be inserted.
/// count[in] : Number (non-zero) of items in array.
///
/// SUCCESS : `v`
/// FAILURE : Does not return on failure
///
#define VecPushFrontArrFastR(v, arr, count) VecInsertRangeFastR((v), (arr), 0, (count))

///
/// Push a complete array into this vector front without preserving the order of elements
/// in vector. By default, this uses L-value semantics (ownership transfer).
///
/// NOTE: Ownership trasfer takes place if vector is not creating it's own copy of items.
///
/// v[in,out] : Vector to insert array items into.
/// arr[in]   : Array to be inserted.
/// count[in] : Number (non-zero) of items in array.
///
/// SUCCESS : `v`
/// FAILURE : Does not return on failure
///
#define VecPushFrontArrFast(v, arr, count) VecPushFrontArrFastL((v), (arr), (count))

///
/// Merge two vectors and store the result in the first vector, with L-value semantics.
/// Call to this makes sure, either both vectors have their own ownerships, or only one
/// owns the objects. Meaning none of the two lists share ownership.
///
/// NOTE: Ownership transfer takes place only if (v) does not create it's own copies of objects.
///       Vectors create their own copy only if `copy_init` method is provided, otherwise simple
///       `memcpy` is performed, which is the case where objects tend to share ownership, that this
///       method automatically resolves for you.
///
/// [in,out] v   : Destination vector that will receive data.
/// [in,out] v2  : Source vector to merge from and reset.
///
/// SUCCESS : `v`
/// FAILURE : Does not return on failure
///
#define VecMergeL(v, v2)                                                                                               \
    do {                                                                                                               \
        if ((v2)->data) {                                                                                              \
            VecPushBackArrL((v), (v2)->data, (v2)->length);                                                            \
            if (!(v)->copy_init && (v2)->data) {                                                                       \
                free((v2)->data);                                                                                      \
                (v2)->data     = NULL;                                                                                 \
                (v2)->length   = 0;                                                                                    \
                (v2)->capacity = 0;                                                                                    \
            }                                                                                                          \
        }                                                                                                              \
    } while (0)

///
/// Merge two vectors and store the result in the first vector, with R-value semantics.
///
/// Data is copied from `v2` into `v`. If a `copy_init` method is provided in `v`,
/// each element from `v2` will be copied using that method. Otherwise, a raw memory
/// copy is performed, which may be unsafe for complex or pointer-containing data.
///
/// NOTE: Unlike VecMergeL, this does NOT zero out the source vector's data after merging.
///
/// The `copy_init` function must be set in `v` if ownership-safe copies are needed.
///
/// [in,out] v   : Destination vector that will receive data.
/// [in]     v2  : Source vector to merge from.
///
/// SUCCESS : `v`
/// FAILURE : Does not return on failure
///
#define VecMergeR(v, v2)                                                                                               \
    do {                                                                                                               \
        if ((v2)->data) {                                                                                              \
            VecPushBackArrR((v), (v2)->data, (v2)->length);                                                            \
        }                                                                                                              \
    } while (0)

///
/// Merge two vectors and store the result in the first vector.
///
/// Data is copied from `v2` into `v`. If a `copy_init` method is provided in `v`,
/// each element from `v2` will be copied using that method. Otherwise, a raw memory
/// copy is performed, which may be unsafe for complex or pointer-containing data.
///
/// NOTE: This function (via VecMergeL) completely transfers ownership from `v2` to `v` by:
///       1. Adding all elements from `v2` to `v`
///       2. Freeing the memory allocated for `v2->data`
///       3. Resetting all fields of `v2` to zero using MemSet
///
/// After this operation, `v2` will be in a reset state (as if just initialized with VecInit).
/// If you want to preserve the source vector, use VecMergeR instead.
///
/// The `copy_init` function must be set in `v` if ownership-safe copies are needed.
///
/// [in,out] v   : Destination vector that will receive data.
/// [in,out] v2  : Source vector to merge from and reset.
///
/// SUCCESS : `v`
/// FAILURE : Does not return on failure
///
#define VecMerge(v, v2) VecMergeL((v), (v2))

///
/// Push an l-value into vector back.
///
/// NOTE: Ownership trasfer takes place if vector is not creating it's own copy of item.
///
/// v[in,out] : Vector to push item into
/// lval[in]  : l-value to be pushed back
///
/// SUCCESS : return
/// FAILURE : Does not return
///
#define VecPushBackL(v, val) VecInsertL((v), (val), (v)->length)

///
/// Push item into vector back.
///
/// v[in,out] : Vector to push item into
/// rval[in]  : r-value to be pushed back
///
/// SUCCESS : return
/// FAILURE : Does not return
///
#define VecPushBackR(v, val) VecInsertR((v), (val), (v)->length)

///
/// Push item into vector back.
///
/// Default behaviour is same as `VecPushBackL`
///
/// v[in,out] : Vector to push item into
/// lval[in]  : l-value to be pushed back
///
/// SUCCESS : return
/// FAILURE : Does not return
///
#define VecPushBack(v, val) VecInsert((v), (val), (v)->length)

///
/// Push item into vector front.
///
/// NOTE: Ownership trasfer takes place if vector is not creating it's own copy of item.
///
/// v[in,out] : Vector to push item into
/// lval[in]  : l-value to be inserted
///
/// SUCCESS : return
/// FAILURE : Does not return
///
#define VecPushFrontL(v, val) VecInsertL((v), (val), 0)

///
/// Push item into vector front.
///
/// v[in,out] : Vector to push item into
/// rval[in]  : r-value to be inserted
///
/// SUCCESS : return
/// FAILURE : Does not return
///
#define VecPushFrontR(v, val) VecInsertR((v), (val), 0)

///
/// Push item into vector front.
///
/// NOTE: default behavior is same as inserting an l-value using `VecPushBackL`
///
/// v[in,out] : Vector to push item into
/// rval[in]  : r-value to be inserted
///
/// SUCCESS : return
/// FAILURE : Does not return
///
#define VecPushFront(v, val) VecPushBackL((v), (val))

///
/// Initialize clone of vector from `vs` to `vd`.
///
/// NOTE: Ownership trasfer takes place if vector is not creating it's own copy of item.
///
/// vd[out] : Destination vector to create clone into
/// vs[in]  : Source vector to create clone using.
///
/// SUCCESS : `vd`
/// FAILURE : Does not return on failure
///
#define VecInitClone(vd, vs)                                                                                           \
    do {                                                                                                               \
        VecDeinit(vd);                                                                                                 \
        VecMerge(vd, vs);                                                                                              \
    } while (0)

#endif // MISRA_STD_CONTAINER_VEC_INSERT_H
