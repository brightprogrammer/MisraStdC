/// file      : tests/std/allocdebug.blind.c
///
/// Blind-spot mutation-hardening tests for DebugAllocator. Each test
/// here kills a specific surviving mutant from the blind sweep that the
/// behavioural AllocDebug.c suite does not detect. The vast majority of
/// the sweep's survivors are provably equivalent (see the campaign
/// notes in the return summary); only the four below have a reachable,
/// deterministic observable in this (LOG_NO_BACKTRACE) test harness.

#include <Misra.h>
#include <Misra/Std/Allocator/Debug.h>
#include <Misra/Std/Zstr.h>

#include "../Util/TestRunner.h"

// =============================================================================
// 397:39 cxx_assign_const -- the underflow-guard else-branch
// `self->base.stats.bytes_in_use = 0u` becomes `= 42`.
//
// The else arm fires only when the freed `requested` exceeds the stats
// counter (an under-accounted state). We force that state by poking the
// stats counter below the live size (intentional bypass, same idiom the
// behavioural suite uses to poke __magic / bytes_in_use), then free.
// Real code clamps the counter to 0; the mutant lands it at 42.

bool test_blind_stats_bytes_in_use_underflow_clamps_zero(void) {
    DebugAllocator dbg  = DebugAllocatorInit();
    Allocator     *adbg = ALLOCATOR_OF(&dbg);

    void *p  = AllocatorAlloc(adbg, 64, false);
    bool  ok = (p != NULL);

    // Drive stats.bytes_in_use below the recorded requested size so the
    // free takes the `requested > bytes_in_use` underflow-guard else arm.
    dbg.base.stats.bytes_in_use = 10;
    AllocatorFree(adbg, p);

    // Real: else arm sets stats.bytes_in_use = 0. Mutant: = 42.
    ok = ok && (AllocatorBytesInUse(adbg) == 0);

    DebugAllocatorDeinit(&dbg);
    return ok;
}

// =============================================================================
// 419:5 cxx_remove_void_call -- `debug_validate_self(self)` inside
// debug_allocator_remap.
//
// With the validate call removed, a NULL self is no longer caught at the
// remap entry: real code LOG_FATALs ("NULL self") which the deadend
// harness captures via longjmp; the mutant skips straight to
// `MapGetFirstPtr(&self->live, ...)` and dereferences NULL, crashing the
// process. Call the typed entry directly so dispatch lands on
// debug_allocator_remap and the ONLY validation is the one under test.

bool test_blind_remap_null_self_aborts(void) {
    int   stackvar = 0;
    void *p        = &stackvar;
    (void)debug_allocator_remap(NULL, p, 32); // -> LOG_FATAL "NULL self"
    return false;                             // unreachable
}

// =============================================================================
// 542:58 cxx_pre_inc_to_pre_dec -- ReportLeaks raw-IP frame loop
// `for (size i = 0; i < alloc_trace_n; ++i)` becomes `--i`.
//
// ReportLeaks appends one "#<i> <ip>" line per captured frame to the
// caller's Str -- an OBSERVABLE output (unlike the LOG_ERROR-only
// debug_emit_trace). With `--i` the index wraps to SIZE_MAX after the
// first iteration, so only "#0" is ever emitted. Real code emits "#1"
// (and beyond) for any leak whose alloc trace captured >= 2 frames.

bool test_blind_report_leaks_emits_all_frames(void) {
    DebugAllocator dbg  = DebugAllocatorInit(); // capture_traces on, depth 8
    Allocator     *adbg = ALLOCATOR_OF(&dbg);

    void *p  = AllocatorAlloc(adbg, 24, true);
    bool  ok = (p != NULL);

    // intentional bypass: no public accessor for the live record's
    // captured frame count; reach in to require >= 2 frames so "#1" must
    // appear in a faithful render.
    DebugRecord *rec = MapGetFirstPtr(&dbg.live, p);
    ok               = ok && (rec != NULL) && (rec->alloc_trace_n >= 2);

    HeapAllocator scratch = HeapAllocatorInit();
    Str           out     = StrInit(&scratch);
    DebugAllocatorReportLeaks(&dbg, &out);

    ok = ok && (ZstrFindSubstring(StrBegin(&out), "leak:") != NULL);
    // Real emits "#0" and "#1"; the `--i` mutant emits only "#0".
    ok = ok && (ZstrFindSubstring(StrBegin(&out), "#0 ") != NULL);
    ok = ok && (ZstrFindSubstring(StrBegin(&out), "#1 ") != NULL);

    StrDeinit(&out);
    HeapAllocatorDeinit(&scratch);
    if (p)
        AllocatorFree(adbg, p);
    DebugAllocatorDeinit(&dbg);
    return ok;
}

// =============================================================================
// 542:32 cxx_lt_to_le -- ReportLeaks raw-IP frame loop bound
// `i < alloc_trace_n` becomes `i <= alloc_trace_n`.
//
// The `<=` flip runs one extra iteration, emitting a spurious
// "#<alloc_trace_n> <ip>" line that real code never produces. Snapshot
// the captured frame count N, render the leak report, and require the
// out-of-range "#N " marker to be ABSENT (real); the mutant adds it.

bool test_blind_report_leaks_no_extra_frame(void) {
    DebugAllocator dbg  = DebugAllocatorInit();
    Allocator     *adbg = ALLOCATOR_OF(&dbg);

    void *p  = AllocatorAlloc(adbg, 24, true);
    bool  ok = (p != NULL);

    // intentional bypass: snapshot the live record's frame count.
    DebugRecord *rec = MapGetFirstPtr(&dbg.live, p);
    ok               = ok && (rec != NULL) && (rec->alloc_trace_n >= 1);
    u32 n            = ok ? rec->alloc_trace_n : 0;

    HeapAllocator scratch = HeapAllocatorInit();
    Str           out     = StrInit(&scratch);
    DebugAllocatorReportLeaks(&dbg, &out);

    // Build the out-of-range marker "#N " that only the `<=` mutant emits.
    Str ns = StrInit(&scratch);
    StrAppendFmt(&ns, "#{} ", n);

    ok = ok && (ZstrFindSubstring(StrBegin(&out), "leak:") != NULL);
    // The valid top frame "#(N-1) " is present; the spurious "#N " is not.
    ok = ok && (ZstrFindSubstring(StrBegin(&out), StrBegin(&ns)) == NULL);

    StrDeinit(&ns);
    StrDeinit(&out);
    HeapAllocatorDeinit(&scratch);
    if (p)
        AllocatorFree(adbg, p);
    DebugAllocatorDeinit(&dbg);
    return ok;
}

int main(void) {
    WriteFmt("[INFO] Starting AllocDebug.Blind tests\n\n");

    TestFunction normal[] = {
        test_blind_stats_bytes_in_use_underflow_clamps_zero,
        test_blind_report_leaks_emits_all_frames,
        test_blind_report_leaks_no_extra_frame,
    };
    TestFunction deadend[] = {
        test_blind_remap_null_self_aborts,
    };

    return run_test_suite(
        normal,
        (int)(sizeof(normal) / sizeof(normal[0])),
        deadend,
        (int)(sizeof(deadend) / sizeof(deadend[0])),
        "AllocDebug.Blind"
    );
}
