#include <Misra/Std/Allocator/Default.h>
#include <Misra/Std/Allocator/Heap.h>
#include <Misra/Std/Container/Str.h>
#include <Misra/Std/Container/Vec.h>
#include <Misra/Std/Log.h>
#include <Misra/Std/Zstr.h>
#include <Misra/Types.h>

// Include test utilities
#include "../../Util/TestRunner.h"

// Function prototypes
bool test_vec_try_reduce_space(void);
bool test_vec_resize(void);
bool test_vec_reserve(void);
bool test_vec_clear(void);
bool test_vec_reserve_capacity_overflow_aborts(void);

// ---- resize_vec / reserve_pow2_vec mutation-hardening (Vec.Mutants3) -----
bool test_resize_grow_branch_allocates(void);
bool test_resize_shrink_deinits_dropped(void);
bool test_resize_grow_reserves_capacity(void);
bool test_resize_grow_sets_exact_length(void);
bool test_reserve_pow2_seed(void);

// ---- reserve_vec mutation-hardening (Vec.Mutants4) -----------------------
bool test_reserve_zero_low_slot(void);
bool test_reserve_zero_high_slot(void);
bool test_reserve_overflow_boundary_returns_false(void);

// ---- clear_vec mutation-hardening (Vec.Mutants5) -------------------------
bool test_clear_str_scrubs_sentinel(void);
bool test_clear_str_cap1_scrubs_sentinel(void);
bool test_clear_large_char_vec_scrub_stride(void);
bool test_clear_runs_deinit_once_per_element(void);

// ---- reduce_space_vec mutation-hardening (Vec.Mutants6) ------------------
bool test_reduce_nonempty_preallocated_keeps_data(void);
bool test_reduce_nonempty_keeps_length(void);
bool test_reduce_empty_capacity_zero(void);
bool test_reduce_empty_length_zero(void);
bool test_reduce_shrinks_capacity_to_length(void);

// ---- validate_vec structural-guard deadend (Vec.Mutants5) ----------------
bool test_validate_rejects_length_over_capacity(void);

// File-static allocator used by the relocated resize_vec mutation tests.
static DefaultAllocator alloc;

// ---- copy_deinit instrumentation for resize_vec shrink (Vec.Mutants3) ----
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

// ---- copy_deinit counter for clear_vec (Vec.Mutants5) --------------------
static size g_deinit_calls;

static void clear_counting_deinit(void *copy, const Allocator *al) {
    (void)copy;
    (void)al;
    g_deinit_calls++;
}

