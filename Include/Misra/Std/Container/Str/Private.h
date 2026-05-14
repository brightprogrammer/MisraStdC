/// file      : std/container/str/private.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Library-internal allocator-facing primitives for Str. These
/// snake_case names take a raw `Allocator *` directly, without the
/// `ALLOCATOR_OF` typed-pointer bridge that the public macros use.
///
/// They exist so library .c code can call into the init path with an
/// allocator it already holds (typically read from a container's
/// embedded `allocator` field) without bouncing through the typed
/// public macro. User code should prefer the public surface and is
/// not expected to include this header.

#ifndef MISRA_STD_CONTAINER_STR_PRIVATE_H
#define MISRA_STD_CONTAINER_STR_PRIVATE_H

#include "Type.h"
#include <Misra/Std/Allocator.h>
#include <Misra/Std/Container/Vec/Private.h>

#ifdef __cplusplus
extern "C" {
#endif

///
/// Empty-Str struct literal bound to a raw allocator pointer.
/// Equivalent to the public `StrInit(typed_alloc_ptr)` macro but takes
/// a raw `Allocator *` rather than a typed allocator handle.
///
#ifdef __cplusplus
#    define str_init_alloc(alloc_ptr) (Str vec_init_alloc(alloc_ptr))
#else
#    define str_init_alloc(alloc_ptr) ((Str)vec_init_alloc(alloc_ptr))
#endif

#ifdef __cplusplus
}
#endif

#endif // MISRA_STD_CONTAINER_STR_PRIVATE_H
