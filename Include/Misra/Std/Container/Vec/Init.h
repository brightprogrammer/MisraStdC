/// file      : std/container/vec/init.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Different types of initializers for a vector.

#ifndef MISRA_STD_CONTAINER_VEC_INIT_H
#define MISRA_STD_CONTAINER_VEC_INIT_H

#include "Type.h"
#include <Misra/Std/Allocator.h>

///
/// Initialize a Vec bound to an allocator. The argument may be either
/// a typed allocator handle (`&heap`, `&arena`, ...) or a raw
/// `Allocator *` — `ALLOCATOR_OF` typechecks both at compile time and
/// converts to `Allocator *` via a whole-pointer typecast.
///
/// USAGE:
///   DefaultAllocator heap = DefaultAllocatorInit();
///   Vec(int) v = VecInit(&heap);
///   ...
///   VecDeinit(&v);
///   DefaultAllocatorDeinit(&heap);
///
/// TAGS: Init, Vec, Length, Size
///
#define VecInit(allocator_ptr)                                                                                         \
    {.length      = 0,                                                                                                 \
     .capacity    = 0,                                                                                                 \
     .copy_init   = NULL,                                                                                              \
     .copy_deinit = NULL,                                                                                              \
     .data        = NULL,                                                                                              \
     .allocator   = ALLOCATOR_OF(allocator_ptr),                                                                       \
     .__magic     = MISRA_VEC_MAGIC}

///
/// Typed-cast variant of `VecInit` for assigning into a typed Vec
/// variable. The cast makes the macro usable both as an in-place
/// initializer (`Vec(int) v = VecInitT(v, alloc);`) and as an
/// assignment target.
///
#ifdef __cplusplus
#    define VecInitT(v, alloc_ptr) (TYPE_OF(v) VecInit(alloc_ptr))
#else
#    define VecInitT(v, alloc_ptr) ((TYPE_OF(v))VecInit(alloc_ptr))
#endif

///
/// Initialize a Vec with deep-copy callbacks. Allocator argument routed
/// through `ALLOCATOR_OF` like `VecInit`.
///
#define VecInitWithDeepCopy(ci, cd, allocator_ptr)                                                                     \
    {.length      = 0,                                                                                                 \
     .capacity    = 0,                                                                                                 \
     .copy_init   = (GenericCopyInit)(ci),                                                                             \
     .copy_deinit = (GenericCopyDeinit)(cd),                                                                           \
     .data        = NULL,                                                                                              \
     .allocator   = ALLOCATOR_OF(allocator_ptr),                                                                       \
     .__magic     = MISRA_VEC_MAGIC}

#ifdef __cplusplus
#    define VecInitWithDeepCopyT(v, ci, cd, alloc_ptr)                                                                 \
        (TYPE_OF(v) VecInitWithDeepCopy(ci, cd, alloc_ptr))
#else
#    define VecInitWithDeepCopyT(v, ci, cd, alloc_ptr)                                                                 \
        ((TYPE_OF(v))VecInitWithDeepCopy(ci, cd, alloc_ptr))
#endif

///
/// Initialize a vector with stack-allocated backing storage and deep-copy
/// callbacks. The vector cannot be dynamically resized; it must not be
/// deinitialized at the end of the scope - the macro tears down the
/// allocation automatically.
///
#define VecInitWithDeepCopyStack(v, typed_alloc_ptr, ne, ci, cd, scoped_body)                                          \
    do {                                                                                                               \
        char ___data___[sizeof(VEC_DATATYPE(&(v))) * ((ne) + 1)] = {0};                                                \
                                                                                                                       \
        (v)          = VecInitWithDeepCopyT((v), (ci), (cd), (typed_alloc_ptr));                                       \
        (v).capacity = (ne);                                                                                           \
        (v).data     = (VEC_DATATYPE(&(v)) *)&___data___[0];                                                           \
                                                                                                                       \
        {scoped_body}                                                                                                  \
                                                                                                                       \
        MemSet(___data___, 0, sizeof(___data___));                                                                     \
        MemSet(&(v), 0, sizeof(v));                                                                                    \
    } while (0)

///
/// Initialize a vector using stack-allocated backing storage.
///
#define VecInitStack(v, typed_alloc_ptr, ne, scoped_body)                                                              \
    VecInitWithDeepCopyStack(v, typed_alloc_ptr, ne, NULL, NULL, scoped_body)

///
/// Deinit vec by freeing its backing buffer.
///
#define VecDeinit(v) deinit_vec(GENERIC_VEC(v), sizeof(VEC_DATATYPE(v)))

#endif // MISRA_STD_CONTAINER_VEC_INIT_H
