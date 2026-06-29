#include <Misra/Std/Allocator/Debug.h>
#include <Misra/Std/Allocator/Default.h>
#include <Misra/Std/Container/Vec.h>
#include <Misra/Std/Log.h>
#include <Misra/Types.h> // For LVAL macro

// Include test utilities
#include "../Util/TestRunner.h"

// Function prototypes
bool test_vec_push_back(void);
bool test_vec_push_front(void);
bool test_vec_insert(void);
bool test_vec_push_back_arr(void);
bool test_vec_push_front_arr(void);
bool test_vec_push_arr(void);
bool test_vec_insert_range(void);
bool test_vec_merge(void);
bool test_vec_init_clone_inherits_allocator_config(void);
bool test_lvalue_rvalue_operations(void);
bool test_lvalue_zero_on_take_after_insertion(void);
bool test_vec_insert_range_fast_overflowing_tail(void);

// ---- insert_range_fast_into_vec mutation-hardening prototypes ----------
bool test_fast_overflow_count_returns_false(void);
bool test_grow_skip_underflow_predicate(void);
bool test_grow_predicate_boundary_equal(void);
bool test_grow_predicate_inverted(void);
bool test_grow_reserve_result_honored(void);
bool test_displacement_move_size(void);
bool test_displacement_move_overcopy(void);
bool test_prezero_target_index(void);
bool test_copy_init_dest_index(void);
bool test_copy_init_source_stride(void);
bool test_first_element_failure_no_deinit(void);
bool test_rollback_start_index_zero(void);
bool test_rollback_guard_first_fail(void);
bool test_rollback_guard_no_extra_deinit(void);
bool test_rollback_counter_increments(void);
bool test_rollback_deinit_target(void);
bool test_inserted_counter_increments(void);
bool test_rollback_restore_source_first_term(void);
bool test_rollback_restore_source_second_term(void);
bool test_rollback_restore_size(void);

// ---- insert_range_into_vec mutation-hardening prototypes ---------------
bool test_insert_overflow_count_returns_false(void);
bool test_aligned_size_const_preserves_originals(void);
bool test_aligned_size_call_preserves_originals(void);
bool test_shift_right_size_middle_insert(void);
bool test_rollback_first_item_no_deinit(void);
bool test_rollback_loop_start_index(void);
bool test_rollback_cond_runs_for_inited(void);
bool test_rollback_cond_skips_failed_slot(void);
bool test_rollback_step_increments(void);
bool test_rollback_deinit_target_slots(void);
bool test_rollback_inserted_count_step(void);
bool test_rollback_restore_middle_guard(void);
bool test_rollback_restore_shiftback_source(void);
bool test_rollback_restore_shiftback_size_sub(void);
bool test_rollback_restore_shiftback_size_div(void);

// ---- vec_insert_range_l / vec_insert_one_l prototypes -----------------
bool test_insert_range_l_preserve_inserts_all(void);
bool test_insert_range_l_fast_inserts_all(void);
bool test_insert_one_l_inserts_value(void);

// ---- L-insert copy_init failure reporting prototypes ------------------
bool test_insert_range_fast_l_reports_failure(void);
bool test_insert_one_l_reports_failure(void);

// Test VecPushBack function
static DefaultAllocator alloc;

bool test_vec_push_back(void) {
    WriteFmt("Testing VecPushBack\n");

    // Create a vector of integers
    typedef Vec(int) IntVec;
    IntVec vec = VecInit(&alloc);

    // Push some elements to the back
    int values[] = {10, 20, 30, 40, 50};
    for (int i = 0; i < 5; i++) {
        VecPushBackR(&vec, values[i]);
    }

    // Check length
    bool result = (VecLen(&vec) == 5);

    // Check elements in order
    for (size i = 0; i < VecLen(&vec); i++) {
        result = result && (VecAt(&vec, i) == values[i]);
    }

    // Clean up
    VecDeinit(&vec);

    return result;
}

// Test VecPushFront function
bool test_vec_push_front(void) {
    WriteFmt("Testing VecPushFront\n");

    // Create a vector of integers
    typedef Vec(int) IntVec;
    IntVec vec = VecInit(&alloc);

    // Push some elements to the front
    int values[] = {10, 20, 30, 40, 50};
    for (int i = 0; i < 5; i++) {
        VecPushFrontR(&vec, values[i]);
    }

    // Check length
    bool result = (VecLen(&vec) == 5);

    // Check elements in reverse order (since we pushed to front)
    for (size i = 0; i < VecLen(&vec); i++) {
        result = result && (VecAt(&vec, i) == values[4 - i]);
    }

    // Clean up
    VecDeinit(&vec);

    return result;
}

// Test VecInsert function
bool test_vec_insert(void) {
    WriteFmt("Testing VecInsert\n");

    // Create a vector of integers
    typedef Vec(int) IntVec;
    IntVec vec = VecInit(&alloc);

    // Insert at index 0 (empty vector)
    VecInsertR(&vec, 10, 0);

    // Check first element
    bool result = (VecLen(&vec) == 1 && VecAt(&vec, 0) == 10);

    // Insert at the end
    VecInsertR(&vec, 30, 1);

    // Check elements
    result = result && (VecLen(&vec) == 2 && VecAt(&vec, 0) == 10 && VecAt(&vec, 1) == 30);

    // Insert in the middle
    VecInsertR(&vec, 20, 1);

    // Check all elements
    result = result && (VecLen(&vec) == 3);
    result = result && (VecAt(&vec, 0) == 10);
    result = result && (VecAt(&vec, 1) == 20);
    result = result && (VecAt(&vec, 2) == 30);

    // Clean up
    VecDeinit(&vec);

    return result;
}

// Test VecPushBackArr function
bool test_vec_push_back_arr(void) {
    WriteFmt("Testing VecPushBackArr\n");

    // Create a vector of integers
    typedef Vec(int) IntVec;
    IntVec vec = VecInit(&alloc);

    // Push an array to the back
    int values[] = {10, 20, 30, 40, 50};
    VecPushBackArrR(&vec, values, 5);

    // Check length
    bool result = (VecLen(&vec) == 5);

    // Check elements in order
    for (size i = 0; i < VecLen(&vec); i++) {
        result = result && (VecAt(&vec, i) == values[i]);
    }

    // Push another array to the back
    int more_values[] = {60, 70, 80};
    VecPushBackArrR(&vec, more_values, 3);

    // Check length
    result = result && (VecLen(&vec) == 8);

    // Check all elements
    for (size i = 0; i < 5; i++) {
        result = result && (VecAt(&vec, i) == values[i]);
    }
    for (size i = 0; i < 3; i++) {
        result = result && (VecAt(&vec, i + 5) == more_values[i]);
    }

    // Clean up
    VecDeinit(&vec);

    return result;
}

// Test VecPushFrontArr function
bool test_vec_push_front_arr(void) {
    WriteFmt("Testing VecPushFrontArr\n");

    // Create a vector of integers
    typedef Vec(int) IntVec;
    IntVec vec = VecInit(&alloc);

    // Push an array to the front of empty vector
    int values[] = {10, 20, 30, 40, 50};
    VecPushFrontArrR(&vec, values, 5);

    // Check length
    bool result = (VecLen(&vec) == 5);

    // Check elements in order
    for (size i = 0; i < VecLen(&vec); i++) {
        result = result && (VecAt(&vec, i) == values[i]);
    }

    // Push another array to the front
    int more_values[] = {60, 70, 80};
    VecPushFrontArrR(&vec, more_values, 3);

    // Check length
    result = result && (VecLen(&vec) == 8);

    // Check all elements
    for (size i = 0; i < 3; i++) {
        result = result && (VecAt(&vec, i) == more_values[i]);
    }
    for (size i = 0; i < 5; i++) {
        result = result && (VecAt(&vec, i + 3) == values[i]);
    }

    // Clean up
    VecDeinit(&vec);

    return result;
}

// Test VecInsertRange function for inserting at a specific index
bool test_vec_push_arr(void) {
    WriteFmt("Testing VecInsertRange at specific index\n");

    // Create a vector of integers
    typedef Vec(int) IntVec;
    IntVec vec = VecInit(&alloc);

    // Push some elements first
    VecPushBackR(&vec, 10);
    VecPushBackR(&vec, 20);

    // Push an array at a specific index
    int values[] = {30, 40, 50};
    VecInsertRangeR(&vec, values, 1, 3);

    // Check length
    bool result = (VecLen(&vec) == 5);

    // Expected result: [10, 30, 40, 50, 20]
    int expected[] = {10, 30, 40, 50, 20};

    // Check all elements
    for (size i = 0; i < VecLen(&vec); i++) {
        result = result && (VecAt(&vec, i) == expected[i]);
    }

    // Clean up
    VecDeinit(&vec);

    return result;
}

