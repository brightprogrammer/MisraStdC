/// file      : sys/dns.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// In-tree DNS resolver. Reads `/etc/hosts` and `/etc/resolv.conf` at
/// init time, then resolves hostnames either out of the local hosts
/// table or via UDP queries to one of the configured nameservers.
///
/// The resolver owns no process-wide state -- each `DnsResolver` is a
/// value the caller stack-allocates, just like every other typed
/// container in MisraStdC.
///
/// v1 scope:
///   - A + AAAA lookups
///   - `/etc/hosts` lookup (no-network fast path)
///   - UDP transport only (no TCP fallback when responses are truncated)
///   - Sequential nameserver iteration (no parallel queries)
///   - No in-memory cache
///   - No CNAME chasing -- if the nameserver only returns CNAME we
///     surface it as a parse error; resolvers we talk to (1.1.1.1,
///     8.8.8.8, systemd-resolved) always return the followed A/AAAA
///     alongside the CNAME, so this is fine in practice
///
/// All of the above are tracked as follow-ups in FUTURE-PLANS.

#ifndef MISRA_SYS_DNS_H
#define MISRA_SYS_DNS_H

#include <Misra/Std/Allocator.h>
#include <Misra/Std/Container/Str.h>
#include <Misra/Std/Container/Vec.h>
#include <Misra/Sys/Socket.h>
#include <Misra/Types.h>

#ifdef __cplusplus
extern "C" {
#endif

    ///
    /// One row from `/etc/hosts` -- a hostname (or alias) paired with
    /// its IP address. We store IPv4 addresses in the first four bytes
    /// of `ip[]`; IPv6 uses all 16.
    ///
    typedef struct HostsEntry {
        Str  name;
        u8   ip[16];
        bool is_ipv6;
    } HostsEntry;

    typedef Vec(HostsEntry) HostsTable;
    typedef Vec(SocketAddr) DnsAddrs;

    ///
    /// In-tree resolver state. Created via `DnsResolverInit` (which
    /// reads `/etc/hosts` and `/etc/resolv.conf` from disk), torn down
    /// via `DnsResolverDeinit`. No globals, no shared cache; create one
    /// per subsystem that needs resolution.
    ///
    typedef struct DnsResolver {
        Allocator *alloc;
        HostsTable hosts;
        DnsAddrs   nameservers; // each is a UDP sockaddr with port 53
        u32        timeout_ms;
        u32        retries;
    } DnsResolver;

    ///
    /// Read `/etc/hosts` + `/etc/resolv.conf` into `out`. Missing or
    /// unreadable files are tolerated (the corresponding table stays
    /// empty); a resolver with no hosts and no nameservers will only
    /// fail-fast at `DnsResolve` time. Default `timeout_ms = 5000`,
    /// `retries = 2`.
    ///
    /// SUCCESS : Returns true. `out` is populated.
    /// FAILURE : Returns false on allocator OOM.
    ///
    bool DnsResolverInit(DnsResolver *out, Allocator *alloc);

    ///
    /// Release every owned string / Vec. Safe on a partially-initialised
    /// resolver.
    ///
    void DnsResolverDeinit(DnsResolver *self);

    ///
    /// Resolve a hostname to one or more `SocketAddr`s.
    ///
    /// Lookup order:
    ///   1. `/etc/hosts` first (case-insensitive, no network)
    ///   2. UDP query against each nameserver in turn, up to `retries`
    ///      attempts per server with `timeout_ms` per attempt; the
    ///      first server that returns NOERROR wins
    ///
    /// `out` is appended to (caller manages clearing). Each entry has
    /// the supplied `port` and a sockaddr appropriate to `kind` (which
    /// influences the protocol byte in the sockaddr, not the resolution
    /// strategy -- we always look up both A and AAAA).
    ///
    /// hostname[in] : NUL-terminated; trailing dot tolerated.
    /// port[in]     : Port number to stamp on every returned address.
    /// kind[in]     : `SOCKET_KIND_TCP` or `SOCKET_KIND_UDP`.
    /// out[in,out]  : Vec to append results to. Stays untouched on failure.
    ///
    /// SUCCESS : Returns true. `out` has at least one new entry.
    /// FAILURE : Returns false. Logs the failure cause (no nameservers
    ///           configured, no answer from any nameserver, NXDOMAIN,
    ///           transport error, response with no A/AAAA).
    ///
    bool DnsResolve(DnsResolver *self, const char *hostname, u16 port, SocketKind kind, DnsAddrs *out);

#ifdef __cplusplus
}
#endif

#endif // MISRA_SYS_DNS_H
