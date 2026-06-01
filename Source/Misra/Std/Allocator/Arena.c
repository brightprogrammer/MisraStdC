/// file      : std/allocator/arena.c
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Per-descriptor bump (arena) allocator implementation.

#include <Misra/Std/Allocator/Arena.h>
#include <Misra/Std/Log.h>
#include <Misra/Std/Memory.h>
#include "_Os.h"

// Relational invariants for ArenaAllocator:
//   - head and tail chain endpoints are both NULL or both non-NULL.
//   - last_ptr / last_size record the most recent allocation for
//     resize-in-place; both NULL/0 means no rollback target, but a
//     non-NULL last_ptr requires a chunk to exist for it to live in.
//
// The structural body is memoized via MAGIC_VALIDATED_BIT: chunk-list
// growth (`arena_new_chunk` linked into head/tail) and bulk teardown
// (Reset / Deinit prep) flip the bit; the per-allocation `last_ptr` /
// `last_size` update maintains the invariant by construction and
// leaves the bit alone.

#define ARENA_MARK_DIRTY(a) MAGIC_MARK_DIRTY(&(a)->base)

static void arena_validate_self_structural(const ArenaAllocator *self) {
    if (!self->base.allocate || !self->base.resize || !self->base.remap || !self->base.deallocate) {
        LOG_FATAL("ArenaAllocator: vtable function pointer is NULL");
    }
    if (self->base.alignment == 0 || (self->base.alignment & (self->base.alignment - 1)) != 0) {
        LOG_FATAL("ArenaAllocator: alignment {} is not a positive power of two", (u64)self->base.alignment);
    }
    if ((self->head == NULL) != (self->tail == NULL)) {
        LOG_FATAL("ArenaAllocator: head/tail mismatch ({x} / {x})", (u64)self->head, (u64)self->tail);
    }
    if ((self->last_ptr == NULL) != (self->last_size == 0)) {
        LOG_FATAL("ArenaAllocator: last_ptr/last_size mismatch ({x} / {})", (u64)self->last_ptr, (u64)self->last_size);
    }
    if (self->last_ptr && !self->head) {
        LOG_FATAL("ArenaAllocator: last_ptr is set but chunk list is empty");
    }
}

static void arena_validate_self(const ArenaAllocator *self) {
    if (!self) {
        LOG_FATAL("ArenaAllocator: NULL self");
    }
    if (!MAGIC_MATCHES(self->base.__magic, ARENA_ALLOCATOR_MAGIC)) {
        LOG_FATAL("type-confusion: allocator passed to arena_allocator_* is not an ArenaAllocator");
    }
    if (!(self->base.__magic & MAGIC_VALIDATED_BIT)) {
        return; // memoized
    }
    arena_validate_self_structural(self);
    ((ArenaAllocator *)(void *)self)->base.__magic &= ~MAGIC_VALIDATED_BIT;
}

#define ARENA_DEFAULT_CHUNK_SIZE (size)(64 * 1024)

struct ArenaChunk {
    struct ArenaChunk *next;
    u8                *base;
    size               capacity;
    size               used;
    size               raw_size;
};

// True if `ptr` lies inside any chunk's user region. Used by both
// remap and deallocate to distinguish "legitimate arena-owned pointer
// the caller is freeing/remapping" from "foreign pointer".
static bool arena_owns_pointer(const ArenaAllocator *arena, const void *ptr) {
    const ArenaChunk *chunk = arena->head;
    while (chunk) {
        const u8 *base = chunk->base;
        if ((const u8 *)ptr >= base && (const u8 *)ptr < base + chunk->capacity) {
            return true;
        }
        chunk = chunk->next;
    }
    return false;
}

static size arena_chunk_size_for(size need_bytes) {
    size minimum = ARENA_DEFAULT_CHUNK_SIZE;
    size wanted  = need_bytes > minimum ? need_bytes : minimum;
    return os_page_round_up(wanted);
}

