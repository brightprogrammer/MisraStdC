/// file      : Foreach.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// copyright : Copyright (c) 2025, Siddharth Mishra, All rights reserved.
///
/// List iterators.
///

#ifndef MISRA_STD_CONTAINER_LIST_FOREACH_H
#define MISRA_STD_CONTAINER_LIST_FOREACH_H

///
/// Iterate over each element var of the given list l.
/// The variable var is declared and defined by this macro.
///
/// Iteration happens in forward order, starting from the head of the list
/// and continuing through the next pointers until the end is reached.
/// The variable var will contain a copy of the value pointed to by each list node.
///
/// l[in]    : List to iterate over.
/// var[out] : Name of the variable to be used which will contain the value of the
///            current element during iteration. The type of var will be the
///            data type of the list elements (obtained via LIST_DATA_TYPE(l)).
///
/// TAGS: Foreach, List, Iteration, Loop
///
#define ListForeach(l, var)                                                                                                      \
    for (TYPE_OF(l) UNPL(pl) = (l); UNPL(pl); UNPL(pl) = NULL)                                                                   \
        if ((ValidateList(UNPL(pl)), 1) && (UNPL(pl)->head))                                                                     \
            for (GenericListNode * UNPL(node) = (GenericListNode *)ListNodeBegin(UNPL(pl)); UNPL(node);                          \
                 UNPL(node)                   = ListNodeNext(UNPL(node)))                                                        \
                if (((void *)UNPL(node)->next != (void *)UNPL(node)) &&                                        \
                    ((void *)UNPL(node)->prev != (void *)UNPL(node)) && (UNPL(node)->data))                    \
                    for (bool UNPL(_once) = true; UNPL(_once); UNPL(_once) = false)                            \
                        for (LIST_DATA_TYPE(UNPL(pl)) var = *((LIST_DATA_TYPE(UNPL(pl)) *)(UNPL(node)->data)); \
                             UNPL(_once);                                                                      \
                             UNPL(_once) = false)

///
/// Iterate over each element var (as a pointer) of the given list l.
/// The variable var is declared and defined by this macro as a pointer to the list's data type.
///
/// Iteration happens in forward order, starting from the head of the list
/// and continuing through the next pointers until the end is reached.
/// The variable var will point to the data associated with each list node.
///
/// l[in,out] : List to iterate over.
/// var[out]  : Name of the pointer variable to be used which will point to the
///             current element during iteration. The type of var will be a pointer
///             to the data type of the list elements (i.e., LIST_DATA_TYPE(l) *).
///
/// TAGS: Foreach, List, Iteration, Loop, Pointer
///
#define ListForeachPtr(l, var)                                                                                                 \
    for (TYPE_OF(l) UNPL(pl) = (l); UNPL(pl); UNPL(pl) = NULL)                                                                 \
        if ((ValidateList(UNPL(pl)), 1) && (UNPL(pl)->head))                                                                   \
            for (GenericListNode * UNPL(node) = (GenericListNode *)ListNodeBegin(UNPL(pl)); UNPL(node);                        \
                 UNPL(node)                   = ListNodeNext(UNPL(node)))                                                      \
                if (((void *)UNPL(node)->next != (void *)UNPL(node)) &&                                      \
                    ((void *)UNPL(node)->prev != (void *)UNPL(node)) && (UNPL(node)->data))                  \
                    for (bool UNPL(_once) = true; UNPL(_once); UNPL(_once) = false)                          \
                        for (LIST_DATA_TYPE(UNPL(pl)) *var = (LIST_DATA_TYPE(UNPL(pl)) *)(UNPL(node)->data); \
                             UNPL(_once);                                                                    \
                             UNPL(_once) = false)

