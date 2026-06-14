/// file      : Tests/Std/SysDns.Blind.c
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Blind-survivor hardening for `Sys/Dns.c`. Targets the one remaining
/// unit-reachable survivor in `dns_resolve_4_vec_zstr`:
///
///   561:9  cxx_init_const on `u64 colon_at = spec_len;`
///
/// The other five survivors all sit on the real-UDP path
/// (`udp_round_trip` / `try_one_query` / the nameserver query loop) and
/// are not unit-reachable -- see the EQUIVALENT proofs in the worker
/// report and the rationale already recorded for `SysDns.Wire`.
///
/// We pull the source unit in directly (same idiom as `SysDns.Wire.c`)
/// so the resolve entry points run against a CRAFTED in-memory hosts
/// table -- no disk, no network. The nameserver table is left empty so
/// any table MISS fails cleanly via the no-nameservers branch instead of
/// hanging on a UDP timeout.

#include <Misra.h>
#include <Misra/Std/Allocator/Default.h>
#include <Misra/Std/Container/Str/Init.h>
#include <Misra/Std/Zstr.h>
#include <Misra/Sys/Dns.h>
#include <Misra/Sys/Socket.h>

#include "../Util/TestRunner.h"

// Pull in the unit under test so the static helpers / file-locals are
// directly reachable from this object.
#include "../../Source/Misra/Sys/Dns.c"

// ---------------------------------------------------------------------------
// Crafted-hosts helpers (no disk, no network) -- mirror SysDns.Api.c.
// ---------------------------------------------------------------------------

static void blind_resolver_init_empty(DnsResolver *r, Allocator *a) {
    r->allocator   = a;
    r->hosts       = VecInitT(r->hosts, a);
    r->nameservers = VecInitT(r->nameservers, a);
    r->timeout_ms  = 5000;
    r->retries     = 2;
}

static void blind_hosts_push_v4(DnsResolver *r, Zstr name, u8 b0, u8 b1, u8 b2, u8 b3) {
    HostsEntry e = {0};
    e.name       = StrInitFromCstr(name, ZstrLen(name), r->allocator);
    e.ip[0]      = b0;
    e.ip[1]      = b1;
    e.ip[2]      = b2;
    e.ip[3]      = b3;
    e.is_ipv6    = false;
    VecPushBackR(&r->hosts, e);
}

static bool blind_v4_is(const SocketAddr *ad, Allocator *a, Zstr expect) {
    if (ad->family != SOCKET_FAMILY_INET) {
        return false;
    }
    Str  s  = SocketAddrFormat(ad, a);
    bool ok = (StrLen(&s) > 0) && ZstrCompare(StrBegin(&s), expect) == 0;
    StrDeinit(&s);
    return ok;
}

// ---------------------------------------------------------------------------
// 561:9  cxx_init_const on `u64 colon_at = spec_len;`
//
// In `dns_resolve_4_vec_zstr`, after the SocketAddrParse short-circuit
// fails (non-numeric spec), the code scans backwards for the last ':' to
// split host from port. `colon_at` is seeded with `spec_len` so that a
// spec with NO ':' is detected by the post-loop guard
// `if (colon_at >= spec_len) -> "no :port" -> return false`.
//
// The mutant replaces that seed with the literal 42. To make the swap
// observable we craft a colon-LESS spec whose length is > 42 and whose
// first 42 bytes are a real (42-char) host-table name, followed by two
// more bytes:
//
//   spec = <42-char name "aaaa...">  +  "80"     (length 44, NO colon)
//
//   Real code  : no ':' found -> colon_at = spec_len = 44.
//                44 >= 44 -> reject -> returns false, out stays empty.
//   Mutant     : colon_at = 42. 42 >= 44 is FALSE -> NOT rejected.
//                42 >= 256 false. colon_at+1 (43) != spec_len (44), and
//                spec[43]='0' is a digit -> port parse succeeds (port 0),
//                host = spec[0..42) = the 42-char name -> MATCHES the
//                table -> returns TRUE with one address.
//
// So the real code returns false/empty here; the mutant returns
// true/non-empty. This test asserts the real-code outcome (false, empty)
// and therefore dies on the mutant.
// ---------------------------------------------------------------------------

