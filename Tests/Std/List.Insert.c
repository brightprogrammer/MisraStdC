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
    if (list->length != count) {
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
    result      = result && (src.copy_init == tracked_copy_init) && (src.copy_deinit == tracked_copy_deinit);

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

int main(void) {
    TestFunction tests[] = {
        test_list_insert_and_push_aliases,
        test_list_push_arr_l_zeroes_all_items,
        test_list_push_arr_zero_count_is_noop,
        test_list_insert_with_deep_copy,
        test_list_merge_l_preserves_source_hooks_for_reuse,
        test_list_merge_variants,
        test_list_merge_edge_cases,
    };

    WriteFmt("[INFO] Starting List.Insert tests\n\n");
    return run_test_suite(tests, (int)(sizeof(tests) / sizeof(tests[0])), NULL, 0, "List.Insert");
}