///
/// Iterate over each element var of the given list l in reverse order.
/// The variable var is declared and defined by this macro.
///
/// Iteration happens in reverse, starting from the tail of the list
/// and continuing through the prev pointers until the head is reached.
/// The variable var will contain a copy of the value pointed to by each list node.
///
/// l[in]    : List to iterate over.
/// var[out] : Name of the variable to be used which will contain the value of the
///            current element during iteration. The type of var will be the
///            data type of the list elements (obtained via LIST_DATA_TYPE(l)).
///
/// TAGS: Foreach, List, Iteration, Loop, Reverse
///
#define ListForeachReverse(l, var)                                                                                               \
    for (TYPE_OF(l) UNPL(pl) = (l); UNPL(pl); UNPL(pl) = NULL)                                                                   \
        if ((ValidateList(UNPL(pl)), 1) && (UNPL(pl)->tail))                                                                     \
            for (GenericListNode * UNPL(node) = (GenericListNode *)ListNodeEnd(UNPL(pl)); UNPL(node);                            \
                 UNPL(node)                   = ListNodePrev(UNPL(node)))                                                        \
                if (((void *)UNPL(node)->next != (void *)UNPL(node)) &&                                        \
                    ((void *)UNPL(node)->prev != (void *)UNPL(node)) && (UNPL(node)->data))                    \
                    for (bool UNPL(_once) = true; UNPL(_once); UNPL(_once) = false)                            \
                        for (LIST_DATA_TYPE(UNPL(pl)) var = *((LIST_DATA_TYPE(UNPL(pl)) *)(UNPL(node)->data)); \
                             UNPL(_once);                                                                      \
                             UNPL(_once) = false)

///
/// Iterate over each element var (as a pointer) of the given list l in reverse order.
/// The variable var is declared and defined by this macro as a pointer to the list's data type.
///
/// Iteration happens in reverse, starting from the tail of the list
/// and continuing through the prev pointers until the head is reached.
/// The variable var will point to the data associated with each list node.
///
/// l[in,out] : List to iterate over.
/// var[out]  : Name of the pointer variable to be used which will point to the
///             current element during iteration. The type of var will be a pointer
///             to the data type of the list elements (i.e., LIST_DATA_TYPE(l) *).
///
/// TAGS: Foreach, List, Iteration, Loop, Reverse, Pointer
///
#define ListForeachPtrReverse(l, var)                                                                                          \
    for (TYPE_OF(l) UNPL(pl) = (l); UNPL(pl); UNPL(pl) = NULL)                                                                 \
        if ((ValidateList(UNPL(pl)), 1) && (UNPL(pl)->tail))                                                                   \
            for (GenericListNode * UNPL(node) = (GenericListNode *)ListNodeEnd(UNPL(pl)); UNPL(node);                          \
                 UNPL(node)                   = ListNodePrev(UNPL(node)))                                                      \
                if (((void *)UNPL(node)->next != (void *)UNPL(node)) &&                                      \
                    ((void *)UNPL(node)->prev != (void *)UNPL(node)) && (UNPL(node)->data))                  \
                    for (bool UNPL(_once) = true; UNPL(_once); UNPL(_once) = false)                          \
                        for (LIST_DATA_TYPE(UNPL(pl)) *var = (LIST_DATA_TYPE(UNPL(pl)) *)(UNPL(node)->data); \
                             UNPL(_once);                                                                    \
                             UNPL(_once) = false)

///
/// Iterate over each element var of the given list l in the index range [start, end).
/// The variable var is declared and defined by this macro.
///
/// This macro performs forward traversal, starting at index start (inclusive)
/// and continuing until index end (exclusive), assuming zero-based indexing.
///
/// Since linked lists are not indexable, the traversal walks node-by-node and skips
/// nodes before start, then continues while tracking the current index.
///
/// l[in]     : List to iterate over.
/// var[out]  : Name of the variable to be used which will contain the value
///             of the current element during iteration.
/// start[in] : Starting index (inclusive).
/// end[in]   : Ending index (exclusive).
///
/// TAGS: Foreach, List, Iteration, Loop, Range
///
#define ListForeachInRange(l, var, start, end)                                                                                           \
    for (TYPE_OF(l) UNPL(pl) = (l); UNPL(pl); UNPL(pl) = NULL)                                                                           \
        if ((ValidateList(UNPL(pl)), 1) && UNPL(pl)->head)                                                                               \
            for (GenericListNode * UNPL(node) = (GenericListNode *)ListNodeBegin(UNPL(pl)); UNPL(node);                                  \
                 UNPL(node)                   = ListNodeNext(UNPL(node)))                                                                \
                for (u64 UNPL(i) = 0; UNPL(node) && UNPL(i) < (end); UNPL(node) = ListNodeNext(UNPL(node)), ++UNPL(i)) \
                    if (UNPL(i) >= (start) && (UNPL(node)->data))                                                      \
                        for (bool UNPL(_once) = true; UNPL(_once); UNPL(_once) = false)                                \
                            for (LIST_DATA_TYPE(UNPL(pl)) var = *((LIST_DATA_TYPE(UNPL(pl)) *)(UNPL(node)->data));     \
                                 UNPL(_once);                                                                          \
                                 UNPL(_once) = false)

