/// file      : Tests/Std/Allocator.Slab.c
///
/// State-machine + edge-case tests for SlabAllocator.
///
/// Each chunk is page-backed and laid out as
///   [ header | bitmap | pad | slot 0 | ... | slot N-1 ]
/// Header + bitmap are allocator-owned metadata. Slots are user data.
///
/// Slot state machine:
///     FREE -- Alloc --> IN_USE -- Free --> FREE
///     pre: bit==0       pre: bit==1
///     post: bit:=1      post: bit:=0
///
/// Rejection edges (Free must reject without writing through ptr):
///   - foreign pointer    (outside every chunk's slot region)
///   - misaligned pointer (not at slot boundary inside its chunk)
///   - double-free        (bit already 0)

#include <Misra/Std/Allocator.h>
#include <Misra/Std/Allocator/Slab.h>
#include <Misra/Std/Io.h>
#include <Misra/Std/Log.h>
#include <Misra/Std/Memory.h>

#include "../Util/TestRunner.h"

typedef struct {
    int  id;
    char tag[24];
} Node;

static bool test_basic_alloc_and_free(void) {
    SlabAllocator slab       = SlabAllocatorInit(sizeof(Node));
    Allocator    *alloc_base = ALLOCATOR_OF(&slab);
    Node         *a          = (Node *)AllocatorAlloc(alloc_base, sizeof(Node), true);
    Node         *b          = (Node *)AllocatorAlloc(alloc_base, sizeof(Node), true);
    bool          ok         = (a != NULL) && (b != NULL) && (a != b);

    if (ok) {
        a->id = 1;
        b->id = 2;
        ok    = (a->id == 1) && (b->id == 2);
        AllocatorFree(alloc_base, a, sizeof(Node));
        AllocatorFree(alloc_base, b, sizeof(Node));
    }

    SlabAllocatorDeinit(&slab);
    return ok;
}

static bool test_free_then_alloc_recycles(void) {
    SlabAllocator slab       = SlabAllocatorInit(sizeof(Node));
    Allocator    *alloc_base = ALLOCATOR_OF(&slab);
    Node         *a          = (Node *)AllocatorAlloc(alloc_base, sizeof(Node), true);
    bool          ok         = (a != NULL);

    AllocatorFree(alloc_base, a, sizeof(Node));
    Node *b = (Node *)AllocatorAlloc(alloc_base, sizeof(Node), true);
    ok      = ok && (b == a); // The free list returned the same slot.

    AllocatorFree(alloc_base, b, sizeof(Node));
    SlabAllocatorDeinit(&slab);
    return ok;
}

static bool test_grow_across_chunks(void) {
    SlabAllocator slab       = SlabAllocatorInit(sizeof(Node));
    Allocator    *alloc_base = ALLOCATOR_OF(&slab);
    Node         *slots[600];
    bool          ok = true;

    for (size i = 0; i < 600; i++) {
        slots[i] = (Node *)AllocatorAlloc(alloc_base, sizeof(Node), true);
        if (!slots[i]) {
            ok = false;
            break;
        }
        slots[i]->id = (int)i;
    }

    for (size i = 0; ok && i < 600; i++) {
        if (slots[i]->id != (int)i) {
            ok = false;
            break;
        }
    }

    for (size i = 0; i < 600; i++) {
        if (slots[i]) {
            AllocatorFree(alloc_base, slots[i], sizeof(Node));
        }
    }

    SlabAllocatorDeinit(&slab);
    return ok;
}

static bool test_oversized_request_fails(void) {
    SlabAllocator slab       = SlabAllocatorInit(sizeof(int));
    Allocator    *alloc_base = ALLOCATOR_OF(&slab);
    void         *big        = AllocatorAlloc(alloc_base, 4096, true);
    bool          ok         = (big == NULL);

    SlabAllocatorDeinit(&slab);
    return ok;
}

static bool test_free_half_then_realloc(void) {
    SlabAllocator slab       = SlabAllocatorInit(sizeof(Node));
    Allocator    *alloc_base = ALLOCATOR_OF(&slab);
    Node         *slots[200];
    bool          ok = true;

    for (size i = 0; i < 200; i++) {
        slots[i] = (Node *)AllocatorAlloc(alloc_base, sizeof(Node), true);
        if (!slots[i]) {
            ok = false;
            break;
        }
        slots[i]->id = (int)i;
    }

    // Free every other slot, then re-allocate 100 more to make the slab
    // walk both the free list (recycling) and the slab on growth.
    for (size i = 0; ok && i < 200; i += 2) {
        AllocatorFree(alloc_base, slots[i], sizeof(Node));
        slots[i] = NULL;
    }

    Node *fresh[100];
    for (size i = 0; ok && i < 100; i++) {
        fresh[i] = (Node *)AllocatorAlloc(alloc_base, sizeof(Node), true);
        if (!fresh[i]) {
            ok = false;
            break;
        }
    }

    SlabAllocatorDeinit(&slab);
    return ok;
}

