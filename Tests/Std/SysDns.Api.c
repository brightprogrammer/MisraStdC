/// file      : Tests/Std/SysDns.Mutants4.c
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Mutation-hardening tests for the Sys/Dns RESOLVE API + init/deinit:
/// dns_resolve_5_{zstr,str}, dns_resolve_4_vec_{zstr,str},
/// dns_resolve_4_one_{zstr,str}, dns_resolver_init, DnsResolverDeinit.
///
/// Every lookup hits a CRAFTED in-memory /etc/hosts table -- we build
/// the `DnsResolver.hosts` Vec by hand from HostsEntry rows so there is
/// no disk read and no network I/O. The nameserver table is left empty
/// so any hosts MISS fails cleanly (the no-nameservers branch) instead
/// of hanging on a UDP timeout.

#include <Misra/Std/Allocator/Debug.h>
#include <Misra/Std/Allocator/Default.h>
#include <Misra/Std/Container/Str/Init.h>
#include <Misra/Std/Log.h>
#include <Misra/Std/Zstr.h>
#include <Misra/Sys/Dns.h>
#include <Misra/Sys/Socket.h>

#include "../Util/TestRunner.h"

// ---------------------------------------------------------------------------
// Crafted-hosts helpers (no disk, no network)
// ---------------------------------------------------------------------------

// Build a DnsResolver with an empty hosts + empty nameservers table.
// Mirrors what dns_resolver_init produces minus the disk reads, so the
// resolve functions see a populated `self->allocator` and valid Vecs.
static void resolver_init_empty(DnsResolver *r, Allocator *a) {
    r->allocator   = a;
    r->hosts       = VecInitT(r->hosts, a);
    r->nameservers = VecInitT(r->nameservers, a);
    r->timeout_ms  = 5000;
    r->retries     = 2;
}

// Append one IPv4 host row. `ip` is four dotted-decimal octets.
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

// Append one IPv6 host row from 16 raw bytes.
static void hosts_push_v6(DnsResolver *r, Zstr name, const u8 ip16[16]) {
    HostsEntry e = {0};
    e.name       = StrInitFromCstr(name, ZstrLen(name), r->allocator);
    for (u64 i = 0; i < 16; ++i) {
        e.ip[i] = ip16[i];
    }
    e.is_ipv6 = true;
    VecPushBackR(&r->hosts, e);
}

// Populate the canonical crafted table used by most tests:
//   1.2.3.4    single
//   5.6.7.8    multi
//   9.10.11.12 multi      (same name -> two IPv4s, in this order)
//   ::1        v6name
//   1.1.1.1    both
//   2606::1    both       (name present in both families)
static void resolver_init_crafted(DnsResolver *r, Allocator *a) {
    resolver_init_empty(r, a);

    hosts_push_v4(r, "single", 1, 2, 3, 4);
    hosts_push_v4(r, "multi", 5, 6, 7, 8);
    hosts_push_v4(r, "multi", 9, 10, 11, 12);

    u8 v6_loopback[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1}; // ::1
    hosts_push_v6(r, "v6name", v6_loopback);

    hosts_push_v4(r, "both", 1, 1, 1, 1);
    u8 v6_both[16] = {0x26, 0x06, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1}; // 2606::1
    hosts_push_v6(r, "both", v6_both);
}

static void resolver_deinit_crafted(DnsResolver *r) {
    DnsResolverDeinit(r);
}

// Compare a resolved IPv4 SocketAddr against expected dotted-decimal
// + port via SocketAddrFormat (exact-byte check without poking raw[]).
static bool v4_is(const SocketAddr *ad, Allocator *a, Zstr expect) {
    if (ad->family != SOCKET_FAMILY_INET) {
        return false;
    }
    Str  s  = SocketAddrFormat(ad, a);
    bool ok = (StrLen(&s) > 0) && ZstrCompare(StrBegin(&s), expect) == 0;
    StrDeinit(&s);
    return ok;
}

