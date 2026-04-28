#include <Misra/Std/Container/List.h>
#include <Misra/Std/Log.h>
#include <Misra/Types.h>

#include "../Util/TestRunner.h"

static int g_copy_init_count   = 0;
static int g_copy_deinit_count = 0;

static bool tracked_copy_init(void *dst, void *src) {
    g_copy_init_count += 1;
    *(int *)dst = *(int *)src;
    return true;
}

static void tracked_copy_deinit(void *data) {
    g_copy_deinit_count += 1;
    *(int *)data = 0;
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

static bool test_list_remove_and_pop(void) {
    WriteFmt("Testing ListRemove and pop helpers\n");

    typedef List(int) IntList;
    IntList list    = ListInit();
    int     removed = 0;

    ListPushBackR(&list, 10);
    ListPushBackR(&list, 20);
    ListPushBackR(&list, 30);
    ListPushBackR(&list, 40);

    ListRemove(&list, &removed, 1);
    bool result = (removed == 20);
    result      = result && list_matches(GENERIC_LIST(&list), (const int[]) {10, 30, 40}, 3);

    ListPopFront(&list, &removed);
    result = result && (removed == 10);
    result = result && list_matches(GENERIC_LIST(&list), (const int[]) {30, 40}, 2);

    ListPopBack(&list, &removed);
    result = result && (removed == 40);
    result = result && list_matches(GENERIC_LIST(&list), (const int[]) {30}, 1);
    result = result && list.head && list.head->data && (*list.head->data == 30);
    result = result && list.tail && list.tail->data && (*list.tail->data == 30);

    ListDeinit(&list);
    return result;
}

static bool test_list_remove_range_and_delete_aliases(void) {
    WriteFmt("Testing ListRemoveRange and delete aliases\n");

    typedef List(int) IntList;
    IntList list     = ListInit();
    int     removed[2] = {0, 0};

    ListPushBackR(&list, 1);
    ListPushBackR(&list, 2);
    ListPushBackR(&list, 3);
    ListPushBackR(&list, 4);
    ListPushBackR(&list, 5);
    ListPushBackR(&list, 6);

    ListRemoveRange(&list, removed, 2, 2);
    bool result = (removed[0] == 3) && (removed[1] == 4);
    result      = result && list_matches(GENERIC_LIST(&list), (const int[]) {1, 2, 5, 6}, 4);

    ListDelete(&list, 1);
    result = result && list_matches(GENERIC_LIST(&list), (const int[]) {1, 5, 6}, 3);

    ListDeleteLast(&list);
    result = result && list_matches(GENERIC_LIST(&list), (const int[]) {1, 5}, 2);

    ListDeleteRange(&list, 0, 2);
    result = result && (ListLen(&list) == 0) && (list.head == NULL) && (list.tail == NULL);

    ListDeinit(&list);
    return result;
}

static bool test_list_remove_range_prefix_suffix_edges(void) {
    WriteFmt("Testing ListRemoveRange prefix/suffix edges\n");

    typedef List(int) IntList;
    IntList list = ListInit();
    int     prefix[2] = {0, 0};
    int     suffix[2] = {0, 0};

    ListPushBackR(&list, 1);
    ListPushBackR(&list, 2);
    ListPushBackR(&list, 3);
    ListPushBackR(&list, 4);
    ListPushBackR(&list, 5);
    ListPushBackR(&list, 6);

    ListRemoveRange(&list, prefix, 0, 2);
    bool result = (prefix[0] == 1) && (prefix[1] == 2);
    result      = result && list_matches(GENERIC_LIST(&list), (const int[]) {3, 4, 5, 6}, 4);

    ListRemoveRange(&list, suffix, 2, 2);
    result = result && (suffix[0] == 5) && (suffix[1] == 6);
    result = result && list_matches(GENERIC_LIST(&list), (const int[]) {3, 4}, 2);
    result = result && list.head && list.head->data && (*list.head->data == 3);
    result = result && list.tail && list.tail->data && (*list.tail->data == 4);

    ListDeinit(&list);
    return result;
}

static bool test_list_remove_range_whole_list_to_buffer(void) {
    WriteFmt("Testing ListRemoveRange whole-list buffered removal\n");

    typedef List(int) IntList;
    IntList list = ListInit();
    int     removed[3] = {0, 0, 0};

    ListPushBackR(&list, 7);
    ListPushBackR(&list, 8);
    ListPushBackR(&list, 9);
    ListRemoveRange(&list, removed, 0, 3);

    bool result = (removed[0] == 7) && (removed[1] == 8) && (removed[2] == 9);
    result      = result && (ListLen(&list) == 0) && (list.head == NULL) && (list.tail == NULL);

    ListDeinit(&list);
    return result;
}

static bool test_list_remove_zero_count_and_deep_copy_delete(void) {
    WriteFmt("Testing ListRemoveRange zero-count and deep-copy delete\n");

    typedef List(int) IntList;
    IntList list    = ListInitWithDeepCopy(tracked_copy_init, tracked_copy_deinit);
    int     removed = 0;

    reset_counters();
    ListPushBackR(&list, 7);
    ListPushBackR(&list, 8);
    ListPushBackR(&list, 9);

    ListRemoveRange(&list, NULL, 1, 0);
    ListRemoveRange(&list, NULL, ListLen(&list), 0);
    ListRemoveRange(&list, NULL, 9999, 0);
    bool result = (g_copy_init_count == 3);
    result      = result && (g_copy_deinit_count == 0);
    result      = result && list_matches(GENERIC_LIST(&list), (const int[]) {7, 8, 9}, 3);

    ListRemove(&list, &removed, 0);
    result = result && (removed == 7);
    result = result && (g_copy_deinit_count == 0);
    result = result && list_matches(GENERIC_LIST(&list), (const int[]) {8, 9}, 2);

    ListDelete(&list, 0);
    result = result && (g_copy_deinit_count == 1);
    result = result && list_matches(GENERIC_LIST(&list), (const int[]) {9}, 1);

    ListClear(&list);
    result = result && (g_copy_deinit_count == 2);
    result = result && (ListLen(&list) == 0) && (list.head == NULL) && (list.tail == NULL);

    ListDeinit(&list);
    return result;
}

static bool test_list_remove_range_with_deep_copy_buffer(void) {
    WriteFmt("Testing ListRemoveRange buffer semantics with deep copy\n");

    typedef List(int) IntList;
    IntList list = ListInitWithDeepCopy(tracked_copy_init, tracked_copy_deinit);
    int     removed[2] = {0, 0};

    reset_counters();
    ListPushBackR(&list, 4);
    ListPushBackR(&list, 5);
    ListPushBackR(&list, 6);

    ListRemoveRange(&list, removed, 1, 2);

    bool result = (g_copy_init_count == 3);
    result      = result && (g_copy_deinit_count == 0);
    result      = result && (removed[0] == 5) && (removed[1] == 6);
    result      = result && list_matches(GENERIC_LIST(&list), (const int[]) {4}, 1);

    ListDeinit(&list);
    result = result && (g_copy_deinit_count == 1);
    return result;
}

int main(void) {
    TestFunction tests[] = {
        test_list_remove_and_pop,
        test_list_remove_range_and_delete_aliases,
        test_list_remove_range_prefix_suffix_edges,
        test_list_remove_range_whole_list_to_buffer,
        test_list_remove_zero_count_and_deep_copy_delete,
        test_list_remove_range_with_deep_copy_buffer,
    };

    WriteFmt("[INFO] Starting List.Remove tests\n\n");
    return run_test_suite(tests, (int)(sizeof(tests) / sizeof(tests[0])), NULL, 0, "List.Remove");
}
