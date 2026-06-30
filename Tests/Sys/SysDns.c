/// file      : Tests/Std/SysDns.c
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// The one SysDns test suite. Everything is driven through the PUBLIC
/// Sys/Dns API -- no `#include` of the implementation TU, no calls to
/// static helpers. The hosts / resolv parsers are exercised by writing
/// crafted fixture files (via the public File API) and feeding them
/// through `DnsResolverAddHostsPath` / `DnsResolverAddResolvPath`, then
/// inspecting the public resolver fields (`r.hosts`, `r.nameservers`).
/// The sockaddr builders are reached through `DnsResolve` (which stamps
/// the port onto a resolved hosts row) and asserted on the returned
/// `SocketAddr`'s public `raw` / `length` / `family`.

#include <Misra/Std/Allocator/Debug.h>
#include <Misra/Std/Allocator/Default.h>
#include <Misra/Std/Container/Str/Init.h>
#include <Misra/Std/File.h>
#include <Misra/Std/Log.h>
#include <Misra/Std/Zstr.h>
#include <Misra/Sys/Dir.h>
#include <Misra/Sys/Dns.h>
#include <Misra/Sys/Socket.h>

#include "../Util/TestRunner.h"

// ---------------------------------------------------------------------------
// Shared helpers
// ---------------------------------------------------------------------------

// Build a resolver with empty hosts + nameserver tables (no disk read).
// This is what `DnsResolverInit` produces minus the /etc parsing, so the
// crafted-table and Add*Path tests start from a known-empty state.
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

static void hosts_push_v6(DnsResolver *r, Zstr name, const u8 ip16[16]) {
    HostsEntry e = {0};
    e.name       = StrInitFromCstr(name, ZstrLen(name), r->allocator);
    for (u64 i = 0; i < 16; ++i) {
        e.ip[i] = ip16[i];
    }
    e.is_ipv6 = true;
    VecPushBackR(&r->hosts, e);
}

// Canonical crafted table used by the resolve-API tests.
static void resolver_init_crafted(DnsResolver *r, Allocator *a) {
    resolver_init_empty(r, a);
    hosts_push_v4(r, "single", 1, 2, 3, 4);
    hosts_push_v4(r, "multi", 5, 6, 7, 8);
    hosts_push_v4(r, "multi", 9, 10, 11, 12);
    u8 v6_loopback[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1};   // ::1
    hosts_push_v6(r, "v6name", v6_loopback);
    hosts_push_v4(r, "both", 1, 1, 1, 1);
    u8 v6_both[16] = {0x26, 0x06, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1}; // 2606::1
    hosts_push_v6(r, "both", v6_both);
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

static bool v6_is(const SocketAddr *ad, Allocator *a, Zstr expect) {
    if (ad->family != SOCKET_FAMILY_INET6) {
        return false;
    }
    Str  s  = SocketAddrFormat(ad, a);
    bool ok = (StrLen(&s) > 0) && ZstrCompare(StrBegin(&s), expect) == 0;
    StrDeinit(&s);
    return ok;
}

// Write `body` to a fresh temp file in the cwd; returns its path (caller
// FileRemove + StrDeinit). The on-disk file holds exactly `body`.
static Str write_temp(Allocator *a, Zstr body) {
    Str  path = StrInit(a);
    File f    = file_open_temp(&path, a);
    if (FileIsOpen(&f)) {
        u64 n = ZstrLen(body);
        if (n > 0) {
            FileWrite(&f, body, n);
        }
        FileClose(&f);
    }
    return path;
}

static void drop_temp(Str *path) {
    FileRemove(path);
    StrDeinit(path);
}

static const HostsEntry *find_host(HostsTable *t, Zstr name) {
    VecForeachPtr(t, e) {
        if (StrLen(&e->name) > 0 && ZstrCompare(StrBegin(&e->name), name) == 0) {
            return e;
        }
    }
    return (const HostsEntry *)0;
}

static bool host_v4_is(const HostsEntry *e, u8 a, u8 b, u8 c, u8 d) {
    return e && !e->is_ipv6 && e->ip[0] == a && e->ip[1] == b && e->ip[2] == c && e->ip[3] == d;
}

// Compare nameserver `i` formatted as "ip:port" against `expect`.
static bool ns_fmt_is(DnsResolver *r, u64 i, Allocator *a, Zstr expect) {
    if (VecLen(&r->nameservers) <= i) {
        return false;
    }
    Str  s  = SocketAddrFormat(VecPtrAt(&r->nameservers, i), a);
    bool ok = (StrLen(&s) > 0) && ZstrCompare(StrBegin(&s), expect) == 0;
    StrDeinit(&s);
    return ok;
}

// ===========================================================================
// Public resolve API against the live /etc files.
// ===========================================================================

// /etc/hosts always has `localhost` -> 127.0.0.1. Resolving "localhost"
// should hit the hosts table without going to the network.
static bool test_dns_resolve_localhost_from_hosts(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);

    DnsResolver r;
    bool        ok = DnsResolverInit(&r, a);
    if (!ok) {
        DefaultAllocatorDeinit(&alloc);
        return false;
    }

    DnsAddrs out = VecInitT(out, a);
    bool     got = DnsResolve(&r, "localhost", 8080, SOCKET_KIND_TCP, &out);

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
    return got && found_v4;
}

// Hostnames are matched case-insensitively per RFC 1035 section 2.3.3.
static bool test_dns_resolve_hosts_case_insensitive(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);

    DnsResolver r;
    DnsResolverInit(&r, a);

    DnsAddrs out = VecInitT(out, a);
    bool     got = DnsResolve(&r, "LocalHost", 0, SOCKET_KIND_TCP, &out);

    VecDeinit(&out);
    DnsResolverDeinit(&r);
    DefaultAllocatorDeinit(&alloc);
    return got;
}

// Trailing dot on a hostname is the FQDN form -- resolves the same.
static bool test_dns_resolve_hosts_trailing_dot(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);

    DnsResolver r;
    DnsResolverInit(&r, a);

    DnsAddrs out = VecInitT(out, a);
    bool     got = DnsResolve(&r, "localhost.", 0, SOCKET_KIND_TCP, &out);

    VecDeinit(&out);
    DnsResolverDeinit(&r);
    DefaultAllocatorDeinit(&alloc);
    return got;
}

// 4-arg overload (vec): numeric IPv4 spec short-circuits via SocketAddrParse.
static bool test_dns_resolve_spec_numeric_v4(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);

    DnsResolver r;
    DnsResolverInit(&r, a);

    DnsAddrs out = VecInitT(out, a);
    bool     got = DnsResolve(&r, "203.0.113.7:9999", SOCKET_KIND_TCP, &out);

    bool ok = got && VecLen(&out) == 1 && VecPtrAt(&out, 0)->family == SOCKET_FAMILY_INET;
    if (ok) {
        Str s = SocketAddrFormat(VecPtrAt(&out, 0), a);
        ok    = (StrLen(&s) > 0) && ZstrCompare(StrBegin(&s), "203.0.113.7:9999") == 0;
        StrDeinit(&s);
    }

    VecDeinit(&out);
    DnsResolverDeinit(&r);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// 4-arg overload (vec): bracketed IPv6 spec short-circuits the same way.
static bool test_dns_resolve_spec_numeric_v6(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);

    DnsResolver r;
    DnsResolverInit(&r, a);

    DnsAddrs out = VecInitT(out, a);
    bool     got = DnsResolve(&r, "[::1]:443", SOCKET_KIND_TCP, &out);

    bool ok = got && VecLen(&out) == 1 && VecPtrAt(&out, 0)->family == SOCKET_FAMILY_INET6;
    if (ok) {
        Str s = SocketAddrFormat(VecPtrAt(&out, 0), a);
        ok    = (StrLen(&s) > 0) && ZstrCompare(StrBegin(&s), "[::1]:443") == 0;
        StrDeinit(&s);
    }

    VecDeinit(&out);
    DnsResolverDeinit(&r);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// 4-arg overload (vec): hostname spec falls through to /etc/hosts.
