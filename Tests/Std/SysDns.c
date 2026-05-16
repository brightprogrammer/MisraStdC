#include <Misra/Std/Allocator/Default.h>
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

int main(void) {
    WriteFmt("[INFO] Starting SysDns tests\n\n");

    TestFunction tests[] = {
        test_dns_resolve_localhost_from_hosts,
        test_dns_resolve_hosts_case_insensitive,
        test_dns_resolve_hosts_trailing_dot,
    };

    return run_test_suite(tests, sizeof(tests) / sizeof(tests[0]), NULL, 0, "SysDns");
}