static bool v6_is(const SocketAddr *ad, Allocator *a, Zstr expect) {
    if (ad->family != SOCKET_FAMILY_INET6) {
        return false;
    }
    Str  s  = SocketAddrFormat(ad, a);
    bool ok = (StrLen(&s) > 0) && ZstrCompare(StrBegin(&s), expect) == 0;
    StrDeinit(&s);
    return ok;
}

// ---------------------------------------------------------------------------
// dns_resolve_5_zstr  (hosts fast path)
// ---------------------------------------------------------------------------

// Single-IP name resolves to exactly {1.2.3.4}. Locks the hosts match
// (StrLen>0, ZstrCompare==0), the append loop, and the exact bytes.
static bool test_sd4_resolve5_single_exact(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);

    DnsResolver r;
    resolver_init_crafted(&r, a);

    DnsAddrs out = VecInitT(out, a);
    bool     got = dns_resolve_5_zstr(&r, "single", 80, SOCKET_KIND_TCP, &out);

    bool ok = got && VecLen(&out) == 1 && v4_is(VecPtrAt(&out, 0), a, "1.2.3.4:80");

    VecDeinit(&out);
    resolver_deinit_crafted(&r);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// A name with two IPv4 rows -> exactly TWO addresses, in table order,
// each exact. Kills the append-loop / count / family-filter mutants.
static bool test_sd4_resolve5_multi_two_in_order(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);

    DnsResolver r;
    resolver_init_crafted(&r, a);

    DnsAddrs out = VecInitT(out, a);
    bool     got = dns_resolve_5_zstr(&r, "multi", 1234, SOCKET_KIND_TCP, &out);

    bool ok = got && VecLen(&out) == 2 && v4_is(VecPtrAt(&out, 0), a, "5.6.7.8:1234") &&
              v4_is(VecPtrAt(&out, 1), a, "9.10.11.12:1234");

    VecDeinit(&out);
    resolver_deinit_crafted(&r);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// A name present in BOTH families -> both the v4 and v6 row come back
// with exact bytes. Kills the per-family dispatch (is_ipv6 select).
static bool test_sd4_resolve5_both_families(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);

    DnsResolver r;
    resolver_init_crafted(&r, a);

    DnsAddrs out = VecInitT(out, a);
    bool     got = dns_resolve_5_zstr(&r, "both", 443, SOCKET_KIND_TCP, &out);

    // Order in the table is v4 (1.1.1.1) then v6 (2606::1).
    bool ok = got && VecLen(&out) == 2 && v4_is(VecPtrAt(&out, 0), a, "1.1.1.1:443") &&
              v6_is(VecPtrAt(&out, 1), a, "[2606::1]:443");

    VecDeinit(&out);
    resolver_deinit_crafted(&r);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// A pure-IPv6 host row resolves to the v6 address with exact bytes.
static bool test_sd4_resolve5_v6_exact(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);

    DnsResolver r;
    resolver_init_crafted(&r, a);

    DnsAddrs out = VecInitT(out, a);
    bool     got = dns_resolve_5_zstr(&r, "v6name", 53, SOCKET_KIND_TCP, &out);

    bool ok = got && VecLen(&out) == 1 && v6_is(VecPtrAt(&out, 0), a, "[::1]:53");

    VecDeinit(&out);
    resolver_deinit_crafted(&r);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// Case-insensitive + trailing-dot normalization still hits the row.
// Locks normalize_hostname being applied before the ZstrCompare.
static bool test_sd4_resolve5_normalized_match(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);

    DnsResolver r;
    resolver_init_crafted(&r, a);

    DnsAddrs out = VecInitT(out, a);
    bool     got = dns_resolve_5_zstr(&r, "SINGLE.", 80, SOCKET_KIND_TCP, &out);

    bool ok = got && VecLen(&out) == 1 && v4_is(VecPtrAt(&out, 0), a, "1.2.3.4:80");

    VecDeinit(&out);
    resolver_deinit_crafted(&r);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// A name that is NOT in the table, with an EMPTY nameserver list, fails
