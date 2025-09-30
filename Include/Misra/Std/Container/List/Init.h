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
#define ListInit() ListInitWithDeepCopy(NULL, NULL)

///
/// Initialize a list with copy init and deinit methods for maintaining a deep-copy.
///
/// l[in]  : Pointer to list memory that needs to be initialized.
/// ci[in] : Copy init method.
/// cd[in] : Copy deinit method.
///
#define ListInitWithDeepCopy(ci, cd)                                                                                   \
    {.head = NULL, .tail = NULL, .copy_init = (ci), .copy_deinit = (cd), .length = 0, .__magic = MISRA_LIST_MAGIC}

#ifdef __cplusplus
#    define ListInitWithDeepCopyT(l, ci, cd) (TYPE_OF(l) ListInitWithDeepCopy(ci, cd))
#else
///
/// Initialize a list of given type with deep-copy.
///
/// l[in]  : Vector pointer to be initialized.
/// ci[in] : Copy init method (for maintaining deep-copy).
/// cd[in] : Copy deinit method (for deiniting copied objects)
///
#    define ListInitWithDeepCopyT(l, ci, cd) ((TYPE_OF(l))ListInitWithDeepCopy(ci, cd))
#endif

#define ListDeinit(v) deinit_list(GENERIC_LIST(v), sizeof(LIST_DATA_TYPE(v)))

#endif // MISRA_STD_CONTAINER_LIST_H