///
/// This macro performs forward traversal, starting at index start (inclusive)
/// and continuing until index end (exclusive), assuming zero-based indexing.
///
/// Since linked lists are not indexable, the traversal walks node-by-node and skips
/// nodes before start, then continues while tracking the current index.
///
/// l[in,out] : List to iterate over.
/// var[in,out]   : Name of the pointer variable to be used which will point to the
///             current element during iteration.
/// start[in] : Starting index (inclusive).
/// end[in]   : Ending index (exclusive).
///
/// TAGS: Foreach, List, Iteration, Loop, Range, Pointer
///
#define ListForeachPtrInRange(l, var, start, end)                                                                                        \
    for (TYPE_OF(l) UNPL(pl) = (l); UNPL(pl); UNPL(pl) = NULL)                                                                           \
        if ((ValidateList(UNPL(pl)), 1) && UNPL(pl)->head)                                                                               \
            for (GenericListNode * UNPL(node) = (GenericListNode *)ListNodeBegin(UNPL(pl)); UNPL(node);                                  \
                 UNPL(node)                   = ListNodeNext(UNPL(node)))                                                                \
                for (u64 UNPL(i) = 0; UNPL(node) && UNPL(i) < (end); UNPL(node) = ListNodeNext(UNPL(node)), ++UNPL(i)) \
                    if (UNPL(i) >= (start) && (UNPL(node)->data))                                                      \
                        for (bool UNPL(_once) = true; UNPL(_once); UNPL(_once) = false)                                \
                            for (LIST_DATA_TYPE(UNPL(pl)) *var = (LIST_DATA_TYPE(UNPL(pl)) *)(UNPL(node)->data);       \
                                 UNPL(_once);                                                                          \
                                 UNPL(_once) = false)

///
/// Iterate over each element var of the given list l in reverse, limited to index range [start, end)
/// relative to the tail of the list. Index 0 corresponds to the tail, 1 to the previous node, and so on.
///
/// The variable var is declared and defined by this macro.
///
/// Since linked lists do not support indexing, this macro counts nodes from the tail
/// and includes only those where the relative reverse index lies in [start, end).
///
/// l[in]         : List to iterate over.
/// var[in,out]   : Name of the variable to be used which will contain the value
///                 of the current element during iteration.
/// start[in]     : Starting index from tail (inclusive).
/// end[in]       : Ending index from tail (exclusive).
///
/// TAGS: Foreach, List, Iteration, Loop, Reverse, Range
///
#define ListForeachReverseInRange(l, var, start, end)                                                                                    \
    for (TYPE_OF(l) UNPL(pl) = (l); UNPL(pl); UNPL(pl) = NULL)                                                                           \
        if ((ValidateList(UNPL(pl)), 1) && UNPL(pl)->tail)                                                                               \
            for (GenericListNode * UNPL(node) = (GenericListNode *)ListNodeEnd(UNPL(pl)); UNPL(node);                                    \
                 UNPL(node)                   = ListNodePrev(UNPL(node)))                                                                \
                for (u64 UNPL(i) = 0; UNPL(node) && UNPL(i) < (end); UNPL(node) = ListNodePrev(UNPL(node)), ++UNPL(i)) \
                    if (UNPL(i) >= (start) && (UNPL(node)->data))                                                      \
                        for (bool UNPL(_once) = true; UNPL(_once); UNPL(_once) = false)                                \
                            for (LIST_DATA_TYPE(UNPL(pl)) var = *((LIST_DATA_TYPE(UNPL(pl)) *)(UNPL(node)->data));     \
                                 UNPL(_once);                                                                          \
                                 UNPL(_once) = false)