// cleanly (no network) and leaves `out` empty. Kills the
// `VecLen(nameservers) == 0` guard and the found-flag handling.
static bool test_sd4_resolve5_miss_no_ns(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);

    DnsResolver r;
    resolver_init_crafted(&r, a);

    DnsAddrs out = VecInitT(out, a);
    bool     got = dns_resolve_5_zstr(&r, "absent", 80, SOCKET_KIND_TCP, &out);

    bool ok = !got && VecLen(&out) == 0;

    VecDeinit(&out);
    resolver_deinit_crafted(&r);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// Hostname >= 256 bytes is rejected up front (kills the `>= 256`
// length-guard comparison/literal mutants).
static bool test_sd4_resolve5_overlong_rejected(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);

    DnsResolver r;
    resolver_init_crafted(&r, a);

    char longname[300];
    for (u64 i = 0; i < 299; ++i) {
        longname[i] = 'a';
    }
    longname[299] = '\0';

    DnsAddrs out = VecInitT(out, a);
    bool     got = dns_resolve_5_zstr(&r, longname, 80, SOCKET_KIND_TCP, &out);

    bool ok = !got && VecLen(&out) == 0;

    VecDeinit(&out);
    resolver_deinit_crafted(&r);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// A hostname of EXACTLY 256 bytes hits the `>= 256` reject boundary and
// returns false CLEANLY (no abort). On real code the guard trips before
// the 256-cap StrInitStack `norm` is touched. Any guard mutation that
// lets a 256-byte name through (`ZstrLen` -> 42, or `>=` -> `>`) would
// push 256 bytes into the non-growable `norm` and LOG_FATAL -- so this
// passing test pins the exact boundary and kills those guard mutants.
static bool test_sd4_resolve5_exact_256_rejected(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);

    DnsResolver r;
    resolver_init_crafted(&r, a);

    char name[257];
    for (u64 i = 0; i < 256; ++i) {
        name[i] = 'a';
    }
    name[256] = '\0';

    DnsAddrs out = VecInitT(out, a);
    bool     got = dns_resolve_5_zstr(&r, name, 80, SOCKET_KIND_TCP, &out);

    bool ok = !got && VecLen(&out) == 0;

    VecDeinit(&out);
    resolver_deinit_crafted(&r);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// A 255-byte hostname is UNDER the cap -> not rejected by the guard; it
// just misses the table (absent) and fails via the no-nameservers
// branch. Pins the boundary so `>= 256` cannot become `> 256` silently
// (a 255-byte name must NOT be treated as overlong, and must still be
// processed -- it reaches the hosts loop and the no-ns branch).
static bool test_sd4_resolve5_under_cap_processed(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);

    DnsResolver r;
    resolver_init_crafted(&r, a);

    // 6 chars "single" + dots to reach 255 total still normalizes to
    // "single" (trailing dots stripped) -> resolves to 1.2.3.4.
    char name[256];
    name[0] = 's';
    name[1] = 'i';
    name[2] = 'n';
    name[3] = 'g';
    name[4] = 'l';
    name[5] = 'e';
    for (u64 i = 6; i < 255; ++i) {
        name[i] = '.';
    }
    name[255] = '\0';

    DnsAddrs out = VecInitT(out, a);
    bool     got = dns_resolve_5_zstr(&r, name, 80, SOCKET_KIND_TCP, &out);

    bool ok = got && VecLen(&out) == 1 && v4_is(VecPtrAt(&out, 0), a, "1.2.3.4:80");

    VecDeinit(&out);
    resolver_deinit_crafted(&r);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// An /etc/hosts row CAN technically carry an empty name (a malformed
