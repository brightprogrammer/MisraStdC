/// file      : Tests/Std/Allocator.Heap.c
/// Smoke tests for the per-descriptor binned heap allocator.

#include <stdint.h>

#include <Misra/Std/Allocator.h>
#include <Misra/Std/Allocator/Heap.h>
#include <Misra/Std/Io.h>
#include <Misra/Std/Log.h>
#include <Misra/Std/Memory.h>

#include "../Util/TestRunner.h"

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
        if (p[i] != 0) {
            ok = false;
        }
    }

    if (p) {
        AllocatorFree(alloc, p, 64);
    }
    HeapAllocatorDeinit(&heap);
    return ok;
}

static bool test_free_then_alloc_recycles(void) {
    HeapAllocator heap  = HeapAllocatorInit();
    Allocator    *alloc = ALLOCATOR_OF(&heap);

    void *a = AllocatorAlloc(alloc, 32, false);
    AllocatorFree(alloc, a, 32);
    void *b  = AllocatorAlloc(alloc, 32, false);
    bool  ok = (a != NULL) && (b == a); // same size class freelist returns it

    if (b) {
        AllocatorFree(alloc, b, 32);
    }
    HeapAllocatorDeinit(&heap);
    return ok;
}

static bool test_large_alloc_passthrough(void) {
    // > 2 KiB hits the page-allocator passthrough path inside Heap.
    HeapAllocator heap  = HeapAllocatorInit();
    Allocator    *alloc = ALLOCATOR_OF(&heap);

    size n  = 64 * 1024;
    u8  *p  = (u8 *)AllocatorAlloc(alloc, n, true);
    bool ok = (p != NULL);
    if (ok) {
        p[0]     = 0xAB;
        p[n - 1] = 0xCD;
        ok       = (p[0] == 0xAB) && (p[n - 1] == 0xCD);
        AllocatorFree(alloc, p, n);
    }

    HeapAllocatorDeinit(&heap);
    return ok;
}

static bool test_realloc_same_bin_keeps_pointer(void) {
    HeapAllocator heap  = HeapAllocatorInit();
    Allocator    *alloc = ALLOCATOR_OF(&heap);

    // Both 24 and 28 fall in the 32-byte bin, so realloc must keep the
    // pointer (no copy, no growth).
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
        // Different bin, so heap copies into a fresh slot.
        ok = (grown != NULL) && (grown[0] == 'h') && (grown[15] == '!');
        if (grown) {
            AllocatorFree(alloc, grown, 200);
        }
    }

    HeapAllocatorDeinit(&heap);
    return ok;
}

static bool test_overaligned_alloc(void) {
    HeapAllocator heap  = HeapAllocatorInitAligned(64);
    Allocator    *alloc = ALLOCATOR_OF(&heap);

    // Alignment > 16 bypasses the bin path and goes via the embedded
    // PageAllocator, which is naturally page-aligned. Stronger
    // alignment requests are honored on the page-acquired pointer.
    void *p  = AllocatorAlloc(alloc, 256, true);
    bool  ok = (p != NULL) && (((uintptr_t)p & 63u) == 0);

    if (p) {
        AllocatorFree(alloc, p, 256);
    }
    HeapAllocatorDeinit(&heap);
    return ok;
}

static bool test_independent_heaps(void) {
    // Two HeapAllocators on the same stack frame must not share any
    // state — the library never owns global heap bookkeeping.
    HeapAllocator h1     = HeapAllocatorInit();
    HeapAllocator h2     = HeapAllocatorInit();
    Allocator    *alloc1 = ALLOCATOR_OF(&h1);
    Allocator    *alloc2 = ALLOCATOR_OF(&h2);

    void *a  = AllocatorAlloc(alloc1, 32, true);
    void *b  = AllocatorAlloc(alloc2, 32, true);
    bool  ok = (a != NULL) && (b != NULL) && (a != b);
    // Two independent heaps must own separate class-S descriptor arrays.
    ok = ok && (h1.s != h2.s) && h1.s_len > 0 && h2.s_len > 0;

    AllocatorFree(alloc1, a, 32);
    AllocatorFree(alloc2, b, 32);
    HeapAllocatorDeinit(&h1);
    HeapAllocatorDeinit(&h2);
    return ok;
}

int main(void) {
    TestFunction tests[] = {
        test_basic_alloc_free,
        test_zeroed_alloc,
        test_free_then_alloc_recycles,
        test_large_alloc_passthrough,
        test_realloc_same_bin_keeps_pointer,
        test_realloc_cross_bin_copies,
        test_overaligned_alloc,
        test_independent_heaps,
    };
    return run_test_suite(tests, (int)(sizeof(tests) / sizeof(tests[0])), NULL, 0, "Allocator.Heap");
}