static bool test_pool_alignment(void) {
    SlabAllocator slab       = SlabAllocatorInitAligned(sizeof(int), 64);
    Allocator    *alloc_base = ALLOCATOR_OF(&slab);
    int          *p          = (int *)AllocatorAlloc(alloc_base, sizeof(int), true);
    bool          ok         = (p != NULL) && (((u64)p & 63u) == 0);

    if (p) {
        AllocatorFree(alloc_base, p, sizeof(int));
    }
    SlabAllocatorDeinit(&slab);
    return ok;
}

// =============================================================================
// Rejection edges

static bool test_reject_foreign_pointer(void) {
    SlabAllocator s1 = SlabAllocatorInit(sizeof(Node));
    SlabAllocator s2 = SlabAllocatorInit(sizeof(Node));
    Allocator    *a1 = ALLOCATOR_OF(&s1);
    Allocator    *a2 = ALLOCATOR_OF(&s2);

    Node *p = (Node *)AllocatorAlloc(a1, sizeof(Node), false);
    AllocatorFree(a2, p, sizeof(Node)); // foreign to s2 -> REJECT

    Node *q  = (Node *)AllocatorAlloc(a2, sizeof(Node), false);
    bool  ok = (q != NULL);
    if (q)
        AllocatorFree(a2, q, sizeof(Node));
    AllocatorFree(a1, p, sizeof(Node));

    SlabAllocatorDeinit(&s1);
    SlabAllocatorDeinit(&s2);
    return ok;
}

static bool test_reject_misaligned_pointer(void) {
    SlabAllocator slab  = SlabAllocatorInit(sizeof(Node));
    Allocator    *alloc = ALLOCATOR_OF(&slab);

    char *p = (char *)AllocatorAlloc(alloc, sizeof(Node), false);
    AllocatorFree(alloc, p + 1, sizeof(Node)); // misaligned -> REJECT

    Node *q  = (Node *)AllocatorAlloc(alloc, sizeof(Node), false);
    bool  ok = (q != NULL) && ((void *)q != (void *)p);
    if (q)
        AllocatorFree(alloc, q, sizeof(Node));
    AllocatorFree(alloc, p, sizeof(Node));
    SlabAllocatorDeinit(&slab);
    return ok;
}

static bool test_reject_double_free(void) {
    SlabAllocator slab  = SlabAllocatorInit(sizeof(Node));
    Allocator    *alloc = ALLOCATOR_OF(&slab);

    Node *p = (Node *)AllocatorAlloc(alloc, sizeof(Node), false);
    AllocatorFree(alloc, p, sizeof(Node));
    AllocatorFree(alloc, p, sizeof(Node)); // bit already 0 -> REJECT

    Node *q  = (Node *)AllocatorAlloc(alloc, sizeof(Node), false);
    bool  ok = (q != NULL) && (q == p);    // bit was correctly cleared once

    if (q)
        AllocatorFree(alloc, q, sizeof(Node));
    SlabAllocatorDeinit(&slab);
    return ok;
}

static bool test_reject_double_free_no_double_vending(void) {
    SlabAllocator slab  = SlabAllocatorInit(sizeof(Node));
    Allocator    *alloc = ALLOCATOR_OF(&slab);

    Node *a = (Node *)AllocatorAlloc(alloc, sizeof(Node), false);
    AllocatorFree(alloc, a, sizeof(Node));
    AllocatorFree(alloc, a, sizeof(Node)); // rejected
    Node *b  = (Node *)AllocatorAlloc(alloc, sizeof(Node), false);
    Node *c  = (Node *)AllocatorAlloc(alloc, sizeof(Node), false);
    bool  ok = a && b && c && (b != c);

    AllocatorFree(alloc, b, sizeof(Node));
    AllocatorFree(alloc, c, sizeof(Node));
    SlabAllocatorDeinit(&slab);
    return ok;
}

int main(void) {
    TestFunction tests[] = {
        test_basic_alloc_and_free,
        test_free_then_alloc_recycles,
        test_grow_across_chunks,
        test_oversized_request_fails,
        test_free_half_then_realloc,
        test_pool_alignment,
        // Rejection edges
        test_reject_foreign_pointer,
        test_reject_misaligned_pointer,
        test_reject_double_free,
        test_reject_double_free_no_double_vending,
    };
    return run_test_suite(tests, (int)(sizeof(tests) / sizeof(tests[0])), NULL, 0, "Allocator.Slab");
}
