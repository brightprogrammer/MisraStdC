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
/// Inside a `Scope(...)` block the allocator argument may be omitted;
/// the macro then binds to the internal `MisraScope` allocator the
/// scope provides. Outside a `Scope`, calling `VecInit()` with no
/// argument fails to compile because `MisraScope` is undeclared - the
/// safety net the design relies on.
///
/// USAGE:
///   Scope(alloc, DefaultAllocator) {
///       Vec(int) v = VecInit();      // uses MisraScope
///       Vec(int) w = VecInit(alloc); // uses the named user-pool
///       ...
///       VecDeinit(&v);
///       VecDeinit(&w);
///   }
///
/// TAGS: Init, Vec, Length, Size
///
#define VecInit(...) MISRA_OVERLOAD(VecInit, __VA_ARGS__)
#define VecInit_0()  VecInit_1(MisraScope)
#define VecInit_1(allocator_ptr)                                                                                       \
    {.length      = 0,                                                                                                 \
     .capacity    = 0,                                                                                                 \
     .copy_init   = NULL,                                                                                              \
     .copy_deinit = NULL,                                                                                              \
     .data        = NULL,                                                                                              \
     .allocator   = ALLOCATOR_OF(allocator_ptr),                                                                       \
     .__magic     = VEC_MAGIC}

///
/// Typed-cast variant of `VecInit` for assigning into a typed Vec
/// variable. The cast makes the macro usable both as an in-place
/// initializer (`Vec(int) v = VecInitT(v, alloc);`) and as an
/// assignment target. The allocator argument is optional inside a
/// `Scope` block.
///
#define VecInitT(v, ...) MISRA_OVERLOAD(VecInitT, v, __VA_ARGS__)
#ifdef __cplusplus
#    define VecInitT_1(v)            (TYPE_OF(v) VecInit_1(MisraScope))
#    define VecInitT_2(v, alloc_ptr) (TYPE_OF(v) VecInit_1(alloc_ptr))
#else
#    define VecInitT_1(v)            ((TYPE_OF(v))VecInit_1(MisraScope))
#    define VecInitT_2(v, alloc_ptr) ((TYPE_OF(v))VecInit_1(alloc_ptr))
#endif

///
/// Initialize a Vec with deep-copy callbacks. The allocator argument
/// is optional in the same way as `VecInit` - inside a `Scope` block
/// you may omit it and `MisraScope` is used automatically.
///
#define VecInitWithDeepCopy(...)      MISRA_OVERLOAD(VecInitWithDeepCopy, __VA_ARGS__)
#define VecInitWithDeepCopy_2(ci, cd) VecInitWithDeepCopy_3(ci, cd, MisraScope)
#define VecInitWithDeepCopy_3(ci, cd, allocator_ptr)                                                                   \
    {.length      = 0,                                                                                                 \
     .capacity    = 0,                                                                                                 \
     .copy_init   = (GenericCopyInit)(ci),                                                                             \
     .copy_deinit = (GenericCopyDeinit)(cd),                                                                           \
     .data        = NULL,                                                                                              \
     .allocator   = ALLOCATOR_OF(allocator_ptr),                                                                       \
     .__magic     = VEC_MAGIC}

#define VecInitWithDeepCopyT(v, ...) MISRA_OVERLOAD(VecInitWithDeepCopyT, v, __VA_ARGS__)
#ifdef __cplusplus
#    define VecInitWithDeepCopyT_3(v, ci, cd)            (TYPE_OF(v) VecInitWithDeepCopy_3(ci, cd, MisraScope))
#    define VecInitWithDeepCopyT_4(v, ci, cd, alloc_ptr) (TYPE_OF(v) VecInitWithDeepCopy_3(ci, cd, alloc_ptr))
#else
#    define VecInitWithDeepCopyT_3(v, ci, cd)            ((TYPE_OF(v))VecInitWithDeepCopy_3(ci, cd, MisraScope))
#    define VecInitWithDeepCopyT_4(v, ci, cd, alloc_ptr) ((TYPE_OF(v))VecInitWithDeepCopy_3(ci, cd, alloc_ptr))
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