// Test VecInsertRange function for inserting from another vector
bool test_vec_insert_range(void) {
    WriteFmt("Testing VecInsertRange from another vector\n");

    // Create a vector of integers
    typedef Vec(int) IntVec;
    IntVec vec = VecInit(&alloc);

    // Add some initial elements
    int initial[] = {10, 20, 30};
    VecPushBackArrR(&vec, initial, 3);

    // Create another vector with elements to insert
    IntVec src          = VecInit(&alloc);
    int    src_values[] = {40, 50, 60};
    VecPushBackArrR(&src, src_values, 3);

    // Insert range in the middle
    VecInsertRangeR(&vec, VecBegin(&src), 1, VecLen(&src));

    // Check length
    bool result = (VecLen(&vec) == 6);

    // Expected result: [10, 40, 50, 60, 20, 30]
    int expected[] = {10, 40, 50, 60, 20, 30};

    // Check all elements
    for (size i = 0; i < VecLen(&vec); i++) {
        result = result && (VecAt(&vec, i) == expected[i]);
    }

    // Clean up
    VecDeinit(&vec);
    VecDeinit(&src);

    return result;
}

// Test VecMerge function
bool test_vec_merge(void) {
    WriteFmt("Testing VecMerge\n");

    // Create a vector of integers
    typedef Vec(int) IntVec;
    IntVec vec1 = VecInit(&alloc);

    // Add some elements to first vector
    int values1[] = {10, 20, 30};
    VecPushBackArrR(&vec1, values1, 3);

    // Create second vector
    IntVec vec2 = VecInit(&alloc);

    // Add some elements to second vector
    int values2[] = {40, 50, 60};
    VecPushBackArrR(&vec2, values2, 3);

    // Merge vec2 into vec1
    VecMergeR(&vec1, &vec2);

    // Check lengths
    bool result = (VecLen(&vec1) == 6);
    result      = result && (VecLen(&vec2) == 3); // VecMergeR doesn't modify source vector

    // Expected result in vec1: [10, 20, 30, 40, 50, 60]
    int expected[] = {10, 20, 30, 40, 50, 60};

    // Check all elements in vec1
    for (size i = 0; i < VecLen(&vec1); i++) {
        result = result && (VecAt(&vec1, i) == expected[i]);
    }

    // Clean up
    VecDeinit(&vec1);
    VecDeinit(&vec2);

    return result;
}

// Test that a manually-cloned Vec shares its allocator configuration.
// (Originally exercised `VecInitClone`, which is currently unavailable; we
// emulate the clone by reusing the source's allocator pointer and copying
// the elements manually.)
bool test_vec_init_clone_inherits_allocator_config(void) {
    WriteFmt("Testing manual clone allocator inheritance\n");

    typedef Vec(int) IntVec;

    HeapAllocator local_heap = HeapAllocatorInit();
    // intentional bypass: no public setter on `Allocator` for effort /
    // retry_limit -- pre-seeded directly so the inheritance path below
    // can be observed end-to-end.
    local_heap.base.effort      = ALLOCATOR_EFFORT_RETRY_FALLBACK;
    local_heap.base.retry_limit = 11;

    IntVec src      = VecInit(&local_heap);
    int    values[] = {10, 20, 30};
    VecPushBackArrR(&src, values, 3);

    // Build dst on the SAME allocator as src, then clone the data.
    // VecPushBackArrR treats the source as a flat C array of elements,
    // so alignment must be 1 (default) for src.data to be a contiguous
    // int[]. Stronger alignment is exercised separately - it's not what
    // this test is asserting.
    IntVec dst = VecInit(VecAllocator(&src));
    // intentional bypass: testing hook propagation; no public VecSetCopyHooks mutator exists
    dst.copy_init   = src.copy_init;
    dst.copy_deinit = src.copy_deinit;
    bool cloned     = VecPushBackArrR(&dst, VecBegin(&src), VecLen(&src));

    bool allocator_matches = VecAllocator(&dst) == VecAllocator(&src);

    bool result = cloned && VecCopyInit(&dst) == VecCopyInit(&src) && VecCopyDeinit(&dst) == VecCopyDeinit(&src) &&
                  VecAllocator(&dst)->effort == ALLOCATOR_EFFORT_RETRY_FALLBACK &&
                  VecAllocator(&dst)->retry_limit == 11 && allocator_matches && VecLen(&src) == 3 &&
                  VecAt(&src, 0) == 10 && VecAt(&src, 1) == 20 && VecAt(&src, 2) == 30 && VecLen(&dst) == 3 &&
                  VecAt(&dst, 0) == 10 && VecAt(&dst, 1) == 20 && VecAt(&dst, 2) == 30;

    VecDeinit(&src);
    VecDeinit(&dst);
    HeapAllocatorDeinit(&local_heap);
    return result;
}

// Test L-value and R-value operations
bool test_lvalue_rvalue_operations(void) {
    WriteFmt("Testing L-value and R-value operations\n");

    // Create a vector of integers
    typedef Vec(int) IntVec;
    IntVec vec = VecInit(&alloc);

    // Test R-value insert operations
    VecPushBackR(&vec, LVAL(42));

    // Check that the element was added
    bool result = (VecLen(&vec) == 1 && VecAt(&vec, 0) == 42);

    // Test L-value insert operations
    int l_value = 100;
    VecPushBackL(&vec, l_value);

    // Check that the element was added
    result = result && (VecLen(&vec) == 2 && VecAt(&vec, 1) == 100);

    // Test R-value insert at index
    VecInsertR(&vec, LVAL(50), 1);

    // Check that the element was inserted
    result = result && (VecLen(&vec) == 3);
    result = result && (VecAt(&vec, 0) == 42);
    result = result && (VecAt(&vec, 1) == 50);
    result = result && (VecAt(&vec, 2) == 100);

    // Test L-value insert at index
    int insert_value = 75;
    VecInsertL(&vec, insert_value, 2);

    // Check that the element was inserted
    result = result && (VecLen(&vec) == 4);
    result = result && (VecAt(&vec, 0) == 42);
    result = result && (VecAt(&vec, 1) == 50);
    result = result && (VecAt(&vec, 2) == 75);
    result = result && (VecAt(&vec, 3) == 100);

    // Test R-value fast insert
    VecInsertFastR(&vec, LVAL(60), 1);

    // Check that the element was inserted
    result = result && (VecLen(&vec) == 5);
    result = result && (VecAt(&vec, 1) == 60);

    // Test L-value fast insert
    int fast_value = 80;
    VecInsertFastL(&vec, fast_value, 3);

    // Check that the element was inserted
    result = result && (VecLen(&vec) == 6);
    result = result && (VecAt(&vec, 3) == 80);

    // Test array operations with L-values and R-values
    int arr[] = {200, 300, 400};

    // R-value array operations
    VecPushBackArrR(&vec, arr, 3);

    // Check that the elements were added
    result = result && (VecLen(&vec) == 9);
    result = result && (VecAt(&vec, 6) == 200);
    result = result && (VecAt(&vec, 7) == 300);
    result = result && (VecAt(&vec, 8) == 400);

    // L-value array operations
    VecPushFrontArrL(&vec, arr, 3);

    // Check that the elements were added
    result = result && (VecLen(&vec) == 12);
    result = result && (VecAt(&vec, 0) == 200);
    result = result && (VecAt(&vec, 1) == 300);
    result = result && (VecAt(&vec, 2) == 400);

    // Clean up
    VecDeinit(&vec);

    return result;
}

// Test that L-value insertions properly zero out values after insertion
bool test_lvalue_zero_on_take_after_insertion(void) {
    WriteFmt("Testing L-value zero-on-take after insertion\n");

    // Create a vector of integers without copy_init
    typedef Vec(int) IntVec;
    IntVec vec = VecInit(&alloc);

    // Test VecPushBackL
    int val1 = 10;
    VecPushBackL(&vec, val1);
    bool result = (val1 == 0); // Should be zeroed

    // Test VecPushFrontL
    int val2 = 20;
    VecPushFrontL(&vec, val2);
    result = result && (val2 == 0); // Should be zeroed

    // Test VecInsertL
    int val3 = 30;
    VecInsertL(&vec, val3, 1);
    result = result && (val3 == 0); // Should be zeroed

    // Test array operations
    int arr[] = {40, 50, 60};
    VecPushBackArrL(&vec, arr, 3);

    // Check that array elements are zeroed
    result = result && (arr[0] == 0);
    result = result && (arr[1] == 0);
    result = result && (arr[2] == 0);

    // Test VecInsertFastL
    int val4 = 70;
    VecInsertFastL(&vec, val4, 2);
    result = result && (val4 == 0); // Should be zeroed

    // Test VecInsertRangeL
    int range[] = {80, 90, 100};
    VecInsertRangeL(&vec, range, 1, 3);

    // Check that array elements are zeroed
    result = result && (range[0] == 0);
    result = result && (range[1] == 0);
    result = result && (range[2] == 0);

    // Test VecInsertRangeFastL
    int fast_range[] = {110, 120, 130};
    VecInsertRangeFastL(&vec, fast_range, 3, 3);

    // Check that array elements are zeroed
    result = result && (fast_range[0] == 0);
    result = result && (fast_range[1] == 0);
    result = result && (fast_range[2] == 0);

    // Test VecMergeL
    IntVec vec2         = VecInit(&alloc);
    int    merge_vals[] = {140, 150, 160};
    for (int i = 0; i < 3; i++) {
        VecPushBackR(&vec2, merge_vals[i]);
    }

    // Merge with L-value semantics
    VecMergeL(&vec, &vec2);

    // Check that the source vector is cleared
    result = result && (VecLen(&vec2) == 0);
    result = result && (VecBegin(&vec2) == NULL);

    // Clean up
    VecDeinit(&vec);
    VecDeinit(&vec2);

    return result;
}

