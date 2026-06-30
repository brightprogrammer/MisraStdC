/// file      : tests/std/allocator.mutants1.c
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Mutation-hardening tests for the generic allocator DISPATCH layer
/// (`Source/Misra/Std/Allocator.c`): the retry-policy arithmetic in
/// `allocator_attempt_limit`, the structural guards in
/// `ValidateAllocator`, the retry loops inside `AllocatorAlloc_dyn` /
/// `AllocatorRemap_dyn`, the `AllocatorFree_dyn` validation order, and
/// `AllocatorResetStats`' preserve-outstanding accounting.
///
/// These do NOT go through a concrete typed allocator's _Generic arm:
/// they construct a plain `Allocator` value (or corrupt a real
/// HeapAllocator's base) and drive it through the `Allocator *` dyn
/// wrappers so the dispatch code itself is the unit under test.
///
/// Mull faithful model: a scalar value-substitution becomes literal 42
/// (truthy) with side effects preserved; operator swaps replace the
/// operator. The retry-count mutants are only observable by counting
/// the number of times the vtable function pointer fires, so the mock
/// allocators below tally their invocations in file-static counters and
/// the tests assert the exact count, not just the returned pointer.

#include <Misra/Std/Allocator.h>
#include <Misra/Std/Allocator/Heap.h>
#include <Misra/Std/Log.h>

#include "../../Util/TestRunner.h"

// =============================================================================
// Mock allocator: a hand-built Allocator base whose `allocate` / `remap`
// fail (return NULL) a controlled number of times before succeeding, and
// tally every invocation. Used to make the retry-count mutants observable.

static size mock_alloc_calls;  // # times the mock allocate fired
static size mock_remap_calls;  // # times the mock remap fired
static size mock_fail_first_n; // fail this many calls, then succeed
static u8   mock_backing[64];  // a real buffer to hand back on "success"

static void *mock_allocate(Allocator *self, size bytes, i8 zeroed) {
    (void)self;
    (void)bytes;
    (void)zeroed;
    mock_alloc_calls++;
    if (mock_alloc_calls <= mock_fail_first_n) {
        return NULL;
    }
    return mock_backing;
}

static void *mock_remap(Allocator *self, void *ptr, size new_size) {
    (void)self;
    (void)ptr;
    (void)new_size;
    mock_remap_calls++;
    if (mock_remap_calls <= mock_fail_first_n) {
        return NULL;
    }
    return mock_backing;
}

static i8 mock_resize(Allocator *self, void *ptr, size new_size) {
    (void)self;
    (void)ptr;
    (void)new_size;
    return 0;
}

static size mock_deallocate(Allocator *self, void *ptr) {
    (void)self;
    (void)ptr;
    return 0;
}

// A fully-valid mock Allocator with the given retry effort/limit. The
// fn pointers are real (so the vtable derefs in the dyn wrappers all
// succeed) and __magic / alignment pass ValidateAllocator.
static Allocator mock_make(AllocatorEffort effort, u32 retry_limit) {
    Allocator a   = {0};
    a.allocate    = mock_allocate;
    a.resize      = mock_resize;
    a.remap       = mock_remap;
    a.deallocate  = mock_deallocate;
    a.alignment   = 8;
    a.effort      = effort;
    a.retry_limit = retry_limit;
    a.__magic     = 0xA11Cu;
    return a;
}

static void mock_reset(size fail_first_n) {
    mock_alloc_calls  = 0;
    mock_remap_calls  = 0;
    mock_fail_first_n = fail_first_n;
}

// =============================================================================
// Retry arithmetic / attempt count  (30, 59, 61, 85, 87, 89)
//
// allocator_attempt_limit(RETRY, retry_limit=N) == N + 1. So with
// effort=RETRY and retry_limit=2 the dyn wrapper makes exactly 3 attempts.

// 30:65 (retry_limit + 1 -> retry_limit - 1): with retry_limit=2 the real
// limit is 3 attempts. A mock that fails 2 times then succeeds returns
// non-NULL on the real 3rd attempt; the mutant allows only 1 attempt
// (2-1) so all attempts fail and it returns NULL.
static bool test_al_alloc_retry_limit_plus_one(void) {
    Allocator a = mock_make(ALLOCATOR_EFFORT_RETRY, 2);
    mock_reset(2); // fail twice, succeed on the 3rd
    void *p = AllocatorAlloc(&a, 16, false);
    return p != NULL && mock_alloc_calls == 3;
}

// 59:11 / 59:22 (attempts -> 42  and  allocator_attempt_limit(self) -> 42):
// with retry_limit=2 the real attempt count is 3. A mock that always
// fails calls allocate exactly 3 times on real code; a mutant that
// substitutes 42 would call it 42 times. Asserting the exact count
// kills both 59 mutants.
static bool test_al_alloc_attempts_count_exact(void) {
    Allocator a = mock_make(ALLOCATOR_EFFORT_RETRY, 2);
    mock_reset(1000); // always fail across any plausible attempt count
    void *p = AllocatorAlloc(&a, 16, false);
    return p == NULL && mock_alloc_calls == 3;
}

