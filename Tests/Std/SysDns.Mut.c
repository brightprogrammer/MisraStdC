/// file      : Tests/Std/SysDns.Mut.c
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Mutation-hardening "gap" suite for `Sys/Dns.c`: kills the reachable
/// survivors a fresh re-mull surfaced that the four existing SysDns.*
/// suites (Hosts / Resolv / Wire / Api) leave alive. The reachable ones
/// are all in the hosts / resolv config-PARSING logic (drivable through
/// the same `file_open` shim those suites use, redirecting the hardcoded
/// `/etc/hosts` and `/etc/resolv.conf` paths to crafted temp files) plus
/// the resolve-API offset/length math. The network send/recv survivors
/// (udp_round_trip and the SocketRecv/DnsParseResponse fan-out inside
/// `try_one_query`) are syscall-dependent and ledgered B -- see the
/// per-id notes in the task return + Conventions/mull-ignores.toml.
///
/// The shim is the documented idiom (mirrors SysDns.Hosts.c /
/// SysDns.Resolv.c): include `File.h` first, redefine `file_open` to a
/// function-like macro routing to a local interposer, then `#include`
/// the source unit so its `static` helpers become local symbols here
/// and its lone `FileOpen` calls reach our interposer. The library's own
/// Dns object is a separate TU, untouched.

#include <Misra.h>
#include <Misra/Std/Allocator/Debug.h>
#include <Misra/Std/Allocator/Default.h>
#include <Misra/Std/Container/Str/Init.h>
#include <Misra/Std/File.h>
#include <Misra/Std/Utility/StrIter.h>
#include <Misra/Std/Zstr.h>
#include <Misra/Sys/Dns.h>
#include <Misra/Sys/Socket.h>

#include "../Util/TestRunner.h"

// ---------------------------------------------------------------------------
// file_open interposition. The next parse call sees `g_content` bytes for
// whichever of the two config paths matches `g_which`; the interposer
// stages them into a private temp file and hands back a real read handle.
// Every other path falls through to the genuine opener.
// ---------------------------------------------------------------------------

static const char *g_content = NULL; // bytes the next parse should see
static u64         g_len     = 0;
static const char *g_which   = NULL; // "/etc/hosts" or "/etc/resolv.conf"

#define SDM_TMP "/tmp/sdm_cfg.tmp"

static File sdm_file_open(Zstr path, Zstr mode) {
    bool match = g_which && path && ZstrCompare(path, g_which) == 0;
    if (!match) {
        return (file_open)(path, mode);
    }
    File w = (file_open)(SDM_TMP, "wb");
    if (FileIsOpen(&w)) {
        if (g_content && g_len > 0) {
            FileWrite(&w, g_content, g_len);
        }
        FileClose(&w);
    }
    return (file_open)(SDM_TMP, "rb");
}

#undef file_open
#define file_open(path, mode) sdm_file_open((path), (mode))

#include "../../Source/Misra/Sys/Dns.c"

#undef file_open

// ---------------------------------------------------------------------------
// Fixtures
// ---------------------------------------------------------------------------

// Parse crafted /etc/hosts text into a fresh table.
static HostsTable parse_hosts_crafted(const char *content, Allocator *a) {
    g_which      = "/etc/hosts";
    g_content    = content;
    g_len        = ZstrLen(content);
    HostsTable t = VecInitT(t, a);
    parse_hosts_table(&t, a);
    g_which   = NULL;
    g_content = NULL;
    g_len     = 0;
    return t;
}

// Parse crafted /etc/resolv.conf text into a caller-owned DnsAddrs.
static void parse_resolv_crafted(const char *content, DnsAddrs *out, Allocator *a) {
    g_which   = "/etc/resolv.conf";
    g_content = content;
    g_len     = ZstrLen(content);
    parse_resolv_conf(out, a);
    g_which   = NULL;
    g_content = NULL;
    g_len     = 0;
}

static const HostsEntry *find_host(const HostsTable *t, const char *name) {
    VecForeachPtr(t, e) {
        if (ZstrCompare(StrBegin(&e->name), name) == 0) {
            return e;
        }
    }
    return (const HostsEntry *)0;
}