// Fast range insert where count > (length - idx): the displacement block must
// be sized to the live tail (length - idx), not to count, otherwise the
// implementation reads past the live region and loses the original tail
// elements. Verifies the new items occupy [idx, idx+count), the displaced
// originals all show up in the new tail (order not guaranteed), and the
// untouched prefix [0, idx) is intact.
bool test_vec_insert_range_fast_overflowing_tail(void) {
    WriteFmt("Testing VecInsertRangeFast with count > (length - idx)\n");

    typedef Vec(int) IntVec;
    IntVec vec = VecInit(&alloc);

    int  originals[] = {10, 20, 30, 40, 50, 60, 70, 80};
    size orig_count  = sizeof(originals) / sizeof(originals[0]);
    for (size i = 0; i < orig_count; i++) {
        VecPushBackR(&vec, originals[i]);
    }

    size idx         = 5;
    int  new_items[] = {100, 200, 300, 400, 500, 600, 700, 800, 900, 1000};
    size new_count   = sizeof(new_items) / sizeof(new_items[0]);

    bool result = VecInsertRangeFastR(&vec, new_items, idx, new_count);
    result      = result && (VecLen(&vec) == orig_count + new_count);

    for (size i = 0; i < idx; i++) {
        result = result && (VecAt(&vec, i) == originals[i]);
    }

    for (size i = 0; i < new_count; i++) {
        result = result && (VecAt(&vec, idx + i) == new_items[i]);
    }

    // Displaced originals [idx, orig_count) must all appear somewhere in
    // [idx + new_count, new length). Order is intentionally unspecified.
    size tail_start = idx + new_count;
    size tail_end   = VecLen(&vec);
    result          = result && (tail_end - tail_start == orig_count - idx);
    for (size i = idx; i < orig_count; i++) {
        bool found = false;
        for (size j = tail_start; j < tail_end; j++) {
            if (VecAt(&vec, j) == originals[i]) {
                found = true;
                break;
            }
        }
        result = result && found;
    }

    VecDeinit(&vec);
    return result;
}

// ===========================================================================
// insert_range_fast_into_vec mutation-hardening suite (from Vec.Mutants1)
// ===========================================================================
//
// A payload whose element stride is > 1 byte (sizeof(Elem) == 8 under the
// default allocator, alignment 1) so that mutants which corrupt the
// `aligned_size` / `i * item_size` arithmetic become observable.
typedef struct {
    int  value;
    bool live; // true only after a successful copy_init; lets copy_deinit
               // detect being run on an un-inited (zeroed) slot.
} Elem;

typedef Vec(Elem) ElemVec;

// ---- copy_init / copy_deinit fixture instrumentation --------------------

// Number of copy_init calls observed (1-based) since the last reset.
static size g_init_calls = 0;
// If non-zero, copy_init fails on exactly the g_fail_at-th call (1-based).
static size g_fail_at = 0;

// copy_deinit accounting.
static size g_deinit_count         = 0;
static int  g_deinit_values[256]   = {0};
static bool g_deinit_was_live[256] = {0};

static void reset_hook_counters(void) {
    g_init_calls   = 0;
    g_deinit_count = 0;
    for (size i = 0; i < 256; i++) {
        g_deinit_values[i]   = 0;
        g_deinit_was_live[i] = false;
    }
}

static bool elem_copy_init(void *dst_, const void *src_, const Allocator *a) {
    (void)a;
    g_init_calls++;
    if (g_fail_at != 0 && g_init_calls == g_fail_at) {
        return false;
    }
    Elem       *dst = (Elem *)dst_;
    const Elem *src = (const Elem *)src_;
    dst->value      = src->value;
    dst->live       = true;
    return true;
}

static void elem_copy_deinit(void *p_, const Allocator *a) {
    (void)a;
    Elem *p = (Elem *)p_;
    if (g_deinit_count < 256) {
        g_deinit_values[g_deinit_count]   = p->value;
        g_deinit_was_live[g_deinit_count] = p->live;
    }
    g_deinit_count++;
    p->value = 0;
    p->live  = false;
}

static bool deinit_log_contains(int value) {
    size n = g_deinit_count < 256 ? g_deinit_count : 256;
    for (size i = 0; i < n; i++) {
        if (g_deinit_values[i] == value) {
            return true;
        }
    }
    return false;
}

// Build a copy_init-enabled vec pre-filled with `count` live Elems carrying
// values seed, seed+1, ... All copy_init calls during prefill succeed.
static ElemVec make_filled_elem_vec(int seed, size count) {
    ElemVec vec  = VecInitWithDeepCopy(elem_copy_init, elem_copy_deinit, &alloc);
    g_fail_at    = 0;
    g_init_calls = 0;
    for (size i = 0; i < count; i++) {
        Elem e = {.value = seed + (int)i, .live = false};
        VecPushBackR(&vec, e);
    }
    return vec;
}

// ---- 300:15 cxx_gt_to_ge ------------------------------------------------
// Overflow guard `count > (size)-1 - length` must let count == SIZE_MAX with
// length 0 through (it then fails gracefully in reserve), NOT abort. The
// mutant `>=` turns the boundary into a LOG_FATAL.
bool test_fast_overflow_count_returns_false(void) {
    WriteFmt("Testing fast-insert SIZE_MAX count returns false (no abort)\n");

    ElemVec vec = VecInit(&alloc);

    Elem src[1] = {
        {.value = 1, .live = false}
    };
    bool ok = VecInsertRangeFastR(&vec, src, 0, (size)-1);

    bool result = (ok == false) && (VecLen(&vec) == 0);

    VecDeinit(&vec);
    return result;
}

// ---- 305:21 cxx_add_to_sub ----------------------------------------------
// Grow predicate `length + count >= capacity`. With length == count == cap,
// the mutant's `length - count` underflows to 0 < cap, skips the grow, and
// the appended elements run past the (capacity + 1) buffer. Assert all
// elements land correctly.
bool test_grow_skip_underflow_predicate(void) {
    WriteFmt("Testing fast-insert grows when length+count reaches capacity\n");

    typedef Vec(int) IntVec;
    IntVec vec = VecInit(&alloc);

    VecReserve(&vec, 8);
    for (int i = 0; i < 8; i++) {
        VecPushBackR(&vec, i);
    }

    int  add[8] = {100, 101, 102, 103, 104, 105, 106, 107};
    bool ok     = VecInsertRangeFastR(&vec, add, VecLen(&vec), 8);

    // Capacity contract: holding 16 live elements requires the grow to have
    // run, so capacity must have expanded past the old value of 8. The mutant
    // skips the grow and leaves capacity == 8.
    bool result = ok && (VecLen(&vec) == 16) && (VecCapacity(&vec) >= 16);
    for (int i = 0; i < 8; i++) {
        result = result && (VecAt(&vec, (size)i) == i);
    }
    // Appended items occupy the new tail [8, 16).
    bool found_all = true;
    for (int v = 100; v < 108; v++) {
        bool found = false;
        for (size j = 8; j < VecLen(&vec); j++) {
            if (VecAt(&vec, j) == v) {
                found = true;
                break;
            }
        }
        found_all = found_all && found;
    }
    result = result && found_all;

    VecDeinit(&vec);
    return result;
}

// ---- 305:29 cxx_ge_to_gt ------------------------------------------------
// Grow predicate `length + count >= capacity` -> `length + count > capacity`.
// The two differ only at the exact boundary length + count == capacity. With a
// NON-power-of-2 capacity (here 10) and an insert that brings length + count to
// exactly capacity, real code calls reserve_pow2_vec(10) which rounds capacity
// up to the next power of two (16) and reallocates. The `>` mutant treats the
// boundary as "fits" and skips the grow, leaving capacity at 10. Assert the
// post-insert capacity was actually bumped past the old non-pow2 value.
bool test_grow_predicate_boundary_equal(void) {
    WriteFmt("Testing fast-insert grows at length+count == capacity boundary\n");

    typedef Vec(int) IntVec;
    IntVec vec = VecInit(&alloc);

    VecReserve(&vec, 10); // exact, non-power-of-2 capacity
    for (int i = 0; i < 5; i++) {
        VecPushBackR(&vec, i);
    }

    // length 5 + count 5 == capacity 10: the equality boundary.
    int  add[5] = {100, 101, 102, 103, 104};
    bool ok     = VecInsertRangeFastR(&vec, add, VecLen(&vec), 5);

    // Real code reserves past the old capacity (10 -> 16). The `>` mutant skips
    // the grow and leaves capacity at 10.
    bool result = ok && (VecLen(&vec) == 10) && (VecCapacity(&vec) > 10);
    for (int i = 0; i < 5; i++) {
        result = result && (VecAt(&vec, (size)i) == i);
    }

    VecDeinit(&vec);
    return result;
}

