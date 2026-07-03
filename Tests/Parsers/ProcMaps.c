#include <Misra.h>
#include <Misra/Std/Allocator/Debug.h>
#include <Misra/Std/Allocator/Default.h>
#include <Misra/Std/Container/Str.h>
#include <Misra/Std/File.h>
#include <Misra/Std/Zstr.h>
#include <Misra/Parsers/ProcMaps.h>
#include <Misra/Sys/Dir.h>

#include "../Util/TestRunner.h"

// A real function whose address must live in an executable mapping.
static int pm2_marker_fn(int x) {
    return x + 1;
}

// Parse crafted `/proc/self/maps`-format TEXT through the public LoadFrom
// seam. The bytes are parsed through a transient buffer, so we can feed string
// literals and still assert exact parsed fields -- no live `/proc` involved. Returns
// the loader's own success flag -- false if any line is malformed, since the
// parse fails (naming the cause) on the first line it cannot decode.
static bool pm_load_text(ProcMaps *m, Zstr text, DefaultAllocator *alloc) {
    return ProcMapsLoadFrom(m, text, ZstrLen(text), alloc);
}

// `path` is an owned, NUL-terminated Str -- compare it against a literal.
static bool path_eq(const ProcMapEntry *e, Zstr want) {
    return ZstrCompare(StrBegin(&e->path), want) == 0;
}

