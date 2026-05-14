/// file      : std/allocator/arena.c
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Per-descriptor bump (arena) allocator implementation.

#include <Misra/Std/Allocator/Arena.h>
#include <Misra/Std/Allocator/Page.h>
#include <Misra/Std/Memory.h>

#include <stdint.h>

#define MISRA_ARENA_DEFAULT_CHUNK_SIZE (size)(64 * 1024)

struct ArenaChunk {
    struct ArenaChunk *next;
    char              *base;
    size               capacity;
    size               used;
    size               raw_size;
};

static size arena_round_up(size value, size alignment) {
    return (value + (alignment - 1)) & ~(alignment - 1);
}

static size arena_effective_alignment(const Allocator *self) {
    return self->alignment > 1 ? self->alignment : 1;
}

static size arena_chunk_size_for(ArenaAllocator *arena, size need_bytes) {
    size page    = PageAllocatorPageSize(&arena->page);
    size minimum = MISRA_ARENA_DEFAULT_CHUNK_SIZE;
    size wanted  = need_bytes > minimum ? need_bytes : minimum;
    if (page > 1) {
        wanted = arena_round_up(wanted, page);
    }
    return wanted;
}

static ArenaChunk *arena_new_chunk(ArenaAllocator *arena, size need_bytes) {
    size  chunk_bytes = arena_chunk_size_for(arena, need_bytes + sizeof(ArenaChunk));
    char *raw         = (char *)AllocatorAlloc(&arena->page.base, chunk_bytes, true);
    if (!raw) {
        return NULL;
    }
    ArenaChunk *chunk = (ArenaChunk *)(void *)raw;
    chunk->next       = NULL;
    chunk->base       = raw + sizeof(ArenaChunk);
    chunk->capacity   = chunk_bytes - sizeof(ArenaChunk);
    chunk->used       = 0;
    chunk->raw_size   = chunk_bytes;
    return chunk;
}

void *arena_allocator_allocate(Allocator *self, size bytes, bool zeroed) {
    (void)zeroed; // page-backed memory is zero-initialized.
    if (!bytes) {
        return NULL;
    }
    ArenaAllocator *arena = (ArenaAllocator *)self;
    size            align = arena_effective_alignment(self);
    size            padded = arena_round_up(bytes, align);

    ArenaChunk *chunk = arena->tail;
    if (chunk) {
        uintptr_t base_addr    = (uintptr_t)chunk->base;
        uintptr_t free_addr    = base_addr + chunk->used;
        uintptr_t aligned_addr = (free_addr + (uintptr_t)(align - 1)) & ~(uintptr_t)(align - 1);
        size      aligned_used = (size)(aligned_addr - base_addr);
        if (aligned_used + padded <= chunk->capacity) {
            char *result      = chunk->base + aligned_used;
            chunk->used       = aligned_used + padded;
            arena->last_ptr   = result;
            arena->last_size  = padded;
            return result;
        }
    }

    chunk = arena_new_chunk(arena, padded + align);
    if (!chunk) {
        return NULL;
    }
    if (!arena->head) {
        arena->head = chunk;
    } else {
        arena->tail->next = chunk;
    }
    arena->tail = chunk;

    uintptr_t base_addr    = (uintptr_t)chunk->base;
    uintptr_t aligned_addr = (base_addr + (uintptr_t)(align - 1)) & ~(uintptr_t)(align - 1);
    size      aligned_used = (size)(aligned_addr - base_addr);
    char     *result       = chunk->base + aligned_used;
    chunk->used            = aligned_used + padded;
    arena->last_ptr        = result;
    arena->last_size       = padded;
    return result;
}

void *arena_allocator_reallocate(Allocator *self, void *ptr, size old_size, size new_size) {
    ArenaAllocator *arena = (ArenaAllocator *)self;
    size            align = arena_effective_alignment(self);

    if (new_size == 0) {
        (void)ptr;
        (void)old_size;
        return NULL;
    }
    if (!ptr) {
        return arena_allocator_allocate(self, new_size, true);
    }

    // Grow in place when `ptr` is the last bump.
    if (arena->last_ptr == ptr && arena->tail) {
        ArenaChunk *chunk      = arena->tail;
        size        padded_new = arena_round_up(new_size, align);
        size        last_off   = (size)((char *)ptr - chunk->base);
        if (last_off + padded_new <= chunk->capacity) {
            chunk->used      = last_off + padded_new;
            arena->last_size = padded_new;
            return ptr;
        }
    }

    void *fresh = arena_allocator_allocate(self, new_size, true);
    if (!fresh) {
        return NULL;
    }
    MemCopy(fresh, ptr, old_size < new_size ? old_size : new_size);
    return fresh;
}

void arena_allocator_deallocate(Allocator *self, void *ptr, size bytes) {
    ArenaAllocator *arena = (ArenaAllocator *)self;
    (void)bytes;
    if (!ptr) {
        return;
    }
    // Rewind only when the caller is freeing the most recent bump.
    if (arena->last_ptr == ptr && arena->tail) {
        ArenaChunk *chunk = arena->tail;
        if (chunk->used >= arena->last_size) {
            chunk->used -= arena->last_size;
        }
        arena->last_ptr  = NULL;
        arena->last_size = 0;
    }
}

void ArenaAllocatorReset(ArenaAllocator *self) {
    if (!self) {
        return;
    }
    ArenaChunk *chunk = self->head;
    while (chunk) {
        chunk->used = 0;
        chunk       = chunk->next;
    }
    self->last_ptr  = NULL;
    self->last_size = 0;
}

void ArenaAllocatorDeinit(ArenaAllocator *self) {
    if (!self) {
        return;
    }
    ArenaChunk *chunk = self->head;
    while (chunk) {
        ArenaChunk *next = chunk->next;
        AllocatorFree(&self->page.base, (void *)chunk, chunk->raw_size);
        chunk = next;
    }
    self->head      = NULL;
    self->tail      = NULL;
    self->last_ptr  = NULL;
    self->last_size = 0;
}
