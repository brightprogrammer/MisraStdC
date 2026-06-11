#include <Misra/Std/Allocator/Default.h>
#include <Misra/Std/Container/List.h>
#include <Misra/Std/Log.h>
#include <Misra/Types.h>

#include "../Util/TestRunner.h"

static int g_copy_init_count   = 0;
static int g_copy_deinit_count = 0;

static bool tracked_copy_init(void *dst, const void *src, const Allocator *alloc) {
    (void)alloc;
    g_copy_init_count += 1;
    *(int *)dst        = *(int *)src + 1000;
    return true;
}

static void tracked_copy_deinit(void *data, const Allocator *alloc) {
    (void)alloc;
    g_copy_deinit_count += 1;
    *(int *)data         = 0;
}

static void reset_counters(void) {
    g_copy_init_count   = 0;
    g_copy_deinit_count = 0;
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

// A controllable deep-copy hook: every call is counted, and the call whose
// index equals `s_copy_fail_at` returns false (simulating a copy that fails
// partway through a multi-element insert). This drives `insert_into_list`
// into its failure path so the rollback logic in `push_arr_list` /
// `merge_list` is actually exercised -- the DefaultAllocator never fails an
// allocation, so a failing copy hook is the only way to reach those branches.
static u64 s_copy_calls   = 0;
static u64 s_copy_fail_at = UINT64_MAX;

static bool failing_copy_init(void *dst, const void *src, const Allocator *alloc) {
    (void)alloc;
    u64 n = s_copy_calls++;
    if (n == s_copy_fail_at) {
        return false;
    }
    *(int *)dst = *(const int *)src;
    return true;
}

static void plain_copy_deinit(void *data, const Allocator *alloc) {
    (void)alloc;
    *(int *)data = 0;
}

static bool list_holds(GenericList *list, const int *expected, size count) {
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

static bool test_list_insert_and_push_aliases(void) {
    WriteFmt("Testing List insert and push aliases\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    typedef List(int) IntList;
    IntList list = ListInit(&alloc);
    int     a    = 10;
    int     b    = 20;
    int     c    = 30;
    int     d    = 40;
    int     e    = 50;
    int     f    = 60;
    int     g    = 70;
    int     h    = 80;
    int     i    = 90;

    ListInsertL(&list, a, 0);
    ListInsertR(&list, b, 1);
    ListInsert(&list, c, 1);
    ListPushFrontL(&list, d);
    ListPushFrontR(&list, h);
    ListPushBackR(&list, e);
    ListPushFront(&list, f);
    ListPushBackL(&list, i);
    ListPushBack(&list, g);

    bool result =
        (a == 0) && (b == 20) && (c == 0) && (d == 0) && (e == 50) && (f == 0) && (g == 0) && (h == 80) && (i == 0);
    result = result && list_matches(GENERIC_LIST(&list), (const int[]) {60, 80, 40, 10, 30, 20, 50, 90, 70}, 9);

    ListDeinit(&list);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

static bool test_list_push_arr_l_zeroes_all_items(void) {
    WriteFmt("Testing ListPushArrL zeroes all transferred items\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    typedef List(int) IntList;
    IntList list  = ListInit(&alloc);
    int     arr[] = {1, 2, 3};

    ListPushArrL(&list, arr, 3);

    bool result = list_matches(GENERIC_LIST(&list), (const int[]) {1, 2, 3}, 3);
    result      = result && (arr[0] == 0) && (arr[1] == 0) && (arr[2] == 0);

    ListDeinit(&list);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

static bool test_list_push_arr_zero_count_is_noop(void) {
    WriteFmt("Testing ListPushArrL zero-count contract\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    typedef List(int) IntList;
    IntList list  = ListInit(&alloc);
    int     arr[] = {4, 5, 6};

    ListPushBackR(&list, 1);
    ListPushArrL(&list, arr, 0);

    bool result = list_matches(GENERIC_LIST(&list), (const int[]) {1}, 1);
    result      = result && (arr[0] == 4) && (arr[1] == 5) && (arr[2] == 6);

    ListDeinit(&list);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

static bool test_list_insert_with_deep_copy(void) {
    WriteFmt("Testing List insert with deep copy\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    typedef List(int) IntList;
    IntList list  = ListInitWithDeepCopy(tracked_copy_init, tracked_copy_deinit, &alloc);
    int     x     = 7;
    int     arr[] = {8, 9};

    reset_counters();
    ListPushBackR(&list, x);
    ListInsertL(&list, x, 1);
    ListPushArrL(&list, arr, 2);

    bool result = (g_copy_init_count == 4);
    result      = result && (x == 7);
    result      = result && (arr[0] == 8) && (arr[1] == 9);
    result      = result && list_matches(GENERIC_LIST(&list), (const int[]) {1007, 1007, 1008, 1009}, 4);

    ListDeinit(&list);
    result = result && (g_copy_deinit_count == 4);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

static bool test_list_merge_l_preserves_source_hooks_for_reuse(void) {
    WriteFmt("Testing ListMergeL preserves source hooks for reuse\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    typedef List(int) IntList;
    IntList dest = ListInit(&alloc);
    IntList src  = ListInitWithDeepCopy(tracked_copy_init, tracked_copy_deinit, &alloc);

    reset_counters();
    ListPushBackR(&src, 3);
    ListPushBackR(&src, 4);
    ListMergeL(&dest, &src);

    bool result = (g_copy_init_count == 2);
    result      = result && list_matches(GENERIC_LIST(&dest), (const int[]) {1003, 1004}, 2);
    result      = result && (ListLen(&src) == 0) && (ListHead(&src) == NULL) && (ListTail(&src) == NULL);
    result      = result && (ListCopyInit(&src) == tracked_copy_init) && (ListCopyDeinit(&src) == tracked_copy_deinit);

    ListPushBackR(&src, 5);
    result = result && (g_copy_init_count == 3);
    result = result && list_matches(GENERIC_LIST(&src), (const int[]) {1005}, 1);

    ListDeinit(&dest);
    ListDeinit(&src);
    result = result && (g_copy_deinit_count == 1);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

static bool test_list_merge_variants(void) {
    WriteFmt("Testing List merge variants\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    typedef List(int) IntList;
    IntList dest_l = ListInit(&alloc);
    IntList src_l  = ListInit(&alloc);
    IntList dest_r = ListInit(&alloc);
    IntList src_r  = ListInit(&alloc);
    IntList dest_a = ListInit(&alloc);
    IntList src_a  = ListInit(&alloc);

    ListPushBackR(&dest_l, 1);
    ListPushBackR(&dest_l, 2);
    ListPushBackR(&src_l, 3);
    ListPushBackR(&src_l, 4);
    ListMergeL(&dest_l, &src_l);

    ListPushBackR(&dest_r, 1);
    ListPushBackR(&dest_r, 2);
    ListPushBackR(&src_r, 3);
    ListPushBackR(&src_r, 4);
    ListMergeR(&dest_r, &src_r);

    ListPushBackR(&dest_a, 5);
    ListPushBackR(&src_a, 6);
    ListMerge(&dest_a, &src_a);

    bool result = list_matches(GENERIC_LIST(&dest_l), (const int[]) {1, 2, 3, 4}, 4);
    result      = result && (ListLen(&src_l) == 0) && (ListHead(&src_l) == NULL) && (ListTail(&src_l) == NULL);
    result      = result && list_matches(GENERIC_LIST(&dest_r), (const int[]) {1, 2, 3, 4}, 4);
    result      = result && list_matches(GENERIC_LIST(&src_r), (const int[]) {3, 4}, 2);
    result      = result && list_matches(GENERIC_LIST(&dest_a), (const int[]) {5, 6}, 2);
    result      = result && (ListLen(&src_a) == 0) && (ListHead(&src_a) == NULL) && (ListTail(&src_a) == NULL);

    ListDeinit(&dest_l);
    ListDeinit(&src_l);
    ListDeinit(&dest_r);
    ListDeinit(&src_r);
    ListDeinit(&dest_a);
    ListDeinit(&src_a);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

static bool test_list_merge_edge_cases(void) {
    WriteFmt("Testing List merge edge cases\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    typedef List(int) IntList;
    IntList deep_dest   = ListInitWithDeepCopy(tracked_copy_init, tracked_copy_deinit, &alloc);
    IntList shallow_src = ListInit(&alloc);
    IntList empty_dest  = ListInit(&alloc);
    IntList empty_src   = ListInit(&alloc);

    reset_counters();
    ListPushBackR(&shallow_src, 11);
    ListPushBackR(&shallow_src, 12);
    ListMergeL(&deep_dest, &shallow_src);
    ListMergeL(&empty_dest, &empty_src);

    bool result = (g_copy_init_count == 2);
    result      = result && (g_copy_deinit_count == 0);
    result      = result && list_matches(GENERIC_LIST(&deep_dest), (const int[]) {1011, 1012}, 2);
    result      = result && list_matches(GENERIC_LIST(&shallow_src), (const int[]) {11, 12}, 2);
    result      = result && (ListLen(&empty_dest) == 0) && (ListLen(&empty_src) == 0);

    ListDeinit(&deep_dest);
    ListDeinit(&shallow_src);
    ListDeinit(&empty_dest);
    ListDeinit(&empty_src);
    result = result && (g_copy_deinit_count == 2);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// list_insert_range_r (the body of ListPushArrR) must actually append the
// source range. cxx_replace_scalar_call at 586 replaces the push_arr_list call
// with the constant 42 (truthy) AND skips the call, so the return value still
// reads as success while no nodes are inserted. We assert both the length grew
// and the values landed.
static bool test_push_arr_r_actually_inserts(void) {
    WriteFmt("Testing ListPushArrR actually appends the source range\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    typedef List(int) IntList;
    IntList list  = ListInit(&alloc);
    int     arr[] = {7, 8, 9};

    bool ok     = ListPushArrR(&list, arr, 3);
    bool result = ok && (ListLen(&list) == 3);
    result      = result && (ListAt(&list, 0) == 7);
    result      = result && (ListAt(&list, 1) == 8);
    result      = result && (ListAt(&list, 2) == 9);

    ListDeinit(&list);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// push_arr_list rollback contract: when a multi-element push fails partway
// through, every node inserted so far must be removed so the list returns to
// its exact pre-call length and contents, and the call returns false.
//
// Kills, on a list pre-loaded with {1,2,3} pushing {7,8,9} that fails on the
// 2nd array element (one node already linked):
//   281:16 cxx_assign_const  old_length=42 -> guard 4>42 false -> no rollback
//                            -> stale partial node left, ListLen==4 not 3.
//   287:30 cxx_gt_to_le      length>old swapped to < -> rollback skipped ->
//                            stale partial node, ListLen==4 not 3.
//   288:17 cxx_remove_void_call rollback call deleted -> stale node, len 4.
//   288:83 cxx_sub_to_add    count = length+old (=7) start=3 -> remove_range
//                            start+count>length -> LOG_FATAL aborts the run.
static bool test_push_arr_failed_rolls_back(void) {
    WriteFmt("Testing push_arr rollback restores pre-call length\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    typedef List(int) IntList;
    IntList list = ListInitWithDeepCopy(failing_copy_init, plain_copy_deinit, &alloc);

    s_copy_calls   = 0;
    s_copy_fail_at = UINT64_MAX;
    ListPushBackR(&list, 1);
    ListPushBackR(&list, 2);
    ListPushBackR(&list, 3);

    // Arm: the second element of the upcoming array push fails to copy.
    s_copy_fail_at = s_copy_calls + 1;

    int  src[3]   = {7, 8, 9};
    bool returned = ListPushArrR(&list, src, 3);

    bool result = (returned == false);
    result      = result && list_holds(GENERIC_LIST(&list), (const int[]) {1, 2, 3}, 3);

    s_copy_fail_at = UINT64_MAX;
    ListDeinit(&list);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

int main(void) {
    TestFunction tests[] = {
        test_list_insert_and_push_aliases,
        test_list_push_arr_l_zeroes_all_items,
        test_list_push_arr_zero_count_is_noop,
        test_list_insert_with_deep_copy,
        test_list_merge_l_preserves_source_hooks_for_reuse,
        test_list_merge_variants,
        test_list_merge_edge_cases,
        test_push_arr_r_actually_inserts,
        test_push_arr_failed_rolls_back,
    };

    WriteFmt("[INFO] Starting List.Insert tests\n\n");
    return run_test_suite(tests, (int)(sizeof(tests) / sizeof(tests[0])), NULL, 0, "List.Insert");
}
