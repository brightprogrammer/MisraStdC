/// file      : Tests/Std/SysDns.Mutants2.c
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Mutation-hardening for the /etc/resolv.conf parsing + hostname
/// normalization path of `Sys/Dns.c`: `parse_resolv_conf`,
/// `normalize_hostname`, `ascii_lower`.
///
/// All three are `static`, so we pull the source unit in directly to
/// reach them. `ascii_lower` and `normalize_hostname` take crafted
/// buffers / Zstrs and are asserted on EXACT outputs.
///
/// `parse_resolv_conf` hard-codes `RESOLV_CONF_FILE_PATH`
/// (`/etc/resolv.conf`), which is neither writable nor deterministic in
/// a test sandbox. To feed it crafted config text without touching the
/// source, we interpose the `file_open` function that
/// `FileOpen(RESOLV_CONF_FILE_PATH, ...)` lowers to: `File.h` is
/// included first (and guarded), then `file_open` is redefined as a
/// function-like macro routing to `sd2_file_open`. When the resolver
/// asks for the resolv.conf path, the interposer materialises the
/// crafted bytes into a private temp file and hands back a real read
/// handle; every other path passes straight through to the genuine
/// `file_open` (reached via the parenthesised `(file_open)(...)` form,
/// which suppresses the macro). The library object is a separate TU, so
/// its own `file_open` is untouched.

#include <Misra.h>
#include <Misra/Std/Allocator/Default.h>
#include <Misra/Std/File.h>
#include <Misra/Std/Zstr.h>
#include <Misra/Sys/Dns.h>
#include <Misra/Sys/Socket.h>
#include <Misra/Std/Utility/StrIter.h>

#include "../Util/TestRunner.h"

// ---------------------------------------------------------------------------
// file_open interposition. Set `sd2_rc_content` to the bytes the next
// parse_resolv_conf call should see; sd2_file_open writes them to a temp
// file and returns a read handle for the resolv.conf path only.
// ---------------------------------------------------------------------------

static const char *sd2_rc_content = NULL;
static u64         sd2_rc_len     = 0;

#define SD2_RC_TMP "/tmp/sd2_resolv.conf"

static File sd2_file_open(Zstr path, Zstr mode) {
    // Only intercept the resolv.conf path; everything else (the
    // /etc/hosts read at init time, etc.) goes to the real opener.
    bool is_rc = path && ZstrCompare(path, "/etc/resolv.conf") == 0;
    if (!is_rc) {
        return (file_open)(path, mode);
    }
    // Materialise the crafted bytes into a private temp file, then open
    // it for reading just like the real resolv.conf path would be.
    File w = (file_open)(SD2_RC_TMP, "wb");
    if (FileIsOpen(&w)) {
        if (sd2_rc_content && sd2_rc_len > 0) {
            FileWrite(&w, sd2_rc_content, sd2_rc_len);
        }
        FileClose(&w);
    }
    return (file_open)(SD2_RC_TMP, "rb");
}

#undef file_open
#define file_open(path, mode) sd2_file_open((path), (mode))

// Pull in the unit under test so the static helpers are callable here.
// `RESOLV_CONF_FILE_PATH` stays "/etc/resolv.conf"; the interposer above
// recognises it and serves the crafted bytes.
#include "../../Source/Misra/Sys/Dns.c"

#undef file_open

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

// Run parse_resolv_conf against crafted config text; caller owns `out`.
static void sd2_parse(const char *content, DnsAddrs *out, Allocator *a) {
    sd2_rc_content = content;
    sd2_rc_len     = ZstrLen(content);
    parse_resolv_conf(out, a);
    sd2_rc_content = NULL;
    sd2_rc_len     = 0;
}

// Format nameserver `i` as a Zstr-comparable string ("ip:port"); writes
// into `buf` (caller-owned Str). Returns true on success.
static bool sd2_ns_str(DnsAddrs *out, u64 i, Str *buf, Allocator *a) {
    if (VecLen(out) <= i) {
        return false;
    }
    Str s = SocketAddrFormat(VecPtrAt(out, i), a);
    StrMergeR(buf, &s);
    StrDeinit(&s);
    return StrLen(buf) > 0;
}

// ---------------------------------------------------------------------------
// ascii_lower -- one ASCII char lowercased; exact mapping + boundaries.
// Line 88-95: for (i<n) { c=p[i]; if (c>='A' && c<='Z') p[i]=c+('a'-'A'); }
// ---------------------------------------------------------------------------

