/// file      : std/container/vec.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// copyright : Copyright (c) 2025, Siddharth Mishra, All rights reserved.
///
/// Provides a type-safe vector implementation in C

#ifndef MISRA_STD_CONTAINER_VEC_H
#define MISRA_STD_CONTAINER_VEC_H

#include <Misra/Std/Container/Common.h>
#include <Misra/Types.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    size              length;
    size              capacity;
    GenericCopyInit   copy_init;
    GenericCopyDeinit copy_deinit;
    char             *data;
    size              alignment;
} GenericVec;

#define GENERIC_VEC(x) ((GenericVec *)(void *)(x))

///
/// Typesafe vector definition.
/// This is much like C++ template std::vector<T>
///
/// NOTE: Using this directly like `Vec(T)` won't always work,
///       because each time this is used it defines a new type, and two
///       `Vec(T)`s are different from each other.
///
///       To deal with this, you must typedef vector for a specific type.
///       Throughout the code, any time that is in plural form is generally
///       a vector. Like `Strs` is a typedef of `Vec(Str)`.
///
/// USAGE:
///   Vec(int) integers; // Vector of integers
///   Vec(CustomStruct) my_data; // Vector of CustomStruct
///   Vec(float) real_numbers; // Vector of float values
///   Vec(const char*) names; Vector of c-style null-terminated strings
///
/// FIELDS:
/// - length      : Number of items currently in vector (always <= capacity)
/// - capacity    : Max number of items this vector can hold before doing a resize.
/// - copy_init   : If provided then is used to create owned copies of items into vector.
/// - copy_deinit : If provided then is used to deinit data held by vector.
///                 Caution when dealing with shared ownership.
/// - data        : Data held by vector. Don't access by direct indexing. Use `VecAt(..)`
/// - alignment   : Alignment requirement for each item in vector.
///
/// TAGS: Vec, Generic, Length, Size, Aligned, Pointer
///
#define Vec(T)                                                                                                         \
    struct {                                                                                                           \
        size              length;                                                                                      \
        size              capacity;                                                                                    \
        GenericCopyInit   copy_init;                                                                                   \
        GenericCopyDeinit copy_deinit;                                                                                 \
        T                *data;                                                                                        \
        size              alignment;                                                                                   \
    }

#define VEC_DATA_TYPE(v) __typeof__((v)->data[0])

///
/// Initialize vector. Default alignment is 1
/// It is mandatory to initialize vectors before use. Not doing so is undefined behaviour.
///
/// USAGE:
///   Vec(HttpRequest) requests = VecInit();
///
/// TAGS: Init, Vec, Length, Size, Aligned
///
#define VecInit()                                                                                                      \
    {.length      = 0,                                                                                                 \
     .capacity    = 0,                                                                                                 \
     .copy_init   = (GenericCopyInit)NULL,                                                                             \
     .copy_deinit = (GenericCopyDeinit)NULL,                                                                           \
     .data        = NULL,                                                                                              \
     .alignment   = 1}

///
/// Initialize given vector. Default alignment is 1
/// It is mandatory to initialize vectors before use. Not doing so is undefined behaviour.
///
/// v[in] : Pointer to type of a vector to be initialized.
///
/// USAGE:
///     void SomeInterestingFn(DataVec* data_vec) {
///         *data_vec = VecInit_T(data);
///
///         // use vector
///     }
///
/// TAGS: Init, Vec, Length, Size, Aligned
///
#define VecInit_T(v)                                                                                                   \
    ((__typeof__(*v)) {.length      = 0,                                                                               \
                       .capacity    = 0,                                                                               \
                       .copy_init   = (GenericCopyInit)NULL,                                                           \
                       .copy_deinit = (GenericCopyDeinit)NULL,                                                         \
                       .data        = NULL,                                                                            \
                       .alignment   = 1})

///
/// Initialize vector. Default alignment is 1
/// It is mandatory to initialize vectors before use. Not doing so is undefined behaviour.
///
/// ci[in]   : Copy init method.
/// cd[in]   : Copy deinit method.
///
/// USAGE:
///   Vec(HttpRequest) requests = VecInitWithDeepCopy(RequestClone, RequestDeinit);
///
/// TAGS: Init, Vec, Length, Size, Aligned, DeepCopy, DeepDeinit
///
#define VecInitWithDeepCopy(ci, cd)                                                                                    \
    {.length      = 0,                                                                                                 \
     .capacity    = 0,                                                                                                 \
     .copy_init   = (GenericCopyInit)(ci),                                                                             \
     .copy_deinit = (GenericCopyDeinit)(cd),                                                                           \
     .data        = NULL,                                                                                              \
     .alignment   = 1}

///
/// Initialize given vector. Default alignment is 1
/// It is mandatory to initialize vectors before use. Not doing so is undefined behaviour.
///
/// v[in]  : Pointer to type of a vector to be initalized.
/// ci[in] : Copy init method.
/// cd[in] : Copy deinit method.
///
/// USAGE:
///     bool DataInitClone(Data* dst, Data* src) { /* cloning logic...*/ }
///     void DataDeinit(Data* d) { /* deinit logic... don't free "d" */ }
///
///     void SomeInterestingFn(DataVec* data_vec) {
///         *data_vec = VecInitWithDeepCopy_T(data, DataInitClone, DataDeinit);
///
///         // use vector
///     }
///
/// TAGS: Init, Vec, Length, Size, Aligned, DeepCopy, DeepDeinit
///
#define VecInitWithDeepCopy_T(v, ci, cd)                                                                               \
    ((__typeof__(*v)) {.length      = 0,                                                                               \
                       .capacity    = 0,                                                                               \
                       .copy_init   = (GenericCopyInit)(ci),                                                           \
                       .copy_deinit = (GenericCopyDeinit)(cd),                                                         \
                       .data        = NULL,                                                                            \
                       .alignment   = 1})

