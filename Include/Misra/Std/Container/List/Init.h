/// file      : Init.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// copyright : Copyright (c) 2025, Siddharth Mishra, All rights reserved.
///
/// Different kinds of initializers for List
///

#ifndef MISRA_STD_CONTAINER_LIST_INIT_H
#define MISRA_STD_CONTAINER_LIST_INIT_H

///
/// Initialize a list. Inside a `Scope` block the allocator argument
/// may be omitted (`MisraScope` is used). Otherwise pass a typed
/// allocator handle or a raw `Allocator *`.
///
#define ListInit(...)               MISRA_OVERLOAD(ListInit, __VA_ARGS__)
#define ListInit_0()                ListInitWithDeepCopy_3(NULL, NULL, MisraScope)
#define ListInit_1(typed_alloc_ptr) ListInitWithDeepCopy_3(NULL, NULL, typed_alloc_ptr)

///
/// Typed-cast variant of `ListInit`.
///
#define ListInitT(l, ...)               MISRA_OVERLOAD(ListInitT, l, __VA_ARGS__)
#define ListInitT_1(l)                  ListInitWithDeepCopyT_4(l, NULL, NULL, MisraScope)
#define ListInitT_2(l, typed_alloc_ptr) ListInitWithDeepCopyT_4(l, NULL, NULL, typed_alloc_ptr)

///
/// Initialize a list with copy init and deinit callbacks. The
/// allocator argument is optional inside a `Scope` block.
///
#define ListInitWithDeepCopy(...)      MISRA_OVERLOAD(ListInitWithDeepCopy, __VA_ARGS__)
#define ListInitWithDeepCopy_2(ci, cd) ListInitWithDeepCopy_3(ci, cd, MisraScope)
#define ListInitWithDeepCopy_3(ci, cd, typed_alloc_ptr)                                                                \
    {.head        = NULL,                                                                                              \
     .tail        = NULL,                                                                                              \
     .copy_init   = (ci),                                                                                              \
     .copy_deinit = (cd),                                                                                              \
     .length      = 0,                                                                                                 \
     .allocator   = ALLOCATOR_OF(typed_alloc_ptr),                                                                     \
     .__magic     = LIST_MAGIC}

#define ListInitWithDeepCopyT(l, ...) MISRA_OVERLOAD(ListInitWithDeepCopyT, l, __VA_ARGS__)
#ifdef __cplusplus
#    define ListInitWithDeepCopyT_3(l, ci, cd) (TYPE_OF(l) ListInitWithDeepCopy_3(ci, cd, MisraScope))
#    define ListInitWithDeepCopyT_4(l, ci, cd, typed_alloc_ptr)                                                        \
        (TYPE_OF(l) ListInitWithDeepCopy_3(ci, cd, typed_alloc_ptr))
#else
#    define ListInitWithDeepCopyT_3(l, ci, cd) ((TYPE_OF(l))ListInitWithDeepCopy_3(ci, cd, MisraScope))
#    define ListInitWithDeepCopyT_4(l, ci, cd, typed_alloc_ptr)                                                        \
        ((TYPE_OF(l))ListInitWithDeepCopy_3(ci, cd, typed_alloc_ptr))
#endif

#define ListDeinit(v) deinit_list(GENERIC_LIST(v), sizeof(LIST_DATA_TYPE(v)))

#endif // MISRA_STD_CONTAINER_LIST_H
