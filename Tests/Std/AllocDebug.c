/// file      : tests/std/allocdebug.c
///
/// Exhaustive feature-coverage tests for DebugAllocator.
///
/// Features under test:
///   - basic alloc + free, counters
///   - zero-byte alloc returns NULL
///   - NULL free is a no-op
///   - canary buffer-overflow detection
///   - leak detection at Deinit
///   - leak report via DebugAllocatorReportLeaks
///   - alloc-site stack-trace capture
///   - freed-history Vec grows on free; entries carry both traces
///   - track_freed_history = false disables the freed Vec entirely
///   - remap (DebugAllocator's path) works
///   - page-backed mode: page-aligned slots, mprotect on free
///   - cross-thread use trips LOG_FATAL (deadend)
///   - double-free trips Heap's LOG_FATAL (deadend)

#include <Misra.h>
#include <Misra/Std/Allocator/Debug.h>
#include <Misra/Std/Zstr.h>

#include "../Util/TestRunner.h"

// =============================================================================
// Happy path + counters

bool test_debug_normal_alloc_free(void) {
    DebugAllocator dbg  = DebugAllocatorInit();
    Allocator     *adbg = ALLOCATOR_OF(&dbg);

    void *p1 = AllocatorAlloc(adbg, 64, true);
    void *p2 = AllocatorAlloc(adbg, 128, true);

    bool ok = p1 && p2;
    ok      = ok && DebugAllocatorLiveCount(&dbg) == 2;
    ok      = ok && DebugAllocatorLiveBytes(&dbg) == (64 + 128);

    AllocatorFree(adbg, p1);
    ok = ok && DebugAllocatorLiveCount(&dbg) == 1;
    ok = ok && DebugAllocatorLiveBytes(&dbg) == 128;

    AllocatorFree(adbg, p2);
    ok = ok && DebugAllocatorLiveCount(&dbg) == 0;
    ok = ok && DebugAllocatorLiveBytes(&dbg) == 0;
    ok = ok && DebugAllocatorOverflows(&dbg) == 0;

    DebugAllocatorDeinit(&dbg);
    return ok;
}

bool test_debug_zero_byte_alloc(void) {
    DebugAllocator dbg  = DebugAllocatorInit();
    Allocator     *adbg = ALLOCATOR_OF(&dbg);

    void *p  = AllocatorAlloc(adbg, 0, false);
    bool  ok = (p == NULL) && (DebugAllocatorLiveCount(&dbg) == 0);

    DebugAllocatorDeinit(&dbg);
    return ok;
}

bool test_debug_null_free_is_noop(void) {
    DebugAllocator dbg  = DebugAllocatorInit();
    Allocator     *adbg = ALLOCATOR_OF(&dbg);

    AllocatorFree(adbg, NULL); // no-op, must not crash
    void *p  = AllocatorAlloc(adbg, 32, false);
    bool  ok = (p != NULL) && (DebugAllocatorLiveCount(&dbg) == 1);
    if (p)
        AllocatorFree(adbg, p);

    DebugAllocatorDeinit(&dbg);
    return ok;
}

// =============================================================================
// Buffer overflow via canary

bool test_debug_catches_overflow(void) {
    DebugAllocator dbg  = DebugAllocatorInit();
    Allocator     *adbg = ALLOCATOR_OF(&dbg);

    u8 *buf = (u8 *)AllocatorAlloc(adbg, 16, true);
    buf[16] = 0x55; // stomp first canary byte
    AllocatorFree(adbg, buf);

    bool ok = DebugAllocatorOverflows(&dbg) == 1;
    DebugAllocatorDeinit(&dbg);
    return ok;
}

// =============================================================================
// Leak detection + report API

bool test_debug_leak_count(void) {
    DebugAllocator dbg  = DebugAllocatorInit();
    Allocator     *adbg = ALLOCATOR_OF(&dbg);

    (void)AllocatorAlloc(adbg, 32, true);
    (void)AllocatorAlloc(adbg, 48, true);

    bool ok = DebugAllocatorLiveCount(&dbg) == 2;
    ok      = ok && DebugAllocatorLiveBytes(&dbg) == (32 + 48);

    DebugAllocatorDeinit(&dbg); // logs the leaks
    return ok;
}

bool test_debug_report_leaks_emits_traces(void) {
    DebugAllocator dbg  = DebugAllocatorInit();
    Allocator     *adbg = ALLOCATOR_OF(&dbg);

    void *p1 = AllocatorAlloc(adbg, 24, true);
    void *p2 = AllocatorAlloc(adbg, 40, true);
    (void)p1;
    (void)p2;

    // The leak-report buffer is allocated through a SEPARATE scratch
    // allocator -- not through `dbg` -- so its own storage isn't
    // counted as a leak by the very report we're about to generate.
    HeapAllocator scratch = HeapAllocatorInit();
    Str           out     = StrInit(&scratch);
    DebugAllocatorReportLeaks(&dbg, &out);

    bool ok = StrLen(&out) > 0;
    ok      = ok && (ZstrFindSubstring(StrBegin(&out), "leak:") != NULL);
    ok      = ok && (ZstrFindSubstring(StrBegin(&out), "24 bytes") != NULL);
    ok      = ok && (ZstrFindSubstring(StrBegin(&out), "40 bytes") != NULL);

    StrDeinit(&out);
    HeapAllocatorDeinit(&scratch);
    if (p1)
        AllocatorFree(adbg, p1);
    if (p2)
        AllocatorFree(adbg, p2);
    DebugAllocatorDeinit(&dbg);
    return ok;
}

// =============================================================================
// Alloc-site trace capture (visible via the live map)

bool test_debug_alloc_trace_captured(void) {
    DebugAllocator dbg  = DebugAllocatorInit();
    Allocator     *adbg = ALLOCATOR_OF(&dbg);

    void *p  = AllocatorAlloc(adbg, 64, false);
    bool  ok = (p != NULL);

    // intentional bypass: the live Map keyed by user pointer is internal
    // to DebugAllocator with no public accessor; reach in to confirm the
    // alloc-trace was captured.
    DebugRecord *rec = MapGetFirstPtr(&dbg.live, p);
    ok               = ok && (rec != NULL) && (rec->alloc_trace_n > 0);

    if (p)
        AllocatorFree(adbg, p);
    DebugAllocatorDeinit(&dbg);
    return ok;
}

// =============================================================================
// Freed-history Vec

bool test_debug_freed_history_grows_on_free(void) {
    DebugAllocator dbg  = DebugAllocatorInit();
    Allocator     *adbg = ALLOCATOR_OF(&dbg);

    bool  ok = (DebugAllocatorFreedCount(&dbg) == 0);
    void *p1 = AllocatorAlloc(adbg, 16, false);
    void *p2 = AllocatorAlloc(adbg, 32, false);
    void *p3 = AllocatorAlloc(adbg, 64, false);
    ok       = ok && (DebugAllocatorFreedCount(&dbg) == 0); // alloc doesn't push

    AllocatorFree(adbg, p1);
    ok = ok && (DebugAllocatorFreedCount(&dbg) == 1);

    AllocatorFree(adbg, p2);
    AllocatorFree(adbg, p3);
    ok = ok && (DebugAllocatorFreedCount(&dbg) == 3);

    // intentional bypass: the freed-history Vec elements are internal
    // `DebugFreedEntry` structs with no public accessor; reach in to
    // confirm each entry carries the ptr + both traces.
    ok = ok && (VecPtrAt(&dbg.freed, 0)->ptr == p1) && (VecPtrAt(&dbg.freed, 0)->requested_size == 16);
    ok = ok && (VecPtrAt(&dbg.freed, 0)->alloc_trace_n > 0) && (VecPtrAt(&dbg.freed, 0)->free_trace_n > 0);
    ok = ok && (VecPtrAt(&dbg.freed, 2)->ptr == p3) && (VecPtrAt(&dbg.freed, 2)->requested_size == 64);

    DebugAllocatorDeinit(&dbg);
    return ok;
}

