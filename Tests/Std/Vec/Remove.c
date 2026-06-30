#include <Misra/Std/Allocator/Default.h>
#include <Misra/Std/Container/Vec.h>
#include <Misra/Std/Log.h>

#include <Misra/Types.h> // For LVAL macro

// Include test utilities for deadend testing
#include "../../Util/TestRunner.h"

// Function prototypes
bool test_vec_pop_back(void);
bool test_vec_pop_front(void);
bool test_vec_delete(void);
bool test_vec_delete_fast(void);
bool test_vec_delete_range(void);
bool test_vec_delete_range_fast(void);
bool test_vec_delete_last(void);
bool test_lvalue_delete_operations(void);
bool test_rvalue_delete_operations(void);
bool test_lvalue_fast_delete_operations(void);
bool test_rvalue_fast_delete_operations(void);
bool test_lvalue_delete_range_operations(void);
bool test_rvalue_delete_range_operations(void);
bool test_lvalue_fast_delete_range_operations(void);
bool test_rvalue_fast_delete_range_operations(void);

// ---- fast_remove_range_vec mutation-hardening prototypes (Vec.Mutants3) --
bool test_fast_remove_whole_vector_succeeds(void);
bool test_fast_remove_copies_out_removed_data(void);
bool test_fast_remove_copies_out_removed_data_stride(void);
bool test_fast_remove_deinits_every_removed_element(void);
bool test_fast_remove_deinit_runs_at_least_once(void);
bool test_fast_remove_deinit_not_run_on_survivor(void);
bool test_fast_remove_deinit_runs_for_each(void);
bool test_fast_remove_deinit_walks_each_element(void);
bool test_fast_remove_zeroes_vacated_tail_sub(void);
bool test_fast_remove_zeroes_vacated_tail_stride(void);
bool test_fast_remove_zeroes_vacated_tail_call(void);

// ---- remove_range_vec mutation-hardening prototypes (Vec.Mutants4) -------
bool test_remove_deinit_init_zero(void);
bool test_remove_deinit_runs_at_all(void);
bool test_remove_deinit_no_overrun(void);
bool test_remove_deinit_all_three(void);
bool test_remove_deinit_advances_cursor(void);
bool test_remove_compaction_len_first_term(void);
bool test_remove_compaction_len_second_term(void);
bool test_remove_compaction_stride(void);
bool test_remove_tail_clear_dest(void);
bool test_remove_tail_clear_len(void);
bool test_remove_tail_clear_stride(void);

// ---- remove compaction size prototypes (from Vec.Blind) -----------------
bool test_remove_compaction_size_first_term(void);
bool test_remove_compaction_size_second_term(void);

// Test VecPopBack function
static DefaultAllocator alloc;

// ---- Shared copy_deinit instrumentation (fast_remove_range_vec, Mutants3) -
static size g_deinit_count;
static int  g_deinit_vals[32];
static size g_deinit_nvals;

static void counting_deinit(void *copy, const Allocator *al) {
    (void)al;
    g_deinit_count++;
    if (g_deinit_nvals < 32) {
        g_deinit_vals[g_deinit_nvals++] = *(int *)copy;
    }
}

static void reset_deinit_state(void) {
    g_deinit_count = 0;
    g_deinit_nvals = 0;
    for (size i = 0; i < 32; i++) {
        g_deinit_vals[i] = 0;
    }
}

// ---- Deinit-tracking element fixture (remove_range_vec, Mutants4) --------
typedef struct {
    int id;
} Elem;

#define LEDGER_N 64
static int g_freed[LEDGER_N];

static void elem_deinit(void *copy, const Allocator *a) {
    (void)a;
    Elem *e = (Elem *)copy;
    if (e->id >= 0 && e->id < LEDGER_N) {
        g_freed[e->id]++;
    }
}

static void reset_ledger(void) {
    for (int i = 0; i < LEDGER_N; i++) {
        g_freed[i] = 0;
    }
}

typedef Vec(Elem) ElemVec;

// Build a deinit-tracking vec holding elements with ids 0..n-1. copy_init is
// left NULL so PushBack just MemCopies the struct; copy_deinit fires on
// removal (removed_data == NULL path).
static ElemVec make_elem_vec(int n) {
    ElemVec vec = VecInitWithDeepCopyT(vec, NULL, elem_deinit, &alloc);
    for (int i = 0; i < n; i++) {
        Elem e = {.id = i};
        VecPushBack(&vec, e);
    }
    return vec;
}

bool test_vec_pop_back(void) {
    WriteFmtLn("Testing VecPopBack");

    // Create a vector of integers
    typedef Vec(int) IntVec;
    IntVec vec = VecInit(&alloc);

    // Add some data
    int values[] = {10, 20, 30, 40, 50};
    for (int i = 0; i < 5; i++) {
        int val = values[i];
        VecPushBack(&vec, val);
    }

    // Initial length should be 5
    bool result = (VecLen(&vec) == 5);

    // Pop from the back
    int popped;
    VecPopBack(&vec, &popped);

    // Check popped value
    result = result && (popped == 50);

    // Check new length
    result = result && (VecLen(&vec) == 4);

    // Check remaining elements
    for (u64 i = 0; i < VecLen(&vec); i++) {
        result = result && (VecAt(&vec, i) == values[i]);
    }

    // Pop again
    VecPopBack(&vec, &popped);

    // Check popped value
    result = result && (popped == 40);

    // Check new length
    result = result && (VecLen(&vec) == 3);

    // Clean up
    VecDeinit(&vec);

    return result;
}

// Test VecPopFront function
bool test_vec_pop_front(void) {
    WriteFmtLn("Testing VecPopFront");

    // Create a vector of integers
    typedef Vec(int) IntVec;
    IntVec vec = VecInit(&alloc);

    // Add some data
    int values[] = {10, 20, 30, 40, 50};
    for (int i = 0; i < 5; i++) {
        int val = values[i];
        VecPushBack(&vec, val);
    }

    // Initial length should be 5
    bool result = (VecLen(&vec) == 5);

    // Pop from the front
    int popped;
    VecPopFront(&vec, &popped);

    // Check popped value
    result = result && (popped == 10);

    // Check new length
    result = result && (VecLen(&vec) == 4);

    // Check remaining elements
    for (u64 i = 0; i < VecLen(&vec); i++) {
        result = result && (VecAt(&vec, i) == values[i + 1]);
    }

    // Pop again
    VecPopFront(&vec, &popped);

    // Check popped value
    result = result && (popped == 20);

    // Check new length
    result = result && (VecLen(&vec) == 3);

    // Clean up
    VecDeinit(&vec);

    return result;
}

