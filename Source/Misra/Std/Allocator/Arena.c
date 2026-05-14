/// file      : std/allocator/arena.c
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Bump (arena) allocator backed by page-mapped memory.

#include <Misra/Std/Allocator/Arena.h>
#include <Misra/Std/Allocator/Page.h>
#include <Misra/Std/Log.h>
#include <Misra/Std/Memory.h>

#include <stdint.h>

#define MISRA_ARENA_DEFAULT_CHUNK_SIZE (size)(64 * 1024)

typedef struct ArenaChunk {
    struct ArenaChunk *next;
    char              *base;
    size               capacity;
    size               used;
} ArenaChunk;

typedef struct {
    ArenaChunk *head;
    ArenaChunk *tail;
    Allocator   page;
    // Address and size of the most recent successful allocation. Used by
    // `reallocate` to grow in place when the caller resizes the very last
    // bump.
    char *last_ptr;
    size  last_size;
} ArenaState;

static size arena_round_up_pow2(size value, size alignment) {
    return (value + (alignment - 1)) & ~(alignment - 1);
}

static size arena_effective_alignment(const Allocator *alloc) {
    return alloc->alignment > 1 ? alloc->alignment : 1;
}

static size arena_chunk_size_for(size need_bytes) {
    size page    = PageAllocatorPageSize();
    size minimum = MISRA_ARENA_DEFAULT_CHUNK_SIZE;
    size wanted  = need_bytes > minimum ? need_bytes : minimum;
    if (page > 1) {
        wanted = arena_round_up_pow2(wanted, page);
    }
    return wanted;
}

static ArenaChunk *arena_new_chunk(ArenaState *state, size need_bytes) {
    size        chunk_bytes = arena_chunk_size_for(need_bytes + sizeof(ArenaChunk));
    char       *raw         = (char *)AllocatorAlloc(&state->page, chunk_bytes, true);
    ArenaChunk *chunk;

    if (!raw) {
        return NULL;
    }

    chunk           = (ArenaChunk *)(void *)raw;
    chunk->next     = NULL;
    chunk->base     = raw + sizeof(ArenaChunk);
    chunk->capacity = chunk_bytes - sizeof(ArenaChunk);
    chunk->used     = 0;
    return chunk;
}

static bool arena_state_init(Allocator *alloc) {
    Allocator   page = PageAllocator();
    ArenaState *state;

    if (!AllocatorEnsureState(&page)) {
        return false;
    }

    state = (ArenaState *)AllocatorAlloc(&page, sizeof(ArenaState), true);
    if (!state) {
        return false;
    }

    state->head      = NULL;
    state->tail      = NULL;
    state->page      = page;
    state->last_ptr  = NULL;
    state->last_size = 0;

    alloc->state = state;
    return true;
}

static void arena_state_deinit(Allocator *alloc) {
    ArenaState *state = (ArenaState *)alloc->state;
    ArenaChunk *chunk;
    ArenaChunk *next;
    Allocator   page;

    if (!state) {
        return;
    }

    page  = state->page;
    chunk = state->head;
    while (chunk) {
        next = chunk->next;
        AllocatorFree(&page, (void *)chunk, chunk->capacity + sizeof(ArenaChunk));
        chunk = next;
    }

    AllocatorFree(&page, state, sizeof(ArenaState));
    AllocatorUnbind(&page);
    alloc->state = NULL;
}

