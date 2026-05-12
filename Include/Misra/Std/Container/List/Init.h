/// file      : Init.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// copyright : Copyright (c) 2025, Siddharth Mishra, All rights reserved.
///
/// Different kinds of initializers for List
///

#ifndef MISRA_STD_CONTAINER_LIST_INIT_H
#define MISRA_STD_CONTAINER_LIST_INIT_H

///
/// Initialize a list with default arguments.
///
#define LIST_INIT_HAS_ARGS_IMPL(_0, _1, count, ...) count
#define LIST_INIT_HAS_ARGS(...) LIST_INIT_HAS_ARGS_IMPL(__VA_OPT__(,) __VA_ARGS__, 1, 0, 0)
#define ListInit(...) CONCAT(ListInit_, LIST_INIT_HAS_ARGS(__VA_ARGS__))(__VA_ARGS__)
#define ListInit_0() ListInitWithDeepCopyAndAlloc(NULL, NULL, DefaultAllocator())
#define ListInit_1(alloc) ListInitWithDeepCopyAndAlloc(NULL, NULL, (alloc))

///
/// Initialize a list with default arguments.
///
/// l[in] : Pointer to list memory that needs to be initialized.
///
/// USAGE:
///     List(i32) list = ListInitT(list);
///
/// TAGS: Init, List, Length, Size, Aligned, DeepCopy, DeepDeinit
///
#define ListInitT(l) ListInitWithDeepCopyT(l, NULL, NULL)

///
/// Initialize a list with copy init and deinit methods for maintaining a deep-copy.
///
/// l[in]  : Pointer to list memory that needs to be initialized.
/// ci[in] : Copy init method.
/// cd[in] : Copy deinit method.
///
#define ListInitWithDeepCopy(ci, cd) ListInitWithDeepCopyAndAlloc((ci), (cd), DefaultAllocator())

#define ListInitWithDeepCopyAndAlloc(ci, cd, alloc)                                                                    \
    {.head = NULL,                                                                                                     \
     .tail = NULL,                                                                                                     \
     .copy_init = (GenericCopyInit)(ci),                                                                               \
     .copy_deinit = (GenericCopyDeinit)(cd),                                                                           \
     .length = 0,                                                                                                      \
     .allocator = AllocatorBind((alloc)),                                                                              \
     .__magic = MISRA_LIST_MAGIC}

#ifdef __cplusplus
#    define ListInitWithDeepCopyT(l, ci, cd) (TYPE_OF(l) ListInitWithDeepCopy(ci, cd))
#    define ListInitWithDeepCopyAllocT(l, ci, cd, alloc)                                                               \
        (TYPE_OF(l) ListInitWithDeepCopyAndAlloc((ci), (cd), (alloc)))
#else
///
/// Initialize a list of given type with deep-copy.
///
/// l[in]  : Vector pointer to be initialized.
/// ci[in] : Copy init method (for maintaining deep-copy).
/// cd[in] : Copy deinit method (for deiniting copied objects)
///
#    define ListInitWithDeepCopyT(l, ci, cd) ((TYPE_OF(l))ListInitWithDeepCopy(ci, cd))
#    define ListInitWithDeepCopyAllocT(l, ci, cd, alloc)                                                               \
        ((TYPE_OF(l))ListInitWithDeepCopyAndAlloc((ci), (cd), (alloc)))
#endif

#define ListDeinit(v) deinit_list(GENERIC_LIST(v), sizeof(LIST_DATA_TYPE(v)))

#endif // MISRA_STD_CONTAINER_LIST_H
