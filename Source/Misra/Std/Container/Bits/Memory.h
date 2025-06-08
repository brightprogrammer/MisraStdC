/// file      : std/container/Bits/memory.h
/// author    : Generated following Misra project patterns
/// This is free and unencumbered software released into the public domain.
///
/// Memory management operations for Bitstors.

#ifndef MISRA_STD_CONTAINER_Bits_MEMORY_H
#define MISRA_STD_CONTAINER_Bits_MEMORY_H

#include "Type.h"
#include <Misra/Types.h>

#ifdef __cplusplus
extern "C" {
#endif

    ///
    /// Reduce Bitstor capacity to match its current length.
    /// Frees any unused memory allocated beyond the current length.
    ///
    /// bv[in] : Bitstor to shrink
    ///
    /// USAGE:
    ///   BitsShrinkToFit(&flags);  // Free unused capacity
    ///
    /// TAGS: Bits, Memory, Shrink, Optimize
    ///
    void BitsTryReduceSpace(Bits *bv);

    ///
    /// Create a deep copy of a Bitstor.
    /// The returned Bitstor must be deinitialized when no longer needed.
    ///
    /// bv[in] : Bitstor to clone
    ///
    /// RETURNS: Deep copy of the Bitstor
    ///
    /// USAGE:
    ///   Bits copy = BitsClone(&flags);
    ///   // ... use copy ...
    ///   BitsDeinit(&copy);
    ///
    /// TAGS: Bits, Memory, Clone, Copy
    ///
    Bits BitsClone(Bits *bv);

#ifdef __cplusplus
}
#endif

#endif // MISRA_STD_CONTAINER_Bits_MEMORY_H

