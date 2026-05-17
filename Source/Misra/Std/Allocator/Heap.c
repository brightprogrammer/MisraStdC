/// file      : std/allocator/heap.c
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Per-descriptor binned heap allocator. Each `HeapAllocator` value carries
/// its own bin free-lists and page-chunk list inline; the library owns no
/// shared global heap state. Page acquisition goes through the embedded
/// `PageAllocator`, so the implementation uses no libc heap functions.

#include <Misra/Std/Allocator/Heap.h>
#include <Misra/Std/Allocator/Page.h>
#include <Misra/Std/Log.h>
#include <Misra/Std/Memory.h>

static void heap_validate_self(const Allocator *self) {
    if (!self || self->__magic != HEAP_ALLOCATOR_MAGIC) {
        LOG_FATAL("type-confusion: allocator passed to heap_allocator_* is not a HeapAllocator");
    }
}

struct HeapFreeSlot {
    struct HeapFreeSlot *next;
};

struct HeapPageChunk {
    struct HeapPageChunk *next;
    size                  bytes;
};

static int heap_bin_for_size(size n) {
    size cls = (size)1u << HEAP_MIN_BIN_LOG;
    int  bin = 0;
    while (cls < n && (u32)bin < HEAP_NUM_BINS) {
        cls <<= 1;
        bin++;
    }
    return (u32)bin < HEAP_NUM_BINS ? bin : -1;
}

static size heap_bin_size(int bin) {
    return (size)1u << (HEAP_MIN_BIN_LOG + (u32)bin);
}

static bool heap_alignment_demands_passthrough(Allocator *self) {
    // Bin slots are 16-byte aligned. Stronger alignment bypasses bins and
    // uses page-aligned memory directly.
    return self->alignment > 16;
}

static bool heap_grow_bin(HeapAllocator *heap, int bin) {
    size  slot_size = heap_bin_size(bin);
    size  page_size = PageAllocatorPageSize(&heap->page);
    char *raw       = (char *)AllocatorAlloc(&heap->page.base, page_size, false);
    if (!raw) {
        return false;
    }

    // Reserve the very first slot in the page as the chunk header so we can
    // walk and unmap chunks on deinit. This costs us one slot per page.
    HeapPageChunk *chunk = (HeapPageChunk *)(void *)raw;
    chunk->next          = heap->chunks_head;
    chunk->bytes         = page_size;
    heap->chunks_head    = chunk;

    size  header_slots = (sizeof(HeapPageChunk) + slot_size - 1) / slot_size;
    size  n_slots      = page_size / slot_size;
    char *cursor       = raw + header_slots * slot_size;
    for (size i = header_slots; i < n_slots; i++) {
        HeapFreeSlot *slot  = (HeapFreeSlot *)(void *)cursor;
        slot->next          = heap->bins[bin];
        heap->bins[bin]     = slot;
        cursor             += slot_size;
    }
    return true;
}

void *heap_allocator_allocate(Allocator *self, size bytes, i8 zeroed) {
    heap_validate_self(self);
    HeapAllocator *heap = (HeapAllocator *)self;
    if (!bytes) {
        return NULL;
    }

    if (heap_alignment_demands_passthrough(self)) {
        // Forward to page allocator with the heap's alignment carried through.
        heap->page.base.alignment = self->alignment;
        return AllocatorAlloc(&heap->page.base, bytes, zeroed);
    }

    int bin = heap_bin_for_size(bytes);
    if (bin < 0) {
        // Large allocation - pass to the embedded page allocator.
        return AllocatorAlloc(&heap->page.base, bytes, zeroed);
    }

    if (!heap->bins[bin]) {
        if (!heap_grow_bin(heap, bin)) {
            return NULL;
        }
    }
    HeapFreeSlot *slot = heap->bins[bin];
    heap->bins[bin]    = slot->next;
    if (zeroed) {
        MemSet(slot, 0, heap_bin_size(bin));
    }
    return slot;
}

void heap_allocator_deallocate(Allocator *self, void *ptr, size bytes) {
    heap_validate_self(self);
    HeapAllocator *heap = (HeapAllocator *)self;
    if (!ptr) {
        return;
    }

    if (heap_alignment_demands_passthrough(self)) {
        heap->page.base.alignment = self->alignment;
        AllocatorFree(&heap->page.base, ptr, bytes);
        return;
    }

    int bin = heap_bin_for_size(bytes);
    if (bin < 0) {
        AllocatorFree(&heap->page.base, ptr, bytes);
        return;
    }

    HeapFreeSlot *slot = (HeapFreeSlot *)ptr;
    slot->next         = heap->bins[bin];
    heap->bins[bin]    = slot;
}

// In-place resize: succeeds only when old + new sizes round to the
// same bin. The binned heap's allocations are bucketed at allocation
// time; a size change that stays inside the same bucket needs zero
// work (the slot is already that big). A size change that crosses
// bins requires picking a slot from a different bucket -- that's a
// remap, not a resize.
i8 heap_allocator_resize(Allocator *self, void *ptr, size old_size, size new_size) {
    heap_validate_self(self);
    (void)ptr;
    if (heap_alignment_demands_passthrough(self)) {
        return 0;
    }
    int old_bin = heap_bin_for_size(old_size);
    int new_bin = heap_bin_for_size(new_size);
    return (old_bin >= 0 && old_bin == new_bin) ? 1 : 0;
}

void *heap_allocator_remap(Allocator *self, void *ptr, size old_size, size new_size) {
    heap_validate_self(self);
    if (new_size == 0) {
        if (ptr) {
            heap_allocator_deallocate(self, ptr, old_size);
        }
        return NULL;
    }
    if (!ptr) {
        return heap_allocator_allocate(self, new_size, false);
    }
    // Same bin: keep in place. (Repeat of the resize check so a
    // direct remap caller still gets the no-copy fast path.)
    if (heap_allocator_resize(self, ptr, old_size, new_size)) {
        return ptr;
    }

    void *fresh = heap_allocator_allocate(self, new_size, false);
    if (!fresh) {
        return NULL;
    }
    MemCopy(fresh, ptr, old_size < new_size ? old_size : new_size);
    heap_allocator_deallocate(self, ptr, old_size);
    return fresh;
}

void HeapAllocatorDeinit(HeapAllocator *self) {
    if (!self) {
        return;
    }

    HeapPageChunk *chunk = self->chunks_head;
    while (chunk) {
        HeapPageChunk *next = chunk->next;
        AllocatorFree(&self->page.base, (void *)chunk, chunk->bytes);
        chunk = next;
    }

    // Zero the whole allocator so any post-deinit dispatch trips
    // ValidateAllocator on a zero __magic instead of silently
    // re-using a stale function-pointer table.
    MemSet(self, 0, sizeof(*self));
}