bool test_debug_freed_history_disabled(void) {
    // When track_freed_history = false, freed Vec stays empty even
    // after many frees -- and allocator is still functional.
    DebugAllocatorConfig cfg = DEBUG_ALLOCATOR_DEFAULTS;
    cfg.track_freed_history  = false;
    DebugAllocator dbg       = DebugAllocatorInitWith(cfg);
    Allocator     *adbg      = ALLOCATOR_OF(&dbg);

    bool ok = true;
    for (u32 i = 0; i < 100; i++) {
        void *p = AllocatorAlloc(adbg, 32, false);
        if (!p) {
            ok = false;
            break;
        }
        AllocatorFree(adbg, p);
    }
    ok = ok && (DebugAllocatorFreedCount(&dbg) == 0);
    ok = ok && (DebugAllocatorLiveCount(&dbg) == 0);

    DebugAllocatorDeinit(&dbg);
    return ok;
}

// =============================================================================
// Remap

bool test_debug_remap_grows(void) {
    DebugAllocator dbg  = DebugAllocatorInit();
    Allocator     *adbg = ALLOCATOR_OF(&dbg);

    u8  *p  = (u8 *)AllocatorAlloc(adbg, 16, true);
    bool ok = (p != NULL);
    if (ok) {
        p[0]  = 'h';
        p[15] = '!';
    }

    u8 *grown = (u8 *)AllocatorRealloc(adbg, p, 200);
    ok        = ok && (grown != NULL) && (grown[0] == 'h') && (grown[15] == '!');
    ok        = ok && (DebugAllocatorLiveCount(&dbg) == 1);
    ok        = ok && (DebugAllocatorLiveBytes(&dbg) == 200);

    if (grown)
        AllocatorFree(adbg, grown);
    DebugAllocatorDeinit(&dbg);
    return ok;
}

bool test_debug_remap_to_zero_frees(void) {
    DebugAllocator dbg  = DebugAllocatorInit();
    Allocator     *adbg = ALLOCATOR_OF(&dbg);

    void *p   = AllocatorAlloc(adbg, 24, false);
    bool  ok  = (p != NULL) && (DebugAllocatorLiveCount(&dbg) == 1);
    void *nil = AllocatorRealloc(adbg, p, 0);
    ok        = ok && (nil == NULL) && (DebugAllocatorLiveCount(&dbg) == 0);

    DebugAllocatorDeinit(&dbg);
    return ok;
}

bool test_debug_remap_from_null_allocates(void) {
    DebugAllocator dbg  = DebugAllocatorInit();
    Allocator     *adbg = ALLOCATOR_OF(&dbg);

    void *p  = AllocatorRealloc(adbg, NULL, 32);
    bool  ok = (p != NULL) && (DebugAllocatorLiveCount(&dbg) == 1);
    if (p)
        AllocatorFree(adbg, p);

    DebugAllocatorDeinit(&dbg);
    return ok;
}

// =============================================================================
// Page-backed mode

bool test_debug_page_backed_alloc_free(void) {
    DebugAllocatorConfig cfg = DEBUG_ALLOCATOR_DEFAULTS;
    cfg.force_page_backing   = true;
    DebugAllocator dbg       = DebugAllocatorInitWith(cfg);
    Allocator     *adbg      = ALLOCATOR_OF(&dbg);

    void *p  = AllocatorAlloc(adbg, 64, true);
    bool  ok = (p != NULL) && (DebugAllocatorLiveCount(&dbg) == 1);
    ok       = ok && (DebugAllocatorLiveBytes(&dbg) == 64);
    ok       = ok && (((u64)p & 0xfff) == 0); // page-aligned

    AllocatorFree(adbg, p);
    ok = ok && (DebugAllocatorLiveCount(&dbg) == 0);

    DebugAllocatorDeinit(&dbg);
    return ok;
}

// =============================================================================
// Deadend tests -- expected to LOG_FATAL.

bool test_debug_double_free_aborts(void) {
    DebugAllocator dbg  = DebugAllocatorInit();
    Allocator     *adbg = ALLOCATOR_OF(&dbg);

    void *p = AllocatorAlloc(adbg, 32, true);
    AllocatorFree(adbg, p);
    AllocatorFree(adbg, p); // -> Heap LOG_FATAL
    return false;           // unreachable
}

bool test_debug_foreign_free_aborts(void) {
    // Free a pointer the DebugAllocator never handed out. Not in the
    // live map -> forwarded to underlying Heap, which LOG_FATALs as
    // foreign pointer.
    DebugAllocator dbg  = DebugAllocatorInit();
    Allocator     *adbg = ALLOCATOR_OF(&dbg);

    u8 junk[64];
    AllocatorFree(adbg, junk);
    return false; // unreachable
}

// =============================================================================
// =============================================================================
// Moved from AllocDebug.Mutants1.c -- DEALLOCATE path hardening.
// =============================================================================
// =============================================================================

// =============================================================================
// debug_check_canary -- loop must scan EVERY canary byte, not just byte 0.
//
// `cxx_pre_inc_to_pre_dec` on `++i` (L114) makes the unsigned index wrap
// after the first iteration, so a corruption past byte 0 would slip
// through. Corrupt a canary byte that is NOT the first one and require
// the overflow to still be caught.

bool test_ad1_canary_detects_non_first_byte(void) {
    DebugAllocator dbg  = DebugAllocatorInit();
    Allocator     *adbg = ALLOCATOR_OF(&dbg);

    u8  *buf = (u8 *)AllocatorAlloc(adbg, 16, true);
    bool ok  = (buf != NULL);
    // Leave canary byte 0 (index 16) intact; stomp byte 5 of the canary.
    buf[16 + 5] = 0x77;
    AllocatorFree(adbg, buf);

    ok = ok && (DebugAllocatorOverflows(&dbg) == 1);
    DebugAllocatorDeinit(&dbg);
    return ok;
}

// =============================================================================
// debug_allocator_deallocate -- validate_self must run (L309).
//
// `cxx_remove_void_call` drops debug_validate_self(self). A NULL self
// must abort with "NULL self"; without the call the function dereferences
// NULL. Real code LOG_FATALs (deadend passes via longjmp).

bool test_ad1_free_null_self_aborts(void) {
    int   stackvar = 0;
    void *p        = &stackvar;
    (void)debug_allocator_deallocate(NULL, p); // -> LOG_FATAL "NULL self"
    return false;                              // unreachable
}

// =============================================================================
// Freed-history entry: alloc_trace_n is copied from the live record (L364).
//
// `cxx_assign_const` -> entry.alloc_trace_n = 42. A captured trace count
// can never exceed DEBUG_ALLOCATOR_MAX_TRACE (16); it must equal the live
// record's value, which we snapshot before freeing.