// line); the resolver guards against it with `StrLen(&e->name) > 0`
// before comparing. Resolving an empty hostname "" must therefore NOT
// match the empty-name row -- it returns not-found. If the length guard
// `> 0` were weakened to `>= 0`, the empty row would match the empty
// query and 7.7.7.7 would come back. This pins that guard.
static bool test_sd4_resolve5_empty_name_row_ignored(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);

    DnsResolver r;
    resolver_init_empty(&r, a);
    hosts_push_v4(&r, "", 7, 7, 7, 7); // malformed empty-name row

    DnsAddrs out = VecInitT(out, a);
    bool     got = dns_resolve_5_zstr(&r, "", 80, SOCKET_KIND_TCP, &out);

    bool ok = !got && VecLen(&out) == 0;

    VecDeinit(&out);
    resolver_deinit_crafted(&r);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// ---------------------------------------------------------------------------
// dns_resolve_5_str  (delegates to _zstr)
// ---------------------------------------------------------------------------

// The Str overload must produce an identical result to the Zstr one.
// Kills the delegating scalar-call mutant.
static bool test_sd4_resolve5_str_delegates(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);

    DnsResolver r;
    resolver_init_crafted(&r, a);

    Str      name = StrInitFromZstr("multi", a);
    DnsAddrs out  = VecInitT(out, a);
    bool     got  = dns_resolve_5_str(&r, &name, 7000, SOCKET_KIND_TCP, &out);

    bool ok = got && VecLen(&out) == 2 && v4_is(VecPtrAt(&out, 0), a, "5.6.7.8:7000") &&
              v4_is(VecPtrAt(&out, 1), a, "9.10.11.12:7000");

    StrDeinit(&name);
    VecDeinit(&out);
    resolver_deinit_crafted(&r);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// ---------------------------------------------------------------------------
// dns_resolve_4_vec_zstr  ("host:port" spec -> all addresses)
// ---------------------------------------------------------------------------

// "single:80" splits on the last ':' -> host "single", port 80 -> the
// one v4 with exact bytes. Kills the colon-scan / port-parse mutants
// and the trailing dns_resolve_5_zstr delegation.
static bool test_sd4_resolve4_vec_single(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);

    DnsResolver r;
    resolver_init_crafted(&r, a);

    DnsAddrs out = VecInitT(out, a);
    bool     got = dns_resolve_4_vec_zstr(&r, "single:80", SOCKET_KIND_TCP, &out);

    bool ok = got && VecLen(&out) == 1 && v4_is(VecPtrAt(&out, 0), a, "1.2.3.4:80");

    VecDeinit(&out);
    resolver_deinit_crafted(&r);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// "multi:9090" -> both v4 rows, in order, with the spec port stamped.
static bool test_sd4_resolve4_vec_multi(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);

    DnsResolver r;
    resolver_init_crafted(&r, a);

    DnsAddrs out = VecInitT(out, a);
    bool     got = dns_resolve_4_vec_zstr(&r, "multi:9090", SOCKET_KIND_TCP, &out);

    bool ok = got && VecLen(&out) == 2 && v4_is(VecPtrAt(&out, 0), a, "5.6.7.8:9090") &&
              v4_is(VecPtrAt(&out, 1), a, "9.10.11.12:9090");

    VecDeinit(&out);
    resolver_deinit_crafted(&r);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// Numeric IPv4 spec short-circuits via SocketAddrParse (no table, no
// network) -> exactly one v4 with exact bytes.
static bool test_sd4_resolve4_vec_numeric(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);

    DnsResolver r;
    resolver_init_crafted(&r, a);

    DnsAddrs out = VecInitT(out, a);
    bool     got = dns_resolve_4_vec_zstr(&r, "203.0.113.7:9999", SOCKET_KIND_TCP, &out);

    bool ok = got && VecLen(&out) == 1 && v4_is(VecPtrAt(&out, 0), a, "203.0.113.7:9999");

    VecDeinit(&out);
    resolver_deinit_crafted(&r);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// A spec with no ':' is rejected (kills the colon_at >= spec_len guard).
static bool test_sd4_resolve4_vec_no_port(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);

    DnsResolver r;
    resolver_init_crafted(&r, a);

    DnsAddrs out = VecInitT(out, a);
    bool     got = dns_resolve_4_vec_zstr(&r, "single", SOCKET_KIND_TCP, &out);

    bool ok = !got && VecLen(&out) == 0;

    VecDeinit(&out);
    resolver_deinit_crafted(&r);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// A bare "host:" with no digits is rejected (kills the empty-port