// Run ascii_lower over a single mutable byte and return the result.
static u8 sd2_lower1(u8 c) {
    u8 b = c;
    ascii_lower(&b, 1);
    return b;
}

// 'A' -> 'a': lower bound of the uppercase range, must be transformed.
static bool test_sd2_lower_A(void) {
    return sd2_lower1('A') == 'a';
}

// 'Z' -> 'z': upper bound of the range, must be transformed.
static bool test_sd2_lower_Z(void) {
    return sd2_lower1('Z') == 'z';
}

// 'M' -> 'm': mid-range, pins the +('a'-'A') arithmetic (not a swap).
static bool test_sd2_lower_M(void) {
    return sd2_lower1('M') == 'm';
}

// 'a' -> 'a': already lowercase, untouched.
static bool test_sd2_lower_a_noop(void) {
    return sd2_lower1('a') == 'a';
}

// '@' is 'A'-1: just below the range, must be left unchanged. Guards the
// `c >= 'A'` lower bound (ge->gt would still leave it, ge->lt would lower
// everything; this plus 'A' pins the bound).
static bool test_sd2_lower_at_unchanged(void) {
    return sd2_lower1('@') == '@';
}

// '[' is 'Z'+1: just above the range, must be left unchanged. Guards the
// `c <= 'Z'` upper bound.
static bool test_sd2_lower_bracket_unchanged(void) {
    return sd2_lower1('[') == '[';
}

// '`' is 'a'-1: a non-uppercase char just below 'a', unchanged.
static bool test_sd2_lower_backtick_unchanged(void) {
    return sd2_lower1('`') == '`';
}

// '0' (digit) unchanged.
static bool test_sd2_lower_digit_unchanged(void) {
    return sd2_lower1('0') == '0';
}

// Multi-byte run: lowercases each uppercase, preserves the rest, and the
// loop visits every index (guards i<n bound and ++i). "Ab9Z" -> "ab9z".
static bool test_sd2_lower_run(void) {
    u8 buf[5] = {'A', 'b', '9', 'Z', 0};
    ascii_lower(buf, 4);
    return buf[0] == 'a' && buf[1] == 'b' && buf[2] == '9' && buf[3] == 'z' && buf[4] == 0;
}

// n == 0: nothing touched (guards the `i < n` start / loop entry).
static bool test_sd2_lower_zero_len(void) {
    u8 buf[2] = {'A', 'B'};
    ascii_lower(buf, 0);
    return buf[0] == 'A' && buf[1] == 'B';
}

// The last byte must be reached: with n==2 over "xZ", the 'Z' at index 1
// is lowered. Guards i<n vs i<=n / i!=n style mutations on the bound.
static bool test_sd2_lower_last_index(void) {
    u8 buf[2] = {'x', 'Z'};
    ascii_lower(buf, 2);
    return buf[0] == 'x' && buf[1] == 'z';
}

// One-past-the-end must NOT be touched: with n==1 over a 2-byte buffer
// whose second byte is uppercase 'X', `i < n` lowers only index 0 and
// leaves index 1 alone. An `i <= n` mutation would lower 'X' -> 'x'.
static bool test_sd2_lower_no_overrun(void) {
    u8 buf[2] = {'A', 'X'};
    ascii_lower(buf, 1);
    return buf[0] == 'a' && buf[1] == 'X';
}

// ---------------------------------------------------------------------------
// normalize_hostname -- trailing-dot strip + lowercase into `out`.
// Line 353-368: strip trailing '.', then lowercase A..Z, push each char.
// ---------------------------------------------------------------------------

// Normalize and compare against `expect`; returns true on exact match.
static bool sd2_norm_eq(Zstr in, Zstr expect, Allocator *a) {
    Str out = StrInit(a);
    normalize_hostname(in, &out);
    bool ok = ZstrCompare(StrBegin(&out), expect) == 0;
    StrDeinit(&out);
    return ok;
}

