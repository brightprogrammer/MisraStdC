/// file      : sys/socket.c
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
// Win64. Call sites inline the `(SOCKET)fd` / `(SockFd)s` casts
// directly -- the conversion is the entirety of the operation, so
// hiding it behind a helper would be an alias wrapper.

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
// `poll` is the only POSIX socket primitive that needs more than a
// direct misra_sys* call: Linux aarch64 dropped the legacy `poll`
// syscall in favor of `ppoll(fds, nfds, ts, sigmask, sizeof(sigmask))`.
// Darwin and Linux x86_64 keep the legacy 3-arg shape.
static inline long sock_poll(void *pfds, unsigned long nfds, int timeout_ms) {
#        if PLATFORM_DARWIN || ARCHITECTURE_X86_64
    return direct_sys3(MISRA_SYS_poll, (long)(u64)pfds, (long)nfds, (long)timeout_ms);
#        else
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
    return direct_sys5(MISRA_SYS_ppoll, (long)(u64)pfds, (long)nfds, (long)(u64)ts_ptr, 0, 0);
#        endif
}
#    endif

// POSIX side: `SockFd` is a thin alias for `int`. Call sites inline
// the `(int)fd` / `(SockFd)s` casts directly -- hiding a zero-width
// conversion behind a helper would be an alias wrapper.

// POSIX: ret is the failing syscall's return value. On Linux direct-
// syscall it carries -errno directly; on libSystem (macOS) the value
// is -1 and errno is set. ErrnoOf papers over the difference.
#    define LOG_SOCK_ERROR(ret, msg) LOG_SYS_ERROR(ErrnoOf(ret), msg)
#endif // PLATFORM_WINDOWS

// ---------------------------------------------------------------------------
// Pure-C parsers / formatters. No platform dependencies -- shared.
// ---------------------------------------------------------------------------

#include "_IpParse.h"

// Parse a decimal port number (0..65535) from a NUL-terminated string.
// Empty / non-numeric input -> false. Out-of-range -> false.
static bool parse_port(Zstr s, u16 *out) {
    if (!s || !*s)
        return false;
    u32 v = 0;
    for (Zstr p = s; *p; ++p) {
        if (*p < '0' || *p > '9')
            return false;
        v = v * 10 + (u32)(*p - '0');
        if (v > 0xFFFFu)
            return false;
    }
    *out = (u16)v;
    return true;
}