///
/// Initialize vector with given alignment.
/// It is mandatory to initialize vectors before use. Not doing so is undefined behaviour.
///
/// Provided alignment is used to keep all objects at an aligned memory location,
/// avoiding UB in some cases. It's recommended to use aligned vector when dealing with
/// structs containing unions.
///
/// aln[in]   : Vector element alignment. All items will be stored by respecting the
///             alignment boundary.
///
/// USAGE:
///   Vec(Node) nodes = VecInitAligned(16);
///
/// TAGS: Init, Vec, Length, Size, Aligned
///
#define VecInitAligned(aln)                                                                                            \
    {.length      = 0,                                                                                                 \
     .capacity    = 0,                                                                                                 \
     .copy_init   = (GenericCopyInit)NULL,                                                                             \
     .copy_deinit = (GenericCopyDeinit)NULL,                                                                           \
     .data        = NULL,                                                                                              \
     .alignment   = (aln)}

///
/// Initialize given vector with given alignment.
/// It is mandatory to initialize vectors before use. Not doing so is undefined behaviour.
///
/// Provided alignment is used to keep all objects at an aligned memory location,
/// avoiding UB in some cases. It's recommended to use aligned vector when dealing with
/// structs containing unions.
///
/// v[in]   : Pointer to type of a vector to be initalized.
/// aln[in] : Vector element alignment. All items will be stored by respecting the
///             alignment boundary.
///
/// USAGE:
///     Vec(Node) nodes = VecInitAligned(16);
///
///     void SomeInterestingFn(DataVec* data_vec) {
///         // align items in "data_vec" at 128 byte boundaries
///         *data_vec = VecInitAligned_T(data, 128);
///
///         // use vector
///     }
///
/// TAGS: Init, Vec, Length, Size, Aligned
///
#define VecInitAligned_T(v, aln)                                                                                       \
    ((__typeof__(*v)) {.length      = 0,                                                                               \
                       .capacity    = 0,                                                                               \
                       .copy_init   = (GenericCopyInit)NULL,                                                           \
                       .copy_deinit = (GenericCopyDeinit)NULL,                                                         \
                       .data        = NULL,                                                                            \
                       .alignment   = (aln)})

///
/// Initialize vector with given alignment.
/// It is mandatory to initialize vectors before use. Not doing so is undefined behaviour.
///
/// Provided alignment is used to keep all objects at an aligned memory location,
/// avoiding UB in some cases. It's recommended to use aligned vector when dealing with
/// structs containing unions.
///
/// ci[in]   : Copy init method.
/// cd[in]   : Copy deinit method.
/// aln[in]   : Vector element alignment. All items will be stored by respecting the
///             alignment boundary.
///
/// USAGE:
///   typedef Vec(Node) NodeVec;
///   NodeVec nodes = VecInitAligned(NodeInitCopy, NodeDeinit, 48);
///
/// TAGS: Init, Vec, Length, Size, Aligned, DeepCopy, DeepDeinit
///
#define VecInitAlignedWithDeepCopy(ci, cd, aln)                                                                        \
    {.length      = 0,                                                                                                 \
     .capacity    = 0,                                                                                                 \
     .copy_init   = (GenericCopyInit)(ci),                                                                             \
     .copy_deinit = (GenericCopyDeinit)(cd),                                                                           \
     .data        = NULL,                                                                                              \
     .alignment   = (aln)}

///
/// Initialize given vector with given alignment.
/// It is mandatory to initialize vectors before use. Not doing so is undefined behaviour.
///
/// Provided alignment is used to keep all objects at an aligned memory location,
/// avoiding UB in some cases. It's recommended to use aligned vector when dealing with
/// structs containing unions.
///
/// v[in]   : Pointer to type of a vector to be initalized.
/// ci[in]   : Copy init method.
/// cd[in]   : Copy deinit method.
/// aln[in] : Vector element alignment. All items will be stored by respecting the
///             alignment boundary.
///
/// USAGE:
///     bool DataInitClone(Data* dst, Data* src) { /* cloning logic...*/ }
///     void DataDeinit(Data* d) { /* deinit logic... don't free "d" */ }
///
///     void SomeInterestingFn(DataVec* data_vec) {
///         // align and store all items in vector at 32 byte boundaries
///         *data_vec = VecInitAlignedWithDeepCopy_T(data, DataInitClone, DataDeinit, 32);
///
///         // use vector
///         // always use VecAt to access elements in vector
///         Data i1= VecAt(data_vec, 9); // get 10th item (index = 9)
///         Data i2 = VecLast(data_vec); // get last item (index = whatever)
///     }
///
/// TAGS: Init, Vec, Length, Size, Aligned, DeepCopy, DeepDeinit
///
#define VecInitAlignedWithDeepCopy_T(v, ci, cd, aln)                                                                   \
    ((__typeof__(*v)) {.length      = 0,                                                                               \
                       .capacity    = 0,                                                                               \
                       .copy_init   = (GenericCopyInit)(ci),                                                           \
                       .copy_deinit = (GenericCopyDeinit)(cd),                                                         \
                       .data        = NULL,                                                                            \
                       .alignment   = (aln)})

