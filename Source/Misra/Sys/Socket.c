/// file      : Socket.c
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// POSIX implementation of the `Sys/Socket` module. Windows is not
/// supported yet; building with `sys_socket=true` on Windows is a
/// compile-time error.

#define _DEFAULT_SOURCE
#define _POSIX_C_SOURCE 200809L

#include <Misra/Sys/Socket.h>

#ifdef _WIN32
#    error "Sys/Socket on Windows is not implemented yet; set sys_socket=false"
#endif

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <poll.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

#include <Misra/Std.h>
#include <Misra/Std/Log.h>

#include "../_Syscall.h"

#include <stdint.h>

#if MISRA_HAVE_DIRECT_SYSCALL
// Linux: direct-syscall wrappers for the BSD-sockets primitives used
// below. `recv` / `send` are mapped onto `recvfrom` / `sendto` with
// the addr arguments cleared since the kernel offers no separate
// syscall for them. macOS / BSD keep libSystem; Windows takes a
// completely different code path (Winsock isn't ported yet anyway).

static inline long misra_sock_socket(int domain, int type, int protocol) {
    return misra_sys3(MISRA_SYS_socket, (long)domain, (long)type, (long)protocol);
}
static inline long misra_sock_bind(int fd, const void *addr, unsigned addrlen) {
    return misra_sys3(MISRA_SYS_bind, (long)fd, (long)(uintptr_t)addr, (long)addrlen);
}
static inline long misra_sock_connect(int fd, const void *addr, unsigned addrlen) {
    return misra_sys3(MISRA_SYS_connect, (long)fd, (long)(uintptr_t)addr, (long)addrlen);
}
static inline long misra_sock_listen(int fd, int backlog) {
    return misra_sys2(MISRA_SYS_listen, (long)fd, (long)backlog);
}
static inline long misra_sock_accept(int fd, void *addr, void *addrlen) {
    return misra_sys3(MISRA_SYS_accept, (long)fd, (long)(uintptr_t)addr, (long)(uintptr_t)addrlen);
}
static inline long misra_sock_recv(int fd, void *buf, unsigned long n, int flags) {
    // recvfrom(fd, buf, n, flags, src_addr=NULL, addrlen=NULL).
    return misra_sys6(MISRA_SYS_recvfrom, (long)fd, (long)(uintptr_t)buf, (long)n, (long)flags, 0, 0);
}
static inline long misra_sock_send(int fd, const void *buf, unsigned long n, int flags) {
    // sendto(fd, buf, n, flags, dest_addr=NULL, addrlen=0).
    return misra_sys6(MISRA_SYS_sendto, (long)fd, (long)(uintptr_t)buf, (long)n, (long)flags, 0, 0);
}
static inline long misra_sock_setsockopt(int fd, int level, int optname, const void *optval, unsigned optlen) {
    return misra_sys5(
        MISRA_SYS_setsockopt,
        (long)fd,
        (long)level,
        (long)optname,
        (long)(uintptr_t)optval,
        (long)optlen
    );
}
static inline long misra_sock_close(int fd) {
    return misra_sys1(MISRA_SYS_close, (long)fd);
}
static inline long misra_sock_fcntl(int fd, int cmd, long arg) {
    return misra_sys3(MISRA_SYS_fcntl, (long)fd, (long)cmd, arg);
}
static inline long misra_sock_poll(void *pfds, unsigned long nfds, int timeout_ms) {
#    if defined(__x86_64__)
    return misra_sys3(MISRA_SYS_poll, (long)(uintptr_t)pfds, (long)nfds, (long)timeout_ms);
#    else
    // aarch64 dropped poll for ppoll(fds, nfds, ts, sigmask, sizeof(sigmask)).
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
    return misra_sys5(MISRA_SYS_ppoll, (long)(uintptr_t)pfds, (long)nfds, (long)(uintptr_t)ts_ptr, 0, 0);
#    endif
}