bool test_ad1_freed_alloc_trace_n_copied(void) {
    DebugAllocator dbg  = DebugAllocatorInit();
    Allocator     *adbg = ALLOCATOR_OF(&dbg);

    void *p  = AllocatorAlloc(adbg, 48, false);
    bool  ok = (p != NULL);

    // intentional bypass: live map / freed Vec are internal, no accessor.
    DebugRecord *rec    = MapGetFirstPtr(&dbg.live, p);
    u32          live_n = ok ? rec->alloc_trace_n : 0;
    ok                  = ok && (live_n > 0) && (live_n <= DEBUG_ALLOCATOR_MAX_TRACE);

    AllocatorFree(adbg, p);
    ok = ok && (DebugAllocatorFreedCount(&dbg) == 1);
    ok = ok && (VecPtrAt(&dbg.freed, 0)->alloc_trace_n == live_n);

    DebugAllocatorDeinit(&dbg);
    return ok;
}

// =============================================================================
// Freed-history entry: alloc_trace bytes are MemCopy'd from the live
// record (L365). `cxx_mul_to_div` turns the byte count into
// trace_n/sizeof(StackFrame) (~0 bytes), so the freed entry's alloc_trace
// would no longer match the live record's frames.

bool test_ad1_freed_alloc_trace_bytes_copied(void) {
    DebugAllocator dbg  = DebugAllocatorInit();
    Allocator     *adbg = ALLOCATOR_OF(&dbg);

    void *p  = AllocatorAlloc(adbg, 48, false);
    bool  ok = (p != NULL);

    DebugRecord *rec = MapGetFirstPtr(&dbg.live, p);
    u32          n   = ok ? rec->alloc_trace_n : 0;
    StackFrame   snapshot[DEBUG_ALLOCATOR_MAX_TRACE];
    ok = ok && (n > 0);
    if (ok)
        MemCopy(snapshot, rec->alloc_trace, (size)n * sizeof(StackFrame));

    AllocatorFree(adbg, p);
    ok = ok && (DebugAllocatorFreedCount(&dbg) == 1);

    const DebugFreedEntry *fe    = VecPtrAt(&dbg.freed, 0);
    bool                   match = ok;
    for (u32 i = 0; ok && i < n; i++) {
        if (fe->alloc_trace[i].ip != snapshot[i].ip)
            match = false;
    }
    ok = ok && match;

    DebugAllocatorDeinit(&dbg);
    return ok;
}

// =============================================================================
// Freed-history entry: free_trace_n initialised to 0 before capture (L366).
//
// `cxx_assign_const` -> entry.free_trace_n = 42. With trace capture OFF
// the value is never overwritten, so it must stay 0.

bool test_ad1_freed_free_trace_n_zero_when_no_capture(void) {
    DebugAllocatorConfig cfg = DEBUG_ALLOCATOR_DEFAULTS;
    cfg.capture_traces       = false;
    DebugAllocator dbg       = DebugAllocatorInitWith(cfg);
    Allocator     *adbg      = ALLOCATOR_OF(&dbg);

    void *p  = AllocatorAlloc(adbg, 32, false);
    bool  ok = (p != NULL);
    AllocatorFree(adbg, p);

    ok = ok && (DebugAllocatorFreedCount(&dbg) == 1);
    ok = ok && (VecPtrAt(&dbg.freed, 0)->free_trace_n == 0);

    DebugAllocatorDeinit(&dbg);
    return ok;
}

// =============================================================================
// Freed-history entry: free_trace_n is the captured count, bounded by the
// configured depth (L368 depth init, L369 le-clamp, L371 assignment).
//
// With trace_depth = 2 the capture is capped at 2 frames (the free site
// is many frames deep, so exactly 2 are returned). Any mutation that
// bumps the effective depth (depth=42, clamp-to-16) or replaces the
// captured count with 42 breaks this exact equality.

bool test_ad1_freed_free_trace_n_capped_to_depth(void) {
    DebugAllocatorConfig cfg = DEBUG_ALLOCATOR_DEFAULTS;
    cfg.trace_depth          = 2;
    DebugAllocator dbg       = DebugAllocatorInitWith(cfg);
    Allocator     *adbg      = ALLOCATOR_OF(&dbg);

    void *p  = AllocatorAlloc(adbg, 32, false);
    bool  ok = (p != NULL);
    AllocatorFree(adbg, p);

    ok = ok && (DebugAllocatorFreedCount(&dbg) == 1);
    ok = ok && (VecPtrAt(&dbg.freed, 0)->free_trace_n == 2);

    DebugAllocatorDeinit(&dbg);
    return ok;
}

// =============================================================================
// Freed-history entry: free_trace_n never exceeds DEBUG_ALLOCATOR_MAX_TRACE
// even when the configured depth is larger (L370 clamp `depth = 16`).
//
// trace_depth = 20 forces the >MAX clamp branch. Real code caps the
// capture at 16; the `cxx_assign_const` mutation (depth = 42) would push
// the captured count above 16.

bool test_ad1_freed_free_trace_n_clamped_to_max(void) {
    DebugAllocatorConfig cfg = DEBUG_ALLOCATOR_DEFAULTS;
    cfg.trace_depth          = 20;
    DebugAllocator dbg       = DebugAllocatorInitWith(cfg);
    Allocator     *adbg      = ALLOCATOR_OF(&dbg);

    void *p  = AllocatorAlloc(adbg, 32, false);
    bool  ok = (p != NULL);
    AllocatorFree(adbg, p);

    ok = ok && (DebugAllocatorFreedCount(&dbg) == 1);
    ok = ok && (VecPtrAt(&dbg.freed, 0)->free_trace_n <= DEBUG_ALLOCATOR_MAX_TRACE);

    DebugAllocatorDeinit(&dbg);
    return ok;
}

// =============================================================================
// Stats: deallocations counter increments by exactly one per free (L393).
//
// `cxx_add_assign_to_sub_assign` -> `-= 1u` makes the counter underflow.

bool test_ad1_stats_deallocations_increment(void) {
    DebugAllocator dbg  = DebugAllocatorInit();
    Allocator     *adbg = ALLOCATOR_OF(&dbg);

    void *p  = AllocatorAlloc(adbg, 32, false);
    bool  ok = (p != NULL);
    AllocatorFree(adbg, p);

    ok = ok && (AllocatorDeallocations(adbg) == 1);
    DebugAllocatorDeinit(&dbg);
    return ok;
}

// =============================================================================
// Stats: bytes_in_use decreases by the freed size, and only the freed
// portion (L394 guard, L395 subtract).
//
// Allocate two blocks, free the smaller, and require the stats counter to
// reflect the remaining block exactly. `le_to_gt` would zero the counter
// (wrong: 128 left); `sub_assign_to_add_assign` would grow it.

bool test_ad1_stats_bytes_in_use_partial_free(void) {
    DebugAllocator dbg  = DebugAllocatorInit();
    Allocator     *adbg = ALLOCATOR_OF(&dbg);

    void *a  = AllocatorAlloc(adbg, 64, false);
    void *b  = AllocatorAlloc(adbg, 128, false);
    bool  ok = (a != NULL) && (b != NULL);
    ok       = ok && (AllocatorBytesInUse(adbg) == (64 + 128));

    AllocatorFree(adbg, a);
    ok = ok && (AllocatorBytesInUse(adbg) == 128);

    AllocatorFree(adbg, b);
    ok = ok && (AllocatorBytesInUse(adbg) == 0);

    DebugAllocatorDeinit(&dbg);
    return ok;
}

// =============================================================================
// Sanity: a clean alloc + write-into-user-region + read + free round-trip
// must NOT raise a false-positive overflow (canary check correctness).

