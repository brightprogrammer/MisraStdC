#include <Misra/Std/Allocator/Default.h>
#include <Misra/Std/Container/List.h>
#include <Misra/Std/Container/List/Private.h>
#include <Misra/Std/Log.h>

#include "../../Util/TestRunner.h"

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

static i32 compare_ints(const void *lhs, const void *rhs) {
    int a = *(const int *)lhs;
    int b = *(const int *)rhs;
    return (a > b) - (a < b);
}

// Build [0, 10, 20, ... ] so value == index * 10 -> data identifies the node.
static void fill_decades(GenericList *list, u64 count) {
    for (u64 i = 0; i < count; i++) {
        int v = (int)(i * 10);
        ListPushBackR((List(int) *)list, v);
    }
}

static bool test_validate_corrupt_empty_list_fails(void) {
    WriteFmt("Testing ValidateList on corrupt empty list\n");

    List(int) list = ListInit(get_test_alloc());
    // intentional bypass: ListHead is read-only, no public setter exists --
    // plant a bogus head pointer on an empty list so ValidateList trips its
    // empty-but-head-set check.
    list.head = (void *)1;
    MAGIC_MARK_DIRTY(&list);
    ValidateList(&list);

    return false;
}

static bool test_validate_null_list_fails(void) {
    WriteFmt("Testing ValidateList on NULL list\n");

    ValidateList(NULL);
    return false;
}

static bool test_validate_invalid_magic_fails(void) {
    WriteFmt("Testing ValidateList on invalid magic\n");

    List(int) list = ListInit(get_test_alloc());
    // intentional bypass: __magic is the private sentinel ValidateList
    // checks; scramble it directly to exercise the type-confusion /
    // uninitialised-handle path.
    list.__magic = 0;
    ValidateList(&list);

    return false;
}

static bool test_validate_corrupt_nonempty_list_fails(void) {
    WriteFmt("Testing ValidateList on corrupt non-empty list\n");

    GenericListNode node = {0};
    List(int) list       = ListInit(get_test_alloc());
    GenericList *g       = GENERIC_LIST(&list);

    // intentional bypass: ListHead/ListTail/ListLen are read-only; plant
    // head set but tail null with length 1 -- proves ValidateList catches
    // the head/tail consistency invariant.
    g->head   = &node;
    g->length = 1;
    g->tail   = NULL;
    MAGIC_MARK_DIRTY(&list);
    ValidateList(&list);

    return false;
}

static bool test_validate_nonempty_head_null_fails(void) {
    WriteFmt("Testing ValidateList on non-empty NULL head\n");

    int             value = 1;
    GenericListNode node  = {.next = NULL, .prev = NULL, .data = &value};
    List(int) list        = ListInit(get_test_alloc());
    GenericList *g        = GENERIC_LIST(&list);

    // intentional bypass: ListHead/ListTail/ListLen are read-only; plant
    // length>0 with NULL head -- proves the empty-length-must-match-empty-
    // list-pointer invariant fires.
    g->head   = NULL;
    g->tail   = &node;
    g->length = 1;
    MAGIC_MARK_DIRTY(&list);
    ValidateList(&list);

    return false;
}

static bool test_validate_head_prev_fails(void) {
    WriteFmt("Testing ValidateList on head prev corruption\n");

    int             value = 1;
    GenericListNode node  = {.next = NULL, .prev = (GenericListNode *)1, .data = &value};
    List(int) list        = ListInit(get_test_alloc());
    GenericList *g        = GENERIC_LIST(&list);

    // intentional bypass: ListHead/ListTail/ListLen are read-only; plant a
    // head node whose prev pointer is non-NULL -- ValidateList must reject
    // the head having a predecessor.
    g->head   = &node;
    g->tail   = &node;
    g->length = 1;
    MAGIC_MARK_DIRTY(&list);
    ValidateList(&list);

    return false;
}