static bool format_ipv4(const u8 octets[4], char *dst, size dst_size) {
    if (!dst || dst_size < 16)
        return false;
    size pos = 0;
    for (i32 i = 0; i < 4; ++i) {
        u32  v       = octets[i];
        bool iter_ok = true;
        StrInitStack(tmp, 4) {
            char *data = StrBegin(&tmp);
            i32   n    = 0;
            if (v == 0) {
                data[n++] = '0';
            } else {
                while (v) {
                    data[n++]  = (char)('0' + (v % 10));
                    v         /= 10;
                }
            }
            while (n--) {
                if (pos + 1 >= dst_size) {
                    iter_ok = false;
                    break;
                }
                dst[pos++] = data[n];
            }
        }
        if (!iter_ok)
            return false;
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
    bool ok = true;
    StrInitStack(tmp, 4) {
        char *data = StrBegin(&tmp);
        i32   n    = 0;
        if (v == 0) {
            data[n++] = '0';
        } else {
            while (v) {
                u8 nib      = v & 0xF;
                data[n++]   = nib < 10 ? (char)('0' + nib) : (char)('a' + nib - 10);
                v         >>= 4;
            }
        }
        while (n--) {
            if (*pos + 1 >= dst_size) {
                ok = false;
                break;
            }
            dst[(*pos)++] = data[n];
        }
    }
    return ok;
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

static bool split_host_port(Zstr spec, char *host_out, size host_cap, Zstr *port_out) {
    if (!spec || !host_out || !port_out) {
        return false;
    }

    if (spec[0] == '[') {
        Zstr close = NULL;
        for (Zstr p = spec + 1; *p; ++p) {
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

    Zstr colon = NULL;
    for (Zstr p = spec; *p; ++p) {
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

bool socket_addr_parse_zstr(SocketAddr *out, Zstr spec, SocketKind kind) {
    if (!out) {
        LOG_FATAL("SocketAddrParse: out is NULL");
    }
    MemSet(out, 0, sizeof(*out));

    if (!spec) {
        return false;
    }

    (void)kind;

    bool ok = false;
    StrInitStack(host, 256) {
        char *host_data = StrBegin(&host);
        Zstr  port_str  = NULL;
        if (!split_host_port(spec, host_data, 256, &port_str)) {
            break;
        }
        // split_host_port NUL-terminates; record the textual length so the
        // Str is internally consistent before any read via StrBegin.
        StrResize(&host, ZstrLen(host_data));

        u16 port = 0;
        if (!parse_port(port_str, &port)) {
            break;
        }

        u8 v4[4];
        u8 v6[16];
        if (parse_ipv4(host_data, v4)) {
            struct sockaddr_in *sa = (struct sockaddr_in *)out->raw;
            MemSet(out, 0, sizeof(*out));
            sa->sin_family = AF_INET;
            sa->sin_port   = FROM_BIG_ENDIAN2(port);
            MemCopy(&sa->sin_addr.s_addr, v4, 4);
            out->length = (u32)sizeof(struct sockaddr_in);
            out->family = SOCKET_FAMILY_INET;
            ok          = true;
            break;
        }
        if (parse_ipv6(host_data, v6)) {
            struct sockaddr_in6 *sa = (struct sockaddr_in6 *)out->raw;
            MemSet(out, 0, sizeof(*out));
            sa->sin6_family = AF_INET6;
            sa->sin6_port   = FROM_BIG_ENDIAN2(port);
            // Windows' IN6_ADDR defines `#define s6_addr u.Byte`, so this
            // works the same on both platforms.
            MemCopy(sa->sin6_addr.s6_addr, v6, 16);
            out->length = (u32)sizeof(struct sockaddr_in6);
            out->family = SOCKET_FAMILY_INET6;
            ok          = true;
            break;
        }
    }

    return ok;
}

bool socket_addr_parse_str(SocketAddr *out, const Str *spec, SocketKind kind) {
    if (!out) {
        LOG_FATAL("SocketAddrParse: out is NULL");
    }
    MemSet(out, 0, sizeof(*out));

    if (!spec) {
        return false;
    }

    // Str values in this codebase are NUL-terminated by construction; the
    // numeric parsers (parse_ipv4 / parse_ipv6) and split_host_port scan
    // until ']' / ':' / '\0', so dispatching via .data preserves identical
    // semantics for non-degenerate input. The empty-spec edge case is
    // already handled by the zstr arm (returns false on no colon).
    return socket_addr_parse_zstr(out, StrBegin(spec), kind);
}

Str socket_addr_format(const SocketAddr *addr, Allocator *alloc) {
    Str out = StrInit(alloc);
    if (!addr || addr->length == 0) {
        return out;
    }

    StrInitStack(host, 48) {
        char *host_data = StrBegin(&host);
        u16   port      = 0;
        if (addr->family == SOCKET_FAMILY_INET) {
            const struct sockaddr_in *sa = (const struct sockaddr_in *)addr->raw;
            if (!format_ipv4((const u8 *)&sa->sin_addr.s_addr, host_data, 48)) {
                LOG_ERROR("SocketAddrFormat: format_ipv4 failed");
                break;
            }
            StrResize(&host, ZstrLen(host_data));
            port = FROM_BIG_ENDIAN2(sa->sin_port);
            StrAppendFmt(&out, "{}:{}", (Zstr)host_data, (u32)port);
        } else if (addr->family == SOCKET_FAMILY_INET6) {
            const struct sockaddr_in6 *sa = (const struct sockaddr_in6 *)addr->raw;
            // Windows' IN6_ADDR aliases `s6_addr` via macro.
            const u8 *v6 = sa->sin6_addr.s6_addr;
            if (!format_ipv6(v6, host_data, 48)) {
                LOG_ERROR("SocketAddrFormat: format_ipv6 failed");
                break;
            }
            StrResize(&host, ZstrLen(host_data));
            port = FROM_BIG_ENDIAN2(sa->sin6_port);
            StrAppendFmt(&out, "[{}]:{}", (Zstr)host_data, (u32)port);
        } else {
            LOG_ERROR("SocketAddrFormat: unknown family {}", (u32)addr->family);
        }
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
    return (SockFd)s;
}

static bool plat_bind(SockFd s, const void *addr, u32 len) {
    if (bind((SOCKET)s, (const struct sockaddr *)addr, (int)len) == SOCKET_ERROR) {
        LOG_SOCK_ERROR(0, "bind() failed");
        return false;
    }
    return true;
}

static bool plat_listen(SockFd s, int backlog) {
    if (listen((SOCKET)s, backlog) == SOCKET_ERROR) {
        LOG_SOCK_ERROR(0, "listen() failed");
        return false;
    }
    return true;
}

static SockFd plat_accept(SockFd s, void *addr, u32 *len_io) {
    int    sl = (int)*len_io;
    SOCKET c  = accept((SOCKET)s, (struct sockaddr *)addr, &sl);
    if (c == INVALID_SOCKET) {
        LOG_SOCK_ERROR(0, "accept() failed");
        return SOCKET_FD_INVALID;
    }
    *len_io = (u32)sl;
    return (SockFd)c;
}

static bool plat_connect(SockFd s, const void *addr, u32 len) {
    if (connect((SOCKET)s, (const struct sockaddr *)addr, (int)len) == SOCKET_ERROR) {
        LOG_SOCK_ERROR(0, "connect() failed");
        return false;
    }
    return true;
}

static i64 plat_recv(SockFd s, void *buf, size n) {
    int len = (int)((n > (size)0x7FFFFFFF) ? (size)0x7FFFFFFF : n);
    int r   = recv((SOCKET)s, (char *)buf, len, 0);
    if (r == SOCKET_ERROR) {
        LOG_SOCK_ERROR(0, "recv() failed");
        return -1;
    }
    return (i64)r;
}

static i64 plat_send(SockFd s, const void *buf, size n) {
    int len = (int)((n > (size)0x7FFFFFFF) ? (size)0x7FFFFFFF : n);
    // No MSG_NOSIGNAL needed -- Winsock doesn't raise SIGPIPE.
    int r = send((SOCKET)s, (Zstr)buf, len, 0);
    if (r == SOCKET_ERROR) {
        LOG_SOCK_ERROR(0, "send() failed");
        return -1;
    }
    return (i64)r;
}

static bool plat_setsockopt(SockFd s, int level, int optname, const void *optval, u32 optlen) {
    if (setsockopt((SOCKET)s, level, optname, (Zstr)optval, (int)optlen) == SOCKET_ERROR) {
        LOG_SOCK_ERROR(0, "setsockopt() failed");
        return false;
    }
    return true;
}

static bool plat_getsockname(SockFd s, void *addr, u32 *len_io) {
    int sl = (int)*len_io;
    if (getsockname((SOCKET)s, (struct sockaddr *)addr, &sl) == SOCKET_ERROR) {
        LOG_SOCK_ERROR(0, "getsockname() failed");
        return false;
    }
    *len_io = (u32)sl;
    return true;
}

static void plat_close(SockFd s) {
    if (s == SOCKET_FD_INVALID)
        return;
    if (closesocket((SOCKET)s) == SOCKET_ERROR) {
        LOG_SOCK_ERROR(0, "closesocket() failed");
    }
}

static bool plat_set_nonblocking(SockFd s, bool nonblock) {
    u_long mode = nonblock ? 1u : 0u;
    if (ioctlsocket((SOCKET)s, FIONBIO, &mode) == SOCKET_ERROR) {
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
    long ret = direct_sys3(MISRA_SYS_socket, (long)af, (long)type, (long)proto);
    if (ret < 0) {
        LOG_SOCK_ERROR(ret, "socket() failed");
        return SOCKET_FD_INVALID;
    }
    return (SockFd)ret;
}

static bool plat_bind(SockFd s, const void *addr, u32 len) {
    long ret =
        direct_sys3(MISRA_SYS_bind, (long)(int)s, (long)(u64)(const struct sockaddr *)addr, (long)(socklen_t)len);
    if (ret < 0) {
        LOG_SOCK_ERROR(ret, "bind() failed");
        return false;
    }
    return true;
}

static bool plat_listen(SockFd s, int backlog) {
    long ret = direct_sys2(MISRA_SYS_listen, (long)(int)s, (long)backlog);
    if (ret < 0) {
        LOG_SOCK_ERROR(ret, "listen() failed");
        return false;
    }
    return true;
}

static SockFd plat_accept(SockFd s, void *addr, u32 *len_io) {
    socklen_t sl  = (socklen_t)*len_io;
    long      ret = direct_sys3(MISRA_SYS_accept, (long)(int)s, (long)(u64)(struct sockaddr *)addr, (long)(u64)&sl);
    if (ret < 0) {
        LOG_SOCK_ERROR(ret, "accept() failed");
        return SOCKET_FD_INVALID;
    }
    *len_io = (u32)sl;
    return (SockFd)ret;
}

static bool plat_connect(SockFd s, const void *addr, u32 len) {
    long ret =
        direct_sys3(MISRA_SYS_connect, (long)(int)s, (long)(u64)(const struct sockaddr *)addr, (long)(socklen_t)len);
    if (ret < 0) {
        LOG_SOCK_ERROR(ret, "connect() failed");
        return false;
    }
    return true;
}

static i64 plat_recv(SockFd s, void *buf, size n) {
    long ret = direct_sys6(MISRA_SYS_recvfrom, (long)(int)s, (long)(u64)buf, (long)(size)n, 0, 0, 0);
    if (ret < 0) {
        LOG_SOCK_ERROR(ret, "recv() failed");
        return -1;
    }
    return (i64)ret;
}

static i64 plat_send(SockFd s, const void *buf, size n) {
    long ret = direct_sys6(MISRA_SYS_sendto, (long)(int)s, (long)(u64)buf, (long)(size)n, (long)MSG_NOSIGNAL, 0, 0);
    if (ret < 0) {
        LOG_SOCK_ERROR(ret, "send() failed");
        return -1;
    }
    return (i64)ret;
}

static bool plat_setsockopt(SockFd s, int level, int optname, const void *optval, u32 optlen) {
    long ret = direct_sys5(
        MISRA_SYS_setsockopt,
        (long)(int)s,
        (long)level,
        (long)optname,
        (long)(u64)optval,
        (long)(socklen_t)optlen
    );
    if (ret < 0) {
        LOG_SOCK_ERROR(ret, "setsockopt() failed");
        return false;
    }
    return true;
}

static bool plat_getsockname(SockFd s, void *addr, u32 *len_io) {
    socklen_t sl = (socklen_t)*len_io;
    long ret     = direct_sys3(MISRA_SYS_getsockname, (long)(int)s, (long)(u64)(struct sockaddr *)addr, (long)(u64)&sl);
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
    long ret = direct_sys1(MISRA_SYS_close, (long)(int)s);
    if (ret < 0) {
        LOG_SOCK_ERROR(ret, "close() failed");
    }
}

static bool plat_set_nonblocking(SockFd s, bool nonblock) {
    long flags = direct_sys3(MISRA_SYS_fcntl, (long)(int)s, (long)F_GETFL, 0);
    if (flags < 0) {
        LOG_SOCK_ERROR(flags, "fcntl(F_GETFL) failed");
        return false;
    }
    if (nonblock) {
        flags |= O_NONBLOCK;
    } else {
        flags &= ~O_NONBLOCK;
    }
    long ret = direct_sys3(MISRA_SYS_fcntl, (long)(int)s, (long)F_SETFL, flags);
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
        pfds   = (plat_pollfd_t *)AllocatorAlloc(&halloc, sizeof(plat_pollfd_t) * count, true);
        if (!pfds) {
            HeapAllocatorDeinit(&halloc);
            LOG_ERROR("SocketPoll: heap allocation for pollfd array failed");
            return -1;
        }
        used_heap = true;
    }

    for (u32 i = 0; i < count; ++i) {
#if PLATFORM_WINDOWS
        pfds[i].fd = (SOCKET)items[i].fd;
#else
        pfds[i].fd = (int)items[i].fd;
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
    // WSAPoll rejects nfds == 0 with WSAEINVAL (POSIX poll() waits and
    // returns 0). Nothing to wait on means nothing is ready: return 0.
    if (count == 0) {
        return 0;
    }
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
        ret = sock_poll(pfds, (unsigned long)count, timeout_ms);
#    if FEATURE_DIRECT_SYSCALL
    } while (ret == -EINTR);
#    else
    } while (ret < 0 && Errno() == EINTR);
#    endif
    if (ret < 0) {
        LOG_SOCK_ERROR(ret, "poll() failed");
    }
#endif

    // --- TEMP DIAGNOSTIC (remove after CI) ---
    LOG_ERROR("SOCKPOLLDBG count={} timeout={} ret={}", count, timeout_ms, ret);
    for (u32 di = 0; di < count && di < 8; ++di) {
        LOG_ERROR(
            "SOCKPOLLDBG   fd[{}]={} events={x} revents={x}",
            di,
            (i64)items[di].fd,
            (u32)pfds[di].events,
            (u32)pfds[di].revents
        );
    }

    if (ret >= 0) {
        for (u32 i = 0; i < count; ++i) {
            u32 ready = 0;
            if (pfds[i].revents & POLLIN) {
                ready |= SOCKET_POLL_READ;
            }
#if PLATFORM_WINDOWS
            // WSAPoll flags a closed/reset peer with POLLHUP but, unlike
            // POSIX poll(), without POLLRDNORM -- yet the socket is still
            // readable (a read drains any buffered bytes, then returns 0
            // for EOF). Surface POLLHUP as read-ready so callers see the
            // same "peer closed -> readable" signal they get on POSIX.
            if (pfds[i].revents & POLLHUP) {
                ready |= SOCKET_POLL_READ;
            }
#endif
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
        AllocatorFree(&halloc, pfds);
        HeapAllocatorDeinit(&halloc);
    }
    return ret;
}
