/// file      : sys/dns.c
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// In-tree DNS resolver: reads /etc/hosts and /etc/resolv.conf at
/// init, then resolves hostnames via the local table first and the
/// configured nameservers (UDP) second. Wire-format work goes through
/// Parsers/Dns; transport goes through Sys/Socket.

#include <Misra/Sys/Dns.h>
#include <Misra/Std/Zstr.h>

#include <Misra/Parsers/Dns.h>
#include <Misra/Std/Allocator/Arena.h>
#include <Misra/Std/Allocator/Heap.h>
#include <Misra/Std/File.h>
#include <Misra/Std/Log.h>
#include <Misra/Std/Memory.h>
#include <Misra/Std/Prng.h>
#include <Misra/Sys/Socket.h>

#include "../_Syscall.h"

// Platform default paths for the two config files we read. POSIX
// (Linux + macOS): the well-known /etc paths. Windows: the same files
// live under `%SystemRoot%\System32\drivers\etc`, which historically
// resolves to `C:\Windows\System32\drivers\etc`. Windows machines
// configured via DHCP usually leave `hosts` and `resolv.conf` empty
// (real nameservers live in the registry / iphlpapi), so the DNS
// path will often have an empty nameserver table on Windows -- users
// who need lookups should populate `resolver->nameservers` directly
// or wait for the iphlpapi-backed reader in a follow-up.
#if PLATFORM_WINDOWS
#    define HOSTS_FILE_PATH       "C:\\Windows\\System32\\drivers\\etc\\hosts"
#    define RESOLV_CONF_FILE_PATH "C:\\Windows\\System32\\drivers\\etc\\resolv.conf"
// Pull in Winsock's `sockaddr_in` / `sockaddr_in6` so the sockaddr_v4 /
// sockaddr_v6 builders below compile. The same headers are used inside
// Sys/Socket.c -- order (winsock2.h before windows.h) is the documented
// requirement.
#    ifndef WIN32_LEAN_AND_MEAN
#        define WIN32_LEAN_AND_MEAN
#    endif
#    include <winsock2.h>
#    include <ws2tcpip.h>
#else
#    define HOSTS_FILE_PATH       "/etc/hosts"
#    define RESOLV_CONF_FILE_PATH "/etc/resolv.conf"
#    include <netinet/in.h>
#    include <sys/socket.h>
#endif

#include "_IpParse.h"

// ---------------------------------------------------------------------------
// SocketAddr builders
// ---------------------------------------------------------------------------

static SocketAddr sockaddr_v4(const u8 ip[4], u16 port) {
    SocketAddr          a  = {0};
    struct sockaddr_in *sa = (struct sockaddr_in *)a.raw;
    sa->sin_family         = AF_INET;
    sa->sin_port           = FROM_BIG_ENDIAN2(port);
    MemCopy(&sa->sin_addr.s_addr, ip, 4);
    a.length = (u32)sizeof(struct sockaddr_in);
    a.family = SOCKET_FAMILY_INET;
    return a;
}

static SocketAddr sockaddr_v6(const u8 ip[16], u16 port) {
    SocketAddr           a  = {0};
    struct sockaddr_in6 *sa = (struct sockaddr_in6 *)a.raw;
    sa->sin6_family         = AF_INET6;
    sa->sin6_port           = FROM_BIG_ENDIAN2(port);
    MemCopy(sa->sin6_addr.s6_addr, ip, 16);
    a.length = (u32)sizeof(struct sockaddr_in6);
    a.family = SOCKET_FAMILY_INET6;
    return a;
}

// ---------------------------------------------------------------------------
// Text helpers
// ---------------------------------------------------------------------------

static bool is_hspace(char c) {
    return c == ' ' || c == '\t';
}

static void ascii_lower(u8 *p, u64 n) {
    for (u64 i = 0; i < n; ++i) {
        u8 c = p[i];
        if (c >= 'A' && c <= 'Z') {
            p[i] = (u8)(c + ('a' - 'A'));
        }
    }
}