#    define socket(d, t, p)              ((i32)misra_sock_socket((d), (t), (p)))
#    define bind(fd, a, l)               ((int)misra_sock_bind((fd), (a), (unsigned)(l)))
#    define connect(fd, a, l)            ((int)misra_sock_connect((fd), (a), (unsigned)(l)))
#    define listen(fd, b)                ((int)misra_sock_listen((fd), (b)))
#    define accept(fd, a, l)             ((i32)misra_sock_accept((fd), (a), (l)))
#    define recv(fd, b, n, f)            ((long)misra_sock_recv((fd), (b), (unsigned long)(n), (f)))
#    define send(fd, b, n, f)            ((long)misra_sock_send((fd), (b), (unsigned long)(n), (f)))
#    define setsockopt(fd, lv, on, v, l) ((int)misra_sock_setsockopt((fd), (lv), (on), (v), (unsigned)(l)))
#    define close(fd)                    ((int)misra_sock_close(fd))
#    define fcntl(fd, cmd, arg)          ((int)misra_sock_fcntl((fd), (cmd), (long)(arg)))
#    define poll(pfds, n, t)             ((int)misra_sock_poll((pfds), (unsigned long)(n), (t)))
#endif

// getaddrinfo() returns one of the EAI_* codes; libc gai_strerror
// maps them to a description. Doing it in-tree avoids a libc symbol
// and keeps the description consistent with how StrError formats
// errno values elsewhere.
static const char *gai_description(i32 gai) {
    switch (gai) {
#ifdef EAI_BADFLAGS
        case EAI_BADFLAGS :
            return "Bad value for ai_flags";
#endif
#ifdef EAI_NONAME
        case EAI_NONAME :
            return "Name or service not known";
#endif
#ifdef EAI_AGAIN
        case EAI_AGAIN :
            return "Temporary failure in name resolution";
#endif
#ifdef EAI_FAIL
        case EAI_FAIL :
            return "Non-recoverable failure in name resolution";
#endif
#ifdef EAI_FAMILY
        case EAI_FAMILY :
            return "ai_family not supported";
#endif
#ifdef EAI_SOCKTYPE
        case EAI_SOCKTYPE :
            return "ai_socktype not supported";
#endif
#ifdef EAI_SERVICE
        case EAI_SERVICE :
            return "Servname not supported for ai_socktype";
#endif
#ifdef EAI_MEMORY
        case EAI_MEMORY :
            return "Memory allocation failure";
#endif
#ifdef EAI_SYSTEM
        case EAI_SYSTEM :
            return "System error (see errno)";
#endif
#ifdef EAI_OVERFLOW
        case EAI_OVERFLOW :
            return "Argument buffer overflow";
#endif
#ifdef EAI_NODATA
        case EAI_NODATA :
            return "No address associated with hostname";
#endif
#ifdef EAI_ADDRFAMILY
        case EAI_ADDRFAMILY :
            return "Address family for hostname not supported";
#endif
        default :
            return "Unknown getaddrinfo error";
    }
}

// Format an IPv4 dotted-quad ("a.b.c.d") into `dst`. `dst_size` must
// be at least 16 to hold the longest "255.255.255.255\0".
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

// Append one hextet (0..0xFFFF) in lowercase hex, no leading zeros.
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

