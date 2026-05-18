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
        AllocatorFree(alloc_base, a);
        AllocatorFree(alloc_base, b);
    }

    SlabAllocatorDeinit(&slab);
    return ok;
}

static bool test_free_then_alloc_recycles(void) {
    SlabAllocator slab       = SlabAllocatorInit(sizeof(Node));
    Allocator    *alloc_base = ALLOCATOR_OF(&slab);
    Node         *a          = (Node *)AllocatorAlloc(alloc_base, sizeof(Node), true);
    bool          ok         = (a != NULL);

    AllocatorFree(alloc_base, a);
    Node *b = (Node *)AllocatorAlloc(alloc_base, sizeof(Node), true);
    ok      = ok && (b == a); // The free list returned the same slot.

    AllocatorFree(alloc_base, b);
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
            AllocatorFree(alloc_base, slots[i]);
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
        AllocatorFree(alloc_base, slots[i]);
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
        AllocatorFree(alloc_base, p);
    }
    SlabAllocatorDeinit(&slab);
    return ok;
}

// =============================================================================
// Rejection edges -- deadend tests. A passing deadend is one where
// the bad free aborts via LOG_FATAL.

static bool test_reject_foreign_pointer(void) {
    SlabAllocator s1 = SlabAllocatorInit(sizeof(Node));
    SlabAllocator s2 = SlabAllocatorInit(sizeof(Node));
    Allocator    *a1 = ALLOCATOR_OF(&s1);
    Allocator    *a2 = ALLOCATOR_OF(&s2);
    Node         *p  = (Node *)AllocatorAlloc(a1, sizeof(Node), false);
    AllocatorFree(a2, p); // foreign to s2 -> LOG_FATAL
    return false;
}

static bool test_reject_misaligned_pointer(void) {
    SlabAllocator slab  = SlabAllocatorInit(sizeof(Node));
    Allocator    *alloc = ALLOCATOR_OF(&slab);
    char         *p     = (char *)AllocatorAlloc(alloc, sizeof(Node), false);
    AllocatorFree(alloc, p + 1); // mis-aligned -> LOG_FATAL
    return false;
}

static bool test_reject_double_free(void) {
    SlabAllocator slab  = SlabAllocatorInit(sizeof(Node));
    Allocator    *alloc = ALLOCATOR_OF(&slab);
    Node         *p     = (Node *)AllocatorAlloc(alloc, sizeof(Node), false);
    AllocatorFree(alloc, p);
    AllocatorFree(alloc, p); // bit already 0 -> LOG_FATAL
    return false;
}

int main(void) {
    TestFunction normal[] = {
        test_basic_alloc_and_free,
        test_free_then_alloc_recycles,
        test_grow_across_chunks,
        test_oversized_request_fails,
        test_free_half_then_realloc,
        test_pool_alignment,
    };
    TestFunction deadend[] = {
        test_reject_foreign_pointer,
        test_reject_misaligned_pointer,
        test_reject_double_free,
    };
    return run_test_suite(
        normal,
        (int)(sizeof(normal) / sizeof(normal[0])),
        deadend,
        (int)(sizeof(deadend) / sizeof(deadend[0])),
        "Allocator.Slab"
    );
}
