/// file      : std/container/vec/init.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Different types of initializers for a vector.

#ifndef MISRA_STD_CONTAINER_VEC_INIT_H
#define MISRA_STD_CONTAINER_VEC_INIT_H

#include "Private.h"
#include "Type.h"

///
/// Initialize vector. Element stride alignment is derived from the allocator's
/// `alignment` field.
/// It is mandatory to initialize vectors before use. Not doing so is undefined behaviour.
/// An allocator can be passed as the final optional argument. If omitted, DefaultAllocator() is used.
/// To request stronger alignment for vector elements, pass an allocator built via
/// `HeapAllocatorAligned(n)` (or set the allocator's `alignment` field directly).
///
/// USAGE:
///   Vec(HttpRequest) requests = VecInit();
///   Vec(HttpRequest) arena_requests = VecInit(arena_allocator);
///   Vec(SimdVec) wide = VecInit(HeapAllocatorAligned(32));
///
/// TAGS: Init, Vec, Length, Size
///
#define VEC_INIT_HAS_ARGS_IMPL(_0, _1, count, ...) count
#define VEC_INIT_HAS_ARGS(...)                     VEC_INIT_HAS_ARGS_IMPL(__VA_OPT__(, ) __VA_ARGS__, 1, 0, 0)
#define VecInit(...)                               CONCAT(VecInit_, VEC_INIT_HAS_ARGS(__VA_ARGS__))(__VA_ARGS__)
#define VecInit_0()                                VEC_INIT_WITH_DEEP_COPY_VALUE(NULL, NULL, DefaultAllocator())
#define VecInit_1(alloc)                           VEC_INIT_WITH_DEEP_COPY_VALUE(NULL, NULL, (alloc))

///
/// Initialize given vector.
/// It is mandatory to initialize vectors before use. Not doing so is undefined behaviour.
///
/// v[in]     : Variable or type of a vector to be initialized.
/// alloc[in] : Optional allocator copied into the vector. If omitted, DefaultAllocator() is used.
///
/// USAGE:
///     void SomeInterestingFn(DataVec* data_vec) {
///         *data_vec = VecInitT(data_vec);
///
///         // use vector
///     }
///
/// TAGS: Init, Vec, Length, Size
///
#define VEC_INIT_T_HAS_ARGS_IMPL(_1, _2, count, ...) count
#define VEC_INIT_T_HAS_ARGS(...)                     VEC_INIT_T_HAS_ARGS_IMPL(__VA_ARGS__, 2, 1, 0)
#define VecInitT(...)                                CONCAT(VecInitT_, VEC_INIT_T_HAS_ARGS(__VA_ARGS__))(__VA_ARGS__)
#define VecInitT_1(v)                                VecInitWithDeepCopyT((v), NULL, NULL)
#define VecInitT_2(v, alloc)                         VecInitWithDeepCopyT((v), NULL, NULL, (alloc))

///
/// Initialize vector with deep-copy callbacks.
/// It is mandatory to initialize vectors before use. Not doing so is undefined behaviour.
///
/// ci[in]    : Copy init method.
/// cd[in]    : Copy deinit method.
/// alloc[in] : Optional allocator copied into the vector. If omitted, DefaultAllocator() is used.
///
/// USAGE:
///   Vec(HttpRequest) requests = VecInitWithDeepCopy(RequestClone, RequestDeinit);
///
/// TAGS: Init, Vec, Length, Size, DeepCopy, DeepDeinit
///
#define VEC_INIT_WITH_DEEP_COPY_HAS_ARGS_IMPL(_1, _2, _3, count, ...) count
#define VEC_INIT_WITH_DEEP_COPY_HAS_ARGS(...)                         VEC_INIT_WITH_DEEP_COPY_HAS_ARGS_IMPL(__VA_ARGS__, 3, 2, 1, 0)
#define VecInitWithDeepCopy(...)                                                                                       \
    CONCAT(VecInitWithDeepCopy_, VEC_INIT_WITH_DEEP_COPY_HAS_ARGS(__VA_ARGS__))(__VA_ARGS__)
#define VecInitWithDeepCopy_2(ci, cd)        VEC_INIT_WITH_DEEP_COPY_VALUE((ci), (cd), DefaultAllocator())
#define VecInitWithDeepCopy_3(ci, cd, alloc) VEC_INIT_WITH_DEEP_COPY_VALUE((ci), (cd), (alloc))

#define VEC_INIT_WITH_DEEP_COPY_VALUE(ci, cd, alloc)                                                                   \
    {.length      = 0,                                                                                                 \
     .capacity    = 0,                                                                                                 \
     .copy_init   = (GenericCopyInit)(ci),                                                                             \
     .copy_deinit = (GenericCopyDeinit)(cd),                                                                           \
     .data        = NULL,                                                                                              \
     .allocator   = AllocatorBind((alloc)),                                                                            \
     .__magic     = MISRA_VEC_MAGIC}