// `colon_at + 1 == spec_len` guard and the add-to-sub mutant).
static bool test_sd4_resolve4_vec_empty_port(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);

    DnsResolver r;
    resolver_init_crafted(&r, a);

    DnsAddrs out = VecInitT(out, a);
    bool     got = dns_resolve_4_vec_zstr(&r, "single:", SOCKET_KIND_TCP, &out);

    bool ok = !got && VecLen(&out) == 0;

    VecDeinit(&out);
    resolver_deinit_crafted(&r);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// A non-numeric port is rejected (kills the digit-range `< '0' || > '9'`
// comparison mutants).
static bool test_sd4_resolve4_vec_bad_port(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);

    DnsResolver r;
    resolver_init_crafted(&r, a);

    DnsAddrs out = VecInitT(out, a);
    bool     got = dns_resolve_4_vec_zstr(&r, "single:9z9", SOCKET_KIND_TCP, &out);

    bool ok = !got && VecLen(&out) == 0;

    VecDeinit(&out);
    resolver_deinit_crafted(&r);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// Port exactly 65535 (0xFFFF) is in range and accepted; the resolved
// address carries it. Pins the `next > 0xFFFF` overflow guard so it
// cannot become `>=` (which would reject the legal max port).
static bool test_sd4_resolve4_vec_max_port(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);

    DnsResolver r;
    resolver_init_crafted(&r, a);

    DnsAddrs out = VecInitT(out, a);
    bool     got = dns_resolve_4_vec_zstr(&r, "single:65535", SOCKET_KIND_TCP, &out);

    bool ok = got && VecLen(&out) == 1 && v4_is(VecPtrAt(&out, 0), a, "1.2.3.4:65535");

    VecDeinit(&out);
    resolver_deinit_crafted(&r);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// A host portion of EXACTLY 256 bytes hits the `colon_at >= 256` reject
// boundary and returns false CLEANLY. On real code the guard trips
// before the 256-cap `host` StrInitStack is filled. If that guard were
// weakened to `> 256`, the 256-byte host would be pushed into the
// non-growable `host` buffer and LOG_FATAL. Pins the boundary.
static bool test_sd4_resolve4_vec_host_256_rejected(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);

    DnsResolver r;
    resolver_init_crafted(&r, a);

    char spec[260];
    for (u64 i = 0; i < 256; ++i) {
        spec[i] = 'a';
    }
    spec[256] = ':';
    spec[257] = '8';
    spec[258] = '0';
    spec[259] = '\0';

    DnsAddrs out = VecInitT(out, a);
    bool     got = dns_resolve_4_vec_zstr(&r, spec, SOCKET_KIND_TCP, &out);

    bool ok = !got && VecLen(&out) == 0;

    VecDeinit(&out);
    resolver_deinit_crafted(&r);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// An absent host with a valid port still fails (no table hit, no
// nameservers). Pins the `ok = dns_resolve_5_zstr(...)` delegation so a
// constant-true substitution (or a dropped call) is observable.
static bool test_sd4_resolve4_vec_absent(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);

    DnsResolver r;
    resolver_init_crafted(&r, a);

    DnsAddrs out = VecInitT(out, a);
    bool     got = dns_resolve_4_vec_zstr(&r, "absent:80", SOCKET_KIND_TCP, &out);

    bool ok = !got && VecLen(&out) == 0;

    VecDeinit(&out);
    resolver_deinit_crafted(&r);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// ---------------------------------------------------------------------------
// dns_resolve_4_vec_str  (delegates to _vec_zstr)
// ---------------------------------------------------------------------------

