/// file      : std/allocator/slab.c
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Per-descriptor fixed-size slot bitmap allocator.
///
/// Each chunk is a single page-backed allocation partitioned into two
/// disjoint regions:
///
///     [ chunk header ][ bitmap ][ pad ][ slot 0 ] ... [ slot N-1 ]
///     ^               ^                ^
///     chunk           bitmap           slots
///
/// The chunk header AND the bitmap are allocator-owned metadata. The
/// slot region is user data. User pointers returned by Alloc always
/// lie in the slot region; Free range-checks the pointer against the
/// slot region, validates alignment within it, and only then touches
/// the bitmap. No metadata is written through the user pointer for the
/// rest of its life.
///
/// Slot state machine:
///     FREE -- Alloc --> IN_USE -- Free --> FREE
///     pre: bit==0       pre: bit==1
///     post: bit:=1      post: bit:=0

#include <Misra/Std/Allocator/Page.h>
#include <Misra/Std/Allocator/Slab.h>
#include <Misra/Std/Log.h>
#include <Misra/Std/Memory.h>

// Fast path: magic on the allocator + magic on the embedded
// PageAllocator. Catches uninitialised / post-deinit / _Generic
// mismatch escape. Always on, FORCE_INLINEd into every caller --
// same treatment as heap_validate_self_fast in Heap.c. Without
// FORCE_INLINE gcc emits a standalone copy at -O3 because the
// LOG_FATAL macro expansions count against the inline cost
// heuristic; that standalone copy then dominates the perf profile
// of every alloc/free pair (51% of self-time in the bench before
// this change).
//
// Full path (FEATURE_HEAP_VALIDATE_FULL): adds vtable / alignment /
// slot_size / slots_per_chunk / head-tail consistency / free_head
// staleness / embedded PageAllocator magic. Costs ~7 ns / dispatch
// when on. `free_head` is a vestigial field from the pre-bitmap
// scheme; the current implementation never sets it, so it must
// stay NULL.
static FORCE_INLINE void slab_validate_self_fast(const Allocator *self) {
    if (!self) {
        LOG_FATAL("SlabAllocator: NULL self");
    }
    if (self->__magic != SLAB_ALLOCATOR_MAGIC) {
        LOG_FATAL("type-confusion: allocator passed to slab_allocator_* is not a SlabAllocator");
    }
    const SlabAllocator *s = (const SlabAllocator *)self;
    if (s->page.base.__magic != PAGE_ALLOCATOR_MAGIC) {
        LOG_FATAL("SlabAllocator: embedded PageAllocator has bad magic");
    }
}

#if FEATURE_HEAP_VALIDATE_FULL
static void slab_validate_self_full(const Allocator *self) {
    slab_validate_self_fast(self);
    if (!self->allocate || !self->resize || !self->remap || !self->deallocate) {
        LOG_FATAL("SlabAllocator: vtable function pointer is NULL");
    }
    if (self->alignment == 0 || (self->alignment & (self->alignment - 1)) != 0) {
        LOG_FATAL("SlabAllocator: alignment {} is not a positive power of two", (u64)self->alignment);
    }
    const SlabAllocator *s = (const SlabAllocator *)self;
    if (s->slot_size == 0) {
        LOG_FATAL("SlabAllocator: slot_size is 0");
    }
    if (s->slots_per_chunk == 0) {
        LOG_FATAL("SlabAllocator: slots_per_chunk is 0");
    }
    if ((s->head == NULL) != (s->tail == NULL)) {
        LOG_FATAL("SlabAllocator: head/tail mismatch ({x} / {x})", (u64)s->head, (u64)s->tail);
    }
    if (s->free_head != NULL) {
        LOG_FATAL("SlabAllocator: stale free_head pointer ({x}); bitmap scheme leaves it NULL", (u64)s->free_head);
    }
}
#endif

#if FEATURE_HEAP_VALIDATE_FULL
#    define slab_validate_self(self) slab_validate_self_full(self)
#else
#    define slab_validate_self(self) slab_validate_self_fast(self)
#endif

