/// file      : std/container/bitvec/memory.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
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
    /// bv[in,out] : Bitvector to shrink.
    ///
    /// SUCCESS : Returns to the caller. Capacity now equals length; any
    ///           over-allocated tail bytes have been returned to the
    ///           allocator. The values of stored bits are preserved.
    /// FAILURE : Function cannot fail in an observable way - if the
    ///           shrink-reallocation fails, the bitvector is left
    ///           unchanged but the caller is not informed (the helper
    ///           silently keeps the larger buffer because the data is
    ///           still consistent).
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
    /// bv1[in,out] : First bitvector.
    /// bv2[in,out] : Second bitvector.
    ///
    /// SUCCESS : Returns to the caller. The data pointer, length,
    ///           capacity, byte_size, and allocator binding of `bv1` and
    ///           `bv2` have been exchanged. No allocations are performed
    ///           and the stored bits are not copied.
    /// FAILURE : Function cannot fail. NULL pointers or invalid magic on
    ///           either bitvector are caller bugs and abort via
    ///           `LOG_FATAL`.
    ///
    /// USAGE:
    ///   BitVecSwap(&flags1, &flags2);  // Swap contents
    ///
    /// TAGS: BitVec, Memory, Swap, Efficient
    ///
    void BitVecSwap(BitVec *bv1, BitVec *bv2);

    ///
    /// Try to create a deep copy of a bitvector.
    /// The returned bitvector must be deinitialized when no longer needed.
    ///
    /// out[out] : Destination bitvector.
    /// bv[in]   : Source bitvector to clone.
    ///
    /// SUCCESS : Returns `true`. `*out` now holds a deep copy of `bv`:
    ///           same length, same bits, capacity sized for the length,
    ///           and the source's allocator binding. The source is
    ///           untouched.
    /// FAILURE : Returns `false` on allocation failure for the new byte
    ///           buffer. `*out` is left in an initialized-but-empty
    ///           state (length 0, capacity 0, data NULL) so the caller
    ///           can safely deinit it.
    ///
    /// TAGS: BitVec, Memory, Clone, Copy, Fallible
    ///
    bool BitVecTryClone(BitVec *out, BitVec *bv);

    ///
    /// Create a deep copy of a bitvector.
    /// The returned bitvector must be deinitialized when no longer needed.
    ///
    /// bv[in] : Bitvector to clone.
    ///
    /// SUCCESS : Returns a fresh `BitVec` holding a deep copy of `bv`:
    ///           same length, same bits, capacity sized for the length,
    ///           inheriting the source's allocator binding. The source
    ///           is untouched.
    /// FAILURE : Returns an empty bitvector (length 0, capacity 0, data
    ///           NULL, allocator still bound) when allocation fails.
    ///           Use `BitVecTryClone` if you need explicit failure
    ///           propagation.
    ///
    /// USAGE:
    ///   BitVec copy = BitVecClone(&flags);
    ///   // ... use copy ...
    ///   BitVecDeinit(&copy);
    ///
    /// TAGS: BitVec, Memory, Clone, Copy
    ///
    BitVec BitVecClone(BitVec *bv);

#ifdef __cplusplus
}
#endif

#endif // MISRA_STD_CONTAINER_BITVEC_MEMORY_H