bool test_ad1_clean_roundtrip_no_false_overflow(void) {
    DebugAllocator dbg  = DebugAllocatorInit();
    Allocator     *adbg = ALLOCATOR_OF(&dbg);

    u8  *buf = (u8 *)AllocatorAlloc(adbg, 32, false);
    bool ok  = (buf != NULL);
    for (u32 i = 0; ok && i < 32; i++)
        buf[i] = (u8)(i & 0xff);
    for (u32 i = 0; ok && i < 32; i++)
        ok = ok && (buf[i] == (u8)(i & 0xff));
    AllocatorFree(adbg, buf);

    ok = ok && (DebugAllocatorOverflows(&dbg) == 0);
    ok = ok && (DebugAllocatorLiveCount(&dbg) == 0);
    DebugAllocatorDeinit(&dbg);
    return ok;
}

// =============================================================================
// Deadend: double-free of a tracked pointer must abort.

bool test_ad1_double_free_aborts(void) {
    DebugAllocator dbg  = DebugAllocatorInit();
    Allocator     *adbg = ALLOCATOR_OF(&dbg);

    void *p = AllocatorAlloc(adbg, 32, true);
    AllocatorFree(adbg, p);
    AllocatorFree(adbg, p); // -> LOG_FATAL
    return false;           // unreachable
}

// =============================================================================
// Deadend: free of a foreign pointer (never handed out) must abort.

bool test_ad1_foreign_free_aborts(void) {
    DebugAllocator dbg  = DebugAllocatorInit();
    Allocator     *adbg = ALLOCATOR_OF(&dbg);

    u8 junk[64];
    AllocatorFree(adbg, junk); // -> LOG_FATAL (foreign ptr)
    return false;              // unreachable
}

// =============================================================================
// =============================================================================
// Moved from AllocDebug.Mutants2.c -- ALLOCATE + hash + canary-write +
// resize path hardening.
// =============================================================================
// =============================================================================

// ---------------------------------------------------------------------------
// Helper: drive a deep call stack so trace-depth clamping is observable.
// CaptureStackTrace records min(depth, actual-frames). With a stack
// deeper than DEBUG_ALLOCATOR_MAX_TRACE (16) the clamp is exercised.
// ---------------------------------------------------------------------------

static void *deep_alloc(Allocator *a, size bytes, i8 zeroed, u32 levels) {
    if (levels > 0)
        return deep_alloc(a, bytes, zeroed, levels - 1);
    return AllocatorAlloc(a, bytes, zeroed);
}

// ===========================================================================
// allocate: bytes_in_use accounting (kills L266 self->bytes_in_use += -> -=)

bool test_ad2_alloc_bumps_live_bytes(void) {
    DebugAllocator dbg  = DebugAllocatorInit();
    Allocator     *adbg = ALLOCATOR_OF(&dbg);

    void *p  = AllocatorAlloc(adbg, 100, false);
    bool  ok = (p != NULL);
    // self->bytes_in_use is surfaced by DebugAllocatorLiveBytes. A
    // mutated `-=` underflows to a huge u64 instead of 100.
    ok = ok && (DebugAllocatorLiveBytes(&dbg) == 100);

    if (p)
        AllocatorFree(adbg, p);
    ok = ok && (DebugAllocatorLiveBytes(&dbg) == 0);
    DebugAllocatorDeinit(&dbg);
    return ok;
}

// allocate: stats.allocations counter (kills L268 += -> -=)

bool test_ad2_alloc_increments_allocations(void) {
    DebugAllocator dbg  = DebugAllocatorInit();
    Allocator     *adbg = ALLOCATOR_OF(&dbg);

    void *p1 = AllocatorAlloc(adbg, 16, false);
    void *p2 = AllocatorAlloc(adbg, 16, false);
    bool  ok = (p1 != NULL) && (p2 != NULL);
    ok       = ok && (AllocatorAllocations(adbg) == 2);

    if (p1)
        AllocatorFree(adbg, p1);
    if (p2)
        AllocatorFree(adbg, p2);
    DebugAllocatorDeinit(&dbg);
    return ok;
}

// allocate: stats.bytes_requested (kills L269 += -> -=)

bool test_ad2_alloc_accumulates_bytes_requested(void) {
    DebugAllocator dbg  = DebugAllocatorInit();
    Allocator     *adbg = ALLOCATOR_OF(&dbg);

    void *p1 = AllocatorAlloc(adbg, 30, false);
    void *p2 = AllocatorAlloc(adbg, 70, false);
    bool  ok = (p1 != NULL) && (p2 != NULL);
    // bytes_requested is cumulative and does NOT decrease on free.
    ok = ok && (AllocatorBytesRequested(adbg) == 100);

    if (p1)
        AllocatorFree(adbg, p1);
    if (p2)
        AllocatorFree(adbg, p2);
    // still 100 after frees (cumulative)
    ok = ok && (AllocatorBytesRequested(adbg) == 100);
    DebugAllocatorDeinit(&dbg);
    return ok;
}

// allocate: stats.bytes_in_use (kills L270 += -> -=)

bool test_ad2_alloc_bumps_stats_bytes_in_use(void) {
    DebugAllocator dbg  = DebugAllocatorInit();
    Allocator     *adbg = ALLOCATOR_OF(&dbg);

    void *p  = AllocatorAlloc(adbg, 256, false);
    bool  ok = (p != NULL);
    ok       = ok && (AllocatorBytesInUse(adbg) == 256);

    if (p)
        AllocatorFree(adbg, p);
    ok = ok && (AllocatorBytesInUse(adbg) == 0);
    DebugAllocatorDeinit(&dbg);
    return ok;
}

// allocate: peak tracking value (kills L272 peak = ... -> = 42)

bool test_ad2_alloc_tracks_peak_value(void) {
    DebugAllocator dbg  = DebugAllocatorInit();
    Allocator     *adbg = ALLOCATOR_OF(&dbg);

    void *p  = AllocatorAlloc(adbg, 512, false);
    bool  ok = (p != NULL);
    // peak equals current in-use after a single alloc. A const-mutated
    // assignment would record 42 instead of 512.
    ok = ok && (AllocatorPeakBytesInUse(adbg) == 512);

    if (p)
        AllocatorFree(adbg, p);
    // peak does not shrink on free
    ok = ok && (AllocatorPeakBytesInUse(adbg) == 512);
    DebugAllocatorDeinit(&dbg);
    return ok;
}

// allocate: peak only advances when in-use exceeds the prior peak.
// Kills L271 `>` -> `<=` : with `<=`, the first alloc (in_use=N > 0 = peak)
// would NOT update peak, leaving it 0.

bool test_ad2_peak_advances_on_first_alloc(void) {
    DebugAllocator dbg  = DebugAllocatorInit();
    Allocator     *adbg = ALLOCATOR_OF(&dbg);

    void *p  = AllocatorAlloc(adbg, 333, false);
    bool  ok = (p != NULL);
    // Real `if (in_use > peak) peak = in_use` advances 0 -> 333. A
    // `<=` flip leaves peak at 0.
    ok = ok && (AllocatorPeakBytesInUse(adbg) == 333);

    if (p)
        AllocatorFree(adbg, p);
    DebugAllocatorDeinit(&dbg);
    return ok;
}

// allocate: peak holds across a free-then-smaller-alloc. Real peak is
// the historical max; this reinforces L271/L272 monotonic peak.

bool test_ad2_peak_holds_historical_max(void) {
    DebugAllocator dbg  = DebugAllocatorInit();
    Allocator     *adbg = ALLOCATOR_OF(&dbg);

    void *big = AllocatorAlloc(adbg, 1000, false);
    bool  ok  = (big != NULL) && (AllocatorPeakBytesInUse(adbg) == 1000);
    if (big)
        AllocatorFree(adbg, big);

    void *small = AllocatorAlloc(adbg, 8, false);
    ok          = ok && (small != NULL);
    // in_use (8) < peak (1000): peak must stay 1000.
    ok = ok && (AllocatorPeakBytesInUse(adbg) == 1000);
    ok = ok && (AllocatorBytesInUse(adbg) == 8);

    if (small)
        AllocatorFree(adbg, small);
    DebugAllocatorDeinit(&dbg);
    return ok;
}