// 61:36 (try_idx < attempts -> <=): the <= variant runs one extra
// iteration, so an always-fail mock would be called attempts+1 times.
// Real code calls it exactly `attempts`.  Shares the exact-count assert.
static bool test_al_alloc_loop_bound_exact(void) {
    Allocator a = mock_make(ALLOCATOR_EFFORT_RETRY, 3); // 4 attempts
    mock_reset(1000);
    void *p = AllocatorAlloc(&a, 16, false);
    return p == NULL && mock_alloc_calls == 4;
}

// 61:55 (try_idx++ -> try_idx--): after the first failed attempt the
// counter decrements and (unsigned) wraps past `attempts`, so the loop
// exits immediately and the wrapper returns NULL. Real code retries and
// succeeds on the second attempt. A fail-once-then-succeed mock makes
// the two diverge by RESULT (non-NULL vs NULL) and by call count.
static bool test_al_alloc_loop_increment(void) {
    Allocator a = mock_make(ALLOCATOR_EFFORT_RETRY, 3); // 4 attempts
    mock_reset(1);                                      // fail once, then succeed
    void *p = AllocatorAlloc(&a, 16, false);
    return p != NULL && mock_alloc_calls == 2;
}

// 85:11 / 85:22 (remap attempts -> 42 both ways): same as the alloc
// attempt-count test but through AllocatorRemap_dyn. new_size != 0 so
// the break-on-zero clause does not short-circuit; an always-fail remap
// mock is called exactly `attempts` (3) times on real code.
static bool test_al_remap_attempts_count_exact(void) {
    Allocator a = mock_make(ALLOCATOR_EFFORT_RETRY, 2); // 3 attempts
    mock_reset(1000);
    void *p = AllocatorRemap(&a, mock_backing, 16);
    return p == NULL && mock_remap_calls == 3;
}

// 87:36 (remap try_idx < attempts -> <=): off-by-one extra remap call.
static bool test_al_remap_loop_bound_exact(void) {
    Allocator a = mock_make(ALLOCATOR_EFFORT_RETRY, 3); // 4 attempts
    mock_reset(1000);
    void *p = AllocatorRemap(&a, mock_backing, 16);
    return p == NULL && mock_remap_calls == 4;
}

// 87:55 (remap try_idx++ -> --): unsigned wrap exits the loop after the
// first failure -> NULL on the mutant, success on real code.
static bool test_al_remap_loop_increment(void) {
    Allocator a = mock_make(ALLOCATOR_EFFORT_RETRY, 3); // 4 attempts
    mock_reset(1);                                      // fail once, then succeed
    void *p = AllocatorRemap(&a, mock_backing, 16);
    return p != NULL && mock_remap_calls == 2;
}

// 89:33 (new_ptr || new_size == 0  ->  new_ptr || new_size != 0): with
// new_size == 0 and a remap mock that returns NULL, real code breaks
// immediately (==0 is true) -> exactly ONE remap call. The mutant turns
// the clause into `new_size != 0` which is false for new_size==0, so it
// keeps retrying the full attempt count. Asserting call-count == 1 kills
// it. (effort=RETRY, retry_limit=3 -> 4 attempts on the mutant.)
static bool test_al_remap_break_on_zero_size(void) {
    Allocator a = mock_make(ALLOCATOR_EFFORT_RETRY, 3);
    mock_reset(1000); // remap always returns NULL
    void *p = AllocatorRemap(&a, mock_backing, 0);
    return p == NULL && mock_remap_calls == 1;
}

// =============================================================================
// ValidateAllocator structural guards via the dyn wrappers
// (51, 57, 71, 83, 100, 106) -- deadend tests: each drives a MOCK
// Allocator whose magic (or alignment) is corrupt through one dyn
// wrapper. The mock's fn pointers do NOT self-validate (unlike the typed
// HeapAllocator, which re-checks its own magic and would abort on either
// the real or mutated dispatch), so the ONLY thing standing between the
// corrupt base and a clean return is the dispatch layer's
// `ValidateAllocator` call. Removing it (the mutant) lets the mock's
// fn pointer run and return normally -> the deadend reaches `return
// false` and is scored a failure. Real code aborts in ValidateAllocator.

// A mock base with zero magic: fails ValidateAllocator's "__magic == 0"
// check (57/71/83/100/106). Fn pointers valid + alignment a power of two
// so ONLY the magic guard trips.
static Allocator mock_make_bad_magic(void) {
    Allocator a = mock_make(ALLOCATOR_EFFORT_ONCE, 0);
    a.__magic   = 0u;
    return a;
}

// 57: AllocatorAlloc_dyn must ValidateAllocator first.
static bool test_al_alloc_validates_magic(void) {
    Allocator a = mock_make_bad_magic();
    mock_reset(0);
    (void)AllocatorAlloc(&a, 16, false); // real -> LOG_FATAL; mutant returns
    return false;
}

