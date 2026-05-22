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
#define ARENA_ALLOCATOR_MAGIC MAKE_NEW_MAGIC_VALUE("arenaalc")

#ifdef __cplusplus
extern "C" {
#endif

    typedef struct ArenaChunk ArenaChunk;

    typedef struct ArenaAllocator {
        Allocator     base;
        ArenaChunk   *head;
        ArenaChunk   *tail;
        u8           *last_ptr;
        size          last_size;
        PageAllocator page;
    } ArenaAllocator;

    void *arena_allocator_allocate(Allocator *self, size bytes, i8 zeroed);
    i8    arena_allocator_resize(Allocator *self, void *ptr, size new_size);
    void *arena_allocator_remap(Allocator *self, void *ptr, size new_size);
    size  arena_allocator_deallocate(Allocator *self, void *ptr);

    ///
    /// Release every chunk currently owned by `self`. Walks the
    /// chunk list and frees each one through the embedded
    /// `PageAllocator`, then zeroes the struct so any post-deinit
    /// dispatch trips `ValidateAllocator` on the cleared `__magic`.
    ///
    /// self[in,out] : ArenaAllocator instance, or NULL.
    ///
    /// SUCCESS: Function returns. Every previously-handed-out pointer
    ///          is invalid; the arena is back to its post-Init zero
    ///          state and cannot be used until re-initialised.
    /// FAILURE: No action when `self` is NULL.
    ///
    /// TAGS: Allocator, Arena, Cleanup
    ///
    void ArenaAllocatorDeinit(ArenaAllocator *self);

    ///
    /// Rewind every chunk's bump cursor and clear the most-recent
    /// allocation snapshot, without releasing the chunks themselves.
    /// Existing kernel mappings are kept so subsequent allocations
    /// can reuse them without going back to `mmap`.
    ///
    /// self[in,out] : ArenaAllocator instance, or NULL.
    ///
    /// SUCCESS: Function returns. Every previously-handed-out pointer
    ///          is invalid; `last_ptr` / `last_size` are cleared so
    ///          resize-the-most-recent-bump has no rollback target.
    /// FAILURE: No action when `self` is NULL.
    ///
    /// TAGS: Allocator, Arena, Reset
    ///
    void ArenaAllocatorReset(ArenaAllocator *self);

#ifdef __cplusplus
}
#endif

#define ArenaAllocatorInit()                                                                                           \
    ((ArenaAllocator) {                                                                                                \
        .base =                                                                                                        \
            {.allocate    = arena_allocator_allocate,                                                                  \
                   .resize      = arena_allocator_resize,                                                                    \
                   .remap       = arena_allocator_remap,                                                                     \
                   .deallocate  = arena_allocator_deallocate,                                                                \
                   .alignment   = 1,                                                                                         \
                   .effort      = ALLOCATOR_EFFORT_ONCE,                                                                     \
                   .retry_limit = 0,                                                                                         \
                   .__magic     = ARENA_ALLOCATOR_MAGIC},                                                                        \
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
                   .resize      = arena_allocator_resize,                                                                    \
                   .remap       = arena_allocator_remap,                                                                     \
                   .deallocate  = arena_allocator_deallocate,                                                                \
                   .alignment   = (N) ? (N) : 1,                                                                             \
                   .effort      = ALLOCATOR_EFFORT_ONCE,                                                                     \
                   .retry_limit = 0,                                                                                         \
                   .__magic     = ARENA_ALLOCATOR_MAGIC},                                                                        \
        .head      = NULL,                                                                                             \
        .tail      = NULL,                                                                                             \
        .last_ptr  = NULL,                                                                                             \
        .last_size = 0,                                                                                                \
        .page      = PageAllocatorInit()                                                                               \
    })

#endif // MISRA_STD_ALLOCATOR_ARENA_H
