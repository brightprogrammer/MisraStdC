/// file      : Tests/Std/Allocator.Budget.c
/// Smoke tests for the caller-buffer fixed-budget allocator.

#include <stdint.h>

#include <Misra/Std/Allocator.h>
#include <Misra/Std/Allocator/Budget.h>
#include <Misra/Std/Io.h>
#include <Misra/Std/Log.h>
#include <Misra/Std/Memory.h>

#include "../Util/TestRunner.h"

typedef struct {
    int  id;
    char tag[24];
} Node;

static bool test_basic_alloc_and_free(void) {
    u8              buf[1024] = {0};
    BudgetAllocator bp        = BudgetAllocatorInit(buf, sizeof(buf), sizeof(Node));
    Allocator      *alloc     = ALLOCATOR_OF(&bp);
    Node           *a         = (Node *)AllocatorAlloc(alloc, sizeof(Node), true);
    Node           *b         = (Node *)AllocatorAlloc(alloc, sizeof(Node), true);
    bool            ok        = (a != NULL) && (b != NULL) && (a != b);

    if (ok) {
        a->id = 1;
        b->id = 2;
        ok    = (a->id == 1) && (b->id == 2);
        AllocatorFree(alloc, a, sizeof(Node));
        AllocatorFree(alloc, b, sizeof(Node));
    }

    BudgetAllocatorDeinit(&bp);
    return ok;
}

static bool test_fails_when_empty(void) {
    // 256 bytes / padded(sizeof(Node)) = a few slots, exhaust them.
    u8              buf[256] = {0};
    BudgetAllocator bp       = BudgetAllocatorInit(buf, sizeof(buf), sizeof(Node));
    Allocator      *alloc    = ALLOCATOR_OF(&bp);

    bool ok = bp.slot_count > 0;
    for (size i = 0; ok && i < bp.slot_count; i++) {
        if (!AllocatorAlloc(alloc, sizeof(Node), false)) {
            ok = false;
        }
    }
    // One more must fail - no growth path.
    void *overflow = AllocatorAlloc(alloc, sizeof(Node), false);
    ok             = ok && (overflow == NULL);

    BudgetAllocatorDeinit(&bp);
    return ok;
}

static bool test_free_then_alloc_recycles(void) {
    u8              buf[1024] = {0};
    BudgetAllocator bp        = BudgetAllocatorInit(buf, sizeof(buf), sizeof(Node));
    Allocator      *alloc     = ALLOCATOR_OF(&bp);

    Node *a  = (Node *)AllocatorAlloc(alloc, sizeof(Node), true);
    bool  ok = (a != NULL);
    AllocatorFree(alloc, a, sizeof(Node));
    Node *b = (Node *)AllocatorAlloc(alloc, sizeof(Node), true);
    ok      = ok && (b == a); // LIFO free list returns the same slot.

    AllocatorFree(alloc, b, sizeof(Node));
    BudgetAllocatorDeinit(&bp);
    return ok;
}

static bool test_oversized_request_fails(void) {
    u8              buf[256] = {0};
    BudgetAllocator bp       = BudgetAllocatorInit(buf, sizeof(buf), sizeof(int));
    Allocator      *alloc    = ALLOCATOR_OF(&bp);
    void           *big      = AllocatorAlloc(alloc, 4096, true);
    bool            ok       = (big == NULL);

    BudgetAllocatorDeinit(&bp);
    return ok;
}

static bool test_alignment_honored(void) {
    static u8 buf[1024];
    MemSet(buf, 0, sizeof(buf));
    BudgetAllocator bp    = BudgetAllocatorInitAligned(buf, sizeof(buf), sizeof(int), 64);
    Allocator      *alloc = ALLOCATOR_OF(&bp);
    int            *p1    = (int *)AllocatorAlloc(alloc, sizeof(int), true);
    int            *p2    = (int *)AllocatorAlloc(alloc, sizeof(int), true);

    bool ok = (p1 != NULL) && (p2 != NULL);
    ok      = ok && (((uintptr_t)p1 & 63u) == 0);
    ok      = ok && (((uintptr_t)p2 & 63u) == 0);

    if (p1) {
        AllocatorFree(alloc, p1, sizeof(int));
    }
    if (p2) {
        AllocatorFree(alloc, p2, sizeof(int));
    }
    BudgetAllocatorDeinit(&bp);
    return ok;
}

static bool test_tiny_buffer_rejects(void) {
    // Buffer smaller than one padded slot must yield zero slots and
    // alloc must therefore return NULL on the very first call.
    u8              buf[4] = {0};
    BudgetAllocator bp     = BudgetAllocatorInit(buf, sizeof(buf), sizeof(Node));
    bool            ok     = (bp.slot_count == 0);

    BudgetAllocatorDeinit(&bp);
    return ok;
}

int main(void) {
    TestFunction tests[] = {
        test_basic_alloc_and_free,
        test_fails_when_empty,
        test_free_then_alloc_recycles,
        test_oversized_request_fails,
        test_alignment_honored,
        test_tiny_buffer_rejects,
    };
    return run_test_suite(tests, (int)(sizeof(tests) / sizeof(tests[0])), NULL, 0, "Allocator.Budget");
}
