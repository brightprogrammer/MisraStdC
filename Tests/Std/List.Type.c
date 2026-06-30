#include <Misra/Std/Allocator/Default.h>
#include <Misra/Std/Container/List.h>
#include <Misra/Std/Container/List/Private.h>
#include <Misra/Std/Log.h>

#include "../Util/TestRunner.h"

// Build [0, 10, 20, ... ] so value == index * 10 -> data identifies the node.
static void fill_decades(GenericList *list, u64 count) {
    for (u64 i = 0; i < count; i++) {
        int v = (int)(i * 10);
        ListPushBackR((List(int) *)list, v);
    }
}

static bool test_list_type_defaults(void) {
    WriteFmt("Testing List type defaults\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    typedef List(int) IntList;
    IntList list = ListInit(&alloc);

    ValidateList(&list);

    // intentional bypass: __magic is the private sentinel ValidateList
    // checks; no public accessor exposes it, so the layout test reads it
    // directly to confirm ListInit planted the right value.
    bool result = (ListHead(&list) == NULL) && (ListTail(&list) == NULL) && (ListCopyInit(&list) == NULL) &&
                  (ListCopyDeinit(&list) == NULL) && (ListLen(&list) == 0) && MAGIC_MATCHES(list.__magic, LIST_MAGIC);

    ListDeinit(&list);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

static bool test_list_node_type_layout(void) {
    WriteFmt("Testing ListNode type layout\n");

    int value          = 42;
    ListNode(int) node = {0};
    // intentional bypass: building a node literal on the stack and reading its
    // fields directly to verify the ListNode(T) layout. The list-managed
    // accessors are designed for nodes owned by a List; this fixture has no list.
    node.data = &value;

    return (node.next == NULL) && (node.prev == NULL) && (node.data == &value) && (*node.data == 42);
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

int main(void) {
    TestFunction tests[] = {
        test_list_type_defaults,
        test_list_node_type_layout,
        test_validate_memoization_skips_structural,
    };

    WriteFmt("[INFO] Starting List.Type tests\n\n");
    return run_test_suite(tests, (int)(sizeof(tests) / sizeof(tests[0])), NULL, 0, "List.Type");
}
