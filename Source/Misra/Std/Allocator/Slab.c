/// file      : std/allocator/slab.c
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Per-descriptor fixed-size slot slab implementation.

#include <Misra/Std/Allocator/Page.h>
#include <Misra/Std/Allocator/Slab.h>
#include <Misra/Std/Log.h>
#include <Misra/Std/Memory.h>

#include <stdint.h>

static void slab_validate_self(const Allocator *self) {
    if (!self || self->__magic != MISRA_SLAB_ALLOCATOR_MAGIC) {
        LOG_FATAL("type-confusion: allocator passed to slab_allocator_* is not a SlabAllocator");
    }
}

struct SlabChunk {
    struct SlabChunk *next;
    char             *slots;
    size              capacity;
    size              raw_size;
};

struct SlabFreeSlot {
    struct SlabFreeSlot *next;
};

static size slab_round_up(size value, size alignment) {
    return (value + (alignment - 1)) & ~(alignment - 1);
}

static size slab_padded_slot_size(size slot_size, size alignment) {
    size required = slot_size > sizeof(struct SlabFreeSlot) ? slot_size : sizeof(struct SlabFreeSlot);
    if (alignment < sizeof(void *)) {
        alignment = sizeof(void *);
    }
    return slab_round_up(required, alignment);
}

static bool slab_grow(SlabAllocator *slab) {
    size align         = slab->base.alignment > 1 ? slab->base.alignment : sizeof(void *);
    size padded_slot   = slab_padded_slot_size(slab->slot_size, align);
    size slot_count    = slab->slots_per_chunk;
    size header_bytes  = sizeof(struct SlabChunk);
    size payload_bytes = slot_count * padded_slot + align;
    size raw_bytes     = header_bytes + payload_bytes;

    char *raw = (char *)AllocatorAlloc(&slab->page.base, raw_bytes, true);
    if (!raw) {
        return false;
    }

    struct SlabChunk *chunk = (struct SlabChunk *)(void *)raw;
    chunk->slots            = raw + header_bytes;
    chunk->capacity         = payload_bytes;
    chunk->raw_size         = raw_bytes;
    chunk->next             = NULL;
    if (!slab->head) {
        slab->head = chunk;
    } else {
        slab->tail->next = chunk;
    }
    slab->tail = chunk;

    uintptr_t base_addr    = (uintptr_t)chunk->slots;
    uintptr_t aligned_addr = (base_addr + (uintptr_t)(align - 1)) & ~(uintptr_t)(align - 1);
    char     *cursor       = (char *)(void *)aligned_addr;
    for (size i = 0; i < slot_count; i++) {
        struct SlabFreeSlot *slot  = (struct SlabFreeSlot *)(void *)cursor;
        slot->next                 = slab->free_head;
        slab->free_head            = slot;
        cursor                    += padded_slot;
    }
    return true;
}

void *slab_allocator_allocate(Allocator *self, size bytes, bool zeroed) {
    slab_validate_self(self);
    SlabAllocator *slab        = (SlabAllocator *)self;
    size           align       = self->alignment > 1 ? self->alignment : sizeof(void *);
    size           padded_slot = slab_padded_slot_size(slab->slot_size, align);

    if (bytes > padded_slot) {
        return NULL;
    }
    if (!slab->free_head && !slab_grow(slab)) {
        return NULL;
    }
    struct SlabFreeSlot *slot = slab->free_head;
    slab->free_head           = slot->next;
    if (zeroed) {
        MemSet(slot, 0, padded_slot);
    }
    return slot;
}

void *slab_allocator_reallocate(Allocator *self, void *ptr, size old_size, size new_size) {
    slab_validate_self(self);
    SlabAllocator *slab   = (SlabAllocator *)self;
    size           align  = self->alignment > 1 ? self->alignment : sizeof(void *);
    size           padded = slab_padded_slot_size(slab->slot_size, align);

    (void)old_size;

    if (!ptr) {
        return slab_allocator_allocate(self, new_size, true);
    }
    if (new_size == 0) {
        struct SlabFreeSlot *slot = (struct SlabFreeSlot *)ptr;
        slot->next                = slab->free_head;
        slab->free_head           = slot;
        return NULL;
    }
    if (new_size <= padded) {
        return ptr;
    }
    return NULL;
}

void slab_allocator_deallocate(Allocator *self, void *ptr, size bytes) {
    slab_validate_self(self);
    SlabAllocator *slab = (SlabAllocator *)self;
    (void)bytes;
    if (!ptr) {
        return;
    }
    struct SlabFreeSlot *slot = (struct SlabFreeSlot *)ptr;
    slot->next                = slab->free_head;
    slab->free_head           = slot;
}

void SlabAllocatorDeinit(SlabAllocator *self) {
    if (!self) {
        return;
    }
    struct SlabChunk *chunk = self->head;
    while (chunk) {
        struct SlabChunk *next = chunk->next;
        AllocatorFree(&self->page.base, (void *)chunk, chunk->raw_size);
        chunk = next;
    }
    self->head      = NULL;
    self->tail      = NULL;
    self->free_head = NULL;
}
