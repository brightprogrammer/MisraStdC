/// file      : std/allocator/pool.c
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Fixed-size slot pool allocator.

#include <Misra/Std/Allocator/Page.h>
#include <Misra/Std/Allocator/Pool.h>
#include <Misra/Std/Allocator/Private.h>
#include <Misra/Std/Memory.h>

#include <stdint.h>

#define MISRA_POOL_DEFAULT_CHUNK_SLOTS 256u

typedef struct PoolChunk {
    struct PoolChunk *next;
    char             *slots;
    size              capacity;
} PoolChunk;

typedef struct PoolFreeSlot {
    struct PoolFreeSlot *next;
} PoolFreeSlot;

typedef struct {
    PoolChunk    *head;
    PoolChunk    *tail;
    PoolFreeSlot *free_head;
    Allocator     page;
    size          slot_size;
    size          slots_per_chunk;
} PoolState;

static size pool_round_up_pow2(size value, size alignment) {
    return (value + (alignment - 1)) & ~(alignment - 1);
}

static size pool_padded_slot_size(size slot_size, size alignment) {
    size required = slot_size > sizeof(PoolFreeSlot) ? slot_size : sizeof(PoolFreeSlot);
    if (alignment < sizeof(void *)) {
        alignment = sizeof(void *);
    }
    return pool_round_up_pow2(required, alignment);
}

static bool pool_state_init(Allocator *alloc) {
    Allocator  page = PageAllocator();
    PoolState *state;
    size       slot_size = (size)alloc->flags;

    if (!slot_size) {
        return false;
    }

    if (!allocator_ensure_state(&page)) {
        return false;
    }

    state = (PoolState *)AllocatorAlloc(&page, sizeof(PoolState), true);
    if (!state) {
        return false;
    }

    state->head            = NULL;
    state->tail            = NULL;
    state->free_head       = NULL;
    state->page            = page;
    state->slot_size       = pool_padded_slot_size(slot_size, alloc->alignment);
    state->slots_per_chunk = MISRA_POOL_DEFAULT_CHUNK_SLOTS;

    alloc->state = state;
    return true;
}

static void pool_state_deinit(Allocator *alloc) {
    PoolState *state = (PoolState *)alloc->state;
    PoolChunk *chunk;
    PoolChunk *next;
    Allocator  page;

    if (!state) {
        return;
    }

    page  = state->page;
    chunk = state->head;
    while (chunk) {
        next = chunk->next;
        AllocatorFree(&page, chunk->slots, chunk->capacity);
        AllocatorFree(&page, chunk, sizeof(PoolChunk));
        chunk = next;
    }

    AllocatorFree(&page, state, sizeof(PoolState));
    AllocatorUnbind(&page);
    alloc->state = NULL;
}

static bool pool_grow(PoolState *state, size alignment) {
    PoolChunk *chunk = (PoolChunk *)AllocatorAlloc(&state->page, sizeof(PoolChunk), true);
    if (!chunk) {
        return false;
    }

    size  slot_count = state->slots_per_chunk;
    size  raw_bytes  = slot_count * state->slot_size + alignment;
    char *raw        = (char *)AllocatorAlloc(&state->page, raw_bytes, true);
    if (!raw) {
        AllocatorFree(&state->page, chunk, sizeof(PoolChunk));
        return false;
    }

    uintptr_t base_addr    = (uintptr_t)raw;
    uintptr_t aligned_addr = (base_addr + (uintptr_t)(alignment - 1)) & ~(uintptr_t)(alignment - 1);

    chunk->slots    = raw;
    chunk->capacity = raw_bytes;
    chunk->next     = NULL;
    if (!state->head) {
        state->head = chunk;
    } else {
        state->tail->next = chunk;
    }
    state->tail = chunk;

    // Link every freshly-mapped slot into the free list.
    char *cursor = (char *)(void *)aligned_addr;
    for (size i = 0; i < slot_count; i++) {
        PoolFreeSlot *slot  = (PoolFreeSlot *)(void *)cursor;
        slot->next          = state->free_head;
        state->free_head    = slot;
        cursor             += state->slot_size;
    }

    return true;
}

static void *pool_allocate(Allocator *alloc, size bytes, bool zeroed) {
    PoolState    *state = (PoolState *)alloc->state;
    PoolFreeSlot *slot;
    size          align = alloc->alignment > 1 ? alloc->alignment : sizeof(void *);

    if (!state) {
        return NULL;
    }

    if (bytes > state->slot_size) {
        return NULL; // Caller asked for more than the pool's slot size.
    }

    if (!state->free_head) {
        if (!pool_grow(state, align)) {
            return NULL;
        }
    }

    slot             = state->free_head;
    state->free_head = slot->next;

    if (zeroed) {
        MemSet(slot, 0, state->slot_size);
    }

    return slot;
}

static void *pool_reallocate(Allocator *alloc, void *ptr, size old_size, size new_size) {
    PoolState *state = (PoolState *)alloc->state;

    (void)old_size;

    if (!ptr) {
        return pool_allocate(alloc, new_size, true);
    }

    if (new_size == 0) {
        // Recycle by pushing back onto the free list.
        PoolFreeSlot *slot = (PoolFreeSlot *)ptr;
        slot->next         = state->free_head;
        state->free_head   = slot;
        return NULL;
    }

    if (new_size <= state->slot_size) {
        return ptr; // Same slot still fits.
    }

    // Caller wants a bigger slot than the pool can provide - fail rather
    // than silently truncate or fall back to another allocator.
    return NULL;
}

static void pool_deallocate(Allocator *alloc, void *ptr, size bytes) {
    PoolState    *state = (PoolState *)alloc->state;
    PoolFreeSlot *slot;

    (void)bytes;

    if (!state || !ptr) {
        return;
    }

    slot             = (PoolFreeSlot *)ptr;
    slot->next       = state->free_head;
    state->free_head = slot;
}

Allocator PoolAllocator(size slot_size) {
    return PoolAllocatorAligned(slot_size, 1);
}

Allocator PoolAllocatorAligned(size slot_size, size alignment) {
    u32 flags_slot = slot_size > (size)UINT32_MAX ? UINT32_MAX : (u32)slot_size;
    return (Allocator) {
        .state        = NULL,
        .state_init   = pool_state_init,
        .state_deinit = pool_state_deinit,
        .allocate     = pool_allocate,
        .reallocate   = pool_reallocate,
        .deallocate   = pool_deallocate,
        .effort       = ALLOCATOR_EFFORT_ONCE,
        .retry_limit  = 0,
        .flags        = flags_slot,
        .alignment    = alignment ? alignment : 1,
    };
}