static void free_hosts(HostsTable *t) {
    VecForeachPtr(t, e) StrDeinit(&e->name);
    VecDeinit(t);
}

// ---------------------------------------------------------------------------
// KILL 224:27 -- `e.is_ipv6 = true` in the /etc/hosts v6 branch
// (cxx_assign_const flips it to false). A v6 row carries the 16 v6 bytes
// AND the is_ipv6 flag; if the flag is forced false the entry would be
// dispatched as IPv4 later. The existing SysDns.Hosts v6 test lives in a
// separate target; this re-asserts the flag here so the surfaced survivor
// dies in THIS suite. The address bytes are checked too so a flip cannot
// hide behind a coincidental v4 reinterpretation.
// ---------------------------------------------------------------------------
static bool test_sdm_hosts_v6_flag_true(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);

    HostsTable        t = parse_hosts_crafted("fe80::1 v6host\n", a);
    const HostsEntry *e = find_host(&t, "v6host");

    bool ok = VecLen(&t) == 1 && e && e->is_ipv6 == true;
    if (ok) {
        // fe80::1 == 0xfe 0x80 then 13 zero bytes then 0x01.
        ok = e->ip[0] == 0xfe && e->ip[1] == 0x80 && e->ip[15] == 0x01;
        for (int i = 2; i < 15; ++i) {
            if (e->ip[i] != 0) {
                ok = false;
            }
        }
    }

    free_hosts(&t);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// Companion: a v4 row must keep is_ipv6 == false. Pairs with the v6 test
// so the assign-const mutant cannot be equivalent in either direction.
static bool test_sdm_hosts_v4_flag_false(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);

    HostsTable        t = parse_hosts_crafted("10.20.30.40 v4host\n", a);
    const HostsEntry *e = find_host(&t, "v4host");

    bool ok = VecLen(&t) == 1 && e && e->is_ipv6 == false && e->ip[0] == 10 && e->ip[1] == 20 && e->ip[2] == 30 &&
              e->ip[3] == 40;

    free_hosts(&t);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// ---------------------------------------------------------------------------
// KILL 303:5 -- `StrDeinit(&buf)` at the tail of `parse_resolv_conf`
// (cxx_remove_void_call). The slurped config buffer is heap-backed by the
// passed allocator; if the final teardown is dropped it leaks. We drive a
// real parse (with content, so `buf` actually allocates a backing store)
// through a DebugAllocator and assert the live count returns to baseline.
// The `out` DnsAddrs is drained/deinited first so the only thing that can
// keep the count elevated is the missing `StrDeinit(&buf)`.
// ---------------------------------------------------------------------------
static bool test_sdm_resolv_buf_no_leak(void) {
    DebugAllocator alloc    = DebugAllocatorInit();
    Allocator     *a        = ALLOCATOR_OF(&alloc);
    size           baseline = DebugAllocatorLiveCount(&alloc);

    DnsAddrs out = VecInitT(out, a);
    // Content large enough to force `buf` past any small-string inline
    // capacity so a real allocation happens (and would leak if dropped).
    parse_resolv_crafted(
        "# leading comment to bulk up the slurp buffer well past inline cap\n"
        "nameserver 8.8.8.8\n"
        "nameserver 1.1.1.1\n"
        "search example.com sub.example.com another.example.org\n"
        "options edns0 trust-ad ndots:1\n",
        &out,
        a
    );
    bool parsed = VecLen(&out) == 2;
    VecDeinit(&out);

    bool back = DebugAllocatorLiveCount(&alloc) == baseline;

    DebugAllocatorDeinit(&alloc);
    return parsed && back;
}