// Mixed case fully lowercased, exact string.
static bool test_sd2_norm_mixed_case(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    bool             ok    = sd2_norm_eq("WWW.Example.COM", "www.example.com", ALLOCATOR_OF(&alloc));
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// Single trailing dot stripped.
static bool test_sd2_norm_trailing_dot(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    bool             ok    = sd2_norm_eq("Host.", "host", ALLOCATOR_OF(&alloc));
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// Multiple trailing dots all stripped (guards the `while len>0 &&
// name[len-1]=='.'` loop and the --len decrement).
static bool test_sd2_norm_multi_trailing_dot(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    bool             ok    = sd2_norm_eq("foo...", "foo", ALLOCATOR_OF(&alloc));
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// Interior dots are preserved; only trailing ones stripped.
static bool test_sd2_norm_interior_dots_kept(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    bool             ok    = sd2_norm_eq("a.b.c.", "a.b.c", ALLOCATOR_OF(&alloc));
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// Digits and hyphens preserved verbatim alongside lowercasing.
static bool test_sd2_norm_digits_hyphens(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    bool             ok    = sd2_norm_eq("Host-01.Net-2", "host-01.net-2", ALLOCATOR_OF(&alloc));
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// Boundary char '@' ('A'-1) inside the body is NOT lowered (guards the
// `c >= 'A'` lower bound in the normalize loop, distinct from ascii_lower).
static bool test_sd2_norm_at_kept(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    bool             ok    = sd2_norm_eq("a@Z", "a@z", ALLOCATOR_OF(&alloc));
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// Boundary char '[' ('Z'+1) is NOT lowered (guards the `c <= 'Z'` upper
// bound; le->lt would drop 'Z' itself, so pair with the 'Z' below).
static bool test_sd2_norm_bracket_kept(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    bool             ok    = sd2_norm_eq("A[Z", "a[z", ALLOCATOR_OF(&alloc));
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// 'Z' itself must be lowered (guards `c <= 'Z'` upper bound: le->lt drops Z).
static bool test_sd2_norm_Z_lowered(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    bool             ok    = sd2_norm_eq("Z", "z", ALLOCATOR_OF(&alloc));
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// A name with no trailing dot is preserved length-exact (the while loop
// must NOT decrement when name[len-1] != '.').
static bool test_sd2_norm_no_dot_len(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Str              out   = StrInit(ALLOCATOR_OF(&alloc));
    normalize_hostname("abc", &out);
    bool ok = StrLen(&out) == 3 && ZstrCompare(StrBegin(&out), "abc") == 0;
    StrDeinit(&out);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// A name that is ONLY dots collapses to empty (len decremented to 0; the
// `len > 0` guard stops the while loop, no underflow).
static bool test_sd2_norm_all_dots_empty(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Str              out   = StrInit(ALLOCATOR_OF(&alloc));
    normalize_hostname("...", &out);
    bool ok = StrLen(&out) == 0;
    StrDeinit(&out);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// NULL name -> early return, out stays empty (guards the `!name` guard).
static bool test_sd2_norm_null(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Str              out   = StrInit(ALLOCATOR_OF(&alloc));
    normalize_hostname(NULL, &out);
    bool ok = StrLen(&out) == 0;
    StrDeinit(&out);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// ---------------------------------------------------------------------------
// parse_resolv_conf -- nameserver / comment / cap handling.
// Asserts EXACT parsed nameserver IPs (via SocketAddrFormat), order, and
// count. Comments (#/;) and non-nameserver lines (search/domain) ignored.
// ---------------------------------------------------------------------------

// Two nameservers parsed in order, exact IPs, search/comment lines ignored.
static bool test_sd2_resolv_basic(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);
    DnsAddrs         out   = VecInitT(out, a);

    sd2_parse(
        "nameserver 8.8.8.8\n"
        "nameserver 1.1.1.1\n"
        "search foo.com bar.com\n"
        "# comment\n"
        "; comment2\n"
        "domain x.com\n",
        &out,
        a
    );

    bool ok = VecLen(&out) == 2;
    if (ok) {
        Str s0 = StrInit(a);
        Str s1 = StrInit(a);
        ok     = sd2_ns_str(&out, 0, &s0, a) && ZstrCompare(StrBegin(&s0), "8.8.8.8:53") == 0;
        ok     = ok && sd2_ns_str(&out, 1, &s1, a) && ZstrCompare(StrBegin(&s1), "1.1.1.1:53") == 0;
        StrDeinit(&s0);
        StrDeinit(&s1);
    }

    VecDeinit(&out);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// IPv6 nameserver parsed; family is INET6 and IP round-trips.
static bool test_sd2_resolv_v6(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);
    DnsAddrs         out   = VecInitT(out, a);

    sd2_parse("nameserver ::1\n", &out, a);

    bool ok = VecLen(&out) == 1 && VecPtrAt(&out, 0)->family == SOCKET_FAMILY_INET6;
    if (ok) {
        Str s = StrInit(a);
        ok    = sd2_ns_str(&out, 0, &s, a) && ZstrCompare(StrBegin(&s), "[::1]:53") == 0;
        StrDeinit(&s);
    }

    VecDeinit(&out);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// A '#'-comment line must be skipped entirely: a nameserver that is
// commented out yields zero entries (guards `c == '#'` skip at line 264).
static bool test_sd2_resolv_hash_comment_skipped(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);
    DnsAddrs         out   = VecInitT(out, a);

    sd2_parse("# nameserver 9.9.9.9\n", &out, a);

    bool ok = VecLen(&out) == 0;
    VecDeinit(&out);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// A ';'-comment line must be skipped (guards `c == ';'` at line 264).
static bool test_sd2_resolv_semicolon_comment_skipped(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);
    DnsAddrs         out   = VecInitT(out, a);

    sd2_parse("; nameserver 9.9.9.9\n", &out, a);

    bool ok = VecLen(&out) == 0;
    VecDeinit(&out);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// A non-nameserver keyword ("search") must NOT be treated as a
// nameserver. Guards the MemCompare keyword match at line 274.
static bool test_sd2_resolv_search_not_ns(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);
    DnsAddrs         out   = VecInitT(out, a);

    sd2_parse("search 8.8.8.8\n", &out, a);

    bool ok = VecLen(&out) == 0;
    VecDeinit(&out);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// A keyword that shares a prefix but differs ("nameserverX 8.8.8.8") must
// be rejected: the separator after "nameserver" is 'X', not space/tab, so
// the `sep == ' ' || sep == '\t'` check at line 275 fails. Guards that the
// separator must be whitespace.
static bool test_sd2_resolv_keyword_needs_sep(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);
    DnsAddrs         out   = VecInitT(out, a);

    sd2_parse("nameserverx 8.8.8.8\n", &out, a);

    bool ok = VecLen(&out) == 0;
    VecDeinit(&out);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// Tab separator after "nameserver" is accepted (guards `sep == '\t'`).
static bool test_sd2_resolv_tab_sep(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);
    DnsAddrs         out   = VecInitT(out, a);

    sd2_parse("nameserver\t8.8.4.4\n", &out, a);

    bool ok = VecLen(&out) == 1;
    if (ok) {
        Str s = StrInit(a);
        ok    = sd2_ns_str(&out, 0, &s, a) && ZstrCompare(StrBegin(&s), "8.8.4.4:53") == 0;
        StrDeinit(&s);
    }
    VecDeinit(&out);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// Leading whitespace before "nameserver" is skipped (guards
// skip_hspace_iter at line 262 + the keyword window starting at the
// cursor, not index 0).
static bool test_sd2_resolv_leading_space(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);
    DnsAddrs         out   = VecInitT(out, a);

    sd2_parse("  nameserver 8.8.8.8\n", &out, a);

    bool ok = VecLen(&out) == 1;
    if (ok) {
        Str s = StrInit(a);
        ok    = sd2_ns_str(&out, 0, &s, a) && ZstrCompare(StrBegin(&s), "8.8.8.8:53") == 0;
        StrDeinit(&s);
    }
    VecDeinit(&out);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// An invalid IP after "nameserver" yields no entry (parse_ipv4/parse_ipv6
// both fail). Guards that a bogus address is not pushed.
static bool test_sd2_resolv_bad_ip(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);
    DnsAddrs         out   = VecInitT(out, a);

    sd2_parse("nameserver not.an.ip.999\n", &out, a);

    bool ok = VecLen(&out) == 0;
    VecDeinit(&out);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// A trailing '#'-comment after the IP on the same line is ignored; the IP
// still parses (guards `c != '#'` stop in the IP-token loop at line 280).
static bool test_sd2_resolv_trailing_comment(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);
    DnsAddrs         out   = VecInitT(out, a);

    sd2_parse("nameserver 8.8.8.8#note\n", &out, a);

    bool ok = VecLen(&out) == 1;
    if (ok) {
        Str s = StrInit(a);
        ok    = sd2_ns_str(&out, 0, &s, a) && ZstrCompare(StrBegin(&s), "8.8.8.8:53") == 0;
        StrDeinit(&s);
    }
    VecDeinit(&out);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// A "nameserver" line with no IP after it yields no entry (ip_len == 0,
// guards `ip_len > 0` at line 284).
static bool test_sd2_resolv_empty_ip(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);
    DnsAddrs         out   = VecInitT(out, a);

    sd2_parse("nameserver \n", &out, a);

    bool ok = VecLen(&out) == 0;
    VecDeinit(&out);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// Order is preserved across three nameservers: index 0/1/2 map to the
// exact lines (guards push order / per-line loop).
static bool test_sd2_resolv_order(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);
    DnsAddrs         out   = VecInitT(out, a);

    sd2_parse(
        "nameserver 10.0.0.1\n"
        "nameserver 10.0.0.2\n"
        "nameserver 10.0.0.3\n",
        &out,
        a
    );

    bool ok = VecLen(&out) == 3;
    if (ok) {
        Str s0 = StrInit(a), s1 = StrInit(a), s2 = StrInit(a);
        ok = sd2_ns_str(&out, 0, &s0, a) && ZstrCompare(StrBegin(&s0), "10.0.0.1:53") == 0;
        ok = ok && sd2_ns_str(&out, 1, &s1, a) && ZstrCompare(StrBegin(&s1), "10.0.0.2:53") == 0;
        ok = ok && sd2_ns_str(&out, 2, &s2, a) && ZstrCompare(StrBegin(&s2), "10.0.0.3:53") == 0;
        StrDeinit(&s0);
        StrDeinit(&s1);
        StrDeinit(&s2);
    }
    VecDeinit(&out);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// The last line of a file with no trailing newline still parses (guards
// the per-line loop / EOL handling on the final line).
static bool test_sd2_resolv_no_trailing_newline(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);
    DnsAddrs         out   = VecInitT(out, a);

    sd2_parse("nameserver 8.8.8.8", &out, a);

    bool ok = VecLen(&out) == 1;
    if (ok) {
        Str s = StrInit(a);
        ok    = sd2_ns_str(&out, 0, &s, a) && ZstrCompare(StrBegin(&s), "8.8.8.8:53") == 0;
        StrDeinit(&s);
    }
    VecDeinit(&out);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

int main(void) {
    WriteFmt("[INFO] Starting SysDns.Mutants2 tests\n\n");

    TestFunction tests[] = {
        // ascii_lower
        test_sd2_lower_A,
        test_sd2_lower_Z,
        test_sd2_lower_M,
        test_sd2_lower_a_noop,
        test_sd2_lower_at_unchanged,
        test_sd2_lower_bracket_unchanged,
        test_sd2_lower_backtick_unchanged,
        test_sd2_lower_digit_unchanged,
        test_sd2_lower_run,
        test_sd2_lower_zero_len,
        test_sd2_lower_last_index,
        test_sd2_lower_no_overrun,

        // normalize_hostname
        test_sd2_norm_mixed_case,
        test_sd2_norm_trailing_dot,
        test_sd2_norm_multi_trailing_dot,
        test_sd2_norm_interior_dots_kept,
        test_sd2_norm_digits_hyphens,
        test_sd2_norm_at_kept,
        test_sd2_norm_bracket_kept,
        test_sd2_norm_Z_lowered,
        test_sd2_norm_no_dot_len,
        test_sd2_norm_all_dots_empty,
        test_sd2_norm_null,

        // parse_resolv_conf
        test_sd2_resolv_basic,
        test_sd2_resolv_v6,
        test_sd2_resolv_hash_comment_skipped,
        test_sd2_resolv_semicolon_comment_skipped,
        test_sd2_resolv_search_not_ns,
        test_sd2_resolv_keyword_needs_sep,
        test_sd2_resolv_tab_sep,
        test_sd2_resolv_leading_space,
        test_sd2_resolv_bad_ip,
        test_sd2_resolv_trailing_comment,
        test_sd2_resolv_empty_ip,
        test_sd2_resolv_order,
        test_sd2_resolv_no_trailing_newline,
    };

    return run_test_suite(tests, sizeof(tests) / sizeof(tests[0]), NULL, 0, "SysDns.Resolv");
}
