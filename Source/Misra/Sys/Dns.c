/// file      : sys/dns.c
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// In-tree DNS resolver: reads /etc/hosts and /etc/resolv.conf at
/// init, then resolves hostnames via the local table first and the
/// configured nameservers (UDP) second. Wire-format work goes through
/// Parsers/Dns; transport goes through Sys/Socket. No libc.

#include <Misra/Sys/Dns.h>

#include <Misra/Parsers/Dns.h>
#include <Misra/Std/Allocator/Default.h>
#include <Misra/Std/File.h>
#include <Misra/Std/Log.h>
#include <Misra/Std/Memory.h>
#include <Misra/Sys/Socket.h>

#include "../_Syscall.h"

#include <netinet/in.h>
#include <stdint.h>
#include <sys/socket.h>

// ---------------------------------------------------------------------------
// IP literal parsers (duplicated from Sys/Socket -- the parsers are
// static there. Sys/Socket and Sys/Dns each need them; rather than
// promote them to a shared header, we copy the small implementations
// here. Both consumers are tiny.)
// ---------------------------------------------------------------------------

static i32 hex_nibble_value(char c) {
    if (c >= '0' && c <= '9')
        return c - '0';
    if (c >= 'a' && c <= 'f')
        return 10 + (c - 'a');
    if (c >= 'A' && c <= 'F')
        return 10 + (c - 'A');
    return -1;
}

static bool parse_ipv4(const char *s, u8 octets[4]) {
    if (!s)
        return false;
    for (i32 i = 0; i < 4; ++i) {
        if (*s < '0' || *s > '9')
            return false;
        u32 v = 0;
        while (*s >= '0' && *s <= '9') {
            v = v * 10 + (u32)(*s - '0');
            if (v > 255)
                return false;
            ++s;
        }
        octets[i] = (u8)v;
        if (i < 3) {
            if (*s != '.')
                return false;
            ++s;
        }
    }
    return *s == '\0';
}

static bool parse_ipv6(const char *s, u8 bytes[16]) {
    if (!s)
        return false;
    u16  before_cc[8];
    u16  after_cc[8];
    i32  before_n = 0;
    i32  after_n  = 0;
    bool seen_cc  = false;
    u16 *slot     = before_cc;
    i32 *slot_n   = &before_n;

    if (s[0] == ':' && s[1] == ':') {
        seen_cc  = true;
        slot     = after_cc;
        slot_n   = &after_n;
        s       += 2;
        if (*s == '\0') {
            for (i32 i = 0; i < 16; ++i)
                bytes[i] = 0;
            return true;
        }
    }

    for (;;) {
        if (*slot_n >= 8)
            return false;
        i32 v        = 0;
        i32 n_digits = 0;
        while (n_digits < 4) {
            i32 nib = hex_nibble_value(*s);
            if (nib < 0)
                break;
            v = (v << 4) | nib;
            ++s;
            ++n_digits;
        }
        if (n_digits == 0)
            return false;
        slot[(*slot_n)++] = (u16)v;
        if (*s == '\0')
            break;
        if (*s != ':')
            return false;
        ++s;
        if (*s == ':') {
            if (seen_cc)
                return false;
            seen_cc = true;
            slot    = after_cc;
            slot_n  = &after_n;
            ++s;
            if (*s == '\0')
                break;
        }
    }

    if (!seen_cc) {
        if (before_n != 8)
            return false;
        for (i32 i = 0; i < 8; ++i) {
            bytes[i * 2]     = (u8)(before_cc[i] >> 8);
            bytes[i * 2 + 1] = (u8)(before_cc[i] & 0xFFu);
        }
        return true;
    }

    if (before_n + after_n > 7)
        return false;
    i32 mid_zeros = 8 - before_n - after_n;
    i32 idx       = 0;
    for (i32 i = 0; i < before_n; ++i, ++idx) {
        bytes[idx * 2]     = (u8)(before_cc[i] >> 8);
        bytes[idx * 2 + 1] = (u8)(before_cc[i] & 0xFFu);
    }
    for (i32 i = 0; i < mid_zeros; ++i, ++idx) {
        bytes[idx * 2]     = 0;
        bytes[idx * 2 + 1] = 0;
    }
    for (i32 i = 0; i < after_n; ++i, ++idx) {
        bytes[idx * 2]     = (u8)(after_cc[i] >> 8);
        bytes[idx * 2 + 1] = (u8)(after_cc[i] & 0xFFu);
    }
    return true;
}

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

static void ascii_lower(char *p, u64 n) {
    for (u64 i = 0; i < n; ++i) {
        char c = p[i];
        if (c >= 'A' && c <= 'Z') {
            p[i] = (char)(c + ('a' - 'A'));
        }
    }
}