// Test VecDelete function
bool test_vec_delete(void) {
    WriteFmtLn("Testing VecDelete");

    // Create a vector of integers
    typedef Vec(int) IntVec;
    IntVec vec = VecInit(&alloc);

    // Add some data
    int values[] = {10, 20, 30, 40, 50};
    for (int i = 0; i < 5; i++) {
        int val = values[i];
        VecPushBack(&vec, val);
    }

    // Initial length should be 5
    bool result = (VecLen(&vec) == 5);

    // Delete element at index 2 (value 30)
    VecDelete(&vec, 2);

    // Check new length
    result = result && (VecLen(&vec) == 4);

    // Check remaining elements (should be [10, 20, 40, 50])
    int expected1[] = {10, 20, 40, 50};
    for (u64 i = 0; i < VecLen(&vec); i++) {
        result = result && (VecAt(&vec, i) == expected1[i]);
    }

    // Delete element at index 0 (value 10)
    VecDelete(&vec, 0);

    // Check new length
    result = result && (VecLen(&vec) == 3);

    // Check remaining elements (should be [20, 40, 50])
    int expected2[] = {20, 40, 50};
    for (u64 i = 0; i < VecLen(&vec); i++) {
        result = result && (VecAt(&vec, i) == expected2[i]);
    }

    // Clean up
    VecDeinit(&vec);

    return result;
}

// Test VecDeleteFast function
bool test_vec_delete_fast(void) {
    WriteFmtLn("Testing VecDeleteFast");

    // Create a vector of integers
    typedef Vec(int) IntVec;
    IntVec vec = VecInit(&alloc);

    // Add some data
    int values[] = {10, 20, 30, 40, 50};
    for (int i = 0; i < 5; i++) {
        int val = values[i];
        VecPushBack(&vec, val);
    }

    // Initial length should be 5
    bool result = (VecLen(&vec) == 5);

    // Delete element at index 1 (value 20) using fast delete
    VecDeleteFast(&vec, 1);

    // Check new length
    result = result && (VecLen(&vec) == 4);

    // With fast delete, the last element is moved to the deleted position
    // So the vector should now be [10, 50, 30, 40]
    int expected[] = {10, 50, 30, 40};
    for (u64 i = 0; i < VecLen(&vec); i++) {
        result = result && (VecAt(&vec, i) == expected[i]);
    }

    // Clean up
    VecDeinit(&vec);

    return result;
}

// Test VecDeleteRange function
bool test_vec_delete_range(void) {
    WriteFmtLn("Testing VecDeleteRange");

    // Create a vector of integers
    typedef Vec(int) IntVec;
    IntVec vec = VecInit(&alloc);

    // Add some data
    int values[] = {10, 20, 30, 40, 50, 60, 70};
    for (int i = 0; i < 7; i++) {
        int val = values[i];
        VecPushBack(&vec, val);
    }

    // Initial length should be 7
    bool result = (VecLen(&vec) == 7);

    // Delete range from index 2 to 4 (values 30, 40, 50)
    VecDeleteRange(&vec, 2, 3);

    // Check new length
    result = result && (VecLen(&vec) == 4);

    // Check remaining elements (should be [10, 20, 60, 70])
    int expected[] = {10, 20, 60, 70};
    for (u64 i = 0; i < VecLen(&vec); i++) {
        result = result && (VecAt(&vec, i) == expected[i]);
    }

    // Clean up
    VecDeinit(&vec);

    return result;
}

// Test VecDeleteRangeFast
bool test_vec_delete_range_fast(void) {
    WriteFmtLn("Testing VecDeleteRangeFast");

    // Create a vector of integers
    typedef Vec(int) IntVec;
    IntVec vec = VecInit(&alloc);

    // Add some data
    for (int i = 0; i < 10; i++) {
        int val = i * 10;
        VecPushBack(&vec, val);
    }

    // Initial length should be 10
    bool result = (VecLen(&vec) == 10);

    // Print before state
    WriteFmt("Before fast range delete: ");
    for (u64 i = 0; i < VecLen(&vec); i++) {
        WriteFmt("{} ", VecAt(&vec, i));
    }
    WriteFmt("\n");

    // Test VecDeleteRangeFast - delete 3 elements starting at index 2
    int start_index = 2;
    int count       = 3;

    // Remember the values that will be moved from the end
    int end_values[3];
    for (int i = 0; i < count; i++) {
        end_values[i] = VecAt(&vec, VecLen(&vec) - count + i);
    }

    VecDeleteRangeFast(&vec, start_index, count);

    // Print after state
    WriteFmt("After fast range delete: ");
    for (u64 i = 0; i < VecLen(&vec); i++) {
        WriteFmt("{} ", VecAt(&vec, i));
    }
    WriteFmt("\n");

    // Check length after deletion
    result = result && (VecLen(&vec) == 7);

    // Check that the last 3 elements moved to the deleted positions
    for (int i = 0; i < count; i++) {
        result = result && (VecAt(&vec, start_index + i) == end_values[i]);
    }

    // Verify all values that should still be present
    bool values_found[10] = {false};
    for (u64 i = 0; i < VecLen(&vec); i++) {
        int val   = VecAt(&vec, i);
        int index = val / 10;
        if (index >= 0 && index < 10) {
            values_found[index] = true;
        }
    }

    // Values 2, 3, 4 should be removed
    for (int i = 0; i < 10; i++) {
        if (i == 2 || i == 3 || i == 4) {
            result = result && !values_found[i];
        } else {
            result = result && values_found[i];
        }
    }

    // Clean up
    VecDeinit(&vec);

    return result;
}

