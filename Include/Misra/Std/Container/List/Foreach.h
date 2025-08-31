/// file      : Foreach.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// copyright : Copyright (c) 2025, Siddharth Mishra, All rights reserved.
///
/// List iterators.
///

#ifndef MISRA_STD_CONTAINER_LIST_FOREACH_H
#define MISRA_STD_CONTAINER_LIST_FOREACH_H

#define ListForeach(l, var, body)                                                                                      \
    do {                                                                                                               \
        ValidateList(l);                                                                                               \
        LIST_NODE_TYPE(l) *_node_##__LINE__ = NULL;                                                                    \
        LIST_DATA_TYPE(l) var               = {0};                                                                     \
        for (_node_##__LINE__ = (l)->head; _node_##__LINE__; _node_##__LINE__ = (_node_##__LINE__)->next) {            \
            if ((_node_##__LINE__)->next == _node_##__LINE__) {                                                        \
                LOG_FATAL("Node points to itself in 'next' field.");                                                   \
            }                                                                                                          \
            if ((_node_##__LINE__)->prev == _node_##__LINE__) {                                                        \
                LOG_FATAL("Node points to itself in 'prev' field.");                                                   \
            }                                                                                                          \
            if (!(_node_##__LINE__)->data) {                                                                           \
                LOG_FATAL("Invalid data in node.");                                                                    \
            }                                                                                                          \
            var = *(_node_##__LINE__)->data;                                                                           \
            {body};                                                                                                    \
        }                                                                                                              \
    } while (0)

#define ListForeachReverse(l, var, body)                                                                               \
    do {                                                                                                               \
        ValidateList(l);                                                                                               \
        LIST_NODE_TYPE(l) *_node_##__LINE__ = NULL;                                                                    \
        LIST_DATA_TYPE(l) var               = {0};                                                                     \
        for (_node_##__LINE__ = (l)->tail; _node_##__LINE__; _node_##__LINE__ = (_node_##__LINE__)->prev, ++idx) {     \
            if ((_node_##__LINE__)->next == _node_##__LINE__) {                                                        \
                LOG_FATAL("Node points to itself in 'next' field.");                                                   \
            }                                                                                                          \
            if ((_node_##__LINE__)->prev == _node_##__LINE__) {                                                        \
                LOG_FATAL("Node points to itself in 'prev' field.");                                                   \
            }                                                                                                          \
            if (!(_node_##__LINE__)->data) {                                                                           \
                LOG_FATAL("Invalid data in node.");                                                                    \
            }                                                                                                          \
            var = *(_node_##__LINE__)->data;                                                                           \
            {body};                                                                                                    \
        }                                                                                                              \
    } while (0)

#define ListForeachPtr(l, var, body)                                                                                   \
    do {                                                                                                               \
        ValidateList(l);                                                                                               \
        LIST_NODE_TYPE(l) *_node_##__LINE__ = NULL;                                                                    \
        LIST_DATA_TYPE(l) *var              = NULL;                                                                    \
        for (_node_##__LINE__ = (l)->head; _node_##__LINE__; _node_##__LINE__ = (_node_##__LINE__)->next, ++idx) {     \
            if ((_node_##__LINE__)->next == _node_##__LINE__) {                                                        \
                LOG_FATAL("Node points to itself in 'next' field.");                                                   \
            }                                                                                                          \
            if ((_node_##__LINE__)->prev == _node_##__LINE__) {                                                        \
                LOG_FATAL("Node points to itself in 'prev' field.");                                                   \
            }                                                                                                          \
            if (!(_node_##__LINE__)->data) {                                                                           \
                LOG_FATAL("Invalid data in node.");                                                                    \
            }                                                                                                          \
            var = (_node_##__LINE__)->data;                                                                            \
            {body};                                                                                                    \
        }                                                                                                              \
    } while (0)

#define ListForeachPtrReverse(l, var, body)                                                                            \
    do {                                                                                                               \
        ValidateList(l);                                                                                               \
        LIST_NODE_TYPE(l) *_node_##__LINE__ = NULL;                                                                    \
        LIST_DATA_TYPE(l) *var              = NULL;                                                                    \
        for (_node_##__LINE__ = (l)->tail; _node_##__LINE__; _node_##__LINE__ = (_node_##__LINE__)->prev, ++idx) {     \
            if ((_node_##__LINE__)->next == _node_##__LINE__) {                                                        \
                LOG_FATAL("Node points to itself in 'next' field.");                                                   \
            }                                                                                                          \
            if ((_node_##__LINE__)->prev == _node_##__LINE__) {                                                        \
                LOG_FATAL("Node points to itself in 'prev' field.");                                                   \
            }                                                                                                          \
            if (!(_node_##__LINE__)->data) {                                                                           \
                LOG_FATAL("Invalid data in node.");                                                                    \
            }                                                                                                          \
            var = (_node_##__LINE__)->data;                                                                            \
            {body};                                                                                                    \
        }                                                                                                              \
    } while (0)

#endif // MISRA_STD_CONTAINER_LIST_FOREACH_H
