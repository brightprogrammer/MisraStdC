/// file      : Socket.c
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Cross-platform implementation of the `Sys/Socket` module. POSIX
/// (Linux + macOS) goes through direct syscalls on Linux and libSystem
/// on macOS; Windows goes through Winsock2 (Ws2_32.dll). The two halves
/// share the pure-C IP parsers/formatters at the top of the file -- the
/// `host:port` string layer is platform-neutral. Only the syscall-shaped
/// bits differ.
///
/// The Windows port targets Win10 2004+ (May 2020): WSAPoll is correct
/// from that build onward, so we use it without the `select()`
/// fallback that older ports needed. Building against Win7 would
/// require adding that fallback for failed `connect()` detection.

#include <Misra/Config.h>

#if !PLATFORM_WINDOWS
#    define _DEFAULT_SOURCE
#    define _POSIX_C_SOURCE 200809L
#endif

#include <Misra/Sys/Socket.h>

#include <Misra/Std.h>
#include <Misra/Std/Log.h>

#if PLATFORM_WINDOWS
// Order matters: winsock2.h must come before windows.h. Defining the
// guard suppresses the legacy winsock.h that windows.h would otherwise
// drag in via mistake.
#    ifndef WIN32_LEAN_AND_MEAN
#        define WIN32_LEAN_AND_MEAN
#    endif
#    include <winsock2.h>
#    include <ws2tcpip.h>
#    include <windows.h>
#    pragma comment(lib, "Ws2_32.lib")

// Winsock's `SOCKET` is unsigned UINT_PTR. The public `SockFd` typedef
// is `u64` on Windows, which holds it losslessly on both Win32 and
// Win64. Cast helpers move between the two without truncation.
static inline SOCKET sf_to_socket(SockFd s) {
    return (SOCKET)s;
}
static inline SockFd socket_to_sf(SOCKET s) {
    return (SockFd)s;
}

// Lazy one-shot WSAStartup. `Sys/Socket` is the only Misra module that
// needs Winsock; doing it here keeps the dependency local. The
// `InitOnceExecuteOnce` API is the documented Win-7+ way to run a
// thread-safe one-shot.
static INIT_ONCE     g_winsock_init_once = INIT_ONCE_STATIC_INIT;
static int           g_winsock_init_rc   = 0;
static BOOL CALLBACK winsock_init_cb(PINIT_ONCE InitOnce, PVOID Parameter, PVOID *Context) {
    (void)InitOnce;
    (void)Parameter;
    (void)Context;
    WSADATA wsa;
    g_winsock_init_rc = WSAStartup(MAKEWORD(2, 2), &wsa);
    return TRUE;
}
static bool ensure_winsock(void) {
    InitOnceExecuteOnce(&g_winsock_init_once, winsock_init_cb, NULL, NULL);
    if (g_winsock_init_rc != 0) {
        LOG_ERROR("ensure_winsock: WSAStartup failed: {}", (i32)g_winsock_init_rc);
        return false;
    }
    return true;
}

// Windows-only: report the last Winsock error. The CRT errno is not
// populated by Winsock calls -- WSAGetLastError is authoritative. The
// `ret` arg is ignored on this side; we keep the signature unified
// with the POSIX version so call sites are platform-independent.
#    define LOG_SOCK_ERROR(ret, msg)                                                                                   \
        do {                                                                                                           \
            (void)(ret);                                                                                               \
            LOG_ERROR(msg " (WSAGetLastError={})", (i32)WSAGetLastError());                                            \
        } while (0)

#else // POSIX

#    include <arpa/inet.h>
#    include <fcntl.h>
#    include <netinet/in.h>
#    include <netinet/tcp.h>
#    include <poll.h>
#    include <sys/socket.h>
#    include <sys/time.h>
#    include <unistd.h>

#    include "../_Syscall.h"

#    if FEATURE_DIRECT_SYSCALL
// Linux: direct-syscall wrappers for the BSD-sockets primitives used
// below. `recv` / `send` are mapped onto `recvfrom` / `sendto` with
// the addr arguments cleared since the kernel offers no separate
// syscall for them. macOS / BSD keep libSystem; Windows is in the
// other half of this file.