// 71: AllocatorResize_dyn must ValidateAllocator first. Non-NULL ptr +
// non-zero size clears the degenerate-args early-out, so the validate is
// the only abort point.
static bool test_al_resize_validates_magic(void) {
    Allocator a = mock_make_bad_magic();
    (void)AllocatorResize(&a, mock_backing, 16); // real -> LOG_FATAL
    return false;
}

// 83: AllocatorRemap_dyn must ValidateAllocator first.
static bool test_al_remap_validates_magic(void) {
    Allocator a = mock_make_bad_magic();
    mock_reset(0);
    (void)AllocatorRemap(&a, mock_backing, 16); // real -> LOG_FATAL
    return false;
}

// 100: AllocatorFree_dyn must ValidateAllocator (for a non-NULL ptr; the
// NULL early-out comes first, so ptr must be non-NULL to reach Validate).
static bool test_al_free_validates_magic(void) {
    Allocator a = mock_make_bad_magic();
    AllocatorFree(&a, mock_backing); // real -> Validate -> LOG_FATAL
    return false;
}

#if FEATURE_ALLOC_STATS
// 106: AllocatorResetStats must ValidateAllocator first.
static bool test_al_reset_stats_validates_magic(void) {
    Allocator a = mock_make_bad_magic();
    AllocatorResetStats(&a); // real -> LOG_FATAL
    return false;
}
#endif

// 51: the alignment power-of-two check. Mock with valid magic but
// alignment=3 (not a power of two). The mutant replaces the whole
// `allocator_alignment_is_pow2(self->alignment)` call with truthy 42, so
// `!42` is false and the abort is skipped -> the mock allocate runs and
// returns -> deadend fails. Real code aborts in the alignment guard.
static bool test_al_validate_alignment_pow2(void) {
    Allocator a = mock_make(ALLOCATOR_EFFORT_ONCE, 0);
    a.alignment = 3u;                    // not a power of two
    mock_reset(0);
    (void)AllocatorAlloc(&a, 16, false); // real -> LOG_FATAL via Validate
    return false;
}

// =============================================================================
// AllocatorResetStats accounting (107, 111, 112) -- requires stats.

#if FEATURE_ALLOC_STATS
// Allocate, then reset: every counter zeroes EXCEPT bytes_in_use and
// peak_bytes_in_use, which must be preserved at the outstanding value.
//
// 107 (in_use captured from stats.bytes_in_use -> 42): the captured value
//     must equal the real outstanding bytes, not 42.
// 111 (stats.bytes_in_use = in_use -> = 42): post-reset bytes_in_use must
//     equal the preserved outstanding value, not 42.
// 112 (stats.peak_bytes_in_use = in_use -> = 42): same for peak.
static bool test_al_reset_stats_preserves_outstanding(void) {
    HeapAllocator heap  = HeapAllocatorInit();
    Allocator    *alloc = ALLOCATOR_OF(&heap);

    void *p = AllocatorAlloc(alloc, 64, false);
    if (!p) {
        HeapAllocatorDeinit(&heap);
        return false;
    }
    u64 outstanding = AllocatorBytesInUse(alloc);

    AllocatorResetStats(alloc);

    bool ok = (outstanding != 0u) &&                          // there really was usage
              (AllocatorBytesInUse(alloc) == outstanding) &&  // 107 + 111
              (AllocatorPeakBytesInUse(alloc) == outstanding) // 107 + 112
           && (AllocatorAllocations(alloc) == 0u)             // other counters zeroed
           && (AllocatorBytesRequested(alloc) == 0u);

    AllocatorFree(alloc, p);
    HeapAllocatorDeinit(&heap);
    return ok;
}
#endif

int main(void) {
    TestFunction normal[] = {
        // Retry arithmetic / attempt counts
        test_al_alloc_retry_limit_plus_one,
        test_al_alloc_attempts_count_exact,
        test_al_alloc_loop_bound_exact,
        test_al_alloc_loop_increment,
        test_al_remap_attempts_count_exact,
        test_al_remap_loop_bound_exact,
        test_al_remap_loop_increment,
        test_al_remap_break_on_zero_size,
#if FEATURE_ALLOC_STATS
        test_al_reset_stats_preserves_outstanding,
#endif
    };
    TestFunction deadend[] = {
        test_al_alloc_validates_magic,
        test_al_resize_validates_magic,
        test_al_remap_validates_magic,
        test_al_free_validates_magic,
#if FEATURE_ALLOC_STATS
        test_al_reset_stats_validates_magic,
#endif
        test_al_validate_alignment_pow2,
    };
    return run_test_suite(
        normal,
        (int)(sizeof(normal) / sizeof(normal[0])),
        deadend,
        (int)(sizeof(deadend) / sizeof(deadend[0])),
        "Allocator.Dispatch"
    );
}