// Slurp a whole File into `out`. Caller owns `out`; allocator inline
// on the Str. Returns false on read error; missing file returns true
// with an empty Str.
static bool slurp_file(const char *path, Str *out) {
    File f = FileOpen(path, "rb");
    if (!FileIsValid(&f)) {
        // Missing config file is fine -- resolver just won't know about it.
        return true;
    }
    char chunk[4096];
    for (;;) {
        i64 n = FileRead(&f, chunk, sizeof(chunk));
        if (n < 0) {
            FileClose(&f);
            return false;
        }
        if (n == 0)
            break;
        for (i64 i = 0; i < n; ++i) {
            StrPushBack(out, chunk[i]);
        }
    }
    FileClose(&f);
    return true;
}

// ---------------------------------------------------------------------------
// /etc/hosts parser
//
// One line per entry:
//   IP   name [alias ...]
// Fields separated by spaces / tabs. Lines starting with `#` are
// comments; blank lines and trailing `#`-comments are ignored.
// ---------------------------------------------------------------------------

static void parse_hosts_table(HostsTable *table, Allocator *alloc) {
    Str buf = StrInit(alloc);
    if (!slurp_file("/etc/hosts", &buf)) {
        StrDeinit(&buf);
        return;
    }

    const char *p   = buf.data;
    const char *end = buf.data ? buf.data + buf.length : NULL;
    while (p && p < end) {
        // Skip leading horizontal whitespace.
        while (p < end && is_hspace(*p))
            ++p;
        // Comment / blank line -> skip to newline.
        if (p >= end || *p == '#' || *p == '\n' || *p == '\r') {
            while (p < end && *p != '\n')
                ++p;
            if (p < end)
                ++p;
            continue;
        }

        // First token: IP literal.
        const char *ip_start = p;
        while (p < end && !is_hspace(*p) && *p != '\n')
            ++p;
        u64 ip_len = (u64)(p - ip_start);

        char ip_buf[64];
        bool got_v4 = false;
        bool got_v6 = false;
        u8   v4[4]  = {0};
        u8   v6[16] = {0};
        if (ip_len > 0 && ip_len < sizeof(ip_buf)) {
            MemCopy(ip_buf, ip_start, ip_len);
            ip_buf[ip_len] = '\0';
            got_v4         = parse_ipv4(ip_buf, v4);
            if (!got_v4) {
                got_v6 = parse_ipv6(ip_buf, v6);
            }
        }

        if (!got_v4 && !got_v6) {
            // Garbled line; skip past newline.
            while (p < end && *p != '\n')
                ++p;
            if (p < end)
                ++p;
            continue;
        }

        // Subsequent tokens: name + aliases.
        while (p < end && *p != '\n') {
            while (p < end && is_hspace(*p))
                ++p;
            if (p >= end || *p == '\n' || *p == '#')
                break;
            const char *nm_start = p;
            while (p < end && !is_hspace(*p) && *p != '\n' && *p != '#')
                ++p;
            u64 nm_len = (u64)(p - nm_start);
            if (nm_len == 0)
                break;

            HostsEntry e = {0};
            e.name       = StrInitFromCstr(nm_start, nm_len, alloc);
            ascii_lower(e.name.data, e.name.length);
            if (got_v4) {
                MemCopy(e.ip, v4, 4);
                e.is_ipv6 = false;
            } else {
                MemCopy(e.ip, v6, 16);
                e.is_ipv6 = true;
            }
            VecPushBackR(table, e);
        }

        // Skip trailing `# ...` comment + the newline.
        while (p < end && *p != '\n')
            ++p;
        if (p < end)
            ++p;
    }

    StrDeinit(&buf);
}

// ---------------------------------------------------------------------------
// /etc/resolv.conf parser
//
// We only care about `nameserver <ip>` directives. Everything else
// (`search`, `domain`, `options`, ...) is ignored for v1.
// ---------------------------------------------------------------------------

