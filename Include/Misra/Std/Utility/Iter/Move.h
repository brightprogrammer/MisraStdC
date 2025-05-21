/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// copyright : Copyright (c) 2025, Siddharth Mishra, All rights reserved.

#ifndef MISRA_STD_UTILITY_ITER_MOVE_H
#define MISRA_STD_UTILITY_ITER_MOVE_H

#include "Access.h"

///
/// Move current reading position of Iterator.
///
/// TAGS: Memory, Iter, Position
///
#define IterMove(mi, n)                                                                                                \
    do {                                                                                                               \
        if (((IterRemainingLength(mi) - (mi)->dir * (i64)(n) <= IterLength(mi)) &&                                     \
             (IterRemainingLength(mi) - (mi)->dir * (i64)(n) >= 0)))                                                   \
            (mi)->pos += (mi)->dir * (n);                                                                              \
    } while (0)

///
/// Move to next element (wrapper for `IterMove`)
///
/// TAGS: Iter, Memory, Position
///
#define IterNext(mi) IterMove(mi, 1)

///
/// Move to previous element (wrapper for `IterMove`)
///
/// TAGS: Iter, Memory, Position
///
#define IterPrev(mi) IterMove(mi, -1)

#endif // MISRA_STD_UTILITY_ITER_MOVE_H