// Test VecDeleteLast function
bool test_vec_delete_last(void) {
    WriteFmtLn("Testing VecDeleteLast");

    // Create a vector of integers
    typedef Vec(int) IntVec;
    IntVec vec = VecInit(&alloc);

    // Add some data
    int values[] = {10, 20, 30, 40, 50};
    for (int i = 0; i < 5; i++) {
        int val = values[i];
        VecPushBack(&vec, val);
    }

    // Initial length should be 5
    bool result = (VecLen(&vec) == 5);

    // Delete the last element
    VecDeleteLast(&vec);

    // Check new length
    result = result && (VecLen(&vec) == 4);

    // Check remaining elements
    for (u64 i = 0; i < VecLen(&vec); i++) {
        result = result && (VecAt(&vec, i) == values[i]);
    }

    // Delete the last element again
    VecDeleteLast(&vec);

    // Check new length
    result = result && (VecLen(&vec) == 3);

    // Check remaining elements
    for (u64 i = 0; i < VecLen(&vec); i++) {
        result = result && (VecAt(&vec, i) == values[i]);
    }

    // Clean up
    VecDeinit(&vec);

    return result;
}

// Test L-value standard delete operations
bool test_lvalue_delete_operations(void) {
    WriteFmtLn("Testing L-value standard delete operations");

    // Create a vector of integers
    typedef Vec(int) IntVec;
    IntVec vec = VecInit(&alloc);

    // Add some data
    int values[] = {10, 20, 30, 40, 50};
    for (int i = 0; i < 5; i++) {
        VecPushBack(&vec, values[i]);
    }

    // Initial length should be 5
    bool result = (VecLen(&vec) == 5);

    // Test L-value delete operation
    int index_to_delete = 2; // Delete 30
    VecDelete(&vec, index_to_delete);

    // Check vector after L-value deletion
    result = result && (VecLen(&vec) == 4);

    // Check remaining elements (should be [10, 20, 40, 50])
    int expected[] = {10, 20, 40, 50};
    for (u64 i = 0; i < VecLen(&vec); i++) {
        result = result && (VecAt(&vec, i) == expected[i]);
    }

    // Clean up
    VecDeinit(&vec);

    return result;
}

// Test R-value standard delete operations
bool test_rvalue_delete_operations(void) {
    WriteFmtLn("Testing R-value standard delete operations");

    // Create a vector of integers
    typedef Vec(int) IntVec;
    IntVec vec = VecInit(&alloc);

    // Add some data
    int values[] = {10, 20, 30, 40, 50};
    for (int i = 0; i < 5; i++) {
        VecPushBack(&vec, values[i]);
    }

    // Initial length should be 5
    bool result = (VecLen(&vec) == 5);

    // Test R-value delete operation
    VecDelete(&vec, 2); // Delete 30

    // Check vector after deletion
    result = result && (VecLen(&vec) == 4);

    // Check remaining elements (should be [10, 20, 40, 50])
    int expected[] = {10, 20, 40, 50};
    for (u64 i = 0; i < VecLen(&vec); i++) {
        result = result && (VecAt(&vec, i) == expected[i]);
    }

    // Clean up
    VecDeinit(&vec);

    return result;
}

// Test L-value fast delete operations
bool test_lvalue_fast_delete_operations(void) {
    WriteFmtLn("Testing L-value fast delete operations");

    // Create a vector of integers
    typedef Vec(int) IntVec;
    IntVec vec = VecInit(&alloc);

    // Add some data
    int values[] = {10, 20, 30, 40, 50};
    for (int i = 0; i < 5; i++) {
        VecPushBack(&vec, values[i]);
    }

    // Initial length should be 5
    bool result = (VecLen(&vec) == 5);

    // Print before state
    WriteFmt("Before L-value fast delete: ");
    for (u64 i = 0; i < VecLen(&vec); i++) {
        WriteFmt("{} ", VecAt(&vec, i));
    }
    WriteFmt("\n");

    // Test L-value fast delete operation
    int fast_index       = 2;                             // Delete 30
    int valueToBeDeleted = VecAt(&vec, fast_index);
    int lastValue        = VecAt(&vec, VecLen(&vec) - 1); // Should move to deleted position
    VecDeleteFast(&vec, fast_index);

    // Print after state
    WriteFmt("After L-value fast delete: ");
    for (u64 i = 0; i < VecLen(&vec); i++) {
        WriteFmt("{} ", VecAt(&vec, i));
    }
    WriteFmt("\n");

    // Check vector after L-value fast deletion
    result = result && (VecLen(&vec) == 4);

    // Verify the deleted value is no longer present
    bool containsValue = false;
    for (u64 i = 0; i < VecLen(&vec); i++) {
        if (VecAt(&vec, i) == valueToBeDeleted) {
            containsValue = true;
            break;
        }
    }
    result = result && !containsValue;

    // Check that the value at the deleted position is now the last value
    result = result && (VecAt(&vec, fast_index) == lastValue);
    WriteFmtLn(
        "Value at deleted position ({}) is now {} (expected {})\n",
        fast_index,
        VecAt(&vec, fast_index),
        lastValue
    );

    // Verify all expected values (except the deleted one and the moved one) are still present
    int expected_values[] = {10, 20, 40}; // 30 was deleted, 50 was moved
    for (int i = 0; i < 3; i++) {
        bool found = false;
        for (u64 j = 0; j < VecLen(&vec); j++) {
            if (VecAt(&vec, j) == expected_values[i]) {
                found = true;
                break;
            }
        }
        result = result && found;
        if (!found) {
            WriteFmtLn("Value {} should be present but was not found", expected_values[i]);
        }
    }

    // Clean up
    VecDeinit(&vec);

    return result;
}