// Test VecTryReduceSpace function
bool test_vec_try_reduce_space(void) {
    WriteFmt("Testing VecTryReduceSpace\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    // Create a vector of integers
    typedef Vec(int) IntVec;
    IntVec vec = VecInit(&alloc);

    // Reserve more space than needed
    VecReserve(&vec, 100);

    // Add some data
    int values[] = {10, 20, 30, 40, 50};
    for (int i = 0; i < 5; i++) {
        VecPushBackR(&vec, values[i]);
    }

    // Original capacity should be at least 100
    bool result = (VecCapacity(&vec) >= 100);

    // Try to reduce space
    VecTryReduceSpace(&vec);

    // Capacity should now be closer to the actual length
    result = result && (VecCapacity(&vec) < 100) && (VecCapacity(&vec) >= VecLen(&vec));

    // Check that the data is still intact
    for (size i = 0; i < VecLen(&vec); i++) {
        result = result && (VecAt(&vec, i) == values[i]);
    }

    // Clean up
    VecDeinit(&vec);

    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Test VecResize function
bool test_vec_resize(void) {
    WriteFmt("Testing VecResize\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    // Create a vector of integers
    typedef Vec(int) IntVec;
    IntVec vec = VecInit(&alloc);

    // Add some data
    int values[] = {10, 20, 30, 40, 50};
    for (int i = 0; i < 5; i++) {
        VecPushBackR(&vec, values[i]);
    }

    // Initial length should be 5
    bool result = (VecLen(&vec) == 5);

    // Resize to a smaller length
    VecResize(&vec, 3);

    // Length should now be 3
    result = result && (VecLen(&vec) == 3);

    // First 3 elements should be unchanged
    for (size i = 0; i < 3; i++) {
        result = result && (VecAt(&vec, i) == values[i]);
    }

    // Resize to a larger length
    VecResize(&vec, 8);

    // Length should now be 8
    result = result && (VecLen(&vec) == 8);

    // First 3 elements should still be the same
    for (size i = 0; i < 3; i++) {
        result = result && (VecAt(&vec, i) == values[i]);
    }

    // Clean up
    VecDeinit(&vec);

    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Test VecReserve function
bool test_vec_reserve(void) {
    WriteFmt("Testing VecReserve\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    // Create a vector of integers
    typedef Vec(int) IntVec;
    IntVec vec = VecInit(&alloc);

    // Initial capacity should be 0
    bool result = (VecCapacity(&vec) == 0);

    // Reserve space for 50 elements
    VecReserve(&vec, 50);

    // Capacity should now be at least 50
    result = result && (VecCapacity(&vec) >= 50);

    // Length should still be 0
    result = result && (VecLen(&vec) == 0);

    // Add some data
    int values[] = {10, 20, 30, 40, 50};
    for (int i = 0; i < 5; i++) {
        VecPushBackR(&vec, values[i]);
    }

    // Capacity should still be at least 50
    result = result && (VecCapacity(&vec) >= 50);

    // Length should now be 5
    result = result && (VecLen(&vec) == 5);

    // Reserve less space (should be a no-op)
    VecReserve(&vec, 20);

    // Capacity should still be at least 50
    result = result && (VecCapacity(&vec) >= 50);

    // Clean up
    VecDeinit(&vec);

    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Test VecClear function
bool test_vec_clear(void) {
    WriteFmt("Testing VecClear\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    // Create a vector of integers
    typedef Vec(int) IntVec;
    IntVec vec = VecInit(&alloc);

    // Add some data
    int values[] = {10, 20, 30, 40, 50};
    for (int i = 0; i < 5; i++) {
        VecPushBackR(&vec, values[i]);
    }

    // Initial length should be 5
    bool result = (VecLen(&vec) == 5);

    // Remember the capacity
    size original_capacity = VecCapacity(&vec);

    // Clear the vector
    VecClear(&vec);

    // Length should now be 0
    result = result && (VecLen(&vec) == 0);

    // Capacity should remain the same
    result = result && (VecCapacity(&vec) == original_capacity);

    // Data pointer should still be valid
    result = result && (VecBegin(&vec) != NULL);

    // Clean up
    VecDeinit(&vec);

    DefaultAllocatorDeinit(&alloc);
    return result;
}

// ===========================================================================
// resize_vec / reserve_pow2_vec mutation-hardening suite (from Vec.Mutants3)
// ===========================================================================

// 525:18 cxx_le_to_gt -- `new_size <= capacity` -> `new_size > capacity`.
bool test_resize_grow_branch_allocates(void) {
    WriteFmtLn("Testing resize grow enlarges capacity (525:18)");

    typedef Vec(u32) U32Vec;
    U32Vec vec = VecInit(&alloc);

    u32 seed[] = {1, 2};
    for (int i = 0; i < 2; i++) {
        VecPushBack(&vec, seed[i]);
    }

    // Grow well past the current capacity.
    bool ok = VecResize(&vec, 100);

    bool result = ok && (VecLen(&vec) == 100) && (VecCapacity(&vec) >= 100);

    VecDeinit(&vec);
    return result;
}

// 527:13 cxx_remove_void_call -- the shrink-path `remove_range_vec(...)` call
// is deleted.
bool test_resize_shrink_deinits_dropped(void) {
    WriteFmtLn("Testing resize shrink deinits dropped tail (527:13)");

    reset_deinit_state();

    typedef Vec(int) IntVec;
    IntVec vec = VecInitWithDeepCopy(NULL, counting_deinit, &alloc);

    int values[] = {10, 20, 30, 40};
    for (int i = 0; i < 4; i++) {
        VecPushBack(&vec, values[i]);
    }

    VecResize(&vec, 1);

    bool result = (VecLen(&vec) == 1) && (g_deinit_count == 3);

    VecDeinit(&vec);
    return result;
}

// 531:14 cxx_replace_scalar_call -- the grow-path `reserve_pow2_vec(...)`
// result is replaced by a constant.
bool test_resize_grow_reserves_capacity(void) {
    WriteFmtLn("Testing resize grow reserves backing capacity (531:14)");

    typedef Vec(u64) U64Vec;
    U64Vec vec = VecInit(&alloc);

    u64 seed[] = {1, 2};
    for (int i = 0; i < 2; i++) {
        VecPushBack(&vec, seed[i]);
    }

    VecResize(&vec, 64);

    // Whichever way the constant forces the branch, a real grow must leave
    // capacity large enough to hold the new length.
    bool result = (VecCapacity(&vec) >= 64);

    VecDeinit(&vec);
    return result;
}

// 534:21 cxx_assign_const -- grow-path `vec->length = new_size` -> const.
bool test_resize_grow_sets_exact_length(void) {
    WriteFmtLn("Testing resize grow sets exact length (534:21)");

    typedef Vec(u32) U32Vec;
    U32Vec vec = VecInit(&alloc);

    u32 seed[] = {7, 8};
    for (int i = 0; i < 2; i++) {
        VecPushBack(&vec, seed[i]);
    }

    VecResize(&vec, 50);

    bool result = (VecLen(&vec) == 50);

    VecDeinit(&vec);
    return result;
}

// 145:10 cxx_init_const -- reserve_pow2_vec's doubling seed `size n2 = 1` ->
// `n2 = 42`.
bool test_reserve_pow2_seed(void) {
    WriteFmtLn("Testing reserve_pow2 rounds to a power of two (145:10)");

    typedef Vec(u32) U32Vec;
    U32Vec vec = VecInit(&alloc);

    // Empty -> grow branch -> reserve_pow2_vec(10). Real: next pow2 >= 10 == 16.
    VecResize(&vec, 10);

    bool result = (VecLen(&vec) == 10) && (VecCapacity(&vec) == 16);

    VecDeinit(&vec);
    return result;
}

// ===========================================================================
// reserve_vec mutation-hardening suite (from Vec.Mutants4)
// ===========================================================================
//
// reserve_vec's MemSet is the only thing that zeroes the freshly-GROWN slots
// when an existing buffer is enlarged in place. A capacity-8 Vec(int) lives in
// the heap's 64-byte size class (16 int slots); growing it to capacity 12 stays
// in the same class, so AllocatorRealloc resizes in place and the buffer is the
// SAME memory -- whatever was in the now-in-range slots persists unless
// reserve_vec zeroes it.
static bool reserve_zero_grown_region(int probe_index) {
    HeapAllocator heap = HeapAllocatorInit();

    typedef Vec(int) IntVec;
    IntVec vec = VecInit(&heap);

    // Capacity 8 -> 64-byte class (16 int slots). Poison the whole class slot.
    VecReserve(&vec, 8);
    int *base = VecBegin(&vec);
    for (int i = 0; i < 16; i++) {
        base[i] = (int)0xAAAAAAAA;
    }

    // Live data in [0,8); resize re-zeroes only the sentinel slot.
    VecResize(&vec, 8);
    for (u64 i = 0; i < VecLen(&vec); i++) {
        VecAt(&vec, i) = 0x11111111;
    }

    // In-place grow to capacity 12 (same 64-byte class). reserve_vec must zero
    // the newly in-range region [8,12).
    VecReserve(&vec, 12);
    VecResize(&vec, 12);

    // Live data below the old capacity is preserved; the probed grown slot must
    // be zeroed by reserve_vec.
    bool result = (VecAt(&vec, 0) == 0x11111111 && VecAt(&vec, probe_index) == 0);

    VecDeinit(&vec);
    HeapAllocatorDeinit(&heap);
    return result;
}

// 125:67 cxx_mul_to_div (grown-region zero byte count collapses to ~0).
bool test_reserve_zero_low_slot(void) {
    WriteFmt("Testing reserve zeroes grown region (low slot)\n");
    return reserve_zero_grown_region(9);
}

// 125:72 cxx_add_to_sub (`(n + 1 - old_capacity)` -> `(n - 1 - old_capacity)`).
bool test_reserve_zero_high_slot(void) {
    WriteFmt("Testing reserve zeroes grown region (high slot)\n");
    return reserve_zero_grown_region(11);
}

// 112:42 cxx_gt_to_ge -- multiplicative-overflow guard boundary.
bool test_reserve_overflow_boundary_returns_false(void) {
    WriteFmt("Testing reserve at the overflow boundary returns false (112:42)\n");

    HeapAllocator heap = HeapAllocatorInit(); // alignment 1 -> aligned_size == 1

    typedef Vec(char) CharVec;
    CharVec vec = VecInit(&heap);

    // n + 1 == SIZE_MAX == (size)-1 / 1: the exact boundary. Real code does not
    // abort; the giant realloc fails and reserve reports false. The mutant
    // aborts here.
    bool ok = VecReserve(&vec, (size)-1 - 1);

    bool result = (ok == false) && (VecLen(&vec) == 0);

    VecDeinit(&vec);
    HeapAllocatorDeinit(&heap);
    return result;
}

// ===========================================================================
// clear_vec mutation-hardening suite (from Vec.Mutants5)
// ===========================================================================

// clear_vec, MemSet-scrub branch (no copy_deinit): after clearing a non-empty
// Str the buffer -- including the NUL sentinel -- must be zeroed.
bool test_clear_str_scrubs_sentinel(void) {
    WriteFmt("Testing VecClear scrubs Str sentinel byte\n");

    DefaultAllocator local = DefaultAllocatorInit();

    Str s = StrInitFromZstr("hello", &local);

    StrClear(&s);

    bool result = (StrLen(&s) == 0) && (ZstrCompare(StrBegin(&s), "") == 0);

    StrDeinit(&s);
    DefaultAllocatorDeinit(&local);
    return result;
}

// clear_vec, MemSet-scrub branch with the `capacity + 1` -> `capacity - 1`
// mutation. A one-character Str has capacity == 1 exactly.
bool test_clear_str_cap1_scrubs_sentinel(void) {
    WriteFmt("Testing VecClear scrubs single-char Str (capacity 1)\n");

    DefaultAllocator local = DefaultAllocatorInit();

    Str s = StrInitFromZstr("x", &local);

    StrClear(&s);

    bool result = (StrLen(&s) == 0) && (ZstrCompare(StrBegin(&s), "") == 0);

    StrDeinit(&s);
    DefaultAllocatorDeinit(&local);
    return result;
}

// clear_vec @ 84:18 / 84:20 (scrub MemSet stride forced to 42).
bool test_clear_large_char_vec_scrub_stride(void) {
    WriteFmt("Testing VecClear scrub stride on a large char vector\n");

    DefaultAllocator local = DefaultAllocatorInit();

    typedef Vec(char) CharVec;
    CharVec vec = VecInit(&local);

    // ~16 KiB of live chars: a 42x over-write spans ~688 KiB, far past the
    // allocation, so the mutated MemSet runs off the buffer and faults.
    const size N = 16384;
    VecReserve(&vec, N);
    for (size i = 0; i < N; i++) {
        char c = (char)('a' + (int)(i % 26));
        VecPushBackR(&vec, c);
    }

    VecClear(&vec);

    bool result = (VecLen(&vec) == 0);

    VecDeinit(&vec);
    DefaultAllocatorDeinit(&local);
    return result;
}

// clear_vec, copy_deinit loop: clearing must invoke the destructor exactly
// once per live element.
bool test_clear_runs_deinit_once_per_element(void) {
    WriteFmt("Testing VecClear invokes copy_deinit once per element\n");

    DefaultAllocator local = DefaultAllocatorInit();

    typedef Vec(int) IntVec;
    IntVec vec      = VecInit(&local);
    vec.copy_deinit = (GenericCopyDeinit)clear_counting_deinit;

    int values[] = {10, 20, 30};
    for (int i = 0; i < 3; i++) {
        VecPushBackR(&vec, values[i]);
    }

    g_deinit_calls = 0;
    VecClear(&vec);

    bool result = (g_deinit_calls == 3) && (VecLen(&vec) == 0);

    // Drop the hook so the final teardown does not re-run the counter.
    vec.copy_deinit = NULL;
    VecDeinit(&vec);
    DefaultAllocatorDeinit(&local);
    return result;
}

// ===========================================================================
// reduce_space_vec mutation-hardening suite (from Vec.Mutants6)
// ===========================================================================

// reduce_space_vec @ 165:18 (aligned_size := const 42 in the length>0 path).
bool test_reduce_nonempty_preallocated_keeps_data(void) {
    WriteFmt("Testing reduce keeps element data for wide elements\n");

    DefaultAllocator local = DefaultAllocatorInit();

    typedef struct {
        u32 tag;
        u8  pad[60];
    } Wide; // sizeof == 64
    typedef Vec(Wide) WideVec;
    WideVec vec = VecInit(&local);

    const u32 N = 60000;
    VecReserve(&vec, N);
    for (u32 i = 0; i < N; i++) {
        Wide w = {0};
        w.tag  = 100 + i;
        VecPushBackR(&vec, w);
    }

    VecMustTryReduceSpace(&vec);

    // Real: every element survives the shrink.
    bool result = (VecLen(&vec) == N);
    for (u32 i = 0; i < N; i++) {
        result = result && (VecAt(&vec, i).tag == 100 + i);
    }

    VecDeinit(&vec);
    DefaultAllocatorDeinit(&local);
    return result;
}

// reduce_space_vec @ 166:21 (length == 0 -> length != 0).
bool test_reduce_nonempty_keeps_length(void) {
    WriteFmt("Testing reduce preserves length of a non-empty vec\n");

    DefaultAllocator local = DefaultAllocatorInit();

    typedef Vec(int) IntVec;
    IntVec vec = VecInit(&local);

    VecReserve(&vec, 64);
    int values[] = {10, 20, 30, 40, 50};
    for (int i = 0; i < 5; i++) {
        VecPushBackR(&vec, values[i]);
    }

    VecMustTryReduceSpace(&vec);

    // Real: length stays 5.
    bool result = (VecLen(&vec) == 5);

    VecDeinit(&vec);
    DefaultAllocatorDeinit(&local);
    return result;
}

// reduce_space_vec @ 169:23 (capacity := const in the length==0 path).
bool test_reduce_empty_capacity_zero(void) {
    WriteFmt("Testing reduce of an empty vec zeroes capacity\n");

    DefaultAllocator local = DefaultAllocatorInit();

    typedef Vec(int) IntVec;
    IntVec vec = VecInit(&local);

    VecReserve(&vec, 50); // length stays 0, capacity > 0, data != NULL

    VecMustTryReduceSpace(&vec);

    // Real: capacity 0.
    bool result = (VecCapacity(&vec) == 0);

    VecDeinit(&vec);
    DefaultAllocatorDeinit(&local);
    return result;
}

// reduce_space_vec @ 170:23 (length := const in the length==0 path).
bool test_reduce_empty_length_zero(void) {
    WriteFmt("Testing reduce of an empty vec keeps length zero\n");

    DefaultAllocator local = DefaultAllocatorInit();

    typedef Vec(int) IntVec;
    IntVec vec = VecInit(&local);

    VecReserve(&vec, 50); // length stays 0

    VecMustTryReduceSpace(&vec);

    // Real: length 0.
    bool result = (VecLen(&vec) == 0);

    VecDeinit(&vec);
    DefaultAllocatorDeinit(&local);
    return result;
}

// reduce_space_vec @ 188:23 (capacity := vec->length replaced by a const).
bool test_reduce_shrinks_capacity_to_length(void) {
    WriteFmt("Testing reduce shrinks capacity to length\n");

    DefaultAllocator local = DefaultAllocatorInit();

    typedef Vec(int) IntVec;
    IntVec vec = VecInit(&local);

    VecReserve(&vec, 100);
    int values[] = {10, 20, 30, 40, 50}; // length 5, deliberately != 42
    for (int i = 0; i < 5; i++) {
        VecPushBackR(&vec, values[i]);
    }

    VecMustTryReduceSpace(&vec);

    // Real: capacity == length == 5.
    bool result = (VecCapacity(&vec) == 5);

    VecDeinit(&vec);
    DefaultAllocatorDeinit(&local);
    return result;
}

// ===========================================================================
// validate_vec structural-guard deadend (from Vec.Mutants5)
// ===========================================================================

// Deadend: validate_vec must run the structural body, which aborts when
// length > capacity.
bool test_validate_rejects_length_over_capacity(void) {
    WriteFmt("Testing validate rejects length > capacity\n");

    DefaultAllocator local = DefaultAllocatorInit();

    typedef Vec(int) IntVec;
    IntVec vec = VecInit(&local);

    int values[] = {10, 20, 30};
    for (int i = 0; i < 3; i++) {
        VecPushBackR(&vec, values[i]);
    }

    // Corrupt the invariant and force the validated bit so the structural
    // body actually runs on the next validating call.
    vec.length   = vec.capacity + 1;
    vec.__magic |= MAGIC_VALIDATED_BIT;

    // VecReserve validates first; with n == 0 it would otherwise be a
    // no-op, so the only observable effect is the structural abort.
    VecReserve(&vec, 0);

    // Unreachable on real code: the structural validator LOG_FATALs above.
    VecDeinit(&vec);
    DefaultAllocatorDeinit(&local);
    return false;
}

// Deadend: a reserve whose `capacity * item_size` overflows `size` must abort
// rather than wrap and under-allocate. Pick a large element stride and an `n`
// whose product with that stride exceeds `size`; the guard fires before any
// allocation, so no huge buffer is ever requested. Without the guard this
// wraps to a small allocation and later element writes run off the buffer.
bool test_vec_reserve_capacity_overflow_aborts(void) {
    WriteFmt("Testing VecReserve capacity*item_size overflow aborts\n");

    DefaultAllocator alloc = DefaultAllocatorInit();

    // 2 MiB element stride; aligned_size is at least sizeof(Big).
    typedef struct {
        u8 bytes[(size)1 << 21];
    } Big;
    typedef Vec(Big) BigVec;
    BigVec vec = VecInit(&alloc);

    // (size)1<<43 * 2^21 == 2^64 -> wraps past `size`. Well under the
    // reserve_pow2 2^63 cap, and VecReserve hands `n` straight through.
    VecReserve(&vec, (size)1 << 43);

    // Unreachable: the guard LOG_FATALs above.
    VecDeinit(&vec);
    DefaultAllocatorDeinit(&alloc);
    return false;
}

// Main function that runs all tests
int main(void) {
    WriteFmt("[INFO] Starting Vec.Memory tests\n\n");

    alloc = DefaultAllocatorInit();

    // Array of test functions
    TestFunction tests[] = {
        test_vec_try_reduce_space,
        test_vec_resize,
        test_vec_reserve,
        test_vec_clear,
        // resize_vec / reserve_pow2_vec mutation-hardening (Vec.Mutants3)
        test_resize_grow_branch_allocates,
        test_resize_shrink_deinits_dropped,
        test_resize_grow_reserves_capacity,
        test_resize_grow_sets_exact_length,
        test_reserve_pow2_seed,
        // reserve_vec mutation-hardening (Vec.Mutants4)
        test_reserve_zero_low_slot,
        test_reserve_zero_high_slot,
        test_reserve_overflow_boundary_returns_false,
        // clear_vec mutation-hardening (Vec.Mutants5)
        test_clear_str_scrubs_sentinel,
        test_clear_str_cap1_scrubs_sentinel,
        test_clear_large_char_vec_scrub_stride,
        test_clear_runs_deinit_once_per_element,
        // reduce_space_vec mutation-hardening (Vec.Mutants6)
        test_reduce_nonempty_preallocated_keeps_data,
        test_reduce_nonempty_keeps_length,
        test_reduce_empty_capacity_zero,
        test_reduce_empty_length_zero,
        test_reduce_shrinks_capacity_to_length
    };

    TestFunction deadend_tests[] = {
        test_vec_reserve_capacity_overflow_aborts,
        test_validate_rejects_length_over_capacity
    };

    int total_tests   = sizeof(tests) / sizeof(tests[0]);
    int deadend_count = sizeof(deadend_tests) / sizeof(deadend_tests[0]);

    // Run all tests using the centralized test driver
    int rc = run_test_suite(tests, total_tests, deadend_tests, deadend_count, "Vec.Memory");
    DefaultAllocatorDeinit(&alloc);
    return rc;
}