///
/// Initialize given vector using memory from stack.
/// Such vectors cannot be dynamically resized. Doing so is UB.
/// It is mandatory to initialize vectors before use. Not doing so is undefined behaviour.
///
/// These vectors are best used where user doesn't get a chance to or does not want
/// to deinit vector, given that no data in vector needs to be deinitialized.
/// Example includes, but does not limit to a Vec(i8), Vec(f32), etc...
///
/// Stack inited vectors mustn't be deinited after use.
///
/// v[in,out] : Pointer to vector memory that needs to be initialized.
/// ne[in]    : Number of elements to allocate stack memory for.
///
/// USAGE:
///   Vec(i32) ids;
///   VecInitStack(&ids, 64, {
///       // scope where vector memory is available
///       MakeClientRequestToFillVector(&ids);
///       VecForeach(&ids, id, {
///           // some relevant logic
///       });
///
///       // Do not call deinit after use!!
///   });
///
/// TAGS: Init, Vec, Stack, Length, Size, Array
///
#define VecInitStack(v, ne, scoped_body)                                                                               \
    do {                                                                                                               \
        VEC_DATA_TYPE(v) ___data___[(ne) + 1] = {0};                                                                   \
                                                                                                                       \
        *(v)          = (__typeof__(*v))VecInit();                                                                     \
        (v)->capacity = (ne);                                                                                          \
        (v)->data     = &___data___[0];                                                                                \
                                                                                                                       \
        {scoped_body}                                                                                                  \
                                                                                                                       \
        memset(___data___, 0, sizeof(___data___));                                                                     \
        memset(v, 0, sizeof(*v));                                                                                      \
    } while (0)

///
/// Initialize given vector with given alignment.
/// It is mandatory to initialize vectors before use. Not doing so is undefined behaviour.
///
/// Provided alignment is used to keep all objects at an aligned memory location,
/// avoiding UB in some cases. It's recommended to use aligned vector when dealing with
/// structs containing unions.
///
/// These vectors are best used where user doesn't get a chance to or does not want
/// to deinit vector, given that no data in vector needs to be deinitialized.
/// Example includes, but does not limit to a Vec(i8), Vec(f32), etc...
///
/// v[in,out] : Pointer to vector memory that needs to be initialized.
/// ne[in]    : Number of elements to allocate aligned stack memory for.
/// aln[in]   : Alignment value to align all emenets to.
///
/// USAGE:
///   Vec(Node*) nodes;
///
///   // initialize vector with stack memory, aligned with 124 byte boundary
///   VecInitAlignedStack(&nodes, 24, 124, {
///       // scope where vector memory is available
///       FindAndFillAllNodes(&nodes, ... /* some other relevant data */);
///
///       UseNodes(&nodes);
///
///       VecForeach(&nodes, node, {
///           DestroyNode(node);
///       });
///
///       // vector deinit will be called for you after this automatically
///       // so any data held by the vector in this scope is invalid outside
///   });
///
/// TAGS: Init, Vec, Stack, Aligned, Length, Size, Array
///
#define VecInitAlignedStack(v, ne, aln, scoped_body)                                                                   \
    do {                                                                                                               \
        char ___data___[ALIGN_UP(sizeof(VEC_DATA_TYPE(v)), (aln)) * ((ne) + 1)] = {0};                                 \
                                                                                                                       \
        *(v)          = (__typeof__(*v))VecInitAligned((aln));                                                         \
        (v)->capacity = (ne);                                                                                          \
        (v)->data     = (VEC_DATA_TYPE(v) *)&___data___[0];                                                            \
                                                                                                                       \
        {scoped_body}                                                                                                  \
                                                                                                                       \
        memset(&___data___[0], 0, sizeof(___data___));                                                                 \
        memset(v, 0, sizeof(*v));                                                                                      \
    } while (0)

///
/// Initialize given vector using memory from stack.
/// Such vectors cannot be dynamically resized. Doing so is UB.
/// It is mandatory to initialize vectors before use. Not doing so is undefined behaviour.
///
/// These vectors are best used where user doesn't get a chance to or does not want
/// to deinit vector, given that no data in vector needs to be deinitialized.
/// Example includes, but does not limit to a Vec(i8), Vec(f32), etc...
///
/// Stack inited vectors mustn't be deinited after use.
///
/// v[in,out] : Pointer to vector memory that needs to be initialized.
/// ne[in]    : Number of elements to allocate stack memory for.
/// ci[in]    : Copy init method for copying over elements in vector.
/// cd[in]    : Copy deinit method for deiniting elements in vector.
///
/// USAGE:
///   Vec(ModelInfo) models;
///   VecInitWithDeepCopyStack(&models, 64, ModelInfoInitClone, ModelInfoDeinit, {
///         // scope where vector memory is available
///         VecForeachPtr(&models, model, {
///             Render(model);
///         });
///
///         // Do not call deinit after use!!
///   });
///
#define VecInitWithDeepCopyStack(v, ne, ci, cd, scoped_body)                                                           \
    do {                                                                                                               \
        VEC_DATA_TYPE(v) ___data___[(ne) + 1] = {0};                                                                   \
                                                                                                                       \
        *(v)          = (__typeof__(*v))VecInit();                                                                     \
        (v)->capacity = (ne);                                                                                          \
        (v)->data     = &___data___[0];                                                                                \
                                                                                                                       \
        { scoped_body }                                                                                                \
                                                                                                                       \
        if ((cd))                                                                                                      \
            VecForeachPtr((v), ve, { (cd)(ve); });                                                                     \
        else                                                                                                           \
            memset(&___data___[0], 0, sizeof(___data___));                                                             \
                                                                                                                       \
        memset(v, 0, sizeof(*v));                                                                                      \
    } while (0)

