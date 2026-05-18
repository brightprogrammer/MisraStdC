/// file      : std/allocator/arena.c
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Per-descriptor bump (arena) allocator implementation.

#include <Misra/Std/Allocator/Arena.h>
#include <Misra/Std/Allocator/Page.h>
#include <Misra/Std/Log.h>
#include <Misra/Std/Memory.h>

#include <stdint.h>

static void arena_validate_self(const Allocator *self) {
    if (!self || self->__magic != ARENA_ALLOCATOR_MAGIC) {
        LOG_FATAL("type-confusion: allocator passed to arena_allocator_* is not an ArenaAllocator");
    }
}

#define ARENA_DEFAULT_CHUNK_SIZE (size)(64 * 1024)

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
    size minimum = ARENA_DEFAULT_CHUNK_SIZE;
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

void *arena_allocator_allocate(Allocator *self, size bytes, i8 zeroed) {
    arena_validate_self(self);
    (void)zeroed; // page-backed memory is zero-initialized.
    if (!bytes) {
        return NULL;
    }
    ArenaAllocator *arena  = (ArenaAllocator *)self;
    size            align  = arena_effective_alignment(self);
    size            padded = arena_round_up(bytes, align);

    ArenaChunk *chunk = arena->tail;
    if (chunk) {
        uintptr_t base_addr    = (uintptr_t)chunk->base;
        uintptr_t free_addr    = base_addr + chunk->used;
        uintptr_t aligned_addr = (free_addr + (uintptr_t)(align - 1)) & ~(uintptr_t)(align - 1);
        size      aligned_used = (size)(aligned_addr - base_addr);
        if (aligned_used + padded <= chunk->capacity) {
            char *result     = chunk->base + aligned_used;
            chunk->used      = aligned_used + padded;
            arena->last_ptr  = result;
            arena->last_size = padded;
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

// Try to grow/shrink in place. The arena can only do this when `ptr`
// is the most recent allocation (its end coincides with the chunk's
// high-water mark), because earlier allocations have stuff after
// them that we can't disturb. Shrinks of older allocations: refused
// too -- they'd leave a hole the arena can't reclaim and the caller
// could just keep the over-large slot. Same answer either way.
i8 arena_allocator_resize(Allocator *self, void *ptr, size old_size, size new_size) {
    arena_validate_self(self);
    ArenaAllocator *arena = (ArenaAllocator *)self;
    size            align = arena_effective_alignment(self);
    (void)old_size;

    if (arena->last_ptr != ptr || !arena->tail) {
        return 0;
    }
    ArenaChunk *chunk      = arena->tail;
    size        padded_new = arena_round_up(new_size, align);
    size        last_off   = (size)((char *)ptr - chunk->base);
    if (last_off + padded_new > chunk->capacity) {
        return 0; // grow doesn't fit in this chunk
    }
    chunk->used      = last_off + padded_new;
    arena->last_size = padded_new;
    return 1;
}

void *arena_allocator_remap(Allocator *self, void *ptr, size old_size, size new_size) {
    arena_validate_self(self);
    ArenaAllocator *arena = (ArenaAllocator *)self;
    (void)arena;

    if (new_size == 0) {
        (void)ptr;
        (void)old_size;
        return NULL;
    }
    if (!ptr) {
        return arena_allocator_allocate(self, new_size, true);
    }

    // Grow in place when `ptr` is the last bump (still a fast path
    // for remap callers that come straight here without trying
    // resize first).
    if (arena_allocator_resize(self, ptr, old_size, new_size)) {
        return ptr;
    }

    void *fresh = arena_allocator_allocate(self, new_size, true);
    if (!fresh) {
        return NULL;
    }
    MemCopy(fresh, ptr, old_size < new_size ? old_size : new_size);
    return fresh;
}

size arena_allocator_deallocate(Allocator *self, void *ptr) {
    arena_validate_self(self);
    ArenaAllocator *arena = (ArenaAllocator *)self;
    if (!ptr) {
        return 0;
    }
    // Rewind only when the caller is freeing the most recent bump.
    if (arena->last_ptr == ptr && arena->tail) {
        ArenaChunk *chunk   = arena->tail;
        size        rewound = arena->last_size;
        if (chunk->used >= rewound) {
            chunk->used -= rewound;
        }
        arena->last_ptr  = NULL;
        arena->last_size = 0;
        return rewound;
    }
    // Mid-stream frees are silently ignored under the bump policy --
    // memory is reclaimed at Reset / Deinit time. Report 0 freed for
    // stats; the caller's logical lifetime accounting is still
    // correct because we never decremented bytes_in_use anyway.
    return 0;
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
        PageAllocatorFree(&self->page, (void *)chunk, chunk->raw_size);
        chunk = next;
    }
    MemSet(self, 0, sizeof(*self));
}