// ---- 305:29 cxx_ge_to_lt ------------------------------------------------
// Inverting the grow predicate makes a genuinely growth-needing insert skip
// reservation, writing into a NULL/tiny buffer. Insert into an empty vec.
bool test_grow_predicate_inverted(void) {
    WriteFmt("Testing fast-insert grows an empty vector\n");

    typedef Vec(int) IntVec;
    IntVec vec = VecInit(&alloc);

    int  add[8] = {10, 20, 30, 40, 50, 60, 70, 80};
    bool ok     = VecInsertRangeFastR(&vec, add, 0, 8);

    // Holding 8 elements requires growth from capacity 0; the mutant's
    // inverted predicate skips it.
    bool result = ok && (VecLen(&vec) == 8) && (VecCapacity(&vec) >= 8);
    for (size i = 0; i < 8; i++) {
        result = result && (VecAt(&vec, i) == add[i]);
    }

    VecDeinit(&vec);
    return result;
}

// ---- 306:14 cxx_replace_scalar_call -------------------------------------
// reserve_pow2_vec's result drives both the early-out and the actual growth.
// Replacing it with a constant either rejects a valid insert or skips the
// realloc. A growth-needing insert must return true with all elements present.
bool test_grow_reserve_result_honored(void) {
    WriteFmt("Testing fast-insert honors reserve growth result\n");

    typedef Vec(int) IntVec;
    IntVec vec = VecInit(&alloc);

    int  add[10] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    bool ok      = VecInsertRangeFastR(&vec, add, 0, 10);

    // A truthy replacement skips the realloc (capacity stays 0); a zero
    // replacement makes the call return false. Either way the real growth
    // contract (true return, all 10 elements, capacity >= 10) breaks.
    bool result = ok && (VecLen(&vec) == 10) && (VecCapacity(&vec) >= 10);
    for (size i = 0; i < 10; i++) {
        result = result && (VecAt(&vec, i) == add[i]);
    }

    VecDeinit(&vec);
    return result;
}

// ---- 304:18 cxx_assign_const / 304:20 cxx_replace_scalar_call -----------
// aligned_size sizes the displacement MemMove. Corrupting it (to 42, or any
// constant != 8) mis-sizes the relocation of the live tail, losing or
// scrambling the displaced originals. A middle insert with displaced > 0 must
// preserve every displaced original after the inserted block.
bool test_displacement_move_size(void) {
    WriteFmt("Testing fast-insert preserves displaced tail (aligned_size)\n");

    ElemVec vec = VecInit(&alloc); // no copy hooks: plain MemCopy path

    Elem orig[4];
    for (int i = 0; i < 4; i++) {
        orig[i].value = 10 + i; // 10,11,12,13
        orig[i].live  = false;
    }
    VecPushBackArrR(&vec, orig, 4);

    // Insert 2 elements at idx 1 -> displaced = min(length-idx, count) = 2.
    Elem add[2] = {
        {.value = 100, .live = false},
        {.value = 200, .live = false}
    };
    bool ok = VecInsertRangeFastR(&vec, add, 1, 2);

    bool result = ok && (VecLen(&vec) == 6);
    // Prefix [0,1) intact, inserted block at [1,3).
    result = result && (VecAt(&vec, 0).value == 10);
    result = result && (VecAt(&vec, 1).value == 100);
    result = result && (VecAt(&vec, 2).value == 200);
    // Displaced originals {11,12} must appear somewhere in the new tail [3,6).
    for (int v = 11; v <= 12; v++) {
        bool found = false;
        for (size j = 3; j < VecLen(&vec); j++) {
            if (VecAt(&vec, j).value == v) {
                found = true;
                break;
            }
        }
        result = result && found;
    }
    // Trailing original 13 (never displaced) also survives.
    bool found13 = false;
    for (size j = 3; j < VecLen(&vec); j++) {
        if (VecAt(&vec, j).value == 13) {
            found13 = true;
            break;
        }
    }
    result = result && found13;

    VecDeinit(&vec);
    return result;
}

// ---- 304:18 cxx_assign_const / 304:20 cxx_replace_scalar_call (over-copy) -
// aligned_size also sizes the displacement MemMove byte count
// (`aligned_size * displaced`). Forcing the stride to 42 (instead of the real
// 8) makes the relocation copy 42*displaced bytes -- a 5x over-copy. On a small
// vector the surplus lands in heap slack and the displaced-tail search still
// finds the originals (so test_displacement_move_size cannot see it). On a LARGE
// vector with displaced == length the source read [0, 42*length) runs far past
// the ~16*length-byte allocation and faults. Real code copies 8*length bytes and
// stays in bounds, producing the front-insert layout.
bool test_displacement_move_overcopy(void) {
    WriteFmt("Testing fast-insert displacement move stays in bounds (over-copy)\n");

    ElemVec vec = VecInit(&alloc); // no copy hooks: plain MemCopy path

    // N live originals; inserting N at idx 0 makes displaced == min(N, N) == N,
    // so the displacement MemMove relocates the whole live block. A 42x byte
    // count reads ~42*N bytes from a buffer holding ~8*(2N+1), overrunning it.
    const size N = 50000;
    for (size i = 0; i < N; i++) {
        Elem e = {.value = (int)i, .live = false};
        VecPushBackR(&vec, e);
    }

    // The inserted block is held in a scratch vec; its raw storage feeds the
    // fast insert.
    ElemVec ins = VecInit(&alloc);
    for (size i = 0; i < N; i++) {
        Elem e = {.value = 1000000 + (int)i, .live = false};
        VecPushBackR(&ins, e);
    }

    bool ok = VecInsertRangeFastR(&vec, VecBegin(&ins), 0, N);

    bool result = ok && (VecLen(&vec) == 2 * N);
    // The new block occupies the front [0, N).
    result = result && (VecAt(&vec, 0).value == 1000000);
    // Every displaced original must still be present somewhere in the tail.
    result = result && (VecAt(&vec, N).value >= 0);

    VecDeinit(&ins);
    VecDeinit(&vec);
    return result;
}

// ---- 330:40 cxx_add_to_sub ----------------------------------------------
// Pre-zero MemSet target `idx + i` -> `idx - i`. For i >= 1 this zeros a live
// prefix slot instead of the destination, destroying a front element. Assert
// the untouched prefix [0, idx) survives a copy_init middle insert.
bool test_prezero_target_index(void) {
    WriteFmt("Testing fast-insert leaves prefix intact (pre-zero index)\n");

    ElemVec vec = make_filled_elem_vec(10, 4); // values 10,11,12,13

    g_fail_at = 0;                             // all copies succeed
    reset_hook_counters();

    Elem add[2] = {
        {.value = 100, .live = false},
        {.value = 200, .live = false}
    };
    bool ok = VecInsertRangeFastR(&vec, add, 2, 2);

    bool result = ok && (VecLen(&vec) == 6);
    // Prefix [0,2) must be the original 10,11.
    result = result && (VecAt(&vec, 0).value == 10);
    result = result && (VecAt(&vec, 1).value == 11);
    // Inserted block correct.
    result = result && (VecAt(&vec, 2).value == 100);
    result = result && (VecAt(&vec, 3).value == 200);

    VecDeinit(&vec);
    return result;
}

// ---- 331:53 cxx_add_to_sub ----------------------------------------------
// copy_init destination `idx + i` -> `idx - i`. For i >= 1 the copy lands in a
// prefix slot and the real destination stays uninitialized. Assert inserted
// block holds the source items in order and prefix is intact.
bool test_copy_init_dest_index(void) {
    WriteFmt("Testing fast-insert copy_init destination index\n");

    ElemVec vec = make_filled_elem_vec(10, 4); // 10,11,12,13

    g_fail_at = 0;
    reset_hook_counters();

    Elem add[2] = {
        {.value = 100, .live = false},
        {.value = 200, .live = false}
    };
    bool ok = VecInsertRangeFastR(&vec, add, 2, 2);

    bool result = ok && (VecLen(&vec) == 6);
    result      = result && (VecAt(&vec, 2).value == 100);
    result      = result && (VecAt(&vec, 3).value == 200);
    // Prefix [0,2) intact.
    result = result && (VecAt(&vec, 0).value == 10);
    result = result && (VecAt(&vec, 1).value == 11);

    VecDeinit(&vec);
    return result;
}