// Slurp a whole File into `out`. Caller owns `out`; allocator inline
// on the Str. Returns false on read error; missing file returns true
// with an empty Str.
static bool slurp_file(Zstr path, Str *out) {
    File f = FileOpen(path, "rb");
    if (!FileIsOpen(&f)) {
        // Missing config file is fine -- resolver just won't know about it.
        return true;
    }
    bool ok = true;
    StrInitStack(chunk, 4096) {
        char *data = StrBegin(&chunk);
        for (;;) {
            i64 n = FileRead(&f, data, 4096);
            if (n < 0) {
                ok = false;
                break;
            }
            if (n == 0)
                break;
            StrResize(&chunk, (size)n);
            StrMergeR(out, &chunk);
        }
    }
    FileClose(&f);
    return ok;
}

// ---------------------------------------------------------------------------
// /etc/hosts parser
//
// One line per entry:
//   IP   name [alias ...]
// Fields separated by spaces / tabs. Lines starting with `#` are
// comments; blank lines and trailing `#`-comments are ignored.
// ---------------------------------------------------------------------------

// Advance `si` over a run of horizontal whitespace at the cursor.
static void skip_hspace_iter(StrIter *si) {
    char c;
    while (StrIterPeek(si, &c) && is_hspace(c)) {
        StrIterMustNext(si);
    }
}

// Eat the rest of the current line, including the terminating '\n' if
// one is present.
static void skip_to_eol_iter(StrIter *si) {
    char c;
    while (StrIterPeek(si, &c) && c != '\n') {
        StrIterMustNext(si);
    }
    if (StrIterRemainingLength(si) > 0) {
        StrIterMustNext(si);
    }
}

static void parse_hosts_path(HostsTable *table, Zstr path, Allocator *alloc) {
    Str buf = StrInit(alloc);
    if (!slurp_file(path, &buf)) {
        StrDeinit(&buf);
        return;
    }

    StrIter si = StrIterFromStr(buf);
    char    c;
    while (StrIterRemainingLength(&si)) {
        skip_hspace_iter(&si);

        // Comment / blank line -> eat the whole line.
        if (!StrIterPeek(&si, &c) || c == '#' || c == '\n' || c == '\r') {
            skip_to_eol_iter(&si);
            continue;
        }

        // First token: IP literal.
        size ip_start = StrIterIndex(&si);
        while (StrIterPeek(&si, &c) && !is_hspace(c) && c != '\n') {
            StrIterMustNext(&si);
        }
        u64 ip_len = (u64)(StrIterIndex(&si) - ip_start);

        bool got_v4 = false;
        bool got_v6 = false;
        u8   v4[4]  = {0};
        u8   v6[16] = {0};
        if (ip_len > 0 && ip_len < 64) {
            // Stack-backed NUL-terminated copy so the Zstr-API
            // `parse_ipv4` / `parse_ipv6` can scan it. Cap matches
            // the longest reasonable IPv6-with-zone-id literal.
            StrInitStack(ip_buf, 64) {
                StrPushBackMany(&ip_buf, (Zstr)StrIterDataAt(&si, ip_start), ip_len);
                got_v4 = parse_ipv4(StrBegin(&ip_buf), v4);
                if (!got_v4) {
                    got_v6 = parse_ipv6(StrBegin(&ip_buf), v6);
                }
            }
        }

        if (!got_v4 && !got_v6) {
            skip_to_eol_iter(&si);
            continue;
        }

        // Subsequent tokens: name + aliases.
        while (StrIterPeek(&si, &c) && c != '\n') {
            skip_hspace_iter(&si);
            if (!StrIterPeek(&si, &c) || c == '\n' || c == '#') {
                break;
            }
            size nm_start = StrIterIndex(&si);
            while (StrIterPeek(&si, &c) && !is_hspace(c) && c != '\n' && c != '#') {
                StrIterMustNext(&si);
            }
            u64 nm_len = (u64)(StrIterIndex(&si) - nm_start);
            if (nm_len == 0) {
                break;
            }

            HostsEntry e = {0};
            e.name       = StrInitFromCstr((Zstr)StrIterDataAt(&si, nm_start), nm_len, alloc);
            ascii_lower((u8 *)StrBegin(&e.name), StrLen(&e.name));
            if (got_v4) {
                MemCopy(e.ip, v4, 4);
                e.is_ipv6 = false;
            } else {
                MemCopy(e.ip, v6, 16);
                e.is_ipv6 = true;
            }
            if (!VecPushBackR(table, e)) {
                // VecPushBackR is R-form (copy semantics) -- on
                // failure ownership of `e.name` stays with us. Release
                // it to avoid leaking the cstr allocation.
                StrDeinit(&e.name);
                break;
            }
        }

        // Trailing `# ...` comment (if any) plus the newline.
        skip_to_eol_iter(&si);
    }

    StrDeinit(&buf);
}

