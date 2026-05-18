/// file      : Tests/Std/Allocator.Heap.c
///
/// State-machine + edge-case tests for HeapAllocator.
///
/// Slot state machine:
///
///                  Alloc(size)               Free(ptr,size)
///       FREE  --------------------->  IN_USE  -------------------->  FREE
///         ^   pre:  bit == 0          | pre:  bit == 1, ptr is slot
///         |   post: bit := 1          |       base, in region, aligned
///         |                           | post: bit := 0
///         |                           |
///         |                           +--[reject: double-free,
///         |                              foreign, mid-alloc,
///         +------------------------------- misaligned, wrong-size]
///                                          → state unchanged
///
/// Rejection edges that MUST NOT mutate any bitmap or descriptor:
///   - foreign pointer       (not in any descriptor list)
///   - wrong size hint       (routes to wrong class -> lookup misses)
///   - misaligned pointer    (not at slot boundary inside its region)
///   - mid-allocation XL ptr (ptr != XL descriptor base)
///   - double-free           (target bit already 0)
///
/// Each rejection test verifies: (a) the bad call returns without
/// LOG_FATAL, (b) a subsequent valid alloc/free on the same heap
/// still works, proving the bad call did not corrupt allocator state.

#include <Misra/Std/Allocator.h>
#include <Misra/Std/Allocator/Heap.h>
#include <Misra/Std/Io.h>
#include <Misra/Std/Log.h>
#include <Misra/Std/Memory.h>

#include "../Util/TestRunner.h"

// =============================================================================
// Happy path

static bool test_basic_alloc_free(void) {
    HeapAllocator heap  = HeapAllocatorInit();
    Allocator    *alloc = ALLOCATOR_OF(&heap);

    char *a  = (char *)AllocatorAlloc(alloc, 32, true);
    char *b  = (char *)AllocatorAlloc(alloc, 128, true);
    bool  ok = (a != NULL) && (b != NULL) && (a != b);

    if (ok) {
        a[0]   = 'A';
        a[31]  = 'Z';
        b[0]   = 'b';
        b[127] = 'B';
        ok     = (a[0] == 'A') && (a[31] == 'Z') && (b[0] == 'b') && (b[127] == 'B');
        AllocatorFree(alloc, a, 32);
        AllocatorFree(alloc, b, 128);
    }

    HeapAllocatorDeinit(&heap);
    return ok;
}

static bool test_zeroed_alloc(void) {
    HeapAllocator heap  = HeapAllocatorInit();
    Allocator    *alloc = ALLOCATOR_OF(&heap);

    u8  *p  = (u8 *)AllocatorAlloc(alloc, 64, true);
    bool ok = (p != NULL);
    for (size i = 0; ok && i < 64; i++) {
        if (p[i] != 0)
            ok = false;
    }
    if (p)
        AllocatorFree(alloc, p, 64);
    HeapAllocatorDeinit(&heap);
    return ok;
}

static bool test_zero_byte_alloc_returns_null(void) {
    HeapAllocator heap  = HeapAllocatorInit();
    Allocator    *alloc = ALLOCATOR_OF(&heap);

    void *p = AllocatorAlloc(alloc, 0, false);

    HeapAllocatorDeinit(&heap);
    return p == NULL;
}

static bool test_free_null_is_noop(void) {
    HeapAllocator heap  = HeapAllocatorInit();
    Allocator    *alloc = ALLOCATOR_OF(&heap);

    // Must be silent + harmless.
    AllocatorFree(alloc, NULL, 32);

    // Heap remains usable.
    void *p  = AllocatorAlloc(alloc, 32, false);
    bool  ok = (p != NULL);
    if (p)
        AllocatorFree(alloc, p, 32);
    HeapAllocatorDeinit(&heap);
    return ok;
}

// =============================================================================
// State machine: no-double-vending and post-free reuse

static bool test_alloc_returns_distinct_pointers(void) {
    // Three allocs of the same size must yield three distinct pointers.
    // After the third alloc, bits 0..2 of bitmap_32 should be set.
    HeapAllocator heap  = HeapAllocatorInit();
    Allocator    *alloc = ALLOCATOR_OF(&heap);

    void *a  = AllocatorAlloc(alloc, 32, false);
    void *b  = AllocatorAlloc(alloc, 32, false);
    void *c  = AllocatorAlloc(alloc, 32, false);
    bool  ok = (a != NULL) && (b != NULL) && (c != NULL) && (a != b) && (b != c) && (a != c);

    AllocatorFree(alloc, a, 32);
    AllocatorFree(alloc, b, 32);
    AllocatorFree(alloc, c, 32);
    HeapAllocatorDeinit(&heap);
    return ok;
}