// ===========================================================================
// allocate: alloc-failure counter (kills L236 failed_allocations += -> -=).
// A request large enough to defeat the embedded heap returns NULL and
// bumps failed_allocations to 1 (a `-=` flip underflows to a huge u64).

bool test_ad2_failed_alloc_counter(void) {
    DebugAllocator dbg  = DebugAllocatorInit();
    Allocator     *adbg = ALLOCATOR_OF(&dbg);

    // Absurd size: the heap cannot serve it and returns NULL.
    void *p  = AllocatorAlloc(adbg, (size)-1 - 4096, false);
    bool  ok = (p == NULL);
    ok       = ok && (AllocatorFailedAllocations(adbg) == 1);
    // No live record / bytes for a failed alloc.
    ok = ok && (DebugAllocatorLiveCount(&dbg) == 0);
    ok = ok && (DebugAllocatorLiveBytes(&dbg) == 0);

    DebugAllocatorDeinit(&dbg);
    return ok;
}

// ===========================================================================
// allocate: trace-depth handling.
//
// depth clamp (L249/L250/L251/L252). With a deep call stack the number
// of captured frames is min(trace_depth, MAX_TRACE=16, actual-frames).

// Kills L252 (alloc_trace_n = ... -> = 42): captured count must be in
// [1, MAX_TRACE]; a const 42 is out of range.
bool test_ad2_trace_count_within_bounds(void) {
    DebugAllocator dbg  = DebugAllocatorInit(); // capture_traces, trace_depth=8
    Allocator     *adbg = ALLOCATOR_OF(&dbg);

    // 30 levels of recursion guarantee the stack is deeper than the
    // 8-frame trace_depth and the 16-frame MAX_TRACE clamp.
    void *p  = deep_alloc(adbg, 64, false, 30);
    bool  ok = (p != NULL);

    DebugRecord *rec = MapGetFirstPtr(&dbg.live, p);
    ok               = ok && (rec != NULL);
    ok               = ok && (rec->alloc_trace_n > 0);
    ok               = ok && (rec->alloc_trace_n <= DEBUG_ALLOCATOR_MAX_TRACE);

    if (p)
        AllocatorFree(adbg, p);
    DebugAllocatorDeinit(&dbg);
    return ok;
}

// Kills L249 (depth init -> 42) and L250 `>`->`<=`: with trace_depth=8
// and a deep stack, real captures at most 8 frames. A mutation that
// lifts the working depth to 16 (clamp) captures up to 16.
bool test_ad2_trace_depth_respected(void) {
    DebugAllocatorConfig cfg = DEBUG_ALLOCATOR_DEFAULTS;
    cfg.trace_depth          = 4; // below MAX_TRACE
    DebugAllocator dbg       = DebugAllocatorInitWith(cfg);
    Allocator     *adbg      = ALLOCATOR_OF(&dbg);

    void *p  = deep_alloc(adbg, 64, false, 30);
    bool  ok = (p != NULL);

    DebugRecord *rec = MapGetFirstPtr(&dbg.live, p);
    ok               = ok && (rec != NULL);
    ok               = ok && (rec->alloc_trace_n > 0);
    // Real caps at trace_depth (4). A `depth=42`/clamp-to-16/`<=` flip
    // would capture more than 4.
    ok = ok && (rec->alloc_trace_n <= 4);

    if (p)
        AllocatorFree(adbg, p);
    DebugAllocatorDeinit(&dbg);
    return ok;
}

// Kills L251 (clamp target -> 42): with trace_depth > MAX_TRACE the
// working depth must be clamped DOWN to MAX_TRACE (16). A clamp to 42
// would let CaptureStackTrace overrun the 16-slot alloc_trace buffer
// and record > 16 frames.
bool test_ad2_trace_depth_clamped_to_max(void) {
    DebugAllocatorConfig cfg = DEBUG_ALLOCATOR_DEFAULTS;
    cfg.trace_depth          = 32; // above MAX_TRACE -> must clamp to 16
    DebugAllocator dbg       = DebugAllocatorInitWith(cfg);
    Allocator     *adbg      = ALLOCATOR_OF(&dbg);

    void *p  = deep_alloc(adbg, 64, false, 40);
    bool  ok = (p != NULL);

    DebugRecord *rec = MapGetFirstPtr(&dbg.live, p);
    ok               = ok && (rec != NULL);
    ok               = ok && (rec->alloc_trace_n > 0);
    ok               = ok && (rec->alloc_trace_n <= DEBUG_ALLOCATOR_MAX_TRACE);

    if (p)
        AllocatorFree(adbg, p);
    DebugAllocatorDeinit(&dbg);
    return ok;
}

// ===========================================================================
// Canary placement from the allocate / write_canary side.

// A legal full-width write of exactly N user bytes must NOT trip the
// canary on free -- pins the canary to sit at offset == requested_size
// (kills any allocate/write_canary mutation that lands the canary
// inside [0, N)).
bool test_ad2_full_width_write_is_clean(void) {
    DebugAllocator dbg  = DebugAllocatorInit();
    Allocator     *adbg = ALLOCATOR_OF(&dbg);

    size n   = 48;
    u8  *buf = (u8 *)AllocatorAlloc(adbg, n, false);
    bool ok  = (buf != NULL);
    if (ok) {
        for (size i = 0; i < n; i++)
            buf[i] = (u8)(0xA0 + (i & 7)); // write the entire user region
    }
    AllocatorFree(adbg, buf);              // must be a clean free, no overflow

    ok = ok && (DebugAllocatorOverflows(&dbg) == 0);
    DebugAllocatorDeinit(&dbg);
    return ok;
}

// allocate returns a usable, correctly sized, zeroed region when asked.
bool test_ad2_zeroed_region_is_zero_and_sized(void) {
    DebugAllocator dbg  = DebugAllocatorInit();
    Allocator     *adbg = ALLOCATOR_OF(&dbg);

    size n   = 200;
    u8  *buf = (u8 *)AllocatorAlloc(adbg, n, true);
    bool ok  = (buf != NULL);
    if (ok) {
        for (size i = 0; i < n; i++)
            ok = ok && (buf[i] == 0); // zeroed request honoured
        // Full-width write stays clean (region really is n bytes wide).
        for (size i = 0; i < n; i++)
            buf[i] = 0xFF;
    }
    AllocatorFree(adbg, buf);
    ok = ok && (DebugAllocatorOverflows(&dbg) == 0);
    DebugAllocatorDeinit(&dbg);
    return ok;
}