// ---- 331:84 cxx_mul_to_div ----------------------------------------------
// copy_init source offset `item_data + i*item_size` -> `i/item_size`. With
// item_size 8 and i < 8 every source read collapses to item_data[0], so all
// inserted slots would carry the first element's value. Assert each inserted
// slot equals its distinct source element.
bool test_copy_init_source_stride(void) {
    WriteFmt("Testing fast-insert copy_init source stride\n");

    ElemVec vec = make_filled_elem_vec(10, 2); // 10,11

    g_fail_at = 0;
    reset_hook_counters();

    Elem add[3] = {
        {.value = 100, .live = false},
        {.value = 200, .live = false},
        {.value = 300, .live = false}
    };
    bool ok = VecInsertRangeFastR(&vec, add, 1, 3);

    bool result = ok && (VecLen(&vec) == 5);
    result      = result && (VecAt(&vec, 1).value == 100);
    result      = result && (VecAt(&vec, 2).value == 200);
    result      = result && (VecAt(&vec, 3).value == 300);

    VecDeinit(&vec);
    return result;
}

// ---- 287:10 cxx_init_const ----------------------------------------------
// inserted_count seeds the rollback loop bound. With copy_init failing on the
// FIRST element no slot was inited, so zero copy_deinit calls must occur. A
// non-zero seed makes the rollback deinit never-inited slots.
bool test_first_element_failure_no_deinit(void) {
    WriteFmt("Testing fast-insert first-element failure runs no deinit\n");

    ElemVec vec = make_filled_elem_vec(10, 4); // 10,11,12,13

    reset_hook_counters();
    g_fail_at = 1;                             // fail on the very first copy_init of this insert

    Elem add[3] = {
        {.value = 100, .live = false},
        {.value = 200, .live = false},
        {.value = 300, .live = false}
    };
    bool ok = VecInsertRangeFastR(&vec, add, 2, 3);

    bool result = (ok == false) && (g_deinit_count == 0) && (VecLen(&vec) == 4);
    // Originals fully intact after rollback.
    result = result && (VecAt(&vec, 0).value == 10);
    result = result && (VecAt(&vec, 3).value == 13);

    g_fail_at = 0; // let VecDeinit clean up the live originals
    VecDeinit(&vec);
    return result;
}

// ---- 332:27 cxx_init_const ----------------------------------------------
// Rollback loop start s = 0. With one successful copy then a failure, exactly
// one copy_deinit (on idx+0) must fire. A non-zero start skips it -> leak.
bool test_rollback_start_index_zero(void) {
    WriteFmt("Testing fast-insert rollback starts at slot 0\n");

    ElemVec vec = make_filled_elem_vec(10, 4); // 10,11,12,13

    reset_hook_counters();
    g_fail_at = 2;                             // first copy succeeds, second fails -> inserted_count == 1

    Elem add[3] = {
        {.value = 100, .live = false},
        {.value = 200, .live = false},
        {.value = 300, .live = false}
    };
    bool ok = VecInsertRangeFastR(&vec, add, 2, 3);

    bool result = (ok == false) && (g_deinit_count == 1);
    // The single deinit must hit the first inserted copy (value 100).
    result = result && deinit_log_contains(100);

    g_fail_at = 0;
    VecDeinit(&vec);
    return result;
}

// ---- 332:36 cxx_lt_to_ge ------------------------------------------------
// Guard `s < inserted_count` -> `s >= inserted_count`. On a first-element
// failure inserted_count == 0; the real guard never enters, the mutant loops
// unbounded. Assert zero deinit calls (real code) — the mutant either crashes
// or records a flood of deinits.
bool test_rollback_guard_first_fail(void) {
    WriteFmt("Testing fast-insert rollback guard on first failure\n");

    ElemVec vec = make_filled_elem_vec(10, 4);

    reset_hook_counters();
    g_fail_at = 1;

    Elem add[2] = {
        {.value = 100, .live = false},
        {.value = 200, .live = false}
    };
    bool ok = VecInsertRangeFastR(&vec, add, 2, 2);

    bool result = (ok == false) && (g_deinit_count == 0);

    g_fail_at = 0;
    VecDeinit(&vec);
    return result;
}

// ---- 332:36 cxx_lt_to_le ------------------------------------------------
// Guard `<` -> `<=` deinits one extra slot: idx+inserted_count, the slot whose
// copy_init just failed (zeroed, live == false). Assert the deinit count
// equals the number of successful copies and every deinit hit a live slot.
bool test_rollback_guard_no_extra_deinit(void) {
    WriteFmt("Testing fast-insert rollback deinits only inited slots\n");

    ElemVec vec = make_filled_elem_vec(10, 5); // 10..14

    reset_hook_counters();
    g_fail_at = 3;                             // two succeed, third fails -> inserted_count == 2

    Elem add[4] = {
        {.value = 100, .live = false},
        {.value = 200, .live = false},
        {.value = 300, .live = false},
        {.value = 400, .live = false}
    };
    bool ok = VecInsertRangeFastR(&vec, add, 2, 4);

    bool result = (ok == false) && (g_deinit_count == 2);
    // Every deinit must have seen a live (truly inited) slot.
    for (size i = 0; i < g_deinit_count && i < 256; i++) {
        result = result && g_deinit_was_live[i];
    }

    g_fail_at = 0;
    VecDeinit(&vec);
    return result;
}

// ---- 332:55 cxx_post_inc_to_post_dec ------------------------------------
// Cleanup counter `s++` -> `s--` wraps after the first iteration, so only
// idx+0 is ever deinited. With two successful copies before the failure both
// must be deinited.
bool test_rollback_counter_increments(void) {
    WriteFmt("Testing fast-insert rollback counter advances\n");

    ElemVec vec = make_filled_elem_vec(10, 5);

    reset_hook_counters();
    g_fail_at = 3; // inserted_count == 2

    Elem add[4] = {
        {.value = 100, .live = false},
        {.value = 200, .live = false},
        {.value = 300, .live = false},
        {.value = 400, .live = false}
    };
    bool ok = VecInsertRangeFastR(&vec, add, 2, 4);

    bool result = (ok == false) && (g_deinit_count == 2);
    result      = result && deinit_log_contains(100) && deinit_log_contains(200);

    g_fail_at = 0;
    VecDeinit(&vec);
    return result;
}

// ---- 333:58 cxx_add_to_sub ----------------------------------------------
// copy_deinit target `idx + s` -> `idx - s`. For s >= 1 it deinits a live
// prefix element instead of the inserted copy. With a middle insert the
// rollback must deinit the inserted copies (values 100,200), never the
// prefix originals.
bool test_rollback_deinit_target(void) {
    WriteFmt("Testing fast-insert rollback deinit target index\n");

    ElemVec vec = make_filled_elem_vec(10, 5); // 10..14

    reset_hook_counters();
    g_fail_at = 3;                             // two succeed (100,200), third fails

    Elem add[4] = {
        {.value = 100, .live = false},
        {.value = 200, .live = false},
        {.value = 300, .live = false},
        {.value = 400, .live = false}
    };
    bool ok = VecInsertRangeFastR(&vec, add, 2, 4);

    bool result = (ok == false) && (g_deinit_count == 2);
    // The rollback must target the inserted copies, including value 200 at
    // slot idx+1. The mutant deinits slot idx-1 (a prefix original) instead.
    result = result && deinit_log_contains(200);
    // And it must NOT have deinited the prefix original at idx-1 (value 11).
    result = result && !deinit_log_contains(11);

    g_fail_at = 0;
    VecDeinit(&vec);
    return result;
}

// ---- 347:27 cxx_post_inc_to_post_dec ------------------------------------
// inserted_count++ -> inserted_count-- wraps the success counter, so a later
// failure drives the rollback loop over a huge range. One success then a
// failure must yield exactly one deinit.
bool test_inserted_counter_increments(void) {
    WriteFmt("Testing fast-insert success counter advances\n");

    ElemVec vec = make_filled_elem_vec(10, 4);

    reset_hook_counters();
    g_fail_at = 2; // element 0 succeeds, element 1 fails

    Elem add[2] = {
        {.value = 100, .live = false},
        {.value = 200, .live = false}
    };
    bool ok = VecInsertRangeFastR(&vec, add, 2, 2);

    bool result = (ok == false) && (g_deinit_count == 1) && deinit_log_contains(100);

    g_fail_at = 0;
    VecDeinit(&vec);
    return result;
}

// ---- 339:53 cxx_add_to_sub ----------------------------------------------
// Failure-path restore source `length+count-displaced`; flipping the first +
// to - reads the wrong region and restores garbage to [idx, ...). After a
// failed middle insert the original tail must be intact.
bool test_rollback_restore_source_first_term(void) {
    WriteFmt("Testing fast-insert rollback restore source (first term)\n");

    ElemVec vec = make_filled_elem_vec(10, 5); // 10,11,12,13,14

    reset_hook_counters();
    g_fail_at = 2;                             // first copy succeeds, second fails -> rollback restores tail

    Elem add[2] = {
        {.value = 100, .live = false},
        {.value = 200, .live = false}
    };
    bool ok = VecInsertRangeFastR(&vec, add, 1, 2);

    bool result = (ok == false) && (VecLen(&vec) == 5);
    // Original elements must all be present and unchanged after rollback.
    for (size i = 0; i < 5; i++) {
        result = result && (VecAt(&vec, i).value == 10 + (int)i);
        result = result && VecAt(&vec, i).live;
    }

    g_fail_at = 0;
    VecDeinit(&vec);
    return result;
}