static bool test_dns_resolve_spec_hostname(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);

    DnsResolver r;
    DnsResolverInit(&r, a);

    DnsAddrs out = VecInitT(out, a);
    bool     got = DnsResolve(&r, "localhost:53", SOCKET_KIND_TCP, &out);

    bool ok = got && VecLen(&out) > 0;
    if (ok) {
        VecForeachPtr(&out, ad) {
            Str s = SocketAddrFormat(ad, a);
            u64 L = StrLen(&s);
            if (L < 3 || StrBegin(&s)[L - 1] != '3' || StrBegin(&s)[L - 2] != '5' || StrBegin(&s)[L - 3] != ':') {
                ok = false;
            }
            StrDeinit(&s);
        }
    }

    VecDeinit(&out);
    DnsResolverDeinit(&r);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// 4-arg overload (single-addr form): pass `SocketAddr *`.
static bool test_dns_resolve_spec_single_addr(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);

    DnsResolver r;
    DnsResolverInit(&r, a);

    SocketAddr one;
    bool       got = DnsResolve(&r, "127.0.0.1:80", SOCKET_KIND_TCP, &one);

    bool ok = got && one.family == SOCKET_FAMILY_INET;
    if (ok) {
        Str s = SocketAddrFormat(&one, a);
        ok    = (StrLen(&s) > 0) && ZstrCompare(StrBegin(&s), "127.0.0.1:80") == 0;
        StrDeinit(&s);
    }

    DnsResolverDeinit(&r);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// 4-arg overload: missing ":port" is rejected.
static bool test_dns_resolve_spec_no_port(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);

    DnsResolver r;
    DnsResolverInit(&r, a);

    DnsAddrs out = VecInitT(out, a);
    bool     got = DnsResolve(&r, "localhost", SOCKET_KIND_TCP, &out);

    bool ok = !got && VecLen(&out) == 0;

    VecDeinit(&out);
    DnsResolverDeinit(&r);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// 4-arg overload: non-numeric port is rejected.
static bool test_dns_resolve_spec_bad_port(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);

    DnsResolver r;
    DnsResolverInit(&r, a);

    DnsAddrs out = VecInitT(out, a);
    bool     got = DnsResolve(&r, "localhost:abc", SOCKET_KIND_TCP, &out);

    bool ok = !got;

    VecDeinit(&out);
    DnsResolverDeinit(&r);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// ===========================================================================
// /etc/hosts parser -- crafted fixtures through DnsResolverAddHostsPath.
// ===========================================================================

// Single simple entry resolves to the exact v4 IP, stored name verbatim.
static bool test_hosts_single_v4(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);

    DnsResolver r;
    resolver_init_empty(&r, a);
    Str path = write_temp(a, "127.0.0.1 localhost\n");
    DnsResolverAddHostsPath(&r, &path);

    const HostsEntry *e = find_host(&r.hosts, "localhost");
    bool ok = VecLen(&r.hosts) == 1 && host_v4_is(e, 127, 0, 0, 1) && ZstrCompare(StrBegin(&e->name), "localhost") == 0;

    drop_temp(&path);
    DnsResolverDeinit(&r);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// One line, name + three aliases -> four entries all sharing the IP.
static bool test_hosts_aliases(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);

    DnsResolver r;
    resolver_init_empty(&r, a);
    Str path = write_temp(a, "192.168.1.5 host.example alias1 alias2 alias3\n");
    DnsResolverAddHostsPath(&r, &path);

    bool ok = VecLen(&r.hosts) == 4 && host_v4_is(find_host(&r.hosts, "host.example"), 192, 168, 1, 5) &&
              host_v4_is(find_host(&r.hosts, "alias1"), 192, 168, 1, 5) &&
              host_v4_is(find_host(&r.hosts, "alias2"), 192, 168, 1, 5) &&
              host_v4_is(find_host(&r.hosts, "alias3"), 192, 168, 1, 5);

    drop_temp(&path);
    DnsResolverDeinit(&r);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// Comment + blank lines produce no entries; the real entry between them parses.
static bool test_hosts_comments_and_blanks(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);

    DnsResolver r;
    resolver_init_empty(&r, a);
    Str path = write_temp(
        a,
        "# a comment line\n"
        "\n"
        "   \n"
        "10.0.0.1 a\n"
        "# 1.2.3.4 commented.host\n"
    );
    DnsResolverAddHostsPath(&r, &path);

    bool ok = VecLen(&r.hosts) == 1 && host_v4_is(find_host(&r.hosts, "a"), 10, 0, 0, 1) &&
              find_host(&r.hosts, "commented.host") == (const HostsEntry *)0;

    drop_temp(&path);
    DnsResolverDeinit(&r);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// A trailing in-line `# comment` after a valid entry is dropped.
static bool test_hosts_inline_comment(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);

    DnsResolver r;
    resolver_init_empty(&r, a);
    Str path = write_temp(a, "10.0.0.2 namex # trailing comment words\n");
    DnsResolverAddHostsPath(&r, &path);

    bool ok = VecLen(&r.hosts) == 1 && host_v4_is(find_host(&r.hosts, "namex"), 10, 0, 0, 2) &&
              find_host(&r.hosts, "trailing") == (const HostsEntry *)0 &&
              find_host(&r.hosts, "#") == (const HostsEntry *)0;

    drop_temp(&path);
    DnsResolverDeinit(&r);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// A `#` immediately after a name token (no space) still terminates the name.
static bool test_hosts_inline_comment_no_space(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);

    DnsResolver r;
    resolver_init_empty(&r, a);
    Str path = write_temp(a, "10.0.0.2 namex#tail\n");
    DnsResolverAddHostsPath(&r, &path);

    const HostsEntry *e = find_host(&r.hosts, "namex");
    bool ok = VecLen(&r.hosts) == 1 && host_v4_is(e, 10, 0, 0, 2) && ZstrCompare(StrBegin(&e->name), "namex") == 0;

    drop_temp(&path);
    DnsResolverDeinit(&r);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// Leading spaces+tabs before the IP are skipped; tabs separate fields.
static bool test_hosts_leading_and_tab_whitespace(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);

    DnsResolver r;
    resolver_init_empty(&r, a);
    Str path = write_temp(a, "  \t 172.16.0.9\thostt\talt\n");
    DnsResolverAddHostsPath(&r, &path);

    bool ok = VecLen(&r.hosts) == 2 && host_v4_is(find_host(&r.hosts, "hostt"), 172, 16, 0, 9) &&
              host_v4_is(find_host(&r.hosts, "alt"), 172, 16, 0, 9);

    drop_temp(&path);
    DnsResolverDeinit(&r);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// The stored name is lowercased: an uppercase source name is found only
// via its lowercase key.
static bool test_hosts_case_lowered(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);

    DnsResolver r;
    resolver_init_empty(&r, a);
    Str path = write_temp(a, "10.1.1.1 MixedCaseHost\n");
    DnsResolverAddHostsPath(&r, &path);

    bool ok = VecLen(&r.hosts) == 1 && host_v4_is(find_host(&r.hosts, "mixedcasehost"), 10, 1, 1, 1) &&
              find_host(&r.hosts, "MixedCaseHost") == (const HostsEntry *)0;

    drop_temp(&path);
    DnsResolverDeinit(&r);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// ascii_lower boundaries: a name spanning the '@' '[' '`' guards. Only
