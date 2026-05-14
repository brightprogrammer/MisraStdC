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
/// An allocator can be passed as the final optional argument. If omitted, DefaultAllocator() is used.
///
#define LIST_INIT_HAS_ARGS_IMPL(_0, _1, count, ...) count
#define LIST_INIT_HAS_ARGS(...)                     LIST_INIT_HAS_ARGS_IMPL(__VA_OPT__(, ) __VA_ARGS__, 1, 0, 0)
#define ListInit(...)                               CONCAT(ListInit_, LIST_INIT_HAS_ARGS(__VA_ARGS__))(__VA_ARGS__)
#define ListInit_0()                                LIST_INIT_WITH_DEEP_COPY_VALUE(NULL, NULL, DefaultAllocator())
#define ListInit_1(alloc)                           LIST_INIT_WITH_DEEP_COPY_VALUE(NULL, NULL, (alloc))

///
/// Initialize a list with default arguments.
///
/// l[in]     : Pointer to list memory that needs to be initialized.
/// alloc[in] : Optional allocator copied into the list. If omitted, DefaultAllocator() is used.
///
/// USAGE:
///     List(i32) list = ListInitT(list);
///
/// TAGS: Init, List, Length, Size, Aligned, DeepCopy, DeepDeinit
///
#define LIST_INIT_T_HAS_ARGS_IMPL(_1, _2, count, ...) count
#define LIST_INIT_T_HAS_ARGS(...)                     LIST_INIT_T_HAS_ARGS_IMPL(__VA_ARGS__, 2, 1, 0)
#define ListInitT(...)                                CONCAT(ListInitT_, LIST_INIT_T_HAS_ARGS(__VA_ARGS__))(__VA_ARGS__)
#define ListInitT_1(l)                                ListInitWithDeepCopyT((l), NULL, NULL)
#define ListInitT_2(l, alloc)                         ListInitWithDeepCopyT((l), NULL, NULL, (alloc))

///
/// Initialize a list with copy init and deinit methods for maintaining a deep-copy.
///
/// ci[in]    : Copy init method.
/// cd[in]    : Copy deinit method.
/// alloc[in] : Optional allocator copied into the list. If omitted, DefaultAllocator() is used.
///
#define LIST_INIT_WITH_DEEP_COPY_HAS_ARGS_IMPL(_1, _2, _3, count, ...) count
#define LIST_INIT_WITH_DEEP_COPY_HAS_ARGS(...)                         LIST_INIT_WITH_DEEP_COPY_HAS_ARGS_IMPL(__VA_ARGS__, 3, 2, 1, 0)
#define ListInitWithDeepCopy(...)                                                                                      \
    CONCAT(ListInitWithDeepCopy_, LIST_INIT_WITH_DEEP_COPY_HAS_ARGS(__VA_ARGS__))(__VA_ARGS__)
#define ListInitWithDeepCopy_2(ci, cd)        LIST_INIT_WITH_DEEP_COPY_VALUE((ci), (cd), DefaultAllocator())
#define ListInitWithDeepCopy_3(ci, cd, alloc) LIST_INIT_WITH_DEEP_COPY_VALUE((ci), (cd), (alloc))

#define LIST_INIT_WITH_DEEP_COPY_VALUE(ci, cd, alloc)                                                                  \
    {.head        = NULL,                                                                                              \
     .tail        = NULL,                                                                                              \
     .copy_init   = (GenericCopyInit)(ci),                                                                             \
     .copy_deinit = (GenericCopyDeinit)(cd),                                                                           \
     .length      = 0,                                                                                                 \
     .allocator   = AllocatorBind((alloc)),                                                                            \
     .__magic     = MISRA_LIST_MAGIC}

#define LIST_INIT_WITH_DEEP_COPY_T_HAS_ARGS_IMPL(_1, _2, _3, _4, count, ...) count
#define LIST_INIT_WITH_DEEP_COPY_T_HAS_ARGS(...)                             LIST_INIT_WITH_DEEP_COPY_T_HAS_ARGS_IMPL(__VA_ARGS__, 4, 3, 2, 1, 0)

#ifdef __cplusplus
#    define ListInitWithDeepCopyT(...)                                                                                 \
        CONCAT(ListInitWithDeepCopyT_, LIST_INIT_WITH_DEEP_COPY_T_HAS_ARGS(__VA_ARGS__))(__VA_ARGS__)
#    define ListInitWithDeepCopyT_3(l, ci, cd)        (TYPE_OF(l) ListInitWithDeepCopy((ci), (cd)))
#    define ListInitWithDeepCopyT_4(l, ci, cd, alloc) (TYPE_OF(l) ListInitWithDeepCopy((ci), (cd), (alloc)))
#else
///
/// Initialize a list of given type with deep-copy.
///
/// l[in]     : List pointer to be initialized.
/// ci[in]    : Copy init method (for maintaining deep-copy).
/// cd[in]    : Copy deinit method (for deiniting copied objects)
/// alloc[in] : Optional allocator copied into the list. If omitted, DefaultAllocator() is used.
///
#    define ListInitWithDeepCopyT(...)                                                                                 \
        CONCAT(ListInitWithDeepCopyT_, LIST_INIT_WITH_DEEP_COPY_T_HAS_ARGS(__VA_ARGS__))(__VA_ARGS__)
#    define ListInitWithDeepCopyT_3(l, ci, cd)        ((TYPE_OF(l))ListInitWithDeepCopy((ci), (cd)))
#    define ListInitWithDeepCopyT_4(l, ci, cd, alloc) ((TYPE_OF(l))ListInitWithDeepCopy((ci), (cd), (alloc)))
#endif

///
/// Deinitialize a list and release every node back to its allocator.
/// Element payloads are torn down through the configured `copy_deinit`
/// handler when present.
///
/// After this call the list is empty and can either be re-initialized or
/// discarded.
///
/// v[in,out] : Pointer to list to deinitialize.
///
/// TAGS: List, Deinit, Cleanup, Memory
///
#define ListDeinit(v) deinit_list(GENERIC_LIST(v), sizeof(LIST_DATA_TYPE(v)))

#endif // MISRA_STD_CONTAINER_LIST_H
