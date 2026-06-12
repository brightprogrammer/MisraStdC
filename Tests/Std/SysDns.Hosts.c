/// file      : Tests/Std/SysDns.Mutants1.c
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Mutation-hardening for the /etc/hosts-parsing path of `Sys/Dns.c`:
/// `parse_hosts_table`, `slurp_file`, `skip_to_eol_iter`.
///
/// These three are `static` and the public resolver drives them off the
/// live, non-deterministic `/etc/hosts`. To assert EXACT parsed results
/// for CRAFTED hosts content we include the source unit directly (so the
/// static functions become local to this object) and intercept the one
/// `file_open` call inside `slurp_file`: when the parser asks for
/// `HOSTS_FILE_PATH` ("/etc/hosts") we hand it a temp file we wrote
/// instead. `slurp_file`, `parse_hosts_table` and `skip_to_eol_iter` all
/// run as real, unmodified code -- only the path that the open syscall
/// resolves is swapped, so the deterministic crafted bytes flow through
/// the real parse loop.

#include <Misra.h>
#include <Misra/Std/Allocator/Default.h>
#include <Misra/Std/File.h>
#include <Misra/Std/Zstr.h>
#include <Misra/Std/Utility/StrIter.h>
#include <Misra/Sys/Dns.h>

#include "../Util/TestRunner.h"

// ---------------------------------------------------------------------------
// file_open interception.
//
// `slurp_file` opens its argument via `FileOpen(path, "rb")`, which the
// header's `_Generic` lowers to `file_open(path, "rb")`. We shadow that
// one call: if the parser is reaching for the well-known hosts path, we
// redirect to whatever temp file the running test staged in
// `g_redirect_to`; every other path falls through to the real
// `file_open`. The parenthesised `(file_open)(...)` form suppresses the
// function-like macro so the real symbol is still callable.
// ---------------------------------------------------------------------------

static const char *g_redirect_to = (const char *)0; // temp path or NULL
static const char *g_redirect_from =
#if PLATFORM_WINDOWS
    "C:\\Windows\\System32\\drivers\\etc\\hosts";
#else
    "/etc/hosts";
#endif

static bool zstr_eq(const char *a, const char *b) {
    return ZstrCompare(a, b) == 0;
}

static File test_file_open(Zstr path, Zstr mode) {
    if (g_redirect_to && zstr_eq(path, g_redirect_from)) {
        return (file_open)(g_redirect_to, mode);
    }
    return (file_open)(path, mode);
}

#define file_open(path, mode) test_file_open((path), (mode))

// Pull in the unit under test. After this, `slurp_file`,
// `parse_hosts_table` and `skip_to_eol_iter` are local statics calling
// our `test_file_open` shim.
#include "../../Source/Misra/Sys/Dns.c"

#undef file_open

// ---------------------------------------------------------------------------
// Fixtures.
// ---------------------------------------------------------------------------

// Stage `content` (length `n`) into a unique temp file and point the
// hosts-path redirect at it. Returns the temp path (static buffer, valid
// until the next stage_hosts call). NULL on write failure.
static char g_tmp_path[64];
static u64  g_tmp_seq = 0;

static const char *stage_hosts(const char *content, u64 n) {
    // Build a unique path without libc: /tmp/sd1_hosts_NN.
    static const char prefix[] = "/tmp/sd1_hosts_";
    u64               L        = sizeof(prefix) - 1;
    MemCopy(g_tmp_path, prefix, L);
    g_tmp_path[L]     = (char)('0' + (int)((g_tmp_seq / 10) % 10));
    g_tmp_path[L + 1] = (char)('0' + (int)(g_tmp_seq % 10));
    g_tmp_path[L + 2] = '\0';
    ++g_tmp_seq;

    i64 w = FileWriteAndClose((Zstr)g_tmp_path, (const void *)content, n);
    if (w < 0) {
        return (const char *)0;
    }
    g_redirect_to = g_tmp_path;
    return g_tmp_path;
}

static void unstage_hosts(void) {
    g_redirect_to = (const char *)0;
}

// Parse the currently-staged hosts file into a fresh table.
static HostsTable parse_staged(Allocator *a) {
    HostsTable t = VecInitT(t, a);
    parse_hosts_table(&t, a);
    return t;
}