static bool test_validate_tail_next_fails(void) {
    WriteFmt("Testing ValidateList on tail next corruption\n");

    int             value = 1;
    GenericListNode node  = {.next = (GenericListNode *)1, .prev = NULL, .data = &value};
    List(int) list        = ListInit(get_test_alloc());
    GenericList *g        = GENERIC_LIST(&list);

    // intentional bypass: ListHead/ListTail/ListLen are read-only; plant a
    // tail node whose next pointer is non-NULL -- ValidateList must reject
    // the tail having a successor.
    g->head   = &node;
    g->tail   = &node;
    g->length = 1;
    MAGIC_MARK_DIRTY(&list);
    ValidateList(&list);

    return false;
}

static bool test_list_ptr_at_empty_fails(void) {
    WriteFmt("Testing ListPtrAt on empty list\n");

    List(int) list = ListInit(get_test_alloc());
    ListPtrAt(&list, 0);

    return false;
}

static bool test_list_ptr_at_out_of_bounds_fails(void) {
    WriteFmt("Testing ListPtrAt out of bounds\n");

    List(int) list = ListInit(get_test_alloc());
    ListPushBackR(&list, 10);
    ListPtrAt(&list, 1);

    return false;
}

static bool test_list_at_out_of_bounds_fails(void) {
    WriteFmt("Testing ListAt out of bounds\n");

    List(int) list = ListInit(get_test_alloc());
    ListPushBackR(&list, 10);
    (void)ListAt(&list, 1);

    return false;
}

static bool test_list_node_ptr_at_empty_fails(void) {
    WriteFmt("Testing ListNodePtrAt on empty list\n");

    List(int) list = ListInit(get_test_alloc());
    ListNodePtrAt(&list, 0);

    return false;
}

static bool test_list_node_ptr_at_out_of_bounds_fails(void) {
    WriteFmt("Testing ListNodePtrAt out of bounds\n");

    List(int) list = ListInit(get_test_alloc());
    ListPushBackR(&list, 10);
    ListNodePtrAt(&list, 1);

    return false;
}

static bool test_list_node_at_out_of_bounds_fails(void) {
    WriteFmt("Testing ListNodeAt out of bounds\n");

    List(int) list = ListInit(get_test_alloc());
    ListPushBackR(&list, 10);
    (void)ListNodeAt(&list, 1);

    return false;
}

static bool test_list_first_on_empty_fails(void) {
    WriteFmt("Testing ListFirst on empty list\n");

    List(int) list = ListInit(get_test_alloc());
    ListFirst(&list);

    return false;
}

static bool test_list_last_on_empty_fails(void) {
    WriteFmt("Testing ListLast on empty list\n");

    List(int) list = ListInit(get_test_alloc());
    ListLast(&list);

    return false;
}

static bool test_list_node_at_empty_fails(void) {
    WriteFmt("Testing ListNodeAt on empty list\n");

    List(int) list = ListInit(get_test_alloc());
    (void)ListNodeAt(&list, 0);

    return false;
}

static bool test_list_insert_out_of_range_fails(void) {
    WriteFmt("Testing ListInsertR out of range\n");

    List(int) list = ListInit(get_test_alloc());
    ListInsertR(&list, 10, 1);

    return false;
}

static bool test_list_remove_out_of_range_fails(void) {
    WriteFmt("Testing ListRemove out of range\n");

    List(int) list = ListInit(get_test_alloc());
    ListPushBackR(&list, 10);
    ListRemove(&list, NULL, 1);

    return false;
}

static bool test_list_pop_front_empty_fails(void) {
    WriteFmt("Testing ListPopFront on empty list\n");

    List(int) list = ListInit(get_test_alloc());
    ListPopFront(&list, NULL);

    return false;
}

static bool test_list_pop_back_empty_fails(void) {
    WriteFmt("Testing ListPopBack on empty list\n");

    List(int) list = ListInit(get_test_alloc());
    ListPopBack(&list, NULL);

    return false;
}

