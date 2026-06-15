/// file      : Tests/Std/List.Blind.c
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Blind-spot mutation-hardening tests for Source/Misra/Std/Container/List.c.
/// Each normal test passes on the real implementation and fails under one
/// specific surviving mutant; each deadend test observes an abort that the
/// real code raises but a guard-loosening mutant swallows.

#include <Misra.h>
#include <Misra/Std/Allocator/Debug.h>
#include <Misra/Std/Allocator/Default.h>
#include <Misra/Std/Container/List.h>
#include <Misra/Std/Container/List/Private.h>
#include <Misra/Std/Log.h>
#include <Misra/Types.h>

#include "../Util/TestRunner.h"

static int node_value(GenericListNode *node) {
    return *(int *)node->data;
}

// Build [0, 10, 20, ...] so value == index * 10 -> data identifies the node.
static void fill_decades(GenericList *list, u64 count) {
    for (u64 i = 0; i < count; i++) {
        int v = (int)(i * 10);
        ListPushBackR((List(int) *)list, v);
    }
}

// ---------------------------------------------------------------------------
// 493:22 cxx_gt_to_ge  in get_node_random_access node-origin forward walk:
//   while (steps > 0 && cur)  ->  while (steps >= 0 && cur)
//
// A non-zero overshoot is silently corrected by the symmetric backward loop
// that follows (a forward overshoot of one is pulled back by one). The one
// case the correction CANNOT undo is when the node origin is the TAIL and
// ridx == 0: real code skips both loops and returns the tail. The mutant's
// forward loop runs once (0 >= 0), steps off the tail to NULL and sets
// steps = -1; the backward loop then sees cur == NULL and cannot step back,
// so the mutant returns NULL.
//
// nidx = length-1 (tail), ridx = 0 -> abs target = tail.
// dist_from_node=0, dist_from_tail=0 -> node origin selected.
// Real returns the tail node (value 80); mutant returns NULL.
// ---------------------------------------------------------------------------
static bool test_node_origin_tail_zero_step(void) {
    WriteFmt("Testing get_node_random_access node-origin tail zero-step\n");

    DefaultAllocator alloc = DefaultAllocatorInit();
    List(int) list         = ListInit(&alloc);
    fill_decades(GENERIC_LIST(&list), 9); // indices 0..8, tail value 80

    GenericList     *g    = GENERIC_LIST(&list);
    GenericListNode *tail = node_at_list(g, sizeof(int), 8);
    GenericListNode *got  = get_node_random_access(g, tail, 8, 0);

    // Real: tail node, value 80. Mutant (steps >= 0): NULL.
    bool result = got && (node_value(got) == 80);

    ListDeinit(&list);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// ---------------------------------------------------------------------------
// 434:22 cxx_and_to_or  in validate_list memoization gate:
//   if (!(l->__magic & MAGIC_VALIDATED_BIT)) return;
//     ->  if (!(l->__magic | MAGIC_VALIDATED_BIT)) return;
// `x | BIT` is always non-zero, so `!(...)` is always false: the mutant
// NEVER takes the early return and ALWAYS runs the structural validation.
//
// We drive the memoization to the "already validated, bit cleared" state
// (one ValidateList clears the bit), then corrupt a structural invariant
// (head->prev set non-NULL). Real code's gate sees the bit cleared and
// returns early -> no structural check -> no abort. The mutant re-runs the
// structural check, observes the bad head->prev, and LOG_FATALs.
// We restore the link before teardown so the real path stays clean.
// ---------------------------------------------------------------------------
static bool test_validate_memoization_skips_structural(void) {
    WriteFmt("Testing validate_list memoization gate (and-to-or)\n");

    DefaultAllocator alloc = DefaultAllocatorInit();
    List(int) list         = ListInit(&alloc);
    fill_decades(GENERIC_LIST(&list), 4);

    GenericList *g = GENERIC_LIST(&list);

    // First validate consumes the validated bit (real path), so subsequent
    // validates take the memoized early-return branch.
    validate_list(g);

    // Corrupt a structural invariant. With the bit cleared, the real gate
    // returns before structural validation can notice. The mutant always
    // validates and aborts here.
    GenericListNode *real_prev = g->head->prev;
    g->head->prev              = (GenericListNode *)(void *)g; // bogus non-NULL

    validate_list(g);                                          // real: early return; mutant: LOG_FATAL

    // Restore the invariant so teardown walks a clean list.
    g->head->prev = real_prev;

    bool result = true;

    ListDeinit(&list);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// ---------------------------------------------------------------------------
// 472:14 cxx_ge_to_gt  in get_node_random_access node-index bound guard:
//   if (nidx >= list->length) LOG_FATAL(...);  ->  if (nidx > list->length)
// `nidx == list->length` is one past the end and must abort. The mutant
// loosens the bound so nidx == length is wrongly accepted. Real code aborts
// (deadend: expected). The mutant does NOT abort, so the deadend test fails
// under the mutation.
// ---------------------------------------------------------------------------
static bool deadend_random_access_nidx_equals_length(void) {
    WriteFmt("Testing get_node_random_access nidx==length is rejected\n");

    DefaultAllocator alloc = DefaultAllocatorInit();
    List(int) list         = ListInit(&alloc);
    fill_decades(GENERIC_LIST(&list), 4);

    GenericList *g = GENERIC_LIST(&list);
    // nidx == length (4) with ridx 0. Real aborts at the bound guard. The
    // node pointer is the head (valid) so only the guard distinguishes.
    get_node_random_access(g, g->head, ListLen(&list), 0);

    // Unreachable on real code (LOG_FATAL longjmps out). If the mutant
    // swallows the guard we fall through here; clean up and return.
    ListDeinit(&list);
    DefaultAllocatorDeinit(&alloc);
    return true;
}

int main(void) {
    TestFunction tests[] = {
        test_node_origin_tail_zero_step,
        test_validate_memoization_skips_structural,
    };
    TestFunction deadend_tests[] = {
        deadend_random_access_nidx_equals_length,
    };

    WriteFmt("[INFO] Starting List.Blind tests\n\n");
    return run_test_suite(
        tests,
        (int)(sizeof(tests) / sizeof(tests[0])),
        deadend_tests,
        (int)(sizeof(deadend_tests) / sizeof(deadend_tests[0])),
        "List.Blind"
    );
}