// ---- 339:61 cxx_sub_to_add ----------------------------------------------
// Same restore expression; flipping `count - displaced` to `count + displaced`
// reads past the parked block, restoring garbage. Original tail must survive.
bool test_rollback_restore_source_second_term(void) {
    WriteFmt("Testing fast-insert rollback restore source (second term)\n");

    ElemVec vec = make_filled_elem_vec(20, 5); // 20,21,22,23,24

    reset_hook_counters();
    g_fail_at = 2;

    Elem add[2] = {
        {.value = 100, .live = false},
        {.value = 200, .live = false}
    };
    bool ok = VecInsertRangeFastR(&vec, add, 1, 2);

    bool result = (ok == false) && (VecLen(&vec) == 5);
    for (size i = 0; i < 5; i++) {
        result = result && (VecAt(&vec, i).value == 20 + (int)i);
        result = result && VecAt(&vec, i).live;
    }

    g_fail_at = 0;
    VecDeinit(&vec);
    return result;
}

// ---- 340:38 cxx_mul_to_div ----------------------------------------------
// Rollback restore MemMove size `aligned_size * displaced` -> `/displaced`.
// With displaced >= 2 far fewer bytes move, so only part of the original tail
// is restored. Assert every displaced original survives a failed insert.
bool test_rollback_restore_size(void) {
    WriteFmt("Testing fast-insert rollback restore size\n");

    ElemVec vec = make_filled_elem_vec(30, 6); // 30..35

    reset_hook_counters();
    // Middle insert at idx 1, count 3 -> displaced = min(length-idx,count)=3.
    // Fail on the 2nd inserted element to force the displacement rollback.
    g_fail_at = 2;

    Elem add[3] = {
        {.value = 100, .live = false},
        {.value = 200, .live = false},
        {.value = 300, .live = false}
    };
    bool ok = VecInsertRangeFastR(&vec, add, 1, 3);

    bool result = (ok == false) && (VecLen(&vec) == 6);
    for (size i = 0; i < 6; i++) {
        result = result && (VecAt(&vec, i).value == 30 + (int)i);
        result = result && VecAt(&vec, i).live;
    }

    g_fail_at = 0;
    VecDeinit(&vec);
    return result;
}

// ===========================================================================
// insert_range_into_vec mutation-hardening suite (from Vec.Mutants2)
// ===========================================================================
//
// Drives a Vec(TrackItem) configured with a deep-copy handler that can be
// armed to fail copy_init on a chosen inserted index, and records every
// copy_deinit call (count + the tag of the deinit'd slot).
typedef struct {
    int   tag;
    void *owned; // always NULL; present only to give the element non-trivial size
} TrackItem;

// Failure arming: copy_init returns false when its call index == ti_fail_at.
// SIZE_MAX disables failure (used while building fixtures).
static size ti_fail_at      = (size)-1;
static size ti_init_calls   = 0; // copy_init invocations since last reset
static size ti_deinit_count = 0; // copy_deinit invocations since last reset

// Tags seen by copy_deinit (bounded recorder) and a flag for whether the
// failed/sentinel slot (tag 0) was ever deinit'd.
#define DEINIT_TAG_CAP 64
static int  g_deinit_tags[DEINIT_TAG_CAP];
static size g_deinit_tag_n    = 0;
static bool g_deinit_saw_zero = false;

static void reset_tracking(size fail_at) {
    ti_fail_at        = fail_at;
    ti_init_calls     = 0;
    ti_deinit_count   = 0;
    g_deinit_tag_n    = 0;
    g_deinit_saw_zero = false;
    for (size i = 0; i < DEINIT_TAG_CAP; i++) {
        g_deinit_tags[i] = -1;
    }
}

static bool track_copy_init(void *dst, const void *src, const Allocator *al) {
    (void)al;
    size this_call = ti_init_calls++;
    if (this_call == ti_fail_at) {
        // Caller already MemSet dst to 0; leave it untouched on failure.
        return false;
    }
    TrackItem       *d = (TrackItem *)dst;
    const TrackItem *s = (const TrackItem *)src;
    d->tag             = s->tag;
    d->owned           = NULL;
    return true;
}

static void track_copy_deinit(void *copy, const Allocator *al) {
    (void)al;
    TrackItem *c = (TrackItem *)copy;
    ti_deinit_count++;
    if (c->tag == 0) {
        g_deinit_saw_zero = true;
    }
    if (g_deinit_tag_n < DEINIT_TAG_CAP) {
        g_deinit_tags[g_deinit_tag_n++] = c->tag;
    }
    // No free: owned is always NULL by construction.
}

static bool deinit_tags_contain(int tag) {
    for (size i = 0; i < g_deinit_tag_n; i++) {
        if (g_deinit_tags[i] == tag) {
            return true;
        }
    }
    return false;
}

typedef Vec(TrackItem) TrackVec;
typedef Vec(u64) U64Vec;

// Build a TrackVec containing items with the given tags (copy_init never
// fails during this phase).
static TrackVec make_track_vec(const int *tags, size n) {
    TrackVec v = VecInitWithDeepCopy(track_copy_init, track_copy_deinit, &alloc);
    reset_tracking((size)-1);
    for (size i = 0; i < n; i++) {
        TrackItem it = {tags[i], NULL};
        VecInsertRangeR(&v, &it, VecLen(&v), 1);
    }
    return v;
}

// 234:15  cxx_gt_to_ge : `count > (size)-1 - length` overflow guard.
bool test_insert_overflow_count_returns_false(void) {
    WriteFmt("Testing insert with count == SIZE_MAX returns false (no overflow)\n");

    U64Vec vec = VecInit(&alloc);
    u64    one = 1;

    bool ret = VecInsertRangeR(&vec, &one, 0, (size)-1);

    bool result = (ret == false) && (VecLen(&vec) == 0);

    VecDeinit(&vec);
    return result;
}

// 238:18  cxx_assign_const : `aligned_size = vec_aligned_size(...)` -> 42.
bool test_aligned_size_const_preserves_originals(void) {
    WriteFmt("Testing front insert preserves originals (aligned_size assign)\n");

    U64Vec vec = VecInit(&alloc);
    VecReserve(&vec, 16);
    u64 originals[] = {10, 20, 30};
    VecPushBackArrR(&vec, originals, 3);

    u64  newv = 99;
    bool ret  = VecInsertRangeR(&vec, &newv, 0, 1);

    bool result = ret && (VecLen(&vec) == 4) && (VecAt(&vec, 0) == 99) && (VecAt(&vec, 1) == 10) &&
                  (VecAt(&vec, 2) == 20) && (VecAt(&vec, 3) == 30);

    VecDeinit(&vec);
    return result;
}

// 238:20  cxx_replace_scalar_call : replaces vec_aligned_size() with a scalar.
bool test_aligned_size_call_preserves_originals(void) {
    WriteFmt("Testing front insert keeps first original (aligned_size call)\n");

    U64Vec vec = VecInit(&alloc);
    VecReserve(&vec, 16);
    u64 originals[] = {11, 22, 33};
    VecPushBackArrR(&vec, originals, 3);

    u64  newv = 77;
    bool ret  = VecInsertRangeR(&vec, &newv, 0, 1);

    // The original at index 0 (11) must survive at index 1 after the insert.
    bool result = ret && (VecLen(&vec) == 4) && (VecAt(&vec, 0) == 77) && (VecAt(&vec, 1) == 11);

    VecDeinit(&vec);
    return result;
}

// 249:26  cxx_sub_to_add : shift-right size `(length - idx)` -> `(length + idx)`.
bool test_shift_right_size_middle_insert(void) {
    WriteFmt("Testing middle insert shift-right size\n");

    // Large N so that the `(length + idx)` over-copy of the mutant overruns the
    // allocation by ~length elements (far past any heap slack) and crashes,
    // while the real single-element move stays in bounds.
    const size N   = 100000;
    U64Vec     vec = VecInit(&alloc);
    for (size i = 0; i < N; i++) {
        u64 v = (u64)i;
        VecInsertRangeR(&vec, &v, VecLen(&vec), 1);
    }

    u64  newv = 999999;
    bool ret  = VecInsertRangeR(&vec, &newv, N - 1, 1); // insert just before the last element

    bool result = ret && (VecLen(&vec) == N + 1) && (VecAt(&vec, 0) == 0) && (VecAt(&vec, N - 2) == (u64)(N - 2)) &&
                  (VecAt(&vec, N - 1) == 999999) && (VecAt(&vec, N) == (u64)(N - 1));

    VecDeinit(&vec);
    return result;
}

// 221:10  cxx_init_const : `inserted_count = 0` -> 42.
bool test_rollback_first_item_no_deinit(void) {
    WriteFmt("Testing rollback on first-item failure deinit's nothing\n");

    int      tags[] = {1, 2, 3};
    TrackVec vec    = make_track_vec(tags, 3);

    VecReserve(&vec, 80); // give slots room so a runaway rollback stays in-bounds

    // Insert 1 item at the front; copy_init fails immediately (index 0).
    reset_tracking(0);
    TrackItem it  = {100, NULL};
    bool      ret = VecInsertRangeR(&vec, &it, 0, 1);

    bool result = (ret == false) && (ti_deinit_count == 0) && (VecLen(&vec) == 3);

    VecDeinit(&vec);
    return result;
}

