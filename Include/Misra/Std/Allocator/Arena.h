/// file      : std/allocator/arena.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Bump (arena) allocator. Hands out memory linearly from page-backed
/// chunks and frees everything in one shot. Best fit for parser/scratch
/// workloads with batch-scoped lifetimes.

#ifndef MISRA_STD_ALLOCATOR_ARENA_H
#define MISRA_STD_ALLOCATOR_ARENA_H

#include <Misra/Std/Allocator.h>

#ifdef __cplusplus
extern "C" {
#endif

    ///
    /// Create an arena allocator descriptor.
    /// Each `AllocatorAlloc` call bumps a cursor inside the current chunk;
    /// when a chunk is exhausted, a new one is mapped through `PageAllocator`.
    /// `AllocatorFree` is a no-op for arena memory - everything is released
    /// together when the arena is unbound.
    ///
    /// The arena's runtime state is created lazily on the first allocation
    /// and torn down by `AllocatorUnbind`. The arena does not call libc
    /// heap functions.
    ///
    /// SUCCESS: Returns an arena allocator descriptor.
    /// FAILURE: Function cannot fail at creation time. Out-of-memory shows
    ///          up as a NULL from the first `AllocatorAlloc` call.
    ///
    /// USAGE:
    ///   Allocator scratch = ArenaAllocator();
    ///   Vec(Token) toks   = VecInit(scratch);
    ///   // ... build many small allocations cheaply ...
    ///   AllocatorUnbind(&scratch); // frees everything
    ///
    /// TAGS: Allocator, Arena, Bump, Initialization, Memory
    ///
    Allocator ArenaAllocator(void);

    ///
    /// Create an arena allocator with a custom alignment floor.
    /// Allocations are padded so the next bump is `alignment`-aligned.
    ///
    /// alignment[in]  : Required minimum alignment in bytes (power of two).
    ///
    /// SUCCESS: Returns a configured arena allocator descriptor.
    /// FAILURE: Function cannot fail at creation time.
    ///
    /// TAGS: Allocator, Arena, Aligned, Initialization
    ///
    Allocator ArenaAllocatorAligned(size alignment);

    ///
    /// Reset the arena cursor without freeing any chunks.
    /// All allocations made through `arena` become invalid, but the chunk
    /// list is kept so subsequent allocations are served without remapping.
    ///
    /// arena[in,out] : Arena allocator descriptor to reset.
    ///
    /// SUCCESS: Arena cursor is rewound to the start of the first chunk.
    /// FAILURE: No action is taken when `arena` is not an arena allocator
    ///          or has no live state.
    ///
    /// TAGS: Allocator, Arena, Reset, Memory
    ///
    void ArenaAllocatorReset(Allocator *arena);

#ifdef __cplusplus
}
#endif

#endif // MISRA_STD_ALLOCATOR_ARENA_H