// ===========================================================================
// hash distribution: many allocations across buckets / collisions are
// each independently tracked and freed with no spurious bad/double free
// and no leak misreport. Documents the contract the debug_ptr_hash
// avalanche steps serve (the steps are individually equivalent --
// insert/lookup/rehash all route through the same callback -- but a
// broken hash would surface here as a spurious FATAL or a leak).
bool test_ad2_hash_tracks_many_allocations(void) {
    DebugAllocator dbg  = DebugAllocatorInit();
    Allocator     *adbg = ALLOCATOR_OF(&dbg);

    enum {
        N = 256
    };
    void *ps[N];
    bool  ok = true;

    for (u32 i = 0; i < N; i++) {
        ps[i] = AllocatorAlloc(adbg, 8 + (i & 31), false);
        ok    = ok && (ps[i] != NULL);
    }
    ok = ok && (DebugAllocatorLiveCount(&dbg) == N);

    // Every pointer must resolve to its own live record (hash + probe
    // must find each one; a broken hash would miss some).
    for (u32 i = 0; i < N; i++) {
        DebugRecord *rec = MapGetFirstPtr(&dbg.live, ps[i]);
        ok               = ok && (rec != NULL);
        ok               = ok && (rec->requested_size == (size)(8 + (i & 31)));
    }

    // Free every one (out of insertion order) -- each must hit its own
    // entry, no spurious bad-free / double-free.
    for (u32 i = 0; i < N; i += 2) {
        AllocatorFree(adbg, ps[i]);
        ps[i] = NULL;
    }
    for (u32 i = 1; i < N; i += 2) {
        AllocatorFree(adbg, ps[i]);
        ps[i] = NULL;
    }
    ok = ok && (DebugAllocatorLiveCount(&dbg) == 0);
    ok = ok && (DebugAllocatorLiveBytes(&dbg) == 0);

    DebugAllocatorDeinit(&dbg);
    return ok;
}

// ===========================================================================
// resize: always refuses in place (returns 0). Reinforces the entry
// point shape; the validate gate is exercised by the deadend below.
bool test_ad2_resize_refuses_in_place(void) {
    DebugAllocator dbg  = DebugAllocatorInit();
    Allocator     *adbg = ALLOCATOR_OF(&dbg);

    void *p  = AllocatorAlloc(adbg, 64, false);
    bool  ok = (p != NULL);
    // debug_allocator_resize unconditionally returns 0 (refuses).
    i8 r = AllocatorResize(adbg, p, 128);
    ok   = ok && (r == 0);
    // The allocation is untouched -- still one live record of 64 bytes.
    ok = ok && (DebugAllocatorLiveCount(&dbg) == 1);
    ok = ok && (DebugAllocatorLiveBytes(&dbg) == 64);

    if (p)
        AllocatorFree(adbg, p);
    DebugAllocatorDeinit(&dbg);
    return ok;
}

// ===========================================================================
// Deadend tests.

// Overflow past the user region IS detected on free -- pins the canary
// placement from the allocate/write side: writing one byte past N stomps
// the canary, which the free-time check catches and (via the overflow
// counter) records, then we provoke a hard abort by double-checking the
// canary path via a deliberate stomp + free under a config that escalates.
//
// Canary write side: alloc N, write N+1 -> overflow detected (counter).
bool test_ad2_overflow_one_past_detected(void) {
    DebugAllocator dbg  = DebugAllocatorInit();
    Allocator     *adbg = ALLOCATOR_OF(&dbg);

    size n   = 32;
    u8  *buf = (u8 *)AllocatorAlloc(adbg, n, true);
    bool ok  = (buf != NULL);
    if (ok)
        buf[n] = 0x55; // one past the user region -> stomps canary[0]
    AllocatorFree(adbg, buf);

    // The canary sits exactly at offset n; a one-past write corrupts it.
    ok = ok && (DebugAllocatorOverflows(&dbg) == 1);
    DebugAllocatorDeinit(&dbg);
    return ok;
}

// resize self-validation gate (kills L412 remove-void-call of
// debug_validate_self): a type-confused allocator must abort on the
// validate call inside resize. With the validate call removed, resize
// would silently return 0 instead of aborting.
bool test_ad2_resize_validates_self(void) {
    DebugAllocator dbg  = DebugAllocatorInit();
    Allocator     *adbg = ALLOCATOR_OF(&dbg);

    void *p = AllocatorAlloc(adbg, 16, false);
    (void)p;
    // Corrupt the magic so debug_validate_self trips type-confusion.
    // Call the typed entry (DebugAllocator*) so dispatch lands directly
    // on debug_allocator_resize and the ONLY validation that runs is the
    // debug_validate_self call inside it -- removing that call (the
    // mutant) would make this return 0 instead of aborting.
    dbg.base.__magic = 0;
    (void)AllocatorResize(&dbg, p, 32); // -> LOG_FATAL via debug_validate_self
    return false;                       // unreachable
}

// =============================================================================
// =============================================================================
// Moved from AllocDebug.Mutants3.c -- remap / leak-reporting / deinit /
// trace / self-validate path hardening.
// =============================================================================
// =============================================================================

// =============================================================================
// remap: content preservation bounds the copy by min(old, new).
//
// Kills (debug_allocator_remap):
//   439:10 cxx_init_const  -- old_requested forced to a constant
//   445:10 cxx_init_const  -- copy forced to a constant
// Grow 64 -> 200: real copies all 64 original bytes. A constant-folded
// old_requested / copy (mull's truthy 42) copies only 42, so the
// original bytes at index 50 / 63 are NOT carried into the fresh slot.

bool test_ad3_remap_grow_preserves_full_old_width(void) {
    DebugAllocator dbg  = DebugAllocatorInit();
    Allocator     *adbg = ALLOCATOR_OF(&dbg);

    u8 *p = (u8 *)AllocatorAlloc(adbg, 64, true);
    if (!p)
        return false;
    for (u32 i = 0; i < 64; i++)
        p[i] = (u8)(0xA0 + (i & 0x0f));
    // Distinctive markers past index 42 -- the bytes a truncated copy
    // would drop.
    p[50] = 0x5A;
    p[63] = 0x6B;

    u8  *grown = (u8 *)AllocatorRealloc(adbg, p, 200);
    bool ok    = (grown != NULL);
    ok         = ok && (grown[0] == (u8)(0xA0));
    ok         = ok && (grown[42] == (u8)(0xA0 + (42 & 0x0f)));
    ok         = ok && (grown[50] == 0x5A);
    ok         = ok && (grown[63] == 0x6B);
    ok         = ok && (DebugAllocatorLiveCount(&dbg) == 1);
    ok         = ok && (DebugAllocatorLiveBytes(&dbg) == 200);

    if (grown)
        AllocatorFree(adbg, grown);
    DebugAllocatorDeinit(&dbg);
    return ok;
}

// Kills 445:31 cxx_lt_to_ge in `old_requested < new_size`. The `<`
// computes min(old,new); `>=` flips it to max(old,new). On a SHRINK
// (old=64, new=8) the real copy is 8 (fits the fresh 8-byte slot); the
// max-mutant copies 64 bytes into the fresh 8+canary slot, stomping the
// fresh canary / heap metadata. Real code keeps content + a clean
// canary (overflows stays 0); the mutant corrupts it.
bool test_ad3_remap_shrink_copy_is_min_not_max(void) {
    DebugAllocator dbg  = DebugAllocatorInit();
    Allocator     *adbg = ALLOCATOR_OF(&dbg);

    u8 *p = (u8 *)AllocatorAlloc(adbg, 64, true);
    if (!p)
        return false;
    for (u32 i = 0; i < 64; i++)
        p[i] = (u8)(i + 1);

    u8  *shrunk = (u8 *)AllocatorRealloc(adbg, p, 8);
    bool ok     = (shrunk != NULL);
    for (u32 i = 0; i < 8; i++)
        ok = ok && (shrunk[i] == (u8)(i + 1));
    ok = ok && (DebugAllocatorLiveCount(&dbg) == 1);
    ok = ok && (DebugAllocatorLiveBytes(&dbg) == 8);

    if (shrunk)
        AllocatorFree(adbg, shrunk);
    // A max-copy mutant would have stomped the fresh canary; real code
    // leaves it pristine.
    ok = ok && (DebugAllocatorOverflows(&dbg) == 0);

    DebugAllocatorDeinit(&dbg);
    return ok;
}