// 'A'..'Z' fold; the bytes just outside the range are preserved. The
// stored name is asserted exactly.  "@AZ[`M" -> "@az[`m".
static bool test_hosts_ascii_lower_boundaries(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);

    DnsResolver r;
    resolver_init_empty(&r, a);
    Str path = write_temp(a, "10.1.1.2 @AZ[`M\n");
    DnsResolverAddHostsPath(&r, &path);

    const HostsEntry *e = find_host(&r.hosts, "@az[`m");
    bool ok = VecLen(&r.hosts) == 1 && host_v4_is(e, 10, 1, 1, 2) && ZstrCompare(StrBegin(&e->name), "@az[`m") == 0;

    drop_temp(&path);
    DnsResolverDeinit(&r);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// Last line without a trailing newline still parses; the final skip-to-eol
// must tolerate the zero-remaining cursor (no over-advance / abort).
static bool test_hosts_no_trailing_newline(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);

    DnsResolver r;
    resolver_init_empty(&r, a);
    Str path = write_temp(a, "10.0.0.1 a\n8.8.8.8 dns");
    DnsResolverAddHostsPath(&r, &path);

    bool ok = VecLen(&r.hosts) == 2 && host_v4_is(find_host(&r.hosts, "a"), 10, 0, 0, 1) &&
              host_v4_is(find_host(&r.hosts, "dns"), 8, 8, 8, 8);

    drop_temp(&path);
    DnsResolverDeinit(&r);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// A comment as the last line with no trailing newline: skip-to-eol must
// stop exactly at EOF (the `remaining > 0` guard). A wrong guard advances
// past the end and aborts.
static bool test_hosts_comment_eof_no_newline(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);

    DnsResolver r;
    resolver_init_empty(&r, a);
    Str path = write_temp(a, "10.0.0.1 a\n# trailing comment no newline");
    DnsResolverAddHostsPath(&r, &path);

    bool ok = VecLen(&r.hosts) == 1 && host_v4_is(find_host(&r.hosts, "a"), 10, 0, 0, 1);

    drop_temp(&path);
    DnsResolverDeinit(&r);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// Each line's entry resolves to ITS OWN ip, in order -- a skip desync
// would merge/drop lines.
static bool test_hosts_multiline_no_desync(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);

    DnsResolver r;
    resolver_init_empty(&r, a);
    Str path = write_temp(
        a,
        "1.1.1.1 one\n"
        "2.2.2.2 two\n"
        "3.3.3.3 three\n"
    );
    DnsResolverAddHostsPath(&r, &path);

    bool ok = VecLen(&r.hosts) == 3 && host_v4_is(find_host(&r.hosts, "one"), 1, 1, 1, 1) &&
              host_v4_is(find_host(&r.hosts, "two"), 2, 2, 2, 2) &&
              host_v4_is(find_host(&r.hosts, "three"), 3, 3, 3, 3);

    drop_temp(&path);
    DnsResolverDeinit(&r);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// IPv6 row parses to the v6 family and the right address bytes.
static bool test_hosts_ipv6(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);

    DnsResolver r;
    resolver_init_empty(&r, a);
    Str path = write_temp(a, "::1 ip6-localhost\n");
    DnsResolverAddHostsPath(&r, &path);

    const HostsEntry *e  = find_host(&r.hosts, "ip6-localhost");
    bool              ok = VecLen(&r.hosts) == 1 && e && e->is_ipv6;
    if (ok) {
        for (int i = 0; i < 15; ++i) {
            if (e->ip[i] != 0) {
                ok = false;
            }
        }
        if (e->ip[15] != 1) {
            ok = false;
        }
    }

    drop_temp(&path);
    DnsResolverDeinit(&r);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// A line whose first token is not a valid IP is rejected wholesale.
static bool test_hosts_non_ip_line_rejected(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);

    DnsResolver r;
    resolver_init_empty(&r, a);
    Str path = write_temp(
        a,
        "notanip somename morestuff\n"
        "10.0.0.7 realname\n"
    );
    DnsResolverAddHostsPath(&r, &path);

    bool ok = VecLen(&r.hosts) == 1 && host_v4_is(find_host(&r.hosts, "realname"), 10, 0, 0, 7) &&
              find_host(&r.hosts, "somename") == (const HostsEntry *)0;

    drop_temp(&path);
    DnsResolverDeinit(&r);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// An IP-only line (no hostname) yields no entries.
static bool test_hosts_ip_only_no_name(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);

    DnsResolver r;
    resolver_init_empty(&r, a);
    Str path = write_temp(
        a,
        "10.0.0.8\n"
        "10.0.0.9 named\n"
    );
    DnsResolverAddHostsPath(&r, &path);

    bool ok = VecLen(&r.hosts) == 1 && host_v4_is(find_host(&r.hosts, "named"), 10, 0, 0, 9);

    drop_temp(&path);
    DnsResolverDeinit(&r);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// A first token longer than the 64-char cap makes the parser skip the IP
// parse entirely, rejecting the line; a real entry after it recovers.
static bool test_hosts_overlong_first_token(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);

    DnsResolver r;
    resolver_init_empty(&r, a);

    char buf[160];
    u64  k = 0;
    for (; k < 70; ++k) {
        buf[k] = 'x';
    }
    Zstr tail = " spuriousname\n10.0.0.4 goodname\n";
    for (u64 i = 0; tail[i] != '\0'; ++i) {
        buf[k++] = tail[i];
    }
    buf[k] = '\0';

    Str path = write_temp(a, (Zstr)buf);
    DnsResolverAddHostsPath(&r, &path);

    bool ok = VecLen(&r.hosts) == 1 && host_v4_is(find_host(&r.hosts, "goodname"), 10, 0, 0, 4) &&
              find_host(&r.hosts, "spuriousname") == (const HostsEntry *)0;

    drop_temp(&path);
    DnsResolverDeinit(&r);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// A rejected first token followed on the same line by a valid IP: the
// reject branch must discard the rest of the line (no spurious entry).
static bool test_hosts_reject_skips_rest_of_line(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);

    DnsResolver r;
    resolver_init_empty(&r, a);
    Str path = write_temp(
        a,
        "bad 10.0.0.5 x\n"
        "10.0.0.6 realone\n"
    );
    DnsResolverAddHostsPath(&r, &path);

    bool ok = VecLen(&r.hosts) == 1 && host_v4_is(find_host(&r.hosts, "realone"), 10, 0, 0, 6) &&
              find_host(&r.hosts, "x") == (const HostsEntry *)0;

    drop_temp(&path);
    DnsResolverDeinit(&r);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// A missing hosts file is tolerated: the table stays empty.
static bool test_hosts_missing_file_tolerated(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);

    DnsResolver r;
    resolver_init_empty(&r, a);
    DnsResolverAddHostsPath(&r, "sysdns_definitely_missing_zzqq", 30);

    bool ok = VecLen(&r.hosts) == 0;

    DnsResolverDeinit(&r);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// A fixture larger than the 4096-byte slurp chunk: the parser must read
// every chunk and still find the single real entry at the very end.
static bool test_hosts_multichunk_slurp(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);

    Str  body   = StrInit(a);
    Zstr pad    = "# padding line to push the fixture past the four kilobyte chunk\n";
    u64  padlen = ZstrLen(pad);
    for (int i = 0; i < 300; ++i) {
        StrPushBackMany(&body, pad, padlen);
    }
    StrPushBackMany(&body, "10.20.30.40 farhost\n", ZstrLen("10.20.30.40 farhost\n"));

    DnsResolver r;
    resolver_init_empty(&r, a);
    Str path = write_temp(a, StrBegin(&body));
    DnsResolverAddHostsPath(&r, &path);

    bool ok = VecLen(&r.hosts) == 1 && host_v4_is(find_host(&r.hosts, "farhost"), 10, 20, 30, 40);

    drop_temp(&path);
    StrDeinit(&body);
    DnsResolverDeinit(&r);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// ===========================================================================
