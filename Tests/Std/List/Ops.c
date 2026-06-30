#include <Misra/Std/Allocator/Default.h>
#include <Misra/Std/Container/List.h>
#include <Misra/Std/Log.h>
#include <Misra/Types.h>

#include "../../Util/TestRunner.h"

static int g_copy_init_count   = 0;
static int g_copy_deinit_count = 0;

static i32 compare_ints(const void *lhs, const void *rhs) {
    int a = *(const int *)lhs;
    int b = *(const int *)rhs;
    return (a > b) - (a < b);
}

static bool tracked_copy_init(void *dst, const void *src, const Allocator *alloc) {
    (void)alloc;
    g_copy_init_count += 1;
    *(int *)dst        = *(int *)src;
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

static bool test_list_clear_and_reuse(void) {
    WriteFmt("Testing ListClear and reuse\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    typedef List(int) IntList;
    IntList list = ListInit(&alloc);

    ListClear(&list);
    ListPushBackR(&list, 1);
    ListPushBackR(&list, 2);
    ListPushBackR(&list, 3);
    ListClear(&list);

    bool result = (ListLen(&list) == 0) && (ListHead(&list) == NULL) && (ListTail(&list) == NULL);

    ListPushBackR(&list, 9);
    result = result && list_matches(GENERIC_LIST(&list), (const int[]) {9}, 1);

    ListDeinit(&list);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

static bool test_list_sort_and_reverse(void) {
    WriteFmt("Testing ListSort and ListReverse\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    typedef List(int) IntList;
    IntList list = ListInit(&alloc);

    ListPushBackR(&list, 4);
    ListPushBackR(&list, 1);
    ListPushBackR(&list, 3);
    ListPushBackR(&list, 2);
    ListPushBackR(&list, 2);

    ListSort(&list, compare_ints);
    bool result = list_matches(GENERIC_LIST(&list), (const int[]) {1, 2, 2, 3, 4}, 5);

    ListReverse(&list);
    result = result && list_matches(GENERIC_LIST(&list), (const int[]) {4, 3, 2, 2, 1}, 5);

    ListDeinit(&list);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

static bool test_list_sort_and_reverse_edge_cases(void) {
    WriteFmt("Testing ListSort and ListReverse edge cases\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    typedef List(int) IntList;
    IntList empty     = ListInit(&alloc);
    IntList singleton = ListInit(&alloc);

    ListSort(&empty, compare_ints);
    ListReverse(&empty);
    ListPushBackR(&singleton, 42);
    ListSort(&singleton, compare_ints);
    ListReverse(&singleton);

    bool result = (ListLen(&empty) == 0) && (ListHead(&empty) == NULL) && (ListTail(&empty) == NULL);
    result      = result && list_matches(GENERIC_LIST(&singleton), (const int[]) {42}, 1);

    ListDeinit(&empty);
    ListDeinit(&singleton);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

static bool test_list_clear_with_deep_copy(void) {
    WriteFmt("Testing ListClear with deep copy\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    typedef List(int) IntList;
    IntList list = ListInitWithDeepCopy(tracked_copy_init, tracked_copy_deinit, &alloc);

    reset_counters();
    ListPushBackR(&list, 7);
    ListPushBackR(&list, 8);

    bool result = (g_copy_init_count == 2);

    ListClear(&list);
    result = result && (g_copy_deinit_count == 2);
    result = result && (ListLen(&list) == 0) && (ListHead(&list) == NULL) && (ListTail(&list) == NULL);

    ListPushBackR(&list, 9);
    result = result && (g_copy_init_count == 3);
    result = result && list_matches(GENERIC_LIST(&list), (const int[]) {9}, 1);

    ListDeinit(&list);
    result = result && (g_copy_deinit_count == 3);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// list_sort short-circuits only when length < 2. cxx_lt_to_le at 184 turns this
// into length <= 2, so a 2-element list returns "sorted" without ever ordering
// its elements. We sort a reverse-ordered length-2 list and assert it is
// actually ascending afterward.
static bool test_sort_two_element_list(void) {
    WriteFmt("Testing ListSort orders a two-element list\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    typedef List(int) IntList;
    IntList list = ListInit(&alloc);
    ListPushBackR(&list, 2);
    ListPushBackR(&list, 1);

    bool ok     = ListSort(&list, compare_ints);
    bool result = ok && (ListLen(&list) == 2);
    result      = result && (ListAt(&list, 0) == 1);
    result      = result && (ListAt(&list, 1) == 2);

    ListDeinit(&list);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// merge_list rollback contract: when appending l2 into l1 fails partway, the
// nodes already linked from l2 must be removed so l1 returns to its pre-merge
// length and contents, and the call returns false.
//
// Kills, on dst {1,2,3} merging src {7,8,9} failing on the 2nd src node:
//   316:16 cxx_assign_const  old_length=42 -> guard 4>42 false -> no rollback
//                            -> stale partial node, ListLen==4 not 3.
//   320:31 cxx_gt_to_le      length>old swapped to < -> rollback skipped.
//   321:17 cxx_remove_void_call rollback call deleted -> stale node.
//   321:85 cxx_sub_to_add    count = length+old (=7) -> remove_range out of
//                            bounds -> LOG_FATAL aborts the run.
static bool test_merge_failed_rolls_back(void) {
    WriteFmt("Testing merge rollback restores pre-merge length\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    typedef List(int) IntList;
    IntList dst = ListInitWithDeepCopy(failing_copy_init, plain_copy_deinit, &alloc);
    IntList src = ListInit(&alloc);

    s_copy_calls   = 0;
    s_copy_fail_at = UINT64_MAX;
    ListPushBackR(&dst, 1);
    ListPushBackR(&dst, 2);
    ListPushBackR(&dst, 3);

    // src is a plain list: its data is read verbatim by merge_list; only
    // dst's failing_copy_init runs during the merge.
    ListPushBackR(&src, 7);
    ListPushBackR(&src, 8);
    ListPushBackR(&src, 9);

    // Arm: the second src element merged into dst fails to copy.
    s_copy_fail_at = s_copy_calls + 1;

    bool returned = ListMergeR(&dst, &src);

    bool result = (returned == false);
    result      = result && list_holds(GENERIC_LIST(&dst), (const int[]) {1, 2, 3}, 3);
    // R-form merge never empties the source.
    result = result && (ListLen(&src) == 3);

    s_copy_fail_at = UINT64_MAX;
    ListDeinit(&dst);
    ListDeinit(&src);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

int main(void) {
    TestFunction tests[] = {
        test_list_clear_and_reuse,
        test_list_sort_and_reverse,
        test_list_sort_and_reverse_edge_cases,
        test_list_clear_with_deep_copy,
        test_sort_two_element_list,
        test_merge_failed_rolls_back,
    };

    WriteFmt("[INFO] Starting List.Ops tests\n\n");
    return run_test_suite(tests, (int)(sizeof(tests) / sizeof(tests[0])), NULL, 0, "List.Ops");
}
