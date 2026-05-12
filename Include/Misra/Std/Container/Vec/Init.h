/// file      : std/container/vec/init.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Different types of initializers for a vector.

#ifndef MISRA_STD_CONTAINER_VEC_INIT_H
#define MISRA_STD_CONTAINER_VEC_INIT_H

#include "Type.h"
#include "Private.h"

///
/// Initialize vector. Default alignment is 1
/// It is mandatory to initialize vectors before use. Not doing so is undefined behaviour.
///
/// USAGE:
///   Vec(HttpRequest) requests = VecInit();
///
/// TAGS: Init, Vec, Length, Size, Aligned
///
#define VEC_INIT_HAS_ARGS_IMPL(_0, _1, count, ...) count
#define VEC_INIT_HAS_ARGS(...) VEC_INIT_HAS_ARGS_IMPL(__VA_OPT__(,) __VA_ARGS__, 1, 0, 0)
#define VecInit(...) CONCAT(VecInit_, VEC_INIT_HAS_ARGS(__VA_ARGS__))(__VA_ARGS__)
#define VecInit_0() VecInitAlignedWithDeepCopyAndAlloc(NULL, NULL, 1, DefaultAllocator())
#define VecInit_1(alloc) VecInitAlignedWithDeepCopyAndAlloc(NULL, NULL, 1, (alloc))

///
/// Initialize given vector. Default alignment is 1
/// It is mandatory to initialize vectors before use. Not doing so is undefined behaviour.
///
/// v[in] : Variable or type of a vector to be initialized.
///
/// USAGE:
///     void SomeInterestingFn(DataVec* data_vec) {
///         *data_vec = VecInitT(data);
///
///         // use vector
///     }
///
/// TAGS: Init, Vec, Length, Size, Aligned
///
#define VecInitT(v) VecInitAlignedWithDeepCopyT(v, NULL, NULL, 1)
#define VecInitAllocT(v, alloc) VecInitAlignedWithDeepCopyAllocT(v, NULL, NULL, 1, (alloc))

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
#define VecInitWithDeepCopy(ci, cd) VecInitAlignedWithDeepCopyAndAlloc((ci), (cd), 1, DefaultAllocator())
#define VecInitWithDeepCopyAlloc(ci, cd, alloc) VecInitAlignedWithDeepCopyAndAlloc((ci), (cd), 1, (alloc))

///
/// Initialize given vector. Default alignment is 1
/// It is mandatory to initialize vectors before use. Not doing so is undefined behaviour.
///
/// v[in] : Variable or type of a vector to be initialized.
/// ci[in] : Copy init method.
/// cd[in] : Copy deinit method.
///
/// USAGE:
///     bool DataInitClone(Data* dst, Data* src) { /* cloning logic...*/ }
///     void DataDeinit(Data* d) { /* deinit logic... don't free "d" */ }
///
///     void SomeInterestingFn(DataVec* data_vec) {
///         *data_vec = VecInitWithDeepCopyT(data, DataInitClone, DataDeinit);
///
///         // use vector
///     }
///
/// TAGS: Init, Vec, Length, Size, Aligned, DeepCopy, DeepDeinit
///
#define VecInitWithDeepCopyT(v, ci, cd) VecInitAlignedWithDeepCopyT(v, ci, cd, 1)
#define VecInitWithDeepCopyAllocT(v, ci, cd, alloc) VecInitAlignedWithDeepCopyAllocT(v, ci, cd, 1, (alloc))

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
#define VecInitAligned(aln) VecInitAlignedWithDeepCopyAndAlloc(NULL, NULL, (aln), DefaultAllocator())
#define VecInitAlignedAlloc(aln, alloc) VecInitAlignedWithDeepCopyAndAlloc(NULL, NULL, (aln), (alloc))

///
/// Initialize given vector with given alignment.
/// It is mandatory to initialize vectors before use. Not doing so is undefined behaviour.
///
/// Provided alignment is used to keep all objects at an aligned memory location,
/// avoiding UB in some cases. It's recommended to use aligned vector when dealing with
/// structs containing unions.
///
/// v[in]   : Variable or type of a vector to be initialized.
/// aln[in] : Vector element alignment. All items will be stored by respecting the
///             alignment boundary.
///
/// USAGE:
///     Vec(Node) nodes = VecInitAligned(16);
///
///     void SomeInterestingFn(DataVec data_vec) {
///         // align items in "data_vec" at 128 byte boundaries
///         data_vec = VecInitAlignedT(data, 128);
///
///         // use vector
///     }
///
/// TAGS: Init, Vec, Length, Size, Aligned
///
#define VecInitAlignedT(v, aln) VecInitAlignedWithDeepCopyT(v, NULL, NULL, aln)
#define VecInitAlignedAllocT(v, aln, alloc) VecInitAlignedWithDeepCopyAllocT(v, NULL, NULL, (aln), (alloc))

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
///   NodeVec nodes = VecInitAlignedWithDeepCopy(NodeInitCopy, NodeDeinit, 48);
///
/// TAGS: Init, Vec, Length, Size, Aligned, DeepCopy, DeepDeinit
///
#define VecInitAlignedWithDeepCopy(ci, cd, aln) VecInitAlignedWithDeepCopyAndAlloc((ci), (cd), (aln), DefaultAllocator())

