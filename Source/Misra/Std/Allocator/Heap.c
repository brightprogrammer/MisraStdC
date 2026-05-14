/// file      : std/allocator/heap.c
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Process-global heap allocator. Small allocations (<= 2 KiB) come from
/// power-of-two bins backed by an intrusive free-list; large allocations
/// pass straight through to `PageAllocator`. The implementation does not
/// call any libc heap functions.

#include <Misra/Std/Allocator/Heap.h>
#include <Misra/Std/Allocator/Page.h>
#include <Misra/Std/Memory.h>

#define HEAP_MIN_BIN_LOG   4u  // smallest bin is 2^4 = 16 bytes
#define HEAP_MAX_BIN_LOG   11u // largest bin  is 2^11 = 2048 bytes
#define HEAP_NUM_BINS      (HEAP_MAX_BIN_LOG - HEAP_MIN_BIN_LOG + 1u)
#define HEAP_MAX_BIN_BYTES ((size)1u << HEAP_MAX_BIN_LOG)

typedef struct HeapFreeSlot {
    struct HeapFreeSlot *next;
} HeapFreeSlot;

static struct {
    HeapFreeSlot *bins[HEAP_NUM_BINS];
    Allocator     page;
    bool          initialized;
} g_heap;

static void heap_lazy_init(void) {
    if (!g_heap.initialized) {
        g_heap.page = PageAllocator();
        for (u32 i = 0; i < HEAP_NUM_BINS; i++) {
            g_heap.bins[i] = NULL;
        }
        g_heap.initialized = true;
    }
}

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

static bool heap_grow_bin(int bin) {
    size  slot_size = heap_bin_size(bin);
    size  page_size = PageAllocatorPageSize();
    char *page      = (char *)AllocatorAlloc(&g_heap.page, page_size, false);
    if (!page) {
        return false;
    }

    size n_slots = page_size / slot_size;
    for (size i = 0; i < n_slots; i++) {
        HeapFreeSlot *slot = (HeapFreeSlot *)(void *)(page + i * slot_size);
        slot->next         = g_heap.bins[bin];
        g_heap.bins[bin]   = slot;
    }
    return true;
}

static bool heap_alignment_demands_passthrough(const Allocator *alloc) {
    // Bins are 16-byte aligned by construction. Anything stronger needs to
    // bypass the bin path and use the page allocator (which gives
    // page-aligned memory).
    return alloc && alloc->alignment > 16;
}

static void *heap_allocate(Allocator *alloc, size bytes, bool zeroed) {
    int bin;

    if (!bytes) {
        return NULL;
    }

    heap_lazy_init();

    if (heap_alignment_demands_passthrough(alloc)) {
        return AllocatorAlloc(&g_heap.page, bytes, zeroed);
    }

    bin = heap_bin_for_size(bytes);
    if (bin < 0) {
        // Large allocation - pass straight through to the page allocator.
        return AllocatorAlloc(&g_heap.page, bytes, zeroed);
    }

    if (!g_heap.bins[bin]) {
        if (!heap_grow_bin(bin)) {
            return NULL;
        }
    }

    HeapFreeSlot *slot = g_heap.bins[bin];
    g_heap.bins[bin]   = slot->next;

    if (zeroed) {
        MemSet(slot, 0, heap_bin_size(bin));
    }

    return slot;
}

static void heap_deallocate_impl(Allocator *alloc, void *ptr, size bytes) {
    int bin;

    if (!ptr) {
        return;
    }

    heap_lazy_init();

    if (heap_alignment_demands_passthrough(alloc)) {
        AllocatorFree(&g_heap.page, ptr, bytes);
        return;
    }

    bin = heap_bin_for_size(bytes);
    if (bin < 0) {
        AllocatorFree(&g_heap.page, ptr, bytes);
        return;
    }

    HeapFreeSlot *slot = (HeapFreeSlot *)ptr;
    slot->next         = g_heap.bins[bin];
    g_heap.bins[bin]   = slot;
}

static void *heap_reallocate(Allocator *alloc, void *ptr, size old_size, size new_size) {
    if (new_size == 0) {
        if (ptr) {
            heap_deallocate_impl(alloc, ptr, old_size);
        }
        return NULL;
    }

    if (!ptr) {
        return heap_allocate(alloc, new_size, false);
    }

    // Same bin (or both above the binned range and same page size): keep the
    // slot in place.
    if (!heap_alignment_demands_passthrough(alloc)) {
        int old_bin = heap_bin_for_size(old_size);
        int new_bin = heap_bin_for_size(new_size);
        if (old_bin >= 0 && old_bin == new_bin) {
            return ptr;
        }
    }

    void *fresh = heap_allocate(alloc, new_size, false);
    if (!fresh) {
        return NULL;
    }
    MemCopy(fresh, ptr, old_size < new_size ? old_size : new_size);
    heap_deallocate_impl(alloc, ptr, old_size);
    return fresh;
}

static void heap_deallocate(Allocator *alloc, void *ptr, size bytes) {
    heap_deallocate_impl(alloc, ptr, bytes);
}

Allocator HeapAllocator(void) {
    return (Allocator) {
        .state        = NULL,
        .state_init   = NULL,
        .state_deinit = NULL,
        .allocate     = heap_allocate,
        .reallocate   = heap_reallocate,
        .deallocate   = heap_deallocate,
        .effort       = ALLOCATOR_EFFORT_ONCE,
        .retry_limit  = 0,
        .flags        = 0,
        // Default alignment of 1 means "no stronger requirement than the
        // backing allocator's natural alignment". Small bins are 16-byte
        // aligned by construction; large allocations are page-aligned.
        .alignment = 1,
    };
}

Allocator HeapAllocatorAligned(size alignment) {
    Allocator alloc = HeapAllocator();
    if (alignment) {
        alloc.alignment = alignment;
    }
    return alloc;
}