#define VEC_INIT_WITH_DEEP_COPY_T_HAS_ARGS_IMPL(_1, _2, _3, _4, count, ...) count
#define VEC_INIT_WITH_DEEP_COPY_T_HAS_ARGS(...)                             VEC_INIT_WITH_DEEP_COPY_T_HAS_ARGS_IMPL(__VA_ARGS__, 4, 3, 2, 1, 0)

#ifdef __cplusplus
#    define VecInitWithDeepCopyT(...)                                                                                  \
        CONCAT(VecInitWithDeepCopyT_, VEC_INIT_WITH_DEEP_COPY_T_HAS_ARGS(__VA_ARGS__))(__VA_ARGS__)
#    define VecInitWithDeepCopyT_3(v, ci, cd)        (TYPE_OF(v) VecInitWithDeepCopy((ci), (cd)))
#    define VecInitWithDeepCopyT_4(v, ci, cd, alloc) (TYPE_OF(v) VecInitWithDeepCopy((ci), (cd), (alloc)))
#else
///
/// Initialize given vector with deep-copy callbacks.
/// It is mandatory to initialize vectors before use. Not doing so is undefined behaviour.
///
/// v[in]     : Variable or type of a vector to be initialized.
/// ci[in]    : Copy init method.
/// cd[in]    : Copy deinit method.
/// alloc[in] : Optional allocator copied into the vector. If omitted, DefaultAllocator() is used.
///
/// TAGS: Init, Vec, Length, Size, DeepCopy, DeepDeinit
///
#    define VecInitWithDeepCopyT(...)                                                                                  \
        CONCAT(VecInitWithDeepCopyT_, VEC_INIT_WITH_DEEP_COPY_T_HAS_ARGS(__VA_ARGS__))(__VA_ARGS__)
#    define VecInitWithDeepCopyT_3(v, ci, cd)        ((TYPE_OF(v))VecInitWithDeepCopy((ci), (cd)))
#    define VecInitWithDeepCopyT_4(v, ci, cd, alloc) ((TYPE_OF(v))VecInitWithDeepCopy((ci), (cd), (alloc)))
#endif

///
/// Initialize given vector with stack-allocated backing storage and deep-copy callbacks.
/// Such vectors cannot be dynamically resized; doing so is undefined behaviour. They
/// must not be deinitialized at the end of the scope - the macro tears down the
/// allocation automatically.
///
/// v[in,out] : Vector that needs to be initialized.
/// ne[in]    : Number of elements to allocate stack memory for.
/// ci[in]    : Copy init method.
/// cd[in]    : Copy deinit method.
/// scoped_body[in] : Block of code where the vector is alive.
///
/// USAGE:
///   Vec(ModelInfo) models;
///   VecInitWithDeepCopyStack(&models, 64, ModelInfoInitClone, ModelInfoDeinit, {
///       VecForeachPtr(&models, model, {
///           Render(model);
///       });
///   });
///
/// TAGS: Init, Vec, Stack, Length, Size, Array, DeepCopy
///
#define VecInitWithDeepCopyStack(v, ne, ci, cd, scoped_body)                                                           \
    do {                                                                                                               \
        char ___data___[sizeof(VEC_DATATYPE(&(v))) * ((ne) + 1)] = {0};                                                \
                                                                                                                       \
        (v)          = VecInitWithDeepCopyT((v), (ci), (cd));                                                          \
        (v).capacity = (ne);                                                                                           \
        (v).data     = (VEC_DATATYPE(&(v)) *)&___data___[0];                                                           \
                                                                                                                       \
        {scoped_body}                                                                                                  \
                                                                                                                       \
        MemSet(___data___, 0, sizeof(___data___));                                                                     \
        MemSet(&(v), 0, sizeof(v));                                                                                    \
    } while (0)

///
/// Initialize given vector using stack-allocated backing storage.
/// Such vectors cannot be dynamically resized; doing so is undefined behaviour. They
/// must not be deinitialized at the end of the scope - the macro tears down the
/// allocation automatically.
///
/// v[in,out] : Vector that needs to be initialized.
/// ne[in]    : Number of elements to allocate stack memory for.
/// scoped_body[in] : Block of code where the vector is alive.
///
/// USAGE:
///   Vec(i32) ids;
///   VecInitStack(ids, 64, {
///       ids = MakeClientRequestToFillVector(ids);
///       VecForeach(&ids, id, {
///           // some relevant logic
///       });
///   });
///
/// TAGS: Init, Vec, Stack, Length, Size, Array
///
#define VecInitStack(v, ne, scoped_body) VecInitWithDeepCopyStack(v, ne, NULL, NULL, scoped_body)

///
/// Deinit vec by freeing all allocations.
///
/// v[in,out] : Pointer to `Vec` to be deinited
///
/// USAGE:
///   Vec(Model)* models = GetAllModels(...);
///   ... // use vector
///   VecDeinit(models);
///
#define VecDeinit(v) deinit_vec(GENERIC_VEC(v), sizeof(VEC_DATATYPE(v)))

#endif // MISRA_STD_CONTAINER_VEC_INIT_H