static inline long misra_sock_socket(int domain, int type, int protocol) {
    return misra_sys3(MISRA_SYS_socket, (long)domain, (long)type, (long)protocol);
}
static inline long misra_sock_bind(int fd, const void *addr, unsigned addrlen) {
    return misra_sys3(MISRA_SYS_bind, (long)fd, (long)(u64)addr, (long)addrlen);
}
static inline long misra_sock_connect(int fd, const void *addr, unsigned addrlen) {
    return misra_sys3(MISRA_SYS_connect, (long)fd, (long)(u64)addr, (long)addrlen);
}
static inline long misra_sock_listen(int fd, int backlog) {
    return misra_sys2(MISRA_SYS_listen, (long)fd, (long)backlog);
}
static inline long misra_sock_accept(int fd, void *addr, void *addrlen) {
    return misra_sys3(MISRA_SYS_accept, (long)fd, (long)(u64)addr, (long)(u64)addrlen);
}
static inline long misra_sock_recv(int fd, void *buf, unsigned long n, int flags) {
    return misra_sys6(MISRA_SYS_recvfrom, (long)fd, (long)(u64)buf, (long)n, (long)flags, 0, 0);
}
static inline long misra_sock_send(int fd, const void *buf, unsigned long n, int flags) {
    return misra_sys6(MISRA_SYS_sendto, (long)fd, (long)(u64)buf, (long)n, (long)flags, 0, 0);
}
static inline long misra_sock_setsockopt(int fd, int level, int optname, const void *optval, unsigned optlen) {
    return misra_sys5(MISRA_SYS_setsockopt, (long)fd, (long)level, (long)optname, (long)(u64)optval, (long)optlen);
}
static inline long misra_sock_getsockname(int fd, void *addr, void *addrlen) {
    return misra_sys3(MISRA_SYS_getsockname, (long)fd, (long)(u64)addr, (long)(u64)addrlen);
}
static inline long misra_sock_close(int fd) {
    return misra_sys1(MISRA_SYS_close, (long)fd);
}
static inline long misra_sock_fcntl(int fd, int cmd, long arg) {
    return misra_sys3(MISRA_SYS_fcntl, (long)fd, (long)cmd, arg);
}
static inline long misra_sock_poll(void *pfds, unsigned long nfds, int timeout_ms) {
#        if PLATFORM_DARWIN || ARCHITECTURE_X86_64
    // Darwin has SYS_poll (#230); Linux x86_64 has SYS_poll (#7). Same shape.
    return misra_sys3(MISRA_SYS_poll, (long)(u64)pfds, (long)nfds, (long)timeout_ms);
#        else
    // Linux aarch64 dropped poll for ppoll(fds, nfds, ts, sigmask, sizeof(sigmask)).
    struct {
        long sec;
        long nsec;
    } ts;
    void *ts_ptr = NULL;
    if (timeout_ms >= 0) {
        ts.sec  = timeout_ms / 1000;
        ts.nsec = (long)(timeout_ms % 1000) * 1000000L;
        ts_ptr  = &ts;
    }
    return misra_sys5(MISRA_SYS_ppoll, (long)(u64)pfds, (long)nfds, (long)(u64)ts_ptr, 0, 0);
#        endif
}

#        define socket(d, t, p)              ((i32)misra_sock_socket((d), (t), (p)))
#        define bind(fd, a, l)               ((int)misra_sock_bind((fd), (a), (unsigned)(l)))
#        define connect(fd, a, l)            ((int)misra_sock_connect((fd), (a), (unsigned)(l)))
#        define listen(fd, b)                ((int)misra_sock_listen((fd), (b)))
#        define accept(fd, a, l)             ((i32)misra_sock_accept((fd), (a), (l)))
#        define recv(fd, b, n, f)            ((long)misra_sock_recv((fd), (b), (unsigned long)(n), (f)))
#        define send(fd, b, n, f)            ((long)misra_sock_send((fd), (b), (unsigned long)(n), (f)))
#        define setsockopt(fd, lv, on, v, l) ((int)misra_sock_setsockopt((fd), (lv), (on), (v), (unsigned)(l)))
#        define getsockname(fd, a, l)        ((int)misra_sock_getsockname((fd), (a), (l)))
#        define close(fd)                    ((int)misra_sock_close(fd))
#        define fcntl(fd, cmd, arg)          ((int)misra_sock_fcntl((fd), (cmd), (long)(arg)))
#        define poll(pfds, n, t)             ((int)misra_sock_poll((pfds), (unsigned long)(n), (t)))
#    endif

// POSIX side: SockFd is a thin alias for `int`. No conversion needed.
static inline int sf_to_int(SockFd s) {
    return (int)s;
}
static inline SockFd int_to_sf(int s) {
    return (SockFd)s;
}

// POSIX: ret is the failing syscall's return value. On Linux direct-
// syscall it carries -errno directly; on libSystem (macOS) the value
// is -1 and errno is set. ErrnoOf papers over the difference.
#    define LOG_SOCK_ERROR(ret, msg) LOG_SYS_ERROR(ErrnoOf(ret), msg)
#endif // PLATFORM_WINDOWS

// ---------------------------------------------------------------------------
// Pure-C parsers / formatters. No platform dependencies -- shared.
// ---------------------------------------------------------------------------

// Hex-nibble helper used by the IPv6 parser. Returns 0..15 on a valid
// digit, -1 otherwise.
static i32 hex_nibble_value(char c) {
    if (c >= '0' && c <= '9')
        return c - '0';
    if (c >= 'a' && c <= 'f')
        return 10 + (c - 'a');
    if (c >= 'A' && c <= 'F')
        return 10 + (c - 'A');
    return -1;
}