static void *arena_allocate(Allocator *alloc, size bytes, bool zeroed) {
    ArenaState *state;
    ArenaChunk *chunk;
    size        align = arena_effective_alignment(alloc);
    size        padded;
    char       *result;

    (void)zeroed; // Pages from PageAllocator are zero-initialized.

    if (!bytes) {
        return NULL;
    }

    state  = (ArenaState *)alloc->state;
    padded = arena_round_up_pow2(bytes, align);

    chunk = state->tail;
    if (chunk) {
        uintptr_t base_addr    = (uintptr_t)chunk->base;
        uintptr_t free_addr    = base_addr + chunk->used;
        uintptr_t aligned_addr = (free_addr + (uintptr_t)(align - 1)) & ~(uintptr_t)(align - 1);
        size      aligned_used = (size)(aligned_addr - base_addr);
        if (aligned_used + padded <= chunk->capacity) {
            result      = chunk->base + aligned_used;
            chunk->used = aligned_used + padded;

            state->last_ptr  = result;
            state->last_size = padded;
            return result;
        }
    }

    // Need a fresh chunk. Account for worst-case alignment slack so a single
    // bump always fits after we re-align inside the new chunk.
    chunk = arena_new_chunk(state, padded + align);
    if (!chunk) {
        return NULL;
    }

    if (!state->head) {
        state->head = chunk;
    } else {
        state->tail->next = chunk;
    }
    state->tail = chunk;

    {
        uintptr_t base_addr    = (uintptr_t)chunk->base;
        uintptr_t aligned_addr = (base_addr + (uintptr_t)(align - 1)) & ~(uintptr_t)(align - 1);
        size      aligned_used = (size)(aligned_addr - base_addr);
        result                 = chunk->base + aligned_used;
        chunk->used            = aligned_used + padded;
    }

    state->last_ptr  = result;
    state->last_size = padded;
    return result;
}

static void *arena_reallocate(Allocator *alloc, void *ptr, size old_size, size new_size) {
    ArenaState *state = (ArenaState *)alloc->state;
    size        align = arena_effective_alignment(alloc);

    if (new_size == 0) {
        // Mirror the free-on-zero semantics: arena cannot return memory but
        // it must not crash.
        (void)alloc;
        (void)ptr;
        (void)old_size;
        return NULL;
    }

    if (!ptr) {
        return arena_allocate(alloc, new_size, true);
    }

    // Grow in place when ptr is the last bump.
    if (state && state->last_ptr == ptr && state->tail) {
        ArenaChunk *chunk      = state->tail;
        size        padded_new = arena_round_up_pow2(new_size, align);
        size        last_off   = (size)((char *)ptr - chunk->base);
        if (last_off + padded_new <= chunk->capacity) {
            chunk->used      = last_off + padded_new;
            state->last_size = padded_new;
            return ptr;
        }
    }

    // Fallback: allocate fresh and copy. The old allocation is abandoned
    // inside the arena, which is acceptable for bump-style allocators.
    void *fresh = arena_allocate(alloc, new_size, true);
    if (!fresh) {
        return NULL;
    }

    MemCopy(fresh, ptr, old_size < new_size ? old_size : new_size);
    return fresh;
}

static void arena_deallocate(Allocator *alloc, void *ptr, size bytes) {
    ArenaState *state = (ArenaState *)alloc->state;

    if (!state || !ptr) {
        return;
    }

    // If the caller is freeing the most recent bump, we can rewind. Anything
    // else is a no-op (typical arena behaviour).
    if (state->last_ptr == ptr && state->tail) {
        ArenaChunk *chunk = state->tail;
        if (chunk->used >= state->last_size) {
            chunk->used -= state->last_size;
        }
        state->last_ptr  = NULL;
        state->last_size = 0;
    }

    (void)bytes;
}

Allocator ArenaAllocator(void) {
    return (Allocator) {
        .state        = NULL,
        .state_init   = arena_state_init,
        .state_deinit = arena_state_deinit,
        .allocate     = arena_allocate,
        .reallocate   = arena_reallocate,
        .deallocate   = arena_deallocate,
        .effort       = ALLOCATOR_EFFORT_ONCE,
        .retry_limit  = 0,
        .flags        = 0,
        .alignment    = 1,
    };
}

Allocator ArenaAllocatorAligned(size alignment) {
    Allocator alloc = ArenaAllocator();
    if (alignment) {
        alloc.alignment = alignment;
    }
    return alloc;
}

void ArenaAllocatorReset(Allocator *arena) {
    ArenaState *state;
    ArenaChunk *chunk;

    if (!arena || !arena->state) {
        return;
    }

    // Only act on arena allocators.
    if (arena->state_init != arena_state_init) {
        return;
    }

    state = (ArenaState *)arena->state;
    chunk = state->head;
    while (chunk) {
        chunk->used = 0;
        chunk       = chunk->next;
    }

    state->last_ptr  = NULL;
    state->last_size = 0;
}