#define VecInitAlignedWithDeepCopyAndAlloc(ci, cd, aln, alloc)                                                         \
    {.length      = 0,                                                                                                 \
     .capacity    = 0,                                                                                                 \
     .copy_init   = (GenericCopyInit)(ci),                                                                             \
     .copy_deinit = (GenericCopyDeinit)(cd),                                                                           \
     .data        = NULL,                                                                                              \
     .alignment   = (aln),                                                                                             \
     .allocator   = AllocatorBind((alloc)),                                                                            \
     .__magic     = MISRA_VEC_MAGIC}

#ifdef __cplusplus
#    define VecInitAlignedWithDeepCopyT(v, ci, cd, aln) (TYPE_OF(v) VecInitAlignedWithDeepCopy((ci), (cd), (aln)))
#    define VecInitAlignedWithDeepCopyAllocT(v, ci, cd, aln, alloc)                                                    \
        (TYPE_OF(v) VecInitAlignedWithDeepCopyAndAlloc((ci), (cd), (aln), (alloc)))
#else
///
/// Initialize given vector with given alignment.
/// It is mandatory to initialize vectors before use. Not doing so is undefined behaviour.
///
/// Provided alignment is used to keep all objects at an aligned memory location,
/// avoiding UB in some cases. It's recommended to use aligned vector when dealing with
/// structs containing unions.
///
/// v[in]   : Variable or type of a vector to be initialized.
/// ci[in]  : Copy init method.
/// cd[in]  : Copy deinit method.
/// aln[in] : Vector element alignment. All items will be stored by respecting the
///             alignment boundary.
///
/// USAGE:
///     bool DataInitClone(Data* dst, Data* src) { /* cloning logic...*/ }
///     void DataDeinit(Data* d) { /* deinit logic... don't free "d" */ }
///
///     void SomeInterestingFn(DataVec data_vec) {
///         // align and store all items in vector at 32 byte boundaries
///         data_vec = VecInitAlignedWithDeepCopyT(data_vec, DataInitClone, DataDeinit, 32);
///
///         // use vector
///         // always use VecAt to access elements in vector
///         Data i1 = VecAt(data_vec, 9); // get 10th item (index = 9)
///         Data i2 = VecLast(data_vec); // get last item (index = whatever)
///     }
///
/// TAGS: Init, Vec, Length, Size, Aligned, DeepCopy, DeepDeinit
///
#    define VecInitAlignedWithDeepCopyT(v, ci, cd, aln) ((TYPE_OF(v))VecInitAlignedWithDeepCopy((ci), (cd), (aln)))
#    define VecInitAlignedWithDeepCopyAllocT(v, ci, cd, aln, alloc)                                                    \
        ((TYPE_OF(v))VecInitAlignedWithDeepCopyAndAlloc((ci), (cd), (aln), (alloc)))
#endif

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
/// v[in,out] : Vector that needs to be initialized.
/// ne[in]    : Number of elements to allocate aligned stack memory for.
/// ci[in]    : Copy init method for copying over elements in vector.
/// cd[in]    : Copy deinit method for deiniting elements in vector.
/// aln[in]   : Alignment value to align all emenets to.
///
/// USAGE:
///   Vec(Node*) nodes;
///
///   // initialize vector with stack memory, aligned with 124 byte boundary
///   VecInitAlignedWithDeepCopyStack(&nodes, 24, NodeInitCopy, NodeDeinit, 124, {
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
/// TAGS: Init, Vec, Stack, Aligned, Length, Size, Array
///
#define VecInitAlignedWithDeepCopyStack(v, ne, ci, cd, aln, scoped_body)                                               \
    do {                                                                                                               \
        char ___data___[ALIGN_UP(sizeof(VEC_DATATYPE(&(v))), (aln)) * ((ne) + 1)] = {0};                               \
                                                                                                                       \
        (v)          = VecInitAlignedWithDeepCopyT((v), (ci), (cd), (aln));                                            \
        (v).capacity = (ne);                                                                                           \
        (v).data     = (VEC_DATATYPE(&(v)) *)&___data___[0];                                                           \
                                                                                                                       \
        {scoped_body}                                                                                                  \
                                                                                                                       \
        memset(___data___, 0, sizeof(___data___));                                                                     \
        memset(&(v), 0, sizeof(v));                                                                                    \
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
/// v[in,out] : Vector that needs to be initialized.
/// ne[in]    : Number of elements to allocate stack memory for.
///
/// USAGE:
///   Vec(i32) ids;
///   VecInitStack(ids, 64, {
///       // scope where vector memory is available
///       ids = MakeClientRequestToFillVector(ids);
///       VecForeach(&ids, id, {
///           // some relevant logic
///       });
///
///       // Do not call deinit after use!!
///   });
///
/// TAGS: Init, Vec, Stack, Length, Size, Array
///
#define VecInitStack(v, ne, scoped_body) VecInitAlignedWithDeepCopyStack(v, ne, NULL, NULL, 1, scoped_body)

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
/// v[in,out] : Vector that needs to be initialized.
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
    VecInitAlignedWithDeepCopyStack(v, ne, NULL, NULL, aln, scoped_body)

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
/// v[in,out] : Vector that needs to be initialized.
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
    VecInitAlignedWithDeepCopyStack(v, ne, ci, cd, 1, scoped_body)

///
/// Deinit vec by freeing all allocations.
///
/// v[in,out] : Pointer to `Vec` to be deinited
///
/// USAGE:
///   Vec(Model)* models = GetAllModels(...);
///   ... // use vector
///   DeinitVec(models)
///
#define VecDeinit(v) deinit_vec(GENERIC_VEC(v), sizeof(VEC_DATATYPE(v)))

#endif // MISRA_STD_CONTAINER_VEC_INIT_H