// Parse a decimal port number (0..65535) from a NUL-terminated string.
// Empty / non-numeric input -> false. Out-of-range -> false.
static bool parse_port(const char *s, u16 *out) {
    if (!s || !*s)
        return false;
    u32 v = 0;
    for (const char *p = s; *p; ++p) {
        if (*p < '0' || *p > '9')
            return false;
        v = v * 10 + (u32)(*p - '0');
        if (v > 0xFFFFu)
            return false;
    }
    *out = (u16)v;
    return true;
}

// Parse an IPv4 dotted-quad ("a.b.c.d") into 4 bytes.
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

// Parse an IPv6 textual form into 16 bytes. Handles RFC 5952 "::"
// compression. Does not handle zone IDs or embedded IPv4.
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

static bool format_ipv4(const u8 octets[4], char *dst, size dst_size) {
    if (!dst || dst_size < 16)
        return false;
    size pos = 0;
    for (i32 i = 0; i < 4; ++i) {
        u32  v = octets[i];
        char tmp[4];
        i32  n = 0;
        if (v == 0) {
            tmp[n++] = '0';
        } else {
            while (v) {
                tmp[n++]  = (char)('0' + (v % 10));
                v        /= 10;
            }
        }
        while (n--) {
            if (pos + 1 >= dst_size)
                return false;
            dst[pos++] = tmp[n];
        }
        if (i < 3) {
            if (pos + 1 >= dst_size)
                return false;
            dst[pos++] = '.';
        }
    }
    dst[pos] = '\0';
    return true;
}

static bool append_hextet(char *dst, size dst_size, size *pos, u16 v) {
    char tmp[4];
    i32  n = 0;
    if (v == 0) {
        tmp[n++] = '0';
    } else {
        while (v) {
            u8 nib     = v & 0xF;
            tmp[n++]   = nib < 10 ? (char)('0' + nib) : (char)('a' + nib - 10);
            v        >>= 4;
        }
    }
    while (n--) {
        if (*pos + 1 >= dst_size)
            return false;
        dst[(*pos)++] = tmp[n];
    }
    return true;
}

static bool format_ipv6(const u8 bytes[16], char *dst, size dst_size) {
    if (!dst || dst_size < 40)
        return false;
    u16 h[8];
    for (i32 i = 0; i < 8; ++i) {
        h[i] = (u16)((bytes[i * 2] << 8) | bytes[i * 2 + 1]);
    }
    i32 best_start = -1;
    i32 best_len   = 0;
    i32 cur_start  = -1;
    i32 cur_len    = 0;
    for (i32 i = 0; i < 8; ++i) {
        if (h[i] == 0) {
            if (cur_start < 0)
                cur_start = i;
            cur_len++;
            if (cur_len > best_len) {
                best_len   = cur_len;
                best_start = cur_start;
            }
        } else {
            cur_start = -1;
            cur_len   = 0;
        }
    }
    if (best_len < 2) {
        best_start = -1;
    }

    size pos            = 0;
    i32  i              = 0;
    bool just_after_run = false;
    while (i < 8) {
        if (i == best_start) {
            if (pos + 2 >= dst_size)
                return false;
            dst[pos++]      = ':';
            dst[pos++]      = ':';
            i              += best_len;
            just_after_run  = true;
            continue;
        }
        if (i > 0 && !just_after_run) {
            if (pos + 1 >= dst_size)
                return false;
            dst[pos++] = ':';
        }
        just_after_run = false;
        if (!append_hextet(dst, dst_size, &pos, h[i]))
            return false;
        ++i;
    }
    if (pos >= dst_size)
        return false;
    dst[pos] = '\0';
    return true;
}

// ---------------------------------------------------------------------------
// Family / kind mappings -- same constants on POSIX & Winsock.
// ---------------------------------------------------------------------------

static i32 sock_kind_to_socktype(SocketKind kind) {
    switch (kind) {
        case SOCKET_KIND_TCP :
            return SOCK_STREAM;
        case SOCKET_KIND_UDP :
            return SOCK_DGRAM;
        default :
            return -1;
    }
}

static i32 sock_kind_to_protocol(SocketKind kind) {
    switch (kind) {
        case SOCKET_KIND_TCP :
            return IPPROTO_TCP;
        case SOCKET_KIND_UDP :
            return IPPROTO_UDP;
        default :
            return 0;
    }
}

static SocketFamily af_to_socket_family(i32 af) {
    switch (af) {
        case AF_INET :
            return SOCKET_FAMILY_INET;
        case AF_INET6 :
            return SOCKET_FAMILY_INET6;
        default :
            return SOCKET_FAMILY_UNSPEC;
    }
}

static i32 socket_family_to_af(SocketFamily f) {
    switch (f) {
        case SOCKET_FAMILY_INET :
            return AF_INET;
        case SOCKET_FAMILY_INET6 :
            return AF_INET6;
        default :
            return AF_UNSPEC;
    }
}

