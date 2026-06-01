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
    /// TAGS: Dns, Type, Hosts
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
    /// TAGS: Dns, Type, Resolver
    ///
    typedef struct DnsResolver {
        Allocator *allocator;
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
    /// TAGS: Dns, Resolve, Init
    ///
    bool dns_resolver_init(DnsResolver *out, Allocator *alloc);
#define DnsResolverInit(...)          OVERLOAD(DnsResolverInit, __VA_ARGS__)
#define DnsResolverInit_1(out)        dns_resolver_init((out), MisraScope)
#define DnsResolverInit_2(out, alloc) dns_resolver_init((out), ALLOCATOR_OF(alloc))

    ///
    /// Release every owned string / Vec. Safe on a partially-initialised
    /// resolver.
    ///
    /// SUCCESS : Returns to the caller. `self` is zeroed.
    /// FAILURE : Function cannot fail.
    ///
    /// TAGS: Dns, Resolve, Deinit, Init
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
    /// hostname[in] : Hostname to resolve. Prefer `Str *`; `Zstr`
    ///                accepted. Trailing dot tolerated.
    /// port[in]     : Port number to stamp on every returned address.
    /// kind[in]     : `SOCKET_KIND_TCP` or `SOCKET_KIND_UDP`.
    /// out[in,out]  : Vec to append results to. Stays untouched on failure.
    ///
    /// SUCCESS : Returns true. `out` has at least one new entry.
    /// FAILURE : Returns false. Logs the failure cause (no nameservers
    ///           configured, no answer from any nameserver, NXDOMAIN,
    ///           transport error, response with no A/AAAA).
    ///
    /// TAGS: Dns, Resolve, API
    ///
    bool dns_resolve_5_zstr(DnsResolver *self, Zstr hostname, u16 port, SocketKind kind, DnsAddrs *out);
    bool dns_resolve_5_str(DnsResolver *self, const Str *hostname, u16 port, SocketKind kind, DnsAddrs *out);
#define DnsResolve_5(self, hostname, port, kind, out)                                                                  \
    _Generic(                                                                                                          \
        (hostname),                                                                                                    \
        Str *: dns_resolve_5_str((self), (const Str *)(hostname), (port), (kind), (out)),                              \
        Zstr: dns_resolve_5_zstr((self), (Zstr)(hostname), (port), (kind), (out)),                                     \
        char *: dns_resolve_5_zstr((self), (Zstr)(hostname), (port), (kind), (out))                                    \
    )

    ///
    /// Spec-based overload (vec form). Accepts a single `"host:port"`
    /// string and appends every matching `SocketAddr` to `out`.
    ///
    /// Tries `SocketAddrParse` first (numeric IPv4 `127.0.0.1:8080` and
    /// bracketed IPv6 `[::1]:8080` short-circuit here with no network
    /// I/O). Otherwise splits `spec` on the last `:` to get host + port
    /// and dispatches to `DnsResolve_5`.
    ///
    /// SUCCESS : Returns true. `out` has at least one new entry.
    /// FAILURE : Returns false. Logs the failure cause.
    ///
    /// TAGS: Dns, Resolve, API
    ///
    bool dns_resolve_4_vec_zstr(DnsResolver *self, Zstr spec, SocketKind kind, DnsAddrs *out);
    bool dns_resolve_4_vec_str(DnsResolver *self, const Str *spec, SocketKind kind, DnsAddrs *out);
#define DnsResolve_4_vec(self, spec, kind, out)                                                                        \
    _Generic(                                                                                                          \
        (spec),                                                                                                        \
        Str *: dns_resolve_4_vec_str((self), (const Str *)(spec), (kind), (out)),                                      \
        Zstr: dns_resolve_4_vec_zstr((self), (Zstr)(spec), (kind), (out)),                                             \
        char *: dns_resolve_4_vec_zstr((self), (Zstr)(spec), (kind), (out))                                            \
    )

    ///
    /// Spec-based overload (single-addr form). Same parse path as the
    /// vec form, but only the first matching `SocketAddr` is written
    /// back -- the "resolve and pick first" idiom in one call so
    /// callers needing exactly one address (typical for `SocketConnect`
    /// / `ListenerOpen`) don't have to manage a throwaway Vec.
    ///
    /// SUCCESS : Returns true; `out` populated.
    /// FAILURE : Returns false; `out` untouched.
    ///
    /// TAGS: Dns, Resolve, API
    ///
    bool dns_resolve_4_one_zstr(DnsResolver *self, Zstr spec, SocketKind kind, SocketAddr *out);
    bool dns_resolve_4_one_str(DnsResolver *self, const Str *spec, SocketKind kind, SocketAddr *out);

    ///
    /// `DnsResolve` dispatches by argument count. The 4-arg form
    /// additionally dispatches on the `out` parameter type:
    /// `DnsAddrs *` selects the vec form, `SocketAddr *` selects the
    /// single-addr form.
    ///
    /// TAGS: Dns, Resolve, API
    ///
#define DnsResolve(...) OVERLOAD(DnsResolve, __VA_ARGS__)
#define DnsResolve_4(self, spec, kind, out)                                                                            \
    _Generic(                                                                                                          \
        (out),                                                                                                         \
        DnsAddrs *: _Generic(                                                                                          \
            (spec),                                                                                                    \
            Str *: dns_resolve_4_vec_str((self), (const Str *)(spec), (kind), (DnsAddrs *)(out)),                      \
            Zstr: dns_resolve_4_vec_zstr((self), (Zstr)(spec), (kind), (DnsAddrs *)(out)),                             \
            char *: dns_resolve_4_vec_zstr((self), (Zstr)(spec), (kind), (DnsAddrs *)(out))                            \
        ),                                                                                                             \
        SocketAddr *: _Generic(                                                                                        \
            (spec),                                                                                                    \
            Str *: dns_resolve_4_one_str((self), (const Str *)(spec), (kind), (SocketAddr *)(out)),                    \
            Zstr: dns_resolve_4_one_zstr((self), (Zstr)(spec), (kind), (SocketAddr *)(out)),                           \
            char *: dns_resolve_4_one_zstr((self), (Zstr)(spec), (kind), (SocketAddr *)(out))                          \
        )                                                                                                              \
    )

#ifdef __cplusplus
}
#endif

#endif // MISRA_SYS_DNS_H
