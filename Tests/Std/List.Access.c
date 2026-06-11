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

// Build [0, 10, 20, ... ] so value == index * 10 -> data identifies the node.
static void fill_decades(GenericList *list, u64 count) {
    for (u64 i = 0; i < count; i++) {
        int v = (int)(i * 10);
        ListPushBackR((List(int) *)list, v);
    }
}

static int node_value(GenericListNode *node) {
    return *(int *)node->data;
}

static i32 compare_ints(const void *lhs, const void *rhs) {
    int a = *(const int *)lhs;
    int b = *(const int *)rhs;
    return (a > b) - (a < b);
}

static bool list_matches(GenericList *list, const int *expected, size count) {
    if (ListLen(list) != count) {
        return false;
    }

    for (size i = 0; i < count; i++) {
        int *value_ptr = item_ptr_at_list(list, sizeof(int), i);
        if (!value_ptr || (*value_ptr != expected[i])) {
            return false;
        }
    }

    return true;
}

static bool test_list_len_empty(void) {
    WriteFmt("Testing ListLen and ListEmpty\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    typedef List(int) IntList;
    IntList list = ListInit(&alloc);

    bool result = (ListLen(&list) == 0);
    result      = result && ListEmpty(&list);

    ListPushBackR(&list, 10);
    ListPushBackR(&list, 20);

    result = result && (ListLen(&list) == 2);
    result = result && !ListEmpty(&list);

    ListClear(&list);
    result = result && (ListLen(&list) == 0);
    result = result && ListEmpty(&list);
    result = result && (ListHead(&list) == NULL) && (ListTail(&list) == NULL);

    ListPushBackR(&list, 30);
    result = result && list_matches(GENERIC_LIST(&list), (const int[]) {30}, 1);

    ListDeinit(&list);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

static bool test_list_value_access_and_swap(void) {
    WriteFmt("Testing List accessors and swap\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    typedef List(int) IntList;
    IntList list = ListInit(&alloc);

    ListPushBackR(&list, 10);
    ListPushBackR(&list, 20);
    ListPushBackR(&list, 30);
    ListPushBackR(&list, 40);

    bool result = ListPtrAt(&list, 0) && (*ListPtrAt(&list, 0) == 10);
    result      = result && ListPtrAt(&list, 3) && (*ListPtrAt(&list, 3) == 40);
    result      = result && (ListAt(&list, 1) == 20);
    result      = result && (ListAt(&list, 2) == 30);
    result      = result && (ListFirst(&list) == 10);
    result      = result && (ListLast(&list) == 40);

    ListSwapItems(&list, 1, 3);
    result = result && list_matches(GENERIC_LIST(&list), (const int[]) {10, 40, 30, 20}, 4);
    ListSwapItems(&list, 2, 2);
    result = result && list_matches(GENERIC_LIST(&list), (const int[]) {10, 40, 30, 20}, 4);

    ListDeinit(&list);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

static bool test_list_node_access_and_navigation(void) {
    WriteFmt("Testing List node access and navigation\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    typedef List(int) IntList;
    IntList list = ListInit(&alloc);

    ListPushBackR(&list, 10);
    ListPushBackR(&list, 20);
    ListPushBackR(&list, 30);
    ListPushBackR(&list, 40);

    GenericListNode *node1   = GENERIC_LIST_NODE(ListNodePtrAt(&list, 1));
    GenericListNode *begin   = GENERIC_LIST_NODE(ListNodeBegin(&list));
    GenericListNode *end     = GENERIC_LIST_NODE(ListNodeEnd(&list));
    GenericListNode *same    = ListNodeRelative(ListNodeBegin(&list), 0);
    GenericListNode *rel_f2  = ListNodeRelative(ListNodeBegin(&list), 2);
    GenericListNode *rel_b2  = ListNodeRelative(ListNodeEnd(&list), -2);
    ListNode(int) *null_node = NULL;

    bool result = node1 && ListNodeData(node1) && (*(int *)ListNodeData(node1) == 20);
    result      = result && begin && ListNodeData(begin) && (*(int *)ListNodeData(begin) == 10);
    result      = result && end && ListNodeData(end) && (*(int *)ListNodeData(end) == 40);
    result      = result && ListNodeData(&ListNodeAt(&list, 2)) && (*ListNodeData(&ListNodeAt(&list, 2)) == 30);
    result      = result && ListNodeData(&ListNodeFirst(&list)) && (*ListNodeData(&ListNodeFirst(&list)) == 10);
    result      = result && ListNodeData(&ListNodeLast(&list)) && (*ListNodeData(&ListNodeLast(&list)) == 40);
    result      = result && ListNodeNext(ListNodeBegin(&list)) && ListNodeData(ListNodeNext(ListNodeBegin(&list))) &&
             (*ListNodeData(ListNodeNext(ListNodeBegin(&list))) == 20);
    result = result && ListNodePrev(ListNodeEnd(&list)) && ListNodeData(ListNodePrev(ListNodeEnd(&list))) &&
             (*ListNodeData(ListNodePrev(ListNodeEnd(&list))) == 30);
    result = result && (same == begin);
    result = result && rel_f2 && ListNodeData(rel_f2) && (*(int *)ListNodeData(rel_f2) == 30);
    result = result && rel_b2 && ListNodeData(rel_b2) && (*(int *)ListNodeData(rel_b2) == 20);
    result = result && (ListNodeRelative(ListNodeBegin(&list), -1) == NULL);
    result = result && (ListNodeRelative(ListNodeEnd(&list), 1) == NULL);
    result = result && (ListNodeNext(null_node) == NULL);
    result = result && (ListNodePrev(null_node) == NULL);

    ListDeinit(&list);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

static bool test_list_find_contains(void) {
    WriteFmt("Testing ListFind and ListContains\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    typedef List(int) IntList;
    IntList list = ListInit(&alloc);

    int  needle  = 20;
    int  missing = 99;
    bool result  = (ListFind(&list, &needle, compare_ints) == SIZE_MAX);
    result       = result && !ListContains(&list, &needle, compare_ints);

    ListPushBackR(&list, 10);
    ListPushBackR(&list, 20);
    ListPushBackR(&list, 30);
    ListPushBackR(&list, 20);

    result = result && (ListFind(&list, &needle, compare_ints) == 1);
    result = result && ListContains(&list, &needle, compare_ints);
    result = result && !ListContains(&list, &missing, compare_ints);
    result = result && (ListFind(&list, &missing, compare_ints) == SIZE_MAX);

    ListDeinit(&list);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Contract guard for get_node_random_access (List.c).
//
// This private helper is reached only through the indexed foreach macros
// when the loop body reassigns `idx` to perform random access from an
// established cursor node. It selects the closest of {cursor, head, tail}
// as the walk origin and must return the node sitting at the requested
// absolute target index regardless of which origin it picks.
//
// `abs_target_idx = nidx + ridx` is the absolute target index. The
// head/tail walk origins step to it, and it also drives origin selection.
// We arrange a forward jump from a far-from-head cursor so that the HEAD
// origin is the one selected, with the cursor index (90) large enough that
// even an abs_target_idx corruption keeps the head selected (rather than
// silently falling back to the node origin, which would walk the correct
// distance anyway). The body must then observe the element actually living
// at the requested index.
static bool test_random_access_head_origin_value(void) {
    WriteFmt("Testing random-access foreach head-origin target value contract\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    typedef List(int) IntList;
    IntList list = ListInit(&alloc);

    // 200 elements: value at index i is exactly i.
    for (int i = 0; i < 200; i++) {
        ListPushBackR(&list, i);
    }

    bool jumped       = false;
    int  jumped_value = -1;
    int  jumped_idx   = -1;

    // Walk forward to index 90 (cursor established there via node-origin
    // steps), then jump forward to index 95. Cursor at 90, target 95:
    // dist_from_node = 5, so real code uses the node origin and lands on
    // index 95. A corruption of abs_target_idx shifts origin selection to
    // the head and walks abs_target_idx steps, landing on the wrong node
    // even though the macro still reports idx == 95.
    ListForeachIdx(&list, value, idx) {
        if (idx == 90 && !jumped) {
            jumped = true;
            idx    = 95;
        } else if (jumped && idx == 95 && jumped_idx < 0) {
            jumped_idx   = (int)idx;
            jumped_value = value;
        }
    }

    // Real code: value at index 95 is 95.
    bool result = (jumped_idx == 95) && (jumped_value == 95);

    ListDeinit(&list);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// nidx=4, ridx=+1 -> target idx 5 (value 50).
// dist_from_node=1, dist_from_head=5, dist_from_tail=3 -> node-origin branch,
// forward step loop runs once. Kills:
//   492 init_const (steps=42 -> walks off end -> NULL)
//   493 gt_to_ge   (overshoots by one -> idx 6, value 60)
//   495 dec_to_inc (steps never reaches 0 -> walks off end -> NULL)
static bool test_random_access_node_origin_forward(void) {
    WriteFmt("Testing get_node_random_access node-origin forward step\n");

    DefaultAllocator alloc = DefaultAllocatorInit();
    List(int) list         = ListInit(&alloc);
    fill_decades(GENERIC_LIST(&list), 9);

    GenericListNode *origin = node_at_list(GENERIC_LIST(&list), sizeof(int), 4);
    GenericListNode *got    = get_node_random_access(GENERIC_LIST(&list), origin, 4, 1);

    bool result = got && (node_value(got) == 50);

    ListDeinit(&list);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// nidx=4, ridx=-1 -> target idx 3 (value 30).
// dist_from_node=1, dist_from_head=3, dist_from_tail=5 -> node-origin branch,
// backward step loop runs once. Kills:
//   499 inc_to_dec (steps never reaches 0 -> walks off head -> NULL)
//   492 init_const (steps=42 -> enters forward loop -> walks off end -> NULL)
static bool test_random_access_node_origin_backward(void) {
    WriteFmt("Testing get_node_random_access node-origin backward step\n");

    DefaultAllocator alloc = DefaultAllocatorInit();
    List(int) list         = ListInit(&alloc);
    fill_decades(GENERIC_LIST(&list), 9);

    GenericListNode *origin = node_at_list(GENERIC_LIST(&list), sizeof(int), 4);
    GenericListNode *got    = get_node_random_access(GENERIC_LIST(&list), origin, 4, -1);

    bool result = got && (node_value(got) == 30);

    ListDeinit(&list);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// A multi-step forward jump from the node origin confirms the loop count is
// exact (not just off-by-one). nidx=3, ridx=+2 -> target idx 5 (value 50).
// dist_from_node=2, dist_from_head=5, dist_from_tail=3 -> node-origin branch.
static bool test_random_access_node_origin_forward_multistep(void) {
    WriteFmt("Testing get_node_random_access node-origin multi-step forward\n");

    DefaultAllocator alloc = DefaultAllocatorInit();
    List(int) list         = ListInit(&alloc);
    fill_decades(GENERIC_LIST(&list), 9);

    GenericListNode *origin = node_at_list(GENERIC_LIST(&list), sizeof(int), 3);
    GenericListNode *got    = get_node_random_access(GENERIC_LIST(&list), origin, 3, 2);

    bool result = got && (node_value(got) == 50);

    ListDeinit(&list);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// get_node_random_access(node@7, nidx=7, ridx=-4) targets index 3. With
// dist_from_node=4 > dist_from_head=3 and dist_from_head <= dist_from_tail, the
// helper takes the head-walk branch. Real code walks 0->1->2->3 and returns the
// node at index 3 (value 30). A bad start index, a flipped `<` bound, or a
// decrementing step makes it stop at the wrong node or run off to NULL.
static bool test_random_access_head_walk_returns_target(void) {
    WriteFmt("Testing get_node_random_access head-walk origin\n");

    List(int) list = ListInit(get_test_alloc());
    FILL_EIGHT(&list);

    GenericList     *g    = GENERIC_LIST(&list);
    GenericListNode *base = node_at_list(g, sizeof(int), 7);
    GenericListNode *got  = get_node_random_access(g, base, 7, -4);

    bool result = got && got->data && (*(int *)got->data == 30);

    ListDeinit(&list);
    return result;
}

// get_node_random_access(node@0, nidx=0, ridx=+4) targets index 4. With
// dist_from_head=4 > dist_from_tail=3, the helper takes the tail-walk branch.
// Real code walks 7->6->5->4 and returns the node at index 4 (value 40). A bad
// start index, a flipped `>` bound, or an incrementing step makes it stop at the
// wrong node or run off to NULL.
static bool test_random_access_tail_walk_returns_target(void) {
    WriteFmt("Testing get_node_random_access tail-walk origin\n");

    List(int) list = ListInit(get_test_alloc());
    FILL_EIGHT(&list);

    GenericList     *g    = GENERIC_LIST(&list);
    GenericListNode *base = node_at_list(g, sizeof(int), 0);
    GenericListNode *got  = get_node_random_access(g, base, 0, 4);

    bool result = got && got->data && (*(int *)got->data == 40);

    ListDeinit(&list);
    return result;
}

// Drive the iteration helper's no-cursor head/tail origin selection across the
// full index range. Whichever origin the helper picks, it must return the node
// whose data equals idx*10, so this also guards against the walk returning a
// neighbouring node.
static bool test_iteration_no_cursor_resolves_every_index(void) {
    WriteFmt("Testing get_node_for_list_iteration no-cursor resolution\n");

    List(int) list = ListInit(get_test_alloc());
    FILL_EIGHT(&list);

    GenericList *g      = GENERIC_LIST(&list);
    bool         result = true;
    for (u64 i = 0; i < 8; i++) {
        GenericListNode *got = get_node_for_list_iteration(g, NULL, 0, i);
        result               = result && got && got->data && (*(int *)got->data == (int)(i * 10));
    }

    ListDeinit(&list);
    return result;
}

// get_node_for_list_iteration with a NULL cursor must resolve target_idx by
// walking from the head when dist_from_head <= dist_from_tail. For a length-5
// list, target_idx 2 selects the head-walk branch (2 <= 5-1-2). The walk must
// land on exactly index 2.
//   - cxx_init_const at 534 (i = 42) skips the loop body and returns the head.
//   - cxx_post_inc_to_post_dec at 534 (i--) underflows after one step and
//     returns index 1.
// Both yield a wrong node; we assert the resolved node carries index-2's data.
static bool test_iteration_head_walk_lands_on_target(void) {
    WriteFmt("Testing iteration head-walk resolves exact target index\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    typedef List(int) IntList;
    IntList list = ListInit(&alloc);
    ListPushBackR(&list, 10);
    ListPushBackR(&list, 20);
    ListPushBackR(&list, 30);
    ListPushBackR(&list, 40);
    ListPushBackR(&list, 50);

    GenericListNode *node   = get_node_for_list_iteration(GENERIC_LIST(&list), NULL, 0, 2);
    bool             result = node && node->data && (*(int *)node->data == 30);

    ListDeinit(&list);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// When dist_from_head > dist_from_tail the helper walks backward from the tail.
// For a length-5 list, target_idx 3 selects the tail-walk branch (3 > 5-1-3).
// The loop counts down with i-- from length-1 to target_idx.
//   - cxx_post_dec_to_post_inc at 540 (i++) keeps the loop condition true and
//     walks off the front of the list, returning NULL.
// We assert the resolved node is non-NULL and carries index-3's data.
static bool test_iteration_tail_walk_lands_on_target(void) {
    WriteFmt("Testing iteration tail-walk resolves exact target index\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    typedef List(int) IntList;
    IntList list = ListInit(&alloc);
    ListPushBackR(&list, 10);
    ListPushBackR(&list, 20);
    ListPushBackR(&list, 30);
    ListPushBackR(&list, 40);
    ListPushBackR(&list, 50);

    GenericListNode *node   = get_node_for_list_iteration(GENERIC_LIST(&list), NULL, 0, 3);
    bool             result = node && node->data && (*(int *)node->data == 40);

    ListDeinit(&list);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Mutant 459:33 cxx_lt_to_le in get_node_relative_to_list_node:
//   if (!node->prev && ridx < 0) return NULL;   ->   ridx <= 0
// When a backward relative walk lands EXACTLY on the head (no prev,
// ridx decremented to 0), real code falls through and returns the head
// node. The mutant treats ridx==0 at the head as "ran past the end" and
// returns NULL. Walking from the tail back by (len-1) must yield the
// head; assert we get a real node carrying the head's value.
static bool test_relative_back_to_head_returns_head(void) {
    WriteFmt("Testing ListNodeRelative back-walk landing on head\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    typedef List(int) IntList;
    IntList list = ListInit(&alloc);

    ListPushBackR(&list, 10);
    ListPushBackR(&list, 20);
    ListPushBackR(&list, 30);
    ListPushBackR(&list, 40);

    // Tail is index 3 (value 40); -(len-1) = -3 lands exactly on head.
    GenericListNode *head_via_back = ListNodeRelative(ListNodeEnd(&list), -3);

    bool result = head_via_back != NULL;
    result      = result && ListNodeData(head_via_back) && (*(int *)ListNodeData(head_via_back) == 10);

    // Cross-check: the same node reached forward from head index 0.
    result = result && (head_via_back == GENERIC_LIST_NODE(ListNodeBegin(&list)));

    ListDeinit(&list);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

int main(void) {
    TestFunction tests[] = {
        test_list_len_empty,
        test_list_value_access_and_swap,
        test_list_node_access_and_navigation,
        test_list_find_contains,
        test_random_access_head_origin_value,
        test_random_access_node_origin_forward,
        test_random_access_node_origin_backward,
        test_random_access_node_origin_forward_multistep,
        test_random_access_head_walk_returns_target,
        test_random_access_tail_walk_returns_target,
        test_iteration_no_cursor_resolves_every_index,
        test_iteration_head_walk_lands_on_target,
        test_iteration_tail_walk_lands_on_target,
        test_relative_back_to_head_returns_head,
    };

    WriteFmt("[INFO] Starting List.Access tests\n\n");
    return run_test_suite(tests, (int)(sizeof(tests) / sizeof(tests[0])), NULL, 0, "List.Access");
}