// /etc/resolv.conf parser -- crafted fixtures through DnsResolverAddResolvPath.
// ===========================================================================

// Two nameservers parsed in order; search/comment/domain lines ignored.
static bool test_resolv_basic(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);

    DnsResolver r;
    resolver_init_empty(&r, a);
    Str path = write_temp(
        a,
        "nameserver 8.8.8.8\n"
        "nameserver 1.1.1.1\n"
        "search foo.com bar.com\n"
        "# comment\n"
        "; comment2\n"
        "domain x.com\n"
    );
    DnsResolverAddResolvPath(&r, &path);

    bool ok = VecLen(&r.nameservers) == 2 && ns_fmt_is(&r, 0, a, "8.8.8.8:53") && ns_fmt_is(&r, 1, a, "1.1.1.1:53");

    drop_temp(&path);
    DnsResolverDeinit(&r);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// IPv6 nameserver: family INET6, IP round-trips, port 53.
static bool test_resolv_v6(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);

    DnsResolver r;
    resolver_init_empty(&r, a);
    Str path = write_temp(a, "nameserver ::1\n");
    DnsResolverAddResolvPath(&r, &path);

    bool ok = VecLen(&r.nameservers) == 1 && VecPtrAt(&r.nameservers, 0)->family == SOCKET_FAMILY_INET6 &&
              ns_fmt_is(&r, 0, a, "[::1]:53");

    drop_temp(&path);
    DnsResolverDeinit(&r);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// A '#'-comment line is skipped entirely.
static bool test_resolv_hash_comment_skipped(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);

    DnsResolver r;
    resolver_init_empty(&r, a);
    Str path = write_temp(a, "# nameserver 9.9.9.9\n");
    DnsResolverAddResolvPath(&r, &path);

    bool ok = VecLen(&r.nameservers) == 0;

    drop_temp(&path);
    DnsResolverDeinit(&r);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// A ';'-comment line is skipped.
static bool test_resolv_semicolon_comment_skipped(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);

    DnsResolver r;
    resolver_init_empty(&r, a);
    Str path = write_temp(a, "; nameserver 9.9.9.9\n");
    DnsResolverAddResolvPath(&r, &path);

    bool ok = VecLen(&r.nameservers) == 0;

    drop_temp(&path);
    DnsResolverDeinit(&r);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// "search <ip>" must not be treated as a nameserver.
static bool test_resolv_search_not_ns(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);

    DnsResolver r;
    resolver_init_empty(&r, a);
    Str path = write_temp(a, "search 8.8.8.8\n");
    DnsResolverAddResolvPath(&r, &path);

    bool ok = VecLen(&r.nameservers) == 0;

    drop_temp(&path);
    DnsResolverDeinit(&r);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// "nameserverx <ip>" -- the separator after the keyword must be whitespace.
static bool test_resolv_keyword_needs_sep(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);

    DnsResolver r;
    resolver_init_empty(&r, a);
    Str path = write_temp(a, "nameserverx 8.8.8.8\n");
    DnsResolverAddResolvPath(&r, &path);

    bool ok = VecLen(&r.nameservers) == 0;

    drop_temp(&path);
    DnsResolverDeinit(&r);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// Tab separator after "nameserver" is accepted.
static bool test_resolv_tab_sep(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);

    DnsResolver r;
    resolver_init_empty(&r, a);
    Str path = write_temp(a, "nameserver\t8.8.4.4\n");
    DnsResolverAddResolvPath(&r, &path);

    bool ok = VecLen(&r.nameservers) == 1 && ns_fmt_is(&r, 0, a, "8.8.4.4:53");

    drop_temp(&path);
    DnsResolverDeinit(&r);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// Leading whitespace before "nameserver" is skipped.
static bool test_resolv_leading_space(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);

    DnsResolver r;
    resolver_init_empty(&r, a);
    Str path = write_temp(a, "  nameserver 8.8.8.8\n");
    DnsResolverAddResolvPath(&r, &path);

    bool ok = VecLen(&r.nameservers) == 1 && ns_fmt_is(&r, 0, a, "8.8.8.8:53");

    drop_temp(&path);
    DnsResolverDeinit(&r);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// An invalid IP after "nameserver" yields no entry.
static bool test_resolv_bad_ip(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);

    DnsResolver r;
    resolver_init_empty(&r, a);
    Str path = write_temp(a, "nameserver not.an.ip.999\n");
    DnsResolverAddResolvPath(&r, &path);

    bool ok = VecLen(&r.nameservers) == 0;

    drop_temp(&path);
    DnsResolverDeinit(&r);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// A trailing '#'-comment after the IP is ignored; the IP still parses.
static bool test_resolv_trailing_comment(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);

    DnsResolver r;
    resolver_init_empty(&r, a);
    Str path = write_temp(a, "nameserver 8.8.8.8#note\n");
    DnsResolverAddResolvPath(&r, &path);

    bool ok = VecLen(&r.nameservers) == 1 && ns_fmt_is(&r, 0, a, "8.8.8.8:53");

    drop_temp(&path);
    DnsResolverDeinit(&r);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// "nameserver" with no IP after it yields no entry.
static bool test_resolv_empty_ip(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);

    DnsResolver r;
    resolver_init_empty(&r, a);
    Str path = write_temp(a, "nameserver \n");
    DnsResolverAddResolvPath(&r, &path);

    bool ok = VecLen(&r.nameservers) == 0;

    drop_temp(&path);
    DnsResolverDeinit(&r);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// Order is preserved across three nameservers.
static bool test_resolv_order(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);

    DnsResolver r;
    resolver_init_empty(&r, a);
    Str path = write_temp(
        a,
        "nameserver 10.0.0.1\n"
        "nameserver 10.0.0.2\n"
        "nameserver 10.0.0.3\n"
    );
    DnsResolverAddResolvPath(&r, &path);

    bool ok = VecLen(&r.nameservers) == 3 && ns_fmt_is(&r, 0, a, "10.0.0.1:53") && ns_fmt_is(&r, 1, a, "10.0.0.2:53") &&
              ns_fmt_is(&r, 2, a, "10.0.0.3:53");

    drop_temp(&path);
    DnsResolverDeinit(&r);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// Last line with no trailing newline still parses.
static bool test_resolv_no_trailing_newline(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);

    DnsResolver r;
    resolver_init_empty(&r, a);
    Str path = write_temp(a, "nameserver 8.8.8.8");
    DnsResolverAddResolvPath(&r, &path);

    bool ok = VecLen(&r.nameservers) == 1 && ns_fmt_is(&r, 0, a, "8.8.8.8:53");

    drop_temp(&path);
    DnsResolverDeinit(&r);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// ===========================================================================
// normalize_hostname -- driven behaviourally through DnsResolve.
// ===========================================================================

// An uppercase query with boundary chars must normalize to the stored
// lowercase row. "A@Z" -> "a@z" matches; the 'A'/'Z' folds and the '@'
// passthrough pin the normalize range bounds.
static bool test_norm_query_case_and_bounds(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);

    DnsResolver r;
    resolver_init_empty(&r, a);
    hosts_push_v4(&r, "a@z", 10, 0, 0, 1);

    DnsAddrs out = VecInitT(out, a);
    bool     got = dns_resolve_5_zstr(&r, "A@Z", 80, SOCKET_KIND_TCP, &out);

    bool ok = got && VecLen(&out) == 1 && v4_is(VecPtrAt(&out, 0), a, "10.0.0.1:80");

    VecDeinit(&out);
    DnsResolverDeinit(&r);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// A query with multiple trailing dots normalizes to the bare name.
