#include <Misra/Std/Allocator/Default.h>
#include <Misra/Std/Container/List.h>
#include <Misra/Std/Log.h>

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

static bool test_list_init_variants(void) {
    WriteFmt("Testing List init variants\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    typedef List(int) IntList;
    IntList list_a = ListInit(&alloc);
    IntList list_b = ListInitT(list_b, &alloc);
    IntList list_c = ListInitWithDeepCopy(tracked_copy_init, tracked_copy_deinit, &alloc);
    IntList list_d = ListInitWithDeepCopyT(list_d, tracked_copy_init, tracked_copy_deinit, &alloc);

    ValidateList(&list_a);
    ValidateList(&list_b);
    ValidateList(&list_c);
    ValidateList(&list_d);

    bool result = (ListCopyInit(&list_a) == NULL) && (ListCopyDeinit(&list_a) == NULL) && (ListLen(&list_a) == 0);
    result      = result && (ListCopyInit(&list_b) == NULL) && (ListCopyDeinit(&list_b) == NULL) && (ListLen(&list_b) == 0);
    result      = result && (ListCopyInit(&list_c) == tracked_copy_init) && (ListCopyDeinit(&list_c) == tracked_copy_deinit);
    result      = result && (ListCopyInit(&list_d) == tracked_copy_init) && (ListCopyDeinit(&list_d) == tracked_copy_deinit);

    ListDeinit(&list_a);
    ListDeinit(&list_b);
    ListDeinit(&list_c);
    ListDeinit(&list_d);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

static bool test_list_init_optional_allocator(void) {
    WriteFmt("Testing List init optional allocator\n");

    typedef List(int) IntList;
    DefaultAllocator alloc = DefaultAllocatorInit();
    alloc.base.retry_limit = 23;

    IntList list_a = ListInit(&alloc);
    IntList list_b = ListInitT(list_b, &alloc);
    IntList list_c = ListInitWithDeepCopy(tracked_copy_init, tracked_copy_deinit, &alloc);
    IntList list_d = ListInitWithDeepCopyT(list_d, tracked_copy_init, tracked_copy_deinit, &alloc);

    ValidateList(&list_a);
    ValidateList(&list_b);
    ValidateList(&list_c);
    ValidateList(&list_d);

    bool result = (ListAllocator(&list_a)->retry_limit == 23) && (ListAllocator(&list_b)->retry_limit == 23);
    result      = result && (ListAllocator(&list_c)->retry_limit == 23) && (ListAllocator(&list_d)->retry_limit == 23);
    result      = result && (ListCopyInit(&list_c) == tracked_copy_init) && (ListCopyDeinit(&list_d) == tracked_copy_deinit);

    ListDeinit(&list_a);
    ListDeinit(&list_b);
    ListDeinit(&list_c);
    ListDeinit(&list_d);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

static bool test_list_deinit_with_deep_copy(void) {
    WriteFmt("Testing ListDeinit with deep copy\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    typedef List(int) IntList;
    IntList list = ListInitWithDeepCopy(tracked_copy_init, tracked_copy_deinit, &alloc);

    reset_counters();
    ListPushBackR(&list, 7);
    ListPushBackR(&list, 9);

    bool result = (g_copy_init_count == 2);
    result      = result && (ListAt(&list, 0) == 1007);
    result      = result && (ListAt(&list, 1) == 1009);

    ListDeinit(&list);

    result = result && (g_copy_deinit_count == 2);
    result = result && (ListHead(&list) == NULL) && (ListTail(&list) == NULL) && (ListLen(&list) == 0);
    // Verifying ListDeinit clears the hook fields.
    result = result && (ListCopyInit(&list) == NULL) && (ListCopyDeinit(&list) == NULL);
    DefaultAllocatorDeinit(&alloc);
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