// Format an IPv6 address into `dst`. Implements the RFC 5952 "::"
// compression: the longest run of >= 2 zero hextets is collapsed,
// ties go to the leftmost run. `dst_size` must be at least 40
// (8 hextets * 4 hex + 7 colons + 1 NUL = 40).
static bool format_ipv6(const u8 bytes[16], char *dst, size dst_size) {
    if (!dst || dst_size < 40)
        return false;
    u16 h[8];
    for (i32 i = 0; i < 8; ++i) {
        h[i] = (u16)((bytes[i * 2] << 8) | bytes[i * 2 + 1]);
    }
    // Find longest zero run of length >= 2.
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
            // Emit the "::" marker. Its two colons subsume both the
            // colon before the run (if there was a preceding hextet)
            // and the colon after the run (if there is a following
            // hextet). We mark `just_after_run` so the next iteration
            // doesn't add another ':' on top.
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
// Internal helpers
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

// Split `spec` into host + port. Handles bracketed IPv6 form
// `[host]:port` so addresses containing ':' do not confuse the parse.
// Returns false if no port separator is found.
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

static void fill_socket_addr_from_sockaddr(SocketAddr *out, const struct sockaddr *sa, socklen_t len) {
    MemSet(out, 0, sizeof(*out));
    if (len > (socklen_t)SOCKET_ADDR_MAX_SIZE) {
        len = (socklen_t)SOCKET_ADDR_MAX_SIZE;
    }
    MemCopy(out->raw, sa, (size)len);
    out->length = (u32)len;
    out->family = af_to_socket_family(sa->sa_family);
}

// ---------------------------------------------------------------------------
// SocketAddr
// ---------------------------------------------------------------------------

bool SocketAddrParse(SocketAddr *out, const char *spec, SocketKind kind) {
    if (!out) {
        LOG_ERROR("SocketAddrParse: out is NULL");
        return false;
    }
    MemSet(out, 0, sizeof(*out));

    if (!spec) {
        LOG_ERROR("SocketAddrParse: spec is NULL");
        return false;
    }

    char        host[256];
    const char *port = NULL;
    if (!split_host_port(spec, host, sizeof(host), &port)) {
        LOG_ERROR("SocketAddrParse: cannot split host:port in \"{}\"", spec);
        return false;
    }

    struct addrinfo hints;
    MemSet(&hints, 0, sizeof(hints));
    hints.ai_family   = AF_UNSPEC;
    hints.ai_socktype = sock_kind_to_socktype(kind);
    hints.ai_protocol = sock_kind_to_protocol(kind);
    hints.ai_flags    = AI_ADDRCONFIG;

    struct addrinfo *res    = NULL;
    i32              gairet = getaddrinfo(host, port, &hints, &res);
    if (gairet != 0) {
        LOG_ERROR("SocketAddrParse: getaddrinfo(\"{}\") failed: {}", spec, gai_description(gairet));
        return false;
    }
    if (!res) {
        LOG_ERROR("SocketAddrParse: getaddrinfo(\"{}\") returned no results", spec);
        return false;
    }

    fill_socket_addr_from_sockaddr(out, res->ai_addr, res->ai_addrlen);
    freeaddrinfo(res);
    return true;
}

Str SocketAddrFormat(const SocketAddr *addr, Allocator *alloc) {
    Str out = StrInit(alloc);
    if (!addr || addr->length == 0) {
        return out;
    }

    char host[48]; // longest IPv6 (39) + scope id slack
    u16  port = 0;

    const char *host_p = host;
    if (addr->family == SOCKET_FAMILY_INET) {
        const struct sockaddr_in *sa = (const struct sockaddr_in *)addr->raw;
        if (!format_ipv4((const u8 *)&sa->sin_addr.s_addr, host, sizeof(host))) {
            LOG_ERROR("SocketAddrFormat: format_ipv4 failed");
            return out;
        }
        port = FROM_BIG_ENDIAN2(sa->sin_port);
        StrWriteFmt(&out, "{}:{}", host_p, (u32)port);
    } else if (addr->family == SOCKET_FAMILY_INET6) {
        const struct sockaddr_in6 *sa = (const struct sockaddr_in6 *)addr->raw;
        if (!format_ipv6(sa->sin6_addr.s6_addr, host, sizeof(host))) {
            LOG_ERROR("SocketAddrFormat: format_ipv6 failed");
            return out;
        }
        port = FROM_BIG_ENDIAN2(sa->sin6_port);
        StrWriteFmt(&out, "[{}]:{}", host_p, (u32)port);
    } else {
        LOG_ERROR("SocketAddrFormat: unknown family {}", (u32)addr->family);
    }

    return out;
}

// ---------------------------------------------------------------------------
// Listener
// ---------------------------------------------------------------------------

bool ListenerOpen(Listener *out, SocketKind kind, const SocketAddr *addr, i32 backlog) {
    if (!out || !addr) {
        LOG_ERROR("ListenerOpen: NULL argument");
        return false;
    }
    MemSet(out, 0, sizeof(*out));

    i32 af       = socket_family_to_af(addr->family);
    i32 socktype = sock_kind_to_socktype(kind);
    i32 proto    = sock_kind_to_protocol(kind);
    if (af == AF_UNSPEC || socktype < 0) {
        LOG_ERROR("ListenerOpen: invalid family/kind combination");
        return false;
    }

    i32 fd = socket(af, socktype, proto);
    if (fd < 0) {
        LOG_SYS_ERROR("ListenerOpen: socket() failed");
        return false;
    }

    i32 yes = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) < 0) {
        LOG_SYS_ERROR("ListenerOpen: setsockopt(SO_REUSEADDR) failed");
        close(fd);
        return false;
    }

    if (bind(fd, (const struct sockaddr *)addr->raw, (socklen_t)addr->length) < 0) {
        LOG_SYS_ERROR("ListenerOpen: bind() failed");
        close(fd);
        return false;
    }

    if (kind == SOCKET_KIND_TCP) {
        if (listen(fd, backlog > 0 ? backlog : 128) < 0) {
            LOG_SYS_ERROR("ListenerOpen: listen() failed");
            close(fd);
            return false;
        }
    }

    out->fd    = fd;
    out->kind  = kind;
    out->bound = *addr;
    return true;
}