// ---------------------------------------------------------------------------
// /etc/resolv.conf parser
//
// We only care about `nameserver <ip>` directives. Everything else
// (`search`, `domain`, `options`, ...) is ignored for v1.
// ---------------------------------------------------------------------------

static void parse_resolv_path(DnsAddrs *out, Zstr path, Allocator *alloc) {
    Str buf = StrInit(alloc);
    if (!slurp_file(path, &buf)) {
        StrDeinit(&buf);
        return;
    }

    static const char NS_KEYWORD[] = "nameserver";
    u64               kw_len       = sizeof(NS_KEYWORD) - 1;

    StrIter si = StrIterFromStr(buf);
    char    c;
    while (StrIterRemainingLength(&si)) {
        skip_hspace_iter(&si);

        if (!StrIterPeek(&si, &c) || c == '#' || c == ';' || c == '\n' || c == '\r') {
            skip_to_eol_iter(&si);
            continue;
        }

        // Match "nameserver" + whitespace at the line head. Use
        // `MemCompare` over the in-iter window plus a peek at offset
        // `kw_len` for the required separator -- both bounds-checked.
        char sep;
        if (StrIterRemainingLength(&si) > kw_len &&
            MemCompare(StrIterDataAt(&si, StrIterIndex(&si)), NS_KEYWORD, kw_len) == 0 &&
            StrIterPeekAt(&si, (i64)kw_len, &sep) && (sep == ' ' || sep == '\t')) {
            StrIterMustMove(&si, (i64)kw_len);
            skip_hspace_iter(&si);

            size ip_start = StrIterIndex(&si);
            while (StrIterPeek(&si, &c) && !is_hspace(c) && c != '\n' && c != '#') {
                StrIterMustNext(&si);
            }
            u64 ip_len = (u64)(StrIterIndex(&si) - ip_start);
            if (ip_len > 0 && ip_len < 64) {
                StrInitStack(ip_buf, 64) {
                    StrPushBackMany(&ip_buf, (Zstr)StrIterDataAt(&si, ip_start), ip_len);
                    u8 v4[4]  = {0};
                    u8 v6[16] = {0};
                    if (parse_ipv4(StrBegin(&ip_buf), v4)) {
                        SocketAddr a = sockaddr_v4(v4, 53);
                        VecPushBackR(out, a);
                    } else if (parse_ipv6(StrBegin(&ip_buf), v6)) {
                        SocketAddr a = sockaddr_v6(v6, 53);
                        VecPushBackR(out, a);
                    }
                }
            }
        }

        skip_to_eol_iter(&si);
    }

    StrDeinit(&buf);
}

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

bool dns_resolver_init(DnsResolver *out, Allocator *alloc) {
    if (!out || !alloc) {
        return false;
    }
    MemSet(out, 0, sizeof(*out));
    out->allocator   = alloc;
    out->hosts       = VecInitT(out->hosts, alloc);
    out->nameservers = VecInitT(out->nameservers, alloc);
    out->timeout_ms  = 5000;
    out->retries     = 2;

    parse_hosts_path(&out->hosts, HOSTS_FILE_PATH, alloc);
    parse_resolv_path(&out->nameservers, RESOLV_CONF_FILE_PATH, alloc);
    return true;
}