// Find entry `name` (case-insensitive: caller passes lowercase) in the
// table. Returns pointer or NULL.
static const HostsEntry *find_host(const HostsTable *t, const char *name) {
    VecForeachPtr(t, e) {
        if (zstr_eq(StrBegin(&e->name), name)) {
            return e;
        }
    }
    return (const HostsEntry *)0;
}

// True when entry's IP is v4 and equals a.b.c.d.
static bool is_v4(const HostsEntry *e, u8 a, u8 b, u8 c, u8 d) {
    return e && !e->is_ipv6 && e->ip[0] == a && e->ip[1] == b && e->ip[2] == c && e->ip[3] == d;
}

// ---------------------------------------------------------------------------
// slurp_file -- direct, on real temp files (no redirect needed; we call
// slurp_file with the temp path directly).
// ---------------------------------------------------------------------------

// Full content is read; the LAST byte is present. Kills:
//  - 102: FileIsOpen replaced (forced -> early `return true` empty).
//  - 106/112: ok init/assign const (the loop / error flag).
//  - 121: FileClose removed/replaced (no observable here, see ledger).
bool test_sd1_slurp_reads_full_content(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);

    const char *body = "abcdefghij0123456789Z"; // last byte 'Z'
    u64         n    = 21;
    const char *path = stage_hosts(body, n);

    Str  out = StrInit(a);
    bool ok  = slurp_file((Zstr)path, &out);
    bool got = ok && StrLen(&out) == n && StrBegin(&out)[n - 1] == 'Z' && StrBegin(&out)[0] == 'a';

    StrDeinit(&out);
    unstage_hosts();
    DefaultAllocatorDeinit(&alloc);
    return got;
}

// A large file (> the 4096 chunk) is read across multiple FileRead
// iterations -- kills the loop-related init/assign mutants by requiring
// every byte through. Last byte distinct.
bool test_sd1_slurp_multi_chunk(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);

    u64 n   = 10000;
    Str big = StrInit(a);
    for (u64 i = 0; i < n; ++i) {
        StrPushBackR(&big, (char)('A' + (int)(i % 26)));
    }
    // Make the last byte unambiguous.
    StrBegin(&big)[n - 1] = '#';
    const char *path      = stage_hosts(StrBegin(&big), n);

    Str  out = StrInit(a);
    bool ok  = slurp_file((Zstr)path, &out);
    bool got = ok && StrLen(&out) == n && StrBegin(&out)[n - 1] == '#' &&
               StrBegin(&out)[4095] == (char)('A' + (4095 % 26)) && StrBegin(&out)[4096] == (char)('A' + (4096 % 26));

    StrDeinit(&out);
    StrDeinit(&big);
    unstage_hosts();
    DefaultAllocatorDeinit(&alloc);
    return got;
}

// Empty file -> slurp succeeds with empty Str.
bool test_sd1_slurp_empty_file(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);

    const char *path = stage_hosts("", 0);

    Str  out = StrInit(a);
    bool ok  = slurp_file((Zstr)path, &out);
    bool got = ok && StrLen(&out) == 0;

    StrDeinit(&out);
    unstage_hosts();
    DefaultAllocatorDeinit(&alloc);
    return got;
}

// Missing file -> slurp returns true (tolerated) with empty Str. Kills
// 102: if `!FileIsOpen` is flipped, a missing file would fall through to
// the read loop on a closed handle instead of returning true.
bool test_sd1_slurp_missing_file(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);

    Str  out = StrInit(a);
    bool ok  = slurp_file((Zstr) "/tmp/sd1_definitely_missing_zzqq", &out);
    bool got = ok && StrLen(&out) == 0;

    StrDeinit(&out);
    DefaultAllocatorDeinit(&alloc);
    return got;
}

// Open succeeds but the read errors: pointing slurp at a DIRECTORY opens
// fine (fd >= 0, FileIsOpen true) yet read() returns -1, driving the
// `n < 0 -> ok = false -> break` path. slurp_file must report failure.
// Kills 112:20 (ok = false assignment -> if forced true, slurp wrongly
// reports success).
bool test_sd1_slurp_read_error_returns_false(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);

    Str  out = StrInit(a);
    bool ok  = slurp_file((Zstr) "/tmp", &out); // a directory
    bool got = (ok == false);

    StrDeinit(&out);
    DefaultAllocatorDeinit(&alloc);
    return got;
}