// 257:27  cxx_init_const : rollback loop start `size s = 0` -> 42.
bool test_rollback_loop_start_index(void) {
    WriteFmt("Testing rollback loop start index deinit's all inited items\n");

    int      tags[] = {1, 2, 3};
    TrackVec vec    = make_track_vec(tags, 3);
    VecReserve(&vec, 32);

    // Append 3 items at the end; item 2 (0-based) fails -> inserted_count==2.
    reset_tracking(2);
    TrackItem items[] = {
        {100, NULL},
        {101, NULL},
        {102, NULL}
    };
    bool ret = VecInsertRangeR(&vec, items, VecLen(&vec), 3);

    bool result = (ret == false) && (ti_deinit_count == 2) && (VecLen(&vec) == 3);

    VecDeinit(&vec);
    return result;
}

// 257:36  cxx_lt_to_ge : rollback condition `s < inserted_count` -> `s >= ...`.
bool test_rollback_cond_runs_for_inited(void) {
    WriteFmt("Testing rollback condition deinit's the single inited item\n");

    int      tags[] = {1, 2, 3};
    TrackVec vec    = make_track_vec(tags, 3);
    VecReserve(&vec, 32);

    // Append 2 items; item 1 fails -> inserted_count == 1.
    reset_tracking(1);
    TrackItem items[] = {
        {100, NULL},
        {101, NULL}
    };
    bool ret = VecInsertRangeR(&vec, items, VecLen(&vec), 2);

    bool result = (ret == false) && (ti_deinit_count == 1) && (VecLen(&vec) == 3);

    VecDeinit(&vec);
    return result;
}

// 257:36  cxx_lt_to_le : rollback condition `s < inserted_count` -> `s <= ...`.
bool test_rollback_cond_skips_failed_slot(void) {
    WriteFmt("Testing rollback never deinit's the failed slot\n");

    int      tags[] = {1, 2, 3};
    TrackVec vec    = make_track_vec(tags, 3);
    VecReserve(&vec, 32);

    // Append 2 items; item 1 fails -> inserted_count == 1.
    reset_tracking(1);
    TrackItem items[] = {
        {100, NULL},
        {101, NULL}
    };
    bool ret = VecInsertRangeR(&vec, items, VecLen(&vec), 2);

    bool result = (ret == false) && (ti_deinit_count == 1) && (g_deinit_saw_zero == false) && (VecLen(&vec) == 3);

    VecDeinit(&vec);
    return result;
}

// 257:55  cxx_post_inc_to_post_dec : rollback step `s++` -> `s--`.
bool test_rollback_step_increments(void) {
    WriteFmt("Testing rollback step deinit's every inited item\n");

    int      tags[] = {1, 2, 3};
    TrackVec vec    = make_track_vec(tags, 3);
    VecReserve(&vec, 32);

    // Append 3 items; item 2 fails -> inserted_count == 2.
    reset_tracking(2);
    TrackItem items[] = {
        {100, NULL},
        {101, NULL},
        {102, NULL}
    };
    bool ret = VecInsertRangeR(&vec, items, VecLen(&vec), 3);

    bool result = (ret == false) && (ti_deinit_count == 2) && (VecLen(&vec) == 3);

    VecDeinit(&vec);
    return result;
}

// 258:58  cxx_add_to_sub : rollback deinit target `idx + s` -> `idx - s`.
bool test_rollback_deinit_target_slots(void) {
    WriteFmt("Testing rollback deinit targets the inserted slots\n");

    int      tags[] = {1}; // single existing element 'A' at slot 0
    TrackVec vec    = make_track_vec(tags, 1);
    VecReserve(&vec, 32);

    // Append (idx == length == 1) 3 items; item 2 fails -> inserted_count == 2.
    reset_tracking(2);
    TrackItem items[] = {
        {100, NULL},
        {101, NULL},
        {102, NULL}
    };
    bool ret = VecInsertRangeR(&vec, items, 1, 3);

    bool result = (ret == false) && (ti_deinit_count == 2) && deinit_tags_contain(100) && deinit_tags_contain(101) &&
                  !deinit_tags_contain(1) && (VecLen(&vec) == 1) && (VecAt(&vec, 0).tag == 1);

    VecDeinit(&vec);
    return result;
}

// 273:27  cxx_post_inc_to_post_dec : `inserted_count++` -> `inserted_count--`.
bool test_rollback_inserted_count_step(void) {
    WriteFmt("Testing inserted_count increment bounds the rollback loop\n");

    int      tags[] = {1, 2, 3};
    TrackVec vec    = make_track_vec(tags, 3);
    VecReserve(&vec, 32);

    // Append 2 items; item 1 fails -> exactly 1 inited item.
    reset_tracking(1);
    TrackItem items[] = {
        {100, NULL},
        {101, NULL}
    };
    bool ret = VecInsertRangeR(&vec, items, VecLen(&vec), 2);

    bool result = (ret == false) && (ti_deinit_count == 1) && (VecLen(&vec) == 3);

    VecDeinit(&vec);
    return result;
}

// 262:25  cxx_lt_to_ge : shift-back guard `idx < length` -> `idx >= length`.
bool test_rollback_restore_middle_guard(void) {
    WriteFmt("Testing failed middle insert restores originals (shift-back guard)\n");

    int      tags[] = {1, 2, 3}; // A,B,C
    TrackVec vec    = make_track_vec(tags, 3);
    VecReserve(&vec, 32);

    // Insert 1 item at idx 1; copy_init fails immediately (inserted_count == 0).
    reset_tracking(0);
    TrackItem it  = {100, NULL};
    bool      ret = VecInsertRangeR(&vec, &it, 1, 1);

    bool result = (ret == false) && (VecLen(&vec) == 3) && (VecAt(&vec, 0).tag == 1) && (VecAt(&vec, 1).tag == 2) &&
                  (VecAt(&vec, 2).tag == 3);

    VecDeinit(&vec);
    return result;
}

// 265:45  cxx_add_to_sub : shift-back source `idx + count` -> `idx - count`.
bool test_rollback_restore_shiftback_source(void) {
    WriteFmt("Testing failed middle insert restores from correct source\n");

    int      tags[] = {1, 2, 3};
    TrackVec vec    = make_track_vec(tags, 3);
    VecReserve(&vec, 32);

    reset_tracking(0);
    TrackItem it  = {100, NULL};
    bool      ret = VecInsertRangeR(&vec, &it, 1, 1);

    bool result = (ret == false) && (VecLen(&vec) == 3) && (VecAt(&vec, 0).tag == 1) && (VecAt(&vec, 1).tag == 2) &&
                  (VecAt(&vec, 2).tag == 3);

    VecDeinit(&vec);
    return result;
}

// 266:38  cxx_sub_to_add : shift-back size `(length - idx)` -> `(length + idx)`.
bool test_rollback_restore_shiftback_size_sub(void) {
    WriteFmt("Testing failed insert shift-back size (sub)\n");

    // Large N: the `(length + idx)` over-move of the mutant runs ~length
    // elements past the buffer during rollback and crashes; the real path moves
    // a single element and restores every original.
    const size N   = 100000;
    TrackVec   vec = VecInitWithDeepCopy(track_copy_init, track_copy_deinit, &alloc);
    reset_tracking((size)-1);
    for (size i = 0; i < N; i++) {
        TrackItem it = {(int)(i + 1), NULL};
        VecInsertRangeR(&vec, &it, VecLen(&vec), 1);
    }

    // Insert 1 item at idx N-1 (< length); copy_init fails immediately.
    reset_tracking(0);
    TrackItem it  = {-100, NULL};
    bool      ret = VecInsertRangeR(&vec, &it, N - 1, 1);

    bool result = (ret == false) && (VecLen(&vec) == N) && (VecAt(&vec, 0).tag == 1) &&
                  (VecAt(&vec, N - 2).tag == (int)(N - 1)) && (VecAt(&vec, N - 1).tag == (int)N);

    VecDeinit(&vec);
    return result;
}

// 266:45  cxx_mul_to_div : shift-back size `(length - idx) * aligned` ->
// `(length - idx) / aligned`.
bool test_rollback_restore_shiftback_size_div(void) {
    WriteFmt("Testing failed middle insert shift-back size (div)\n");

    int      tags[] = {1, 2, 3};
    TrackVec vec    = make_track_vec(tags, 3);
    VecReserve(&vec, 32);

    reset_tracking(0);
    TrackItem it  = {100, NULL};
    bool      ret = VecInsertRangeR(&vec, &it, 1, 1);

    bool result = (ret == false) && (VecLen(&vec) == 3) && (VecAt(&vec, 0).tag == 1) && (VecAt(&vec, 1).tag == 2) &&
                  (VecAt(&vec, 2).tag == 3);

    VecDeinit(&vec);
    return result;
}

