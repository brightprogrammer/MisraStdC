/// file      : Tests/Std/Allocator.Budget.c
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
        AllocatorFree(alloc, a, sizeof(Node));
        AllocatorFree(alloc, b, sizeof(Node));
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
    AllocatorFree(alloc, NULL, sizeof(Node));
    void *p  = AllocatorAlloc(alloc, sizeof(Node), false);
    bool  ok = (p != NULL);
    if (p)
        AllocatorFree(alloc, p, sizeof(Node));
    BudgetAllocatorDeinit(&bp);
    return ok;
}

// =============================================================================
// State machine: capacity + reuse

static bool test_fails_when_empty(void) {
    u8              buf[256] = {0};
    BudgetAllocator bp       = BudgetAllocatorInit(buf, sizeof(buf), sizeof(Node));
    Allocator      *alloc    = ALLOCATOR_OF(&bp);

    bool ok = bp.slot_count > 0;
    for (size i = 0; ok && i < bp.slot_count; i++) {
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
    AllocatorFree(alloc, a, sizeof(Node));
    Node *b = (Node *)AllocatorAlloc(alloc, sizeof(Node), true);
    // ctz finds the lowest clear bit, which is the one we just freed.
    ok = ok && (b == a);

    AllocatorFree(alloc, b, sizeof(Node));
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
        AllocatorFree(alloc, a, sizeof(Node));
    if (b)
        AllocatorFree(alloc, b, sizeof(Node));
    if (c)
        AllocatorFree(alloc, c, sizeof(Node));
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
        AllocatorFree(alloc, p1, sizeof(int));
    if (p2)
        AllocatorFree(alloc, p2, sizeof(int));
    BudgetAllocatorDeinit(&bp);
    return ok;
}

static bool test_tiny_buffer_rejects(void) {
    u8              buf[4] = {0};
    BudgetAllocator bp     = BudgetAllocatorInit(buf, sizeof(buf), sizeof(Node));
    bool            ok     = (bp.slot_count == 0);
    BudgetAllocatorDeinit(&bp);
    return ok;
}

// =============================================================================
// Rejection edges

static bool test_reject_foreign_pointer(void) {
    u8              buf1[1024] = {0};
    u8              buf2[1024] = {0};
    BudgetAllocator bp1        = BudgetAllocatorInit(buf1, sizeof(buf1), sizeof(Node));
    BudgetAllocator bp2        = BudgetAllocatorInit(buf2, sizeof(buf2), sizeof(Node));
    Allocator      *alloc1     = ALLOCATOR_OF(&bp1);
    Allocator      *alloc2     = ALLOCATOR_OF(&bp2);

    Node *p = (Node *)AllocatorAlloc(alloc1, sizeof(Node), false);

    // Foreign-free attempt: hand bp1's ptr to bp2.
    AllocatorFree(alloc2, p, sizeof(Node));

    // bp2 must still work.
    Node *q  = (Node *)AllocatorAlloc(alloc2, sizeof(Node), false);
    bool  ok = (q != NULL);
    if (q)
        AllocatorFree(alloc2, q, sizeof(Node));

    AllocatorFree(alloc1, p, sizeof(Node));
    BudgetAllocatorDeinit(&bp1);
    BudgetAllocatorDeinit(&bp2);
    return ok;
}

static bool test_reject_pointer_before_slot_region(void) {
    // Pointer to bitmap region (front of buf) must be rejected as foreign.
    u8              buf[1024] = {0};
    BudgetAllocator bp        = BudgetAllocatorInit(buf, sizeof(buf), sizeof(Node));
    Allocator      *alloc     = ALLOCATOR_OF(&bp);

    // bp.bitmap points at the bitmap region (before slots). Try to free it.
    AllocatorFree(alloc, bp.bitmap, sizeof(Node));

    // Allocator must still be usable.
    Node *p  = (Node *)AllocatorAlloc(alloc, sizeof(Node), false);
    bool  ok = (p != NULL);
    if (p)
        AllocatorFree(alloc, p, sizeof(Node));
    BudgetAllocatorDeinit(&bp);
    return ok;
}

static bool test_reject_misaligned_pointer(void) {
    u8              buf[1024] = {0};
    BudgetAllocator bp        = BudgetAllocatorInit(buf, sizeof(buf), sizeof(Node));
    Allocator      *alloc     = ALLOCATOR_OF(&bp);

    char *p = (char *)AllocatorAlloc(alloc, sizeof(Node), false);

    // Mid-slot pointer in valid range, wrong alignment -> REJECT.
    AllocatorFree(alloc, p + 1, sizeof(Node));

    // Allocator + original slot unaffected.
    Node *q  = (Node *)AllocatorAlloc(alloc, sizeof(Node), false);
    bool  ok = (q != NULL) && ((void *)q != (void *)p);
    if (q)
        AllocatorFree(alloc, q, sizeof(Node));
    AllocatorFree(alloc, p, sizeof(Node));
    BudgetAllocatorDeinit(&bp);
    return ok;
}

static bool test_reject_double_free(void) {
    u8              buf[1024] = {0};
    BudgetAllocator bp        = BudgetAllocatorInit(buf, sizeof(buf), sizeof(Node));
    Allocator      *alloc     = ALLOCATOR_OF(&bp);

    Node *p = (Node *)AllocatorAlloc(alloc, sizeof(Node), false);
    AllocatorFree(alloc, p, sizeof(Node));

    // Second free of same ptr -> bit already 0 -> REJECT.
    AllocatorFree(alloc, p, sizeof(Node));

    // Allocator still usable; recycled slot is the same.
    Node *q  = (Node *)AllocatorAlloc(alloc, sizeof(Node), false);
    bool  ok = (q != NULL) && (q == p);
    if (q)
        AllocatorFree(alloc, q, sizeof(Node));
    BudgetAllocatorDeinit(&bp);
    return ok;
}

static bool test_reject_double_free_no_double_vending(void) {
    // Free,free,alloc,alloc must NOT return aliased pointers.
    u8              buf[1024] = {0};
    BudgetAllocator bp        = BudgetAllocatorInit(buf, sizeof(buf), sizeof(Node));
    Allocator      *alloc     = ALLOCATOR_OF(&bp);

    Node *a = (Node *)AllocatorAlloc(alloc, sizeof(Node), false);
    AllocatorFree(alloc, a, sizeof(Node));
    AllocatorFree(alloc, a, sizeof(Node)); // rejected
    Node *b  = (Node *)AllocatorAlloc(alloc, sizeof(Node), false);
    Node *c  = (Node *)AllocatorAlloc(alloc, sizeof(Node), false);
    bool  ok = a && b && c && (b != c);

    AllocatorFree(alloc, b, sizeof(Node));
    AllocatorFree(alloc, c, sizeof(Node));
    BudgetAllocatorDeinit(&bp);
    return ok;
}

int main(void) {
    TestFunction tests[] = {
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
        test_tiny_buffer_rejects,

        // Rejection edges
        test_reject_foreign_pointer,
        test_reject_pointer_before_slot_region,
        test_reject_misaligned_pointer,
        test_reject_double_free,
        test_reject_double_free_no_double_vending,
    };
    return run_test_suite(tests, (int)(sizeof(tests) / sizeof(tests[0])), NULL, 0, "Allocator.Budget");
}
