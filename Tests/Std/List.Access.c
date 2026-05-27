#include <Misra/Std/Allocator/Default.h>
#include <Misra/Std/Container/List.h>
#include <Misra/Std/Log.h>
#include <Misra/Types.h>

#include "../Util/TestRunner.h"

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

int main(void) {
    TestFunction tests[] = {
        test_list_len_empty,
        test_list_value_access_and_swap,
        test_list_node_access_and_navigation,
        test_list_find_contains,
    };

    WriteFmt("[INFO] Starting List.Access tests\n\n");
    return run_test_suite(tests, (int)(sizeof(tests) / sizeof(tests[0])), NULL, 0, "List.Access");
}