static bool test_norm_query_trailing_dots(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);

    DnsResolver r;
    resolver_init_empty(&r, a);
    hosts_push_v4(&r, "host", 10, 0, 0, 2);

    DnsAddrs out = VecInitT(out, a);
    bool     got = dns_resolve_5_zstr(&r, "HOST...", 80, SOCKET_KIND_TCP, &out);

    bool ok = got && VecLen(&out) == 1 && v4_is(VecPtrAt(&out, 0), a, "10.0.0.2:80");

    VecDeinit(&out);
    DnsResolverDeinit(&r);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// Interior dots are preserved; only trailing ones are stripped.
static bool test_norm_query_interior_dots_kept(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);

    DnsResolver r;
    resolver_init_empty(&r, a);
    hosts_push_v4(&r, "a.b.c", 10, 0, 0, 3);

    DnsAddrs out = VecInitT(out, a);
    bool     got = dns_resolve_5_zstr(&r, "A.B.C.", 80, SOCKET_KIND_TCP, &out);

    bool ok = got && VecLen(&out) == 1 && v4_is(VecPtrAt(&out, 0), a, "10.0.0.3:80");

    VecDeinit(&out);
    DnsResolverDeinit(&r);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// A query that is only dots normalizes to empty and matches no real row.
static bool test_norm_query_all_dots_no_match(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);

    DnsResolver r;
    resolver_init_empty(&r, a);
    hosts_push_v4(&r, "host", 10, 0, 0, 4);

    DnsAddrs out = VecInitT(out, a);
    bool     got = dns_resolve_5_zstr(&r, "...", 80, SOCKET_KIND_TCP, &out);

    bool ok = !got && VecLen(&out) == 0;

    VecDeinit(&out);
    DnsResolverDeinit(&r);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// ===========================================================================
// sockaddr_v4 / sockaddr_v6 -- reached through DnsResolve, asserted on the
// returned SocketAddr's public raw / length / family.
// ===========================================================================

// A v4 hosts row resolved with a distinct-byte port (4660 = 0x1234): the
// family, length, raw family byte, network-order port bytes and the four
// address octets are all asserted -- a dropped htons swap or a wrong
// constant in sockaddr_v4 shows here.
static bool test_wire_v4_fields(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);

    DnsResolver r;
    resolver_init_empty(&r, a);
    hosts_push_v4(&r, "myhost", 1, 2, 3, 4);

    DnsAddrs out = VecInitT(out, a);
    bool     got = dns_resolve_5_zstr(&r, "myhost", 0x1234, SOCKET_KIND_TCP, &out);

    bool ok = got && VecLen(&out) == 1;
    if (ok) {
        const SocketAddr *ad = VecPtrAt(&out, 0);
        ok                   = ad->family == SOCKET_FAMILY_INET && ad->length == 16u;
        // Network byte order: port bytes are 0x12 then 0x34 at raw[2..4].
        ok = ok && ad->raw[2] == 0x12u && ad->raw[3] == 0x34u;
        // sin_addr at raw[4..8] holds the four octets verbatim.
        ok = ok && ad->raw[4] == 1u && ad->raw[5] == 2u && ad->raw[6] == 3u && ad->raw[7] == 4u;
#if PLATFORM_LINUX
        // sin_family low byte (AF_INET == 2 on Linux).
        ok = ok && ad->raw[0] == 2u && ad->raw[1] == 0u;
#endif
        ok = ok && v4_is(ad, a, "1.2.3.4:4660");
    }

    VecDeinit(&out);
    DnsResolverDeinit(&r);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// A v6 hosts row resolved with a distinct-byte port: family, length, raw
// family byte, port bytes and the 16-byte address are all asserted.
static bool test_wire_v6_fields(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);

    DnsResolver r;
    resolver_init_empty(&r, a);
    u8 ip6[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1}; // ::1
    hosts_push_v6(&r, "myhost6", ip6);

    DnsAddrs out = VecInitT(out, a);
    bool     got = dns_resolve_5_zstr(&r, "myhost6", 0x1234, SOCKET_KIND_TCP, &out);

    bool ok = got && VecLen(&out) == 1;
    if (ok) {
        const SocketAddr *ad = VecPtrAt(&out, 0);
        ok                   = ad->family == SOCKET_FAMILY_INET6 && ad->length == 28u;
        // sin6_port at raw[2..4], network order.
        ok = ok && ad->raw[2] == 0x12u && ad->raw[3] == 0x34u;
        // sin6_addr at raw[8..24]: ::1 is 15 zero bytes then 0x01.
        for (u32 i = 8; i < 23; ++i) {
            if (ad->raw[i] != 0u) {
                ok = false;
            }
        }
        ok = ok && ad->raw[23] == 1u;
#if PLATFORM_LINUX
        // sin6_family low byte (AF_INET6 == 10 on Linux).
        ok = ok && ad->raw[0] == 10u && ad->raw[1] == 0u;
#endif
        ok = ok && v6_is(ad, a, "[::1]:4660");
    }

    VecDeinit(&out);
    DnsResolverDeinit(&r);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// ===========================================================================
// DnsResolverAddNameserver
// ===========================================================================

// Appending a parsed nameserver grows the list by one and round-trips
// (its own port, not the resolv.conf default 53).
static bool test_ns_add_nameserver(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);

    DnsResolver r;
    resolver_init_empty(&r, a);

    SocketAddr ns;
    bool       parsed = SocketAddrParse(&ns, "127.0.0.1:5353", SOCKET_KIND_UDP);

    u64  before = VecLen(&r.nameservers);
    bool added  = DnsResolverAddNameserver(&r, ns);

    bool ok = parsed && added && VecLen(&r.nameservers) == before + 1 && ns_fmt_is(&r, before, a, "127.0.0.1:5353");

    DnsResolverDeinit(&r);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// A NULL resolver is rejected.
static bool test_ns_add_nameserver_null(void) {
    SocketAddr ns      = {0};
    bool       refused = !dns_resolver_add_nameserver(NULL, ns);
    return refused;
}

// ===========================================================================
// Additional config paths -- the Add*Path / Add*Paths plumbing.
// ===========================================================================

// AddHostsPath grows ONLY the hosts table, even when the file also carries
// a `nameserver` directive.
static bool test_addpath_hosts_only(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);

    DnsResolver r;
    resolver_init_empty(&r, a);
    Str path = write_temp(
        a,
        "1.2.3.4 hostrow\n"
        "nameserver 8.8.8.8\n"
    );
    bool added = DnsResolverAddHostsPath(&r, &path);

    bool ok = added && VecLen(&r.hosts) == 1 && host_v4_is(find_host(&r.hosts, "hostrow"), 1, 2, 3, 4) &&
              VecLen(&r.nameservers) == 0;

    drop_temp(&path);
    DnsResolverDeinit(&r);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// AddResolvPath grows ONLY the nameserver table, even when the file also
// carries a hosts row.
static bool test_addpath_resolv_only(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);

    DnsResolver r;
    resolver_init_empty(&r, a);
    Str path = write_temp(
        a,
        "1.2.3.4 hostrow\n"
        "nameserver 8.8.8.8\n"
    );
    bool added = DnsResolverAddResolvPath(&r, &path);

    bool ok = added && VecLen(&r.nameservers) == 1 && ns_fmt_is(&r, 0, a, "8.8.8.8:53") && VecLen(&r.hosts) == 0;

    drop_temp(&path);
    DnsResolverDeinit(&r);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// Self-classifying AddPath runs BOTH parsers: a combined file contributes
// its hosts row AND its nameserver.
static bool test_addpath_self_classifying(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);

    DnsResolver r;
    resolver_init_empty(&r, a);
    Str path = write_temp(
        a,
        "1.2.3.4 hostrow\n"
        "nameserver 8.8.8.8\n"
    );
    bool added = DnsResolverAddPath(&r, &path);

    bool ok = added && VecLen(&r.hosts) == 1 && host_v4_is(find_host(&r.hosts, "hostrow"), 1, 2, 3, 4) &&
              VecLen(&r.nameservers) == 1 && ns_fmt_is(&r, 0, a, "8.8.8.8:53");

    drop_temp(&path);
    DnsResolverDeinit(&r);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// The (Zstr, len) overload copies exactly [path, path+len): trailing
