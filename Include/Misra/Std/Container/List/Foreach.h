/// file      : std/container/list/foreach.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
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
/// SUCCESS : The loop body runs once for each node from head to tail
///           with `var` bound to a copy of that node's data. The body
///           is skipped when `l` is empty. The list is not modified by
///           the macro itself.
/// FAILURE : The macro itself does not fail. `LOG_FATAL` via
///           `ValidateList(l)` when `l` is uninitialised or corrupted.
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
/// SUCCESS : The loop body runs once for each node from head to tail
///           with `var` bound to the in-node data address. Use this
///           form when the body mutates elements in place. The body is
///           skipped when `l` is empty.
/// FAILURE : The macro itself does not fail. `LOG_FATAL` via
///           `ValidateList(l)` when `l` is uninitialised or corrupted.
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
/// SUCCESS : The loop body runs once for each node from tail to head
///           with `var` bound to a copy of that node's data. The body
///           is skipped when `l` is empty.
/// FAILURE : The macro itself does not fail. `LOG_FATAL` via
///           `ValidateList(l)` when `l` is uninitialised or corrupted.
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
/// SUCCESS : The loop body runs once for each node from tail to head
///           with `var` bound to the in-node data address. Use this
///           form when the body mutates elements in place. The body is
///           skipped when `l` is empty.
/// FAILURE : The macro itself does not fail. `LOG_FATAL` via
///           `ValidateList(l)` when `l` is uninitialised or corrupted.
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
/// SUCCESS : The loop body runs once for each node whose head-relative
///           index lies in `[start, end)`, with `var` bound to a copy
///           of that node's data. The body is skipped when the range
///           is empty or `l` is empty.
/// FAILURE : The macro itself does not fail. `LOG_FATAL` via
///           `ValidateList(l)` when `l` is uninitialised or corrupted.
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
/// SUCCESS : The loop body runs once for each node whose head-relative
///           index lies in `[start, end)`, with `var` bound to the
///           in-node data address. Use this form when the body mutates
///           elements in place. The body is skipped when the range or
///           `l` is empty.
/// FAILURE : The macro itself does not fail. `LOG_FATAL` via
///           `ValidateList(l)` when `l` is uninitialised or corrupted.
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
/// SUCCESS : The loop body runs once for each node whose tail-relative
///           index lies in `[start, end)` while walking tail-to-head,
///           with `var` bound to a copy of that node's data. The body
///           is skipped when the range is empty or `l` is empty.
/// FAILURE : The macro itself does not fail. `LOG_FATAL` via
///           `ValidateList(l)` when `l` is uninitialised or corrupted.
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
/// SUCCESS : The loop body runs once for each node whose tail-relative
///           index lies in `[start, end)` while walking tail-to-head,
///           with `var` bound to the in-node data address. Use this
///           form when the body mutates elements in place. The body is
///           skipped when the range or `l` is empty.
/// FAILURE : The macro itself does not fail. `LOG_FATAL` via
///           `ValidateList(l)` when `l` is uninitialised or corrupted.
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
/// SUCCESS : The loop body runs once for each node from head to tail
///           with `idx` advancing from `0` to `l->length - 1` and
///           `var` bound to a copy of `l[idx]`'s data. The body may
///           reassign `idx` to perform random access; the macro will
///           reposition to that node. The body is skipped when `l` is
///           empty.
/// FAILURE : The macro itself does not fail. `LOG_FATAL` via
///           `ValidateList(l)` when `l` is uninitialised or corrupted.
///
/// TAGS: Foreach, List, Iteration, Loop, Index
///
#define ListForeachIdx(l, var, idx)                                                                                      \
    for (TYPE_OF(l) UNPL(pl) = (l); UNPL(pl); UNPL(pl) = NULL)                                                           \
        if ((ValidateList(UNPL(pl)), 1) && UNPL(pl)->head)                                                               \
            for (GenericListNode * UNPL(cursor) = NULL, *UNPL(node) = NULL; UNPL(pl); UNPL(pl) = NULL)                 \
                for (u64 idx = 0, UNPL(cursor_idx) = 0, UNPL(resolved_idx) = 0; idx < UNPL(pl)->length;                \
                     UNPL(cursor) = UNPL(node), UNPL(cursor_idx) = UNPL(resolved_idx),                                  \
                         idx = ((idx != UNPL(resolved_idx)) ? idx : (UNPL(resolved_idx) + 1)))                          \
                    if ((UNPL(node) = get_node_for_list_iteration(                                                       \
                             GENERIC_LIST(UNPL(pl)), UNPL(cursor), UNPL(cursor_idx), idx)) &&                           \
                        ((UNPL(resolved_idx) = idx), 1) && UNPL(node)->data)                                             \
                        for (bool UNPL(_once) = true; UNPL(_once); UNPL(_once) = false)                                  \
                            for (LIST_DATA_TYPE(UNPL(pl)) var = *((LIST_DATA_TYPE(UNPL(pl)) *)(UNPL(node)->data));      \
                                 UNPL(_once);                                                                            \
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
/// SUCCESS : The loop body runs once for each node from head to tail
///           with `idx` advancing from `0` to `l->length - 1` and
///           `var` bound to the in-node data address. Use this form
///           when the body mutates elements in place. The body may
///           reassign `idx` to perform random access. The body is
///           skipped when `l` is empty.
/// FAILURE : The macro itself does not fail. `LOG_FATAL` via
///           `ValidateList(l)` when `l` is uninitialised or corrupted.
///
/// TAGS: Foreach, List, Iteration, Loop, Index, Pointer
///
#define ListForeachPtrIdx(l, var, idx)                                                                                   \
    for (TYPE_OF(l) UNPL(pl) = (l); UNPL(pl); UNPL(pl) = NULL)                                                           \
        if ((ValidateList(UNPL(pl)), 1) && UNPL(pl)->head)                                                               \
            for (GenericListNode * UNPL(cursor) = NULL, *UNPL(node) = NULL; UNPL(pl); UNPL(pl) = NULL)                 \
                for (u64 idx = 0, UNPL(cursor_idx) = 0, UNPL(resolved_idx) = 0; idx < UNPL(pl)->length;                \
                     UNPL(cursor) = UNPL(node), UNPL(cursor_idx) = UNPL(resolved_idx),                                  \
                         idx = ((idx != UNPL(resolved_idx)) ? idx : (UNPL(resolved_idx) + 1)))                          \
                    if ((UNPL(node) = get_node_for_list_iteration(                                                       \
                             GENERIC_LIST(UNPL(pl)), UNPL(cursor), UNPL(cursor_idx), idx)) &&                           \
                        ((UNPL(resolved_idx) = idx), 1) && UNPL(node)->data)                                             \
                        for (bool UNPL(_once) = true; UNPL(_once); UNPL(_once) = false)                                  \
                            for (LIST_DATA_TYPE(UNPL(pl)) *var = (LIST_DATA_TYPE(UNPL(pl)) *)UNPL(node)->data;          \
                                 UNPL(_once);                                                                            \
                                 UNPL(_once) = false)

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
/// SUCCESS : The loop body runs once for each node walking tail to
///           head with `idx` counting down from `l->length - 1` to `0`
///           and `var` bound to a copy of `l[idx]`'s data. The body may
///           reassign `idx` to perform random access. The body is
///           skipped when `l` is empty.
/// FAILURE : The macro itself does not fail. `LOG_FATAL` via
///           `ValidateList(l)` when `l` is uninitialised or corrupted.
///
/// TAGS: Foreach, List, Iteration, Loop, Index, Reverse
///
#define ListForeachReverseIdx(l, var, idx)                                                                               \
    for (TYPE_OF(l) UNPL(pl) = (l); UNPL(pl); UNPL(pl) = NULL)                                                           \
        if ((ValidateList(UNPL(pl)), 1) && UNPL(pl)->tail && UNPL(pl)->length > 0)                                      \
            for (GenericListNode * UNPL(cursor) = NULL, *UNPL(node) = NULL; UNPL(pl); UNPL(pl) = NULL)                 \
                for (u64 idx = UNPL(pl)->length - 1, UNPL(cursor_idx) = 0, UNPL(resolved_idx) = 0;                     \
                     idx < UNPL(pl)->length;                                                                            \
                     UNPL(cursor) = UNPL(node), UNPL(cursor_idx) = UNPL(resolved_idx),                                  \
                         idx = ((idx != UNPL(resolved_idx)) ? idx :                                                      \
                                                               (UNPL(resolved_idx) > 0 ? UNPL(resolved_idx) - 1        \
                                                                                        : UNPL(pl)->length)))            \
                    if ((UNPL(node) = get_node_for_list_iteration(                                                       \
                             GENERIC_LIST(UNPL(pl)), UNPL(cursor), UNPL(cursor_idx), idx)) &&                           \
                        ((UNPL(resolved_idx) = idx), 1) && UNPL(node)->data)                                             \
                        for (bool UNPL(_once) = true; UNPL(_once); UNPL(_once) = false)                                  \
                            for (LIST_DATA_TYPE(UNPL(pl)) var = *((LIST_DATA_TYPE(UNPL(pl)) *)(UNPL(node)->data));      \
                                 UNPL(_once);                                                                            \
                                 UNPL(_once) = false)

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
/// SUCCESS : The loop body runs once for each node walking tail to
///           head with `idx` counting down from `l->length - 1` to `0`
///           and `var` bound to the in-node data address. Use this
///           form when the body mutates elements in place. The body
///           may reassign `idx` to perform random access. The body is
///           skipped when `l` is empty.
/// FAILURE : The macro itself does not fail. `LOG_FATAL` via
///           `ValidateList(l)` when `l` is uninitialised or corrupted.
///
/// TAGS: Foreach, List, Iteration, Loop, Index, Reverse, Pointer
///
#define ListForeachPtrReverseIdx(l, var, idx)                                                                            \
    for (TYPE_OF(l) UNPL(pl) = (l); UNPL(pl); UNPL(pl) = NULL)                                                           \
        if ((ValidateList(UNPL(pl)), 1) && UNPL(pl)->tail && UNPL(pl)->length > 0)                                      \
            for (GenericListNode * UNPL(cursor) = NULL, *UNPL(node) = NULL; UNPL(pl); UNPL(pl) = NULL)                 \
                for (u64 idx = UNPL(pl)->length - 1, UNPL(cursor_idx) = 0, UNPL(resolved_idx) = 0;                     \
                     idx < UNPL(pl)->length;                                                                            \
                     UNPL(cursor) = UNPL(node), UNPL(cursor_idx) = UNPL(resolved_idx),                                  \
                         idx = ((idx != UNPL(resolved_idx)) ? idx :                                                      \
                                                               (UNPL(resolved_idx) > 0 ? UNPL(resolved_idx) - 1        \
                                                                                        : UNPL(pl)->length)))            \
                    if ((UNPL(node) = get_node_for_list_iteration(                                                       \
                             GENERIC_LIST(UNPL(pl)), UNPL(cursor), UNPL(cursor_idx), idx)) &&                           \
                        ((UNPL(resolved_idx) = idx), 1) && UNPL(node)->data)                                             \
                        for (bool UNPL(_once) = true; UNPL(_once); UNPL(_once) = false)                                  \
                            for (LIST_DATA_TYPE(UNPL(pl)) *var = (LIST_DATA_TYPE(UNPL(pl)) *)UNPL(node)->data;          \
                                 UNPL(_once);                                                                            \
                                 UNPL(_once) = false)

#endif // MISRA_STD_CONTAINER_LIST_FOREACH_H