static bool test_free_then_alloc_recycles(void) {
    HeapAllocator heap  = HeapAllocatorInit();
    Allocator    *alloc = ALLOCATOR_OF(&heap);

    void *a = AllocatorAlloc(alloc, 32, false);
    AllocatorFree(alloc, a, 32);
    void *b = AllocatorAlloc(alloc, 32, false);
    // ctz finds the LOWEST clear bit, which is the one we just freed.
    bool ok = (a != NULL) && (b == a);

    if (b)
        AllocatorFree(alloc, b, 32);
    HeapAllocatorDeinit(&heap);
    return ok;
}

static bool test_fill_class_grows_new_page(void) {
    // Class S/32 has 32 slots per page. Allocate 33 → second page must
    // appear in heap->s.
    HeapAllocator heap  = HeapAllocatorInit();
    Allocator    *alloc = ALLOCATOR_OF(&heap);

    void *ptrs[33];
    bool  ok = true;
    for (u32 i = 0; i < 33; i++) {
        ptrs[i] = AllocatorAlloc(alloc, 32, false);
        if (!ptrs[i])
            ok = false;
    }
    // After 33 allocations of 32 bytes, we must have 2 S-class pages.
    ok = ok && (heap.s_len == 2);

    for (u32 i = 0; i < 33; i++) {
        if (ptrs[i])
            AllocatorFree(alloc, ptrs[i], 32);
    }
    HeapAllocatorDeinit(&heap);
    return ok;
}

static bool test_alloc_across_every_sub_bin(void) {
    // Exercise every (class, sub-bin) at least once.
    HeapAllocator heap  = HeapAllocatorInit();
    Allocator    *alloc = ALLOCATOR_OF(&heap);

    size  sizes[] = {16, 32, 64, 128, 256, 512, 1024, 2048};
    void *ptrs[8];
    bool  ok = true;
    for (u32 i = 0; i < 8; i++) {
        ptrs[i] = AllocatorAlloc(alloc, sizes[i], true);
        if (!ptrs[i])
            ok = false;
    }
    // Pointers across sub-bins must be distinct.
    for (u32 i = 0; ok && i < 8; i++) {
        for (u32 j = i + 1; ok && j < 8; j++) {
            if (ptrs[i] == ptrs[j])
                ok = false;
        }
    }
    // All four classes must have at least one descriptor.
    ok = ok && (heap.s_len >= 1) && (heap.m_len >= 1) && (heap.l_len >= 1);

    for (u32 i = 0; i < 8; i++) {
        if (ptrs[i])
            AllocatorFree(alloc, ptrs[i], sizes[i]);
    }
    HeapAllocatorDeinit(&heap);
    return ok;
}

// =============================================================================
// Large + alignment

static bool test_large_alloc_passthrough(void) {
    // > 2 KiB hits the XL path.
    HeapAllocator heap  = HeapAllocatorInit();
    Allocator    *alloc = ALLOCATOR_OF(&heap);

    size n  = 64 * 1024;
    u8  *p  = (u8 *)AllocatorAlloc(alloc, n, true);
    bool ok = (p != NULL);
    if (ok) {
        p[0]     = 0xAB;
        p[n - 1] = 0xCD;
        ok       = (p[0] == 0xAB) && (p[n - 1] == 0xCD) && (heap.xl_len == 1);
        AllocatorFree(alloc, p, n);
        ok = ok && (heap.xl_len == 0);
    }
    HeapAllocatorDeinit(&heap);
    return ok;
}

static bool test_overaligned_alloc(void) {
    HeapAllocator heap  = HeapAllocatorInitAligned(64);
    Allocator    *alloc = ALLOCATOR_OF(&heap);

    void *p  = AllocatorAlloc(alloc, 256, true);
    bool  ok = (p != NULL) && (((u64)p & 63u) == 0);

    if (p)
        AllocatorFree(alloc, p, 256);
    HeapAllocatorDeinit(&heap);
    return ok;
}

// =============================================================================
// Realloc

static bool test_realloc_same_bin_keeps_pointer(void) {
    HeapAllocator heap  = HeapAllocatorInit();
    Allocator    *alloc = ALLOCATOR_OF(&heap);

    char *p  = (char *)AllocatorAlloc(alloc, 24, true);
    bool  ok = (p != NULL);
    if (ok) {
        p[0]        = 'x';
        char *grown = (char *)AllocatorRealloc(alloc, p, 24, 28);
        ok          = (grown == p) && (grown[0] == 'x');
        AllocatorFree(alloc, grown, 28);
    }
    HeapAllocatorDeinit(&heap);
    return ok;
}