struct SlabChunk {
    struct SlabChunk *next;
    void             *raw;          // base of the page-backed region (for Free on Deinit)
    size              raw_size;     // bytes mmap'd from PageAllocator
    u64              *bitmap;       // owned-by-this-chunk metadata region
    u32               bitmap_words; // u64 count
    char             *slots;        // start of user slot region
    u32               slot_count;
};

// SlabFreeSlot from the old API stays declared (it's referenced by
// the typedef in Slab.h) but is unused under the bitmap scheme.
struct SlabFreeSlot {
    int _unused;
};

static size slab_padded_slot_size(size slot_size, size alignment) {
    if (alignment < sizeof(void *))
        alignment = sizeof(void *);
    return ALIGN_UP_POW2(slot_size, alignment);
}

#if defined(_MSC_VER) && !defined(__clang__)
#    include <intrin.h>
static u32 ctz64(u64 x) {
    unsigned long idx;
    _BitScanForward64(&idx, x);
    return (u32)idx;
}
#else
static u32 ctz64(u64 x) {
    return (u32)__builtin_ctzll(x);
}
#endif

// ---------------------------------------------------------------------------
// Chunk allocation. Layout inside one page-backed chunk:
//   [ SlabChunk header ][ u64 bitmap ][ alignment pad ][ slots... ]

static bool slab_grow(SlabAllocator *slab) {
    size align       = slab->base.alignment > 1 ? slab->base.alignment : sizeof(void *);
    size padded_slot = slab_padded_slot_size(slab->slot_size, align);
    size slot_count  = slab->slots_per_chunk;

    // `slot_count` and `bitmap_words` are stored u32 on the chunk
    // header. Refuse configurations that would truncate either --
    // a wrapped u32 slot_count makes the alloc-side scan loop
    // forever or skip live slots silently. Bound is generous
    // (2^32 slots, 2^32 64-bit bitmap words) but the numerics are
    // now closed.
    if (slot_count == 0 || slot_count > (size)(u32)-1) {
        return false;
    }
    size header_bytes = ALIGN_UP_POW2(sizeof(struct SlabChunk), sizeof(u64));
    // (slot_count + 63u) cannot wrap because slot_count <= U32_MAX.
    size bitmap_words_full = (slot_count + 63u) / 64u;
    if (bitmap_words_full > (size)(u32)-1) {
        return false;
    }
    size bitmap_bytes = bitmap_words_full * 8u;
    // raw_bytes math: every term is bounded above. slot_count*padded_slot
    // is the dominant term; refuse if it would wrap a size.
    if (padded_slot && slot_count > ((size)-1) / padded_slot) {
        return false;
    }
    size slots_bytes = slot_count * padded_slot;
    size pre_slots   = header_bytes + bitmap_bytes + (align - 1);
    if (pre_slots > (size)-1 - slots_bytes) {
        return false;
    }
    size raw_bytes = pre_slots + slots_bytes;

    char *raw = (char *)AllocatorAlloc(&slab->page.base, raw_bytes, true);
    if (!raw)
        return false;

    struct SlabChunk *chunk = (struct SlabChunk *)(void *)raw;
    chunk->next             = NULL;
    chunk->raw              = raw;
    chunk->raw_size         = raw_bytes;
    chunk->bitmap           = (u64 *)(void *)(raw + header_bytes);
    chunk->bitmap_words     = (u32)bitmap_words_full;
    char *slots_raw         = raw + header_bytes + bitmap_bytes;
    u64   aligned           = ((u64)slots_raw + (u64)(align - 1)) & ~(u64)(align - 1);
    chunk->slots            = (char *)(void *)aligned;
    chunk->slot_count       = (u32)slot_count;

    if (!slab->head)
        slab->head = chunk;
    else
        slab->tail->next = chunk;
    slab->tail = chunk;
    return true;
}

// ---------------------------------------------------------------------------
// Public alloc / free / resize / remap.

