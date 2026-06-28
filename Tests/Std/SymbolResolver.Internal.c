/// file      : Tests/Std/SymbolResolver.Internal.c
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Mutation-hardening for the STATIC internals of
/// `Sys/SymbolResolver.c`, reachable only by including the source unit
/// directly so the statics become local to this object:
///
///   * sidecar / build-id path handling (test_sr2_*):
///     `append_build_id_path`, `sidecar_matches`, `try_open_sidecar`.
///   * module cache + teardown (test_sr3_*):
///     `resolver_cache_find_or_open` (find-loop bound, key compare,
///     hit-vs-miss branch) and `SymbolResolverDeinit` (per-entry close
///     loop + map/vec teardown).
///
/// The test executable already supplies the public symbols
/// (SymbolResolverInit, SymbolResolverResolve, SymbolResolverDeinit) via
/// the single source include below, so the linker never pulls the
/// library's SymbolResolver object — no duplicate definitions.

#include <Misra.h>
#include <Misra/Std/Allocator/Debug.h>
#include <Misra/Std/Allocator/Default.h>
#include <Misra/Std/Zstr.h>
#include <Misra/Sys/Dir.h>
#include <Misra/Sys/SymbolResolver.h>

#include "../Util/TestRunner.h"

// Pull in the unit under test ONCE so all the static helpers
// (sidecar / build-id path handling + the module cache) become callable
// and the cache-entry internals are visible to the asserts.
#include "../../Source/Misra/Sys/SymbolResolver.c"

// ===========================================================================
// test_sr2_* — sidecar / build-id path handling
// ===========================================================================

// ---------------------------------------------------------------------------
// append_build_id_path — PURE: assert the EXACT formatted path fragment.
//
// Format: first byte -> "AA/", remaining bytes -> "BBBB...", lowercase
// hex, high nibble then low nibble per byte. The full sidecar path is
// "/usr/lib/debug/.build-id/" + this + ".debug".
// ---------------------------------------------------------------------------

// Compare the produced fragment against an expected NUL-terminated string.
static bool bid_fragment_eq(const u8 *id, u32 n, const char *expect) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Str              out   = StrInit(ALLOCATOR_OF(&alloc));
    append_build_id_path(&out, id, n);
    StrPushBackR(&out, '\0'); // terminate so ZstrCompare is well-defined
    bool ok = ZstrCompare(StrBegin(&out), expect) == 0;
    StrDeinit(&out);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// Canonical multi-byte build-id -> exact "ab/cdef0123" fragment. Pins the
// first-byte/rest split index, the high/low nibble order, lowercase hex,
// and the loop running over bytes [1, n).
bool test_sr2_bid_canonical(void) {
    const u8 id[] = {0xab, 0xcd, 0xef, 0x01, 0x23};
    return bid_fragment_eq(id, 5, "ab/cdef0123");
}

