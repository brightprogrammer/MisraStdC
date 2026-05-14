/// file      : std/allocator/private.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Internal helpers shared by the allocator runtime and the bundled
/// allocator backends (Heap, Page, Arena, Pool). Not part of the public
/// API: user code should never call these directly.

#ifndef MISRA_STD_ALLOCATOR_PRIVATE_H
#define MISRA_STD_ALLOCATOR_PRIVATE_H

#include <Misra/Std/Allocator.h>

#ifdef __cplusplus
extern "C" {
#endif

    ///
    /// Ensure that an allocator's runtime state is initialized.
    /// If the allocator has no `state_init` callback, or already has a state
    /// object, this succeeds immediately. Used by the generic API entrypoints
    /// (`AllocatorAlloc`/`Realloc`) and by allocator backends that embed
    /// another allocator and need its state ready before forwarding calls
    /// (e.g. `ArenaAllocator`/`PoolAllocator` embedding `PageAllocator`).
    ///
    /// alloc[in,out] : Allocator to prepare for allocation.
    ///
    /// SUCCESS: Returns `true` when allocator state is ready.
    /// FAILURE: Returns `false` when state initialization fails or `alloc` is NULL.
    ///
    /// TAGS: Allocator, State, Internal
    ///
    bool allocator_ensure_state(Allocator *alloc);

#ifdef __cplusplus
}
#endif

#endif // MISRA_STD_ALLOCATOR_PRIVATE_H