// garbage past `len` must be ignored. We append "ZZZ" to the real path and
// pass the real length, so a wrong length opens the wrong (missing) file.
static bool test_addpath_zstr_len_exact(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);

    DnsResolver r;
    resolver_init_empty(&r, a);
    Str path = write_temp(a, "1.2.3.4 lenhost\n");
    u64 plen = StrLen(&path);

    Str smeared = StrInitFromStr(&path, a);
    StrPushBackMany(&smeared, "ZZZ", 3);

    bool added = DnsResolverAddHostsPath(&r, StrBegin(&smeared), plen);

    bool ok = added && VecLen(&r.hosts) == 1 && host_v4_is(find_host(&r.hosts, "lenhost"), 1, 2, 3, 4);

    StrDeinit(&smeared);
    drop_temp(&path);
    DnsResolverDeinit(&r);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// The (Zstr, len) overload must not leak its internal path copy.
static bool test_addpath_zstr_len_no_leak(void) {
    DebugAllocator alloc = DebugAllocatorInit();
    Allocator     *a     = ALLOCATOR_OF(&alloc);

    Str path = write_temp(a, "1.2.3.4 leakhost\n");
    u64 plen = StrLen(&path);

    size baseline = DebugAllocatorLiveCount(&alloc);

    DnsResolver r;
    resolver_init_empty(&r, a);
    DnsResolverAddHostsPath(&r, StrBegin(&path), plen);
    DnsResolverDeinit(&r);

    bool ok = DebugAllocatorLiveCount(&alloc) == baseline;

    drop_temp(&path);
    DebugAllocatorDeinit(&alloc);
    return ok;
}

// The plural AddHostsPaths applies the singular to each path in a Strs.
static bool test_addpath_plural_hosts(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);

    DnsResolver r;
    resolver_init_empty(&r, a);

    Str p0 = write_temp(a, "1.1.1.1 alpha\n");
    Str p1 = write_temp(a, "2.2.2.2 beta\n");

    Strs paths = VecInitT(paths, a);
    VecPushBackR(&paths, p0);
    VecPushBackR(&paths, p1);

    bool added = DnsResolverAddHostsPaths(&r, &paths);

    bool ok = added && VecLen(&r.hosts) == 2 && host_v4_is(find_host(&r.hosts, "alpha"), 1, 1, 1, 1) &&
              host_v4_is(find_host(&r.hosts, "beta"), 2, 2, 2, 2);

    FileRemove(&p0);
    FileRemove(&p1);
    VecForeachPtr(&paths, p) {
        StrDeinit(p);
    }
    VecDeinit(&paths);
    DnsResolverDeinit(&r);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// The plural AddResolvPaths applies the singular to each path in a Strs.
static bool test_addpath_plural_resolv(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);

    DnsResolver r;
    resolver_init_empty(&r, a);

    Str p0 = write_temp(a, "nameserver 10.0.0.1\n");
    Str p1 = write_temp(a, "nameserver 10.0.0.2\n");

    Strs paths = VecInitT(paths, a);
    VecPushBackR(&paths, p0);
    VecPushBackR(&paths, p1);

    bool added = DnsResolverAddResolvPaths(&r, &paths);

    bool ok = added && VecLen(&r.nameservers) == 2 && ns_fmt_is(&r, 0, a, "10.0.0.1:53") &&
              ns_fmt_is(&r, 1, a, "10.0.0.2:53");

    FileRemove(&p0);
    FileRemove(&p1);
    VecForeachPtr(&paths, p) {
        StrDeinit(p);
    }
    VecDeinit(&paths);
    DnsResolverDeinit(&r);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// The self-classifying plural AddPaths runs both parsers over each file.
static bool test_addpath_plural_self(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);

    DnsResolver r;
    resolver_init_empty(&r, a);

    Str p0 = write_temp(a, "1.2.3.4 hh\n");
    Str p1 = write_temp(a, "nameserver 9.9.9.9\n");

    Strs paths = VecInitT(paths, a);
    VecPushBackR(&paths, p0);
    VecPushBackR(&paths, p1);

    bool added = DnsResolverAddPaths(&r, &paths);

    bool ok = added && VecLen(&r.hosts) == 1 && host_v4_is(find_host(&r.hosts, "hh"), 1, 2, 3, 4) &&
              VecLen(&r.nameservers) == 1 && ns_fmt_is(&r, 0, a, "9.9.9.9:53");

    FileRemove(&p0);
    FileRemove(&p1);
    VecForeachPtr(&paths, p) {
        StrDeinit(p);
    }
    VecDeinit(&paths);
    DnsResolverDeinit(&r);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// Self-classifying AddPath via the (Zstr, len) overload returns success
// and runs both parsers.
static bool test_addpath_self_zstr_len(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);

    DnsResolver r;
    resolver_init_empty(&r, a);
    Str path = write_temp(
        a,
        "1.2.3.4 zlhost\n"
        "nameserver 8.8.8.8\n"
    );
    bool added = DnsResolverAddPath(&r, StrBegin(&path), StrLen(&path));

    bool ok = added && VecLen(&r.hosts) == 1 && host_v4_is(find_host(&r.hosts, "zlhost"), 1, 2, 3, 4) &&
              VecLen(&r.nameservers) == 1 && ns_fmt_is(&r, 0, a, "8.8.8.8:53");

    drop_temp(&path);
    DnsResolverDeinit(&r);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// AddResolvPath via the (Zstr, len) overload returns success and grows the
// nameserver table only.
static bool test_addpath_resolv_zstr_len(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);

    DnsResolver r;
    resolver_init_empty(&r, a);
    Str  path  = write_temp(a, "nameserver 4.3.2.1\n");
    bool added = DnsResolverAddResolvPath(&r, StrBegin(&path), StrLen(&path));

    bool ok = added && VecLen(&r.nameservers) == 1 && ns_fmt_is(&r, 0, a, "4.3.2.1:53") && VecLen(&r.hosts) == 0;

    drop_temp(&path);
    DnsResolverDeinit(&r);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// A NULL resolver / NULL path is rejected by the Add*Path guards.
static bool test_addpath_null_guards(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);

    DnsResolver r;
    resolver_init_empty(&r, a);

    bool refused = !dns_resolver_add_hosts_path_str(NULL, NULL) && !dns_resolver_add_resolv_path_str(NULL, NULL) &&
                   !dns_resolver_add_path_str(NULL, NULL) && !dns_resolver_add_paths(&r, NULL) &&
                   !dns_resolver_add_hosts_paths(&r, NULL) && !dns_resolver_add_resolv_paths(&r, NULL);

    DnsResolverDeinit(&r);
    DefaultAllocatorDeinit(&alloc);
    return refused;
}

// ===========================================================================
// dns_resolve_5 / dns_resolve_4 / init / deinit -- crafted-table API tests.
// ===========================================================================

static bool test_resolve5_single_exact(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);

    DnsResolver r;
    resolver_init_crafted(&r, a);

    DnsAddrs out = VecInitT(out, a);
    bool     got = dns_resolve_5_zstr(&r, "single", 80, SOCKET_KIND_TCP, &out);

    bool ok = got && VecLen(&out) == 1 && v4_is(VecPtrAt(&out, 0), a, "1.2.3.4:80");

    VecDeinit(&out);
    DnsResolverDeinit(&r);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

