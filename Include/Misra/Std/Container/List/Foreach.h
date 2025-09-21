/// file      : Foreach.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// copyright : Copyright (c) 2025, Siddharth Mishra, All rights reserved.
///
/// List iterators.
///

#ifndef MISRA_STD_CONTAINER_LIST_FOREACH_H
#define MISRA_STD_CONTAINER_LIST_FOREACH_H

#define ListForeach(l, var)                                                                                            \
    for (TYPE_OF(l) UNPL(pl) = (l); UNPL(pl); UNPL(pl) = NULL)                                                         \
        if ((ValidateList(UNPL(pl)), 1) && (UNPL(pl)->head))                                                           \
            for (LIST_NODE_TYPE(UNPL(pl)) * UNPL(node) = UNPL(pl)->head; UNPL(node); UNPL(node) = UNPL(node)->next)    \
                if ((UNPL(node)->next != UNPL(node)) && (UNPL(node)->prev != UNPL(node)) && (UNPL(node)->data))        \
                    for (bool UNPL(_once) = true; UNPL(_once); UNPL(_once) = false)                                    \
                        for (LIST_DATA_TYPE(UNPL(pl)) var = *(UNPL(node)->data); UNPL(_once); UNPL(_once) = false)

#define ListForeachPtr(l, var)                                                                                         \
    for (TYPE_OF(l) UNPL(pl) = (l); UNPL(pl); UNPL(pl) = NULL)                                                         \
        if ((ValidateList(UNPL(pl)), 1) && (UNPL(pl)->head))                                                           \
            for (LIST_NODE_TYPE(UNPL(pl)) * UNPL(node) = UNPL(pl)->head; UNPL(node); UNPL(node) = UNPL(node)->next)    \
                if ((UNPL(node)->next != UNPL(node)) && (UNPL(node)->prev != UNPL(node)) && (UNPL(node)->data))        \
                    for (bool UNPL(_once) = true; UNPL(_once); UNPL(_once) = false)                                    \
                        for (LIST_DATA_TYPE(UNPL(pl)) *var = UNPL(node)->data; UNPL(_once); UNPL(_once) = false)

#define ListForeachReverse(l, var)                                                                                     \
    for (TYPE_OF(l) UNPL(pl) = (l); UNPL(pl); UNPL(pl) = NULL)                                                         \
        if ((ValidateList(UNPL(pl)), 1) && (UNPL(pl)->tail))                                                           \
            for (LIST_NODE_TYPE(UNPL(pl)) * UNPL(node) = UNPL(pl)->tail; UNPL(node); UNPL(node) = UNPL(node)->prev)    \
                if ((UNPL(node)->next != UNPL(node)) && (UNPL(node)->prev != UNPL(node)) && (UNPL(node)->data))        \
                    for (bool UNPL(_once) = true; UNPL(_once); UNPL(_once) = false)                                    \
                        for (LIST_DATA_TYPE(UNPL(pl)) var = *(UNPL(node)->data); UNPL(_once); UNPL(_once) = false)

#define ListForeachPtrReverse(l, var)                                                                                  \
    for (TYPE_OF(l) UNPL(pl) = (l); UNPL(pl); UNPL(pl) = NULL)                                                         \
        if ((ValidateList(UNPL(pl)), 1) && (UNPL(pl)->tail))                                                           \
            for (LIST_NODE_TYPE(UNPL(pl)) * UNPL(node) = UNPL(pl)->tail; UNPL(node); UNPL(node) = UNPL(node)->prev)    \
                if ((UNPL(node)->next != UNPL(node)) && (UNPL(node)->prev != UNPL(node)) && (UNPL(node)->data))        \
                    for (bool UNPL(_once) = true; UNPL(_once); UNPL(_once) = false)                                    \
                        for (LIST_DATA_TYPE(UNPL(pl)) *var = UNPL(node)->data; UNPL(_once); UNPL(_once) = false)



#endif // MISRA_STD_CONTAINER_LIST_FOREACH_H
