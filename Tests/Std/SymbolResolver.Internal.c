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
///
/// (The module cache + teardown is now covered through the public
/// Init/Resolve/Deinit API in `SymbolResolver.c`, not here.)
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
    };

    return run_test_suite(tests, sizeof(tests) / sizeof(tests[0]), NULL, 0, "SymbolResolver.Internal");
}
