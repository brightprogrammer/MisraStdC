/// file      : Type.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// copyright : Copyright (c) 2025, Siddharth Mishra, All rights reserved.
///
/// Define list type.
///

#ifndef MISRA_STD_CONTAINER_LIST_TYPE_H
#define MISRA_STD_CONTAINER_LIST_TYPE_H

#include <Misra/Std/Container/Common.h>

typedef struct GenericListNode GenericListNode;

///
/// Definition of a single node in a linked list.
/// A double linked list where both participants in a link, contain references to each other.
/// This is not meant to be directly used by user. If you need to use this in user-code
/// either you're doing something wrong, or we need to improve this design!
///
struct GenericListNode {
    GenericListNode *next;
    GenericListNode *prev;
    void            *data;
};

///
/// The generic linked list.
/// Not meant to be directly used by user code.
///
typedef struct {
    GenericListNode  *head;
    GenericListNode  *tail;
    GenericCopyInit   copy_init;
    GenericCopyDeinit copy_deinit;
    u64               length;
    u64               __magic;
} GenericList;

///
/// Cast any list to a generic list
///
#define GENERIC_LIST(list) ((GenericList *)(void *)(list))

///
/// Cast any list node to a generic list node
///
#define GENERIC_LIST_NODE(node) ((GenericListNode *)(void *)(node))

///
/// Doubly-linked list node
///
/// FIELDS:
/// - next : Reference to next node. NULL if no next node exists (tail).
/// - prev : Reference to previous node. NULL if no previous node exists (head).
/// - data : Type specific pointer to data.
///
#define ListNode(T)                                                                                                    \
    struct {                                                                                                           \
        GenericListNode *next;                                                                                         \
        GenericListNode *prev;                                                                                         \
        T               *data;                                                                                         \
    }

///
/// Get data type stored by this list
///
#define LIST_DATA_TYPE(list) TYPE_OF(*((list)->head->data))

///
/// Get node type stored by this list
///
#define LIST_NODE_TYPE(list) ListNode(LIST_DATA_TYPE(list))

///
///  Double linked list.
///
/// FIELDS:
/// - head        : Reference to head node of linked list.
/// - tail        : Reference to tail node of linked list.
/// - copy_init   : A user-provided type-specific method to initialize copies of types.
/// - copy_deinit : A user-provided type-specific method deinitialize already created copies of types.
/// - length      : Length of this linked list.
///
#define List(T)                                                                                                        \
    struct {                                                                                                           \
        ListNode(T) * head;                                                                                            \
        ListNode(T) * tail;                                                                                            \
        GenericCopyInit   copy_init;                                                                                   \
        GenericCopyDeinit copy_deinit;                                                                                 \
        u64               length;                                                                                      \
        u64               __magic;                                                                                     \
    }

// magic bytes to give only 1 in a 2^64 chance for un-initialized or curropted list objects to be considered valid
// if the magic bytes of a list does not match this value then it's considered corrupted or un-initialized.
// this will mean that there's a bug in application and ValidateList will immediately abort!
#define MISRA_LIST_MAGIC MISRA_MAKE_NEW_MAGIC_VALUE("listimpl")

///
/// Validate whether a given `List` object is valid.
/// Not foolproof but will work most of the time.
/// Aborts if provided `List` is not valid.
///
/// l[in] : Pointer to `List` object to validate.
///
/// SUCCESS: Continue execution, meaning given `List` object is most probably a valid `List`.
/// FAILURE: `abort` with an error message.
///
#define ValidateList(l) validate_list((const GenericList *)GENERIC_LIST(l))

#endif // MISRA_STD_CONTAINER_LIST_TYPE_H
