#include <Misra/Std/Allocator/Default.h>
#include <Misra/Std/Container/List.h>
#include <Misra/Std/Log.h>

#include "../Util/TestRunner.h"

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

#define FILL_INT_LIST(list_ptr)                                                                                        \
    do {                                                                                                               \
        ListPushBackR((list_ptr), 10);                                                                                 \
        ListPushBackR((list_ptr), 20);                                                                                 \
        ListPushBackR((list_ptr), 30);                                                                                 \
        ListPushBackR((list_ptr), 40);                                                                                 \
        ListPushBackR((list_ptr), 50);                                                                                 \
    } while (0)

static bool test_list_foreach_basic(void) {
    WriteFmt("Testing basic List foreach macros\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    typedef List(int) IntList;
    IntList list              = ListInit(&alloc);
    int     reverse_values[5] = {0};
    int     reverse_i         = 0;
    int     forward_sum       = 0;

    FILL_INT_LIST(&list);

    ListForeach(&list, value) {
        forward_sum += value;
    }

    ListForeachPtr(&list, value_ptr) {
        *value_ptr += 1;
    }

    ListForeachReverse(&list, value) {
        reverse_values[reverse_i++] = value;
    }

    ListForeachPtrReverse(&list, value_ptr) {
        *value_ptr -= 1;
    }

    bool result = (forward_sum == 150);
    result      = result && (reverse_i == 5);
    result      = result && (reverse_values[0] == 51) && (reverse_values[1] == 41) && (reverse_values[2] == 31) &&
             (reverse_values[3] == 21) && (reverse_values[4] == 11);
    result = result && list_matches(GENERIC_LIST(&list), (const int[]) {10, 20, 30, 40, 50}, 5);

    ListDeinit(&list);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

static bool test_list_foreach_ranges(void) {
    WriteFmt("Testing ranged List foreach macros\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    typedef List(int) IntList;
    IntList list              = ListInit(&alloc);
    int     forward_range_sum = 0;
    int     reverse_range_sum = 0;

    FILL_INT_LIST(&list);

    ListForeachInRange(&list, value, 1, 4) {
        forward_range_sum += value;
    }

    ListForeachPtrInRange(&list, value_ptr, 1, 4) {
        *value_ptr *= 2;
    }

    ListForeachReverseInRange(&list, value, 1, 4) {
        reverse_range_sum += value;
    }

    ListForeachPtrReverseInRange(&list, value_ptr, 1, 4) {
        *value_ptr /= 2;
    }

    bool result = (forward_range_sum == 90);
    result      = result && (reverse_range_sum == 180);
    result      = result && list_matches(GENERIC_LIST(&list), (const int[]) {10, 20, 30, 40, 50}, 5);

    ListDeinit(&list);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

static bool test_list_foreach_range_edge_cases(void) {
    WriteFmt("Testing List ranged foreach edge cases\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    typedef List(int) IntList;
    IntList list  = ListInit(&alloc);
    int     count = 0;

    FILL_INT_LIST(&list);

    ListForeachInRange(&list, value, 2, 2) {
        (void)value;
        count += 1;
    }

    ListForeachPtrInRange(&list, value_ptr, 7, 9) {
        (void)value_ptr;
        count += 10;
    }

    ListForeachReverseInRange(&list, value, 3, 3) {
        (void)value;
        count += 100;
    }

    ListForeachPtrReverseInRange(&list, value_ptr, 8, 10) {
        (void)value_ptr;
        count += 1000;
    }

    ListForeachInRange(&list, value, 4, 2) {
        (void)value;
        count += 10000;
    }

    ListForeachReverseInRange(&list, value, 4, 2) {
        (void)value;
        count += 100000;
    }

    bool result = (count == 0);
    result      = result && list_matches(GENERIC_LIST(&list), (const int[]) {10, 20, 30, 40, 50}, 5);

    ListDeinit(&list);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

static bool test_list_foreach_index_variants(void) {
    WriteFmt("Testing indexed List foreach macros\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    typedef List(int) IntList;
    IntList list              = ListInit(&alloc);
    int     reverse_values[5] = {0};
    int     reverse_i         = 0;
    int     idx_sum           = 0;
    int     reverse_idx_sum   = 0;
    int     value_sum         = 0;

    FILL_INT_LIST(&list);

    ListForeachIdx(&list, value, idx) {
        idx_sum   += (int)idx;
        value_sum += value;
    }

    ListForeachPtrIdx(&list, value_ptr, idx) {
        *value_ptr += (int)idx;
    }

    ListForeachReverseIdx(&list, value, idx) {
        reverse_idx_sum             += (int)idx;
        reverse_values[reverse_i++]  = value;
    }

    ListForeachPtrReverseIdx(&list, value_ptr, idx) {
        *value_ptr -= (int)idx;
    }

    bool result = (idx_sum == 10) && (value_sum == 150);
    result      = result && (reverse_idx_sum == 10) && (reverse_i == 5);
    result      = result && (reverse_values[0] == 54) && (reverse_values[1] == 43) && (reverse_values[2] == 32) &&
             (reverse_values[3] == 21) && (reverse_values[4] == 10);
    result = result && list_matches(GENERIC_LIST(&list), (const int[]) {10, 20, 30, 40, 50}, 5);

    ListDeinit(&list);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

static bool test_list_foreach_index_jump_contract(void) {
    WriteFmt("Testing indexed List foreach jump contract\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    typedef List(int) IntList;
    IntList list              = ListInit(&alloc);
    int     forward_values[3] = {0};
    int     reverse_values[3] = {0};
    int     forward_i         = 0;
    int     reverse_i         = 0;

    FILL_INT_LIST(&list);

    ListForeachIdx(&list, value, idx) {
        forward_values[forward_i++] = value;
        if (idx == 0) {
            idx = 2;
        } else if (idx == 2) {
            idx = 4;
        }
    }

    ListForeachPtrIdx(&list, value_ptr, idx) {
        *value_ptr += (int)idx;
        if (idx == 0) {
            idx = 2;
        } else if (idx == 2) {
            idx = 4;
        }
    }

    ListForeachReverseIdx(&list, value, idx) {
        reverse_values[reverse_i++] = value;
        if (idx == 4) {
            idx = 2;
        } else if (idx == 2) {
            idx = 0;
        }
    }

    ListForeachPtrReverseIdx(&list, value_ptr, idx) {
        *value_ptr -= (int)idx;
        if (idx == 4) {
            idx = 2;
        } else if (idx == 2) {
            idx = 0;
        }
    }

    bool result = (forward_i == 3) && (reverse_i == 3);
    result      = result && (forward_values[0] == 10) && (forward_values[1] == 30) && (forward_values[2] == 50);
    result      = result && (reverse_values[0] == 54) && (reverse_values[1] == 32) && (reverse_values[2] == 10);
    result      = result && list_matches(GENERIC_LIST(&list), (const int[]) {10, 20, 30, 40, 50}, 5);

    ListDeinit(&list);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

static bool test_list_foreach_empty_lists(void) {
    WriteFmt("Testing List foreach macros on empty list\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    typedef List(int) IntList;
    IntList list  = ListInit(&alloc);
    u64     count = 0;

    ListForeach(&list, value) {
        (void)value;
        count += 1;
    }

    ListForeachPtr(&list, value_ptr) {
        (void)value_ptr;
        count += 10;
    }

    ListForeachReverse(&list, value) {
        (void)value;
        count += 100;
    }

    ListForeachPtrReverse(&list, value_ptr) {
        (void)value_ptr;
        count += 1000;
    }

    ListForeachInRange(&list, value, 0, 1) {
        (void)value;
        count += 10000;
    }

    ListForeachPtrInRange(&list, value_ptr, 0, 1) {
        (void)value_ptr;
        count += 100000;
    }

    ListForeachReverseInRange(&list, value, 0, 1) {
        (void)value;
        count += 1000000;
    }

    ListForeachPtrReverseInRange(&list, value_ptr, 0, 1) {
        (void)value_ptr;
        count += 10000000;
    }

    ListForeachIdx(&list, value, idx) {
        (void)value;
        (void)idx;
        count += 100000000;
    }

    ListForeachPtrIdx(&list, value_ptr, idx) {
        (void)value_ptr;
        (void)idx;
        count += 1000000000;
    }

    ListForeachReverseIdx(&list, value, idx) {
        (void)value;
        (void)idx;
        count += 10000000000;
    }

    ListForeachPtrReverseIdx(&list, value_ptr, idx) {
        (void)value_ptr;
        (void)idx;
        count += 100000000000;
    }

    ListDeinit(&list);
    DefaultAllocatorDeinit(&alloc);
    return count == 0;
}

int main(void) {
    TestFunction tests[] = {
        test_list_foreach_basic,
        test_list_foreach_ranges,
        test_list_foreach_range_edge_cases,
        test_list_foreach_index_variants,
        test_list_foreach_index_jump_contract,
        test_list_foreach_empty_lists,
    };

    WriteFmt("[INFO] Starting List.Foreach tests\n\n");
    return run_test_suite(tests, (int)(sizeof(tests) / sizeof(tests[0])), NULL, 0, "List.Foreach");
}
