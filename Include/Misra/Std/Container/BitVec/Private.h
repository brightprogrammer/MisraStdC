/// file      : std/container/bitvec/private.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Library-internal allocator-facing primitives for BitVec. These
/// snake_case names take a raw `Allocator *` directly, without the
/// `ALLOCATOR_OF` typed-pointer bridge that the public macros use.
///
/// They exist so library .c code can call into the init path with an
/// allocator it already holds (typically read from a container's
/// embedded `allocator` field) without bouncing through the typed
/// public macro. User code should prefer the public surface and is
/// not expected to include this header.

#ifndef MISRA_STD_CONTAINER_BITVEC_PRIVATE_H
#define MISRA_STD_CONTAINER_BITVEC_PRIVATE_H

#include "Type.h"
#include <Misra/Std/Allocator.h>

#ifdef __cplusplus
extern "C" {
#endif

///
/// Empty-bitvector struct literal bound to a raw allocator pointer.
///
#ifdef __cplusplus
#    define bitvec_init_alloc(alloc_ptr)                                                                               \
        (BitVec {.length    = 0,                                                                                       \
                 .capacity  = 0,                                                                                       \
                 .data      = NULL,                                                                                    \
                 .byte_size = 0,                                                                                       \
                 .allocator = (alloc_ptr),                                                                             \
                 .__magic   = MISRA_BITVEC_MAGIC})
#else
#    define bitvec_init_alloc(alloc_ptr)                                                                               \
        ((BitVec) {.length    = 0,                                                                                     \
                   .capacity  = 0,                                                                                     \
                   .data      = NULL,                                                                                  \
                   .byte_size = 0,                                                                                     \
                   .allocator = (alloc_ptr),                                                                           \
                   .__magic   = MISRA_BITVEC_MAGIC})
#endif

#ifdef __cplusplus
}
#endif

#endif // MISRA_STD_CONTAINER_BITVEC_PRIVATE_H
