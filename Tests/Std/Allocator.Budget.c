/// file      : tests/std/allocator.budget.c
///
/// State-machine + edge-case tests for BudgetAllocator.
///
/// The caller-buffer is partitioned by Init into [bitmap | pad | slots].
/// Alloc/Free operate only on the slot region; the bitmap is allocator-
/// owned metadata.
///
/// Slot state machine:
///     FREE  -- Alloc -->  IN_USE  -- Free -->  FREE
///     pre: bit==0          pre: bit==1
///     post: bit:=1         post: bit:=0
///
/// Rejection edges (Free must reject without writing through ptr):
///   - foreign pointer    (outside slot region)
///   - misaligned pointer (not at slot boundary)
///   - double-free        (bit already 0)

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

// =============================================================================
// Happy path

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
        AllocatorFree(alloc, a);
        AllocatorFree(alloc, b);
    }

    BudgetAllocatorDeinit(&bp);
    return ok;
}

static bool test_zero_byte_alloc_returns_null(void) {
    u8              buf[256] = {0};
    BudgetAllocator bp       = BudgetAllocatorInit(buf, sizeof(buf), sizeof(Node));
    Allocator      *alloc    = ALLOCATOR_OF(&bp);
    void           *p        = AllocatorAlloc(alloc, 0, false);
    BudgetAllocatorDeinit(&bp);
    return p == NULL;
}

static bool test_free_null_is_noop(void) {
    u8              buf[256] = {0};
    BudgetAllocator bp       = BudgetAllocatorInit(buf, sizeof(buf), sizeof(Node));
    Allocator      *alloc    = ALLOCATOR_OF(&bp);
    AllocatorFree(alloc, NULL);
    void *p  = AllocatorAlloc(alloc, sizeof(Node), false);
    bool  ok = (p != NULL);
    if (p)
        AllocatorFree(alloc, p);
    BudgetAllocatorDeinit(&bp);
    return ok;
}

// =============================================================================
// State machine: capacity + reuse

static bool test_fails_when_empty(void) {
    u8              buf[256] = {0};
    BudgetAllocator bp       = BudgetAllocatorInit(buf, sizeof(buf), sizeof(Node));
    Allocator      *alloc    = ALLOCATOR_OF(&bp);

    bool ok = BudgetAllocatorSlotCount(&bp) > 0;
    for (size i = 0; ok && i < BudgetAllocatorSlotCount(&bp); i++) {
        if (!AllocatorAlloc(alloc, sizeof(Node), false))
            ok = false;
    }
    // One more must fail -- no growth path.
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
    AllocatorFree(alloc, a);
    Node *b = (Node *)AllocatorAlloc(alloc, sizeof(Node), true);
    // ctz finds the lowest clear bit, which is the one we just freed.
    ok = ok && (b == a);

    AllocatorFree(alloc, b);
    BudgetAllocatorDeinit(&bp);
    return ok;
}

static bool test_alloc_distinct_pointers(void) {
    u8              buf[1024] = {0};
    BudgetAllocator bp        = BudgetAllocatorInit(buf, sizeof(buf), sizeof(Node));
    Allocator      *alloc     = ALLOCATOR_OF(&bp);

    Node *a  = (Node *)AllocatorAlloc(alloc, sizeof(Node), false);
    Node *b  = (Node *)AllocatorAlloc(alloc, sizeof(Node), false);
    Node *c  = (Node *)AllocatorAlloc(alloc, sizeof(Node), false);
    bool  ok = (a && b && c && a != b && b != c && a != c);

    if (a)
        AllocatorFree(alloc, a);
    if (b)
        AllocatorFree(alloc, b);
    if (c)
        AllocatorFree(alloc, c);
    BudgetAllocatorDeinit(&bp);
    return ok;
}

// =============================================================================
// Init edge cases

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
    ok      = ok && (((u64)p1 & 63u) == 0);
    ok      = ok && (((u64)p2 & 63u) == 0);

    if (p1)
        AllocatorFree(alloc, p1);
    if (p2)
        AllocatorFree(alloc, p2);
    BudgetAllocatorDeinit(&bp);
    return ok;
}