static void parse_resolv_conf(DnsAddrs *out, Allocator *alloc) {
    Str buf = StrInit(alloc);
    if (!slurp_file("/etc/resolv.conf", &buf)) {
        StrDeinit(&buf);
        return;
    }

    static const char NS_KEYWORD[] = "nameserver";
    u64               kw_len       = sizeof(NS_KEYWORD) - 1;

    const char *p   = buf.data;
    const char *end = buf.data ? buf.data + buf.length : NULL;
    while (p && p < end) {
        while (p < end && is_hspace(*p))
            ++p;
        if (p >= end || *p == '#' || *p == ';' || *p == '\n' || *p == '\r') {
            while (p < end && *p != '\n')
                ++p;
            if (p < end)
                ++p;
            continue;
        }

        // Match "nameserver" + whitespace at the line head.
        if ((u64)(end - p) > kw_len && MemCompare(p, NS_KEYWORD, kw_len) == 0 &&
            (p[kw_len] == ' ' || p[kw_len] == '\t')) {
            p += kw_len;
            while (p < end && is_hspace(*p))
                ++p;
            const char *ip_start = p;
            while (p < end && !is_hspace(*p) && *p != '\n' && *p != '#')
                ++p;
            u64  ip_len = (u64)(p - ip_start);
            char ip_buf[64];
            if (ip_len > 0 && ip_len < sizeof(ip_buf)) {
                MemCopy(ip_buf, ip_start, ip_len);
                ip_buf[ip_len] = '\0';
                u8 v4[4]       = {0};
                u8 v6[16]      = {0};
                if (parse_ipv4(ip_buf, v4)) {
                    SocketAddr a = sockaddr_v4(v4, 53);
                    VecPushBackR(out, a);
                } else if (parse_ipv6(ip_buf, v6)) {
                    SocketAddr a = sockaddr_v6(v6, 53);
                    VecPushBackR(out, a);
                }
            }
        }

        while (p < end && *p != '\n')
            ++p;
        if (p < end)
            ++p;
    }

    StrDeinit(&buf);
}

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

bool DnsResolverInit(DnsResolver *out, Allocator *alloc) {
    if (!out || !alloc) {
        return false;
    }
    MemSet(out, 0, sizeof(*out));
    out->alloc       = alloc;
    out->hosts       = VecInitT(out->hosts, alloc);
    out->nameservers = VecInitT(out->nameservers, alloc);
    out->timeout_ms  = 5000;
    out->retries     = 2;

    parse_hosts_table(&out->hosts, alloc);
    parse_resolv_conf(&out->nameservers, alloc);
    return true;
}

void DnsResolverDeinit(DnsResolver *self) {
    if (!self) {
        return;
    }
    if (self->hosts.data) {
        VecForeachPtr(&self->hosts, e) {
            StrDeinit(&e->name);
        }
        VecDeinit(&self->hosts);
    }
    VecDeinit(&self->nameservers);
    self->alloc = NULL;
}

// ---------------------------------------------------------------------------
// Resolution
// ---------------------------------------------------------------------------

// Strip trailing dot, lowercase, write into `out` (caller-managed Str).
static void normalize_hostname(const char *name, Str *out) {
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
        StrPushBack(out, c);
    }
}