static bool test_sd4_resolve4_vec_str_delegates(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);

    DnsResolver r;
    resolver_init_crafted(&r, a);

    Str      spec = StrInitFromZstr("multi:8080", a);
    DnsAddrs out  = VecInitT(out, a);
    bool     got  = dns_resolve_4_vec_str(&r, &spec, SOCKET_KIND_TCP, &out);

    bool ok = got && VecLen(&out) == 2 && v4_is(VecPtrAt(&out, 0), a, "5.6.7.8:8080") &&
              v4_is(VecPtrAt(&out, 1), a, "9.10.11.12:8080");

    StrDeinit(&spec);
    VecDeinit(&out);
    resolver_deinit_crafted(&r);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// ---------------------------------------------------------------------------
// dns_resolve_4_one_zstr  (resolve, take first)
// ---------------------------------------------------------------------------

// Single-IP spec -> the one address.
static bool test_sd4_resolve4_one_single(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);

    DnsResolver r;
    resolver_init_crafted(&r, a);

    SocketAddr one;
    bool       got = dns_resolve_4_one_zstr(&r, "single:80", SOCKET_KIND_TCP, &one);

    bool ok = got && v4_is(&one, a, "1.2.3.4:80");

    resolver_deinit_crafted(&r);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// Multi-IP spec -> exactly the FIRST address (5.6.7.8), not the second.
// Kills the "take index 0" / VecLen>0 mutants.
static bool test_sd4_resolve4_one_first_of_many(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);

    DnsResolver r;
    resolver_init_crafted(&r, a);

    SocketAddr one;
    bool       got = dns_resolve_4_one_zstr(&r, "multi:80", SOCKET_KIND_TCP, &one);

    bool ok = got && v4_is(&one, a, "5.6.7.8:80");

    resolver_deinit_crafted(&r);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// Absent name (empty nameservers) -> not found, `out` untouched.
static bool test_sd4_resolve4_one_miss(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);

    DnsResolver r;
    resolver_init_crafted(&r, a);

    SocketAddr one;
    bool       got = dns_resolve_4_one_zstr(&r, "absent:80", SOCKET_KIND_TCP, &one);

    bool ok = !got;

    resolver_deinit_crafted(&r);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// ---------------------------------------------------------------------------
// dns_resolve_4_one_str  (delegates to _one_zstr)
// ---------------------------------------------------------------------------

static bool test_sd4_resolve4_one_str_delegates(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);

    DnsResolver r;
    resolver_init_crafted(&r, a);

    Str        spec = StrInitFromZstr("multi:80", a);
    SocketAddr one;
    bool       got = dns_resolve_4_one_str(&r, &spec, SOCKET_KIND_TCP, &one);

    bool ok = got && v4_is(&one, a, "5.6.7.8:80");

    StrDeinit(&spec);
    resolver_deinit_crafted(&r);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// ---------------------------------------------------------------------------
// dns_resolver_init  (real /etc/hosts + /etc/resolv.conf path)
// ---------------------------------------------------------------------------

// A genuine init succeeds, sets the documented defaults (timeout_ms =
// 5000, retries = 2) and wires the allocator. Kills the two
// assign-const mutants on the default fields and the MemSet/alloc set.
static bool test_sd4_init_defaults(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);

    DnsResolver r;
    bool        ok = dns_resolver_init(&r, a);

    ok = ok && r.timeout_ms == 5000 && r.retries == 2 && r.allocator == a;

    DnsResolverDeinit(&r);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// After a real init the hosts table is populated from disk (every Linux
// /etc/hosts has `localhost`) and resolves without network. This
// exercises parse_hosts_table being CALLED (kills the remove-void-call
// mutant on the parse_hosts_table line) -- if that call were removed
// the table would be empty and "localhost" would not resolve.
static bool test_sd4_init_loads_hosts(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);

    DnsResolver r;
    bool        ok = dns_resolver_init(&r, a);

    DnsAddrs out = VecInitT(out, a);
    bool     got = dns_resolve_5_zstr(&r, "localhost", 80, SOCKET_KIND_TCP, &out);

    bool found_v4 = false;
    if (got) {
        VecForeachPtr(&out, ad) {
            if (ad->family == SOCKET_FAMILY_INET) {
                found_v4 = true;
            }
        }
    }

    VecDeinit(&out);
    DnsResolverDeinit(&r);
    DefaultAllocatorDeinit(&alloc);
    return ok && got && found_v4;
}

