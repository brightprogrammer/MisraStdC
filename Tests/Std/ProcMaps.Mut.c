/// file      : tests/std/procmaps.mut.c
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Mut tests for `Sys/ProcMaps.c`. The line/field PARSE survivors are all
/// either dead-store / boundary-equivalent (bucket C) or live only on the
/// `/proc/self/maps` loader path (bucket B, fs-dependent) — those are
/// ledgered, not killed here. The one parse/value survivor that IS killable
/// without the real loader is the loop bound in `ProcMapsFindByAddr`
/// (`i < VecLen` -> `i <= VecLen`), which over-runs the entries vector by
/// one. We drive it directly through the public API with a hand-built
/// ProcMaps whose capacity holds a stale entry just past `length`.
///
/// The static parsers are exercised exhaustively by ProcMaps.Parse.c; we
/// source-include the unit here only to reuse the parse harness shape and
/// keep the static symbols local.

#include <Misra.h>
#include <Misra/Std/Allocator/Debug.h>
#include <Misra/Parsers/ProcMaps.h>

#include "../Util/TestRunner.h"

// Pull in the unit under test so we can build a ProcMaps by hand.
#include "../../Source/Misra/Parsers/ProcMaps.c"

// ---------------------------------------------------------------------------
// ProcMapsFindByAddr — loop bound `i < VecLen` must NOT be `i <= VecLen`.
//
// We push 3 entries, then shrink `length` back to 2 by hand. The 3rd entry
// stays resident in the vector's capacity at index 2. A search address that
// only matches the stale slot must return NULL: the original scans i=0,1 and
// misses it; the `i <= VecLen` mutant additionally reads index 2 and would
// wrongly match it.
// ---------------------------------------------------------------------------
bool test_pm_find_by_addr_no_overrun(void) {
    DebugAllocator alloc = DebugAllocatorInit();
    ProcMaps       pm;
    MemSet(&pm, 0, sizeof(pm));
    pm.raw     = StrInit(ALLOCATOR_OF(&alloc));
    pm.entries = VecInitT(pm.entries, ALLOCATOR_OF(&alloc));

    ProcMapEntry e0 = {.start = 0x1000, .end = 0x2000, .perms = 0, .file_offset = 0, .path = ""};
    ProcMapEntry e1 = {.start = 0x3000, .end = 0x4000, .perms = 0, .file_offset = 0, .path = ""};
    ProcMapEntry e2 = {.start = 0x5000, .end = 0x6000, .perms = 0, .file_offset = 0, .path = ""};

    bool pushed = VecPushBackR(&pm.entries, e0) && VecPushBackR(&pm.entries, e1) && VecPushBackR(&pm.entries, e2);
    if (!pushed) {
        ProcMapsDeinit(&pm);
        DebugAllocatorDeinit(&alloc);
        return false;
    }

    // Logically drop the 3rd entry; its bytes stay at index 2 in capacity.
    pm.entries.length = 2;

    // 0x5500 is inside the stale e2 range only — must be unreachable.
    const ProcMapEntry *hit = ProcMapsFindByAddr(&pm, 0x5500);
    bool                ok  = (hit == NULL);

    // Sanity: live entries are still found at the right slots.
    const ProcMapEntry *h0 = ProcMapsFindByAddr(&pm, 0x1500);
    const ProcMapEntry *h1 = ProcMapsFindByAddr(&pm, 0x3500);
    ok                     = ok && h0 != NULL && h0->start == 0x1000 && h1 != NULL && h1->start == 0x3000;

    ProcMapsDeinit(&pm);
    DebugAllocatorDeinit(&alloc);
    return ok;
}

int main(void) {
    TestFunction tests[] = {
        test_pm_find_by_addr_no_overrun,
    };
    return run_test_suite(tests, sizeof(tests) / sizeof(tests[0]), NULL, 0, "ProcMaps.Mut");
}
