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
        Allocator   base;
        ArenaChunk *head;
        ArenaChunk *tail;
        u8         *last_ptr;
        size        last_size;
    } ArenaAllocator;

    ///
    /// Bump-allocate from the tail chunk. When the tail chunk has no
    /// room for the aligned request the arena links in a fresh,
    /// page-backed chunk sized to fit. The returned pointer is also
    /// recorded as the most-recent-allocation snapshot so a subsequent
    /// `resize` of the same pointer can rewind / extend the bump.
    ///
    /// SUCCESS: Returns a writable, alignment-correct pointer to at
    ///          least `bytes` bytes. Zeroed when `zeroed` is non-zero.
    /// FAILURE: Returns NULL when chunk allocation fails (the kernel
    ///          page map for a fresh chunk could not be obtained).
    ///
    /// TAGS: Allocator, Arena, Memory, Allocation
    ///
    void *arena_allocator_allocate(ArenaAllocator *self, size bytes, i8 zeroed);

    ///
    /// Resize the most-recent allocation in place. Only the snapshot
    /// pointer (the last value returned by `arena_allocator_allocate`)
    /// can be resized; any earlier pointer fails immediately.
    ///
    /// SUCCESS: Returns 1 when `ptr` is the most-recent allocation and
    ///          the new size still fits within its chunk. The pointer
    ///          stays valid for `new_size` bytes; the bump cursor
    ///          moves accordingly.
    /// FAILURE: Returns 0 when `ptr` is not the most-recent allocation
    ///          or the chunk has no room for the grown size.
    ///
    /// TAGS: Allocator, Arena, Memory, InPlace
    ///
    i8 arena_allocator_resize(ArenaAllocator *self, void *ptr, size new_size);

    ///
    /// Resize an arena allocation with relocation allowed. Only the
    /// most-recent allocation (the snapshot pointer) is remappable: if
    /// it fits in its own chunk the bump cursor extends in place; if
    /// not, a fresh bump in a new (or current) chunk is taken and the
    /// old bytes are copied into it. Older allocations cannot be
    /// remapped under the bump policy and abort via `LOG_FATAL`.
    ///
    /// SUCCESS: Returns the (possibly moved) pointer. When `ptr` is
    ///          NULL this behaves like
    ///          `arena_allocator_allocate(self, new_size, true)` --
    ///          fresh allocations from a remap-NULL are zeroed.
    ///          When `new_size == 0` returns NULL without freeing
    ///          (arena bump policy reclaims only at Reset / Deinit).
    /// FAILURE: Returns NULL when a fresh chunk cannot be obtained.
    ///          The old allocation is left untouched. Aborts via
    ///          `LOG_FATAL` when `ptr` is foreign to this arena or
    ///          is not the most-recent allocation.
    ///
    /// TAGS: Allocator, Arena, Memory, Reallocation
    ///
    void *arena_allocator_remap(ArenaAllocator *self, void *ptr, size new_size);

    ///
    /// Per-allocation free hook. Arena reclaims everything together at
    /// `ArenaAllocatorDeinit` / `ArenaAllocatorReset` time; for ordinary
    /// allocations this is a deliberate no-op. The most-recent-allocation
    /// snapshot can rewind the bump cursor.
    ///
    /// SUCCESS: Returns the byte count rewound when `ptr` is the most-
    ///          recent allocation (what stats accounting sees), or 0
    ///          when `ptr` is an older pointer left in place until
    ///          arena teardown.
    /// FAILURE: Aborts via `LOG_FATAL` when `ptr` is foreign to this
    ///          arena (does not lie within any chunk's bump range).
    ///
    /// TAGS: Allocator, Arena, Memory, Deallocation
    ///
    size arena_allocator_deallocate(ArenaAllocator *self, void *ptr);

    ///
    /// Release every chunk currently owned by `self`. Walks the
    /// chunk list and returns each one to the kernel, then zeroes the
    /// struct so any post-deinit dispatch trips `ValidateAllocator`
    /// on the cleared `__magic`.
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

/// Construct an ArenaAllocator with default alignment (`1`). Use as a
/// designated-initializer. The arena starts empty: no chunks, no
/// rollback snapshot. The first allocation triggers a kernel page
/// map for the initial chunk.
///
///     ArenaAllocator a = ArenaAllocatorInit();
///     void *p = AllocatorAlloc(&a, 128, false);
///     ArenaAllocatorDeinit(&a);
///
/// SUCCESS: Returns a fully-initialised `ArenaAllocator` value. No OS
///          calls are made; the first alloc triggers the lazy
///          page-map for the initial chunk.
/// FAILURE: Cannot fail at macro-expansion time.
///
/// TAGS: Allocator, Arena, Init
///
#define ArenaAllocatorInit()                                                                                           \
    ((ArenaAllocator) {                                                                                                \
        .base =                                                                                                        \
            {.allocate        = (AllocatorAllocateFn)arena_allocator_allocate,                                         \
                   .resize          = (AllocatorResizeFn)arena_allocator_resize,                                             \
                   .remap           = (AllocatorRemapFn)arena_allocator_remap,                                               \
                   .deallocate      = (AllocatorDeallocateFn)arena_allocator_deallocate,                                     \
                   .alignment       = 1,                                                                                     \
                   .effort          = ALLOCATOR_EFFORT_ONCE,                                                                 \
                   .retry_limit     = 0,                                                                                     \
                   .__magic         = ARENA_ALLOCATOR_MAGIC,                                                                 \
                   .footprint_bytes = 0},                                                                                    \
        .head      = NULL,                                                                                             \
        .tail      = NULL,                                                                                             \
        .last_ptr  = NULL,                                                                                             \
        .last_size = 0                                                                                                 \
    })

/// Same as `ArenaAllocatorInit()` but with a caller-supplied
/// `alignment` floor (`N`, in bytes). `N == 0` is silently coerced
/// to 1.
///
///     ArenaAllocator a = ArenaAllocatorInitAligned(64);
///
/// SUCCESS: Returns a fully-initialised `ArenaAllocator` value with
///          the requested alignment floor recorded in `base.alignment`.
/// FAILURE: Cannot fail at macro-expansion time.
///
/// TAGS: Allocator, Arena, Init, Alignment
///
#define ArenaAllocatorInitAligned(N)                                                                                   \
    ((ArenaAllocator) {                                                                                                \
        .base =                                                                                                        \
            {.allocate        = (AllocatorAllocateFn)arena_allocator_allocate,                                         \
                   .resize          = (AllocatorResizeFn)arena_allocator_resize,                                             \
                   .remap           = (AllocatorRemapFn)arena_allocator_remap,                                               \
                   .deallocate      = (AllocatorDeallocateFn)arena_allocator_deallocate,                                     \
                   .alignment       = (N) ? (N) : 1,                                                                         \
                   .effort          = ALLOCATOR_EFFORT_ONCE,                                                                 \
                   .retry_limit     = 0,                                                                                     \
                   .__magic         = ARENA_ALLOCATOR_MAGIC,                                                                 \
                   .footprint_bytes = 0},                                                                                    \
        .head      = NULL,                                                                                             \
        .tail      = NULL,                                                                                             \
        .last_ptr  = NULL,                                                                                             \
        .last_size = 0                                                                                                 \
    })

#endif // MISRA_STD_ALLOCATOR_ARENA_H