static bool split_host_port(const char *spec, char *host_out, size host_cap, const char **port_out) {
    if (!spec || !host_out || !port_out) {
        return false;
    }

    if (spec[0] == '[') {
        const char *close = NULL;
        for (const char *p = spec + 1; *p; ++p) {
            if (*p == ']') {
                close = p;
                break;
            }
        }
        if (!close || close[1] != ':') {
            return false;
        }
        size host_len = (size)(close - (spec + 1));
        if (host_len + 1 > host_cap) {
            return false;
        }
        MemCopy(host_out, spec + 1, host_len);
        host_out[host_len] = '\0';
        *port_out          = close + 2;
        return true;
    }

    const char *colon = NULL;
    for (const char *p = spec; *p; ++p) {
        if (*p == ':') {
            colon = p;
            break;
        }
    }
    if (!colon) {
        return false;
    }
    size host_len = (size)(colon - spec);
    if (host_len + 1 > host_cap) {
        return false;
    }
    MemCopy(host_out, spec, host_len);
    host_out[host_len] = '\0';
    *port_out          = colon + 1;
    return true;
}

// On-the-wire `sockaddr_in` / `sockaddr_in6` bytes are wire-compatible
// across POSIX and Winsock, but the *struct layout* differs: Darwin /
// BSD prepend a 1-byte `sin_len`, putting `sa_family` (u8) at offset 1.
// Linux and Windows put `sin_family` (u16, host order) at offset 0.
// Reading through `struct sockaddr->sa_family` lets the compiler pick
// the right field for each platform.
static void fill_socket_addr_from_sockaddr(SocketAddr *out, const struct sockaddr *sa, u32 len) {
    MemSet(out, 0, sizeof(*out));
    if (len > (u32)SOCKET_ADDR_MAX_SIZE) {
        len = (u32)SOCKET_ADDR_MAX_SIZE;
    }
    MemCopy(out->raw, sa, (size)len);
    out->length = len;
    out->family = af_to_socket_family((i32)sa->sa_family);
}

// ---------------------------------------------------------------------------
// SocketAddr
// ---------------------------------------------------------------------------

bool SocketAddrParse(SocketAddr *out, const char *spec, SocketKind kind) {
    if (!out) {
        LOG_FATAL("SocketAddrParse: out is NULL");
    }
    MemSet(out, 0, sizeof(*out));

    if (!spec) {
        return false;
    }

    (void)kind;

    char        host[256];
    const char *port_str = NULL;
    if (!split_host_port(spec, host, sizeof(host), &port_str)) {
        return false;
    }

    u16 port = 0;
    if (!parse_port(port_str, &port)) {
        return false;
    }

    u8 v4[4];
    u8 v6[16];
    if (parse_ipv4(host, v4)) {
        struct sockaddr_in *sa = (struct sockaddr_in *)out->raw;
        MemSet(out, 0, sizeof(*out));
        sa->sin_family = AF_INET;
        sa->sin_port   = FROM_BIG_ENDIAN2(port);
        MemCopy(&sa->sin_addr.s_addr, v4, 4);
        out->length = (u32)sizeof(struct sockaddr_in);
        out->family = SOCKET_FAMILY_INET;
        return true;
    }
    if (parse_ipv6(host, v6)) {
        struct sockaddr_in6 *sa = (struct sockaddr_in6 *)out->raw;
        MemSet(out, 0, sizeof(*out));
        sa->sin6_family = AF_INET6;
        sa->sin6_port   = FROM_BIG_ENDIAN2(port);
        // Windows' IN6_ADDR defines `#define s6_addr u.Byte`, so this
        // works the same on both platforms.
        MemCopy(sa->sin6_addr.s6_addr, v6, 16);
        out->length = (u32)sizeof(struct sockaddr_in6);
        out->family = SOCKET_FAMILY_INET6;
        return true;
    }

    return false;
}

Str socket_addr_format(const SocketAddr *addr, Allocator *alloc) {
    Str out = StrInit(alloc);
    if (!addr || addr->length == 0) {
        return out;
    }

    char host[48];
    u16  port = 0;

    const char *host_p = host;
    if (addr->family == SOCKET_FAMILY_INET) {
        const struct sockaddr_in *sa = (const struct sockaddr_in *)addr->raw;
        if (!format_ipv4((const u8 *)&sa->sin_addr.s_addr, host, sizeof(host))) {
            LOG_ERROR("SocketAddrFormat: format_ipv4 failed");
            return out;
        }
        port = FROM_BIG_ENDIAN2(sa->sin_port);
        StrAppendFmt(&out, "{}:{}", host_p, (u32)port);
    } else if (addr->family == SOCKET_FAMILY_INET6) {
        const struct sockaddr_in6 *sa = (const struct sockaddr_in6 *)addr->raw;
        // Windows' IN6_ADDR aliases `s6_addr` via macro.
        const u8 *v6 = sa->sin6_addr.s6_addr;
        if (!format_ipv6(v6, host, sizeof(host))) {
            LOG_ERROR("SocketAddrFormat: format_ipv6 failed");
            return out;
        }
        port = FROM_BIG_ENDIAN2(sa->sin6_port);
        StrAppendFmt(&out, "[{}]:{}", host_p, (u32)port);
    } else {
        LOG_ERROR("SocketAddrFormat: unknown family {}", (u32)addr->family);
    }

    return out;
}

