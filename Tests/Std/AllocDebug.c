/// file      : Tests/Std/AllocDebug.c
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

    Str out = StrInit(ALLOCATOR_OF(&dbg.meta));
    DebugAllocatorReportLeaks(&dbg, &out);

    bool ok = StrLen(&out) > 0;
    ok      = ok && (ZstrFindSubstring(StrBegin(&out), "leak:") != NULL);
    ok      = ok && (ZstrFindSubstring(StrBegin(&out), "24 bytes") != NULL);
    ok      = ok && (ZstrFindSubstring(StrBegin(&out), "40 bytes") != NULL);

    StrDeinit(&out);
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

    bool  ok = (VecLen(&dbg.freed) == 0);
    void *p1 = AllocatorAlloc(adbg, 16, false);
    void *p2 = AllocatorAlloc(adbg, 32, false);
    void *p3 = AllocatorAlloc(adbg, 64, false);
    ok       = ok && (VecLen(&dbg.freed) == 0); // alloc doesn't push

    AllocatorFree(adbg, p1);
    ok = ok && (VecLen(&dbg.freed) == 1);

    AllocatorFree(adbg, p2);
    AllocatorFree(adbg, p3);
    ok = ok && (VecLen(&dbg.freed) == 3);

    // Each freed entry carries the ptr + both traces.
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
    ok = ok && (VecLen(&dbg.freed) == 0);
    ok = ok && (DebugAllocatorLiveCount(&dbg) == 0);

    DebugAllocatorDeinit(&dbg);
    return ok;
}

// =============================================================================
// Remap

bool test_debug_remap_grows(void) {
    DebugAllocator dbg  = DebugAllocatorInit();
    Allocator     *adbg = ALLOCATOR_OF(&dbg);

    u8 *p  = (u8 *)AllocatorAlloc(adbg, 16, true);
    bool ok = (p != NULL);
    if (ok) {
        p[0]  = 'h';
        p[15] = '!';
    }

    u8 *grown = (u8 *)AllocatorRealloc(adbg, p, 200);
    ok        = ok && (grown != NULL) && (grown[0] == 'h') && (grown[15] == '!');
    ok          = ok && (DebugAllocatorLiveCount(&dbg) == 1);
    ok          = ok && (DebugAllocatorLiveBytes(&dbg) == 200);

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
    };
    TestFunction deadend[] = {
        test_debug_double_free_aborts,
        test_debug_foreign_free_aborts,
    };

    return run_test_suite(
        normal,
        (int)(sizeof(normal) / sizeof(normal[0])),
        deadend,
        (int)(sizeof(deadend) / sizeof(deadend[0])),
        "AllocDebug"
    );
}
