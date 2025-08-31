/// file      : Insert.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// copyright : Copyright (c) 2025, Siddharth Mishra, All rights reserved.
///
/// List insertion helpers.
///

#ifndef MISRA_STD_CONTAINER_LIST_INSERT_H
#define MISRA_STD_CONTAINER_LIST_INSERT_H

///
/// Insert an `l-value` into list of it's type.
/// Insertion index must not exceed list length.
///
/// NOTE: Ownership of item is transferred to list if no `copy_init` method is set.
///       This is to prevent multiple ownership of same object, once inserted into list.
///       Object may not be usable after this call if `copy_init` is not set.
///
/// INFO: If `copy_init` is set, then vector will create it's own copy of items.
/// INFO: If `copy_init` is not set, then provided l-value will be memset to 0 to keep unique ownership.
///
/// l[in,out] : List to insert item into
/// lval[in]  : l-value to be inserted
/// idx[in]   : Index to insert item at.
///
/// USAGE:
///   // the data
///   int x = 10;
///   int y = 20;
///
///   // vector
///   List(int) integers = ListInit();
///
///   // insert items
///   ListInsertL(&integers, x, 0); // x = 0 now, unlike x = 10 in ListInsertR
///   ListInsertL(&integers, y, 0); // y = 0 now, unlike y = 20 in ListInsertR
///   ListInsertL(&integers, LVAL(101), 1); // took a r-value, and converted to l-value on spot
///   // better use ListInsertR in this last case though, otherwise there'll be an extra useless function call
///
/// SUCCESS: return
/// FAILURE: Does not return
///
#define ListInsertL(l, lval, idx)                                                                                      \
    do {                                                                                                               \
        ValidateList(l);                                                                                               \
        LIST_DATA_TYPE(l) *__ptr__val_##__LINE__ = &(lval);                                                            \
        LIST_DATA_TYPE(l) __tmp__val_##__LINE__  = (lval);                                                             \
        insert_into_list(GENERIC_LIST(l), __tmp__val_##__LINE__, sizeof(LIST_DATA_TYPE(l)), (idx));                    \
        if (!(l)->copy_init) {                                                                                         \
            memset(__ptr__val_##__LINE__, 0, sizeof(LIST_DATA_TYPE(l)));                                               \
        }                                                                                                              \
    } while (0)


///
/// Insert an `r-value` into list of it's type.
/// Insertion index must not exceed list length.
///
/// l[in,out] : List to insert item into
/// rval[in]  : r-value to be inserted
/// idx[in]   : Index to insert item at.
///
/// USAGE:
///   // the data
///   int x = 10;
///   int y = 20;
///
///   // vector
///   List(int) integers = VecInit();
///
///   // insert items
///   ListInsertR(&integers, x, 0); // x remains 10, unlike 0 in ListInsertL, two copies of x
///   ListInsertR(&integers, y, 0); // y remains 20, unlike 0 in ListInsertL, two copies of y
///   ListInsertR(&integers, 5, 1);
///
/// SUCCESS: return
/// FAILURE: Does not return
///
#define ListInsertR(l, rval, idx)                                                                                      \
    do {                                                                                                               \
        ValidateList(l);                                                                                               \
        LIST_DATA_TYPE(l) __tmp__val_##__LINE__ = (rval);                                                              \
        insert_range_into_vec(GENERIC_LIST(l), (char *)&__tmp__val_##__LINE__, sizeof(LIST_DATA_TYPE(l)), (idx), 1);   \
    } while (0)

///
/// Insert an l-value into given list
///
/// NOTE: Ownership transfer takes place. Provided l-val memset to 0.
///
/// l[in,out] : List to insert item into
/// lval[in]  : l-value to be inserted
/// idx[in]   : Index to insert item at.
///
/// SUCCESS: return
/// FAILURE: Does not return
///
#define ListInsert(l, lval, idx) ListInsertL((l), (lval), (idx))

///
/// Insert a l-value at the very beginning of list.
///
/// NOTE: Ownership transfer takes place. Provided l-val memset to 0.
///
/// l[in]    : List to push item into.
/// lval[in] : Value (l-value) to be prepended.
///
/// SUCCESS: Return.
/// FAILURE: Does not return.
///
#define ListPushFrontL(l, lval) ListInsertL((l), (lval), 0);

///
/// Push a l-value at the back of list.
///
/// NOTE: Ownership transfer takes place. Provided l-val memset to 0.
///
/// l[in]   : List to push item into
/// val[in] : Pointer to value to be pushed
///
/// SUCCESS: Return
/// FAILURE: Does not return
///
#define ListPushBackL(l, lval) ListInsertL((l), (lval), (l)->length)

///
/// Insert a r-value at the very beginning of list.
///
/// l[in]    : List to push item into.
/// rval[in] : Value (r-value) to be prepended.
///
/// SUCCESS: Return.
/// FAILURE: Does not return.
///
#define ListPushFrontR(l, rval) ListInsertR((l), (rval), 0);

///
/// Push a r-value at the back of list.
///
/// l[in]    : List to push item into
/// rval[in] : Value to be pushed
///
/// SUCCESS: Returns `v` the list itself on success.
/// FAILURE: Returns `NULL` otherwise.
///
#define ListPushBackR(l, rval) ListInsertR((l), (rval), (l)->length)

///
/// Insert a l-value at the very beginning of list.
///
/// NOTE: Ownership transfer takes place. Provided l-val memset to 0.
///
/// l[in]    : List to push item into.
/// lval[in] : Value (l-value) to be prepended.
///
/// SUCCESS: Return.
/// FAILURE: Does not return.
///
#define ListPushFront(l, lval) ListPushFrontL((l), (lval), 0);

///
/// Push a l-value at the back of list.
///
/// NOTE: Ownership transfer takes place. Provided l-val memset to 0.
///
/// l[in]    : List to push item into
/// lval[in] : Pointer to value to be pushed
///
/// SUCCESS: Return
/// FAILURE: Does not return
///
#define ListPushBack(l, lval) ListPushBackL((l), (lval), (l)->length)

///
/// Push a complete array into this list.
///
/// NOTE: If provided list `l` does not make it's own copy of items, the provided array `arr`
///       will be memset to 0, to keep single ownership.
///
/// l[in,out] : List to insert array items into.
/// arr[in]   : Array to be inserted.
/// count[in] : Number (non-zero) of items in array.
///
/// SUCCESS: Return.
/// FAILURE: Do not return.
///
#define ListPushArrL(l, arr, count)                                                                                    \
    do {                                                                                                               \
        ValidateList(l);                                                                                               \
        LIST_DATA_TYPE(l) *__ptr_val_##__LINE__      = (arr);                                                          \
        const LIST_DATA_TYPE(l) __tmp_val_##__LINE__ = *(arr);                                                         \
        (void)__tmp_val_##__LINE__;                                                                                    \
        push_arr_list(GENERIC_LIST(l), sizeof(LIST_DATA_TYPE(l)), __ptr_val_##__LINE__, (count));                      \
        if (!(l)->copy_init) {                                                                                         \
            memset(__ptr_val_##__LINE__, 0, sizeof(LIST_DATA_TYPE(l)));                                                \
        }                                                                                                              \
    } while (0)

///
/// Merge two lists and store the result in first list.
///
/// NOTE : Merge list `l2` with `l` with r-value semantics.
///
/// l[in,out] : List to insert array items into.
/// l2[in]    : Array to be inserted.
///
/// SUCCESS: Return
/// FAILURE: Do not return.
///
#define ListMergeL(l, l2)                                                                                              \
    do {                                                                                                               \
        ValidateList(l);                                                                                               \
        ValidateList(l2);                                                                                              \
        {                                                                                                              \
            LIST_DATA_TYPE(l) __tmp1_##__LINE__  = {0};                                                                \
            LIST_DATA_TYPE(l2) __tmp2_##__LINE__ = {0};                                                                \
            __tmp1_##__LINE__                    = __tmp2_##__LINE__;                                                  \
            (void)__tmp1_##__LINE__;                                                                                   \
            (void)__tmp2_##__LINE__;                                                                                   \
        }                                                                                                              \
        merge_list(GENERIC_LIST(l), sizeof(LIST_DATA_TYPE(l)), GENERIC_LIST(l2));                                      \
        if (!(l)->copy_init) {                                                                                         \
            GenericCopyDeinit cd = (l2)->copy_deinit;                                                                  \
            (l2)->copy_deinit    = NULL;                                                                               \
            ListDeinit(l2);                                                                                            \
            (l2)->copy_deinit = cd;                                                                                    \
        }                                                                                                              \
    } while (0)

///
/// Merge two lists and store the result in first list.
///
/// NOTE : Merge list `l2` with `l` with r-value semantics.
///
/// l[in,out] : List to insert array items into.
/// l2[in]    : Array to be inserted.
///
/// SUCCESS: Return.
/// FAILURE: Do not return.
///
#define ListMergeR(l, l2)                                                                                              \
    do {                                                                                                               \
        ValidateList(l);                                                                                               \
        ValidateList(l2);                                                                                              \
        {                                                                                                              \
            LIST_DATA_TYPE(l) __tmp1_##__LINE__  = {0};                                                                \
            LIST_DATA_TYPE(l2) __tmp2_##__LINE__ = {0};                                                                \
            __tmp1_##__LINE__                    = __tmp2_##__LINE__;                                                  \
            (void)__tmp1_##__LINE__;                                                                                   \
            (void)__tmp2_##__LINE__;                                                                                   \
        }                                                                                                              \
        merge_list(GENERIC_LIST(l), sizeof(LIST_DATA_TYPE(l)), GENERIC_LIST(l2));                                      \
    } while (0)

///
/// Merge two lists and store the result in first list.
///
/// NOTE : Merge list `l2` with `l` with r-value semantics.
///
/// l[in,out] : List to insert array items into.
/// l2[in]    : Array to be inserted.
///
/// SUCCESS: Return
/// FAILURE: Do not return.
///
#define ListMerge(l, l2) ListMergeL((l), (l2))

#endif // MISRA_STD_CONTAINER_LIST_INSERT_H