// ---------------------------------------------------------------------------
// Platform-specific syscall shims used by Listener / Socket below.
// Wrapping at this level lets the high-level functions share code.
// ---------------------------------------------------------------------------

#if PLATFORM_WINDOWS

// Returns SOCKET_FD_INVALID on failure. `ret`-passing to LOG_SOCK_ERROR
// is a no-op on Windows (the macro reads WSAGetLastError instead) but
// keeps the signature unified with the POSIX path.
static SockFd plat_socket(int af, int type, int proto) {
    if (!ensure_winsock())
        return SOCKET_FD_INVALID;
    SOCKET s = socket(af, type, proto);
    if (s == INVALID_SOCKET) {
        LOG_SOCK_ERROR(0, "socket() failed");
        return SOCKET_FD_INVALID;
    }
    return socket_to_sf(s);
}

static bool plat_bind(SockFd s, const void *addr, u32 len) {
    if (bind(sf_to_socket(s), (const struct sockaddr *)addr, (int)len) == SOCKET_ERROR) {
        LOG_SOCK_ERROR(0, "bind() failed");
        return false;
    }
    return true;
}

static bool plat_listen(SockFd s, int backlog) {
    if (listen(sf_to_socket(s), backlog) == SOCKET_ERROR) {
        LOG_SOCK_ERROR(0, "listen() failed");
        return false;
    }
    return true;
}

static SockFd plat_accept(SockFd s, void *addr, u32 *len_io) {
    int    sl = (int)*len_io;
    SOCKET c  = accept(sf_to_socket(s), (struct sockaddr *)addr, &sl);
    if (c == INVALID_SOCKET) {
        LOG_SOCK_ERROR(0, "accept() failed");
        return SOCKET_FD_INVALID;
    }
    *len_io = (u32)sl;
    return socket_to_sf(c);
}

static bool plat_connect(SockFd s, const void *addr, u32 len) {
    if (connect(sf_to_socket(s), (const struct sockaddr *)addr, (int)len) == SOCKET_ERROR) {
        LOG_SOCK_ERROR(0, "connect() failed");
        return false;
    }
    return true;
}

static i64 plat_recv(SockFd s, void *buf, size n) {
    int len = (int)((n > (size)0x7FFFFFFF) ? (size)0x7FFFFFFF : n);
    int r   = recv(sf_to_socket(s), (char *)buf, len, 0);
    if (r == SOCKET_ERROR) {
        LOG_SOCK_ERROR(0, "recv() failed");
        return -1;
    }
    return (i64)r;
}

static i64 plat_send(SockFd s, const void *buf, size n) {
    int len = (int)((n > (size)0x7FFFFFFF) ? (size)0x7FFFFFFF : n);
    // No MSG_NOSIGNAL needed -- Winsock doesn't raise SIGPIPE.
    int r = send(sf_to_socket(s), (const char *)buf, len, 0);
    if (r == SOCKET_ERROR) {
        LOG_SOCK_ERROR(0, "send() failed");
        return -1;
    }
    return (i64)r;
}

static bool plat_setsockopt(SockFd s, int level, int optname, const void *optval, u32 optlen) {
    if (setsockopt(sf_to_socket(s), level, optname, (const char *)optval, (int)optlen) == SOCKET_ERROR) {
        LOG_SOCK_ERROR(0, "setsockopt() failed");
        return false;
    }
    return true;
}

static bool plat_getsockname(SockFd s, void *addr, u32 *len_io) {
    int sl = (int)*len_io;
    if (getsockname(sf_to_socket(s), (struct sockaddr *)addr, &sl) == SOCKET_ERROR) {
        LOG_SOCK_ERROR(0, "getsockname() failed");
        return false;
    }
    *len_io = (u32)sl;
    return true;
}

static void plat_close(SockFd s) {
    if (s == SOCKET_FD_INVALID)
        return;
    if (closesocket(sf_to_socket(s)) == SOCKET_ERROR) {
        LOG_SOCK_ERROR(0, "closesocket() failed");
    }
}

static bool plat_set_nonblocking(SockFd s, bool nonblock) {
    u_long mode = nonblock ? 1u : 0u;
    if (ioctlsocket(sf_to_socket(s), FIONBIO, &mode) == SOCKET_ERROR) {
        LOG_SOCK_ERROR(0, "ioctlsocket(FIONBIO) failed");
        return false;
    }
    return true;
}

#else  // POSIX