// A real init also reads /etc/resolv.conf into the nameserver table.
// The run environment's resolv.conf carries at least one `nameserver`
// directive, so after init the table is non-empty. This exercises
// parse_resolv_conf being CALLED -- if that call were removed the
// nameserver table would stay empty.
static bool test_sd4_init_loads_nameservers(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);

    DnsResolver r;
    bool        ok = dns_resolver_init(&r, a);

    bool have_ns = VecLen(&r.nameservers) > 0;

    DnsResolverDeinit(&r);
    DefaultAllocatorDeinit(&alloc);
    return ok && have_ns;
}

// Init rejects a NULL allocator (returns false). Pins the early guard.
static bool test_sd4_init_null_alloc(void) {
    DnsResolver r;
    bool        got = dns_resolver_init(&r, NULL);
    return !got;
}

// ---------------------------------------------------------------------------
// DnsResolverDeinit  (frees every owned string / Vec)
// ---------------------------------------------------------------------------

// Populate a crafted table, Deinit, and assert the allocator's
// live-allocation count returns to the pre-population baseline. Uses an
// explicit DebugAllocator so the leak check is exact regardless of the
// build's default-allocator backend. If the StrDeinit cleanup loop or
// either VecDeinit were removed, the crafted host-name strings / the
// hosts Vec backing store would leak and the live count would stay
// elevated.
static bool test_sd4_deinit_no_leak(void) {
    DebugAllocator alloc    = DebugAllocatorInit();
    Allocator     *a        = ALLOCATOR_OF(&alloc);
    size           baseline = DebugAllocatorLiveCount(&alloc);

    DnsResolver r;
    resolver_init_crafted(&r, a);

    // Sanity: population actually allocated something.
    bool grew = DebugAllocatorLiveCount(&alloc) > baseline;

    DnsResolverDeinit(&r);

    bool back = DebugAllocatorLiveCount(&alloc) == baseline;
    bool ok   = grew && back && r.allocator == NULL;

    DebugAllocatorDeinit(&alloc);
    return ok;
}

// ---------------------------------------------------------------------------

int main(void) {
    WriteFmt("[INFO] Starting SysDns.Mutants4 tests\n\n");

    TestFunction tests[] = {
        test_sd4_resolve5_single_exact,
        test_sd4_resolve5_multi_two_in_order,
        test_sd4_resolve5_both_families,
        test_sd4_resolve5_v6_exact,
        test_sd4_resolve5_normalized_match,
        test_sd4_resolve5_miss_no_ns,
        test_sd4_resolve5_overlong_rejected,
        test_sd4_resolve5_exact_256_rejected,
        test_sd4_resolve5_under_cap_processed,
        test_sd4_resolve5_empty_name_row_ignored,
        test_sd4_resolve5_str_delegates,
        test_sd4_resolve4_vec_single,
        test_sd4_resolve4_vec_multi,
        test_sd4_resolve4_vec_numeric,
        test_sd4_resolve4_vec_no_port,
        test_sd4_resolve4_vec_empty_port,
        test_sd4_resolve4_vec_bad_port,
        test_sd4_resolve4_vec_max_port,
        test_sd4_resolve4_vec_host_256_rejected,
        test_sd4_resolve4_vec_absent,
        test_sd4_resolve4_vec_str_delegates,
        test_sd4_resolve4_one_single,
        test_sd4_resolve4_one_first_of_many,
        test_sd4_resolve4_one_miss,
        test_sd4_resolve4_one_str_delegates,
        test_sd4_init_defaults,
        test_sd4_init_loads_hosts,
        test_sd4_init_loads_nameservers,
        test_sd4_init_null_alloc,
        test_sd4_deinit_no_leak,
    };

    return run_test_suite(tests, sizeof(tests) / sizeof(tests[0]), NULL, 0, "SysDns.Api");
}
