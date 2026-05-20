/// file      : std/container/int/memory.h
/// author    : Generated following Misra project patterns
/// This is free and unencumbered software released into the public domain.
///
/// Copy-oriented helpers for Int.

#ifndef MISRA_STD_CONTAINER_INT_MEMORY_H
#define MISRA_STD_CONTAINER_INT_MEMORY_H

#include "Type.h"

#ifdef __cplusplus
extern "C" {
#endif

    ///
    /// Create a deep copy of an integer.
    ///
    /// out[in,out] : Destination Int that receives the clone. Must already
    ///               be a valid initialised Int; its existing storage is
    ///               deinitialised before the copy is installed.
    /// value[in]   : Source Int to clone. Not modified.
    ///
    /// SUCCESS : Returns `true`. `*out` holds an independent deep copy of
    ///           `value`'s bit-vector, normalised, and bound to `value`'s
    ///           allocator.
    /// FAILURE : Returns `false` on allocation failure while copying the
    ///           bit-vector. `*out` is left as a freshly initialised empty
    ///           Int bound to `value`'s allocator.
    ///
    /// TAGS: Int, Memory, Clone, Copy
    ///
    bool IntTryClone(Int *out, Int *value);

    ///
    /// Create a deep copy of an integer.
    ///
    /// value[in] : Integer to clone
    ///
    /// RETURNS: Independent copy of `value`.
    ///
    /// USAGE:
    ///   Int copy = IntClone(&value);
    ///
    /// TAGS: Int, Memory, Clone, Copy
    ///
    Int IntClone(Int *value);

#ifdef __cplusplus
}
#endif

#endif // MISRA_STD_CONTAINER_INT_MEMORY_H
