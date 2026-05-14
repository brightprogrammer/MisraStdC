/// file      : std/allocator/arena.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Per-descriptor bump (arena) allocator. Hands out memory linearly from
/// page-backed chunks. `AllocatorFree` is a no-op for ordinary allocations
/// (the arena reclaims everything at `ArenaAllocatorDeinit` time); the
/// most-recent allocation can be rewound. State is inline.

#ifndef MISRA_STD_ALLOCATOR_ARENA_H
#define MISRA_STD_ALLOCATOR_ARENA_H

#include <Misra/Std/Allocator.h>
#include <Misra/Std/Allocator/Page.h>

///
/// Per-type magic for `ArenaAllocator`. Stamped into
/// `Allocator.base.__magic` by `ArenaAllocatorInit*`. The arena
/// implementation functions validate this exact value so other
/// allocator instances reinterpreted as an `ArenaAllocator *` are
/// rejected at runtime as type-confusion.
///
#define MISRA_ARENA_ALLOCATOR_MAGIC MISRA_MAKE_NEW_MAGIC_VALUE("arenaalc")

#ifdef __cplusplus
extern "C" {
#endif

    typedef struct ArenaChunk ArenaChunk;

    typedef struct ArenaAllocator {
        Allocator     base;
        ArenaChunk   *head;
        ArenaChunk   *tail;
        char         *last_ptr;
        size          last_size;
        PageAllocator page;
    } ArenaAllocator;

    void *arena_allocator_allocate(Allocator *self, size bytes, i8 zeroed);
    void *arena_allocator_reallocate(Allocator *self, void *ptr, size old_size, size new_size);
    void  arena_allocator_deallocate(Allocator *self, void *ptr, size bytes);

    ///
    /// Release every chunk currently owned by `self`. After this call, the
    /// arena is back to its post-`ArenaAllocatorInit` state.
    ///
    void ArenaAllocatorDeinit(ArenaAllocator *self);

    ///
    /// Rewind the arena cursor without releasing chunks. All allocations
    /// previously made become invalid; subsequent allocations reuse the
    /// existing chunks.
    ///
    void ArenaAllocatorReset(ArenaAllocator *self);

#ifdef __cplusplus
}
#endif

#define ArenaAllocatorInit()                                                                                           \
    ((ArenaAllocator) {                                                                                                \
        .base =                                                                                                        \
            {.allocate    = arena_allocator_allocate,                                                                  \
                   .reallocate  = arena_allocator_reallocate,                                                                \
                   .deallocate  = arena_allocator_deallocate,                                                                \
                   .alignment   = 1,                                                                                         \
                   .effort      = ALLOCATOR_EFFORT_ONCE,                                                                     \
                   .retry_limit = 0,                                                                                         \
                   .__magic     = MISRA_ARENA_ALLOCATOR_MAGIC},                                                                  \
        .head      = NULL,                                                                                             \
        .tail      = NULL,                                                                                             \
        .last_ptr  = NULL,                                                                                             \
        .last_size = 0,                                                                                                \
        .page      = PageAllocatorInit()                                                                               \
    })

#define ArenaAllocatorInitAligned(N)                                                                                   \
    ((ArenaAllocator) {                                                                                                \
        .base =                                                                                                        \
            {.allocate    = arena_allocator_allocate,                                                                  \
                   .reallocate  = arena_allocator_reallocate,                                                                \
                   .deallocate  = arena_allocator_deallocate,                                                                \
                   .alignment   = (N) ? (N) : 1,                                                                             \
                   .effort      = ALLOCATOR_EFFORT_ONCE,                                                                     \
                   .retry_limit = 0,                                                                                         \
                   .__magic     = MISRA_ARENA_ALLOCATOR_MAGIC},                                                                  \
        .head      = NULL,                                                                                             \
        .tail      = NULL,                                                                                             \
        .last_ptr  = NULL,                                                                                             \
        .last_size = 0,                                                                                                \
        .page      = PageAllocatorInit()                                                                               \
    })

#endif // MISRA_STD_ALLOCATOR_ARENA_H
