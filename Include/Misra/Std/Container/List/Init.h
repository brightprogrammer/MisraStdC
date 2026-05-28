/// file      : std/container/list/init.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
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
/// TAGS: List, Init, API
///
#define ListInit(...)               OVERLOAD(ListInit, __VA_ARGS__)
#define ListInit_0()                ListInitWithDeepCopy_3(NULL, NULL, MisraScope)
#define ListInit_1(typed_alloc_ptr) ListInitWithDeepCopy_3(NULL, NULL, typed_alloc_ptr)

///
/// Typed-cast variant of `ListInit`.
///
/// TAGS: List, Init, API
///
#define ListInitT(l, ...)               OVERLOAD(ListInitT, l, __VA_ARGS__)
#define ListInitT_1(l)                  ListInitWithDeepCopyT_4(l, NULL, NULL, MisraScope)
#define ListInitT_2(l, typed_alloc_ptr) ListInitWithDeepCopyT_4(l, NULL, NULL, typed_alloc_ptr)

///
/// Initialize a list with copy init and deinit callbacks. The
/// allocator argument is optional inside a `Scope` block.
///
/// TAGS: List, Init, DeepCopy, Copy
///
#define ListInitWithDeepCopy(...)      OVERLOAD(ListInitWithDeepCopy, __VA_ARGS__)
#define ListInitWithDeepCopy_2(ci, cd) ListInitWithDeepCopy_3(ci, cd, MisraScope)
#define ListInitWithDeepCopy_3(ci, cd, typed_alloc_ptr)                                                                \
    {.head        = NULL,                                                                                              \
     .tail        = NULL,                                                                                              \
     .copy_init   = (GenericCopyInit)(ci),                                                                             \
     .copy_deinit = (GenericCopyDeinit)(cd),                                                                           \
     .length      = 0,                                                                                                 \
     .allocator   = ALLOCATOR_OF(typed_alloc_ptr),                                                                     \
     .__magic     = LIST_MAGIC}

#define ListInitWithDeepCopyT(l, ...) OVERLOAD(ListInitWithDeepCopyT, l, __VA_ARGS__)
#ifdef __cplusplus
#    define ListInitWithDeepCopyT_3(l, ci, cd) (TYPE_OF(l) ListInitWithDeepCopy_3(ci, cd, MisraScope))
#    define ListInitWithDeepCopyT_4(l, ci, cd, typed_alloc_ptr)                                                        \
        (TYPE_OF(l) ListInitWithDeepCopy_3(ci, cd, typed_alloc_ptr))
#else
#    define ListInitWithDeepCopyT_3(l, ci, cd) ((TYPE_OF(l))ListInitWithDeepCopy_3(ci, cd, MisraScope))
#    define ListInitWithDeepCopyT_4(l, ci, cd, typed_alloc_ptr)                                                        \
        ((TYPE_OF(l))ListInitWithDeepCopy_3(ci, cd, typed_alloc_ptr))
#endif

///
/// Release a list's nodes and zero its handle. Calls any configured
/// `copy_deinit` hook on each live element before freeing storage.
///
/// l[in,out] : Pointer to a `List(T)` handle.
///
/// SUCCESS : Returns to the caller. The handle is zeroed; all node
///           storage reclaimed through the configured allocator.
/// FAILURE : `ValidateList` aborts via `LOG_FATAL` when `l` is NULL
///           or uninitialised.
///
/// TAGS: List, Deinit, Lifecycle
///
#define ListDeinit(v) deinit_list(GENERIC_LIST(v), sizeof(LIST_DATA_TYPE(v)))

#endif // MISRA_STD_CONTAINER_LIST_INIT_H