// Test R-value fast delete operations
bool test_rvalue_fast_delete_operations(void) {
    WriteFmtLn("Testing R-value fast delete operations");

    // Create a vector of integers
    typedef Vec(int) IntVec;
    IntVec vec = VecInit(&alloc);

    // Add some data
    int values[] = {10, 20, 30, 40, 50};
    for (int i = 0; i < 5; i++) {
        VecPushBack(&vec, values[i]);
    }

    // Initial length should be 5
    bool result = (VecLen(&vec) == 5);

    // Print before state
    WriteFmt("Before R-value fast delete: ");
    for (u64 i = 0; i < VecLen(&vec); i++) {
        WriteFmt("{} ", VecAt(&vec, i));
    }
    WriteFmt("\n");

    // Remember the value to be deleted and the last value
    int valueToBeDeleted = VecAt(&vec, 2);                // 30
    int lastValue        = VecAt(&vec, VecLen(&vec) - 1); // Should move to deleted position

    // Test R-value fast delete operation
    VecDeleteFast(&vec, 2);

    // Print after state
    WriteFmt("After R-value fast delete: ");
    for (u64 i = 0; i < VecLen(&vec); i++) {
        WriteFmt("{} ", VecAt(&vec, i));
    }
    WriteFmt("\n");

    // Check length
    result = result && (VecLen(&vec) == 4);

    // Verify the deleted value is no longer present
    bool containsValue = false;
    for (u64 i = 0; i < VecLen(&vec); i++) {
        if (VecAt(&vec, i) == valueToBeDeleted) {
            containsValue = true;
            break;
        }
    }
    result = result && !containsValue;

    // Check that the value at the deleted position is now the last value
    result = result && (VecAt(&vec, 2) == lastValue);
    WriteFmtLn("Value at deleted position (2) is now {} (expected {})\n", VecAt(&vec, 2), lastValue);

    // Verify all expected values (except the deleted one and the moved one) are still present
    int expected_values[] = {10, 20, 40}; // 30 was deleted, 50 was moved
    for (int i = 0; i < 3; i++) {
        bool found = false;
        for (u64 j = 0; j < VecLen(&vec); j++) {
            if (VecAt(&vec, j) == expected_values[i]) {
                found = true;
                break;
            }
        }
        result = result && found;
        if (!found) {
            WriteFmtLn("Value {} should be present but was not found", expected_values[i]);
        }
    }

    // Clean up
    VecDeinit(&vec);

    return result;
}

// Test L-value delete range operations
bool test_lvalue_delete_range_operations(void) {
    WriteFmtLn("Testing L-value delete range operations");

    // Create a vector of integers
    typedef Vec(int) IntVec;
    IntVec vec = VecInit(&alloc);

    // Add some data
    int values[] = {10, 20, 30, 40, 50, 60, 70};
    for (int i = 0; i < 7; i++) {
        VecPushBack(&vec, values[i]);
    }

    // Initial length should be 7
    bool result = (VecLen(&vec) == 7);

    // Test L-value delete range operation
    int start_index = 2;
    int count       = 3;
    VecDeleteRange(&vec, start_index, count); // Delete 30, 40, 50

    // Check vector after L-value range deletion
    result = result && (VecLen(&vec) == 4);

    // Expected result: [10, 20, 60, 70]
    int expected[] = {10, 20, 60, 70};
    for (u64 i = 0; i < VecLen(&vec); i++) {
        result = result && (VecAt(&vec, i) == expected[i]);
    }

    // Clean up
    VecDeinit(&vec);

    return result;
}

// Test R-value delete range operations
bool test_rvalue_delete_range_operations(void) {
    WriteFmtLn("Testing R-value delete range operations");

    // Create a vector of integers
    typedef Vec(int) IntVec;
    IntVec vec = VecInit(&alloc);

    // Add some data
    int values[] = {10, 20, 30, 40, 50, 60, 70};
    for (int i = 0; i < 7; i++) {
        VecPushBack(&vec, values[i]);
    }

    // Initial length should be 7
    bool result = (VecLen(&vec) == 7);

    // Test R-value delete range operation
    VecDeleteRange(&vec, 2, 3); // Delete 30, 40, 50

    // Check vector after R-value range deletion
    result = result && (VecLen(&vec) == 4);

    // Expected result: [10, 20, 60, 70]
    int expected[] = {10, 20, 60, 70};
    for (u64 i = 0; i < VecLen(&vec); i++) {
        result = result && (VecAt(&vec, i) == expected[i]);
    }

    // Clean up
    VecDeinit(&vec);

    return result;
}

// Test L-value fast delete range operations
bool test_lvalue_fast_delete_range_operations(void) {
    WriteFmtLn("Testing L-value fast delete range operations");

    // Create a vector of integers
    typedef Vec(int) IntVec;
    IntVec vec = VecInit(&alloc);

    // Add some data
    int values[] = {10, 20, 30, 40, 50, 60, 70};
    for (int i = 0; i < 7; i++) {
        VecPushBack(&vec, values[i]);
    }

    // Initial length should be 7
    bool result = (VecLen(&vec) == 7);

    // Print before state
    WriteFmt("Before L-value fast range delete: ");
    for (u64 i = 0; i < VecLen(&vec); i++) {
        WriteFmt("{} ", VecAt(&vec, i));
    }
    WriteFmt("\n");

    // Values that should be deleted (30, 40, 50)
    int valuesToDelete[] = {values[2], values[3], values[4]};

    // Test L-value fast delete range operation
    int fast_start = 2;
    int fast_count = 3;
    VecDeleteRangeFast(&vec, fast_start, fast_count);

    // Print after state
    WriteFmt("After L-value fast range delete: ");
    for (u64 i = 0; i < VecLen(&vec); i++) {
        WriteFmt("{} ", VecAt(&vec, i));
    }
    WriteFmt("\n");

    // Check vector after L-value fast range deletion
    result = result && (VecLen(&vec) == 4);

    // Verify the deleted values are no longer present
    for (int i = 0; i < 3; i++) {
        bool found = false;
        for (u64 j = 0; j < VecLen(&vec); j++) {
            if (VecAt(&vec, j) == valuesToDelete[i]) {
                found = true;
                break;
            }
        }
        result = result && !found;
        if (found) {
            WriteFmtLn("Value {} should be deleted but was found", valuesToDelete[i]);
        }
    }

    // Verify all other values are still present
    int remainingValues[] = {10, 20, 60, 70};
    for (int i = 0; i < 4; i++) {
        bool found = false;
        for (u64 j = 0; j < VecLen(&vec); j++) {
            if (VecAt(&vec, j) == remainingValues[i]) {
                found = true;
                break;
            }
        }
        result = result && found;
        if (!found) {
            WriteFmtLn("Value {} should be present but was not found", remainingValues[i]);
        }
    }

    // Clean up
    VecDeinit(&vec);

    return result;
}