///
/// Initialize given vector with given alignment.
/// It is mandatory to initialize vectors before use. Not doing so is undefined behaviour.
///
/// Provided alignment is used to keep all objects at an aligned memory location,
/// avoiding UB in some cases. It's recommended to use aligned vector when dealing with
/// structs containing unions.
///
/// These vectors are best used where user doesn't get a chance to or does not want
/// to deinit vector, given that no data in vector needs to be deinitialized.
/// Example includes, but does not limit to a Vec(i8), Vec(f32), etc...
///
/// v[in,out] : Pointer to vector memory that needs to be initialized.
/// ne[in]    : Number of elements to allocate aligned stack memory for.
/// ci[in]    : Copy init method for copying over elements in vector.
/// cd[in]    : Copy deinit method for deiniting elements in vector.
/// aln[in]   : Alignment value to align all emenets to.
///
/// USAGE:
///   Vec(Node*) nodes;
///
///   // initialize vector with stack memory, aligned with 124 byte boundary
///   VecInitAlignedStack(&nodes, 24, NodeInitCopy, NodeDeinit, 124, NULL, NULL, {
///         // scope where vector memory is available
///         FindAndFillAllNodes(&nodes, ... /* some other relevant data */);
///
///         UseNodes(&nodes);
///
///         VecForeach(&nodes, node, {
///         });
///
///         // vector deinit will be called for you after this automatically
///         // so any data held by the vector in this scope is invalid outside
///   });
///
#define VecInitAlignedWithDeepCopyStack(v, ne, ci, cd, aln, scoped_body)                                               \
    do {                                                                                                               \
        char ___data___[ALIGN_UP(sizeof(VEC_DATA_TYPE(v)), (aln)) * ((ne) + 1)] = {0};                                 \
                                                                                                                       \
        *(v)          = (__typeof__(*v))VecInitAligned((aln));                                                         \
        (v)->capacity = (ne);                                                                                          \
        (v)->data     = (VEC_DATA_TYPE(v) *)&___data___[0];                                                            \
                                                                                                                       \
        { scoped_body }                                                                                                \
                                                                                                                       \
        if ((cd))                                                                                                      \
            VecForeachPtr((v), ve, { (cd)(ve); });                                                                     \
        else                                                                                                           \
            memset(&___data___[0], 0, sizeof(___data___));                                                             \
                                                                                                                       \
        memset(v, 0, sizeof(*v));                                                                                      \
    } while (0)

///
/// Deinit vec by freeing all allocations.
///
/// v[in,out] : Pointer to vector to be destroyed
///
/// USAGE:
///   Vec(Model)* models = GetAllModels(...);
///   ... // use vector
///   DeinitVec(models)
///
#define VecDeinit(v) deinit_vec(GENERIC_VEC(v), sizeof(VEC_DATA_TYPE(v)))