static bool test_resolve5_multi_two_in_order(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);

    DnsResolver r;
    resolver_init_crafted(&r, a);

    DnsAddrs out = VecInitT(out, a);
    bool     got = dns_resolve_5_zstr(&r, "multi", 1234, SOCKET_KIND_TCP, &out);

    bool ok = got && VecLen(&out) == 2 && v4_is(VecPtrAt(&out, 0), a, "5.6.7.8:1234") &&
              v4_is(VecPtrAt(&out, 1), a, "9.10.11.12:1234");

    VecDeinit(&out);
    DnsResolverDeinit(&r);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

static bool test_resolve5_both_families(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);

    DnsResolver r;
    resolver_init_crafted(&r, a);

    DnsAddrs out = VecInitT(out, a);
    bool     got = dns_resolve_5_zstr(&r, "both", 443, SOCKET_KIND_TCP, &out);

    bool ok = got && VecLen(&out) == 2 && v4_is(VecPtrAt(&out, 0), a, "1.1.1.1:443") &&
              v6_is(VecPtrAt(&out, 1), a, "[2606::1]:443");

    VecDeinit(&out);
    DnsResolverDeinit(&r);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

static bool test_resolve5_v6_exact(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);

    DnsResolver r;
    resolver_init_crafted(&r, a);

    DnsAddrs out = VecInitT(out, a);
    bool     got = dns_resolve_5_zstr(&r, "v6name", 53, SOCKET_KIND_TCP, &out);

    bool ok = got && VecLen(&out) == 1 && v6_is(VecPtrAt(&out, 0), a, "[::1]:53");

    VecDeinit(&out);
    DnsResolverDeinit(&r);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

static bool test_resolve5_normalized_match(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);

    DnsResolver r;
    resolver_init_crafted(&r, a);

    DnsAddrs out = VecInitT(out, a);
    bool     got = dns_resolve_5_zstr(&r, "SINGLE.", 80, SOCKET_KIND_TCP, &out);

    bool ok = got && VecLen(&out) == 1 && v4_is(VecPtrAt(&out, 0), a, "1.2.3.4:80");

    VecDeinit(&out);
    DnsResolverDeinit(&r);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

static bool test_resolve5_miss_no_ns(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);

    DnsResolver r;
    resolver_init_crafted(&r, a);

    DnsAddrs out = VecInitT(out, a);
    bool     got = dns_resolve_5_zstr(&r, "absent", 80, SOCKET_KIND_TCP, &out);

    bool ok = !got && VecLen(&out) == 0;

    VecDeinit(&out);
    DnsResolverDeinit(&r);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

