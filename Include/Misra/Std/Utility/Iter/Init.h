/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// copyright : Copyright (c) 2025, Siddharth Mishra, All rights reserved.

#ifndef MISRA_STD_UTILITY_ITER_INIT_H
#define MISRA_STD_UTILITY_ITER_INIT_H

#include "Type.h"

///
/// Initialize default `Iter` object to iterate in forward direction.
///
/// TAGS: Initialization, Memory
///
#define IterInit() {.data = NULL, .length = 0, .pos = 0, .alignment = 1, .dir = 1}

///
/// Initialize default `Iter` object to iterate in backward direction.
///
/// TAGS: Initialization, Memory
///
#define IterInitRev() {.data = NULL, .length = 0, .pos = 0, .alignment = 1, .dir = -1}

///
/// Initialize `Iter` with custom alignment to iterate in forward direction.
///
/// aln[in] : Alignment requirement
///
/// TAGS: Initialization, Memory
///
#define IterInitAligned(aln) {.data = NULL, .length = 0, .pos = 0, .alignment = (aln), .dir = 1}

///
/// Initialize `Iter` with custom alignment to iterate in backward direction.
///
/// aln[in] : Alignment requirement
///
/// TAGS: Initialization, Memory
///
#define IterInitRevAligned(aln) {.data = NULL, .length = 0, .pos = 0, .alignment = (aln), .dir = -1}

///
/// Initialize `Iter` from vector data to iterate in forward direction.
///
/// v[in] : Source vector
///
/// TAGS: Initialization, Container, Vector
///
#define IterInitFromVec(v) {.data = (v).data, .length = (v).length, .pos = 0, .alignment = (v).alignment, .dir = 1}

///
/// Initialize `Iter` from vector data to iterate in reverse direction.
///
/// v[in] : Source vector
///
/// TAGS: Initialization, Container, Vector
///
#define IterInitRevFromVec(v) {.data = (v).data, .length = (v).length, .pos = 0, .alignment = (v).alignment, .dir = -1}

///
/// Initialize default `Iter` object to iterate in forward direction.
///
/// i[in] : Variable or Type to be initialized.
///
/// TAGS: Initialization, Memory
///
#define IterInitT(i) ((__typeof__(i)) {.data = NULL, .length = 0, .pos = 0, .alignment = 1, .dir = 1})

///
/// Initialize default `Iter` object to iterate in backward direction.
///
/// i[in] : Variable or Type to be initialized.
///
/// TAGS: Initialization, Memory
///
#define IterInitRevT(i) ((__typeof__(i)) {.data = NULL, .length = 0, .pos = 0, .alignment = 1, .dir = -1})

///
/// Initialize `Iter` with custom alignment to iterate in forward direction.
///
/// i[in] : Variable or Type to be initialized.
/// aln[in] : Alignment requirement
///
/// TAGS: Initialization, Memory
///
#define IterInitAlignedT(i, aln) ((__typeof__(i)) {.data = NULL, .length = 0, .pos = 0, .alignment = (aln), .dir = 1})

///
/// Initialize `Iter` with custom alignment to iterate in backward direction.
///
/// i[in] : Variable or Type to be initialized.
/// aln[in] : Alignment requirement
///
/// TAGS: Initialization, Memory
///
#define IterInitRevAlignedT(i, aln)                                                                                    \
    ((__typeof__(i)) {.data = NULL, .length = 0, .pos = 0, .alignment = (aln), .dir = -1})

///
/// Initialize `Iter` from vector data to iterate in forward direction.
///
/// i[in] : Variable or Type to be initialized.
/// v[in] : Source vector
///
/// TAGS: Initialization, Container, Vector
///
#define IterInitFromVecT(i, v)                                                                                         \
    ((__typeof__(i)) {.data = (v).data, .length = (v).length, .pos = 0, .alignment = (v).alignment, .dir = 1})

///
/// Initialize `Iter` from vector data starting at back
///
/// i[in] : Variable or Type to be initialized.
/// v[in] : Source vector
///
/// TAGS: Initialization, Container, Vector
///
#define IterInitRevFromVecT(i, v)                                                                                      \
    ((__typeof__(i)) {.data = (v).data, .length = (v).length, .pos = 0, .alignment = (v).alignment, .dir = -1})

#endif // MISRA_STD_UTILITY_ITER_INIT_H