// ---------------------------------------------------------------------------
// Additional config paths / nameservers
// ---------------------------------------------------------------------------

static bool dns_add_path(DnsResolver *self, Zstr path, bool do_hosts, bool do_resolv) {
    if (!self || !path)
        return false;
    if (do_hosts)
        parse_hosts_path(&self->hosts, path, self->allocator);
    if (do_resolv)
        parse_resolv_path(&self->nameservers, path, self->allocator);
    return true;
}

static bool dns_add_path_len(DnsResolver *self, Zstr path, u64 len, bool do_hosts, bool do_resolv) {
    if (!self || !path)
        return false;
    Str buf = StrInit(self->allocator);
    StrPushBackMany(&buf, path, len);
    bool ok = dns_add_path(self, StrBegin(&buf), do_hosts, do_resolv);
    StrDeinit(&buf);
    return ok;
}

bool dns_resolver_add_path_str(DnsResolver *self, const Str *path) {
    if (!self || !path)
        return false;
    return dns_add_path(self, StrBegin(path), true, true);
}

bool dns_resolver_add_path_zstr(DnsResolver *self, Zstr path, u64 len) {
    return dns_add_path_len(self, path, len, true, true);
}

bool dns_resolver_add_hosts_path_str(DnsResolver *self, const Str *path) {
    if (!self || !path)
        return false;
    return dns_add_path(self, StrBegin(path), true, false);
}

bool dns_resolver_add_hosts_path_zstr(DnsResolver *self, Zstr path, u64 len) {
    return dns_add_path_len(self, path, len, true, false);
}

bool dns_resolver_add_resolv_path_str(DnsResolver *self, const Str *path) {
    if (!self || !path)
        return false;
    return dns_add_path(self, StrBegin(path), false, true);
}

bool dns_resolver_add_resolv_path_zstr(DnsResolver *self, Zstr path, u64 len) {
    return dns_add_path_len(self, path, len, false, true);
}

bool dns_resolver_add_paths(DnsResolver *self, const Strs *paths) {
    if (!self || !paths)
        return false;
    VecForeachPtr(paths, p) dns_resolver_add_path_str(self, p);
    return true;
}

bool dns_resolver_add_hosts_paths(DnsResolver *self, const Strs *paths) {
    if (!self || !paths)
        return false;
    VecForeachPtr(paths, p) dns_resolver_add_hosts_path_str(self, p);
    return true;
}

bool dns_resolver_add_resolv_paths(DnsResolver *self, const Strs *paths) {
    if (!self || !paths)
        return false;
    VecForeachPtr(paths, p) dns_resolver_add_resolv_path_str(self, p);
    return true;
}

bool dns_resolver_add_nameserver(DnsResolver *self, SocketAddr ns) {
    if (!self)
        return false;
    return VecPushBackR(&self->nameservers, ns);
}

void DnsResolverDeinit(DnsResolver *self) {
    if (!self) {
        return;
    }
    // Tolerate `DnsResolver r = {0}; DnsResolverDeinit(&r);` (e.g. a
    // failed `DnsResolverInit` left both vecs zeroed): a zero-init
    // Vec has `data == NULL` AND `__magic == 0`, so `validate_vec`
    // would abort. Skip the teardown when there's nothing to tear
    // down. `VecBegin` is a `NULL`-safe field read that does not
    // call into the validator.
    if (VecBegin(&self->hosts)) {
        VecForeachPtr(&self->hosts, e) {
            StrDeinit(&e->name);
        }
        VecDeinit(&self->hosts);
    }
    if (VecBegin(&self->nameservers)) {
        VecDeinit(&self->nameservers);
    }
    self->allocator = NULL;
}

// ---------------------------------------------------------------------------
// Resolution
// ---------------------------------------------------------------------------

// Strip trailing dot, lowercase, write into `out` (caller-managed Str).
static void normalize_hostname(Zstr name, Str *out) {
    if (!name) {
        return;
    }
    u64 len = ZstrLen(name);
    while (len > 0 && name[len - 1] == '.') {
        --len;
    }
    for (u64 i = 0; i < len; ++i) {
        char c = name[i];
        if (c >= 'A' && c <= 'Z') {
            c = (char)(c + ('a' - 'A'));
        }
        StrPushBackR(out, c);
    }
}

