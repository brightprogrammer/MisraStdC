#include <Misra/Std/Allocator/Default.h>
#include <Misra/Std/Container/List.h>
#include <Misra/Std/Log.h>

#include "../Util/TestRunner.h"

static bool test_list_type_defaults(void) {
    WriteFmt("Testing List type defaults\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    typedef List(int) IntList;
    IntList list = ListInit(&alloc);

    ValidateList(&list);

    bool result = (list.head == NULL) && (list.tail == NULL) && (list.copy_init == NULL) &&
                  (list.copy_deinit == NULL) && (ListLen(&list) == 0) && (list.__magic == LIST_MAGIC);

    ListDeinit(&list);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

static bool test_list_node_type_layout(void) {
    WriteFmt("Testing ListNode type layout\n");

    int value          = 42;
    ListNode(int) node = {0};
    node.data          = &value;

    return (node.next == NULL) && (node.prev == NULL) && (node.data == &value) && (*node.data == 42);
}

int main(void) {
    TestFunction tests[] = {
        test_list_type_defaults,
        test_list_node_type_layout,
    };

    WriteFmt("[INFO] Starting List.Type tests\n\n");
    return run_test_suite(tests, (int)(sizeof(tests) / sizeof(tests[0])), NULL, 0, "List.Type");
}
