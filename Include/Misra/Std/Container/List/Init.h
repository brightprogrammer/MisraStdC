/// file      : Init.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// copyright : Copyright (c) 2025, Siddharth Mishra, All rights reserved.
///
/// Different kinds of initializers for List
///

#ifndef MISRA_STD_CONTAINER_LIST_INIT_H
#define MISRA_STD_CONTAINER_LIST_INIT_H

///
/// Initialize a list with a user-owned allocator.
///
#define ListInit(typed_alloc_ptr) ListInitWithDeepCopy(NULL, NULL, typed_alloc_ptr)

///
/// Initialize a list with the given typed allocator pointer (typed variant).
///
#define ListInitT(l, typed_alloc_ptr) ListInitWithDeepCopyT(l, NULL, NULL, typed_alloc_ptr)

///
/// Initialize a list with copy init and deinit callbacks.
///
#define ListInitWithDeepCopy(ci, cd, typed_alloc_ptr)                                                                  \
    {.head        = NULL,                                                                                              \
     .tail        = NULL,                                                                                              \
     .copy_init   = (ci),                                                                                              \
     .copy_deinit = (cd),                                                                                              \
     .length      = 0,                                                                                                 \
     .allocator   = ALLOCATOR_OF(typed_alloc_ptr),                                                                     \
     .__magic     = MISRA_LIST_MAGIC}

#ifdef __cplusplus
#    define ListInitWithDeepCopyT(l, ci, cd, typed_alloc_ptr) (TYPE_OF(l) ListInitWithDeepCopy(ci, cd, typed_alloc_ptr))
#else
#    define ListInitWithDeepCopyT(l, ci, cd, typed_alloc_ptr)                                                          \
        ((TYPE_OF(l))ListInitWithDeepCopy(ci, cd, typed_alloc_ptr))
#endif

#define ListDeinit(v) deinit_list(GENERIC_LIST(v), sizeof(LIST_DATA_TYPE(v)))

#endif // MISRA_STD_CONTAINER_LIST_H