// Generate a transaction id from clock_gettime + getpid. Not
// cryptographic; the goal is to make off-path response injection
// require guessing 16 bits.
static u16 random_query_id(void) {
#if MISRA_HAVE_DIRECT_SYSCALL
    struct {
        long sec;
        long nsec;
    } ts = {0, 0};
    (void)misra_sys2(MISRA_SYS_clock_gettime, 0L, (long)(uintptr_t)&ts);
    u64 mix = (u64)ts.sec ^ ((u64)ts.nsec << 21) ^ (u64)misra_sys0(MISRA_SYS_getpid);
    return (u16)(mix ^ (mix >> 16) ^ (mix >> 32));
#else
    // Non-Linux: weak fallback. macOS/Windows ports can swap in
    // platform-native time sources.
    static u64 counter = 0xC0FFEE;
    counter            = counter * 6364136223846793005ULL + 1442695040888963407ULL;
    return (u16)counter;
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
// extracted with NOERROR rcode; false otherwise. `id_in` must match
// the response's transaction id.
static bool try_one_query(
    DnsResolver      *self,
    const SocketAddr *ns,
    const char       *hostname,
    DnsType           qtype,
    u16               port,
    DnsAddrs         *out
) {
    DefaultAllocator scratch = DefaultAllocatorInit();
    Allocator       *sa      = ALLOCATOR_OF(&scratch);

    DnsWireBuf query = VecInitT(query, sa);
    u16        id    = random_query_id();
    if (!DnsBuildQuery(&query, id, hostname, qtype)) {
        VecDeinit(&query);
        DefaultAllocatorDeinit(&scratch);
        return false;
    }

    u8  resp_buf[1232]; // safe UDP payload (avoids IP fragmentation)
    i64 got = udp_round_trip(ns, query.data, query.length, resp_buf, sizeof(resp_buf), self->timeout_ms);
    VecDeinit(&query);
    if (got <= 0) {
        DefaultAllocatorDeinit(&scratch);
        return false;
    }

    DnsResponse resp = {0};
    bool        ok   = DnsParseResponse(&resp, resp_buf, (u64)got, sa);
    if (!ok || resp.id != id || resp.rcode != DNS_RCODE_NOERROR) {
        DnsResponseDeinit(&resp);
        DefaultAllocatorDeinit(&scratch);
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
    DefaultAllocatorDeinit(&scratch);
    return found;
}

bool DnsResolve_5(DnsResolver *self, const char *hostname, u16 port, SocketKind kind, DnsAddrs *out) {
    (void)kind; // protocol byte doesn't affect resolution
    if (!self || !hostname || !out) {
        return false;
    }

    // Normalize the input: strip trailing dot, lowercase. Stack-bound
    // scratch -- hostnames are bounded by DNS limits at 253 chars.
    char             buf[256];
    DefaultAllocator scratch = DefaultAllocatorInit();
    Str              norm    = StrInit(ALLOCATOR_OF(&scratch));
    normalize_hostname(hostname, &norm);
    if (norm.length >= sizeof(buf)) {
        StrDeinit(&norm);
        DefaultAllocatorDeinit(&scratch);
        LOG_ERROR("DnsResolve: hostname \"{}\" exceeds 255 bytes", (const char *)hostname);
        return false;
    }
    MemCopy(buf, norm.data, norm.length);
    buf[norm.length] = '\0';
    StrDeinit(&norm);
    DefaultAllocatorDeinit(&scratch);

    // 1. /etc/hosts fast path.
    bool found = false;
    VecForeachPtr(&self->hosts, e) {
        if (e->name.length > 0 && ZstrCompare(e->name.data, buf) == 0) {
            SocketAddr a = e->is_ipv6 ? sockaddr_v6(e->ip, port) : sockaddr_v4(e->ip, port);
            VecPushBackR(out, a);
            found = true;
        }
    }
    if (found) {
        return true;
    }

    // 2. Nameserver query path.
    if (self->nameservers.length == 0) {
        LOG_ERROR("DnsResolve: no nameservers configured (read /etc/resolv.conf at init)");
        return false;
    }

    static const DnsType QUERY_TYPES[] = {DNS_TYPE_A, DNS_TYPE_AAAA};
    for (u32 i = 0; i < sizeof(QUERY_TYPES) / sizeof(QUERY_TYPES[0]); ++i) {
        DnsType qtype = QUERY_TYPES[i];
        // Iterate nameservers; each gets up to `retries` attempts.
        VecForeachPtr(&self->nameservers, ns) {
            for (u32 attempt = 0; attempt < self->retries + 1; ++attempt) {
                if (try_one_query(self, ns, buf, qtype, port, out)) {
                    found = true;
                    goto next_qtype;
                }
            }
        }
next_qtype:;
    }

    if (!found) {
        LOG_ERROR("DnsResolve: no A/AAAA records found for \"{}\"", (const char *)hostname);
    }
    return found;
}

bool DnsResolve_4_vec(DnsResolver *self, const char *spec, SocketKind kind, DnsAddrs *out) {
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
        LOG_ERROR("DnsResolve: spec \"{}\" has no \":port\"", (const char *)spec);
        return false;
    }
    char host[256];
    if (colon_at >= sizeof(host)) {
        LOG_ERROR("DnsResolve: host portion of \"{}\" exceeds 255 bytes", (const char *)spec);
        return false;
    }
    MemCopy(host, spec, colon_at);
    host[colon_at] = '\0';

    u16 port = 0;
    for (u64 i = colon_at + 1; i < spec_len; ++i) {
        char c = spec[i];
        if (c < '0' || c > '9') {
            LOG_ERROR("DnsResolve: non-numeric port in \"{}\"", (const char *)spec);
            return false;
        }
        u32 next = (u32)port * 10u + (u32)(c - '0');
        if (next > 0xFFFFu) {
            LOG_ERROR("DnsResolve: port in \"{}\" out of range", (const char *)spec);
            return false;
        }
        port = (u16)next;
    }
    // A bare "host:" with no digits after the colon is malformed.
    if (colon_at + 1 == spec_len) {
        LOG_ERROR("DnsResolve: empty port in \"{}\"", (const char *)spec);
        return false;
    }

    return DnsResolve_5(self, host, port, kind, out);
}

bool DnsResolve_4_one(DnsResolver *self, const char *spec, SocketKind kind, SocketAddr *out) {
    if (!self || !spec || !out) {
        return false;
    }
    DnsAddrs addrs    = VecInitT(addrs, self->alloc);
    bool     ok       = DnsResolve_4_vec(self, spec, kind, &addrs);
    bool     have_one = ok && addrs.length > 0;
    if (have_one) {
        *out = addrs.data[0];
    }
    VecDeinit(&addrs);
    return have_one;
}
