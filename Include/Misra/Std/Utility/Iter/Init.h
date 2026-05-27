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
/// Alignment is taken from the vector's allocator; stack-init vecs
/// (NULL allocator) have no alignment requirement on the data layout
/// and collapse to alignment 1, which gives an unpadded
/// `sizeof(T)` stride that matches the `_Alignas(T) char[]` backing
/// buffer planted by `VecInitStack`.
///
/// v[in] : Source vector (by value -- the resulting iter holds a
///         non-owning pointer into `v`'s buffer).
///
/// SUCCESS : Always succeeds; returns a struct-literal `Iter` that
///           reads `v.length` elements starting at `v.data`.
/// FAILURE : Macro cannot fail. Reading from the resulting iter
///           when `v.data == NULL` simply has remaining-length 0.
///
/// TAGS: Initialization, Container, Vector
///
#define IterInitFromVec(v)                                                                                             \
    {.data      = (v).data,                                                                                            \
     .length    = (v).length,                                                                                          \
     .pos       = 0,                                                                                                   \
     .alignment = (v).allocator ? (v).allocator->alignment : 1,                                                        \
     .dir       = 1}

///
/// Initialize `Iter` from vector data to iterate in reverse direction.
/// See `IterInitFromVec` for the NULL-allocator handling.
///
/// v[in] : Source vector
///
/// SUCCESS : Always succeeds; returns a struct-literal `Iter` whose
///           cursor advances backwards through `v`.
/// FAILURE : Macro cannot fail.
///
/// TAGS: Initialization, Container, Vector
///
#define IterInitRevFromVec(v)                                                                                          \
    {.data      = (v).data,                                                                                            \
     .length    = (v).length,                                                                                          \
     .pos       = 0,                                                                                                   \
     .alignment = (v).allocator ? (v).allocator->alignment : 1,                                                        \
     .dir       = -1}

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
/// Typed-cast variant of `IterInitFromVec` for assigning into a typed
/// iter variable. See `IterInitFromVec` for the NULL-allocator handling
/// and overall semantics.
///
/// i[in] : Variable or Type to be initialized.
/// v[in] : Source vector
///
/// SUCCESS : Always succeeds; returns a typed compound-literal `Iter`.
/// FAILURE : Macro cannot fail.
///
/// TAGS: Initialization, Container, Vector
///
#define IterInitFromVecT(i, v)                                                                                         \
    ((TYPE_OF(i)) {.data      = (v).data,                                                                              \
                   .length    = (v).length,                                                                            \
                   .pos       = 0,                                                                                     \
                   .alignment = (v).allocator ? (v).allocator->alignment : 1,                                          \
                   .dir       = 1})

///
/// Typed-cast variant of `IterInitRevFromVec`. See `IterInitFromVec`
/// for the NULL-allocator handling.
///
/// i[in] : Variable or Type to be initialized.
/// v[in] : Source vector
///
/// SUCCESS : Always succeeds; returns a typed compound-literal `Iter`
///           that iterates backwards.
/// FAILURE : Macro cannot fail.
///
/// TAGS: Initialization, Container, Vector
///
#define IterInitRevFromVecT(i, v)                                                                                      \
    ((TYPE_OF(i)) {.data      = (v).data,                                                                              \
                   .length    = (v).length,                                                                            \
                   .pos       = 0,                                                                                     \
                   .alignment = (v).allocator ? (v).allocator->alignment : 1,                                          \
                   .dir       = -1})

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