///
/// Iterate over each element var (as a pointer) of the given list l in reverse,
/// limited to index range [start, end) relative to the tail of the list.
/// Index 0 corresponds to the tail, 1 to the previous node, and so on.
///
/// The variable var is declared and defined by this macro as a pointer to the list's data type.
///
/// Since linked lists do not support indexing, this macro counts nodes from the tail
/// and includes only those where the relative reverse index lies in [start, end).
///
/// l[in,out] : List to iterate over.
/// var[out]  : Name of the pointer variable to be used which will point to the
///             current element during iteration.
/// start[in] : Starting index from tail (inclusive).
/// end[in]   : Ending index from tail (exclusive).
///
/// TAGS: Foreach, List, Iteration, Loop, Reverse, Range, Pointer
///
#define ListForeachPtrReverseInRange(l, var, start, end)                                                                                 \
    for (TYPE_OF(l) UNPL(pl) = (l); UNPL(pl); UNPL(pl) = NULL)                                                                           \
        if ((ValidateList(UNPL(pl)), 1) && UNPL(pl)->tail)                                                                               \
            for (GenericListNode * UNPL(node) = (GenericListNode *)ListNodeEnd(UNPL(pl)); UNPL(node);                                    \
                 UNPL(node)                   = ListNodePrev(UNPL(node)))                                                                \
                for (u64 UNPL(i) = 0; UNPL(node) && UNPL(i) < (end); UNPL(node) = ListNodePrev(UNPL(node)), ++UNPL(i)) \
                    if (UNPL(i) >= (start) && (UNPL(node)->data))                                                      \
                        for (bool UNPL(_once) = true; UNPL(_once); UNPL(_once) = false)                                \
                            for (LIST_DATA_TYPE(UNPL(pl)) *var = (LIST_DATA_TYPE(UNPL(pl)) *)(UNPL(node)->data);       \
                                 UNPL(_once);                                                                          \
                                 UNPL(_once) = false)

///
/// Iterate over each element `var` of the given list `l`, with index `idx`.
/// The variable `var` is declared and defined by this macro.
///
/// Iteration happens in forward order, starting from the head of the list.
/// This macro also tracks the index (`idx`) of each element during iteration.
///
/// `var` will contain a copy of the value pointed to by each list node,
/// and `idx` will be the zero-based index of the current element.
///
/// INFO: The macro supports iteration using relative traversal if random access is required.
///       This means user code can change `idx` to any value in list boundaries and the macro
///       will adjust node automatically for the new index.
///
/// l[in]     : List to iterate over.
/// var[out]  : Name of the variable that will hold the current value during iteration.
/// idx[out]  : Name of the variable that will hold the current index during iteration.
///
/// TAGS: Foreach, List, Iteration, Loop, Index
///
#define ListForeachIdx(l, var, idx)                                                                                    \
    for (TYPE_OF(l) UNPL(pl) = (l); UNPL(pl); UNPL(pl) = NULL)                                                         \
        if ((ValidateList(UNPL(pl)), 1) && UNPL(pl)->head)                                                             \
            for (u64 idx = 0, UNPL(pidx) = 0; idx < UNPL(pl)->length;)                                                 \
                for (GenericListNode * UNPL(node) = (GenericListNode *)ListNodeBegin(UNPL(pl)), *UNPL(next) = NULL;    \
                     UNPL(node) &&                                                                                     \
                     (UNPL(next) =                                                                                     \
                          (UNPL(pidx) ? (GenericListNode *)get_node_random_access(                                     \
                                            GENERIC_LIST(UNPL(pl)),                                                    \
                                            GENERIC_LIST_NODE(UNPL(node)),                                             \
                                            UNPL(pidx),                                                                \
                                            (i64)(idx) - (i64)UNPL(pidx)                                               \
                                        ) :                                                                            \
                                        (GenericListNode *)UNPL(pl)->head)) &&                                         \
                     UNPL(next) && UNPL(next)->data;                                                                   \
                     UNPL(pidx) = ++idx, UNPL(node) = UNPL(next))                                                      \
                    for (bool UNPL(_once) = true; UNPL(_once); UNPL(_once) = false)                                    \
                        for (LIST_DATA_TYPE(UNPL(pl)) var = *((LIST_DATA_TYPE(UNPL(pl)) *)(UNPL(next)->data));         \
                             UNPL(_once);                                                                              \
                             UNPL(_once) = false)

