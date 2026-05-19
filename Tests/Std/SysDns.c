#include <Misra/Std/Allocator/Default.h>
#include <Misra/Std/Zstr.h>
#include <Misra/Std/Log.h>
#include <Misra/Sys/Dns.h>
#include <Misra/Sys/Socket.h>

#include "../Util/TestRunner.h"

// /etc/hosts always has `localhost` -> 127.0.0.1 and (on most distros)
// `::1 localhost`. Resolving "localhost" should hit the hosts table
// without going to the network.
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

    // At minimum one address should come back (127.0.0.1). The exact
    // count varies by distro -- some only list v4, some both.
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

// Trailing dot on a hostname is the FQDN form -- should resolve the
// same as without the dot.
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

// 4-arg overload (vec): numeric IPv4 spec must short-circuit through
// SocketAddrParse. Verify via SocketAddrFormat round-trip rather than
// poking at SocketAddr's opaque `raw[]` bytes.
static bool test_dns_resolve_spec_numeric_v4(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);

    DnsResolver r;
    DnsResolverInit(&r, a);

    DnsAddrs out = VecInitT(out, a);
    bool     got = DnsResolve(&r, "203.0.113.7:9999", SOCKET_KIND_TCP, &out);

    bool ok = got && out.length == 1 && out.data[0].family == SOCKET_FAMILY_INET;
    if (ok) {
        Str s = SocketAddrFormat(&out.data[0], a);
        ok    = (s.length > 0) && ZstrCompare(s.data, "203.0.113.7:9999") == 0;
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

    bool ok = got && out.length == 1 && out.data[0].family == SOCKET_FAMILY_INET6;
    if (ok) {
        Str s = SocketAddrFormat(&out.data[0], a);
        // SocketAddrFormat emits the bracketed form for IPv6.
        ok = (s.length > 0) && ZstrCompare(s.data, "[::1]:443") == 0;
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

    // Sanity: each returned address should format with ":53" suffix.
    bool ok = got && out.length > 0;
    if (ok) {
        VecForeachPtr(&out, ad) {
            Str s = SocketAddrFormat(ad, a);
            u64 L = s.length;
            if (L < 3 || s.data[L - 1] != '3' || s.data[L - 2] != '5' || s.data[L - 3] != ':') {
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

// 4-arg overload (single-addr form): pass `SocketAddr *` instead of
// `DnsAddrs *` and the macro routes to DnsResolve_4_one.
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
        ok    = (s.length > 0) && ZstrCompare(s.data, "127.0.0.1:80") == 0;
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

    bool ok = !got && out.length == 0;

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

int main(void) {
    WriteFmt("[INFO] Starting SysDns tests\n\n");

    TestFunction tests[] = {
        test_dns_resolve_localhost_from_hosts,
        test_dns_resolve_hosts_case_insensitive,
        test_dns_resolve_hosts_trailing_dot,
        test_dns_resolve_spec_numeric_v4,
        test_dns_resolve_spec_numeric_v6,
        test_dns_resolve_spec_hostname,
        test_dns_resolve_spec_single_addr,
        test_dns_resolve_spec_no_port,
        test_dns_resolve_spec_bad_port,
    };

    return run_test_suite(tests, sizeof(tests) / sizeof(tests[0]), NULL, 0, "SysDns");
}
