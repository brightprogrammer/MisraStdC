/// file      : std/container/vec/insert.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// copyright : Copyright (c) 2025, Siddharth Mishra, All rights reserved.
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
///       Object won't be usable after this call if `copy_init` is not set.
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
///   VecInsertL(&integers, &x, 0); // x inserted at position 0
///   VecInsertL(&integers, &y, 0); // x shifted one position and y is inserted
///   VecInsertL(&integers, LVAL(101), 1); // x shifted one position and 101 is inserted at index 1
///
/// SUCCESS : return
/// FAILURE : Does not return
///
#define VecInsertL(v, lval, idx)                                                                                       \
    do {                                                                                                               \
        VEC_DATATYPE(v) __tmp__val = (lval);                                                                           \
        insert_range_into_vec(GENERIC_VEC(v), (char *)&__tmp__val, sizeof(VEC_DATATYPE(v)), (idx), 1);                 \
        if (!(v)->copy_init) {                                                                                         \
            memset(&(lval), 0, sizeof(lval));                                                                          \
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
///   VecInsert(&integers, &x, 0); // x inserted at position 0
///   VecInsert(&integers, &y, 0); // x shifted one position and y is inserted
///   VecInsert(&integers, ((int[]){5}), 1); // x shifted one position and 5 is inserted at index 1
///
/// SUCCESS : return
/// FAILURE : Does not return
///
#define VecInsertR(v, rval, idx)                                                                                       \
    do {                                                                                                               \
        VEC_DATATYPE(v) __tmp__val = (rval);                                                                           \
        insert_range_into_vec(GENERIC_VEC(v), (char *)&__tmp__val, sizeof(VEC_DATATYPE(v)), (idx), 1);                 \
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
        VEC_DATATYPE(v) __tmp__val = (val);                                                                            \
        insert_range_fast_into_vec(GENERIC_VEC(v), (char *)&__tmp__val, sizeof(VEC_DATATYPE(v)), (idx), 1);            \
        if (!(v).copy_init) {                                                                                          \
            memset(&(val), 0, sizeof(val));                                                                            \
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
        VEC_DATATYPE(v) __tmp__val = (val);                                                                            \
        insert_range_fast_into_vec(GENERIC_VEC(v), (char *)&__tmp__val, sizeof(VEC_DATATYPE(v)), (idx), 1);            \
        if (!(v).copy_init) {                                                                                          \
            memset(&(val), 0, sizeof(val));                                                                            \
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
/// Insert item into vector of it's type.
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
#define VecInsertRange(v, varr, idx, count)                                                                            \
    do {                                                                                                               \
        {                                                                                                              \
            if (!varr) {                                                                                               \
                LOG_FATAL("Expected a valid pointer");                                                                 \
            }                                                                                                          \
            const VEC_DATATYPE(v) __x = *(varr);                                                                       \
            (void)__x;                                                                                                 \
        }                                                                                                              \
        const VEC_DATATYPE(v) *__tmp__ptr = (varr);                                                                    \
        insert_range_into_vec(GENERIC_VEC(v), (char *)__tmp__ptr, sizeof(VEC_DATATYPE(v)), (idx), (count));            \
        if (!(v)->copy_init) {                                                                                         \
            memset((void *)(varr), 0, (count) * sizeof(*varr));                                                        \
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
/// NOTE: Ownership of items in array is transferred to vector if no `copy_init` method is set.
///       This is to prevent multiple ownership of same object, once inserted into vector.
///       Object won't be usable after this call if `copy_init` is not set.
///
/// INFO: If `copy_init` is set, then vector will create it's own copy of items.
///
/// v[in,out] : Vector to insert item into
/// val[in]   : Value to be inserted
/// idx[in]   : Index to insert item at.
///
/// SUCCESS : return
/// FAILURE : Does not return
///
#define VecInsertRangeFast(v, varr, idx, count)                                                                        \
    do {                                                                                                               \
        {                                                                                                              \
            if (!varr) {                                                                                               \
                LOG_FATAL("Expected a valid pointer");                                                                 \
            }                                                                                                          \
            const VEC_DATATYPE(v) __x = *(varr);                                                                       \
            (void)__x;                                                                                                 \
        }                                                                                                              \
        const VEC_DATATYPE(v) *__tmp__ptr = (varr);                                                                    \
        insert_range_fast_into_vec(GENERIC_VEC(v), (char *)__tmp__ptr, sizeof(VEC_DATATYPE(v)), (idx), (count));       \
        if (!(v)->copy_init) {                                                                                         \
            memset((void *)(varr), 0, (count) * sizeof(*varr));                                                        \
        }                                                                                                              \
    } while (0)

///
/// Push a complete array into this vector.
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
#define VecPushBackArr(v, arr, count) VecInsertRange((v), (arr), (v)->length, (count))

///
/// Push a complete array into this vector.
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
#define VecPushFrontArr(v, arr, count) VecInsertRange((v), (arr), 0, (count))

///
/// Push a complete array into this vector without preserving the order of elements
/// in vector.
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
#define VecPushFrontArrFast(v, arr, count) VecInsertRangeFast((v), (arr), 0, (count))

///
/// Merge two vectors and store the result in the first vector.
///
/// Data is copied from `v2` into `v`. If a `copy_init` method is provided in `v`,
/// each element from `v2` will be copied using that method. Otherwise, a raw memory
/// copy is performed, which may be unsafe for complex or pointer-containing data.
///
/// NOTE: Ownership trasfer takes place if vector is not creating it's own copy of items.
///
/// The `copy_init` function must be set in `v` if ownership-safe copies are needed.
///
/// [in,out] v   : Destination vector that will receive data.
/// [in]     v2  : Source vector to merge from.
///
/// SUCCESS : `v`
/// FAILURE : Does not return on failure
///
#define VecMerge(v, v2) VecPushBackArr((v), (v2)->data, (v2)->length)

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
#define VecInitClone(vd, vs) (VecDeinit(vd), VecMerge(vd, vs))

#endif // MISRA_STD_CONTAINER_VEC_INSERT_H