///
/// Iterate over each element `var` (as a pointer) of the given list `l`, with index `idx`.
/// The variable `var` is declared and defined by this macro as a pointer to the data.
///
/// Iteration happens in forward order, starting from the head of the list.
/// This macro also tracks the index (`idx`) of each element during iteration.
///
/// `var` will point to the data associated with the current list node,
/// and `idx` will be the zero-based index of the current element.
///
/// INFO: The macro supports iteration using relative traversal if random access is required.
///       This means user code can change `idx` to any value in list boundaries and the macro
///       will adjust node automatically for the new index.
///
/// l[in]     : List to iterate over.
/// var[out]  : Pointer variable that will point to the current element.
/// idx[out]  : Name of the variable that will hold the current index during iteration.
///
/// TAGS: Foreach, List, Iteration, Loop, Index, Pointer
///
#define ListForeachPtrIdx(l, var, idx)                                                                                 \
    for (TYPE_OF(l) UNPL(pl) = (l); UNPL(pl); UNPL(pl) = NULL)                                                         \
        if ((ValidateList(UNPL(pl)), 1) && UNPL(pl)->head)                                                             \
            for (u64 idx = 0, UNPL(pidx) = 0; idx < UNPL(pl)->length;)                                                 \
                for (GenericListNode * UNPL(node) = (GenericListNode *)ListNodeBegin(UNPL(pl)), *UNPL(next) = NULL;    \
                     UNPL(node) &&                                                                                     \
                     (UNPL(next) =                                                                                     \
                          (UNPL(pidx) ? (GenericListNode *)get_node_random_access(                                     \
                                            GENERIC_LIST(UNPL(pl)),                                                    \
                                            GENERIC_LIST_NODE(UNPL(node)),                                             \
                                            UNPL(pidx),                                                                \
                                            (i64)(idx) - (i64)UNPL(pidx)                                               \
                                        ) :                                                                            \
                                        (GenericListNode *)UNPL(pl)->head)) &&                                         \
                     UNPL(next) && UNPL(next)->data;                                                                   \
                     UNPL(pidx) = ++idx, UNPL(node) = UNPL(next))                                                      \
                    for (bool UNPL(_once) = true; UNPL(_once); UNPL(_once) = false)                                    \
                        for (LIST_DATA_TYPE(UNPL(pl)) *var = UNPL(next)->data; UNPL(_once); UNPL(_once) = false)

///
/// Iterate over each element `var` of the given list `l` in reverse order, with index `idx`.
/// The variable `var` is declared and defined by this macro.
///
/// Iteration starts from the tail and moves backward using the `prev` pointers.
/// The variable `idx` will contain the zero-based index from the head:
/// index length-1 corresponds to the tail, length-2 to the previous node, and so on down to 0 (head).
///
/// INFO: The macro supports iteration using relative traversal if random access is required.
///       This means user code can change `idx` to any value in list boundaries and the macro
///       will adjust node automatically for the new index.
///
/// l[in]     : List to iterate over.
/// var[out]  : Name of the variable to hold the value during iteration.
/// idx[out]  : Variable that will track the index from the head.
///
/// TAGS: Foreach, List, Iteration, Loop, Index, Reverse
///
#define ListForeachReverseIdx(l, var, idx)                                                                             \
    for (TYPE_OF(l) UNPL(pl) = (l); UNPL(pl); UNPL(pl) = NULL)                                                         \
        if ((ValidateList(UNPL(pl)), 1) && UNPL(pl)->tail && UNPL(pl)->length > 0)                                     \
            for (u64 idx            = UNPL(pl)->length - 1,                                                            \
                     UNPL(pidx)     = UNPL(pl)->length - 1,                                                            \
                     UNPL(first)    = 1,                                                                               \
                     UNPL(user_idx) = 0;                                                                               \
                 idx < UNPL(pl)->length;)                                                                              \
                for (GenericListNode * UNPL(node)  = (UNPL(first) ? (GenericListNode *)UNPL(pl)->tail : NULL),         \
                                       *UNPL(next) = NULL;                                                             \
                     UNPL(node) && idx < UNPL(pl)->length && (UNPL(first) = 0) &&                                      \
                     (UNPL(next) =                                                                                     \
                          (UNPL(pidx) != idx ? (GenericListNode *)get_node_random_access(                              \
                                                   GENERIC_LIST(UNPL(pl)),                                             \
                                                   GENERIC_LIST_NODE(UNPL(node)),                                      \
                                                   UNPL(pidx),                                                         \
                                                   (i64)(idx) - (i64)UNPL(pidx)                                        \
                                               ) :                                                                     \
                                               UNPL(node))) &&                                                         \
                     UNPL(next) && UNPL(next)->data;                                                                   \
                     UNPL(node) = UNPL(next), UNPL(pidx) = idx)                                                        \
                    for (bool UNPL(_once) = true; UNPL(_once); UNPL(_once) = false)                                    \
                        for (LIST_DATA_TYPE(UNPL(pl)) var = *((LIST_DATA_TYPE(UNPL(pl)) *)(UNPL(next)->data));         \
                             UNPL(_once);                                                                              \
                             UNPL(_once) = false)                                                                      \
                            for (bool UNPL(_update)  = true; UNPL(_update);                                            \
                                 UNPL(_update)       = false,                                                          \
                                      UNPL(user_idx) = idx,                                                            \
                                      idx =                                                                            \
                                          (UNPL(user_idx) != UNPL(pidx) ? UNPL(user_idx) :                             \
                                                                          (idx > 0 ? idx - 1 : UNPL(pl)->length)))