// Each wrapper captures the syscall's return value into `ret` so we
// can hand it to LOG_SOCK_ERROR -- on the Linux direct-syscall path
// the kernel returns -errno in that value, and ErrnoOf unpacks it
// without going through the libc errno TLS slot. On macOS / non-
// direct-syscall the value is just -1 and ErrnoOf falls back to
// reading errno; both paths land at the same log shape.
static SockFd plat_socket(int af, int type, int proto) {
    long ret = socket(af, type, proto);
    if (ret < 0) {
        LOG_SOCK_ERROR(ret, "socket() failed");
        return SOCKET_FD_INVALID;
    }
    return int_to_sf((int)ret);
}

static bool plat_bind(SockFd s, const void *addr, u32 len) {
    long ret = bind(sf_to_int(s), (const struct sockaddr *)addr, (socklen_t)len);
    if (ret < 0) {
        LOG_SOCK_ERROR(ret, "bind() failed");
        return false;
    }
    return true;
}

static bool plat_listen(SockFd s, int backlog) {
    long ret = listen(sf_to_int(s), backlog);
    if (ret < 0) {
        LOG_SOCK_ERROR(ret, "listen() failed");
        return false;
    }
    return true;
}

static SockFd plat_accept(SockFd s, void *addr, u32 *len_io) {
    socklen_t sl  = (socklen_t)*len_io;
    long      ret = accept(sf_to_int(s), (struct sockaddr *)addr, &sl);
    if (ret < 0) {
        LOG_SOCK_ERROR(ret, "accept() failed");
        return SOCKET_FD_INVALID;
    }
    *len_io = (u32)sl;
    return int_to_sf((int)ret);
}

static bool plat_connect(SockFd s, const void *addr, u32 len) {
    long ret = connect(sf_to_int(s), (const struct sockaddr *)addr, (socklen_t)len);
    if (ret < 0) {
        LOG_SOCK_ERROR(ret, "connect() failed");
        return false;
    }
    return true;
}

static i64 plat_recv(SockFd s, void *buf, size n) {
    long ret = recv(sf_to_int(s), buf, (size)n, 0);
    if (ret < 0) {
        LOG_SOCK_ERROR(ret, "recv() failed");
        return -1;
    }
    return (i64)ret;
}

static i64 plat_send(SockFd s, const void *buf, size n) {
    long ret = send(sf_to_int(s), buf, (size)n, MSG_NOSIGNAL);
    if (ret < 0) {
        LOG_SOCK_ERROR(ret, "send() failed");
        return -1;
    }
    return (i64)ret;
}

static bool plat_setsockopt(SockFd s, int level, int optname, const void *optval, u32 optlen) {
    long ret = setsockopt(sf_to_int(s), level, optname, optval, (socklen_t)optlen);
    if (ret < 0) {
        LOG_SOCK_ERROR(ret, "setsockopt() failed");
        return false;
    }
    return true;
}

static bool plat_getsockname(SockFd s, void *addr, u32 *len_io) {
    socklen_t sl  = (socklen_t)*len_io;
    long      ret = getsockname(sf_to_int(s), (struct sockaddr *)addr, &sl);
    if (ret < 0) {
        LOG_SOCK_ERROR(ret, "getsockname() failed");
        return false;
    }
    *len_io = (u32)sl;
    return true;
}

static void plat_close(SockFd s) {
    if (s == SOCKET_FD_INVALID)
        return;
    long ret = close(sf_to_int(s));
    if (ret < 0) {
        LOG_SOCK_ERROR(ret, "close() failed");
    }
}

static bool plat_set_nonblocking(SockFd s, bool nonblock) {
    long flags = fcntl(sf_to_int(s), F_GETFL, 0);
    if (flags < 0) {
        LOG_SOCK_ERROR(flags, "fcntl(F_GETFL) failed");
        return false;
    }
    if (nonblock) {
        flags |= O_NONBLOCK;
    } else {
        flags &= ~O_NONBLOCK;
    }
    long ret = fcntl(sf_to_int(s), F_SETFL, flags);
    if (ret < 0) {
        LOG_SOCK_ERROR(ret, "fcntl(F_SETFL) failed");
        return false;
    }
    return true;
}

#endif // PLATFORM_WINDOWS

// ---------------------------------------------------------------------------
// Listener
// ---------------------------------------------------------------------------