void *slab_allocator_allocate(Allocator *self, size bytes, i8 zeroed) {
    slab_validate_self(self);
    SlabAllocator *slab        = (SlabAllocator *)self;
    size           align       = self->alignment > 1 ? self->alignment : sizeof(void *);
    size           padded_slot = slab_padded_slot_size(slab->slot_size, align);

    if (bytes == 0 || bytes > padded_slot)
        return NULL;

    // Find a chunk with at least one free bit.
    struct SlabChunk *chunk = slab->head;
    while (chunk) {
        for (u32 w = 0; w < chunk->bitmap_words; w++) {
            u64 inv = ~chunk->bitmap[w];
            if (inv == 0)
                continue;
            u32  bit    = ctz64(inv);
            size global = (size)w * 64u + bit;
            if (global >= chunk->slot_count)
                break;
            if (chunk->bitmap[w] & ((u64)1 << bit)) {
                LOG_FATAL("SlabAllocator bitmap corruption: chunk {x} word {} bit {}", (u64)chunk, (u64)w, (u64)bit);
            }
            chunk->bitmap[w] |= ((u64)1 << bit);
            void *slot        = chunk->slots + global * padded_slot;
            if (zeroed)
                MemSet(slot, 0, padded_slot);
            return slot;
        }
        chunk = chunk->next;
    }

    // All chunks full -- grow and retry on the new chunk.
    if (!slab_grow(slab))
        return NULL;
    chunk = slab->tail;
    // First slot is guaranteed free.
    chunk->bitmap[0] |= 1u;
    void *slot        = chunk->slots;
    if (zeroed)
        MemSet(slot, 0, padded_slot);
    return slot;
}

i8 slab_allocator_resize(Allocator *self, void *ptr, size new_size) {
    slab_validate_self(self);
    SlabAllocator *slab   = (SlabAllocator *)self;
    size           align  = self->alignment > 1 ? self->alignment : sizeof(void *);
    size           padded = slab_padded_slot_size(slab->slot_size, align);
    (void)ptr;
    return new_size <= padded ? 1 : 0;
}

void *slab_allocator_remap(Allocator *self, void *ptr, size new_size) {
    slab_validate_self(self);
    SlabAllocator *slab   = (SlabAllocator *)self;
    size           align  = self->alignment > 1 ? self->alignment : sizeof(void *);
    size           padded = slab_padded_slot_size(slab->slot_size, align);

    if (!ptr)
        return slab_allocator_allocate(self, new_size, true);
    if (new_size == 0) {
        slab_allocator_deallocate(self, ptr);
        return NULL;
    }
    return new_size <= padded ? ptr : NULL;
}

size slab_allocator_deallocate(Allocator *self, void *ptr) {
    slab_validate_self(self);
    SlabAllocator *slab        = (SlabAllocator *)self;
    size           align       = self->alignment > 1 ? self->alignment : sizeof(void *);
    size           padded_slot = slab_padded_slot_size(slab->slot_size, align);
    if (!ptr)
        return 0;

    // Find the chunk whose slot range contains ptr.
    char             *p     = (char *)ptr;
    struct SlabChunk *chunk = slab->head;
    while (chunk) {
        char *end = chunk->slots + (size)chunk->slot_count * padded_slot;
        if (p >= chunk->slots && p < end) {
            size off = (size)(p - chunk->slots);
            if (off % padded_slot != 0) {
                LOG_FATAL("slab_free: misaligned ptr {x} (slot size {})", (u64)p, (u64)padded_slot);
                return 0;
            }
            size idx = off / padded_slot;
            u32  w   = (u32)(idx >> 6);
            u32  b   = (u32)(idx & 63u);
            if (!(chunk->bitmap[w] & ((u64)1 << b))) {
                LOG_FATAL("slab_free: double-free of {x} (idx {})", (u64)p, (u64)idx);
                return 0;
            }
            chunk->bitmap[w] &= ~((u64)1 << b);
            return padded_slot;
        }
        chunk = chunk->next;
    }
    LOG_FATAL("slab_free: foreign ptr {x} not in any chunk's slot region", (u64)p);
    return 0;
}

void SlabAllocatorDeinit(SlabAllocator *self) {
    if (!self)
        return;
    struct SlabChunk *chunk = self->head;
    while (chunk) {
        struct SlabChunk *next = chunk->next;
        void             *raw  = chunk->raw;
        AllocatorFree(&self->page.base, raw);
        chunk = next;
    }
    MemSet(self, 0, sizeof(*self));
}