static bool test_sdb_vec_no_colon_rejected_over_42(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);

    DnsResolver r;
    blind_resolver_init_empty(&r, a);

    // A 42-char host name made entirely of 'a'. Registering it in the
    // table is what lets the mutant (colon_at=42) "succeed": its host
    // slice spec[0..42) is exactly this name.
    char name42[43];
    for (u64 i = 0; i < 42; ++i) {
        name42[i] = 'a';
    }
    name42[42] = '\0';
    blind_hosts_push_v4(&r, name42, 5, 6, 7, 8);

    // spec = the 42-char name followed by "80" -- 44 bytes, NO colon.
    char spec[45];
    for (u64 i = 0; i < 42; ++i) {
        spec[i] = 'a';
    }
    spec[42] = '8';
    spec[43] = '0';
    spec[44] = '\0';

    DnsAddrs out = VecInitT(out, a);
    bool     got = dns_resolve_4_vec_zstr(&r, spec, SOCKET_KIND_TCP, &out);

    // Real code: no ":port" -> rejected -> false, nothing appended.
    bool ok = (!got) && VecLen(&out) == 0;

    VecDeinit(&out);
    DnsResolverDeinit(&r);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// A second, complementary pin: a colon-LESS spec whose length is < 42
// must ALSO be rejected. Here the mutant's seed (42) and the real seed
// (spec_len) agree on the reject decision (both >= spec_len when
// spec_len <= 42), so this case does NOT distinguish the mutant -- but it
// guards the "short no-colon spec" branch against regressions and keeps
// the documented reject behaviour asserted on both sides of 42.
static bool test_sdb_vec_no_colon_rejected_under_42(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);

    DnsResolver r;
    blind_resolver_init_empty(&r, a);
    blind_hosts_push_v4(&r, "host", 1, 2, 3, 4);

    DnsAddrs out = VecInitT(out, a);
    bool     got = dns_resolve_4_vec_zstr(&r, "hostnoport", SOCKET_KIND_TCP, &out);

    bool ok = (!got) && VecLen(&out) == 0;

    VecDeinit(&out);
    DnsResolverDeinit(&r);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// A normal "name:port" spec longer than 42 bytes total still resolves
// correctly: the real `colon_at = spec_len` seed is immediately
// overwritten by the backward ':' scan, so a valid colon spec is
// unaffected by the seed value. Confirms the seed only governs the
// no-colon reject path (and that the 42-byte name above is a legitimate
// table key, not an artefact).
static bool test_sdb_vec_long_name_with_port_resolves(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);

    DnsResolver r;
    blind_resolver_init_empty(&r, a);

    char name42[43];
    for (u64 i = 0; i < 42; ++i) {
        name42[i] = 'a';
    }
    name42[42] = '\0';
    blind_hosts_push_v4(&r, name42, 5, 6, 7, 8);

    // 42-char name + ":80" -> host = name42, port 80.
    char spec[46];
    for (u64 i = 0; i < 42; ++i) {
        spec[i] = 'a';
    }
    spec[42] = ':';
    spec[43] = '8';
    spec[44] = '0';
    spec[45] = '\0';

    DnsAddrs out = VecInitT(out, a);
    bool     got = dns_resolve_4_vec_zstr(&r, spec, SOCKET_KIND_TCP, &out);

    bool ok = got && VecLen(&out) == 1 && blind_v4_is(VecPtrAt(&out, 0), a, "5.6.7.8:80");

    VecDeinit(&out);
    DnsResolverDeinit(&r);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// ---------------------------------------------------------------------------

int main(void) {
    WriteFmt("[INFO] Starting SysDns.Blind tests\n\n");

    TestFunction tests[] = {
        test_sdb_vec_no_colon_rejected_over_42,
        test_sdb_vec_no_colon_rejected_under_42,
        test_sdb_vec_long_name_with_port_resolves,
    };

    return run_test_suite(tests, sizeof(tests) / sizeof(tests[0]), NULL, 0, "SysDns.Blind");
}