///
/// Iterate over each element `var` of the given list `l` in reverse order, with index `idx`.
/// The variable `var` is declared and defined by this macro.
///
/// Iteration starts from the tail and moves backward using the `prev` pointers.
/// The variable `idx` will contain the zero-based index from the head:
/// index length-1 corresponds to the tail, length-2 to the previous node, and so on down to 0 (head).
///
/// INFO: The macro supports iteration using relative traversal if random access is required.
///       This means user code can change `idx` to any value in list boundaries and the macro
///       will adjust node automatically for the new index.
///
/// l[in]     : List to iterate over.
/// var[out]  : Name of the variable to hold the pointer to the value during iteration.
/// idx[out]  : Variable that will track the index from the head.
///
/// TAGS: Foreach, List, Iteration, Loop, Index, Reverse
///
#define ListForeachPtrReverseIdx(l, var, idx)                                                                          \
    for (TYPE_OF(l) UNPL(pl) = (l); UNPL(pl); UNPL(pl) = NULL)                                                         \
        if ((ValidateList(UNPL(pl)), 1) && UNPL(pl)->tail && UNPL(pl)->length > 0)                                     \
            for (u64 idx            = UNPL(pl)->length - 1,                                                            \
                     UNPL(pidx)     = UNPL(pl)->length - 1,                                                            \
                     UNPL(first)    = 1,                                                                               \
                     UNPL(user_idx) = 0;                                                                               \
                 idx < UNPL(pl)->length;)                                                                              \
                for (GenericListNode * UNPL(node)  = (UNPL(first) ? (GenericListNode *)UNPL(pl)->tail : NULL),         \
                                       *UNPL(next) = NULL;                                                             \
                     UNPL(node) && idx < UNPL(pl)->length && (UNPL(first) = 0) &&                                      \
                     (UNPL(next) =                                                                                     \
                          (UNPL(pidx) != idx ? get_node_random_access(                                                 \
                                                   GENERIC_LIST(UNPL(pl)),                                             \
                                                   GENERIC_LIST_NODE(UNPL(node)),                                      \
                                                   UNPL(pidx),                                                         \
                                                   (i64)(idx) - (i64)(UNPL(pidx))                                      \
                                               ) :                                                                     \
                                               UNPL(node))) &&                                                         \
                     UNPL(next) && UNPL(next)->data;                                                                   \
                     UNPL(node) = UNPL(next), UNPL(pidx) = idx)                                                        \
                    for (bool UNPL(_once) = true; UNPL(_once); UNPL(_once) = false)                                    \
                        for (LIST_DATA_TYPE(UNPL(pl)) *var = UNPL(next)->data; UNPL(_once); UNPL(_once) = false)       \
                            for (bool UNPL(_update)  = true; UNPL(_update);                                            \
                                 UNPL(_update)       = false,                                                          \
                                      UNPL(user_idx) = idx,                                                            \
                                      idx =                                                                            \
                                          (UNPL(user_idx) != UNPL(pidx) ? UNPL(user_idx) :                             \
                                                                          (idx > 0 ? idx - 1 : UNPL(pl)->length)))

#endif // MISRA_STD_CONTAINER_LIST_FOREACH_H
