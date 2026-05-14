/// file      : std/allocator/slab.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Per-descriptor fixed-size slot slab. Every allocation must match the
/// configured slot size; alloc/free are O(1) via an intrusive free list.
/// State is inline; the embedded `PageAllocator` provides backing slabs.

#ifndef MISRA_STD_ALLOCATOR_SLAB_H
#define MISRA_STD_ALLOCATOR_SLAB_H

#include <Misra/Std/Allocator.h>
#include <Misra/Std/Allocator/Page.h>

///
/// Per-type magic for `SlabAllocator`. Stamped into
/// `Allocator.base.__magic` by `SlabAllocatorInit*`. The slab
/// implementation functions validate this exact value so other
/// allocator instances reinterpreted as a `SlabAllocator *` are
/// rejected at runtime as type-confusion.
///
#define MISRA_SLAB_ALLOCATOR_MAGIC MISRA_MAKE_NEW_MAGIC_VALUE("slaballc")

#ifdef __cplusplus
extern "C" {
#endif

    typedef struct SlabChunk    SlabChunk;
    typedef struct SlabFreeSlot SlabFreeSlot;

    typedef struct SlabAllocator {
        Allocator     base;
        SlabChunk    *head;
        SlabChunk    *tail;
        SlabFreeSlot *free_head;
        size          slot_size;
        size          slots_per_chunk;
        PageAllocator page;
    } SlabAllocator;

    void *slab_allocator_allocate(Allocator *self, size bytes, bool zeroed);
    void *slab_allocator_reallocate(Allocator *self, void *ptr, size old_size, size new_size);
    void  slab_allocator_deallocate(Allocator *self, void *ptr, size bytes);

    void SlabAllocatorDeinit(SlabAllocator *self);

#ifdef __cplusplus
}
#endif

#define MISRA_SLAB_DEFAULT_CHUNK_SLOTS 256u

///
/// Initialize a `SlabAllocator` with the given slot size. Slot size is
/// padded internally so each slot holds the intrusive free-list pointer.
///
#define SlabAllocatorInit(slot_size_bytes)                                                                             \
    ((SlabAllocator) {                                                                                                 \
        .base =                                                                                                        \
            {.allocate    = slab_allocator_allocate,                                                                   \
                   .reallocate  = slab_allocator_reallocate,                                                                 \
                   .deallocate  = slab_allocator_deallocate,                                                                 \
                   .alignment   = 1,                                                                                         \
                   .effort      = ALLOCATOR_EFFORT_ONCE,                                                                     \
                   .retry_limit = 0,                                                                                         \
                   .__magic     = MISRA_SLAB_ALLOCATOR_MAGIC},                                                                   \
        .head            = NULL,                                                                                       \
        .tail            = NULL,                                                                                       \
        .free_head       = NULL,                                                                                       \
        .slot_size       = (slot_size_bytes),                                                                          \
        .slots_per_chunk = MISRA_SLAB_DEFAULT_CHUNK_SLOTS,                                                             \
        .page            = PageAllocatorInit()                                                                         \
    })

///
/// Initialize a `SlabAllocator` with a custom alignment floor.
///
#define SlabAllocatorInitAligned(slot_size_bytes, alignment_value)                                                     \
    ((SlabAllocator) {                                                                                                 \
        .base =                                                                                                        \
            {.allocate    = slab_allocator_allocate,                                                                   \
                   .reallocate  = slab_allocator_reallocate,                                                                 \
                   .deallocate  = slab_allocator_deallocate,                                                                 \
                   .alignment   = (alignment_value) ? (alignment_value) : 1,                                                 \
                   .effort      = ALLOCATOR_EFFORT_ONCE,                                                                     \
                   .retry_limit = 0,                                                                                         \
                   .__magic     = MISRA_SLAB_ALLOCATOR_MAGIC},                                                                   \
        .head            = NULL,                                                                                       \
        .tail            = NULL,                                                                                       \
        .free_head       = NULL,                                                                                       \
        .slot_size       = (slot_size_bytes),                                                                          \
        .slots_per_chunk = MISRA_SLAB_DEFAULT_CHUNK_SLOTS,                                                             \
        .page            = PageAllocatorInit()                                                                         \
    })

#endif // MISRA_STD_ALLOCATOR_SLAB_H
