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
/// Initialize a vector with a user-owned allocator. The allocator must
/// outlive the vector. Pass the typed allocator pointer directly - the
/// container macro verifies at compile time that the argument has an
/// `Allocator base` field.
///
/// USAGE:
///   HeapAllocator heap = HeapAllocatorInit();
///   Vec(int) v = VecInit(&heap);
///   ...
///   VecDeinit(&v);
///   HeapAllocatorDeinit(&heap);
///
/// TAGS: Init, Vec, Length, Size
///
#define VecInit(typed_alloc_ptr) VEC_INIT_WITH_DEEP_COPY_VALUE(NULL, NULL, typed_alloc_ptr)

///
/// Initialize a vector named `v` with the given typed allocator pointer.
/// The cast makes the macro usable both as an in-place initializer
/// (`Vec(int) v = VecInitT(v, &heap);`) and as an assignment target.
///
#ifdef __cplusplus
#    define VecInitT(v, typed_alloc_ptr) (TYPE_OF(v) VecInit(typed_alloc_ptr))
#else
#    define VecInitT(v, typed_alloc_ptr) ((TYPE_OF(v))VecInit(typed_alloc_ptr))
#endif

///
/// Initialize a vector with deep-copy callbacks.
///
#define VecInitWithDeepCopy(ci, cd, typed_alloc_ptr) VEC_INIT_WITH_DEEP_COPY_VALUE(ci, cd, typed_alloc_ptr)

#ifdef __cplusplus
#    define VecInitWithDeepCopyT(v, ci, cd, typed_alloc_ptr)                                                           \
        (TYPE_OF(v) VecInitWithDeepCopy(ci, cd, typed_alloc_ptr))
#else
#    define VecInitWithDeepCopyT(v, ci, cd, typed_alloc_ptr)                                                           \
        ((TYPE_OF(v))VecInitWithDeepCopy(ci, cd, typed_alloc_ptr))
#endif

#define VEC_INIT_WITH_DEEP_COPY_VALUE(ci, cd, typed_alloc_ptr)                                                         \
    {.length      = 0,                                                                                                 \
     .capacity    = 0,                                                                                                 \
     .copy_init   = (GenericCopyInit)(ci),                                                                             \
     .copy_deinit = (GenericCopyDeinit)(cd),                                                                           \
     .data        = NULL,                                                                                              \
     .allocator   = ALLOCATOR_OF(typed_alloc_ptr),                                                                     \
     .__magic     = MISRA_VEC_MAGIC}

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