// Test R-value fast delete range operations
bool test_rvalue_fast_delete_range_operations(void) {
    WriteFmtLn("Testing R-value fast delete range operations");

    // Create a vector of integers
    typedef Vec(int) IntVec;
    IntVec vec = VecInit(&alloc);

    // Add some data
    int values[] = {10, 20, 30, 40, 50, 60, 70};
    for (int i = 0; i < 7; i++) {
        VecPushBack(&vec, values[i]);
    }

    // Initial length should be 7
    bool result = (VecLen(&vec) == 7);

    // Print before state
    WriteFmt("Before R-value fast range delete: ");
    for (u64 i = 0; i < VecLen(&vec); i++) {
        WriteFmt("{} ", VecAt(&vec, i));
    }
    WriteFmt("\n");

    // Values that should be deleted (30, 40, 50)
    int valuesToDelete[] = {values[2], values[3], values[4]};

    // Test R-value fast delete range operation
    VecDeleteRangeFast(&vec, 2, 3);

    // Print after state
    WriteFmt("After R-value fast range delete: ");
    for (u64 i = 0; i < VecLen(&vec); i++) {
        WriteFmt("{} ", VecAt(&vec, i));
    }
    WriteFmt("\n");

    // Check vector after R-value fast range deletion
    result = result && (VecLen(&vec) == 4);

    // Verify the deleted values are no longer present
    for (int i = 0; i < 3; i++) {
        bool found = false;
        for (u64 j = 0; j < VecLen(&vec); j++) {
            if (VecAt(&vec, j) == valuesToDelete[i]) {
                found = true;
                break;
            }
        }
        result = result && !found;
        if (found) {
            WriteFmtLn("Value {} should be deleted but was found", valuesToDelete[i]);
        }
    }

    // Verify all other values are still present
    int remainingValues[] = {10, 20, 60, 70};
    for (int i = 0; i < 4; i++) {
        bool found = false;
        for (u64 j = 0; j < VecLen(&vec); j++) {
            if (VecAt(&vec, j) == remainingValues[i]) {
                found = true;
                break;
            }
        }
        result = result && found;
        if (!found) {
            WriteFmtLn("Value {} should be present but was not found", remainingValues[i]);
        }
    }

    // Clean up
    VecDeinit(&vec);

    return result;
}

// ===========================================================================
// fast_remove_range_vec mutation-hardening suite (from Vec.Mutants3)
// ===========================================================================

// 413:23 cxx_gt_to_ge -- removing the entire vector (start 0, count length) is
// documented-valid; under the mutant it LOG_FATALs.
bool test_fast_remove_whole_vector_succeeds(void) {
    WriteFmtLn("Testing fast remove of whole vector (413:23)");

    typedef Vec(int) IntVec;
    IntVec vec = VecInit(&alloc);

    int values[] = {10, 20, 30};
    for (int i = 0; i < 3; i++) {
        VecPushBack(&vec, values[i]);
    }

    // start + count == length: valid, removes everything.
    VecDeleteRangeFast(&vec, 0, 3);

    bool result = (VecLen(&vec) == 0);

    VecDeinit(&vec);
    return result;
}

// 418:72 cxx_mul_to_div -- removed-data copy size `count * stride` -> `count /
// stride` copies (almost) nothing into the caller's buffer.
bool test_fast_remove_copies_out_removed_data(void) {
    WriteFmtLn("Testing fast remove populates removed_data (418:72)");

    typedef Vec(int) IntVec;
    IntVec vec = VecInit(&alloc);

    int values[] = {10, 20, 30, 40};
    for (int i = 0; i < 4; i++) {
        VecPushBack(&vec, values[i]);
    }

    int out[2] = {-1, -1};
    VecRemoveRangeFast(&vec, out, 0, 2);

    bool result = (out[0] == 10) && (out[1] == 20);

    VecDeinit(&vec);
    return result;
}

// 418:74 cxx_replace_scalar_call -- the `vec_aligned_size(...)` stride in the
// removed-data copy is replaced by a constant (0), so 0 bytes are copied.
bool test_fast_remove_copies_out_removed_data_stride(void) {
    WriteFmtLn("Testing fast remove removed_data byte count (418:74)");

    typedef Vec(int) IntVec;
    IntVec vec = VecInit(&alloc);

    int values[] = {11, 22, 33, 44};
    for (int i = 0; i < 4; i++) {
        VecPushBack(&vec, values[i]);
    }

    int out[2] = {-1, -1};
    VecRemoveRangeFast(&vec, out, 0, 2);

    bool result = (out[0] == 11) && (out[1] == 22);

    VecDeinit(&vec);
    return result;
}

// 422:23 cxx_init_const -- copy_deinit loop initializer `s = 0` -> `s = 42`.
bool test_fast_remove_deinits_every_removed_element(void) {
    WriteFmtLn("Testing fast remove deinits every removed element (422:23)");

    reset_deinit_state();

    typedef Vec(int) IntVec;
    IntVec vec = VecInitWithDeepCopy(NULL, counting_deinit, &alloc);

    int values[] = {10, 20, 30};
    for (int i = 0; i < 3; i++) {
        VecPushBack(&vec, values[i]);
    }

    // NULL out-ptr -> copy_deinit invoked on each removed element.
    VecDeleteRangeFast(&vec, 0, 3);

    bool result = (g_deinit_count == 3);

    VecDeinit(&vec);
    return result;
}

// 422:32 cxx_lt_to_ge -- loop guard `s < count` -> `s >= count`.
bool test_fast_remove_deinit_runs_at_least_once(void) {
    WriteFmtLn("Testing fast remove deinit loop runs (422:32 ge)");

    reset_deinit_state();

    typedef Vec(int) IntVec;
    IntVec vec = VecInitWithDeepCopy(NULL, counting_deinit, &alloc);

    int values[] = {10, 20, 30};
    for (int i = 0; i < 3; i++) {
        VecPushBack(&vec, values[i]);
    }

    VecDeleteRangeFast(&vec, 0, 3);

    bool result = (g_deinit_count == 3);

    VecDeinit(&vec);
    return result;
}

