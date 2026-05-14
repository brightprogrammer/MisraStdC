/// file      : Tests/Std/Allocator.Pool.c
/// Smoke tests for the fixed-size pool allocator.

#include <stdint.h>

#include <Misra/Std/Allocator.h>
#include <Misra/Std/Allocator/Pool.h>
#include <Misra/Std/Io.h>
#include <Misra/Std/Log.h>
#include <Misra/Std/Memory.h>

#include "../Util/TestRunner.h"

typedef struct {
    int  id;
    char tag[24];
} Node;

static bool test_basic_alloc_and_free(void) {
    PoolAllocator pool       = PoolAllocatorInit(sizeof(Node));
    Allocator    *alloc_base = ALLOCATOR_OF(&pool);
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

    PoolAllocatorDeinit(&pool);
    return ok;
}

static bool test_free_then_alloc_recycles(void) {
    PoolAllocator pool       = PoolAllocatorInit(sizeof(Node));
    Allocator    *alloc_base = ALLOCATOR_OF(&pool);
    Node         *a          = (Node *)AllocatorAlloc(alloc_base, sizeof(Node), true);
    bool          ok         = (a != NULL);

    AllocatorFree(alloc_base, a, sizeof(Node));
    Node *b = (Node *)AllocatorAlloc(alloc_base, sizeof(Node), true);
    ok      = ok && (b == a); // The free list returned the same slot.

    AllocatorFree(alloc_base, b, sizeof(Node));
    PoolAllocatorDeinit(&pool);
    return ok;
}

static bool test_grow_across_chunks(void) {
    PoolAllocator pool       = PoolAllocatorInit(sizeof(Node));
    Allocator    *alloc_base = ALLOCATOR_OF(&pool);
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

    PoolAllocatorDeinit(&pool);
    return ok;
}

static bool test_oversized_request_fails(void) {
    PoolAllocator pool       = PoolAllocatorInit(sizeof(int));
    Allocator    *alloc_base = ALLOCATOR_OF(&pool);
    void         *big        = AllocatorAlloc(alloc_base, 4096, true);
    bool          ok         = (big == NULL);

    PoolAllocatorDeinit(&pool);
    return ok;
}

static bool test_free_half_then_realloc(void) {
    PoolAllocator pool       = PoolAllocatorInit(sizeof(Node));
    Allocator    *alloc_base = ALLOCATOR_OF(&pool);
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

    // Free every other slot, then re-allocate 100 more to make the pool
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

    PoolAllocatorDeinit(&pool);
    return ok;
}

static bool test_pool_alignment(void) {
    PoolAllocator pool       = PoolAllocatorInitAligned(sizeof(int), 64);
    Allocator    *alloc_base = ALLOCATOR_OF(&pool);
    int          *p          = (int *)AllocatorAlloc(alloc_base, sizeof(int), true);
    bool          ok         = (p != NULL) && (((uintptr_t)p & 63u) == 0);

    if (p) {
        AllocatorFree(alloc_base, p, sizeof(int));
    }
    PoolAllocatorDeinit(&pool);
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
    };
    return run_test_suite(tests, (int)(sizeof(tests) / sizeof(tests[0])), NULL, 0, "Allocator.Pool");
}