static bool test_tiny_buffer_rejects(void) {
    // Tiny buffer -> BudgetAllocatorInit fires LOG_FATAL at the caller's
    // line. A passing deadend is one where the abort fired.
    u8              buf[4] = {0};
    BudgetAllocator bp     = BudgetAllocatorInit(buf, sizeof(buf), sizeof(Node));
    BudgetAllocatorDeinit(&bp);
    return false;
}

// =============================================================================
// Rejection edges -- deadend tests. Each triggers a bad free that
// LOG_FATALs. A passing deadend is one where the abort fired.

static bool test_reject_foreign_pointer(void) {
    u8              buf1[1024] = {0};
    u8              buf2[1024] = {0};
    BudgetAllocator bp1        = BudgetAllocatorInit(buf1, sizeof(buf1), sizeof(Node));
    BudgetAllocator bp2        = BudgetAllocatorInit(buf2, sizeof(buf2), sizeof(Node));
    Allocator      *alloc1     = ALLOCATOR_OF(&bp1);
    Allocator      *alloc2     = ALLOCATOR_OF(&bp2);

    Node *p = (Node *)AllocatorAlloc(alloc1, sizeof(Node), false);
    AllocatorFree(alloc2, p); // foreign to bp2 -> LOG_FATAL
    return false;
}

static bool test_reject_pointer_before_slot_region(void) {
    // Pointer to bitmap region (front of buf) must be rejected as foreign.
    u8              buf[1024] = {0};
    BudgetAllocator bp        = BudgetAllocatorInit(buf, sizeof(buf), sizeof(Node));
    Allocator      *alloc     = ALLOCATOR_OF(&bp);

    // intentional bypass: the bitmap pointer is private allocator
    // metadata with no public accessor (and should not get one --
    // exposing it would invite misuse); the test needs a non-NULL
    // pointer in the bitmap region to exercise the foreign-ptr check.
    AllocatorFree(alloc, bp.bitmap); // bitmap region -> LOG_FATAL
    return false;
}

static bool test_reject_misaligned_pointer(void) {
    u8              buf[1024] = {0};
    BudgetAllocator bp        = BudgetAllocatorInit(buf, sizeof(buf), sizeof(Node));
    Allocator      *alloc     = ALLOCATOR_OF(&bp);
    char           *p         = (char *)AllocatorAlloc(alloc, sizeof(Node), false);
    AllocatorFree(alloc, p + 1); // mis-aligned -> LOG_FATAL
    return false;
}

static bool test_reject_double_free(void) {
    u8              buf[1024] = {0};
    BudgetAllocator bp        = BudgetAllocatorInit(buf, sizeof(buf), sizeof(Node));
    Allocator      *alloc     = ALLOCATOR_OF(&bp);
    Node           *p         = (Node *)AllocatorAlloc(alloc, sizeof(Node), false);
    AllocatorFree(alloc, p);
    AllocatorFree(alloc, p); // bit already 0 -> LOG_FATAL
    return false;
}

int main(void) {
    TestFunction normal[] = {
        // Happy path
        test_basic_alloc_and_free,
        test_zero_byte_alloc_returns_null,
        test_free_null_is_noop,
        // State machine
        test_fails_when_empty,
        test_free_then_alloc_recycles,
        test_alloc_distinct_pointers,
        // Init edges
        test_oversized_request_fails,
        test_alignment_honored,
    };
    TestFunction deadend[] = {
        test_tiny_buffer_rejects,
        test_reject_foreign_pointer,
        test_reject_pointer_before_slot_region,
        test_reject_misaligned_pointer,
        test_reject_double_free,
    };
    return run_test_suite(
        normal,
        (int)(sizeof(normal) / sizeof(normal[0])),
        deadend,
        (int)(sizeof(deadend) / sizeof(deadend[0])),
        "Allocator.Budget"
    );
}