static bool test_realloc_cross_bin_copies(void) {
    HeapAllocator heap  = HeapAllocatorInit();
    Allocator    *alloc = ALLOCATOR_OF(&heap);

    char *p  = (char *)AllocatorAlloc(alloc, 16, true);
    bool  ok = (p != NULL);
    if (ok) {
        p[0]        = 'h';
        p[15]       = '!';
        char *grown = (char *)AllocatorRealloc(alloc, p, 16, 200);
        // Different bin -> heap must copy into a fresh slot.
        ok = (grown != NULL) && (grown != p) && (grown[0] == 'h') && (grown[15] == '!');
        if (grown)
            AllocatorFree(alloc, grown, 200);
    }
    HeapAllocatorDeinit(&heap);
    return ok;
}

// =============================================================================
// Independence

static bool test_independent_heaps(void) {
    HeapAllocator h1     = HeapAllocatorInit();
    HeapAllocator h2     = HeapAllocatorInit();
    Allocator    *alloc1 = ALLOCATOR_OF(&h1);
    Allocator    *alloc2 = ALLOCATOR_OF(&h2);

    void *a  = AllocatorAlloc(alloc1, 32, true);
    void *b  = AllocatorAlloc(alloc2, 32, true);
    bool  ok = (a != NULL) && (b != NULL) && (a != b) && (h1.s != h2.s) && (h1.s_len > 0) && (h2.s_len > 0);

    AllocatorFree(alloc1, a, 32);
    AllocatorFree(alloc2, b, 32);
    HeapAllocatorDeinit(&h1);
    HeapAllocatorDeinit(&h2);
    return ok;
}

// =============================================================================
// Rejection edges -- each one verifies (a) the bad op returns without
// crashing and (b) the heap is still usable afterwards.
//
// All of these intentionally log via LOG_ERROR. Seeing those messages
// in test output is the expected signal that the validation kicked in.

static bool test_reject_foreign_pointer(void) {
    // Alloc from h1, attempt to free the resulting pointer via h2 (which
    // owns no descriptor for that page). h2 must reject and remain
    // usable; h1's slot must remain IN_USE.
    HeapAllocator h1     = HeapAllocatorInit();
    HeapAllocator h2     = HeapAllocatorInit();
    Allocator    *alloc1 = ALLOCATOR_OF(&h1);
    Allocator    *alloc2 = ALLOCATOR_OF(&h2);

    void *p = AllocatorAlloc(alloc1, 32, false);

    // Foreign-free attempt on h2.
    AllocatorFree(alloc2, p, 32);

    // h2 still works (was empty, now we exercise it).
    void *q  = AllocatorAlloc(alloc2, 64, false);
    bool  ok = (q != NULL);
    if (q)
        AllocatorFree(alloc2, q, 64);

    // h1's slot was never freed -- free it now to keep DebugAllocator happy.
    AllocatorFree(alloc1, p, 32);

    HeapAllocatorDeinit(&h1);
    HeapAllocatorDeinit(&h2);
    return ok;
}

static bool test_reject_double_free(void) {
    HeapAllocator heap  = HeapAllocatorInit();
    Allocator    *alloc = ALLOCATOR_OF(&heap);

    void *p = AllocatorAlloc(alloc, 32, false);
    AllocatorFree(alloc, p, 32);

    // Second free of the same pointer: bit is already 0 -> REJECT.
    AllocatorFree(alloc, p, 32);

    // Heap still usable.
    void *q  = AllocatorAlloc(alloc, 32, false);
    bool  ok = (q != NULL);
    // After the double-free was rejected, the slot remained FREE, so
    // ctz finds bit 0 again -> q should equal p.
    ok = ok && (q == p);

    if (q)
        AllocatorFree(alloc, q, 32);
    HeapAllocatorDeinit(&heap);
    return ok;
}

static bool test_reject_wrong_size_hint_routes_to_wrong_class(void) {
    // Alloc with size 32 -> class S. Free with bytes=128 -> claims
    // class M. h->m is empty so the page lookup misses -> REJECT.
    HeapAllocator heap  = HeapAllocatorInit();
    Allocator    *alloc = ALLOCATOR_OF(&heap);

    void *p = AllocatorAlloc(alloc, 32, false);

    AllocatorFree(alloc, p, 128); // lies: pretends this is class M

    // Heap still usable.
    void *q  = AllocatorAlloc(alloc, 64, false);
    bool  ok = (q != NULL);
    if (q)
        AllocatorFree(alloc, q, 64);

    // Real slot is still IN_USE -- free it correctly.
    AllocatorFree(alloc, p, 32);
    HeapAllocatorDeinit(&heap);
    return ok;
}