// ===========================================================================
// vec_insert_range_l / vec_insert_one_l mutation-hardening (from Vec.Mutants5)
// ===========================================================================

// vec_insert_range_l, preserve-order path.
bool test_insert_range_l_preserve_inserts_all(void) {
    WriteFmt("Testing VecInsertRangeL (preserve order) inserts all items\n");

    DefaultAllocator local = DefaultAllocatorInit();

    typedef Vec(u32) U32Vec;
    U32Vec vec = VecInit(&local);

    u32  items[] = {111u, 222u, 333u};
    bool ok      = VecInsertRangeL(&vec, items, 0, 3);

    bool result =
        ok && (VecLen(&vec) == 3) && (VecAt(&vec, 0) == 111u) && (VecAt(&vec, 1) == 222u) && (VecAt(&vec, 2) == 333u);

    VecDeinit(&vec);
    DefaultAllocatorDeinit(&local);
    return result;
}

// vec_insert_range_l, fast (non-preserve) path.
bool test_insert_range_l_fast_inserts_all(void) {
    WriteFmt("Testing VecInsertRangeFastL inserts all items\n");

    DefaultAllocator local = DefaultAllocatorInit();

    typedef Vec(u32) U32Vec;
    U32Vec vec = VecInit(&local);

    u32  items[] = {444u, 555u, 666u};
    bool ok      = VecInsertRangeFastL(&vec, items, 0, 3);

    // Fast insert into an empty vector keeps order, so a direct index
    // check is valid here.
    bool result =
        ok && (VecLen(&vec) == 3) && (VecAt(&vec, 0) == 444u) && (VecAt(&vec, 1) == 555u) && (VecAt(&vec, 2) == 666u);

    VecDeinit(&vec);
    DefaultAllocatorDeinit(&local);
    return result;
}

// vec_insert_one_l.
bool test_insert_one_l_inserts_value(void) {
    WriteFmt("Testing VecInsertL inserts a single value\n");

    DefaultAllocator local = DefaultAllocatorInit();

    typedef Vec(u32) U32Vec;
    U32Vec vec = VecInit(&local);

    u32  val = 777u;
    bool ok  = VecInsertL(&vec, val, 0);

    bool result = ok && (VecLen(&vec) == 1) && (VecAt(&vec, 0) == 777u);

    VecDeinit(&vec);
    DefaultAllocatorDeinit(&local);
    return result;
}

// An element wide enough to carry a copy_init hook that can be armed to fail.
typedef struct {
    int value;
} MutElem;

typedef Vec(MutElem) MutElemVec;

// copy_init that fails on the very first call of an insert (armed via g_fail).
static bool g_fail = false;

static bool mut_copy_init(void *dst_, const void *src_, const Allocator *a) {
    (void)a;
    if (g_fail) {
        return false;
    }
    MutElem       *dst = (MutElem *)dst_;
    const MutElem *src = (const MutElem *)src_;
    dst->value         = src->value;
    return true;
}

static void mut_copy_deinit(void *p_, const Allocator *a) {
    (void)a;
    ((MutElem *)p_)->value = 0;
}

// ---- 603:32 cxx_replace_scalar_call -------------------------------------
// vec_insert_range_l, fast (preserve_order == false) path:
//   success = ... : insert_range_fast_into_vec(vec, items, item_size, idx, count);
// The mutant replaces the call's return value with the literal 42 (truthy),
// keeping the side effects. On a copy_init FAILURE the real call returns false,
// so VecInsertRangeFastL must report failure; the mutant reports success (42 ->
// true). Drive a fast L-insert whose first copy_init fails and assert the return
// is false.
bool test_insert_range_fast_l_reports_failure(void) {
    WriteFmt("Testing VecInsertRangeFastL reports copy_init failure (603:32)\n");

    // Canary allocator: the empty-vec / idx==length failure path also pins the
    // displaced-init mutant (261:10, `size displaced = 0` -> 42). With idx ==
    // length the live-tail recompute is skipped, so a non-zero displaced init
    // drives the rollback's restore MemMove out of bounds; the overflow smashes
    // the trailing canary, which the free at VecDeinit records as an overflow
    // event -- so asserting zero overflows kills the mutant deterministically
    // instead of relying on a chance heap crash.
    DebugAllocator dbg = DebugAllocatorInit();

    MutElemVec vec = VecInitWithDeepCopy(mut_copy_init, mut_copy_deinit, &dbg);

    // Arm copy_init to fail, then attempt a fast L-insert into the empty vec.
    g_fail           = true;
    MutElem items[2] = {{.value = 10}, {.value = 20}};
    bool    ok       = VecInsertRangeFastL(&vec, items, 0, 2);
    g_fail           = false;

    // Real: the insert fails, ok == false, nothing landed in the vec.
    bool result = (ok == false) && (VecLen(&vec) == 0);

    VecDeinit(&vec);
    result = result && (DebugAllocatorOverflows(&dbg) == 0);
    DebugAllocatorDeinit(&dbg);
    return result;
}

// ---- 584:10 cxx_init_const ----------------------------------------------
// vec_insert_one_l (the single-element L-insert backing VecInsertL):
//   bool success = preserve_order ? insert_range_into_vec(...) :
//                                   insert_range_fast_into_vec(...);
// col 10 is the initialiser of `success`. The mutant rewrites it to the
// literal 42 (truthy) while keeping the ternary's side effects, so a FAILED
// copy_init insert (real success == false) is reported as success. The
// pre-existing 603:32 test covers the range (Many) backend; this covers the
// single-element backend. Drive a one-element preserve-order L-insert whose
// copy_init fails and assert the return is false with nothing landed.
bool test_insert_one_l_reports_failure(void) {
    WriteFmt("Testing VecInsertL reports copy_init failure (584:10)\n");

    MutElemVec vec = VecInitWithDeepCopy(mut_copy_init, mut_copy_deinit, &alloc);

    g_fail       = true;
    MutElem item = {.value = 99};
    bool    ok   = VecInsertL(&vec, item, 0);
    g_fail       = false;

    // Real: the insert fails, ok == false, nothing landed in the vec.
    bool result = (ok == false) && (VecLen(&vec) == 0);

    VecDeinit(&vec);
    return result;
}

// Main function that runs all tests
int main(void) {
    alloc = DefaultAllocatorInit();
    WriteFmt("[INFO] Starting Vec.Insert tests\n\n");

    // Array of test functions
    TestFunction tests[] = {
        test_vec_push_back,
        test_vec_push_front,
        test_vec_insert,
        test_vec_push_back_arr,
        test_vec_push_front_arr,
        test_vec_push_arr,
        test_vec_insert_range,
        test_vec_merge,
        test_vec_init_clone_inherits_allocator_config,
        test_lvalue_rvalue_operations,
        test_lvalue_zero_on_take_after_insertion,
        test_vec_insert_range_fast_overflowing_tail,
        // insert_range_fast_into_vec mutation-hardening (Vec.Mutants1)
        test_fast_overflow_count_returns_false,
        test_grow_skip_underflow_predicate,
        test_grow_predicate_boundary_equal,
        test_grow_predicate_inverted,
        test_grow_reserve_result_honored,
        test_displacement_move_size,
        test_displacement_move_overcopy,
        test_prezero_target_index,
        test_copy_init_dest_index,
        test_copy_init_source_stride,
        test_first_element_failure_no_deinit,
        test_rollback_start_index_zero,
        test_rollback_guard_first_fail,
        test_rollback_guard_no_extra_deinit,
        test_rollback_counter_increments,
        test_rollback_deinit_target,
        test_inserted_counter_increments,
        test_rollback_restore_source_first_term,
        test_rollback_restore_source_second_term,
        test_rollback_restore_size,
        // insert_range_into_vec mutation-hardening (Vec.Mutants2)
        test_insert_overflow_count_returns_false,
        test_aligned_size_const_preserves_originals,
        test_aligned_size_call_preserves_originals,
        test_shift_right_size_middle_insert,
        test_rollback_first_item_no_deinit,
        test_rollback_loop_start_index,
        test_rollback_cond_runs_for_inited,
        test_rollback_cond_skips_failed_slot,
        test_rollback_step_increments,
        test_rollback_deinit_target_slots,
        test_rollback_inserted_count_step,
        test_rollback_restore_middle_guard,
        test_rollback_restore_shiftback_source,
        test_rollback_restore_shiftback_size_sub,
        test_rollback_restore_shiftback_size_div,
        // vec_insert_range_l / vec_insert_one_l (Vec.Mutants5)
        test_insert_range_l_preserve_inserts_all,
        test_insert_range_l_fast_inserts_all,
        test_insert_one_l_inserts_value,
        // L-insert copy_init failure reporting (Vec.Mut)
        test_insert_range_fast_l_reports_failure,
        test_insert_one_l_reports_failure
    };

    int total_tests = sizeof(tests) / sizeof(tests[0]);

    // Run all tests using the centralized test driver
    int rc = run_test_suite(tests, total_tests, NULL, 0, "Vec.Insert");
    DefaultAllocatorDeinit(&alloc);
    return rc;
}
