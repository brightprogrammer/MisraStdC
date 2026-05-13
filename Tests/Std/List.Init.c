#include <Misra/Std/Container/List.h>
#include <Misra/Std/Log.h>

#include "../Util/TestRunner.h"

static int g_copy_init_count   = 0;
static int g_copy_deinit_count = 0;

static bool tracked_copy_init(void *dst, const void *src, const Allocator *alloc) {
    (void)alloc;
    g_copy_init_count += 1;
    *(int *)dst = *(int *)src + 1000;
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

static bool test_list_init_variants(void) {
    WriteFmt("Testing List init variants\n");

    typedef List(int) IntList;
    IntList list_a = ListInit();
    IntList list_b = ListInitT(list_b);
    IntList list_c = ListInitWithDeepCopy(tracked_copy_init, tracked_copy_deinit);
    IntList list_d = ListInitWithDeepCopyT(list_d, tracked_copy_init, tracked_copy_deinit);

    ValidateList(&list_a);
    ValidateList(&list_b);
    ValidateList(&list_c);
    ValidateList(&list_d);

    bool result = (list_a.copy_init == NULL) && (list_a.copy_deinit == NULL) && (list_a.length == 0);
    result      = result && (list_b.copy_init == NULL) && (list_b.copy_deinit == NULL) && (list_b.length == 0);
    result      = result && (list_c.copy_init == tracked_copy_init) && (list_c.copy_deinit == tracked_copy_deinit);
    result      = result && (list_d.copy_init == tracked_copy_init) && (list_d.copy_deinit == tracked_copy_deinit);

    ListDeinit(&list_a);
    ListDeinit(&list_b);
    ListDeinit(&list_c);
    ListDeinit(&list_d);
    return result;
}

static bool test_list_init_optional_allocator(void) {
    WriteFmt("Testing List init optional allocator\n");

    typedef List(int) IntList;
    Allocator alloc = DefaultAllocator();
    alloc.retry_limit = 23;

    IntList list_a = ListInit(alloc);
    IntList list_b = ListInitT(list_b, alloc);
    IntList list_c = ListInitWithDeepCopy(tracked_copy_init, tracked_copy_deinit, alloc);
    IntList list_d = ListInitWithDeepCopyT(list_d, tracked_copy_init, tracked_copy_deinit, alloc);

    ValidateList(&list_a);
    ValidateList(&list_b);
    ValidateList(&list_c);
    ValidateList(&list_d);

    bool result = (list_a.allocator.retry_limit == 23) && (list_b.allocator.retry_limit == 23);
    result      = result && (list_c.allocator.retry_limit == 23) && (list_d.allocator.retry_limit == 23);
    result      = result && (list_c.copy_init == tracked_copy_init) && (list_d.copy_deinit == tracked_copy_deinit);

    ListDeinit(&list_a);
    ListDeinit(&list_b);
    ListDeinit(&list_c);
    ListDeinit(&list_d);
    return result;
}

static bool test_list_deinit_with_deep_copy(void) {
    WriteFmt("Testing ListDeinit with deep copy\n");

    typedef List(int) IntList;
    IntList list = ListInitWithDeepCopy(tracked_copy_init, tracked_copy_deinit);

    reset_counters();
    ListPushBackR(&list, 7);
    ListPushBackR(&list, 9);

    bool result = (g_copy_init_count == 2);
    result      = result && (ListAt(&list, 0) == 1007);
    result      = result && (ListAt(&list, 1) == 1009);

    ListDeinit(&list);

    result = result && (g_copy_deinit_count == 2);
    result = result && (list.head == NULL) && (list.tail == NULL) && (list.length == 0);
    result = result && (list.copy_init == NULL) && (list.copy_deinit == NULL);
    return result;
}

int main(void) {
    TestFunction tests[] = {
        test_list_init_variants,
        test_list_init_optional_allocator,
        test_list_deinit_with_deep_copy,
    };

    WriteFmt("[INFO] Starting List.Init tests\n\n");
    return run_test_suite(tests, (int)(sizeof(tests) / sizeof(tests[0])), NULL, 0, "List.Init");
}
