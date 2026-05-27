/// file      : std/container/float/memory.h
/// author    : Generated following Misra project patterns
/// This is free and unencumbered software released into the public domain.
///
/// Copy-oriented helpers for Float.

#ifndef MISRA_STD_CONTAINER_FLOAT_MEMORY_H
#define MISRA_STD_CONTAINER_FLOAT_MEMORY_H

#include "Type.h"

#ifdef __cplusplus
extern "C" {
#endif

    ///
    /// Create a deep copy of a floating-point value.
    ///
    /// out[in,out] : Destination Float that receives the clone. Must
    ///               already be a valid initialised Float; its existing
    ///               storage is deinitialised before the copy is installed.
    /// value[in]   : Source Float to clone. Not modified.
    ///
    /// SUCCESS : Returns `true`. `*out` holds an independent deep copy of
    ///           `value` (sign, significand bits, and exponent) bound to
    ///           `value`'s allocator.
    /// FAILURE : Returns `false` on allocation failure while copying the
    ///           significand. `*out` is left as a freshly initialised empty
    ///           Float bound to `value`'s allocator.
    ///
    /// TAGS: Float, Memory, Clone, Copy
    ///
    bool FloatTryClone(Float *out, Float *value);

    ///
    /// Create a deep copy of a floating-point value.
    ///
    /// value[in] : Float to clone
    ///
    /// SUCCESS : Returns an independent deep copy of `value` (sign,
    ///           significand bits, exponent) bound to `value`'s
    ///           allocator. `value` is not modified.
    /// FAILURE : Returns a freshly initialised empty `Float` bound to
    ///           `value`'s allocator on allocation failure during the
    ///           significand copy. The caller cannot distinguish that
    ///           from a true zero result; use `FloatTryClone` directly
    ///           when explicit failure propagation is required.
    ///
    /// USAGE:
    ///   Float copy = FloatClone(&value);
    ///
    /// TAGS: Float, Memory, Clone, Copy
    ///
    Float FloatClone(Float *value);

#ifdef __cplusplus
}
#endif

#endif // MISRA_STD_CONTAINER_FLOAT_MEMORY_H