static bool test_resolve5_overlong_rejected(void) {
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
    DnsResolverDeinit(&r);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

static bool test_resolve5_exact_256_rejected(void) {
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
    DnsResolverDeinit(&r);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

static bool test_resolve5_under_cap_processed(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);

    DnsResolver r;
    resolver_init_crafted(&r, a);

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
    DnsResolverDeinit(&r);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

static bool test_resolve5_empty_name_row_ignored(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);

    DnsResolver r;
    resolver_init_empty(&r, a);
    hosts_push_v4(&r, "", 7, 7, 7, 7);

    DnsAddrs out = VecInitT(out, a);
    bool     got = dns_resolve_5_zstr(&r, "", 80, SOCKET_KIND_TCP, &out);

    bool ok = !got && VecLen(&out) == 0;

    VecDeinit(&out);
    DnsResolverDeinit(&r);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

static bool test_resolve5_str_delegates(void) {
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
    DnsResolverDeinit(&r);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

static bool test_resolve4_vec_single(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);

    DnsResolver r;
    resolver_init_crafted(&r, a);

    DnsAddrs out = VecInitT(out, a);
    bool     got = dns_resolve_4_vec_zstr(&r, "single:80", SOCKET_KIND_TCP, &out);

    bool ok = got && VecLen(&out) == 1 && v4_is(VecPtrAt(&out, 0), a, "1.2.3.4:80");

    VecDeinit(&out);
    DnsResolverDeinit(&r);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

static bool test_resolve4_vec_multi(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);

    DnsResolver r;
    resolver_init_crafted(&r, a);

    DnsAddrs out = VecInitT(out, a);
    bool     got = dns_resolve_4_vec_zstr(&r, "multi:9090", SOCKET_KIND_TCP, &out);

    bool ok = got && VecLen(&out) == 2 && v4_is(VecPtrAt(&out, 0), a, "5.6.7.8:9090") &&
              v4_is(VecPtrAt(&out, 1), a, "9.10.11.12:9090");

    VecDeinit(&out);
    DnsResolverDeinit(&r);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

static bool test_resolve4_vec_numeric(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);

    DnsResolver r;
    resolver_init_crafted(&r, a);

    DnsAddrs out = VecInitT(out, a);
    bool     got = dns_resolve_4_vec_zstr(&r, "203.0.113.7:9999", SOCKET_KIND_TCP, &out);

    bool ok = got && VecLen(&out) == 1 && v4_is(VecPtrAt(&out, 0), a, "203.0.113.7:9999");

    VecDeinit(&out);
    DnsResolverDeinit(&r);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

static bool test_resolve4_vec_no_port(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);

    DnsResolver r;
    resolver_init_crafted(&r, a);

    DnsAddrs out = VecInitT(out, a);
    bool     got = dns_resolve_4_vec_zstr(&r, "single", SOCKET_KIND_TCP, &out);

    bool ok = !got && VecLen(&out) == 0;

    VecDeinit(&out);
    DnsResolverDeinit(&r);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

static bool test_resolve4_vec_empty_port(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);

    DnsResolver r;
    resolver_init_crafted(&r, a);

    DnsAddrs out = VecInitT(out, a);
    bool     got = dns_resolve_4_vec_zstr(&r, "single:", SOCKET_KIND_TCP, &out);

    bool ok = !got && VecLen(&out) == 0;

    VecDeinit(&out);
    DnsResolverDeinit(&r);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

static bool test_resolve4_vec_bad_port(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);

    DnsResolver r;
    resolver_init_crafted(&r, a);

    DnsAddrs out = VecInitT(out, a);
    bool     got = dns_resolve_4_vec_zstr(&r, "single:9z9", SOCKET_KIND_TCP, &out);

    bool ok = !got && VecLen(&out) == 0;

    VecDeinit(&out);
    DnsResolverDeinit(&r);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

static bool test_resolve4_vec_max_port(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);

    DnsResolver r;
    resolver_init_crafted(&r, a);

    DnsAddrs out = VecInitT(out, a);
    bool     got = dns_resolve_4_vec_zstr(&r, "single:65535", SOCKET_KIND_TCP, &out);

    bool ok = got && VecLen(&out) == 1 && v4_is(VecPtrAt(&out, 0), a, "1.2.3.4:65535");

    VecDeinit(&out);
    DnsResolverDeinit(&r);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

static bool test_resolve4_vec_host_256_rejected(void) {
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
    DnsResolverDeinit(&r);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

static bool test_resolve4_vec_absent(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);

    DnsResolver r;
    resolver_init_crafted(&r, a);

    DnsAddrs out = VecInitT(out, a);
    bool     got = dns_resolve_4_vec_zstr(&r, "absent:80", SOCKET_KIND_TCP, &out);

    bool ok = !got && VecLen(&out) == 0;

    VecDeinit(&out);
    DnsResolverDeinit(&r);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

static bool test_resolve4_vec_str_delegates(void) {
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
    DnsResolverDeinit(&r);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

static bool test_resolve4_one_single(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);

    DnsResolver r;
    resolver_init_crafted(&r, a);

    SocketAddr one;
    bool       got = dns_resolve_4_one_zstr(&r, "single:80", SOCKET_KIND_TCP, &one);

    bool ok = got && v4_is(&one, a, "1.2.3.4:80");

    DnsResolverDeinit(&r);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

static bool test_resolve4_one_first_of_many(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);

    DnsResolver r;
    resolver_init_crafted(&r, a);

    SocketAddr one;
    bool       got = dns_resolve_4_one_zstr(&r, "multi:80", SOCKET_KIND_TCP, &one);

    bool ok = got && v4_is(&one, a, "5.6.7.8:80");

    DnsResolverDeinit(&r);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

static bool test_resolve4_one_miss(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);

    DnsResolver r;
    resolver_init_crafted(&r, a);

    SocketAddr one;
    bool       got = dns_resolve_4_one_zstr(&r, "absent:80", SOCKET_KIND_TCP, &one);

    bool ok = !got;

    DnsResolverDeinit(&r);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

static bool test_resolve4_one_str_delegates(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);

    DnsResolver r;
    resolver_init_crafted(&r, a);

    Str        spec = StrInitFromZstr("multi:80", a);
    SocketAddr one;
    bool       got = dns_resolve_4_one_str(&r, &spec, SOCKET_KIND_TCP, &one);

    bool ok = got && v4_is(&one, a, "5.6.7.8:80");

    StrDeinit(&spec);
    DnsResolverDeinit(&r);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

static bool test_init_defaults(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);

    DnsResolver r;
    bool        ok = dns_resolver_init(&r, a);

    ok = ok && r.timeout_ms == 5000 && r.retries == 2 && r.allocator == a;

    DnsResolverDeinit(&r);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

static bool test_init_loads_hosts(void) {
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

// A real init populates the hosts table from disk (every /etc/hosts has
// `localhost`). Asserted directly on `r.hosts` so the parse_hosts_path
// call cannot be masked by a network fallback for "localhost".
static bool test_init_loads_hosts_table(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);

    DnsResolver r;
    bool        ok = dns_resolver_init(&r, a);

    bool have_hosts = VecLen(&r.hosts) > 0;

    DnsResolverDeinit(&r);
    DefaultAllocatorDeinit(&alloc);
    return ok && have_hosts;
}

static bool test_init_loads_nameservers(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);

    DnsResolver r;
    bool        ok = dns_resolver_init(&r, a);

    bool have_ns = VecLen(&r.nameservers) > 0;

    DnsResolverDeinit(&r);
    DefaultAllocatorDeinit(&alloc);
    return ok && have_ns;
}

static bool test_init_null_alloc(void) {
    DnsResolver r;
    bool        got = dns_resolver_init(&r, NULL);
    return !got;
}

static bool test_deinit_no_leak(void) {
    DebugAllocator alloc    = DebugAllocatorInit();
    Allocator     *a        = ALLOCATOR_OF(&alloc);
    size           baseline = DebugAllocatorLiveCount(&alloc);

    DnsResolver r;
    resolver_init_crafted(&r, a);

    bool grew = DebugAllocatorLiveCount(&alloc) > baseline;

    DnsResolverDeinit(&r);

    bool back = DebugAllocatorLiveCount(&alloc) == baseline;
    bool ok   = grew && back && r.allocator == NULL;

    DebugAllocatorDeinit(&alloc);
    return ok;
}

static bool test_init_real_disk_no_leak(void) {
    DebugAllocator alloc    = DebugAllocatorInit();
    Allocator     *a        = ALLOCATOR_OF(&alloc);
    size           baseline = DebugAllocatorLiveCount(&alloc);

    DnsResolver r;
    bool        ok = dns_resolver_init(&r, a);

    DnsResolverDeinit(&r);

    bool back = DebugAllocatorLiveCount(&alloc) == baseline;

    DebugAllocatorDeinit(&alloc);
    return ok && back;
}

// ---------------------------------------------------------------------------

int main(void) {
    WriteFmt("[INFO] Starting SysDns tests\n\n");

    TestFunction tests[] = {
        // public resolve API against live /etc files
        test_dns_resolve_localhost_from_hosts,
        test_dns_resolve_hosts_case_insensitive,
        test_dns_resolve_hosts_trailing_dot,
        test_dns_resolve_spec_numeric_v4,
        test_dns_resolve_spec_numeric_v6,
        test_dns_resolve_spec_hostname,
        test_dns_resolve_spec_single_addr,
        test_dns_resolve_spec_no_port,
        test_dns_resolve_spec_bad_port,

        // /etc/hosts parser
        test_hosts_single_v4,
        test_hosts_aliases,
        test_hosts_comments_and_blanks,
        test_hosts_inline_comment,
        test_hosts_inline_comment_no_space,
        test_hosts_leading_and_tab_whitespace,
        test_hosts_case_lowered,
        test_hosts_ascii_lower_boundaries,
        test_hosts_no_trailing_newline,
        test_hosts_comment_eof_no_newline,
        test_hosts_multiline_no_desync,
        test_hosts_ipv6,
        test_hosts_non_ip_line_rejected,
        test_hosts_ip_only_no_name,
        test_hosts_overlong_first_token,
        test_hosts_reject_skips_rest_of_line,
        test_hosts_missing_file_tolerated,
        test_hosts_multichunk_slurp,

        // /etc/resolv.conf parser
        test_resolv_basic,
        test_resolv_v6,
        test_resolv_hash_comment_skipped,
        test_resolv_semicolon_comment_skipped,
        test_resolv_search_not_ns,
        test_resolv_keyword_needs_sep,
        test_resolv_tab_sep,
        test_resolv_leading_space,
        test_resolv_bad_ip,
        test_resolv_trailing_comment,
        test_resolv_empty_ip,
        test_resolv_order,
        test_resolv_no_trailing_newline,

        // normalize_hostname (behavioural)
        test_norm_query_case_and_bounds,
        test_norm_query_trailing_dots,
        test_norm_query_interior_dots_kept,
        test_norm_query_all_dots_no_match,

        // sockaddr_v4 / sockaddr_v6
        test_wire_v4_fields,
        test_wire_v6_fields,

        // DnsResolverAddNameserver
        test_ns_add_nameserver,
        test_ns_add_nameserver_null,

        // Add*Path plumbing
        test_addpath_hosts_only,
        test_addpath_resolv_only,
        test_addpath_self_classifying,
        test_addpath_zstr_len_exact,
        test_addpath_zstr_len_no_leak,
        test_addpath_plural_hosts,
        test_addpath_plural_resolv,
        test_addpath_plural_self,
        test_addpath_self_zstr_len,
        test_addpath_resolv_zstr_len,
        test_addpath_null_guards,

        // resolve API + init/deinit (crafted table)
        test_resolve5_single_exact,
        test_resolve5_multi_two_in_order,
        test_resolve5_both_families,
        test_resolve5_v6_exact,
        test_resolve5_normalized_match,
        test_resolve5_miss_no_ns,
        test_resolve5_overlong_rejected,
        test_resolve5_exact_256_rejected,
        test_resolve5_under_cap_processed,
        test_resolve5_empty_name_row_ignored,
        test_resolve5_str_delegates,
        test_resolve4_vec_single,
        test_resolve4_vec_multi,
        test_resolve4_vec_numeric,
        test_resolve4_vec_no_port,
        test_resolve4_vec_empty_port,
        test_resolve4_vec_bad_port,
        test_resolve4_vec_max_port,
        test_resolve4_vec_host_256_rejected,
        test_resolve4_vec_absent,
        test_resolve4_vec_str_delegates,
        test_resolve4_one_single,
        test_resolve4_one_first_of_many,
        test_resolve4_one_miss,
        test_resolve4_one_str_delegates,
        test_init_defaults,
        test_init_loads_hosts,
        test_init_loads_hosts_table,
        test_init_loads_nameservers,
        test_init_null_alloc,
        test_deinit_no_leak,
        test_init_real_disk_no_leak,
    };

    return run_test_suite(tests, sizeof(tests) / sizeof(tests[0]), NULL, 0, "SysDns");
}