// parse_hosts_table when slurp_file FAILS (read error): the early
// `if (!slurp_file(...)) { StrDeinit; return; }` must fire, leaving the
// table empty. Kills 156:10 (slurp return forced truthy -> early-return
// skipped -> parse proceeds on an empty/garbage buf) and exercises 157.
bool test_sd1_hosts_slurp_failure_empty_table(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);

    g_redirect_to = "/tmp"; // directory: open ok, read fails
    HostsTable t  = parse_staged(a);
    bool       ok = VecLen(&t) == 0;

    VecForeachPtr(&t, e) StrDeinit(&e->name);
    VecDeinit(&t);
    unstage_hosts();
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// ---------------------------------------------------------------------------
// skip_to_eol_iter -- direct on a crafted StrIter.
// ---------------------------------------------------------------------------

// From mid-line, skip_to_eol lands the cursor just past the next '\n'.
// Kills 146 (c != '\n' -> if flipped to ==, it would stop immediately /
// run wrong) and 149 (>0 guard on the final advance over the newline).
bool test_sd1_skip_eol_advances_past_newline(void) {
    Zstr    s  = "aaa\nbbb\nccc";
    StrIter si = StrIterFromZstr(s);
    // Move to index 1 (inside first line).
    StrIterMustNext(&si);
    skip_to_eol_iter(&si);
    // Cursor should now sit at 'b' (index 4).
    char c;
    bool ok = StrIterPeek(&si, &c) && c == 'b' && StrIterIndex(&si) == 4;
    return ok;
}

// skip_to_eol on the LAST line with no trailing newline consumes to EOF
// and leaves the iterator exhausted. Kills 149: if the remaining>0 guard
// is wrong, behaviour at the final char diverges.
bool test_sd1_skip_eol_last_line_no_newline(void) {
    Zstr    s  = "xy";
    StrIter si = StrIterFromZstr(s);
    skip_to_eol_iter(&si);
    bool ok = StrIterRemainingLength(&si) == 0;
    return ok;
}

// skip_to_eol at a '\n' already under the cursor consumes exactly that
// one newline (advances by one). Distinguishes the != / > mutants.
bool test_sd1_skip_eol_on_newline(void) {
    Zstr    s  = "\nQ";
    StrIter si = StrIterFromZstr(s);
    skip_to_eol_iter(&si);
    char c;
    bool ok = StrIterPeek(&si, &c) && c == 'Q' && StrIterIndex(&si) == 1;
    return ok;
}

// ---------------------------------------------------------------------------
// parse_hosts_table -- crafted hosts content through the real parser.
// ---------------------------------------------------------------------------