// Companion leak guard for parse_hosts_table's own slurp `buf`
// (`StrDeinit(&buf)` at line 239) -- not on the surfaced list but the same
// remove-void-call shape; cheap to pin and keeps the parse buffers honest.
static bool test_sdm_hosts_buf_no_leak(void) {
    DebugAllocator alloc    = DebugAllocatorInit();
    Allocator     *a        = ALLOCATOR_OF(&alloc);
    size           baseline = DebugAllocatorLiveCount(&alloc);

    HostsTable t = parse_hosts_crafted(
        "# bulk comment line to grow the slurp buffer beyond inline capacity\n"
        "127.0.0.1 localhost loopback\n"
        "10.0.0.5 host.example\n",
        a
    );
    bool parsed = VecLen(&t) == 3; // localhost, loopback, host.example
    free_hosts(&t);

    bool back = DebugAllocatorLiveCount(&alloc) == baseline;

    DebugAllocatorDeinit(&alloc);
    return parsed && back;
}

// ---------------------------------------------------------------------------
// Helpers for the resolve-API survivor (629:46).
// ---------------------------------------------------------------------------

static void resolver_init_empty(DnsResolver *r, Allocator *a) {
    r->allocator   = a;
    r->hosts       = VecInitT(r->hosts, a);
    r->nameservers = VecInitT(r->nameservers, a);
    r->timeout_ms  = 5000;
    r->retries     = 2;
}

static void hosts_push_v4(DnsResolver *r, Zstr name, u8 b0, u8 b1, u8 b2, u8 b3) {
    HostsEntry e = {0};
    e.name       = StrInitFromCstr(name, ZstrLen(name), r->allocator);
    e.ip[0]      = b0;
    e.ip[1]      = b1;
    e.ip[2]      = b2;
    e.ip[3]      = b3;
    e.is_ipv6    = false;
    VecPushBackR(&r->hosts, e);
}

static bool v4_is(const SocketAddr *ad, Allocator *a, Zstr expect) {
    if (ad->family != SOCKET_FAMILY_INET) {
        return false;
    }
    Str  s  = SocketAddrFormat(ad, a);
    bool ok = (StrLen(&s) > 0) && ZstrCompare(StrBegin(&s), expect) == 0;
    StrDeinit(&s);
    return ok;
}

// ---------------------------------------------------------------------------
// KILL 629:46 -- `have_one = ok && VecLen(&addrs) > 0` -- hit/miss contract.
//
// On a HOSTS HIT the vec form returns true with one address; the single
// address must be written back EXACTLY (take-first). On a MISS with empty
// nameservers the vec form returns false and `out` is untouched. These two
// pin the take-first behaviour and the `ok && ...` short-circuit. (The
// pure GE-vs-GT boundary at this line is bucket C: a true return from the
// vec form always carries >= 1 address, so `> 0` and `>= 0` yield the same
// have_one for every reachable input -- see the task return notes.)
// ---------------------------------------------------------------------------
static bool test_sdm_resolve_one_hit_writes_addr(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);

    DnsResolver r;
    resolver_init_empty(&r, a);
    hosts_push_v4(&r, "pick", 9, 8, 7, 6);

    SocketAddr one;
    bool       got = dns_resolve_4_one_zstr(&r, "pick:443", SOCKET_KIND_TCP, &one);
    bool       ok  = got && v4_is(&one, a, "9.8.7.6:443");

    DnsResolverDeinit(&r);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

static bool test_sdm_resolve_one_miss_false(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);

    DnsResolver r;
    resolver_init_empty(&r, a);
    hosts_push_v4(&r, "pick", 9, 8, 7, 6);

    SocketAddr one;
    bool       got = dns_resolve_4_one_zstr(&r, "absent:443", SOCKET_KIND_TCP, &one);

    DnsResolverDeinit(&r);
    DefaultAllocatorDeinit(&alloc);
    return !got;
}

// ---------------------------------------------------------------------------

int main(void) {
    WriteFmt("[INFO] Starting SysDns.Mut tests\n\n");

    TestFunction tests[] = {
        test_sdm_hosts_v6_flag_true,
        test_sdm_hosts_v4_flag_false,
        test_sdm_resolv_buf_no_leak,
        test_sdm_hosts_buf_no_leak,
        test_sdm_resolve_one_hit_writes_addr,
        test_sdm_resolve_one_miss_false,
    };

    return run_test_suite(tests, sizeof(tests) / sizeof(tests[0]), NULL, 0, "SysDns.Mut");
}