// 422:32 cxx_lt_to_le -- loop guard `s < count` -> `s <= count` runs one extra
// iteration. Remove 2 of 4: real code deinits exactly 2.
bool test_fast_remove_deinit_not_run_on_survivor(void) {
    WriteFmtLn("Testing fast remove deinit count is exact (422:32 le)");

    reset_deinit_state();

    typedef Vec(int) IntVec;
    IntVec vec = VecInitWithDeepCopy(NULL, counting_deinit, &alloc);

    int values[] = {10, 20, 30, 40};
    for (int i = 0; i < 4; i++) {
        VecPushBack(&vec, values[i]);
    }

    VecDeleteRangeFast(&vec, 0, 2);

    bool result = (g_deinit_count == 2);

    VecDeinit(&vec);
    return result;
}

// 422:42 cxx_post_inc_to_post_dec -- `s++` -> `s--`.
bool test_fast_remove_deinit_runs_for_each(void) {
    WriteFmtLn("Testing fast remove deinit runs once per element (422:42)");

    reset_deinit_state();

    typedef Vec(int) IntVec;
    IntVec vec = VecInitWithDeepCopy(NULL, counting_deinit, &alloc);

    int values[] = {10, 20, 30};
    for (int i = 0; i < 3; i++) {
        VecPushBack(&vec, values[i]);
    }

    VecDeleteRangeFast(&vec, 0, 3);

    bool result = (g_deinit_count == 3);

    VecDeinit(&vec);
    return result;
}

// 424:29 cxx_replace_scalar_call -- cursor advance `vec_data += stride` has
// stride replaced by a constant (0). Assert the handler sees the distinct
// removed elements in order.
bool test_fast_remove_deinit_walks_each_element(void) {
    WriteFmtLn("Testing fast remove deinit walks each element (424:29)");

    reset_deinit_state();

    typedef Vec(int) IntVec;
    IntVec vec = VecInitWithDeepCopy(NULL, counting_deinit, &alloc);

    int values[] = {10, 20, 30};
    for (int i = 0; i < 3; i++) {
        VecPushBack(&vec, values[i]);
    }

    VecDeleteRangeFast(&vec, 0, 3);

    bool result =
        (g_deinit_nvals == 3) && (g_deinit_vals[0] == 10) && (g_deinit_vals[1] == 20) && (g_deinit_vals[2] == 30);

    VecDeinit(&vec);
    return result;
}

// 449:40 cxx_sub_to_add -- tail-zeroing MemSet targets `length - count` ->
// `length + count`.
bool test_fast_remove_zeroes_vacated_tail_sub(void) {
    WriteFmtLn("Testing fast remove zeroes vacated tail (449:40)");

    typedef Vec(int) IntVec;
    IntVec vec = VecInit(&alloc);

    int values[] = {1, 2, 3, 4, 5};
    for (int i = 0; i < 5; i++) {
        VecPushBack(&vec, values[i]);
    }

    VecDeleteRangeFast(&vec, 0, 2);

    // Length is now 3; the raw slot at index 4 was part of the vacated tail
    // and must read back zero.
    bool result = (VecLen(&vec) == 3) && (*VecPtrAt(&vec, 4) == 0);

    VecDeinit(&vec);
    return result;
}

// 449:70 cxx_mul_to_div -- tail-zeroing MemSet size `count * stride` ->
// `count / stride`.
bool test_fast_remove_zeroes_vacated_tail_stride(void) {
    WriteFmtLn("Testing fast remove tail-zero byte count (449:70)");

    typedef Vec(int) IntVec;
    IntVec vec = VecInit(&alloc);

    int values[] = {1, 2, 3, 4, 5};
    for (int i = 0; i < 5; i++) {
        VecPushBack(&vec, values[i]);
    }

    VecDeleteRangeFast(&vec, 0, 2);

    bool result = (VecLen(&vec) == 3) && (*VecPtrAt(&vec, 4) == 0);

    VecDeinit(&vec);
    return result;
}

// 449:72 cxx_replace_scalar_call -- the `vec_aligned_size(...)` stride in the
// tail-zeroing MemSet size is replaced by the constant 42.
bool test_fast_remove_zeroes_vacated_tail_call(void) {
    WriteFmtLn("Testing fast remove tail-zero stride call (449:72)");

    typedef Vec(int) IntVec;
    IntVec vec = VecInit(&alloc);

    const int N = 100000;
    for (int i = 0; i < N; i++) {
        VecPushBackR(&vec, i);
    }

    // Remove a large window: the vacated-tail MemSet writes count*stride bytes
    // from vec_ptr_at(length-count); forcing the stride to 42 runs the write
    // ~42*count bytes (several MB) past the buffer end, far beyond the mapping.
    const u64 K = 80000;
    VecDeleteRangeFast(&vec, 0, K);

    bool result = (VecLen(&vec) == (u64)N - K);

    VecDeinit(&vec);
    return result;
}

// ===========================================================================
// remove_range_vec mutation-hardening suite (from Vec.Mutants4)
// ===========================================================================

// 380:23 cxx_init_const  (`size s = 0` -> `s = 42`): the deinit loop never
// runs, so removed elements are dropped without releasing their resources.
bool test_remove_deinit_init_zero(void) {
    WriteFmt("Testing remove copy_deinit loop starts at zero\n");

    reset_ledger();
    ElemVec vec = make_elem_vec(5);           // ids 0..4

    VecRemoveRange(&vec, (Elem *)NULL, 0, 2); // remove ids {0,1}

    bool result = (g_freed[0] == 1 && g_freed[1] == 1);

    VecDeinit(&vec);
    return result;
}

// 380:32 cxx_lt_to_ge (`s < count` -> `s >= count`): false on entry, loop
// body never runs.
bool test_remove_deinit_runs_at_all(void) {
    WriteFmt("Testing remove copy_deinit loop runs\n");

    reset_ledger();
    ElemVec vec = make_elem_vec(5);           // ids 0..4

    VecRemoveRange(&vec, (Elem *)NULL, 0, 2); // remove ids {0,1}

    bool result = (g_freed[0] == 1 && g_freed[1] == 1);

    VecDeinit(&vec);
    return result;
}

// 380:32 cxx_lt_to_le (`s < count` -> `s <= count`): one extra iteration
// deinitializes a SURVIVING element.
bool test_remove_deinit_no_overrun(void) {
    WriteFmt("Testing remove copy_deinit does not overrun window\n");

    reset_ledger();
    ElemVec vec = make_elem_vec(5);           // ids 0..4

    VecRemoveRange(&vec, (Elem *)NULL, 0, 2); // remove ids {0,1}; id 2 survives

    // Survivor id 2 must be untouched by the removal.
    bool result = (g_freed[2] == 0 && g_freed[0] == 1 && g_freed[1] == 1);

    VecDeinit(&vec);
    return result;
}