bool ListenerAccept(Listener *self, Socket *out_conn) {
    if (!self || !out_conn) {
        LOG_ERROR("ListenerAccept: NULL argument");
        return false;
    }
    MemSet(out_conn, 0, sizeof(*out_conn));

    struct sockaddr_storage peer;
    socklen_t               peer_len = sizeof(peer);
    i32                     cfd      = accept(self->fd, (struct sockaddr *)&peer, &peer_len);
    if (cfd < 0) {
        LOG_SYS_ERROR("ListenerAccept: accept() failed");
        return false;
    }

    out_conn->fd   = cfd;
    out_conn->kind = self->kind;
    fill_socket_addr_from_sockaddr(&out_conn->peer, (const struct sockaddr *)&peer, peer_len);
    return true;
}

void ListenerClose(Listener *self) {
    if (!self) {
        return;
    }
    if (self->fd > 0) {
        if (close(self->fd) < 0) {
            LOG_SYS_ERROR("ListenerClose: close() failed");
        }
    }
    MemSet(self, 0, sizeof(*self));
}

// ---------------------------------------------------------------------------
// Socket
// ---------------------------------------------------------------------------

bool SocketConnect(Socket *out, SocketKind kind, const SocketAddr *target) {
    if (!out || !target) {
        LOG_ERROR("SocketConnect: NULL argument");
        return false;
    }
    MemSet(out, 0, sizeof(*out));

    i32 af       = socket_family_to_af(target->family);
    i32 socktype = sock_kind_to_socktype(kind);
    i32 proto    = sock_kind_to_protocol(kind);
    if (af == AF_UNSPEC || socktype < 0) {
        LOG_ERROR("SocketConnect: invalid family/kind combination");
        return false;
    }

    i32 fd = socket(af, socktype, proto);
    if (fd < 0) {
        LOG_SYS_ERROR("SocketConnect: socket() failed");
        return false;
    }

    if (connect(fd, (const struct sockaddr *)target->raw, (socklen_t)target->length) < 0) {
        LOG_SYS_ERROR("SocketConnect: connect() failed");
        close(fd);
        return false;
    }

    out->fd   = fd;
    out->kind = kind;
    out->peer = *target;
    return true;
}

i64 SocketRecv(Socket *self, void *buf, size n) {
    if (!self || !buf) {
        LOG_ERROR("SocketRecv: NULL argument");
        return -1;
    }
    ssize_t ret = recv(self->fd, buf, (size_t)n, 0);
    if (ret < 0) {
        LOG_SYS_ERROR("SocketRecv: recv() failed");
        return -1;
    }
    return (i64)ret;
}

i64 SocketSend(Socket *self, const void *buf, size n) {
    if (!self || !buf) {
        LOG_ERROR("SocketSend: NULL argument");
        return -1;
    }
    ssize_t ret = send(self->fd, buf, (size_t)n, MSG_NOSIGNAL);
    if (ret < 0) {
        LOG_SYS_ERROR("SocketSend: send() failed");
        return -1;
    }
    return (i64)ret;
}

void SocketClose(Socket *self) {
    if (!self) {
        return;
    }
    if (self->fd > 0) {
        if (close(self->fd) < 0) {
            LOG_SYS_ERROR("SocketClose: close() failed");
        }
    }
    MemSet(self, 0, sizeof(*self));
}

