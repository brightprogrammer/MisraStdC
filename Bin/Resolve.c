/// file      : bin/resolve.c
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// `resolve` is a small CLI on top of `Sys/Dns`. Usage:
///
///     resolve <hostname>
///
/// Walks /etc/hosts first, then the nameservers from /etc/resolv.conf,
/// prints one IPv4/IPv6 per line. Useful for debugging the in-tree
/// resolver and as a scriptable companion to `getent hosts`.

#include <Misra.h>
#include <Misra/Std/Allocator/Default.h>
#include <Misra/Std/ArgParse.h>
#include <Misra/Sys/Dns.h>
#include <Misra/Sys/Socket.h>

int main(int argc, char **argv) {
    Scope(alloc, DefaultAllocator) {
        const char *hostname = NULL;

        ArgParse ap = ArgParseInit("resolve", "look up a hostname via /etc/hosts and DNS");
        ArgPositional(&ap, "hostname", &hostname, "name to resolve");

        ArgRun rc = ArgParseRun(&ap, argc, argv);
        ArgParseDeinit(&ap);
        if (rc != ARG_RUN_OK) {
            return rc == ARG_RUN_HELP ? 0 : 1;
        }

        DnsResolver r;
        if (!DnsResolverInit(&r, alloc)) {
            LOG_ERROR("failed to init resolver");
            return 1;
        }

        DnsAddrs addrs = VecInitT(addrs, alloc);
        if (!DnsResolve(&r, hostname, 0, SOCKET_KIND_TCP, &addrs)) {
            DnsResolverDeinit(&r);
            return 1;
        }

        VecForeachPtr(&addrs, a) {
            Str s = SocketAddrFormat(a, alloc);
            // Strip the trailing ":0" since we resolve with port=0; keep
            // bracket form on v6 to round-trip through SocketAddrParse.
            if (s.length >= 2 && s.data[s.length - 1] == '0' && s.data[s.length - 2] == ':') {
                s.length         -= 2;
                s.data[s.length]  = '\0';
            }
            WriteFmtLn("{}", s);
            StrDeinit(&s);
        }

        VecDeinit(&addrs);
        DnsResolverDeinit(&r);
    }

    return 0;
}