// 380:42 cxx_post_inc_to_post_dec (`s++` -> `s--`).
bool test_remove_deinit_all_three(void) {
    WriteFmt("Testing remove copy_deinit covers every removed element\n");

    reset_ledger();
    ElemVec vec = make_elem_vec(5);           // ids 0..4

    VecRemoveRange(&vec, (Elem *)NULL, 0, 3); // remove ids {0,1,2}

    bool result = (g_freed[0] == 1 && g_freed[1] == 1 && g_freed[2] == 1);

    VecDeinit(&vec);
    return result;
}

// 382:29 cxx_replace_scalar_call (cursor stride replaced by constant 0).
bool test_remove_deinit_advances_cursor(void) {
    WriteFmt("Testing remove copy_deinit advances per element\n");

    reset_ledger();
    ElemVec vec = make_elem_vec(5);           // ids 0..4

    VecRemoveRange(&vec, (Elem *)NULL, 1, 3); // remove ids {1,2,3}

    bool result = (g_freed[1] == 1 && g_freed[2] == 1 && g_freed[3] == 1);

    VecDeinit(&vec);
    return result;
}

// 395:22 cxx_sub_to_add (`length - start - count` -> `length + start - count`).
bool test_remove_compaction_len_first_term(void) {
    WriteFmt("Testing remove compaction length (first term, large start)\n");

    typedef Vec(int) IntVec;
    IntVec vec = VecInit(&alloc);

    const int N = 40002;
    for (int i = 0; i < N; i++) {
        VecPushBackR(&vec, i);
    }

    // start large, count small: real move = N - start - 2; mutant move adds
    // 2*start ~= 40000 elements (~160 KB) of over-read/over-write past the end.
    VecDeleteRange(&vec, 20000, 2);

    bool result = (VecLen(&vec) == (u64)(N - 2)) && (VecAt(&vec, 0) == 0) && (VecAt(&vec, 19999) == 19999) &&
                  (VecAt(&vec, 20000) == 20002);

    VecDeinit(&vec);
    return result;
}

// 395:30 cxx_sub_to_add (`length - start - count` -> `length - start + count`).
bool test_remove_compaction_len_second_term(void) {
    WriteFmt("Testing remove compaction length (second term, large count)\n");

    typedef Vec(int) IntVec;
    IntVec vec = VecInit(&alloc);

    const int N = 40000;
    for (int i = 0; i < N; i++) {
        VecPushBackR(&vec, i);
    }

    // start 0, count large: real move = N - K; mutant move = N + K, adding
    // 2*K ~= 40000 elements (~160 KB) of over-read/over-write past the end.
    const u64 K = 20000;
    VecDeleteRange(&vec, 0, K);

    bool result = (VecLen(&vec) == (u64)N - K) && (VecAt(&vec, 0) == (int)K) && (VecAt(&vec, 19999) == (int)(N - 1));

    VecDeinit(&vec);
    return result;
}

// 395:41 cxx_replace_scalar_call (compaction MemMove stride replaced by 42).
bool test_remove_compaction_stride(void) {
    WriteFmt("Testing remove compaction stride (large over-move)\n");

    typedef Vec(int) IntVec;
    IntVec vec = VecInit(&alloc);

    const int N = 40000;
    for (int i = 0; i < N; i++) {
        VecPushBackR(&vec, i);
    }

    VecDeleteRange(&vec, 1, 2); // real move ~ (N-3)*4 bytes; mutant ~ (N-3)*42 (~1.5 MB)

    bool result = (VecLen(&vec) == (u64)(N - 2)) && (VecAt(&vec, 0) == 0) && (VecAt(&vec, 1) == 3) &&
                  (VecAt(&vec, (u64)(N - 3)) == (int)(N - 1));

    VecDeinit(&vec);
    return result;
}

// 397:41 cxx_sub_to_add (tail-clear `vec->length - count` -> `length + count`).
bool test_remove_tail_clear_dest(void) {
    WriteFmt("Testing remove tail-clear destination\n");

    typedef Vec(int) IntVec;
    IntVec vec = VecInit(&alloc);
    for (int i = 0; i < 10; i++) {
        VecPushBackR(&vec, i);  // [0..9]
    }

    VecDeleteRange(&vec, 3, 2); // length becomes 8

    // Dead slot 9 (previously a duplicate of 9) must be zeroed.
    bool result = (VecLen(&vec) == 8 && *VecPtrAt(&vec, 9) == 0);

    VecDeinit(&vec);
    return result;
}

// 397:72 cxx_mul_to_div (`count * stride` -> `count / stride`).
bool test_remove_tail_clear_len(void) {
    WriteFmt("Testing remove tail-clear length\n");

    typedef Vec(int) IntVec;
    IntVec vec = VecInit(&alloc);
    for (int i = 0; i < 10; i++) {
        VecPushBackR(&vec, i);  // [0..9]
    }

    VecDeleteRange(&vec, 3, 2); // length becomes 8

    // Dead slot 9 must be zeroed, not left as a stale duplicate.
    bool result = (VecLen(&vec) == 8 && *VecPtrAt(&vec, 9) == 0);

    VecDeinit(&vec);
    return result;
}

// 397:74 cxx_replace_scalar_call (tail-clear stride replaced by constant 42).
bool test_remove_tail_clear_stride(void) {
    WriteFmt("Testing remove tail-clear stride (large over-write)\n");

    typedef Vec(int) IntVec;
    IntVec vec = VecInit(&alloc);

    const int N = 40000;
    for (int i = 0; i < N; i++) {
        VecPushBackR(&vec, i);
    }

    // Remove a large window: the tail-clear MemSet targets [length-count,
    // length) and, with the stride forced to 42, writes ~42*count bytes (~1.2 MB)
    // from vec_ptr_at(length-count), running well past the end of the buffer.
    const u64 K = 30000;
    VecDeleteRange(&vec, 0, K); // length becomes N-K

    bool result = (VecLen(&vec) == (u64)N - K) && (VecAt(&vec, 0) == (int)K);

    VecDeinit(&vec);
    return result;
}