// ---------------------------------------------------------------------------
// Options
// ---------------------------------------------------------------------------

bool SocketSetNonBlocking(i32 fd, bool nonblock) {
    i32 flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0) {
        LOG_SYS_ERROR("SocketSetNonBlocking: fcntl(F_GETFL) failed");
        return false;
    }
    if (nonblock) {
        flags |= O_NONBLOCK;
    } else {
        flags &= ~O_NONBLOCK;
    }
    if (fcntl(fd, F_SETFL, flags) < 0) {
        LOG_SYS_ERROR("SocketSetNonBlocking: fcntl(F_SETFL) failed");
        return false;
    }
    return true;
}

bool SocketSetNoDelay(i32 fd, bool nodelay) {
    i32 v = nodelay ? 1 : 0;
    if (setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &v, sizeof(v)) < 0) {
        LOG_SYS_ERROR("SocketSetNoDelay: setsockopt(TCP_NODELAY) failed");
        return false;
    }
    return true;
}

bool SocketSetKeepAlive(i32 fd, bool keepalive) {
    i32 v = keepalive ? 1 : 0;
    if (setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &v, sizeof(v)) < 0) {
        LOG_SYS_ERROR("SocketSetKeepAlive: setsockopt(SO_KEEPALIVE) failed");
        return false;
    }
    return true;
}

bool SocketSetReuseAddr(i32 fd, bool reuse) {
    i32 v = reuse ? 1 : 0;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &v, sizeof(v)) < 0) {
        LOG_SYS_ERROR("SocketSetReuseAddr: setsockopt(SO_REUSEADDR) failed");
        return false;
    }
    return true;
}

bool SocketSetRecvTimeoutMs(i32 fd, u32 ms) {
    struct timeval tv;
    tv.tv_sec  = (long)(ms / 1000);
    tv.tv_usec = (long)((ms % 1000) * 1000);
    if (setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
        LOG_SYS_ERROR("SocketSetRecvTimeoutMs: setsockopt(SO_RCVTIMEO) failed");
        return false;
    }
    return true;
}

bool SocketSetSendTimeoutMs(i32 fd, u32 ms) {
    struct timeval tv;
    tv.tv_sec  = (long)(ms / 1000);
    tv.tv_usec = (long)((ms % 1000) * 1000);
    if (setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv)) < 0) {
        LOG_SYS_ERROR("SocketSetSendTimeoutMs: setsockopt(SO_SNDTIMEO) failed");
        return false;
    }
    return true;
}

// ---------------------------------------------------------------------------
// Multiplexing
// ---------------------------------------------------------------------------

i32 SocketPoll(SocketPollItem *items, u32 count, i32 timeout_ms) {
    if (!items && count > 0) {
        LOG_ERROR("SocketPoll: items is NULL but count > 0");
        return -1;
    }

    // Stack array for small counts, heap fallback for large ones. The
    // crossover is a guess; tune if it matters.
    enum {
        STACK_MAX = 64
    };
    struct pollfd  stack_pfds[STACK_MAX];
    struct pollfd *pfds = stack_pfds;
    HeapAllocator  halloc;
    bool           used_heap = false;
    if (count > STACK_MAX) {
        halloc = HeapAllocatorInit();
        pfds   = (struct pollfd *)AllocatorAlloc(ALLOCATOR_OF(&halloc), sizeof(struct pollfd) * count, true);
        if (!pfds) {
            HeapAllocatorDeinit(&halloc);
            LOG_ERROR("SocketPoll: heap allocation for pollfd array failed");
            return -1;
        }
        used_heap = true;
    }

    for (u32 i = 0; i < count; ++i) {
        pfds[i].fd     = items[i].fd;
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
    do {
        ret = poll(pfds, (nfds_t)count, timeout_ms);
    } while (ret < 0 && errno == EINTR);

    if (ret < 0) {
        LOG_SYS_ERROR("SocketPoll: poll() failed");
    } else {
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
        AllocatorFree(ALLOCATOR_OF(&halloc), pfds, sizeof(struct pollfd) * count);
        HeapAllocatorDeinit(&halloc);
    }
    return ret;
}
