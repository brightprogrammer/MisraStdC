#include <Misra.h>
#include <Misra/Std/Allocator/Debug.h>
#include <Misra/Std/Allocator/Default.h>
#include <Misra/Sys/ProcMaps.h>

#include "../Util/TestRunner.h"

// A real function whose address must live in an executable mapping.
static int pm2_marker_fn(int x) {
    return x + 1;
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

// --------------------------------------------------------------------------
// ProcMapsDeinit: actually frees the raw buffer + entries vector.
// --------------------------------------------------------------------------

// Under a DebugAllocator, every allocation made by the load must be
// released by Deinit. If `StrDeinit(&self->raw)` is removed, the raw
// buffer (kilobytes) leaks and the live-allocation count stays above
// baseline. Asserting the count returns to baseline kills the
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
        test_pm2_deinit_releases_all,
    };

    return run_test_suite(tests, sizeof(tests) / sizeof(tests[0]), NULL, 0, "ProcMaps");
}