// ===========================================================================
// remove compaction size mutation-hardening (from Vec.Blind)
// ===========================================================================

// ---- 395:22 cxx_sub_to_add (length - start -> length + start) -------------
// remove_range_vec compacts the surviving tail with a MemMove of
// `(vec->length - start - count) * aligned_size` bytes. The element count is a
// pure size argument: every element inside the new logical length is correct
// regardless of an over-large move (it merely copies extra), and the
// always-zeroed capacity tail makes any in-bounds surplus read back as zero, so
// the corruption is NOT observable as logical data. The only reachable effect
// of `vec->length + start` is a gross over-read/over-write: the move balloons to
// `length + start - count` elements. Sizing the buffer to exactly `length + 1`
// (via VecTryReduceSpace) and removing the final element (start == length-1,
// count == 1 -> real move size 0) makes the mutant relocate ~2*length elements
// from a `length+1`-element allocation, running far past the buffer and
// trapping -- the same overrun-fault detection the behavioural displacement
// over-copy test relies on. Real code moves 0 bytes and returns cleanly.
bool test_remove_compaction_size_first_term(void) {
    WriteFmt("Testing remove compaction size (length - start term)\n");

    typedef Vec(u64) U64Vec;
    U64Vec vec = VecInit(&alloc);

    // Large, tightly-allocated buffer: capacity == length so the only slack is
    // the single sentinel slot. A few-element overrun would hide in heap slack;
    // a ~2*length-element overrun runs far past the allocation and faults.
    const size N = 100000;
    for (u64 i = 0; i < N; i++) {
        VecPushBackR(&vec, i + 1); // all non-zero
    }
    VecTryReduceSpace(&vec);       // capacity := length, buffer := length + 1

    size old_len = VecLen(&vec);

    // Remove the final element. Real move size = length - (length-1) - 1 = 0.
    // Mutant size = length + (length-1) - 1 = 2*length - 2 elements -> overrun.
    VecDeleteRange(&vec, old_len - 1, 1);

    bool result = (VecLen(&vec) == old_len - 1);
    // Surviving prefix is intact (the real path moved nothing).
    result = result && (VecAt(&vec, 0) == 1);
    result = result && (VecAt(&vec, old_len - 2) == (u64)(old_len - 1));

    VecDeinit(&vec);
    return result;
}

// ---- 395:30 cxx_sub_to_add ((..) - count -> (..) + count) -----------------
// Same MemMove element count; flipping the second `- count` to `+ count` blows
// the move up to `length - start + count` elements. As above the in-bounds
// surplus is unobservable (zeroed capacity tail), so detection is via the gross
// over-read/over-write. Removing `length - 1` elements from the front
// (start == 0, count == length-1 -> real move size 1) makes the mutant relocate
// `length - 0 + (length-1)` = 2*length - 1 elements from a `length+1`-element
// buffer -> far overrun and trap. Real code moves exactly the one surviving
// tail element and compacts correctly.
bool test_remove_compaction_size_second_term(void) {
    WriteFmt("Testing remove compaction size (- count term)\n");

    typedef Vec(u64) U64Vec;
    U64Vec vec = VecInit(&alloc);

    const size N = 100000;
    for (u64 i = 0; i < N; i++) {
        VecPushBackR(&vec, i + 1); // all non-zero
    }
    VecTryReduceSpace(&vec);       // capacity := length, buffer := length + 1

    size old_len = VecLen(&vec);

    // Remove all but the last element from the front. Real move size =
    // length - 0 - (length-1) = 1. Mutant size = length - 0 + (length-1) =
    // 2*length - 1 elements -> overrun far past the buffer.
    VecDeleteRange(&vec, 0, old_len - 1);

    bool result = (VecLen(&vec) == 1);
    // The single survivor is the original last element.
    result = result && (VecAt(&vec, 0) == (u64)old_len);

    VecDeinit(&vec);
    return result;
}

// Main function that runs all tests
int main(void) {
    // Array of normal test functions
    TestFunction normal_tests[] = {
        test_vec_pop_back,
        test_vec_pop_front,
        test_vec_delete,
        test_vec_delete_fast,
        test_vec_delete_range,
        test_vec_delete_range_fast,
        test_vec_delete_last,
        test_lvalue_delete_operations,
        test_rvalue_delete_operations,
        test_lvalue_fast_delete_operations,
        test_rvalue_fast_delete_operations,
        test_lvalue_delete_range_operations,
        test_rvalue_delete_range_operations,
        test_lvalue_fast_delete_range_operations,
        test_rvalue_fast_delete_range_operations,
        // fast_remove_range_vec mutation-hardening (Vec.Mutants3)
        test_fast_remove_whole_vector_succeeds,
        test_fast_remove_copies_out_removed_data,
        test_fast_remove_copies_out_removed_data_stride,
        test_fast_remove_deinits_every_removed_element,
        test_fast_remove_deinit_runs_at_least_once,
        test_fast_remove_deinit_not_run_on_survivor,
        test_fast_remove_deinit_runs_for_each,
        test_fast_remove_deinit_walks_each_element,
        test_fast_remove_zeroes_vacated_tail_sub,
        test_fast_remove_zeroes_vacated_tail_stride,
        test_fast_remove_zeroes_vacated_tail_call,
        // remove_range_vec mutation-hardening (Vec.Mutants4)
        test_remove_deinit_init_zero,
        test_remove_deinit_runs_at_all,
        test_remove_deinit_no_overrun,
        test_remove_deinit_all_three,
        test_remove_deinit_advances_cursor,
        test_remove_compaction_len_first_term,
        test_remove_compaction_len_second_term,
        test_remove_compaction_stride,
        test_remove_tail_clear_dest,
        test_remove_tail_clear_len,
        test_remove_tail_clear_stride,
        // remove compaction size (Vec.Blind)
        test_remove_compaction_size_first_term,
        test_remove_compaction_size_second_term
    };

    int normal_count = sizeof(normal_tests) / sizeof(normal_tests[0]);

    alloc  = DefaultAllocatorInit();
    int rc = run_test_suite(normal_tests, normal_count, NULL, 0, "Vec.Remove");
    DefaultAllocatorDeinit(&alloc);
    return rc;
}