bool test_procmaps_load(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    ProcMaps         maps;

    if (!ProcMapsLoad(&maps, ALLOCATOR_OF(&alloc))) {
        DefaultAllocatorDeinit(&alloc);
        return false;
    }

    // We expect many mappings — at least the binary itself plus libc.
    bool ok = VecLen(&maps.entries) > 5;

    // At least one entry should be executable (the code section of
    // either the test binary or libc).
    bool any_exec = false;
    for (u64 i = 0; i < VecLen(&maps.entries); ++i) {
        if (VecPtrAt(&maps.entries, i)->perms & PROC_MAP_PERM_EXEC) {
            any_exec = true;
            break;
        }
    }
    ok = ok && any_exec;

    ProcMapsDeinit(&maps);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

bool test_procmaps_find_self(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    ProcMaps         maps;

    if (!ProcMapsLoad(&maps, ALLOCATOR_OF(&alloc))) {
        DefaultAllocatorDeinit(&alloc);
        return false;
    }

    // The address of this function should land inside an executable
    // mapping of the test binary itself.
    u64                 self_addr = (u64)&test_procmaps_find_self;
    const ProcMapEntry *entry     = ProcMapsFindByAddr(&maps, self_addr);

    bool ok = entry != NULL && (entry->perms & PROC_MAP_PERM_EXEC) != 0;

    ProcMapsDeinit(&maps);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// --------------------------------------------------------------------------
// proc_maps_load: produces a populated entry list with sane regions.
// --------------------------------------------------------------------------

// The load must yield many regions. Kills value/count mutations that would
// collapse the loop or the append into producing an empty or near-empty
// list (e.g. the per-line VecPushBack being skipped).
bool test_pm2_load_populates_many_entries(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    ProcMaps         maps;

    if (!ProcMapsLoad(&maps, ALLOCATOR_OF(&alloc))) {
        DefaultAllocatorDeinit(&alloc);
        return false;
    }

    // A live process maps at minimum its own binary, libc, ld, heap,
    // stack, vdso/vsyscall -- comfortably more than five regions.
    bool ok = VecLen(&maps.entries) > 5;

    ProcMapsDeinit(&maps);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// The address of a function in this binary must fall inside exactly one
// returned region, and that region must be executable. Kills the
// start/end range-comparison flips and proves the executable code
// mapping was parsed with correct bounds + perms.
bool test_pm2_load_function_addr_in_exec_region(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    ProcMaps         maps;

    if (!ProcMapsLoad(&maps, ALLOCATOR_OF(&alloc))) {
        DefaultAllocatorDeinit(&alloc);
        return false;
    }

    u64                 fn_addr = (u64)&pm2_marker_fn;
    const ProcMapEntry *e       = ProcMapsFindByAddr(&maps, fn_addr);

    bool ok = e != NULL;
    ok      = ok && (e->perms & PROC_MAP_PERM_EXEC) != 0;
    // The matched region must actually contain the address.
    ok = ok && fn_addr >= e->start && fn_addr < e->end;

    ProcMapsDeinit(&maps);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// A stack local's address must resolve to a region that is read+write
// (and not executable). A second known address, distinct from the
// function address, that must land in its own correctly-bounded region
// with the expected permission bits -- reinforces the perms decode and
// the start/end range comparisons.
bool test_pm2_load_stack_addr_in_rw_region(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    ProcMaps         maps;

    volatile int local = 0xABCD;

    if (!ProcMapsLoad(&maps, ALLOCATOR_OF(&alloc))) {
        DefaultAllocatorDeinit(&alloc);
        return false;
    }

    u64                 stack_addr = (u64)&local;
    const ProcMapEntry *e          = ProcMapsFindByAddr(&maps, stack_addr);

    bool ok = e != NULL;
    ok      = ok && (e->perms & PROC_MAP_PERM_READ) != 0;
    ok      = ok && (e->perms & PROC_MAP_PERM_WRITE) != 0;
    ok      = ok && stack_addr >= e->start && stack_addr < e->end;

    (void)local;

    ProcMapsDeinit(&maps);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// --------------------------------------------------------------------------
// ProcMapsFindByAddr: range-boundary correctness.
// --------------------------------------------------------------------------

// addr == start is INSIDE [start, end). Kills `addr >= start` -> `addr > start`.
bool test_pm2_find_start_boundary_inside(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    ProcMaps         maps;

    if (!ProcMapsLoad(&maps, ALLOCATOR_OF(&alloc))) {
        DefaultAllocatorDeinit(&alloc);
        return false;
    }

    bool ok = VecLen(&maps.entries) > 0;
    if (ok) {
        // Pick a non-degenerate region and query its exact start.
        const ProcMapEntry *region = NULL;
        for (u64 i = 0; i < VecLen(&maps.entries); ++i) {
            const ProcMapEntry *cand = VecPtrAt(&maps.entries, i);
            if (cand->end > cand->start) {
                region = cand;
                break;
            }
        }
        ok = region != NULL;
        if (ok) {
            u64                 start = region->start;
            const ProcMapEntry *hit   = ProcMapsFindByAddr(&maps, start);
            // The start address must resolve, and to a region whose
            // start is exactly that address.
            ok = hit != NULL && hit->start == start;
        }
    }

    ProcMapsDeinit(&maps);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// addr == end-1 is INSIDE; addr == end is NOT inside this region. Kills
// `addr < end` -> `addr <= end`. We pick the LAST entry (highest range)
// so `end` is above every mapping and must resolve to NULL; the `<=`
// mutant would instead return the last entry.
bool test_pm2_find_end_boundary_excluded(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    ProcMaps         maps;

    if (!ProcMapsLoad(&maps, ALLOCATOR_OF(&alloc))) {
        DefaultAllocatorDeinit(&alloc);
        return false;
    }

    bool ok = VecLen(&maps.entries) > 0;
    if (ok) {
        // The entry with the highest `end` -- `end` itself maps to nothing.
        const ProcMapEntry *top = VecPtrAt(&maps.entries, 0);
        for (u64 i = 1; i < VecLen(&maps.entries); ++i) {
            const ProcMapEntry *cand = VecPtrAt(&maps.entries, i);
            if (cand->end > top->end)
                top = cand;
        }

        ok = top->end > top->start; // non-degenerate

        // end - 1 is the last byte inside the region: must resolve to it.
        if (ok) {
            const ProcMapEntry *last_in = ProcMapsFindByAddr(&maps, top->end - 1);
            ok                          = last_in != NULL && last_in->end == top->end;
        }
        // end is one-past the top region and above all mappings: NULL.
        if (ok) {
            const ProcMapEntry *past = ProcMapsFindByAddr(&maps, top->end);
            ok                       = past == NULL;
        }
    }

    ProcMapsDeinit(&maps);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// An address below the lowest mapping resolves to nothing. Forces the
// linear scan to run to completion (no early return) and asserts a clean
// NULL, reinforcing the range comparisons from the "no match anywhere"
// side. (The `i < VecLen` -> `i <= VecLen` bound flip is unobservable
// here: VecPtrAt is unchecked, so the extra read returns uncontrolled
// bytes that won't contain this address -- see ledger.)
bool test_pm2_find_below_all_returns_null(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    ProcMaps         maps;

    if (!ProcMapsLoad(&maps, ALLOCATOR_OF(&alloc))) {
        DefaultAllocatorDeinit(&alloc);
        return false;
    }

    // Address 0 / a tiny address is never mapped in a normal process.
    const ProcMapEntry *e = ProcMapsFindByAddr(&maps, (u64)0x1);

    bool ok = e == NULL;

    ProcMapsDeinit(&maps);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// An address in a GAP between two adjacent mappings resolves to nothing,
// and never accidentally to either neighbour. Reinforces the
// start/end range comparisons from the other side.
bool test_pm2_find_gap_returns_null(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    ProcMaps         maps;

    if (!ProcMapsLoad(&maps, ALLOCATOR_OF(&alloc))) {
        DefaultAllocatorDeinit(&alloc);
        return false;
    }

    // Find a real gap: an address >= some region's end that is < no
    // other region's start. We scan for the first such hole.
    bool ok        = false;
    bool found_gap = false;
    for (u64 i = 0; i < VecLen(&maps.entries) && !found_gap; ++i) {
        u64 hole = VecPtrAt(&maps.entries, i)->end;
        // Is `hole` inside any region?
        bool inside = false;
        for (u64 j = 0; j < VecLen(&maps.entries); ++j) {
            const ProcMapEntry *e = VecPtrAt(&maps.entries, j);
            if (hole >= e->start && hole < e->end) {
                inside = true;
                break;
            }
        }
        if (!inside) {
            found_gap                  = true;
            const ProcMapEntry *at_gap = ProcMapsFindByAddr(&maps, hole);
            ok                         = at_gap == NULL;
        }
    }

    // If by chance every region is back-to-back (no gap), fall back to a
    // definitely-unmapped high canonical-hole address.
    if (!found_gap) {
        const ProcMapEntry *at = ProcMapsFindByAddr(&maps, (u64)0x0000800000000000ULL);
        ok                     = at == NULL;
    }

    ProcMapsDeinit(&maps);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// The scan must stop at `length`, never read into spare capacity. We push 3
// entries then shrink `length` to 2 by hand; the 3rd stays resident at index 2.
// An address that only matches the stale slot must return NULL -- the original
// scans i=0,1 and misses it, while the `i <= VecLen` overrun would read index 2
// and wrongly match. Kills `i < VecLen` -> `i <= VecLen`.
bool test_pm2_find_no_overrun_past_length(void) {
    DebugAllocator alloc = DebugAllocatorInit();
    ProcMaps       pm;
    MemSet(&pm, 0, sizeof(pm));
    pm.entries = VecInitT(pm.entries, ALLOCATOR_OF(&alloc));

    ProcMapEntry e0 = {.start = 0x1000, .end = 0x2000, .perms = 0, .file_offset = 0};
    ProcMapEntry e1 = {.start = 0x3000, .end = 0x4000, .perms = 0, .file_offset = 0};
    ProcMapEntry e2 = {.start = 0x5000, .end = 0x6000, .perms = 0, .file_offset = 0};
    e0.path         = StrInit(ALLOCATOR_OF(&alloc));
    e1.path         = StrInit(ALLOCATOR_OF(&alloc));
    e2.path         = StrInit(ALLOCATOR_OF(&alloc));

    bool pushed = VecPushBackR(&pm.entries, e0) && VecPushBackR(&pm.entries, e1) && VecPushBackR(&pm.entries, e2);
    if (!pushed) {
        ProcMapsDeinit(&pm);
        DebugAllocatorDeinit(&alloc);
        return false;
    }

    // Logically drop the 3rd entry; its bytes stay at index 2 in capacity.
    pm.entries.length = 2;

    // 0x5500 is inside the stale e2 range only -- must be unreachable.
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

// --------------------------------------------------------------------------
// ProcMapsDeinit: actually frees every entry's owned path + entries vector.
// --------------------------------------------------------------------------

// Under a DebugAllocator, every allocation made by the load must be
// released by Deinit. If `StrDeinit(&e->path)` is dropped from the per-entry
// loop, those owned path copies leak and the live-allocation count stays
// above baseline. Asserting the count returns to baseline kills the
// removed-Deinit mutation observably.
bool test_pm2_deinit_releases_all(void) {
    DebugAllocator dbg  = DebugAllocatorInit();
    Allocator     *base = ALLOCATOR_OF(&dbg);

    size before = DebugAllocatorLiveCount(&dbg);

    ProcMaps maps;
    if (!proc_maps_load(&maps, base)) {
        DebugAllocatorDeinit(&dbg);
        return false;
    }

    // The load must have taken at least one live allocation (raw buffer
    // and/or entries vector); otherwise the assertion below is vacuous.
    size during = DebugAllocatorLiveCount(&dbg);
    bool ok     = during > before;

    ProcMapsDeinit(&maps);

    // After Deinit every load allocation is gone again.
    size after = DebugAllocatorLiveCount(&dbg);
    ok         = ok && after == before;

    DebugAllocatorDeinit(&dbg);
    return ok;
}

// min_addr must equal the lowest `start` across all entries, and be non-zero
// for a real process. Pins the min_addr cache loop in proc_maps_load: a loop
// that never runs, starts past the entries, skips entry 0, or stores a constant
// leaves min_addr at 0 or above the true minimum.
bool test_pm2_min_addr_is_lowest_start(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    ProcMaps         maps;
    if (!ProcMapsLoad(&maps, ALLOCATOR_OF(&alloc))) {
        DefaultAllocatorDeinit(&alloc);
        return false;
    }

    bool ok = VecLen(&maps.entries) > 0;
    if (ok) {
        u64 lowest = VecPtrAt(&maps.entries, 0)->start;
        for (u64 i = 1; i < VecLen(&maps.entries); ++i) {
            u64 start = VecPtrAt(&maps.entries, i)->start;
            if (start < lowest)
                lowest = start;
        }
        ok = maps.min_addr == lowest && maps.min_addr != 0;
    }

    ProcMapsDeinit(&maps);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// --------------------------------------------------------------------------
// Line parsing via the public LoadFrom seam. Crafted maps-text exercises the
// hex / perms / token / path decode that the live `/proc/self/maps` loader
// can't pin (its content is non-deterministic). Each test asserts EXACT
// parsed fields so any digit-table, accumulation, perms-bit, token-skip, or
// in-place NUL mutation diverges observably.
// --------------------------------------------------------------------------

// A single well-formed line decodes to EXACTLY these fields. Distinctive,
// non-overlapping values mean any field/shift/perms-bit mutation shows up.
// The `dead` offset carries lowercase a/d/e hex; r-xp pins three perms bits
// set and one clear; the path proves the dev+inode tokens were skipped and
// the trailing '\n' was turned into the path's NUL terminator.
bool test_pm_parse_full_line_fields(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    ProcMaps         m;
    if (!pm_load_text(&m, "1000-2000 r-xp 0000dead 08:01 12345 /x/y\n", &alloc)) {
        DefaultAllocatorDeinit(&alloc);
        return false;
    }

    bool ok = VecLen(&m.entries) == 1;
    if (ok) {
        const ProcMapEntry *e = VecPtrAt(&m.entries, 0);
        ok                    = e->start == 0x1000ULL && e->end == 0x2000ULL && e->file_offset == 0xdeadULL &&
             e->perms == (u32)(PROC_MAP_PERM_READ | PROC_MAP_PERM_EXEC | PROC_MAP_PERM_PRIVATE) && path_eq(e, "/x/y");
    }

    ProcMapsDeinit(&m);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// Uppercase A-F hex digits decode to the same values as lowercase. Pins the
// `'A'..'F'` arm of the digit table: without it an uppercase address is
// rejected or mis-valued, so a crafted uppercase start/end/offset is the only
// way to reach that branch (the kernel only ever emits lowercase).
bool test_pm_parse_uppercase_hex_addr(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    ProcMaps         m;
    if (!pm_load_text(&m, "ABCDEF-FEDCBA r-xp 0000CA00 00:00 0 /u\n", &alloc)) {
        DefaultAllocatorDeinit(&alloc);
        return false;
    }

    bool ok = VecLen(&m.entries) == 1;
    if (ok) {
        const ProcMapEntry *e = VecPtrAt(&m.entries, 0);
        ok                    = e->start == 0xABCDEFULL && e->end == 0xFEDCBAULL && e->file_offset == 0xCA00ULL;
    }

    ProcMapsDeinit(&m);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// The digit '9' sits on the upper edge of the `'0'..'9'` table arm. A start
// and end that hinge on '9' decoding to 9 (not being rejected) pin that
// boundary: drop it and the address parse stops early or skips the line.
bool test_pm_parse_addr_with_nine(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    ProcMaps         m;
    if (!pm_load_text(&m, "9-1990 r-xp 0 0:0 0 /n\n", &alloc)) {
        DefaultAllocatorDeinit(&alloc);
        return false;
    }

    bool ok = VecLen(&m.entries) == 1;
    if (ok) {
        const ProcMapEntry *e = VecPtrAt(&m.entries, 0);
        ok                    = e->start == 0x9ULL && e->end == 0x1990ULL;
    }

    ProcMapsDeinit(&m);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// An address field with NO hex digits (here an empty start, the line opening
// straight on the '-') is malformed, so the load fails. Pins the "no digits
// consumed -> reject" guard in the hex reader: a reader that reports success on
// an empty run would accept the line, so asserting the load FAILS is the kill.
bool test_pm_parse_rejects_empty_start_field(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    ProcMaps         m;
    bool             failed = !pm_load_text(&m, "-2000 r-xp 0 0:0 0 /x\n", &alloc);

    if (!failed) // a failed load already freed + zeroed `m`; only clean up on success
        ProcMapsDeinit(&m);
    DefaultAllocatorDeinit(&alloc);
    return failed;
}

// min_addr is the lowest start across ALL entries, regardless of file order.
// Two entries in DESCENDING address order force the cache loop to scan past
// entry 0: a loop that only ever sees the first entry (or runs backwards)
// would cache the higher start and miss the true minimum.
bool test_pm_parse_min_addr_descending(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    ProcMaps         m;
    if (!pm_load_text(&m, "5000-6000 r-xp 0 0:0 0 /a\n1000-2000 r-xp 0 0:0 0 /b\n", &alloc)) {
        DefaultAllocatorDeinit(&alloc);
        return false;
    }

    bool ok = VecLen(&m.entries) == 2;
    if (ok) {
        const ProcMapEntry *e0 = VecPtrAt(&m.entries, 0);
        const ProcMapEntry *e1 = VecPtrAt(&m.entries, 1);
        ok                     = e0->start == 0x5000ULL && e1->start == 0x1000ULL && m.min_addr == 0x1000ULL;
    }

    ProcMapsDeinit(&m);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// A line the grammar cannot parse fails the whole load -- this is not a
// compiler, so the first bad line stops the parse rather than being skipped over
// to reach a later good line. Feeding "garbage" ahead of a well-formed line and
// asserting the load FAILS pins that: a skip-and-continue reader would instead
// return the good entry.
bool test_pm_parse_fails_on_malformed_line(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    ProcMaps         m;
    bool             failed = !pm_load_text(&m, "garbage line here\n3000-4000 r-xp 0 0:0 0 /good\n", &alloc);

    if (!failed) // a failed load already freed + zeroed `m`; only clean up on success
        ProcMapsDeinit(&m);
    DefaultAllocatorDeinit(&alloc);
    return failed;
}

// Field-level fault tolerance: a well-formed line up to a point, then one bad
// field. The grammar has no partial-line recovery -- a bad field fails the line
// and thus the load. Each of these logs a caret diagnostic (see the [ERROR]
// lines when the suite runs); the assertion is just that the load fails.

// A non-hex character in the END address fails the `end` field.
bool test_pm_parse_fails_on_bad_end(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    ProcMaps         m;
    bool             failed = !pm_load_text(&m, "1000-XYZ r-xp 0 0:0 0 /x\n", &alloc);

    if (!failed)
        ProcMapsDeinit(&m);
    DefaultAllocatorDeinit(&alloc);
    return failed;
}

// A character outside `rwxp-s` in the permission field fails `perms`.
bool test_pm_parse_fails_on_bad_perms(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    ProcMaps         m;
    bool             failed = !pm_load_text(&m, "1000-2000 rZxp 0 0:0 0 /x\n", &alloc);

    if (!failed)
        ProcMapsDeinit(&m);
    DefaultAllocatorDeinit(&alloc);
    return failed;
}

// A non-hex character in the file-offset field fails `offset`.
bool test_pm_parse_fails_on_bad_offset(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    ProcMaps         m;
    bool             failed = !pm_load_text(&m, "1000-2000 r-xp ZZZ 0:0 0 /x\n", &alloc);

    if (!failed)
        ProcMapsDeinit(&m);
    DefaultAllocatorDeinit(&alloc);
    return failed;
}

// A line that ends before its offset/dev/inode fields fails: the next field has
// nothing to parse.
bool test_pm_parse_fails_on_truncated_line(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    ProcMaps         m;
    bool             failed = !pm_load_text(&m, "1000-2000 r-xp\n", &alloc);

    if (!failed)
        ProcMapsDeinit(&m);
    DefaultAllocatorDeinit(&alloc);
    return failed;
}

// The File loader must read to TRUE EOF, not stop at the first chunk. We write
// a maps file larger than one read chunk whose LAST line is a distinctive high
// mapping, then load it through the File seam: a loader that breaks after the
// first short read drops every entry past the chunk boundary, so the high
// entry would be missing.
bool test_pm_parse_large_file_reads_all_chunks(void) {
    DefaultAllocator alloc      = DefaultAllocatorInit();
    Allocator       *alloc_base = ALLOCATOR_OF(&alloc);

    Str  path;
    File f = FileOpenTemp(&path, alloc_base);
    if (!FileIsOpen(&f)) {
        DefaultAllocatorDeinit(&alloc);
        return false;
    }

    // 200 * 26 bytes = 5200 bytes of filler, comfortably past one 4096 chunk,
    // before the distinctive trailing entry lands.
    Zstr filler   = "1000-1001 ---p 0 0:0 0 /f\n";
    bool wrote_ok = true;
    for (int i = 0; i < 200 && wrote_ok; ++i) {
        i64 w    = FileWrite(&f, filler, (u64)ZstrLen(filler));
        wrote_ok = (w == (i64)ZstrLen(filler));
    }
    Zstr late = "deadbeef000-deadbeef100 r-xp 0 0:0 0 /late\n";
    if (wrote_ok) {
        i64 w    = FileWrite(&f, late, (u64)ZstrLen(late));
        wrote_ok = (w == (i64)ZstrLen(late));
    }
    FileClose(&f);
    if (!wrote_ok) {
        FileRemove(&path);
        StrDeinit(&path);
        DefaultAllocatorDeinit(&alloc);
        return false;
    }

    File rf = FileOpen(&path, "rb");
    if (!FileIsOpen(&rf)) {
        FileRemove(&path);
        StrDeinit(&path);
        DefaultAllocatorDeinit(&alloc);
        return false;
    }

    ProcMaps m;
    bool     loaded = ProcMapsLoadFrom(&m, &rf, &alloc);
    FileClose(&rf);

    bool ok = loaded;
    if (ok) {
        // The trailing high mapping lives past the first read chunk; a loader
        // that stopped early would never have parsed it.
        const ProcMapEntry *e = ProcMapsFindByAddr(&m, 0xdeadbeef050ULL);
        ok = e != NULL && e->start == 0xdeadbeef000ULL && e->end == 0xdeadbeef100ULL && path_eq(e, "/late");
        ProcMapsDeinit(&m);
    }

    FileRemove(&path);
    StrDeinit(&path);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// A File that opens but whose reads error out drives load_from_file's
// read-failure arm. A directory opens read-only yet every read() returns an
// error, so the chunked reader fails AFTER its first StrReserve has already
// grown `raw` -- the failure arm must release that partially-grown buffer.
// Under a DebugAllocator the live-allocation count must return to baseline;
// if the arm's ProcMapsDeinit is removed the kilobyte raw buffer leaks and the
// count stays elevated. Asserting baseline (and that the load reported failure)
// kills the removed-Deinit mutation on the read-failure arm.
bool test_pm_load_from_file_read_fail_frees_raw(void) {
    DebugAllocator dbg  = DebugAllocatorInit();
    Allocator     *base = ALLOCATOR_OF(&dbg);

    // A directory: open(2) succeeds (FileIsOpen true) but read(2) returns -1.
    File dir = FileOpen("/proc/self", "rb");
    if (!FileIsOpen(&dir)) {
        DebugAllocatorDeinit(&dbg);
        return false;
    }

    size before = DebugAllocatorLiveCount(&dbg);

    ProcMaps m;
    bool     loaded = ProcMapsLoadFrom(&m, &dir, base);
    FileClose(&dir);

    // The unreadable directory makes the chunked read fail, so the loader
    // takes the read-failure cleanup arm and reports failure.
    bool ok = (loaded == false);

    // That arm must have freed everything the partial read reserved.
    size after = DebugAllocatorLiveCount(&dbg);
    ok         = ok && (after == before);

    DebugAllocatorDeinit(&dbg);
    return ok;
}

int main(void) {
    WriteFmt("[INFO] Starting ProcMaps tests\n\n");

    TestFunction tests[] = {
        test_procmaps_load,
        test_procmaps_find_self,
        test_pm2_load_populates_many_entries,
        test_pm2_load_function_addr_in_exec_region,
        test_pm2_load_stack_addr_in_rw_region,
        test_pm2_find_start_boundary_inside,
        test_pm2_find_end_boundary_excluded,
        test_pm2_find_below_all_returns_null,
        test_pm2_find_gap_returns_null,
        test_pm2_find_no_overrun_past_length,
        test_pm2_deinit_releases_all,
        test_pm2_min_addr_is_lowest_start,

        test_pm_parse_full_line_fields,
        test_pm_parse_uppercase_hex_addr,
        test_pm_parse_addr_with_nine,
        test_pm_parse_rejects_empty_start_field,
        test_pm_parse_min_addr_descending,
        test_pm_parse_fails_on_malformed_line,
        test_pm_parse_fails_on_bad_end,
        test_pm_parse_fails_on_bad_perms,
        test_pm_parse_fails_on_bad_offset,
        test_pm_parse_fails_on_truncated_line,
        test_pm_parse_large_file_reads_all_chunks,
        test_pm_load_from_file_read_fail_frees_raw,
    };

    return run_test_suite(tests, sizeof(tests) / sizeof(tests[0]), NULL, 0, "ProcMaps");
}
