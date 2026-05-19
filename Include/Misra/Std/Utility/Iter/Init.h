/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.

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
#define IterInitFromVec(v)                                                                                             \
    {.data = (v).data, .length = (v).length, .pos = 0, .alignment = (v).allocator->alignment, .dir = 1}

///
/// Initialize `Iter` from vector data to iterate in reverse direction.
///
/// v[in] : Source vector
///
/// TAGS: Initialization, Container, Vector
///
#define IterInitRevFromVec(v)                                                                                          \
    {.data = (v).data, .length = (v).length, .pos = 0, .alignment = (v).allocator->alignment, .dir = -1}

///
/// Initialize default `Iter` object to iterate in forward direction.
///
/// i[in] : Variable or Type to be initialized.
///
/// TAGS: Initialization, Memory
///
#define IterInitT(i) ((TYPE_OF(i)) {.data = NULL, .length = 0, .pos = 0, .alignment = 1, .dir = 1})

///
/// Initialize default `Iter` object to iterate in backward direction.
///
/// i[in] : Variable or Type to be initialized.
///
/// TAGS: Initialization, Memory
///
#define IterInitRevT(i) ((TYPE_OF(i)) {.data = NULL, .length = 0, .pos = 0, .alignment = 1, .dir = -1})

///
/// Initialize `Iter` with custom alignment to iterate in forward direction.
///
/// i[in] : Variable or Type to be initialized.
/// aln[in] : Alignment requirement
///
/// TAGS: Initialization, Memory
///
#define IterInitAlignedT(i, aln) ((TYPE_OF(i)) {.data = NULL, .length = 0, .pos = 0, .alignment = (aln), .dir = 1})

///
/// Initialize `Iter` with custom alignment to iterate in backward direction.
///
/// i[in] : Variable or Type to be initialized.
/// aln[in] : Alignment requirement
///
/// TAGS: Initialization, Memory
///
#define IterInitRevAlignedT(i, aln) ((TYPE_OF(i)) {.data = NULL, .length = 0, .pos = 0, .alignment = (aln), .dir = -1})

///
/// Initialize `Iter` from vector data to iterate in forward direction.
///
/// i[in] : Variable or Type to be initialized.
/// v[in] : Source vector
///
/// TAGS: Initialization, Container, Vector
///
#define IterInitFromVecT(i, v)                                                                                         \
    ((TYPE_OF(i)) {.data = (v).data, .length = (v).length, .pos = 0, .alignment = (v).allocator->alignment, .dir = 1})

///
/// Initialize `Iter` from vector data starting at back
///
/// i[in] : Variable or Type to be initialized.
/// v[in] : Source vector
///
/// TAGS: Initialization, Container, Vector
///
#define IterInitRevFromVecT(i, v)                                                                                      \
    ((TYPE_OF(i)) {.data = (v).data, .length = (v).length, .pos = 0, .alignment = (v).allocator->alignment, .dir = -1})

///
/// Carve a child iterator from a parent. The child starts at the
/// parent's current position with length `n` (so its valid range is
/// the parent's `[pos, pos + n)`), pos `0`, inheriting the parent's
/// alignment and direction. The parent is unchanged -- after the
/// sub-read returns, the parent can keep iterating from where it
/// left off.
///
/// parent[in] : Source iterator. Must outlive the child read.
/// n[in]      : Number of elements the child can read.
///
/// TAGS: Initialization, Iter, Subview
///
#define IterCarve(parent, n)                                                                                           \
    ((TYPE_OF(*(parent))) {.data      = (parent)->data + (parent)->pos,                                                \
                           .length    = (n),                                                                           \
                           .pos       = 0,                                                                             \
                           .alignment = (parent)->alignment,                                                           \
                           .dir       = (parent)->dir})

#endif // MISRA_STD_UTILITY_ITER_INIT_H