// Generate a transaction id from clock_gettime + getpid. Not
// cryptographic; the goal is to make off-path response injection
// require guessing 16 bits.
static u16 random_query_id(void) {
#if FEATURE_DIRECT_SYSCALL
    // Time + pid entropy. Linux has clock_gettime as a direct syscall;
    // Darwin's clock_gettime is libSystem-only, so use gettimeofday
    // (BSD #116) -- microsecond resolution is enough for query-id
    // randomization.
#    if PLATFORM_DARWIN
    struct {
        long sec;
        long usec;
    } tv = {0, 0};
    (void)direct_sys2(MISRA_SYS_gettimeofday, (long)(u64)&tv, 0);
    u64 mix = (u64)tv.sec ^ ((u64)tv.usec << 21) ^ (u64)direct_sys0(MISRA_SYS_getpid);
#    else
    struct {
        long sec;
        long nsec;
    } ts = {0, 0};
    (void)direct_sys2(MISRA_SYS_clock_gettime, 0L, (long)(u64)&ts);
    u64 mix = (u64)ts.sec ^ ((u64)ts.nsec << 21) ^ (u64)direct_sys0(MISRA_SYS_getpid);
#    endif
    return (u16)(mix ^ (mix >> 16) ^ (mix >> 32));
#else
    // Hosted build (Windows or any non-direct-syscall target). Defer to
    // the in-tree Prng -- it owns its own kernel-seeded state and is
    // already the project-sanctioned process-lifetime singleton; no
    // file-local LCG global needed here.
    return Prng16();
#endif
}

// Send `query` to `ns` over UDP, wait up to `timeout_ms` for a reply,
// land bytes in `resp_buf[0 .. resp_cap)`. Returns the response length
// on success or -1 on failure.
static i64 udp_round_trip(const SocketAddr *ns, const u8 *query, u64 qlen, u8 *resp_buf, u64 resp_cap, u32 timeout_ms) {
    Socket sock = {0};
    if (!SocketConnect(&sock, SOCKET_KIND_UDP, ns)) {
        return -1;
    }
    if (!SocketSetRecvTimeoutMs(sock.fd, timeout_ms)) {
        SocketClose(&sock);
        return -1;
    }
    i64 sent = SocketSend(&sock, query, qlen);
    if (sent != (i64)qlen) {
        SocketClose(&sock);
        return -1;
    }
    i64 got = SocketRecv(&sock, resp_buf, resp_cap);
    SocketClose(&sock);
    return got;
}

// Try one (server, qtype) combo. Appends matching A/AAAA records to
// `out` as SocketAddrs. Returns true if at least one record was
// extracted with NOERROR rcode; false otherwise. The transaction id
// is generated locally per call (see `random_query_id`) and the
// response's id is checked to match before the records are extracted.
static bool
    try_one_query(DnsResolver *self, const SocketAddr *ns, Zstr hostname, DnsType qtype, u16 port, DnsAddrs *out) {
    // Per-call scratch: one DNS query buffer (<= 1232 B) plus the parsed
    // response with its records vector. Everything is dropped at function
    // exit, so a bump arena fits better than DefaultAllocator -- a single
    // page-backed chunk handles a typical response with zero per-free
    // bookkeeping.
    ArenaAllocator scratch = ArenaAllocatorInit();

    DnsWireBuf query = VecInitT(query, &scratch);
    u16        id    = random_query_id();
    if (!DnsBuildQuery(&query, id, hostname, qtype)) {
        VecDeinit(&query);
        ArenaAllocatorDeinit(&scratch);
        return false;
    }

    u8  resp_buf[1232]; // safe UDP payload (avoids IP fragmentation)
    i64 got = udp_round_trip(ns, VecBegin(&query), VecLen(&query), resp_buf, sizeof(resp_buf), self->timeout_ms);
    VecDeinit(&query);
    if (got <= 0) {
        ArenaAllocatorDeinit(&scratch);
        return false;
    }

    DnsResponse resp = {0};
    // DnsParseResponse takes `Allocator *` -- legitimate erasure
    // boundary; pass at the call site, no intermediate variable.
    bool ok = DnsParseResponse(&resp, resp_buf, (u64)got, ALLOCATOR_OF(&scratch));
    if (!ok || resp.id != id || resp.rcode != DNS_RCODE_NOERROR) {
        DnsResponseDeinit(&resp);
        ArenaAllocatorDeinit(&scratch);
        return false;
    }

    bool found = false;
    VecForeachPtr(&resp.answers, rec) {
        if (rec->type == qtype) {
            SocketAddr a = qtype == DNS_TYPE_A ? sockaddr_v4(rec->ipv4, port) : sockaddr_v6(rec->ipv6, port);
            VecPushBackR(out, a);
            found = true;
        }
    }
    DnsResponseDeinit(&resp);
    ArenaAllocatorDeinit(&scratch);
    return found;
}

