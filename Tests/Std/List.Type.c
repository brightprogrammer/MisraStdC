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

    // `__magic` is verifying the private magic-value invariant the validator depends on
    // (intentional bypass for that field; the rest go through public accessors).
    bool result = (ListHead(&list) == NULL) && (ListTail(&list) == NULL) && (ListCopyInit(&list) == NULL) &&
                  (ListCopyDeinit(&list) == NULL) && (ListLen(&list) == 0) && (list.__magic == LIST_MAGIC);

    ListDeinit(&list);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

static bool test_list_node_type_layout(void) {
    WriteFmt("Testing ListNode type layout\n");

    int value          = 42;
    ListNode(int) node = {0};
    // intentional bypass: building a node literal on the stack and reading its
    // fields directly to verify the ListNode(T) layout. The list-managed
    // accessors are designed for nodes owned by a List; this fixture has no list.
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