bool ListenerOpen(Listener *out, SocketKind kind, const SocketAddr *addr, i32 backlog) {
    if (!out || !addr) {
        LOG_FATAL("ListenerOpen: NULL argument");
    }
    MemSet(out, 0, sizeof(*out));
    out->fd = SOCKET_FD_INVALID;

    i32 af       = socket_family_to_af(addr->family);
    i32 socktype = sock_kind_to_socktype(kind);
    i32 proto    = sock_kind_to_protocol(kind);
    if (af == AF_UNSPEC || socktype < 0) {
        LOG_ERROR("ListenerOpen: invalid family/kind combination");
        return false;
    }

    SockFd fd = plat_socket(af, socktype, proto);
    if (fd == SOCKET_FD_INVALID) {
        return false;
    }

    // "Let me restart my server without TIME_WAIT pain" semantics:
    //   - POSIX: SO_REUSEADDR (rebinding allowed; another *process* still
    //     gets EADDRINUSE -- safe).
    //   - Windows: SO_EXCLUSIVEADDRUSE (locks the port to us; SO_REUSEADDR
    //     on Windows lets other processes hijack -- never use it on a
    //     server).
    i32 yes = 1;
#if PLATFORM_WINDOWS
    if (!plat_setsockopt(fd, SOL_SOCKET, SO_EXCLUSIVEADDRUSE, &yes, sizeof(yes))) {
        plat_close(fd);
        return false;
    }
#else
    if (!plat_setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes))) {
        plat_close(fd);
        return false;
    }
#endif

    if (!plat_bind(fd, addr->raw, addr->length)) {
        plat_close(fd);
        return false;
    }

    if (kind == SOCKET_KIND_TCP) {
        if (!plat_listen(fd, backlog > 0 ? backlog : 128)) {
            plat_close(fd);
            return false;
        }
    }

    out->fd    = fd;
    out->kind  = kind;
    out->bound = *addr;
    return true;
}

bool ListenerLocalAddr(const Listener *self, SocketAddr *out) {
    if (!self || !out) {
        LOG_FATAL("ListenerLocalAddr: NULL argument");
    }
    MemSet(out, 0, sizeof(*out));
    u8  buf[SOCKET_ADDR_MAX_SIZE];
    u32 len = (u32)sizeof(buf);
    if (!plat_getsockname(self->fd, buf, &len)) {
        return false;
    }
    fill_socket_addr_from_sockaddr(out, (const struct sockaddr *)buf, len);
    return true;
}

bool ListenerAccept(Listener *self, Socket *out_conn) {
    if (!self || !out_conn) {
        LOG_FATAL("ListenerAccept: NULL argument");
    }
    MemSet(out_conn, 0, sizeof(*out_conn));
    out_conn->fd = SOCKET_FD_INVALID;

    u8     peer[SOCKET_ADDR_MAX_SIZE];
    u32    peer_len = (u32)sizeof(peer);
    SockFd cfd      = plat_accept(self->fd, peer, &peer_len);
    if (cfd == SOCKET_FD_INVALID) {
        return false;
    }

    out_conn->fd   = cfd;
    out_conn->kind = self->kind;
    fill_socket_addr_from_sockaddr(&out_conn->peer, (const struct sockaddr *)peer, peer_len);
    return true;
}

void ListenerClose(Listener *self) {
    if (!self) {
        return;
    }
    if (self->fd != SOCKET_FD_INVALID) {
        plat_close(self->fd);
    }
    MemSet(self, 0, sizeof(*self));
    self->fd = SOCKET_FD_INVALID;
}

// ---------------------------------------------------------------------------
// Socket
// ---------------------------------------------------------------------------

bool SocketConnect(Socket *out, SocketKind kind, const SocketAddr *target) {
    if (!out || !target) {
        LOG_FATAL("SocketConnect: NULL argument");
    }
    MemSet(out, 0, sizeof(*out));
    out->fd = SOCKET_FD_INVALID;

    i32 af       = socket_family_to_af(target->family);
    i32 socktype = sock_kind_to_socktype(kind);
    i32 proto    = sock_kind_to_protocol(kind);
    if (af == AF_UNSPEC || socktype < 0) {
        LOG_ERROR("SocketConnect: invalid family/kind combination");
        return false;
    }

    SockFd fd = plat_socket(af, socktype, proto);
    if (fd == SOCKET_FD_INVALID) {
        return false;
    }

    if (!plat_connect(fd, target->raw, target->length)) {
        plat_close(fd);
        return false;
    }

    out->fd   = fd;
    out->kind = kind;
    out->peer = *target;
    return true;
}

i64 SocketRecv(Socket *self, void *buf, size n) {
    if (!self || !buf) {
        LOG_FATAL("SocketRecv: NULL argument");
    }
    return plat_recv(self->fd, buf, n);
}

i64 SocketSend(Socket *self, const void *buf, size n) {
    if (!self || !buf) {
        LOG_FATAL("SocketSend: NULL argument");
    }
    return plat_send(self->fd, buf, n);
}

void SocketClose(Socket *self) {
    if (!self) {
        return;
    }
    if (self->fd != SOCKET_FD_INVALID) {
        plat_close(self->fd);
    }
    MemSet(self, 0, sizeof(*self));
    self->fd = SOCKET_FD_INVALID;
}

// ---------------------------------------------------------------------------
// Options
// ---------------------------------------------------------------------------

bool SocketSetNonBlocking(SockFd fd, bool nonblock) {
    return plat_set_nonblocking(fd, nonblock);
}

