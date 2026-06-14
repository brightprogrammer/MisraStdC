/// file      : Tests/Std/ProcMaps.Blind.c
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Blind mutation-hardening for `Sys/ProcMaps.c`. Targets the surviving
/// mutants in the live-`/proc/self/maps` loader path of
/// `proc_maps_load` that the parse-level suite (which feeds crafted
/// StrIters to the static parsers) cannot reach.
///
/// The static parsers and the public loader are made local here by
/// source-including the unit (same technique as ProcMaps.Parse.c), so
/// the loader can be driven with a chosen allocator (DebugAllocator, to
/// observe overflow + leak counters) directly.
///
/// NOTE on the read loop (lines ~190-208): `/proc/self/maps` is a
/// kernel-synthesised file. A single `read(2)` against it never returns
/// a full requested chunk -- the kernel stops at a map-record boundary,
/// so a 4096-byte request yields ~4067-4085 bytes even when much more
/// data follows. `file_read` issues exactly one `read(2)` per call (no
/// refill loop), so `FileRead(&f, StrEnd, CHUNK)` always returns
/// `n < CHUNK`, and the loader's `if (n < (i64)CHUNK) break;` exits
/// after the first read. The growth/short-read mutants on that loop are
/// therefore equivalent in this environment (documented in the worker
/// analysis); the tests below assert the loader's real, reachable
/// invariants instead.

#include <Misra.h>
#include <Misra/Std/Allocator/Debug.h>
#include <Misra/Std/Allocator/Default.h>
#include <Misra/Sys/ProcMaps.h>
#include <Misra/Std/Utility/StrIter.h>

#include "../Util/TestRunner.h"

// Pull in the unit under test so the static parsers + loader are local.
#include "../../Source/Misra/Sys/ProcMaps.c"

// ---------------------------------------------------------------------------
// proc_maps_load reachable invariants (single-read loader path).
// ---------------------------------------------------------------------------

// The success path captures a non-empty raw buffer and parses at least the
// running binary's own mappings into entries. Exercises the loop body's read
// + resize + the empty-guard at line 210 (which must NOT trip on a real,
// non-empty file).
bool test_pmb_load_captures_nonempty(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    ProcMaps         maps;

    if (!proc_maps_load(&maps, ALLOCATOR_OF(&alloc))) {
        DefaultAllocatorDeinit(&alloc);
        return false;
    }

    bool ok = StrLen(&maps.raw) > 0 && VecLen(&maps.entries) > 0;

    ProcMapsDeinit(&maps);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// Under a DebugAllocator the loader's buffer growth must never write past its
// reservation: the `StrReserve(len + CHUNK + 1)` headroom covers the
// subsequent `FileRead(..., CHUNK)`. Asserting zero recorded overflows
// confirms the reservation arithmetic keeps the write in-bounds on the real
// read.
bool test_pmb_load_no_overflow_under_debug(void) {
    DebugAllocator dbg = DebugAllocatorInit();

    ProcMaps maps;
    if (!proc_maps_load(&maps, ALLOCATOR_OF(&dbg))) {
        DebugAllocatorDeinit(&dbg);
        return false;
    }

    bool ok = DebugAllocatorOverflows(&dbg) == 0 && StrLen(&maps.raw) > 0;

    ProcMapsDeinit(&maps);
    DebugAllocatorDeinit(&dbg);
    return ok;
}

// A function address in this binary must resolve to an executable mapping
// that was parsed from the captured maps text -- proves the loader produced
// usable entries (read -> resize -> NUL-sentinel -> parse) end to end.
bool test_pmb_load_self_addr_resolves(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    ProcMaps         maps;

    if (!proc_maps_load(&maps, ALLOCATOR_OF(&alloc))) {
        DefaultAllocatorDeinit(&alloc);
        return false;
    }

    u64                 self = (u64)&test_pmb_load_self_addr_resolves;
    const ProcMapEntry *e    = ProcMapsFindByAddr(&maps, self);
    bool                ok   = e != NULL && (e->perms & PROC_MAP_PERM_EXEC) != 0;

    ProcMapsDeinit(&maps);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

int main(void) {
    WriteFmt("[INFO] Starting ProcMaps.Blind tests\n\n");

    TestFunction tests[] = {
        test_pmb_load_captures_nonempty,
        test_pmb_load_no_overflow_under_debug,
        test_pmb_load_self_addr_resolves,
    };

    return run_test_suite(tests, sizeof(tests) / sizeof(tests[0]), NULL, 0, "ProcMaps.Blind");
}