// Full path assembled exactly as try_open_sidecar would: prefix + fragment
// + ".debug". Pins the whole observable string.
bool test_sr2_bid_full_path(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Str              out   = StrInit(ALLOCATOR_OF(&alloc));
    StrPushBackMany(&out, "/usr/lib/debug/.build-id/");
    const u8 id[] = {0xab, 0xcd, 0xef, 0x01, 0x23};
    append_build_id_path(&out, id, 5);
    StrPushBackMany(&out, ".debug");
    StrPushBackR(&out, '\0');
    bool ok = ZstrCompare(StrBegin(&out), "/usr/lib/debug/.build-id/ab/cdef0123.debug") == 0;
    StrDeinit(&out);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// Single-byte build-id: only the "AA/" dir, no filename bytes. Guards the
// loop bound (start at 1, so nothing after the slash) and the dir split.
bool test_sr2_bid_single_byte(void) {
    const u8 id[] = {0x7f};
    return bid_fragment_eq(id, 1, "7f/");
}

// Two-byte build-id: "AA/BB". Distinguishes the +/- on the split index
// (byte 0 -> dir, byte 1 -> first filename byte).
bool test_sr2_bid_two_bytes(void) {
    const u8 id[] = {0x12, 0x34};
    return bid_fragment_eq(id, 2, "12/34");
}

// n == 0 -> empty fragment (early return). Guards the `n == 0` test.
bool test_sr2_bid_empty(void) {
    const u8 id[] = {0xff};
    return bid_fragment_eq(id, 0, "");
}

// High nibble vs low nibble ordering: 0xa0 -> "a0" not "0a"; 0x0f -> "0f".
// Single byte goes to the dir, so use it as byte 0 and check the dir.
bool test_sr2_bid_nibble_order_high(void) {
    const u8 id[] = {0xa0};
    return bid_fragment_eq(id, 1, "a0/");
}

bool test_sr2_bid_nibble_order_low(void) {
    const u8 id[] = {0x0f};
    return bid_fragment_eq(id, 1, "0f/");
}

// A filename byte (index 1) exercises the same shift/mask on the loop path,
// distinct from the dir byte. 0x5c -> dir "00/", filename "5c".
bool test_sr2_bid_filename_nibble(void) {
    const u8 id[] = {0x00, 0x5c};
    return bid_fragment_eq(id, 2, "00/5c");
}

// Boundary hex values 0x00 and 0xff hit '0' and 'f' on both nibbles.
bool test_sr2_bid_boundary_hex(void) {
    const u8 id[] = {0x00, 0xff};
    return bid_fragment_eq(id, 2, "00/ff");
}

// ---------------------------------------------------------------------------
// sidecar_matches — exact match/no-match decisions.
//
// by_build_id == false -> always true.
// by_build_id == true  -> requires both build_ids present, equal sizes,
//                         and byte-for-byte equal contents.
// We fabricate Elf structs by hand: only the build_id fields are read.
// ---------------------------------------------------------------------------

static Elf make_elf_with_build_id(const u8 *id, u32 size) {
    Elf e;
    MemSet(&e, 0, sizeof(e));
    e.build_id      = id;
    e.build_id_size = size;
    return e;
}

// Not by build-id: matches unconditionally (debuglink presence is enough).
bool test_sr2_match_not_by_build_id(void) {
    const u8 a[] = {0x01};
    const u8 b[] = {0x02};
    Elf      ea  = make_elf_with_build_id(a, 1);
    Elf      eb  = make_elf_with_build_id(b, 1);
    // Even with differing build-ids, by_build_id=false -> true.
    return sidecar_matches(&ea, &eb, false) == true;
}

// Equal build-ids -> match. Pins the MemCompare == 0 branch.
bool test_sr2_match_equal_build_ids(void) {
    const u8 a[] = {0xde, 0xad, 0xbe, 0xef};
    const u8 b[] = {0xde, 0xad, 0xbe, 0xef};
    Elf      ea  = make_elf_with_build_id(a, 4);
    Elf      eb  = make_elf_with_build_id(b, 4);
    return sidecar_matches(&ea, &eb, true) == true;
}

// One byte differs -> no match. Guards the MemCompare result test
// (== 0 -> != 0 mutation flips this to a false "match").
bool test_sr2_match_one_byte_differs(void) {
    const u8 a[] = {0xde, 0xad, 0xbe, 0xef};
    const u8 b[] = {0xde, 0xad, 0xbe, 0xee}; // last byte differs
    Elf      ea  = make_elf_with_build_id(a, 4);
    Elf      eb  = make_elf_with_build_id(b, 4);
    return sidecar_matches(&ea, &eb, true) == false;
}

// First byte differs -> no match (MemCompare must scan from the front).
bool test_sr2_match_first_byte_differs(void) {
    const u8 a[] = {0x00, 0xad, 0xbe, 0xef};
    const u8 b[] = {0x01, 0xad, 0xbe, 0xef};
    Elf      ea  = make_elf_with_build_id(a, 4);
    Elf      eb  = make_elf_with_build_id(b, 4);
    return sidecar_matches(&ea, &eb, true) == false;
}

// Different sizes -> no match. Guards the size compare (!= -> ==).
bool test_sr2_match_size_mismatch(void) {
    const u8 a[] = {0xde, 0xad, 0xbe, 0xef};
    const u8 b[] = {0xde, 0xad, 0xbe};
    Elf      ea  = make_elf_with_build_id(a, 4);
    Elf      eb  = make_elf_with_build_id(b, 3);
    return sidecar_matches(&ea, &eb, true) == false;
}

// Same prefix but different lengths where the shorter is a prefix of the
// longer: still no match because sizes differ (not a content scan).
bool test_sr2_match_prefix_size_mismatch(void) {
    const u8 a[] = {0x11, 0x22};
    const u8 b[] = {0x11, 0x22, 0x33};
    Elf      ea  = make_elf_with_build_id(a, 2);
    Elf      eb  = make_elf_with_build_id(b, 3);
    return sidecar_matches(&ea, &eb, true) == false;
}

// main has no build-id -> no match.
bool test_sr2_match_main_no_build_id(void) {
    const u8 b[] = {0x01};
    Elf      ea  = make_elf_with_build_id(NULL, 0);
    Elf      eb  = make_elf_with_build_id(b, 1);
    return sidecar_matches(&ea, &eb, true) == false;
}

// sidecar has no build-id -> no match.
bool test_sr2_match_sidecar_no_build_id(void) {
    const u8 a[] = {0x01};
    Elf      ea  = make_elf_with_build_id(a, 1);
    Elf      eb  = make_elf_with_build_id(NULL, 0);
    return sidecar_matches(&ea, &eb, true) == false;
}

// ---------------------------------------------------------------------------
// try_open_sidecar — drive the debuglink paths with a REAL on-disk sidecar.
//
// Path #2 is "{dir(main_path)}/{debuglink_name}" and #3 is
// "{dir(main_path)}/.debug/{debuglink_name}". We materialise a genuine
// ELF (a byte-for-byte copy of this very test binary, /proc/self/exe) at
// those locations so `ElfOpen` succeeds and the sidecar is accepted
// (by_build_id == false -> sidecar_matches returns true).
// ---------------------------------------------------------------------------

// Copy /proc/self/exe to `dst`. Returns true on success.
static bool copy_self_exe(Zstr dst) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Buf              bytes = BufInit(ALLOCATOR_OF(&alloc));
    bool             ok    = false;
    if (FileReadAndClose("/proc/self/exe", &bytes) >= 0) {
        ok = FileWriteAndClose(dst, BufData(&bytes), BufLength(&bytes)) >= 0;
    }
    BufDeinit(&bytes);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// Build a `main` Elf with NO build-id (so the build-id /usr/lib/debug
// branch is skipped) but a debuglink_name set, so try_open_sidecar takes
// the debuglink search.
static Elf make_main_with_debuglink(Zstr debuglink) {
    Elf e;
    MemSet(&e, 0, sizeof(e));
    e.build_id       = NULL;
    e.build_id_size  = 0;
    e.debuglink_name = debuglink;
    return e;
}

// Path #2: "{dir}/{debuglink}". Materialise the sidecar right next to the
// (notional) binary and assert try_open_sidecar opens it.
bool test_sr2_open_sidecar_adjacent(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Zstr             dir   = "/tmp/sr2_adj";
    DirRemoveAll(dir);
    DirCreateAll(dir);

    if (!copy_self_exe("/tmp/sr2_adj/side.debug")) {
        DirRemoveAll(dir);
        DefaultAllocatorDeinit(&alloc);
        return false;
    }

    Elf main = make_main_with_debuglink("side.debug");
    Elf out;
    MemSet(&out, 0, sizeof(out));
    // main_path lives in `dir`; its basename is irrelevant (no real file).
    bool ok = try_open_sidecar("/tmp/sr2_adj/fakebin", &main, &out, ALLOCATOR_OF(&alloc));
    if (ok)
        ElfDeinit(&out);

    DirRemoveAll(dir);
    DefaultAllocatorDeinit(&alloc);
    return ok == true;
}

// Path #3: "{dir}/.debug/{debuglink}". Sidecar only in the .debug subdir,
// NOT adjacent, so path #2 misses and path #3 must hit. This pins the
// ".debug/" literal and the sys_append_dirname/StrPushBackMany calls on
// that branch.
bool test_sr2_open_sidecar_debug_subdir(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Zstr             dir   = "/tmp/sr2_sub";
    DirRemoveAll(dir);
    DirCreateAll("/tmp/sr2_sub/.debug");

    if (!copy_self_exe("/tmp/sr2_sub/.debug/side.debug")) {
        DirRemoveAll(dir);
        DefaultAllocatorDeinit(&alloc);
        return false;
    }

    Elf main = make_main_with_debuglink("side.debug");
    Elf out;
    MemSet(&out, 0, sizeof(out));
    bool ok = try_open_sidecar("/tmp/sr2_sub/fakebin", &main, &out, ALLOCATOR_OF(&alloc));
    if (ok)
        ElfDeinit(&out);

    DirRemoveAll(dir);
    DefaultAllocatorDeinit(&alloc);
    return ok == true;
}

// No debuglink and no build-id -> nothing to search -> returns false.
// Guards the debuglink-presence gate (line 89) and the final return false.
bool test_sr2_open_sidecar_none(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Elf              main;
    MemSet(&main, 0, sizeof(main)); // no build_id, no debuglink
    Elf out;
    MemSet(&out, 0, sizeof(out));
    bool ok = try_open_sidecar("/tmp/sr2_none/fakebin", &main, &out, ALLOCATOR_OF(&alloc));
    DefaultAllocatorDeinit(&alloc);
    return ok == false;
}

// Debuglink set but the named sidecar does not exist anywhere reachable
// -> returns false (every path #2/#3/#4 misses on this host).
bool test_sr2_open_sidecar_missing(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Zstr             dir   = "/tmp/sr2_miss";
    DirRemoveAll(dir);
    DirCreateAll(dir);

    Elf main = make_main_with_debuglink("nope.debug");
    Elf out;
    MemSet(&out, 0, sizeof(out));
    bool ok = try_open_sidecar("/tmp/sr2_miss/fakebin", &main, &out, ALLOCATOR_OF(&alloc));

    DirRemoveAll(dir);
    DefaultAllocatorDeinit(&alloc);
    return ok == false;
}

// Adjacent file exists but is NOT a valid ELF -> ElfOpen fails -> path #2
// rejected. With no other copy present, the whole search returns false.
// Pins the `&& ElfOpen(...)` short-circuit on path #2.
bool test_sr2_open_sidecar_adjacent_not_elf(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Zstr             dir   = "/tmp/sr2_bad";
    DirRemoveAll(dir);
    DirCreateAll(dir);

    // A non-ELF file at the adjacent path.
    const char junk[] = "this is not an elf file at all";
    if (FileWriteAndClose("/tmp/sr2_bad/side.debug", junk, sizeof(junk) - 1) < 0) {
        DirRemoveAll(dir);
        DefaultAllocatorDeinit(&alloc);
        return false;
    }

    Elf main = make_main_with_debuglink("side.debug");
    Elf out;
    MemSet(&out, 0, sizeof(out));
    bool ok = try_open_sidecar("/tmp/sr2_bad/fakebin", &main, &out, ALLOCATOR_OF(&alloc));

    DirRemoveAll(dir);
    DefaultAllocatorDeinit(&alloc);
    return ok == false;
}

// Path #3 with a non-ELF file in the .debug subdir: sys_path_exists is
// true but ElfOpen fails, so path #3 is rejected. Pins the
// `&& ElfOpen(...)` short-circuit on the .debug-subdir branch (a
// scalar-call mutation that forces ElfOpen truthy would wrongly accept
// it and return true).
bool test_sr2_open_sidecar_debug_subdir_not_elf(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Zstr             dir   = "/tmp/sr2_sub_bad";
    DirRemoveAll(dir);
    DirCreateAll("/tmp/sr2_sub_bad/.debug");

    const char junk[] = "definitely not elf";
    if (FileWriteAndClose("/tmp/sr2_sub_bad/.debug/side.debug", junk, sizeof(junk) - 1) < 0) {
        DirRemoveAll(dir);
        DefaultAllocatorDeinit(&alloc);
        return false;
    }

    Elf main = make_main_with_debuglink("side.debug");
    Elf out;
    MemSet(&out, 0, sizeof(out));
    bool ok = try_open_sidecar("/tmp/sr2_sub_bad/fakebin", &main, &out, ALLOCATOR_OF(&alloc));

    DirRemoveAll(dir);
    DefaultAllocatorDeinit(&alloc);
    return ok == false;
}

// ===========================================================================
// test_sr3_* — module cache + teardown
// ===========================================================================

// Two distinct, no-inline marker functions in OUR module. -O0 test builds
// keep their .symtab entries, so each resolves to a named symbol whose
// range our taken address lands in.
static __attribute__((noinline)) void sr3_marker_a(void) {
    __asm__ __volatile__("" ::
                             : "memory");
}
static __attribute__((noinline)) void sr3_marker_b(void) {
    __asm__ __volatile__("" ::
                             : "memory");
}

// Find an executable, file-backed mapping whose path differs from
// `self_path`. Returns the mapping start (a valid code address in another
// module) via `*out_addr`, or false if none exists.
static bool sr3_other_module_addr(SymbolResolver *res, Zstr self_path, u64 *out_addr) {
    for (u64 i = 0; i < VecLen(&res->maps.entries); ++i) {
        ProcMapEntry *m = VecPtrAt(&res->maps.entries, i);
        if (m->path && m->path[0] == '/' && (m->perms & PROC_MAP_PERM_EXEC) && ZstrCompare(m->path, self_path) != 0) {
            *out_addr = m->start;
            return true;
        }
    }
    return false;
}

// ---------------------------------------------------------------------------
// resolver_cache_find_or_open — cache HIT keeps one entry, no re-open
// ---------------------------------------------------------------------------

// Two resolves into the SAME module: the module is opened+cached on the
// first, and the second is a HIT that returns the cached entry. Kills the
// find-loop start/step mutations (i=42, ++i->--i) and the hit branch: a
// broken cache key or skipped loop re-opens the module -> a second cache
// entry and a fresh ELF allocation.
bool test_sr3_same_module_cache_hit_no_realloc(void) {
    DebugAllocator alloc = DebugAllocatorInit();
    SymbolResolver res;
    if (!SymbolResolverInit(&res, ALLOCATOR_OF(&alloc)))
        return false;

    ResolvedSymbol ra;
    bool           ok = SymbolResolverResolve(&res, (void *)&sr3_marker_a, &ra);
    ok = ok && ra.module_path && ra.symbol_name && ZstrFindSubstring(ra.symbol_name, "sr3_marker_a") != NULL;
    u64  len_after_first  = VecLen(&res.cache);
    size live_after_first = DebugAllocatorLiveCount(&alloc);

    // Second resolve, different address, SAME module -> cache HIT.
    ResolvedSymbol rb;
    ok = ok && SymbolResolverResolve(&res, (void *)&sr3_marker_b, &rb);
    ok = ok && rb.module_path && rb.symbol_name && ZstrFindSubstring(rb.symbol_name, "sr3_marker_b") != NULL;
    u64  len_after_second  = VecLen(&res.cache);
    size live_after_second = DebugAllocatorLiveCount(&alloc);

    // Exactly one cached module, opened once.
    ok = ok && len_after_first == 1 && len_after_second == 1;
    // No new allocation on the HIT: the module was not re-opened.
    ok = ok && live_after_second == live_after_first;
    // Both resolved to the very same module file.
    ok = ok && ZstrCompare(ra.module_path, rb.module_path) == 0;

    SymbolResolverDeinit(&res);
    DebugAllocatorDeinit(&alloc);
    return ok;
}

// ---------------------------------------------------------------------------
// resolver_cache_find_or_open — cross-module MISS makes a distinct entry
// ---------------------------------------------------------------------------

// Resolve in our module, then resolve in a DIFFERENT module. The second
// lookup must NOT match the first cached entry (different path): it is a
// MISS that opens + inserts a second entry. Kills the key-compare swaps
// (== -> != at the pointer compare and at the ZstrCompare==0 fallback):
// a flipped compare returns the WRONG first entry for the second module,
// so the second address resolves against the wrong file.
bool test_sr3_cross_module_distinct_entry(void) {
    DebugAllocator alloc = DebugAllocatorInit();
    SymbolResolver res;
    if (!SymbolResolverInit(&res, ALLOCATOR_OF(&alloc)))
        return false;

    ResolvedSymbol ra;
    bool           ok = SymbolResolverResolve(&res, (void *)&sr3_marker_a, &ra);
    ok                = ok && ra.module_path;
    u64 len1          = VecLen(&res.cache);

    u64 other = 0;
    ok        = ok && sr3_other_module_addr(&res, ra.module_path, &other);

    ResolvedSymbol ro;
    ok       = ok && SymbolResolverResolve(&res, (void *)other, &ro);
    u64 len2 = VecLen(&res.cache);

    // First resolve: one entry. Cross-module resolve: a second, distinct
    // entry. A flipped key compare would re-use entry[0] -> len stays 1
    // and ro.module_path would equal ra.module_path (wrong file).
    ok = ok && len1 == 1 && len2 == 2;
    ok = ok && ro.module_path && ZstrCompare(ro.module_path, ra.module_path) != 0;

    SymbolResolverDeinit(&res);
    DebugAllocatorDeinit(&alloc);
    return ok;
}

// Resolving the cross-module address AGAIN is a HIT on the second entry:
// the cache stays at two entries and nothing is re-opened. This pins the
// find-loop to actually walk past entry[0] and match entry[1].
bool test_sr3_second_module_hit(void) {
    DebugAllocator alloc = DebugAllocatorInit();
    SymbolResolver res;
    if (!SymbolResolverInit(&res, ALLOCATOR_OF(&alloc)))
        return false;

    ResolvedSymbol ra;
    bool           ok = SymbolResolverResolve(&res, (void *)&sr3_marker_a, &ra);
    ok                = ok && ra.module_path;

    u64 other = 0;
    ok        = ok && sr3_other_module_addr(&res, ra.module_path, &other);

    ResolvedSymbol r1;
    ok                   = ok && SymbolResolverResolve(&res, (void *)other, &r1);
    u64  len_before_hit  = VecLen(&res.cache);
    size live_before_hit = DebugAllocatorLiveCount(&alloc);

    ResolvedSymbol r2;
    ok                  = ok && SymbolResolverResolve(&res, (void *)other, &r2);
    u64  len_after_hit  = VecLen(&res.cache);
    size live_after_hit = DebugAllocatorLiveCount(&alloc);

    ok = ok && len_before_hit == 2 && len_after_hit == 2;
    ok = ok && live_after_hit == live_before_hit;
    ok = ok && r1.module_path && r2.module_path && ZstrCompare(r1.module_path, r2.module_path) == 0;

    SymbolResolverDeinit(&res);
    DebugAllocatorDeinit(&alloc);
    return ok;
}

// ---------------------------------------------------------------------------
// SymbolResolverDeinit — full teardown returns live count to baseline
// ---------------------------------------------------------------------------

// Populate the cache + ELF + DWARF lines by resolving, then Deinit. A
// correct teardown frees everything and the DebugAllocator's live count
// returns to the pre-resolver baseline (0). Each removed deinit in the
// cleanup loop (ELF close, DwarfLines close) and the trailing VecDeinit /
// the loop itself (start/bound/step) leaves outstanding allocations the
// live count catches.
bool test_sr3_deinit_frees_everything(void) {
    DebugAllocator alloc    = DebugAllocatorInit();
    size           baseline = DebugAllocatorLiveCount(&alloc);

    SymbolResolver res;
    if (!SymbolResolverInit(&res, ALLOCATOR_OF(&alloc)))
        return false;

    ResolvedSymbol ra;
    bool           ok = SymbolResolverResolve(&res, (void *)&sr3_marker_a, &ra);
    ok                = ok && ra.module_path;
    // Resolve a second module too, so the cleanup loop must walk >1 entry
    // (kills the loop-bound / start / step mutations: an under-run leaves
    // the later entries' ELFs + DWARF leaked).
    u64 other = 0;
    if (ok && sr3_other_module_addr(&res, ra.module_path, &other)) {
        ResolvedSymbol ro;
        ok = ok && SymbolResolverResolve(&res, (void *)other, &ro);
    }
    // After a successful resolve the cache holds at least one open ELF and
    // its built DwarfLines, so the live count is strictly above baseline.
    ok = ok && DebugAllocatorLiveCount(&alloc) > baseline;

    SymbolResolverDeinit(&res);
    ok = ok && DebugAllocatorLiveCount(&alloc) == baseline;

    DebugAllocatorDeinit(&alloc);
    return ok;
}

int main(void) {
    WriteFmt("[INFO] Starting SymbolResolver.Internal tests\n\n");

    TestFunction tests[] = {
        // --- test_sr2_*: append_build_id_path ---
        test_sr2_bid_canonical,
        test_sr2_bid_full_path,
        test_sr2_bid_single_byte,
        test_sr2_bid_two_bytes,
        test_sr2_bid_empty,
        test_sr2_bid_nibble_order_high,
        test_sr2_bid_nibble_order_low,
        test_sr2_bid_filename_nibble,
        test_sr2_bid_boundary_hex,

        // --- test_sr2_*: sidecar_matches ---
        test_sr2_match_not_by_build_id,
        test_sr2_match_equal_build_ids,
        test_sr2_match_one_byte_differs,
        test_sr2_match_first_byte_differs,
        test_sr2_match_size_mismatch,
        test_sr2_match_prefix_size_mismatch,
        test_sr2_match_main_no_build_id,
        test_sr2_match_sidecar_no_build_id,

        // --- test_sr2_*: try_open_sidecar ---
        test_sr2_open_sidecar_adjacent,
        test_sr2_open_sidecar_debug_subdir,
        test_sr2_open_sidecar_none,
        test_sr2_open_sidecar_missing,
        test_sr2_open_sidecar_adjacent_not_elf,
        test_sr2_open_sidecar_debug_subdir_not_elf,

        // --- test_sr3_*: cache + deinit ---
        test_sr3_same_module_cache_hit_no_realloc,
        test_sr3_cross_module_distinct_entry,
        test_sr3_second_module_hit,
        test_sr3_deinit_frees_everything,
    };

    return run_test_suite(tests, sizeof(tests) / sizeof(tests[0]), NULL, 0, "SymbolResolver.Internal");
}
