#include <Misra/Std/Container/List.h>
#include <Misra/Std/Log.h>

#include "../Util/TestRunner.h"

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
    *(int *)dst = *(int *)src;
    return true;
}

static void tracked_copy_deinit(void *data, const Allocator *alloc) {
    (void)alloc;
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

static bool test_list_clear_and_reuse(void) {
    WriteFmt("Testing ListClear and reuse\n");

    typedef List(int) IntList;
    IntList list = ListInit();

    ListClear(&list);
    ListPushBackR(&list, 1);
    ListPushBackR(&list, 2);
    ListPushBackR(&list, 3);
    ListClear(&list);

    bool result = (ListLen(&list) == 0) && (list.head == NULL) && (list.tail == NULL);

    ListPushBackR(&list, 9);
    result = result && list_matches(GENERIC_LIST(&list), (const int[]) {9}, 1);

    ListDeinit(&list);
    return result;
}

static bool test_list_sort_and_reverse(void) {
    WriteFmt("Testing ListSort and ListReverse\n");

    typedef List(int) IntList;
    IntList list = ListInit();

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
    return result;
}

static bool test_list_sort_and_reverse_edge_cases(void) {
    WriteFmt("Testing ListSort and ListReverse edge cases\n");

    typedef List(int) IntList;
    IntList empty     = ListInit();
    IntList singleton = ListInit();

    ListSort(&empty, compare_ints);
    ListReverse(&empty);
    ListPushBackR(&singleton, 42);
    ListSort(&singleton, compare_ints);
    ListReverse(&singleton);

    bool result = (ListLen(&empty) == 0) && (empty.head == NULL) && (empty.tail == NULL);
    result      = result && list_matches(GENERIC_LIST(&singleton), (const int[]) {42}, 1);

    ListDeinit(&empty);
    ListDeinit(&singleton);
    return result;
}

static bool test_list_clear_with_deep_copy(void) {
    WriteFmt("Testing ListClear with deep copy\n");

    typedef List(int) IntList;
    IntList list = ListInitWithDeepCopy(tracked_copy_init, tracked_copy_deinit);

    reset_counters();
    ListPushBackR(&list, 7);
    ListPushBackR(&list, 8);

    bool result = (g_copy_init_count == 2);

    ListClear(&list);
    result = result && (g_copy_deinit_count == 2);
    result = result && (ListLen(&list) == 0) && (list.head == NULL) && (list.tail == NULL);

    ListPushBackR(&list, 9);
    result = result && (g_copy_init_count == 3);
    result = result && list_matches(GENERIC_LIST(&list), (const int[]) {9}, 1);

    ListDeinit(&list);
    result = result && (g_copy_deinit_count == 3);
    return result;
}

int main(void) {
    TestFunction tests[] = {
        test_list_clear_and_reuse,
        test_list_sort_and_reverse,
        test_list_sort_and_reverse_edge_cases,
        test_list_clear_with_deep_copy,
    };

    WriteFmt("[INFO] Starting List.Ops tests\n\n");
    return run_test_suite(tests, (int)(sizeof(tests) / sizeof(tests[0])), NULL, 0, "List.Ops");
}