///
/// Insert item into vector of it's type.
/// Insertion index must not exceed vector length.
/// This preserves the ordering of elements. Best to be used with sorted vectors,
/// if the sorted property is to be preserved.
///
/// In worst case this would to to O(n)
///
/// v[in,out] : Vector to insert item into
/// val[in]   : Value to be inserted
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
#define VecInsert(v, val, idx)                                                                                         \
    do {                                                                                                               \
        VEC_DATA_TYPE(v) __tmp__val = (val);                                                                           \
        insert_range_into_vec(GENERIC_VEC(v), (char *)&__tmp__val, sizeof(VEC_DATA_TYPE(v)), (idx), 1);                \
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
/// val[in]   : Value to be inserted
/// idx[in]   : Index to insert item at.
///
/// SUCCESS : return
/// FAILURE : Does not return
///
#define VecInsertFast(v, val, idx)                                                                                     \
    do {                                                                                                               \
        VEC_DATA_TYPE(v) __tmp__val = (val);                                                                           \
        insert_range_fast_into_vec(GENERIC_VEC(v), (char *)&__tmp__val, sizeof(VEC_DATA_TYPE(v)), (idx), 1);           \
    } while (0)

///
/// Insert item into vector of it's type.
/// Insertion index must not exceed vector length.
/// This preserves the ordering of elements. Best to be used with sorted vectors,
/// if the sorted property is to be preserved.
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
        const VEC_DATA_TYPE(v) *__tmp__ptr = (varr);                                                                   \
        insert_range_into_vec(GENERIC_VEC(v), (char *)__tmp__ptr, sizeof(VEC_DATA_TYPE(v)), (idx), (count));           \
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
/// val[in]   : Value to be inserted
/// idx[in]   : Index to insert item at.
///
/// SUCCESS : return
/// FAILURE : Does not return
///
#define VecInsertRangeFast(v, varr, idx, count)                                                                        \
    do {                                                                                                               \
        VEC_DATA_TYPE(v) __tmp__ptr = (varr);                                                                          \
        insert_range_fast_into_vec(GENERIC_VEC(v), (char *)&__tmp__ptr, sizeof(VEC_DATA_TYPE(v)), (idx), (count));     \
    } while (0)

///
/// Remove item from vector at given index and store in given pointer.
/// Order of elements is guaranteed to be preserved.
///
/// v[in,out] : Vector to remove item from.
/// ptr[out]  : Where removed item will be stored. If not provided then it's equivalent to
///             deleting the item at specified index.
/// idx[in]   : Index in vector to remove item from.
///
/// SUCCESS : return
/// FAILURE : Does not return
///
#define VecRemove(v, ptr, idx)                                                                                         \
    do {                                                                                                               \
        VEC_DATA_TYPE(v) *p = (ptr);                                                                                   \
        remove_range_vec(GENERIC_VEC(v), (char *)p, sizeof(VEC_DATA_TYPE(v)), (idx), 1);                               \
    } while (0)

///
/// Remove item from vector at given index and store in given pointer.
/// Order of elements inside vector is not guaranteed to be preserved.
/// The implementation is faster in some scenarios that `VecRemove`
///
/// v[in,out] : Vector to remove item from.
/// ptr[out]  : Where removed item will be stored. If not provided then it's equivalent to
///             deleting the item at specified index.
/// idx[in]   : Index in vector to remove item from.
///
/// SUCCESS : return
/// FAILURE : Does not return
///
#define VecRemoveFast(v, ptr, idx)                                                                                     \
    do {                                                                                                               \
        VEC_DATA_TYPE(v) *p = (ptr);                                                                                   \
        fast_remove_range_vec(GENERIC_VEC(v), (char *)(p), sizeof(VEC_DATA_TYPE(v)), (idx), 1);                        \
    } while (0)

///
/// Remove data from vector in given range [start, start + count)
/// Order of elements is guaranteed to be preserved.
///
/// v[in,out] : Vector to remove item from.
/// ptr[out]  : Where removed data will be stored. If not provided then it's equivalent to
///             deleting the items in specified range.
/// start[in] : Index in vector to removing items from.
/// count[in] : Number of items from starting index.
///
/// SUCCESS : return
/// FAILURE : Does not return
///
#define VecRemoveRange(v, ptr, start, count)                                                                           \
    do {                                                                                                               \
        VEC_DATA_TYPE(v) *p = (ptr);                                                                                   \
        remove_range_vec(GENERIC_VEC(v), (char *)p, sizeof(VEC_DATA_TYPE(v)), (start), (count));                       \
    } while (0)

///
/// Remove item from vector at given index and store in given pointer.
/// Order of elements inside vector is not guaranteed to be preserved.
/// The implementation is faster in some scenarios that `VecRemove`
///
/// v[in,out] : Vector to remove item from.
/// ptr[out]  : Where removed data will be stored. If not provided then it's equivalent to
///             deleting the items in specified range.
/// start[in] : Index in vector to removing items from.
/// count[in] : Number of items from starting index.
///
/// SUCCESS : return
/// FAILURE : Does not return
///
#define VecRemoveRangeFast(v, ptr, start, count)                                                                       \
    do {                                                                                                               \
        VEC_DATA_TYPE(v) *p = (ptr);                                                                                   \
        fast_remove_range_vec(GENERIC_VEC(v), (char *)p, sizeof(VEC_DATA_TYPE(v)), (start), (count));                  \
    } while (0)

///
/// Push item into vector back.
///
/// v[in,out]   : Vector to push item into
/// val[in] : Pointer to value to be pushed
///
/// SUCCESS : return
/// FAILURE : Does not return
///
#define VecPushBack(v, val) VecInsert((v), (val), (v)->length)

///
/// Pop item from vector back.
///
/// v[in,out]  : Vector to pop item from.
/// ptr[out]   : Popped item will be stored here. Make sure this has sufficient memory
///              to store memcopied data. If no pointer is provided, then it's equivalent
///              to deleting item from last position.
///
/// SUCCESS : return
/// FAILURE : Does not return
///
#define VecPopBack(v, ptr) VecRemove((v), (ptr), (v)->length - 1)

///
/// Push item into vector front.
///
/// v[in,out]   : Vector to push item into
/// val[in] : Pointer to value to be pushed
///
/// SUCCESS : return
/// FAILURE : Does not return
///
#define VecPushFront(v, val) VecInsert((v), (val), 0)

///
/// Pop item from vector front.
///
/// v[in,out]  : Vector to pop item from.
/// ptr[out]   : Popped item will be stored here. Make sure this has sufficient memory
///              to store memcopied data. If no pointer is provided, then it's equivalent
///              to deleting item from last position.
///
#define VecPopFront(v, ptr) VecRemove((v), (ptr), 0)

///
/// Delete last item from vec
///
/// SUCCESS : return
/// FAILURE : Does not return
///
#define VecDeleteLast(v) VecPopBack((v), NULL)

///
/// Delete item at given index
///
/// SUCCESS : return
/// FAILURE : Does not return
///
#define VecDelete(v, idx) VecRemove((v), NULL, (idx))

///
/// Delete item at given index using faster implementation.
/// Order preservation is not guaranteed
///
/// SUCCESS : return
/// FAILURE : Does not return
///
#define VecDeleteFast(v, idx) VecRemoveFast((v), NULL, (idx))

///
/// Delete items in given range [start, start + count)
///
/// SUCCESS : return
/// FAILURE : Does not return
///
#define VecDeleteRange(v, start, count) VecRemoveRange((v), NULL, (start), (count))

///
/// Delete items in given range [start, start + count) using faster implementation.
/// Order preservation is not guaranteed
///
/// SUCCESS : return
/// FAILURE : Does not return
///
#define VecDeleteRangeFast(v, start, count) VecRemoveRangeFast((v), NULL, (start), (count))

///
/// Sort given vector with given comparator using quicksort algorithm.
///
/// v[in,out]  : Vector to be sorted.
/// compare[in] : Compare function. Signature and behaviour must be similar to that of `strcmp`.
///
/// SUCCESS : return
/// FAILURE : Does not return
///
#define VecSort(v, compare) (qsort_vec(GENERIC_VEC(v), sizeof(VEC_DATA_TYPE(v)), (GenericCompare)(compare)))

///
/// Try reducing memory footprint of vector.
/// This is to be used when we know actual allocated memory for vec is large,
/// and we won't need it in future, so we can reduce it to whatever's required at
/// the moment.
///
/// v[in,out] : Vector
///
/// SUCCESS : return
/// FAILURE : Does not return
///
#define VecTryReduceSpace(v) (reduce_space_vec(GENERIC_VEC(v)))

///
/// Swap items at given indices.
///
/// v[in,out] : Vector to swap items in.
/// idx1[in]  : Index/Position of first item.
/// idx1[in]  : Index/Position of second item.
///
/// SUCCESS : return
/// FAILURE : Does not return
///
#define VecSwapItems(v, idx1, idx2) (swap_vec(GENERIC_VEC(v), sizeof(VEC_DATA_TYPE(v)), (idx1), (idx2)))

///
/// Resize vector.
/// If length is smaller than current capacity, vector length is shrinked.
/// If length is greater than current capacity, space is reserved and vector is expanded.
///
/// vec[in,out] : Vector to be resized.
/// len[in]     : New length of vector.
///
/// SUCCESS : return
/// FAILURE : Does not return
///
#define VecResize(v, len) (resize_vec(GENERIC_VEC(v), sizeof(VEC_DATA_TYPE(v)), (len)))

///
/// Reserve space for vector.
///
/// vec[in,out] : Vector to be resized.
/// len[in]     : New capacity of vector.
///
/// SUCCESS : return
/// FAILURE : Does not return
///
#define VecReserve(v, n) (reserve_vec(GENERIC_VEC(v), sizeof(VEC_DATA_TYPE(v)), (n)))

///
/// Clear vec contents.
///
/// vec[in,out] : Vector to be cleared.
///
/// SUCCESS : return
/// FAILURE : Does not return
///
#define VecClear(v) (clear_vec(GENERIC_VEC(v), sizeof(VEC_DATA_TYPE(v))))

///
/// Vector implementation already manages alignment for stored objects.
/// It makes sure that objects of improper size are stored at alignment of 8 bytes each
/// to avoid UB.
///
/// v[in]   : Vector to get aligned offset for.
/// idx[in] : Index of element to get offset of
///
/// SUCCESS : Alignment address.
/// FAILURE : Does not return on failure
///
#define VecAlignedOffsetAt(v, idx)                                                                                     \
    ((v) ? ((idx) * ALIGN_UP(sizeof(VEC_DATA_TYPE(v)), (v)->alignment)) :                                              \
           (LOG_FATAL("Invalid vector provided to VecAlignedOffsetAt! Aborting..."), 0))

///
/// Value at given index in a vector.
/// It's strongly recommended to always use this instead of directly accessing the data.
///
/// v[in]   : Vector to get data from
/// idx[in] : Index to get data at
///
#define VecAt(v, idx) ((VEC_DATA_TYPE(v) *)(VecAlignedOffsetAt(v, idx) + (char *)(v)->data))[0]

///
/// Value at given index in a vector.
/// It's strongly recommended to always use this instead of directly accessing the data.
///
/// v[in]   : Vector to get data from
/// idx[in] : Index to get data at
///
#define VecPtrAt(v, idx) (VEC_DATA_TYPE(v) *)(VecAlignedOffsetAt(v, idx) + (char *)(v)->data)

///
/// Value of first element in vector.
///
/// v[in] : Vector to get first element of.
///
#define VecFirst(v) VecAt(v, 0)

///
/// Value of last element in vector.
///
/// v[in] : Vector to get last element of.
///
#define VecLast(v) VecAt(v, (v)->length - 1)

///
/// Pointer to first element in vector
///
/// v[in] : Vector to get beginning ptr of.
///
#define VecBegin(v) VecPtrAt(v, 0)

///
/// Pointer at the end (after last element) of vector
///
/// v[in] : Vector to get end of.
///
#define VecEnd(v) VecPtrAt(v, (v)->length)

///
/// Push a complete array into this vector.
///
/// If array and count are both NULL and zero together correspondingly,
/// then the function simply returns without any error. Any other combination
/// is invalid and will result in an `abort()` call.
///
/// v[in,out] : Vector to insert array items into.
/// arr[in]   : Array to be inserted.
/// count[in] : Number of items in array.
///
/// SUCCESS : `v`
/// FAILURE : Does not return on failure
///
#define VecPushArr(v, arr, count, pos)                                                                                 \
    (push_arr_vec(GENERIC_VEC(v), sizeof(VEC_DATA_TYPE(v)), (void *)(arr), (count), (pos)))

///
/// Push a complete array into this vector.
///
/// v[in,out] : Vector to insert array items into.
/// arr[in]   : Array to be inserted.
/// count[in] : Number (non-zero) of items in array.
///
/// SUCCESS : `v`
/// FAILURE : Does not return on failure
///
#define VecPushBackArr(v, arr, count) VecPushArr((v), (arr), (count), (v)->length)

///
/// Push a complete array into this vector.
///
/// v[in,out] : Vector to insert array items into.
/// arr[in]   : Array to be inserted.
/// count[in] : Number (non-zero) of items in array.
///
/// SUCCESS : `v`
/// FAILURE : Does not return on failure
///
#define VecPushFrontArr(v, arr, count) VecPushArr((v), (arr), (count), 0)

///
/// Merge two vectors and store the result in the first vector.
///
/// Data is copied from `v2` into `v`. If a `copy_init` method is provided in `v`,
/// each element from `v2` will be copied using that method. Otherwise, a raw memory
/// copy is performed, which may be unsafe for complex or pointer-containing data.
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
/// Merge `v2` into `v`, transferring ownership of `v2`'s data into `v`.
///
/// This macro ensures safe transfer when `v2` should no longer be used after merge.
///
/// [in,out] v   : Destination vector that will own the resulting data.
/// [in,out] v2  : Source vector whose data is merged and deinitialized.
///
/// SUCCESS : `v`
/// FAILURE : Does not return on failure
///
#define VecMergeAndOwn(v, v2)                                                                                          \
    ((v2)->copy_init = (v)->copy_init,                                                                                 \
     (v)->copy_init  = NULL,                                                                                           \
     VecMerge((v), (v2)),                                                                                              \
     (v)->copy_init    = (v2)->copy_init,                                                                              \
     (v2)->copy_deinit = NULL,                                                                                         \
     VecDeinit(v2))

///
/// Initialize clone of vector from `vs` to `vd`.
///
/// vd[out] : Destination vector to create clone into
/// vs[in]  : Source vector to create clone using.
///
/// SUCCESS : `vd`
/// FAILURE : Does not return on failure
///
#define VecInitClone(vd, vs) (VecDeinit(vd), VecMerge(vd, vs))

///
/// Size of vector in bytes. Use this instead of multiplying
/// size of item with vector length!
///
/// v[in] : Vector to get length of
///
#define VecSize(v) VecAlignedOffsetAt(v, v->length)

///
/// Length of vector.
///
/// v[in] : Vector to get length of
///
#define VecLen(v) VecAlignedOffsetAt(v, v->length)

///
/// Reverse contents of this vector.
///
/// v[in,out] : Vector to be reversed.
///
/// SUCCESS : `v`
/// FAILURE : Does not return on failure
///
#define VecReverse(v) (reverse_vec(GENERIC_VEC(v), sizeof(VEC_DATA_TYPE(v))))

///
/// Iterate over each element `var` of given vector `v` at each index `idx` into the vector.
/// The variables `var` and `idx` declared and defined by this macro.
///
/// `idx` will start from 0 and will go till v->length - 1
///
/// v[in,out] : Vector to iterate over.
/// var[in]   : Name of variable to be used which'll contain value at iterated index `idx`
/// idx[in]   : Name of variable to be used for iterating over indices.
/// body      : Body of this foreach loop
///
#define VecForeachIdx(v, var, idx, body)                                                                               \
    do {                                                                                                               \
        i64 idx              = 0;                                                                                      \
        VEC_DATA_TYPE(v) var = {0};                                                                                    \
        if ((v) && (v)->length) {                                                                                      \
            for ((idx) = 0; (idx) < (v)->length; ++(idx)) {                                                            \
                if ((idx) < 0) {                                                                                       \
                    LOG_FATAL(                                                                                         \
                        "Vector range underflow : Invalid index reached "                                              \
                        "during Foreach iteration."                                                                    \
                    );                                                                                                 \
                }                                                                                                      \
                var = VecAt(v, idx);                                                                                   \
                { body }                                                                                               \
            }                                                                                                          \
        }                                                                                                              \
    } while (0)

///
/// Iterate over each element `var` of given vector `v` at each index `idx` into the vector.
/// The variables `var` and `idx` declared and defined by this macro.
///
/// `idx` will start from v->length - 1 and will go till 0
///
/// v[in,out] : Vector to iterate over.
/// var[in]   : Name of variable to be used which'll contain value at iterated index `idx`
/// idx[in]   : Name of variable to be used for iterating over indices.
/// body      : Body of this foreach loop
///
#define VecForeachReverseIdx(v, var, idx, body)                                                                        \
    do {                                                                                                               \
        i64 idx              = 0;                                                                                      \
        VEC_DATA_TYPE(v) var = {0};                                                                                    \
        if ((v) && (v)->length) {                                                                                      \
            for ((idx) = (v)->length - 1; (idx) >= 0; --(idx)) {                                                       \
                if ((idx) < 0) {                                                                                       \
                    LOG_FATAL(                                                                                         \
                        "Vector range underflow : Invalid index reached "                                              \
                        "during Foreach reverse "                                                                      \
                        "iteration."                                                                                   \
                    );                                                                                                 \
                }                                                                                                      \
                if ((idx) >= (v)->length) {                                                                            \
                    LOG_FATAL(                                                                                         \
                        "Vector range overflow : Invalid index reached "                                               \
                        "during Foreach reverse "                                                                      \
                        "iteration."                                                                                   \
                    );                                                                                                 \
                }                                                                                                      \
                var = VecAt(v, idx);                                                                                   \
                { body }                                                                                               \
            }                                                                                                          \
        }                                                                                                              \
    } while (0)

///
/// Iterate over each element `var` of given vector `v` at each index `idx` into the vector.
/// The variables `var` and `idx` declared and defined by this macro.
///
/// `idx` will start from 0 and will go till v->length - 1
///
/// v[in,out] : Vector to iterate over.
/// var[in]   : Name of variable to be used which'll contain value at iterated index `idx`
/// idx[in]   : Name of variable to be used for iterating over indices.
/// body      : Body of this foreach loop
///
#define VecForeachPtrIdx(v, var, idx, body)                                                                            \
    do {                                                                                                               \
        i64 idx               = 0;                                                                                     \
        VEC_DATA_TYPE(v) *var = {0};                                                                                   \
        if ((v) && (v)->length) {                                                                                      \
            for ((idx) = 0; (idx) < (v)->length; ++(idx)) {                                                            \
                if ((idx) < 0) {                                                                                       \
                    LOG_FATAL(                                                                                         \
                        "Vector range underflow : Invalid index reached "                                              \
                        "during Foreach iteration."                                                                    \
                    );                                                                                                 \
                }                                                                                                      \
                var = VecPtrAt(v, idx);                                                                                \
                { body }                                                                                               \
            }                                                                                                          \
        }                                                                                                              \
    } while (0)

///
/// Iterate over each element `var` of given vector `v` at each index `idx` into the vector.
/// The variables `var` and `idx` declared and defined by this macro.
///
/// `idx` will start from v->length - 1 and will go till 0
///
/// v[in,out] : Vector to iterate over.
/// var[in]   : Name of variable to be used which'll contain value at iterated index `idx`
/// idx[in]   : Name of variable to be used for iterating over indices.
/// body      : Body of this foreach loop
///
#define VecForeachPtrReverseIdx(v, var, idx, body)                                                                     \
    do {                                                                                                               \
        i64 idx               = 0;                                                                                     \
        VEC_DATA_TYPE(v) *var = {0};                                                                                   \
        if ((v) && (v)->length) {                                                                                      \
            for ((idx) = (v)->length - 1; (idx) >= 0; --(idx)) {                                                       \
                if ((idx) < 0) {                                                                                       \
                    LOG_FATAL(                                                                                         \
                        "Vector range underflow : Invalid index reached "                                              \
                        "during Foreach reverse "                                                                      \
                        "iteration."                                                                                   \
                    );                                                                                                 \
                }                                                                                                      \
                if ((idx) >= (v)->length) {                                                                            \
                    LOG_FATAL(                                                                                         \
                        "Vector range overflow : Invalid index reached "                                               \
                        "during Foreach reverse "                                                                      \
                        "iteration."                                                                                   \
                    );                                                                                                 \
                }                                                                                                      \
                var = VecPtrAt(v, idx);                                                                                \
                { body }                                                                                               \
            }                                                                                                          \
        }                                                                                                              \
    } while (0)

///
/// Iterate over each element `var` of the given vector `v`.
/// This is a convenience macro that iterates forward using an internally managed index.
/// The variable `var` is declared and defined by this macro.
///
/// v[in,out] : Vector to iterate over.
/// var[in]   : Name of the variable to be used which will contain the value of the
///             current element during iteration. The type of `var` will be the
///             data type of the vector elements (obtained via `VEC_DATA_TYPE(v)`).
/// body      : The block of code to be executed for each element of the vector.
///
/// SUCCESS : The `body` is executed for each element of the vector `v` from the
///           beginning to the end.
/// FAILURE : If the vector `v` is NULL or its length is zero, the loop body will not
///           be executed. Any failures within the `VecForeachIdx` macro (like invalid
///           index access) will result in a fatal log message and program termination.
///
#define VecForeach(v, var, body) VecForeachIdx((v), (var), (____iter___), {body})

///
/// Iterate over each element `var` of the given vector `v` in reverse order.
/// This is a convenience macro that iterates backward using an internally managed index.
/// The variable `var` is declared and defined by this macro.
///
/// v[in,out] : Vector to iterate over.
/// var[in]   : Name of the variable to be used which will contain the value of the
///             current element during iteration. The type of `var` will be the
///             data type of the vector elements (obtained via `VEC_DATA_TYPE(v)`).
/// body      : The block of code to be executed for each element of the vector.
///
/// SUCCESS : The `body` is executed for each element of the vector `v` from the
///           end to the beginning.
/// FAILURE : If the vector `v` is NULL or its length is zero, the loop body will not
///           be executed. Any failures within the `VecForeachReverseIdx` macro (like
///           invalid index access) will result in a fatal log message and program termination.
///
#define VecForeachReverse(v, var, body) VecForeachReverseIdx((v), (var), (____iter___), {body})

///
/// Iterate over each element `var` (as a pointer) of the given vector `v`.
/// This is a convenience macro that iterates forward using an internally managed index
/// and provides a pointer to each element. The variable `var` is declared and defined
/// by this macro as a pointer to the vector's data type.
///
/// v[in,out] : Vector to iterate over.
/// var[in]   : Name of the pointer variable to be used which will point to the
///             current element during iteration. The type of `var` will be a pointer
///             to the data type of the vector elements (obtained via
///             `VEC_DATA_TYPE(v) *`).
/// body      : The block of code to be executed for each element of the vector.
///
/// SUCCESS : The `body` is executed for each element of the vector `v` (with `var`
///           pointing to the current element) from the beginning to the end.
/// FAILURE : If the vector `v` is NULL or its length is zero, the loop body will not
///           be executed. Any failures within the `VecForeachPtrIdx` macro (like invalid
///           index access) will result in a fatal log message and program termination.
///
#define VecForeachPtr(v, var, body) VecForeachPtrIdx((v), (var), (____iter___), {body})

///
/// Iterate over each element `var` (as a pointer) of the given vector `v` in reverse order.
/// This is a convenience macro that iterates backward using an internally managed index
/// and provides a pointer to each element. The variable `var` is declared and defined
/// by this macro as a pointer to the vector's data type.
///
/// v[in,out] : Vector to iterate over.
/// var[in]   : Name of the pointer variable to be used which will point to the
///             current element during iteration. The type of `var` will be a pointer
///             to the data type of the vector elements (obtained via
///             `VEC_DATA_TYPE(v) *`).
/// body      : The block of code to be executed for each element of the vector.
///
/// SUCCESS : The `body` is executed for each element of the vector `v` (with `var`
///           pointing to the current element) from the end to the beginning.
/// FAILURE : If the vector `v` is NULL or its length is zero, the loop body will not
///           be executed. Any failures within the `VecForeachPtrReverseIdx` macro (like
///           invalid index access) will result in a fatal log message and program termination.
///
#define VecForeachPtrReverse(v, var, body) VecForeachPtrReverseIdx((v), (var), (____iter___), {body})


void init_vec(
    GenericVec       *vec,
    size              item_size,
    GenericCopyInit   copy_init,
    GenericCopyDeinit copy_deinit,
    size              alignment
);
void init_vec_on_stack(
    GenericVec       *vec,
    char             *stack_mem,
    size              capacity,
    size              item_size,
    GenericCopyInit   copy_init,
    GenericCopyDeinit copy_deinit,
    size              alignment
);
void deinit_vec(GenericVec *vec, size item_size);
void clear_vec(GenericVec *vec, size item_size);
void resize_vec(GenericVec *vec, size item_size, size new_size);
void reserve_vec(GenericVec *vec, size item_size, size n);
void reserve_pow2_vec(GenericVec *vec, size item_size, size n);
void reduce_space_vec(GenericVec *vec, size item_size);
void insert_range_into_vec(GenericVec *vec, char *item_data, size item_size, size idx, size count);
void insert_range_fast_into_vec(GenericVec *vec, char *item_data, size item_size, size idx, size count);
void remove_range_vec(GenericVec *vec, void *removed_data, size item_size, size start, size count);
void fast_remove_range_vec(GenericVec *vec, void *removed_data, size item_size, size start, size count);
void qsort_vec(GenericVec *vec, size item_size, GenericCompare comp);
void swap_vec(GenericVec *vec, size item_size, size idx1, size idx2);
void reverse_vec(GenericVec *vec, size item_size);
void push_arr_vec(GenericVec *vec, size item_size, char *arr, size count, size pos);

#endif // MISRA_STD_CONTAINER_VEC_H