static bool test_reject_wrong_size_hint_in_class(void) {
    // Alloc with size 16 -> S/16 region (offset 0). Free with bytes=64
    // -> still class S, but ptr's offset is in the 16-byte region not
    // the 64-byte region -> REJECT via the region-bounds check.
    HeapAllocator heap  = HeapAllocatorInit();
    Allocator    *alloc = ALLOCATOR_OF(&heap);

    void *p = AllocatorAlloc(alloc, 16, false);
    AllocatorFree(alloc, p, 64); // wrong sub-bin within same class

    // Heap still usable; real slot still IN_USE.
    void *q  = AllocatorAlloc(alloc, 16, false);
    bool  ok = (q != NULL) && (q != p); // q must be a DIFFERENT slot
    if (q)
        AllocatorFree(alloc, q, 16);
    AllocatorFree(alloc, p, 16);

    HeapAllocatorDeinit(&heap);
    return ok;
}

static bool test_reject_misaligned_pointer(void) {
    HeapAllocator heap  = HeapAllocatorInit();
    Allocator    *alloc = ALLOCATOR_OF(&heap);

    char *p = (char *)AllocatorAlloc(alloc, 64, false);
    // Mid-slot pointer: in the right region, wrong alignment -> REJECT.
    AllocatorFree(alloc, p + 1, 64);

    // Heap still usable; original slot still IN_USE.
    void *q  = AllocatorAlloc(alloc, 64, false);
    bool  ok = (q != NULL) && (q != p);
    if (q)
        AllocatorFree(alloc, q, 64);
    AllocatorFree(alloc, p, 64);

    HeapAllocatorDeinit(&heap);
    return ok;
}

static bool test_reject_mid_xl_pointer(void) {
    HeapAllocator heap  = HeapAllocatorInit();
    Allocator    *alloc = ALLOCATOR_OF(&heap);

    size  n = 16 * 1024;
    char *p = (char *)AllocatorAlloc(alloc, n, false);
    // Mid-allocation pointer in XL -> REJECT.
    AllocatorFree(alloc, p + 128, n);

    // XL descriptor still present; real free succeeds.
    bool ok = (heap.xl_len == 1);
    AllocatorFree(alloc, p, n);
    ok = ok && (heap.xl_len == 0);

    HeapAllocatorDeinit(&heap);
    return ok;
}

static bool test_reject_double_free_does_not_double_vend(void) {
    // Defense against the classic libc-malloc freelist attack:
    // alloc, free, free, alloc, alloc -- the third alloc must NOT
    // return the same pointer as the second (which would be a
    // double-vending bug = use-after-free primitive).
    HeapAllocator heap  = HeapAllocatorInit();
    Allocator    *alloc = ALLOCATOR_OF(&heap);

    void *a = AllocatorAlloc(alloc, 32, false);
    AllocatorFree(alloc, a, 32);
    AllocatorFree(alloc, a, 32); // rejected
    void *b = AllocatorAlloc(alloc, 32, false);
    void *c = AllocatorAlloc(alloc, 32, false);

    bool ok = (a != NULL) && (b != NULL) && (c != NULL) && (b != c);

    AllocatorFree(alloc, b, 32);
    AllocatorFree(alloc, c, 32);
    HeapAllocatorDeinit(&heap);
    return ok;
}

int main(void) {
    TestFunction tests[] = {
        // Happy path
        test_basic_alloc_free,
        test_zeroed_alloc,
        test_zero_byte_alloc_returns_null,
        test_free_null_is_noop,

        // State machine
        test_alloc_returns_distinct_pointers,
        test_free_then_alloc_recycles,
        test_fill_class_grows_new_page,
        test_alloc_across_every_sub_bin,

        // Large + alignment
        test_large_alloc_passthrough,
        test_overaligned_alloc,

        // Realloc
        test_realloc_same_bin_keeps_pointer,
        test_realloc_cross_bin_copies,

        // Independence
        test_independent_heaps,

        // Rejection edges (deliberately emit LOG_ERROR)
        test_reject_foreign_pointer,
        test_reject_double_free,
        test_reject_wrong_size_hint_routes_to_wrong_class,
        test_reject_wrong_size_hint_in_class,
        test_reject_misaligned_pointer,
        test_reject_mid_xl_pointer,
        test_reject_double_free_does_not_double_vend,
    };
    return run_test_suite(tests, (int)(sizeof(tests) / sizeof(tests[0])), NULL, 0, "Allocator.Heap");
}