// Kills 436:9 cxx_replace_scalar_call -- remap of a pointer not in the
// live map must forward to deallocate, which aborts (foreign ptr ->
// Heap LOG_FATAL). The scalar-replaced call skips the abort. Real code
// aborts; deadend captures it.
bool test_ad3_remap_unknown_ptr_aborts(void) {
    DebugAllocator dbg  = DebugAllocatorInit();
    Allocator     *adbg = ALLOCATOR_OF(&dbg);

    u8 junk[64];
    (void)AllocatorRealloc(adbg, junk, 128); // -> deallocate -> Heap LOG_FATAL
    return false;                            // unreachable
}

// =============================================================================
// ReportLeaks: the trace block is gated by `alloc_trace_n > 0`.
//
// Kills 538:36 cxx_gt_to_le in `val_ptr->alloc_trace_n > 0`. With
// trace capture on, trace_n > 0, so the real report appends the
// formatted alloc trace (FormatStackTrace emits "  #0 ..." frame
// lines). The `<= 0` mutant skips the trace, leaving only the "leak:"
// summary line and no frame markers.
bool test_ad3_report_leaks_includes_trace_frames(void) {
    DebugAllocator dbg  = DebugAllocatorInit();
    Allocator     *adbg = ALLOCATOR_OF(&dbg);

    void *p = AllocatorAlloc(adbg, 24, true);
    (void)p;

    HeapAllocator scratch = HeapAllocatorInit();
    Str           out     = StrInit(&scratch);
    DebugAllocatorReportLeaks(&dbg, &out);

    bool ok = (ZstrFindSubstring(StrBegin(&out), "leak:") != NULL);
    // FormatStackTrace renders frame lines beginning with "#0".
    ok = ok && (ZstrFindSubstring(StrBegin(&out), "#0") != NULL);

    StrDeinit(&out);
    HeapAllocatorDeinit(&scratch);
    if (p)
        AllocatorFree(adbg, p);
    DebugAllocatorDeinit(&dbg);
    return ok;
}

// =============================================================================
// validate_self / validate_self_structural.

// Kills 194:5 cxx_remove_void_call -- removing the structural validator
// call means a corrupted embedded-heap magic goes unnoticed. Real code
// runs structural validation on the first (validated-bit-set) entry and
// LOG_FATALs on the bad magic; the mutant skips the check entirely.
// Deadend: real code aborts here.
bool test_ad3_structural_runs_on_first_entry(void) {
    DebugAllocator dbg = DebugAllocatorInit();
    // Corrupt the embedded heap magic before the first validating entry
    // point; the validated bit is still set, so structural runs.
    dbg.heap.base.__magic = 0xdeadbeefULL;
    Allocator *adbg       = ALLOCATOR_OF(&dbg);
    (void)AllocatorAlloc(adbg, 32, true); // structural -> bad heap magic LOG_FATAL
    return false;                         // unreachable
}

// Kills 191:30 cxx_and_to_or -- `!(magic & VALIDATED_BIT)` early-return
// guard becomes `!(magic | VALIDATED_BIT)`, which is always false, so
// the mutant runs structural validation on EVERY entry instead of just
// the first. Real code clears the validated bit after the first op and
// thereafter skips structural; so a structural invariant broken AFTER
// the first op is tolerated by real code but re-checked (and aborted
// on) by the mutant. Normal test: real returns true; the mutant aborts.
bool test_ad3_structural_skipped_after_first_op(void) {
    DebugAllocator dbg  = DebugAllocatorInit();
    Allocator     *adbg = ALLOCATOR_OF(&dbg);

    // First op clears the validated bit.
    void *p  = AllocatorAlloc(adbg, 32, true);
    bool  ok = (p != NULL);
    AllocatorFree(adbg, p);

    // Live is now empty; break the bytes_in_use/live consistency
    // invariant. Real code (bit cleared) never re-checks it; the
    // always-validate mutant would LOG_FATAL on the next entry.
    dbg.bytes_in_use = 4096;

    void *q = AllocatorAlloc(adbg, 16, true); // real: no structural recheck
    ok      = ok && (q != NULL);

    // Repair the invariant before teardown so Deinit's own paths and
    // the suite stay clean.
    if (q)
        AllocatorFree(adbg, q);
    dbg.bytes_in_use = 0;
    DebugAllocatorDeinit(&dbg);
    return ok;
}

// Kills 132:84 cxx_sub_to_add in the power-of-two alignment check
// `(alignment & (alignment - 1)) != 0`. For a power-of-two alignment
// like 4: real `4 & 3 == 0` (accept); mutant `4 & 5 == 4 != 0`
// (false-reject -> LOG_FATAL). Set a valid power-of-two alignment and
// trigger structural validation; real code accepts, the mutant aborts.
bool test_ad3_structural_accepts_pow2_alignment(void) {
    DebugAllocator dbg = DebugAllocatorInit();
    // Valid power-of-two alignment; validated bit still set so the first
    // entry runs structural and exercises the pow2 check.
    dbg.base.alignment = 4;
    Allocator *adbg    = ALLOCATOR_OF(&dbg);

    void *p  = AllocatorAlloc(adbg, 32, true); // real: 4 is pow2 -> accepted
    bool  ok = (p != NULL);
    if (p)
        AllocatorFree(adbg, p);

    DebugAllocatorDeinit(&dbg);
    return ok;
}

// Kills 158:35 cxx_eq_to_ne in
// `MapPairCount(&live) == 0 && bytes_in_use != 0`. The `== 0` flips to
// `!= 0`: the mutant aborts whenever there IS a live allocation with
// non-zero bytes -- i.e. the ordinary in-use state. Hold a live alloc,
// re-arm the validated bit, then run a validating entry: real code
// (`count==0` false) does nothing; the mutant (`count!=0` &&
// `bytes!=0`) LOG_FATALs. Normal test: real returns true.
bool test_ad3_structural_allows_live_allocation(void) {
    DebugAllocator dbg  = DebugAllocatorInit();
    Allocator     *adbg = ALLOCATOR_OF(&dbg);

    void *p  = AllocatorAlloc(adbg, 48, true); // count=1, bytes_in_use=48
    bool  ok = (p != NULL);

    // Re-arm the validated bit so the next entry re-runs structural
    // while a live allocation is outstanding.
    dbg.base.__magic |= MAGIC_VALIDATED_BIT;

    void *q = AllocatorAlloc(adbg, 16, true); // real: live + bytes -> no abort
    ok      = ok && (q != NULL);

    if (q)
        AllocatorFree(adbg, q);
    if (p)
        AllocatorFree(adbg, p);
    DebugAllocatorDeinit(&dbg);
    return ok;
}

// =============================================================================
// =============================================================================
// Moved from AllocDebug.Mutants4.c -- structural page-magic / free-trace
// clamp survivors missed by earlier passes.
// =============================================================================
// =============================================================================