// Single simple entry resolves to the exact v4 IP.
bool test_sd1_hosts_single_v4(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);

    const char *body = "127.0.0.1 localhost\n";
    stage_hosts(body, ZstrLen((Zstr)body));
    HostsTable t = parse_staged(a);

    bool ok = VecLen(&t) == 1 && is_v4(find_host(&t, "localhost"), 127, 0, 0, 1);

    VecForeachPtr(&t, e) StrDeinit(&e->name);
    VecDeinit(&t);
    unstage_hosts();
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// Multiple aliases on one line: name + every alias resolves to the same
// IP. Kills the alias loop (202/204/208/211/212) and the per-alias
// VecPushBack / MemCopy (218/224/230) -- each must produce a distinct
// entry. 5 entries from one line.
bool test_sd1_hosts_aliases(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);

    const char *body = "192.168.1.5 host.example alias1 alias2 alias3\n";
    stage_hosts(body, ZstrLen((Zstr)body));
    HostsTable t = parse_staged(a);

    bool ok = VecLen(&t) == 4 && is_v4(find_host(&t, "host.example"), 192, 168, 1, 5) &&
              is_v4(find_host(&t, "alias1"), 192, 168, 1, 5) && is_v4(find_host(&t, "alias2"), 192, 168, 1, 5) &&
              is_v4(find_host(&t, "alias3"), 192, 168, 1, 5);

    VecForeachPtr(&t, e) StrDeinit(&e->name);
    VecDeinit(&t);
    unstage_hosts();
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// Comment lines and blank lines produce no entries; a real entry between
// them still parses. Kills the comment/blank detection (167) and the
// skip_to_eol calls (164/168/197/236).
bool test_sd1_hosts_comments_and_blanks(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);

    const char *body =
        "# a comment line\n"
        "\n"
        "   \n"
        "10.0.0.1 a\n"
        "# 1.2.3.4 commented.host\n";
    stage_hosts(body, ZstrLen((Zstr)body));
    HostsTable t = parse_staged(a);

    bool ok = VecLen(&t) == 1 && is_v4(find_host(&t, "a"), 10, 0, 0, 1) &&
              find_host(&t, "commented.host") == (const HostsEntry *)0;

    VecForeachPtr(&t, e) StrDeinit(&e->name);
    VecDeinit(&t);
    unstage_hosts();
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// A trailing in-line `# comment` after a valid entry is dropped; the
// entry parses, the comment token does not. Kills the name-loop '#'
// checks (204/208) and the leading-whitespace/break logic.
bool test_sd1_hosts_inline_comment(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);

    const char *body = "10.0.0.2 namex # trailing comment words\n";
    stage_hosts(body, ZstrLen((Zstr)body));
    HostsTable t = parse_staged(a);

    bool ok = VecLen(&t) == 1 && is_v4(find_host(&t, "namex"), 10, 0, 0, 2) &&
              find_host(&t, "trailing") == (const HostsEntry *)0 && find_host(&t, "#") == (const HostsEntry *)0;

    VecForeachPtr(&t, e) StrDeinit(&e->name);
    VecDeinit(&t);
    unstage_hosts();
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// Leading whitespace (spaces + tabs) before the IP is skipped; tabs and
// spaces both separate fields. Kills skip_hspace usage (164/203) and the
// is_hspace-driven token scans (174/208).
bool test_sd1_hosts_leading_and_tab_whitespace(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);

    const char *body = "  \t 172.16.0.9\thostT\talT\n";
    stage_hosts(body, ZstrLen((Zstr)body));
    HostsTable t = parse_staged(a);

    bool ok =
        VecLen(&t) == 2 && is_v4(find_host(&t, "hostt"), 172, 16, 0, 9) && is_v4(find_host(&t, "alt"), 172, 16, 0, 9);

    VecForeachPtr(&t, e) StrDeinit(&e->name);
    VecDeinit(&t);
    unstage_hosts();
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// Case folding: the stored name is lowercased (218 ascii_lower), so an
// uppercase source name is found via its lowercase key and NOT via the
// original casing.
bool test_sd1_hosts_case_lowered(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);

    const char *body = "10.1.1.1 MixedCaseHost\n";
    stage_hosts(body, ZstrLen((Zstr)body));
    HostsTable t = parse_staged(a);

    bool ok = VecLen(&t) == 1 && is_v4(find_host(&t, "mixedcasehost"), 10, 1, 1, 1) &&
              find_host(&t, "MixedCaseHost") == (const HostsEntry *)0;

    VecForeachPtr(&t, e) StrDeinit(&e->name);
    VecDeinit(&t);
    unstage_hosts();
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// Last line WITHOUT a trailing newline (EOF) still parses. Kills the
// outer-loop bound and the EOF handling in the name scan (174/202/208
// `c != '\n'`) and skip_to_eol's EOF path.
bool test_sd1_hosts_no_trailing_newline(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);

    const char *body = "10.0.0.1 a\n8.8.8.8 dns"; // no '\n' at EOF
    stage_hosts(body, ZstrLen((Zstr)body));
    HostsTable t = parse_staged(a);

    bool ok = VecLen(&t) == 2 && is_v4(find_host(&t, "a"), 10, 0, 0, 1) && is_v4(find_host(&t, "dns"), 8, 8, 8, 8);

    VecForeachPtr(&t, e) StrDeinit(&e->name);
    VecDeinit(&t);
    unstage_hosts();
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// Multi-line file: every line's entry resolves to ITS OWN ip. A skip
// desync (skip_to_eol / line-iteration mutant) would merge or drop a
// line and corrupt line 2/3. Kills 163 (outer loop), 236 (trailing
// skip), and the exact entry count.
bool test_sd1_hosts_multiline_no_desync(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);

    const char *body =
        "1.1.1.1 one\n"
        "2.2.2.2 two\n"
        "3.3.3.3 three\n";
    stage_hosts(body, ZstrLen((Zstr)body));
    HostsTable t = parse_staged(a);

    bool ok = VecLen(&t) == 3 && is_v4(find_host(&t, "one"), 1, 1, 1, 1) && is_v4(find_host(&t, "two"), 2, 2, 2, 2) &&
              is_v4(find_host(&t, "three"), 3, 3, 3, 3);

    VecForeachPtr(&t, e) StrDeinit(&e->name);
    VecDeinit(&t);
    unstage_hosts();
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// IPv6 entry parses to the v6 family and the right address bytes. Kills
// the v6 branch: 191 (parse_ipv6 call), 224 (MemCopy v6), is_ipv6 flag.
bool test_sd1_hosts_ipv6(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);

    const char *body = "::1 ip6-localhost\n";
    stage_hosts(body, ZstrLen((Zstr)body));
    HostsTable t = parse_staged(a);

    const HostsEntry *e  = find_host(&t, "ip6-localhost");
    bool              ok = VecLen(&t) == 1 && e && e->is_ipv6;
    if (ok) {
        // ::1 == 15 zero bytes then 0x01.
        for (int i = 0; i < 15; ++i) {
            if (e->ip[i] != 0) {
                ok = false;
            }
        }
        if (e->ip[15] != 1) {
            ok = false;
        }
    }

    VecForeachPtr(&t, e2) StrDeinit(&e2->name);
    VecDeinit(&t);
    unstage_hosts();
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// A line whose first token is NOT a valid IP is rejected wholesale --
// no entry, even though it has name-shaped tokens. Kills 196 (!got_v4 &&
// !got_v6 -> skip) and 183 (ip_len bounds) and 189/191 (parse calls).
bool test_sd1_hosts_non_ip_line_rejected(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);

    const char *body =
        "notanip somename morestuff\n"
        "10.0.0.7 realname\n";
    stage_hosts(body, ZstrLen((Zstr)body));
    HostsTable t = parse_staged(a);

    bool ok = VecLen(&t) == 1 && is_v4(find_host(&t, "realname"), 10, 0, 0, 7) &&
              find_host(&t, "somename") == (const HostsEntry *)0;

    VecForeachPtr(&t, e) StrDeinit(&e->name);
    VecDeinit(&t);
    unstage_hosts();
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// An IP-only line (no hostname) yields no entries -- the name loop must
// find zero tokens. Kills the nm_len==0 break (212) and the inner
// peek/advance logic.
bool test_sd1_hosts_ip_only_no_name(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);

    const char *body =
        "10.0.0.8\n"
        "10.0.0.9 named\n";
    stage_hosts(body, ZstrLen((Zstr)body));
    HostsTable t = parse_staged(a);

    bool ok = VecLen(&t) == 1 && is_v4(find_host(&t, "named"), 10, 0, 0, 9);

    VecForeachPtr(&t, e) StrDeinit(&e->name);
    VecDeinit(&t);
    unstage_hosts();
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// slurp_file failure inside parse_hosts_table: when the open succeeds but
// the read errors, parse leaves the table empty (156 return-value mutant:
// if slurp_file's result is forced truthy the early-return is skipped).
// We exercise the success-with-content path elsewhere; here we confirm a
// path that does NOT exist via the redirect yields an empty table (the
// open fails -> slurp returns true -> empty buf -> zero entries).
bool test_sd1_hosts_missing_yields_empty(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);

    g_redirect_to = "/tmp/sd1_definitely_missing_zzqq2";
    HostsTable t  = parse_staged(a);
    bool       ok = VecLen(&t) == 0;

    VecForeachPtr(&t, e) StrDeinit(&e->name);
    VecDeinit(&t);
    unstage_hosts();
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// A bare IP as the WHOLE last line with no trailing newline: the IP
// token scan must terminate on EOF via `StrIterPeek` returning false
// (174:41). If that peek is forced truthy the scan walks past the buffer
// (StrIterMustNext aborts) -- so a clean parse here pins the peek. The
// line yields no entry (no name), and a following real line still parses.
bool test_sd1_hosts_bare_ip_eof(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);

    // First line bare IP + newline, last line a bare IP token that runs
    // straight into EOF (no name, no newline).
    const char *body = "10.0.0.3 keep\n9.9.9.9";
    stage_hosts(body, ZstrLen((Zstr)body));
    HostsTable t = parse_staged(a);

    bool ok = VecLen(&t) == 1 && is_v4(find_host(&t, "keep"), 10, 0, 0, 3);

    VecForeachPtr(&t, e) StrDeinit(&e->name);
    VecDeinit(&t);
    unstage_hosts();
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// A first token longer than the 64-char IP cap: the `ip_len < 64` guard
// (183) makes the parser SKIP the parse_ip block entirely, so got_v4 /
// got_v6 keep their `false` initialisers (179/180). If either initialiser
// is flipped to true, this over-long line is wrongly accepted and its
// trailing token becomes a spurious entry. A real entry after it confirms
// the parser recovers.
bool test_sd1_hosts_overlong_first_token(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);

    // 70-char first token (>= 64) followed by a name, then a real entry.
    char line[160];
    u64  k = 0;
    for (; k < 70; ++k) {
        line[k] = 'x';
    }
    const char *tail = " spuriousname\n10.0.0.4 goodname\n";
    for (u64 i = 0; tail[i] != '\0'; ++i) {
        line[k++] = tail[i];
    }
    line[k] = '\0';
    stage_hosts(line, k);
    HostsTable t = parse_staged(a);

    bool ok = VecLen(&t) == 1 && is_v4(find_host(&t, "goodname"), 10, 0, 0, 4) &&
              find_host(&t, "spuriousname") == (const HostsEntry *)0;

    VecForeachPtr(&t, e) StrDeinit(&e->name);
    VecDeinit(&t);
    unstage_hosts();
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// A rejected first token FOLLOWED on the same line by a valid IP token:
// the reject branch's `skip_to_eol_iter` (197) must discard the rest of
// the line. If that skip is removed, the parser re-scans "10.0.0.5" as a
// fresh IP and emits a spurious "x" entry. The real entry on line 2
// confirms recovery.
bool test_sd1_hosts_reject_skips_rest_of_line(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);

    const char *body =
        "bad 10.0.0.5 x\n"
        "10.0.0.6 realone\n";
    stage_hosts(body, ZstrLen((Zstr)body));
    HostsTable t = parse_staged(a);

    bool ok =
        VecLen(&t) == 1 && is_v4(find_host(&t, "realone"), 10, 0, 0, 6) && find_host(&t, "x") == (const HostsEntry *)0;

    VecForeachPtr(&t, e) StrDeinit(&e->name);
    VecDeinit(&t);
    unstage_hosts();
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// ---------------------------------------------------------------------------

