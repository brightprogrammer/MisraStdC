/// file      : std/allocator/pool.c
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Per-descriptor fixed-size slot pool implementation.

#include <Misra/Std/Allocator/Page.h>
#include <Misra/Std/Allocator/Pool.h>
#include <Misra/Std/Log.h>
#include <Misra/Std/Memory.h>

#include <stdint.h>

static void pool_validate_self(const Allocator *self) {
    if (!self || self->__magic != MISRA_POOL_ALLOCATOR_MAGIC) {
        LOG_FATAL("type-confusion: allocator passed to pool_allocator_* is not a PoolAllocator");
    }
}

struct PoolChunk {
    struct PoolChunk *next;
    char             *slots;
    size              capacity;
    size              raw_size;
};

struct PoolFreeSlot {
    struct PoolFreeSlot *next;
};

static size pool_round_up(size value, size alignment) {
    return (value + (alignment - 1)) & ~(alignment - 1);
}

static size pool_padded_slot_size(size slot_size, size alignment) {
    size required = slot_size > sizeof(struct PoolFreeSlot) ? slot_size : sizeof(struct PoolFreeSlot);
    if (alignment < sizeof(void *)) {
        alignment = sizeof(void *);
    }
    return pool_round_up(required, alignment);
}

static bool pool_grow(PoolAllocator *pool) {
    size align         = pool->base.alignment > 1 ? pool->base.alignment : sizeof(void *);
    size padded_slot   = pool_padded_slot_size(pool->slot_size, align);
    size slot_count    = pool->slots_per_chunk;
    size header_bytes  = sizeof(struct PoolChunk);
    size payload_bytes = slot_count * padded_slot + align;
    size raw_bytes     = header_bytes + payload_bytes;

    char *raw = (char *)AllocatorAlloc(&pool->page.base, raw_bytes, true);
    if (!raw) {
        return false;
    }

    struct PoolChunk *chunk = (struct PoolChunk *)(void *)raw;
    chunk->slots            = raw + header_bytes;
    chunk->capacity         = payload_bytes;
    chunk->raw_size         = raw_bytes;
    chunk->next             = NULL;
    if (!pool->head) {
        pool->head = chunk;
    } else {
        pool->tail->next = chunk;
    }
    pool->tail = chunk;

    uintptr_t base_addr    = (uintptr_t)chunk->slots;
    uintptr_t aligned_addr = (base_addr + (uintptr_t)(align - 1)) & ~(uintptr_t)(align - 1);
    char     *cursor       = (char *)(void *)aligned_addr;
    for (size i = 0; i < slot_count; i++) {
        struct PoolFreeSlot *slot  = (struct PoolFreeSlot *)(void *)cursor;
        slot->next                 = pool->free_head;
        pool->free_head            = slot;
        cursor                    += padded_slot;
    }
    return true;
}

void *pool_allocator_allocate(Allocator *self, size bytes, bool zeroed) {
    pool_validate_self(self);
    PoolAllocator *pool        = (PoolAllocator *)self;
    size           align       = self->alignment > 1 ? self->alignment : sizeof(void *);
    size           padded_slot = pool_padded_slot_size(pool->slot_size, align);

    if (bytes > padded_slot) {
        return NULL;
    }
    if (!pool->free_head && !pool_grow(pool)) {
        return NULL;
    }
    struct PoolFreeSlot *slot = pool->free_head;
    pool->free_head           = slot->next;
    if (zeroed) {
        MemSet(slot, 0, padded_slot);
    }
    return slot;
}

void *pool_allocator_reallocate(Allocator *self, void *ptr, size old_size, size new_size) {
    pool_validate_self(self);
    PoolAllocator *pool   = (PoolAllocator *)self;
    size           align  = self->alignment > 1 ? self->alignment : sizeof(void *);
    size           padded = pool_padded_slot_size(pool->slot_size, align);

    (void)old_size;

    if (!ptr) {
        return pool_allocator_allocate(self, new_size, true);
    }
    if (new_size == 0) {
        struct PoolFreeSlot *slot = (struct PoolFreeSlot *)ptr;
        slot->next                = pool->free_head;
        pool->free_head           = slot;
        return NULL;
    }
    if (new_size <= padded) {
        return ptr;
    }
    return NULL;
}

void pool_allocator_deallocate(Allocator *self, void *ptr, size bytes) {
    pool_validate_self(self);
    PoolAllocator *pool = (PoolAllocator *)self;
    (void)bytes;
    if (!ptr) {
        return;
    }
    struct PoolFreeSlot *slot = (struct PoolFreeSlot *)ptr;
    slot->next                = pool->free_head;
    pool->free_head           = slot;
}

void PoolAllocatorDeinit(PoolAllocator *self) {
    if (!self) {
        return;
    }
    struct PoolChunk *chunk = self->head;
    while (chunk) {
        struct PoolChunk *next = chunk->next;
        AllocatorFree(&self->page.base, (void *)chunk, chunk->raw_size);
        chunk = next;
    }
    self->head      = NULL;
    self->tail      = NULL;
    self->free_head = NULL;
}