bool SocketSetNoDelay(SockFd fd, bool nodelay) {
    i32 v = nodelay ? 1 : 0;
    return plat_setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &v, sizeof(v));
}

bool SocketSetKeepAlive(SockFd fd, bool keepalive) {
    i32 v = keepalive ? 1 : 0;
    return plat_setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &v, sizeof(v));
}

bool SocketSetReuseAddr(SockFd fd, bool reuse) {
    i32 v = reuse ? 1 : 0;
#if PLATFORM_WINDOWS
    return plat_setsockopt(fd, SOL_SOCKET, SO_EXCLUSIVEADDRUSE, &v, sizeof(v));
#else
    return plat_setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &v, sizeof(v));
#endif
}

bool SocketSetRecvTimeoutMs(SockFd fd, u32 ms) {
#if PLATFORM_WINDOWS
    DWORD tv = (DWORD)ms;
    return plat_setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
#else
    struct timeval tv;
    tv.tv_sec  = (long)(ms / 1000);
    tv.tv_usec = (long)((ms % 1000) * 1000);
    return plat_setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
#endif
}

bool SocketSetSendTimeoutMs(SockFd fd, u32 ms) {
#if PLATFORM_WINDOWS
    DWORD tv = (DWORD)ms;
    return plat_setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
#else
    struct timeval tv;
    tv.tv_sec  = (long)(ms / 1000);
    tv.tv_usec = (long)((ms % 1000) * 1000);
    return plat_setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
#endif
}

// ---------------------------------------------------------------------------
// Multiplexing
// ---------------------------------------------------------------------------

#if PLATFORM_WINDOWS
typedef WSAPOLLFD plat_pollfd_t;
#else
typedef struct pollfd plat_pollfd_t;
#endif

i32 SocketPoll(SocketPollItem *items, u32 count, i32 timeout_ms) {
    if (!items && count > 0) {
        LOG_FATAL("SocketPoll: items is NULL but count > 0");
    }

    enum {
        STACK_MAX = 64
    };
    plat_pollfd_t  stack_pfds[STACK_MAX];
    plat_pollfd_t *pfds = stack_pfds;
    HeapAllocator  halloc;
    bool           used_heap = false;
    if (count > STACK_MAX) {
        halloc = HeapAllocatorInit();
        pfds   = (plat_pollfd_t *)AllocatorAlloc(ALLOCATOR_OF(&halloc), sizeof(plat_pollfd_t) * count, true);
        if (!pfds) {
            HeapAllocatorDeinit(&halloc);
            LOG_ERROR("SocketPoll: heap allocation for pollfd array failed");
            return -1;
        }
        used_heap = true;
    }

    for (u32 i = 0; i < count; ++i) {
#if PLATFORM_WINDOWS
        pfds[i].fd = sf_to_socket(items[i].fd);
#else
        pfds[i].fd = sf_to_int(items[i].fd);
#endif
        pfds[i].events = 0;
        if (items[i].events_requested & SOCKET_POLL_READ) {
            pfds[i].events |= POLLIN;
        }
        if (items[i].events_requested & SOCKET_POLL_WRITE) {
            pfds[i].events |= POLLOUT;
        }
        pfds[i].revents = 0;
    }

    i32 ret;
#if PLATFORM_WINDOWS
    // WSAPoll on Win10 2004+ is correct; older Windows had a bug on
    // failed connect that we'd need a select() fallback for. We accept
    // the modern-Windows-only constraint.
    ret = (i32)WSAPoll(pfds, (ULONG)count, timeout_ms);
    if (ret == SOCKET_ERROR) {
        LOG_SOCK_ERROR(0, "WSAPoll() failed");
        ret = -1;
    }
#else
    // Linux direct-syscall path returns -errno directly; macOS / BSD
    // libSystem poll() returns -1 and sets errno. Both branches walk
    // through here, so check whichever signal is meaningful for the
    // platform without going through `errno` if we don't have to.
    do {
        ret = poll(pfds, (nfds_t)count, timeout_ms);
#    if FEATURE_DIRECT_SYSCALL
    } while (ret == -EINTR);
#    else
    } while (ret < 0 && Errno() == EINTR);
#    endif
    if (ret < 0) {
        LOG_SOCK_ERROR(ret, "poll() failed");
    }
#endif

    if (ret >= 0) {
        for (u32 i = 0; i < count; ++i) {
            u32 ready = 0;
            if (pfds[i].revents & POLLIN) {
                ready |= SOCKET_POLL_READ;
            }
            if (pfds[i].revents & POLLOUT) {
                ready |= SOCKET_POLL_WRITE;
            }
            if (pfds[i].revents & (POLLERR | POLLHUP | POLLNVAL)) {
                ready |= SOCKET_POLL_ERROR;
            }
            items[i].events_ready = ready;
        }
    }

    if (used_heap) {
        AllocatorFree(ALLOCATOR_OF(&halloc), pfds);
        HeapAllocatorDeinit(&halloc);
    }
    return ret;
}