bool dns_resolve_5_zstr(DnsResolver *self, Zstr hostname, u16 port, SocketKind kind, DnsAddrs *out) {
    (void)kind; // protocol byte doesn't affect resolution
    if (!self || !hostname || !out) {
        return false;
    }

    // Hostnames are capped by DNS at 253 chars; the stack-backed
    // `Str` below holds 256 to keep the bound check and the buffer
    // aligned with one literal.
    if (ZstrLen(hostname) >= 256) {
        LOG_ERROR("DnsResolve: hostname \"{}\" exceeds 255 bytes", hostname);
        return false;
    }

    bool found = false;
    StrInitStack(norm, 256) {
        // `normalize_hostname` strips trailing dots and lowercases;
        // `StrInitStack` zero-fills the trailing slot so `StrBegin`
        // is a valid Zstr for the matchers / DNS query below.
        normalize_hostname(hostname, &norm);
        Zstr nq = StrBegin(&norm);

        // 1. /etc/hosts fast path.
        VecForeachPtr(&self->hosts, e) {
            if (StrLen(&e->name) > 0 && ZstrCompare(StrBegin(&e->name), nq) == 0) {
                SocketAddr a = e->is_ipv6 ? sockaddr_v6(e->ip, port) : sockaddr_v4(e->ip, port);
                VecPushBackR(out, a);
                found = true;
            }
        }
        if (found) {
            break;
        }

        // 2. Nameserver query path.
        if (VecLen(&self->nameservers) == 0) {
            LOG_ERROR("DnsResolve: no nameservers configured (read /etc/resolv.conf at init)");
            break;
        }

        static const DnsType QUERY_TYPES[] = {DNS_TYPE_A, DNS_TYPE_AAAA};
        for (u32 i = 0; i < sizeof(QUERY_TYPES) / sizeof(QUERY_TYPES[0]); ++i) {
            DnsType qtype = QUERY_TYPES[i];
            // Iterate nameservers; each gets up to `retries` attempts.
            VecForeachPtr(&self->nameservers, ns) {
                for (u32 attempt = 0; attempt < self->retries + 1; ++attempt) {
                    if (try_one_query(self, ns, nq, qtype, port, out)) {
                        found = true;
                        goto next_qtype;
                    }
                }
            }
next_qtype:;
        }

        if (!found) {
            LOG_ERROR("DnsResolve: no A/AAAA records found for \"{}\"", hostname);
        }
    }
    return found;
}