int main(void) {
    WriteFmt("[INFO] Starting SysDns.Mutants1 tests\n\n");

    TestFunction tests[] = {
        test_sd1_slurp_reads_full_content,
        test_sd1_slurp_multi_chunk,
        test_sd1_slurp_empty_file,
        test_sd1_slurp_missing_file,
        test_sd1_slurp_read_error_returns_false,
        test_sd1_hosts_slurp_failure_empty_table,
        test_sd1_skip_eol_advances_past_newline,
        test_sd1_skip_eol_last_line_no_newline,
        test_sd1_skip_eol_on_newline,
        test_sd1_hosts_single_v4,
        test_sd1_hosts_aliases,
        test_sd1_hosts_comments_and_blanks,
        test_sd1_hosts_inline_comment,
        test_sd1_hosts_leading_and_tab_whitespace,
        test_sd1_hosts_case_lowered,
        test_sd1_hosts_no_trailing_newline,
        test_sd1_hosts_multiline_no_desync,
        test_sd1_hosts_ipv6,
        test_sd1_hosts_non_ip_line_rejected,
        test_sd1_hosts_ip_only_no_name,
        test_sd1_hosts_missing_yields_empty,
        test_sd1_hosts_bare_ip_eof,
        test_sd1_hosts_overlong_first_token,
        test_sd1_hosts_reject_skips_rest_of_line,
    };

    return run_test_suite(tests, sizeof(tests) / sizeof(tests[0]), NULL, 0, "SysDns.Hosts");
}