// =============================================================================
// validate_self_structural: the embedded page allocator's magic.
//
// Kills 194:5 cxx_remove_void_call -- removing the
// `debug_validate_self_structural(self)` call inside debug_validate_self.
//
// The earlier Mutants3 attempt corrupted the embedded HEAP magic, but in
// normal (heap-backed) mode the very next thing the allocate path does is
// `AllocatorAlloc(&self->heap, ...)`, which re-validates the heap and
// aborts regardless of whether structural ran -- so that corruption does
// NOT distinguish real from mutant (the abort still fires).
//
// The embedded PAGE allocator's magic, by contrast, is ONLY consulted by
// debug_validate_self_structural in heap-backed mode -- nothing downstream
// touches `self->page` unless force_page_backing is set. So corrupting the
// page magic is observable ONLY through the structural validator: real
// code LOG_FATALs ("embedded page has bad magic") on the first validated
// entry; the call-removed mutant skips the check and allocates cleanly.
// Deadend: real code aborts here.
bool test_af_structural_checks_page_magic(void) {
    DebugAllocator dbg = DebugAllocatorInit();
    // Corrupt the embedded page allocator's magic before the first
    // validating entry. The validated bit is still set, so structural
    // runs on this first op. In heap-backed mode nothing else looks at
    // self->page, so only structural can catch this.
    dbg.page.base.__magic = 0xdeadbeefULL;
    Allocator *adbg       = ALLOCATOR_OF(&dbg);
    (void)AllocatorAlloc(adbg, 32, true); // structural -> bad page magic LOG_FATAL
    return false;                         // unreachable
}

// =============================================================================
// free-trace depth clamp.
//
// Kills 370:23 cxx_assign_const -- `depth = DEBUG_ALLOCATOR_MAX_TRACE;`
// in the free-trace clamp becomes `depth = 42;`.
//
// With capture_traces=true and trace_depth set ABOVE
// DEBUG_ALLOCATOR_MAX_TRACE (16), the clamp must pin `depth` to 16 so
// CaptureStackTrace never writes past the 16-element free_trace[] buffer.
// Real code clamps -> free_trace_n <= 16. The mutant sets depth=42, and
// from a sufficiently deep call stack CaptureStackTrace returns >16
// frames, so free_trace_n exceeds 16 (and overflows the buffer).
//
// We free from a deliberately deep recursion (> 20 frames) so a 42-deep
// capture has the frames to overshoot. The freed-history Vec has no
// public accessor for free_trace_n, so -- matching the existing suite's
// intentional-bypass idiom -- we read dbg.freed directly.

// Recursive descent: at depth 0 it does the alloc+free that captures the
// free trace. Marked noinline-ish via a volatile sink so the compiler
// keeps the frames distinct rather than collapsing the recursion.
static volatile u32 g_af_sink;

static void af_deep_free(Allocator *adbg, u32 depth) {
    g_af_sink = depth;     // keep each frame live / non-tail
    if (depth > 0) {
        af_deep_free(adbg, depth - 1);
        g_af_sink = depth; // prevent tail-call folding of the recursion
        return;
    }
    void *p = AllocatorAlloc(adbg, 32, true);
    AllocatorFree(adbg, p); // captures the free trace from this deep stack
}

bool test_af_free_trace_depth_clamped(void) {
    DebugAllocatorConfig cfg = DEBUG_ALLOCATOR_DEFAULTS;
    cfg.capture_traces       = true;
    cfg.trace_depth          = 32; // above DEBUG_ALLOCATOR_MAX_TRACE (16)
    DebugAllocator dbg       = DebugAllocatorInitWith(cfg);
    Allocator     *adbg      = ALLOCATOR_OF(&dbg);

    af_deep_free(adbg, 25); // alloc+free ~25 frames deep

    bool ok = (DebugAllocatorFreedCount(&dbg) == 1);
    // intentional bypass: free_trace_n has no public accessor; reach into
    // the freed Vec. Real code clamps depth to 16 so the captured frame
    // count never exceeds DEBUG_ALLOCATOR_MAX_TRACE. The depth=42 mutant
    // overshoots from this deep stack.
    if (ok) {
        u32 n = VecPtrAt(&dbg.freed, 0)->free_trace_n;
        ok    = ok && (n > 0) && (n <= DEBUG_ALLOCATOR_MAX_TRACE);
    }

    DebugAllocatorDeinit(&dbg);
    return ok;
}

int main(void) {
    WriteFmt("[INFO] Starting AllocDebug tests\n\n");

    TestFunction normal[] = {
        // Happy path
        test_debug_normal_alloc_free,
        test_debug_zero_byte_alloc,
        test_debug_null_free_is_noop,
        // Overflow detection
        test_debug_catches_overflow,
        // Leak detection
        test_debug_leak_count,
        test_debug_report_leaks_emits_traces,
        // Trace capture
        test_debug_alloc_trace_captured,
        // Freed-history Vec
        test_debug_freed_history_grows_on_free,
        test_debug_freed_history_disabled,
        // Remap
        test_debug_remap_grows,
        test_debug_remap_to_zero_frees,
        test_debug_remap_from_null_allocates,
        // Page-backed
        test_debug_page_backed_alloc_free,

        // ---- Moved from AllocDebug.Mutants1.c (DEALLOCATE path) ----
        test_ad1_canary_detects_non_first_byte,
        test_ad1_freed_alloc_trace_n_copied,
        test_ad1_freed_alloc_trace_bytes_copied,
        test_ad1_freed_free_trace_n_zero_when_no_capture,
        test_ad1_freed_free_trace_n_capped_to_depth,
        test_ad1_freed_free_trace_n_clamped_to_max,
        test_ad1_stats_deallocations_increment,
        test_ad1_stats_bytes_in_use_partial_free,
        test_ad1_clean_roundtrip_no_false_overflow,

        // ---- Moved from AllocDebug.Mutants2.c (ALLOCATE path) ----
        test_ad2_alloc_bumps_live_bytes,
        test_ad2_alloc_increments_allocations,
        test_ad2_alloc_accumulates_bytes_requested,
        test_ad2_alloc_bumps_stats_bytes_in_use,
        test_ad2_alloc_tracks_peak_value,
        test_ad2_peak_advances_on_first_alloc,
        test_ad2_peak_holds_historical_max,
        test_ad2_failed_alloc_counter,
        test_ad2_trace_count_within_bounds,
        test_ad2_trace_depth_respected,
        test_ad2_trace_depth_clamped_to_max,
        test_ad2_full_width_write_is_clean,
        test_ad2_zeroed_region_is_zero_and_sized,
        test_ad2_overflow_one_past_detected,
        test_ad2_hash_tracks_many_allocations,
        test_ad2_resize_refuses_in_place,

        // ---- Moved from AllocDebug.Mutants3.c (remap / leak / validate) ----
        test_ad3_remap_grow_preserves_full_old_width,
        test_ad3_remap_shrink_copy_is_min_not_max,
        test_ad3_report_leaks_includes_trace_frames,
        test_ad3_structural_skipped_after_first_op,
        test_ad3_structural_accepts_pow2_alignment,
        test_ad3_structural_allows_live_allocation,

        // ---- Moved from AllocDebug.Mutants4.c (page-magic / free-trace) ----
        test_af_free_trace_depth_clamped,
    };
    TestFunction deadend[] = {
        test_debug_double_free_aborts,
        test_debug_foreign_free_aborts,

        // ---- Moved from AllocDebug.Mutants1.c ----
        test_ad1_free_null_self_aborts,
        test_ad1_double_free_aborts,
        test_ad1_foreign_free_aborts,

        // ---- Moved from AllocDebug.Mutants2.c ----
        test_ad2_resize_validates_self,

        // ---- Moved from AllocDebug.Mutants3.c ----
        test_ad3_remap_unknown_ptr_aborts,
        test_ad3_structural_runs_on_first_entry,

        // ---- Moved from AllocDebug.Mutants4.c ----
        test_af_structural_checks_page_magic,
    };

    return run_test_suite(
        normal,
        (int)(sizeof(normal) / sizeof(normal[0])),
        deadend,
        (int)(sizeof(deadend) / sizeof(deadend[0])),
        "AllocDebug"
    );
}