bool dns_resolve_4_vec_zstr(DnsResolver *self, Zstr spec, SocketKind kind, DnsAddrs *out) {
    if (!self || !spec || !out) {
        return false;
    }

    // Numeric short-circuit -- SocketAddrParse handles both IPv4
    // (`a.b.c.d:port`) and bracketed IPv6 (`[::1]:port`) without
    // touching DNS or /etc/hosts. Most callers paying the cost of
    // setting up a DnsResolver still expect numeric addresses to skip
    // the network.
    SocketAddr direct;
    if (SocketAddrParse(&direct, spec, kind)) {
        VecPushBackR(out, direct);
        return true;
    }

    // Otherwise it's hostname:port. Split on the last ':'; hostnames
    // never contain colons and the IPv6 case was eliminated by the
    // SocketAddrParse attempt above.
    u64 spec_len = ZstrLen(spec);
    u64 colon_at = spec_len;
    for (u64 i = spec_len; i > 0; --i) {
        if (spec[i - 1] == ':') {
            colon_at = i - 1;
            break;
        }
    }
    if (colon_at >= spec_len) {
        LOG_ERROR("DnsResolve: spec \"{}\" has no \":port\"", spec);
        return false;
    }
    if (colon_at >= 256) {
        LOG_ERROR("DnsResolve: host portion of \"{}\" exceeds 255 bytes", spec);
        return false;
    }

    // Parse and validate the port first so we can fail loudly without
    // touching the host buffer.
    u16 port = 0;
    for (u64 i = colon_at + 1; i < spec_len; ++i) {
        char c = spec[i];
        if (c < '0' || c > '9') {
            LOG_ERROR("DnsResolve: non-numeric port in \"{}\"", spec);
            return false;
        }
        u32 next = (u32)port * 10u + (u32)(c - '0');
        if (next > 0xFFFFu) {
            LOG_ERROR("DnsResolve: port in \"{}\" out of range", spec);
            return false;
        }
        port = (u16)next;
    }
    // A bare "host:" with no digits after the colon is malformed.
    if (colon_at + 1 == spec_len) {
        LOG_ERROR("DnsResolve: empty port in \"{}\"", spec);
        return false;
    }

    // Hold the host slice in a stack-backed Str so the trailing
    // NUL lives next to the bytes, with no parallel `char host[]`.
    bool ok = false;
    StrInitStack(host, 256) {
        StrPushBackMany(&host, spec, colon_at);
        ok = dns_resolve_5_zstr(self, StrBegin(&host), port, kind, out);
    }
    return ok;
}

bool dns_resolve_5_str(DnsResolver *self, const Str *hostname, u16 port, SocketKind kind, DnsAddrs *out) {
    if (!self || !hostname || !out) {
        return false;
    }
    return dns_resolve_5_zstr(self, StrBegin(hostname), port, kind, out);
}

bool dns_resolve_6_cstr(DnsResolver *self, Zstr hostname, u64 hostname_len, u16 port, SocketKind kind, DnsAddrs *out) {
    if (!self || !hostname || !out) {
        return false;
    }
    if (hostname_len >= 256) {
        LOG_ERROR("DnsResolve: hostname exceeds 255 bytes");
        return false;
    }
    bool ok = false;
    StrInitStack(host, 256) {
        StrPushBackMany(&host, hostname, hostname_len);
        ok = dns_resolve_5_zstr(self, StrBegin(&host), port, kind, out);
    }
    return ok;
}

bool dns_resolve_4_vec_str(DnsResolver *self, const Str *spec, SocketKind kind, DnsAddrs *out) {
    if (!self || !spec || !out) {
        return false;
    }
    return dns_resolve_4_vec_zstr(self, StrBegin(spec), kind, out);
}

bool dns_resolve_4_one_zstr(DnsResolver *self, Zstr spec, SocketKind kind, SocketAddr *out) {
    if (!self || !spec || !out) {
        return false;
    }
    DnsAddrs addrs    = VecInitT(addrs, self->allocator);
    bool     ok       = dns_resolve_4_vec_zstr(self, spec, kind, &addrs);
    bool     have_one = ok && VecLen(&addrs) > 0;
    if (have_one) {
        *out = VecAt(&addrs, 0);
    }
    VecDeinit(&addrs);
    return have_one;
}

bool dns_resolve_4_one_str(DnsResolver *self, const Str *spec, SocketKind kind, SocketAddr *out) {
    if (!self || !spec || !out) {
        return false;
    }
    return dns_resolve_4_one_zstr(self, StrBegin(spec), kind, out);
}
