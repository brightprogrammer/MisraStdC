#include <Misra/Std/Container/List.h>
#include <Misra/Std/Log.h>
#include <Misra/Types.h>

#include "../Util/TestRunner.h"

static i32 compare_ints(const void *lhs, const void *rhs) {
    int a = *(const int *)lhs;
    int b = *(const int *)rhs;
    return (a > b) - (a < b);
}

static bool test_list_len_empty(void) {
    WriteFmt("Testing ListLen and ListEmpty\n");

    typedef List(int) IntList;
    IntList list = ListInit();

    bool result = (ListLen(&list) == 0);
    result      = result && ListEmpty(&list);

    ListPushBackR(&list, 10);
    ListPushBackR(&list, 20);

    result = result && (ListLen(&list) == 2);
    result = result && !ListEmpty(&list);

    ListClear(&list);
    result = result && (ListLen(&list) == 0);
    result = result && ListEmpty(&list);

    ListDeinit(&list);
    return result;
}

static bool test_list_find_contains(void) {
    WriteFmt("Testing ListFind and ListContains\n");

    typedef List(int) IntList;
    IntList list = ListInit();

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
    return result;
}

int main(void) {
    TestFunction tests[] = {
        test_list_len_empty,
        test_list_find_contains,
    };

    WriteFmt("[INFO] Starting List.Access tests\n\n");
    return run_test_suite(tests, (int)(sizeof(tests) / sizeof(tests[0])), NULL, 0, "List.Access");
}
