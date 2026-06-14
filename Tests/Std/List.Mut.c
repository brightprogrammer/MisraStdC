/// file : tests/std/list.mut.c
/// Targeted mutation-kill tests for List: each test drives an input that makes a
/// specific surviving mutant produce an observably-wrong result. Distinct from the
/// existing List.* tests -- do NOT duplicate.
#include <Misra.h>
#include <Misra/Std/Allocator/Default.h>
#include <Misra/Std/Container/List.h>
#include <Misra/Std/Container/List/Private.h>
#include <Misra/Std/Log.h>
#include <Misra/Types.h>

#include "../Util/TestRunner.h"

static DefaultAllocator *get_test_alloc(void) {
    static DefaultAllocator s_alloc;
    static bool             s_initialized = false;
    if (!s_initialized) {
        s_alloc       = DefaultAllocatorInit();
        s_initialized = true;
    }
    return &s_alloc;
}

// Build 0,10,20,...,70 (8 nodes, indices 0..7).
#define FILL_EIGHT(list_ptr)                                                                                           \
    do {                                                                                                               \
        for (int fill_i = 0; fill_i < 8; fill_i++) {                                                                   \
            ListPushBackR((list_ptr), fill_i * 10);                                                                    \
        }                                                                                                              \
    } while (0)

// Deadend for get_node_random_access upper relative-bounds guard (List.c:476):
//   (ridx > 0 && nidx + (u64)ridx >= list->length)  ->  ge_to_gt makes it `>`.
// With node@nidx=4 and ridx=+4 the absolute target is index 8 on a length-8
// list -- one past the end. Real code's `>=` fires LOG_FATAL ("Relative node
// index outside of list bounds"). The `>` mutant lets target==length through,
// then computes dist_from_tail = length-1-8 (underflow) and walks off the end
// instead of aborting. We assert the call aborts.
static bool test_random_access_relative_target_at_length_fails(void) {
    WriteFmt("Testing get_node_random_access relative target == length aborts\n");

    List(int) list = ListInit(get_test_alloc());
    FILL_EIGHT(&list);

    GenericList     *g    = GENERIC_LIST(&list);
    GenericListNode *base = node_at_list(g, sizeof(int), 4);
    (void)get_node_random_access(g, base, 4, 4);

    return false;
}

int main(void) {
    TestFunction deadend_tests[] = {
        test_random_access_relative_target_at_length_fails,
    };

    WriteFmt("[INFO] Starting List.Mut tests\n\n");
    return run_test_suite(NULL, 0, deadend_tests, (int)(sizeof(deadend_tests) / sizeof(deadend_tests[0])), "List.Mut");
}