static bool test_list_remove_range_out_of_range_fails(void) {
    WriteFmt("Testing ListRemoveRange out of range\n");

    List(int) list = ListInit(get_test_alloc());
    ListPushBackR(&list, 10);
    ListPushBackR(&list, 20);
    ListRemoveRange(&list, NULL, 1, 2);

    return false;
}

static bool test_list_swap_items_out_of_range_fails(void) {
    WriteFmt("Testing ListSwapItems out of range\n");

    List(int) list = ListInit(get_test_alloc());
    ListPushBackR(&list, 10);
    ListSwapItems(&list, 0, 1);

    return false;
}

static bool test_list_find_without_compare_fails(void) {
    WriteFmt("Testing ListFind without compare function\n");

    List(int) list = ListInit(get_test_alloc());
    int key        = 10;
    ListPushBackR(&list, 10);
    ListFind(&list, &key, NULL);

    return false;
}

static bool test_list_find_without_key_fails(void) {
    WriteFmt("Testing ListFind without key pointer\n");

    List(int) list = ListInit(get_test_alloc());
    ListPushBackR(&list, 10);
    ListFind(&list, NULL, compare_ints);

    return false;
}

static bool test_list_node_relative_null_fails(void) {
    WriteFmt("Testing ListNodeRelative with NULL node\n");

    ListNodeRelative(NULL, 1);
    return false;
}

static bool test_list_sort_without_compare_fails(void) {
    WriteFmt("Testing ListSort without compare function\n");

    List(int) list = ListInit(get_test_alloc());
    ListPushBackR(&list, 10);
    ListSort(&list, NULL);

    return false;
}

// Deadend: get_node_for_list_iteration must abort when target_idx == length
// (out of bounds). Real code's `target_idx >= length` check fires; a `>` mutant
// would let the out-of-bounds index through.
static bool test_iteration_target_at_length_fails(void) {
    WriteFmt("Testing get_node_for_list_iteration target == length\n");

    List(int) list = ListInit(get_test_alloc());
    FILL_EIGHT(&list);

    GenericList *g = GENERIC_LIST(&list);
    (void)get_node_for_list_iteration(g, NULL, 0, ListLen(&list));

    return false;
}

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
    TestFunction deadend_tests[] = {
        test_iteration_target_at_length_fails,
        test_random_access_relative_target_at_length_fails,
        deadend_random_access_nidx_equals_length,
        test_validate_corrupt_empty_list_fails,
        test_validate_null_list_fails,
        test_validate_invalid_magic_fails,
        test_validate_corrupt_nonempty_list_fails,
        test_validate_nonempty_head_null_fails,
        test_validate_head_prev_fails,
        test_validate_tail_next_fails,
        test_list_ptr_at_empty_fails,
        test_list_ptr_at_out_of_bounds_fails,
        test_list_at_out_of_bounds_fails,
        test_list_node_ptr_at_empty_fails,
        test_list_node_ptr_at_out_of_bounds_fails,
        test_list_node_at_out_of_bounds_fails,
        test_list_first_on_empty_fails,
        test_list_last_on_empty_fails,
        test_list_node_at_empty_fails,
        test_list_insert_out_of_range_fails,
        test_list_remove_out_of_range_fails,
        test_list_pop_front_empty_fails,
        test_list_pop_back_empty_fails,
        test_list_remove_range_out_of_range_fails,
        test_list_swap_items_out_of_range_fails,
        test_list_find_without_compare_fails,
        test_list_find_without_key_fails,
        test_list_node_relative_null_fails,
        test_list_sort_without_compare_fails,
    };

    WriteFmt("[INFO] Starting List.Deadend tests\n\n");
    return run_test_suite(
        NULL,
        0,
        deadend_tests,
        (int)(sizeof(deadend_tests) / sizeof(deadend_tests[0])),
        "List.Deadend"
    );
}