static ArenaChunk *arena_new_chunk(ArenaAllocator *arena, size need_bytes) {
    size chunk_bytes = arena_chunk_size_for(need_bytes + sizeof(ArenaChunk));
    u8  *raw         = (u8 *)os_page_map(&arena->base, chunk_bytes);
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

void *arena_allocator_allocate(ArenaAllocator *self, size bytes, i8 zeroed) {
    arena_validate_self(self);
    if (!bytes) {
        return NULL;
    }
    size  align  = self->base.alignment;
    size  padded = ALIGN_UP_POW2(bytes, align);
    void *result = NULL;

    ArenaChunk *chunk = self->tail;
    if (chunk) {
        u64  base_addr    = (u64)chunk->base;
        u64  free_addr    = base_addr + chunk->used;
        u64  aligned_addr = (free_addr + (u64)(align - 1)) & ~(u64)(align - 1);
        size aligned_used = (size)(aligned_addr - base_addr);
        if (aligned_used + padded <= chunk->capacity) {
            u8 *out         = chunk->base + aligned_used;
            chunk->used     = aligned_used + padded;
            self->last_ptr  = out;
            self->last_size = padded;
            // Fresh kernel pages are zero, but bumping into an
            // already-touched chunk (or one that was reused after
            // ArenaAllocatorReset) means the slot can contain stale
            // bytes from the previous tenant. Honor `zeroed`
            // unconditionally on the in-chunk path.
            if (zeroed) {
                MemSet(out, 0, padded);
            }
            result = out;
            goto done;
        }
    }

    chunk = arena_new_chunk(self, padded + align);
    if (!chunk) {
        goto done;
    }
    if (!self->head) {
        self->head = chunk;
    } else {
        self->tail->next = chunk;
    }
    self->tail = chunk;
    // No MARK_DIRTY: chunk-list grow moves head/tail from (NULL,NULL) ->
    // (chunk,chunk) or (h,t) -> (h,chunk); the structural invariant
    // (head==NULL) == (tail==NULL) is preserved either way.

    {
        u64  base_addr    = (u64)chunk->base;
        u64  aligned_addr = (base_addr + (u64)(align - 1)) & ~(u64)(align - 1);
        size aligned_used = (size)(aligned_addr - base_addr);
        u8  *out          = chunk->base + aligned_used;
        chunk->used       = aligned_used + padded;
        self->last_ptr    = out;
        self->last_size   = padded;
        // Fresh chunk came from os_page_map -> kernel-zeroed, so a
        // `zeroed` request is already satisfied without an extra MemSet.
        (void)zeroed;
        result = out;
    }

done:
#if FEATURE_ALLOC_STATS
    if (result) {
        // bytes_in_use tracks effective reserved bytes (post-alignment
        // padding), matching what arena_allocator_deallocate's rewound
        // path subtracts. `bytes` (the user request) flows into
        // bytes_requested instead -- that's the cumulative user-demand
        // counter.
        self->base.stats.allocations     += 1u;
        self->base.stats.bytes_requested += (u64)bytes;
        self->base.stats.bytes_in_use    += (u64)padded;
        if (self->base.stats.bytes_in_use > self->base.stats.peak_bytes_in_use) {
            self->base.stats.peak_bytes_in_use = self->base.stats.bytes_in_use;
        }
    } else {
        self->base.stats.failed_allocations += 1u;
    }
#endif
    return result;
}

// Try to grow/shrink in place. The arena can only do this when `ptr`
// is the most recent allocation (its end coincides with the chunk's
// high-water mark), because earlier allocations have stuff after
// them that we can't disturb. Shrinks of older allocations: refused
// too -- they'd leave a hole the arena can't reclaim and the caller
// could just keep the over-large slot. Same answer either way.
i8 arena_allocator_resize(ArenaAllocator *self, void *ptr, size new_size) {
    arena_validate_self(self);
    size align = self->base.alignment;

    if (self->last_ptr != ptr || !self->tail) {
        return 0;
    }
    ArenaChunk *chunk      = self->tail;
    size        padded_new = ALIGN_UP_POW2(new_size, align);
    size        last_off   = (size)((u8 *)ptr - chunk->base);
    if (last_off + padded_new > chunk->capacity) {
        return 0; // grow doesn't fit in this chunk
    }
    chunk->used     = last_off + padded_new;
    self->last_size = padded_new;
#if FEATURE_ALLOC_STATS
    // In-place resize does NOT move bytes_in_use (see AllocatorStats
    // doc in Allocator.h), so no peak refresh is possible here.
    self->base.stats.reallocations   += 1u;
    self->base.stats.bytes_requested += (u64)new_size;
#endif
    return 1;
}

void *arena_allocator_remap(ArenaAllocator *self, void *ptr, size new_size) {
    arena_validate_self(self);

    if (new_size == 0) {
        (void)ptr;
        return NULL;
    }
    if (!ptr) {
        return arena_allocator_allocate(self, new_size, true);
    }

    // Grow in place when `ptr` is the last bump (still a fast path
    // for remap callers that come straight here without trying
    // resize first).
    if (arena_allocator_resize(self, ptr, new_size)) {
        return ptr;
    }

    // Foreign pointer or non-last bump: abort. The LOG_FATAL messages
    // below carry the distinction.
    if (!arena_owns_pointer(self, ptr)) {
        LOG_FATAL("arena_remap: foreign ptr {x} (not in any chunk)", (u64)ptr);
        return NULL;
    }
    if (self->last_ptr != ptr) {
        LOG_FATAL(
            "arena_remap: ptr {x} is not the most recent bump; bump allocators "
            "cannot remap mid-stream allocations. Use HeapAllocator for "
            "resize-of-anything.",
            (u64)ptr
        );
        return NULL;
    }
    size  old_padded = self->last_size;
    void *fresh      = arena_allocator_allocate(self, new_size, true);
    if (!fresh) {
        return NULL;
    }
    MemCopy(fresh, ptr, old_padded < new_size ? old_padded : new_size);
#if FEATURE_ALLOC_STATS
    // The user is abandoning the old `ptr` in favour of `fresh`. The
    // arena keeps the old bytes physically reserved until the next
    // reset/deinit, but from the user's perspective those bytes are
    // gone -- draw them out of bytes_in_use so the counter stays in
    // sync with what the caller is actually holding. The
    // arena_allocator_allocate call above already bumped bytes_in_use
    // for `fresh`; without this draw-down the remap would double-count
    // the old allocation.
    if ((u64)old_padded <= self->base.stats.bytes_in_use) {
        self->base.stats.bytes_in_use -= (u64)old_padded;
    } else {
        self->base.stats.bytes_in_use = 0u;
    }
#endif
    return fresh;
}

size arena_allocator_deallocate(ArenaAllocator *self, void *ptr) {
    arena_validate_self(self);
    if (!ptr) {
        return 0;
    }
    // Rewind only when the caller is freeing the most recent bump.
    if (self->last_ptr == ptr && self->tail) {
        ArenaChunk *chunk   = self->tail;
        size        rewound = self->last_size;
        if (chunk->used >= rewound) {
            chunk->used -= rewound;
        }
        self->last_ptr  = NULL;
        self->last_size = 0;
#if FEATURE_ALLOC_STATS
        self->base.stats.deallocations += 1u;
        if ((u64)rewound <= self->base.stats.bytes_in_use) {
            self->base.stats.bytes_in_use -= (u64)rewound;
        } else {
            self->base.stats.bytes_in_use = 0u;
        }
#endif
        return rewound;
    }
    // Mid-stream free of an arena-owned pointer is a no-op under the
    // bump policy: the bytes get reclaimed at Reset / Deinit. We still
    // verify ownership -- a foreign pointer is a caller bug and aborts.
    if (!arena_owns_pointer(self, ptr)) {
        LOG_FATAL("arena_free: foreign ptr {x} (not in any chunk)", (u64)ptr);
        return 0;
    }
    return 0;
}

void ArenaAllocatorReset(ArenaAllocator *self) {
    if (!self) {
        return;
    }
    arena_validate_self(self);
    ArenaChunk *chunk = self->head;
    while (chunk) {
        chunk->used = 0;
        chunk       = chunk->next;
    }
    self->last_ptr  = NULL;
    self->last_size = 0;
    // No MARK_DIRTY: both fields go to zero together; the structural
    // invariant (last_ptr==NULL) == (last_size==0) is preserved.
}

void ArenaAllocatorDeinit(ArenaAllocator *self) {
    if (!self) {
        return;
    }
    ArenaChunk *chunk = self->head;
    while (chunk) {
        ArenaChunk *next      = chunk->next;
        size        raw_bytes = chunk->raw_size;
        os_page_unmap(&self->base, (void *)chunk, raw_bytes);
        chunk = next;
    }
    MemSet(self, 0, sizeof(*self));
}
