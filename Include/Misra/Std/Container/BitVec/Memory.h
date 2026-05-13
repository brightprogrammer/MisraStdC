/// file      : std/container/bitvec/memory.h
/// author    : Generated following Misra project patterns
/// This is free and unencumbered software released into the public domain.
///
/// Memory management operations for bitvectors.

#ifndef MISRA_STD_CONTAINER_BITVEC_MEMORY_H
#define MISRA_STD_CONTAINER_BITVEC_MEMORY_H

#include "Type.h"
#include <Misra/Types.h>

#ifdef __cplusplus
extern "C" {
#endif

    ///
    /// Reduce bitvector capacity to match its current length.
    /// Frees any unused memory allocated beyond the current length.
    ///
    /// bv[in] : Bitvector to shrink
    ///
    /// USAGE:
    ///   BitVecShrinkToFit(&flags);  // Free unused capacity
    ///
    /// TAGS: BitVec, Memory, Shrink, Optimize
    ///
    void BitVecShrinkToFit(BitVec *bv);



    ///
    /// Efficiently swap the contents of two bitvectors.
    /// This is much faster than copying both bitvectors.
    ///
    /// bv1[in] : First bitvector
    /// bv2[in] : Second bitvector
    ///
    /// USAGE:
    ///   BitVecSwap(&flags1, &flags2);  // Swap contents
    ///
    /// TAGS: BitVec, Memory, Swap, Efficient
    ///
    void BitVecSwap(BitVec *bv1, BitVec *bv2);

    ///
    /// Create a deep copy of a bitvector.
    /// The returned bitvector must be deinitialized when no longer needed.
    ///
    /// bv[in] : Bitvector to clone
    ///
    /// RETURNS: Deep copy of the bitvector
    ///
    /// USAGE:
    ///   BitVec copy = BitVecClone(&flags);
    ///   // ... use copy ...
    ///   BitVecDeinit(&copy);
    ///
    /// TAGS: BitVec, Memory, Clone, Copy
    ///
    bool BitVecTryClone(BitVec *out, BitVec *bv);
    BitVec BitVecClone(BitVec *bv);

#ifdef __cplusplus
}
#endif

#endif // MISRA_STD_CONTAINER_BITVEC_MEMORY_H
