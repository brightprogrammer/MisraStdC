/// file      : Socket.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Portable TCP / UDP socket abstraction. Two value types:
///
///   `Listener` — a bound + listening fd. Accepts incoming `Socket`s.
///   `Socket`   — a connected fd. Returned by `ListenerAccept` (server
///                side) or `SocketConnect` (client side). Carries the
///                peer's `SocketAddr` for logging / routing.
///
/// Addresses are resolved through `SocketAddrParse` (numeric only) or
/// the `Sys/Dns` resolver (numeric + name).
///
/// **Platform note on `SockFd`.** POSIX socket descriptors are `int`.
/// Winsock's `SOCKET` is `UINT_PTR` (unsigned, pointer-sized). The
/// `SockFd` typedef below resolves to whichever the platform needs;
/// the rest of the API stays identical. The `MISRA_SOCK_FD_INVALID`
/// constant is the equivalent of POSIX `-1` / Winsock `INVALID_SOCKET`
/// for portable invalid-checks. Do *not* compare a `SockFd` with `< 0`
/// — on Windows it's unsigned.

#ifndef MISRA_SYS_SOCKET_H
#define MISRA_SYS_SOCKET_H

#include <Misra/Std/Allocator.h>
#include <Misra/Std/Container/Str.h>
#include <Misra/Types.h>

// `sockaddr_storage` is 128 bytes on every major platform. Lift the
// size inline so the public header does not drag in `<sys/socket.h>`.
#define SOCKET_ADDR_MAX_SIZE 128

///
/// Portable socket-descriptor type. `u64` on Windows so it can carry a
/// real Winsock `SOCKET` (UINT_PTR), `i32` everywhere else so it stays
/// printf-friendly and matches the POSIX `int` fd.
///
#ifdef _WIN32
typedef u64 SockFd;
#    define MISRA_SOCK_FD_INVALID ((SockFd) ~(u64)0) // == INVALID_SOCKET
#else
typedef i32 SockFd;
#    define MISRA_SOCK_FD_INVALID ((SockFd) - 1)
#endif

typedef enum SocketKind {
    SOCKET_KIND_INVALID = 0,
    SOCKET_KIND_TCP,
    SOCKET_KIND_UDP,
} SocketKind;

typedef enum SocketFamily {
    SOCKET_FAMILY_UNSPEC = 0,
    SOCKET_FAMILY_INET,  // IPv4
    SOCKET_FAMILY_INET6, // IPv6
} SocketFamily;

///
/// Wraps `struct sockaddr_storage`. `raw[0..length]` carries the actual
/// address bytes; `family` records the address family so callers do not
/// have to peek into `raw`. The byte layout of `sockaddr_in` /
/// `sockaddr_in6` is identical on Linux, macOS, and Windows, so
/// `SocketAddr` is platform-portable as a value.
///
typedef struct SocketAddr {
    u8           raw[SOCKET_ADDR_MAX_SIZE];
    u32          length;
    SocketFamily family;
} SocketAddr;

///
/// A bound + listening socket. `kind` records whether stream (TCP) or
/// datagram (UDP) semantics apply. `bound` is the address that was
/// passed to `ListenerOpen` (mostly for logging); see
/// `ListenerLocalAddr` for the actually-bound address when the caller
/// asked the kernel to pick the port.
///
typedef struct Listener {
    SockFd     fd;
    SocketKind kind;
    SocketAddr bound;
} Listener;

///
/// A connected socket: either returned by `ListenerAccept` (server side)
/// or by `SocketConnect` (client side). `peer` is the remote endpoint.
///
typedef struct Socket {
    SockFd     fd;
    SocketKind kind;
    SocketAddr peer;
} Socket;

typedef enum SocketPollFlags {
    SOCKET_POLL_READ  = 1u << 0,
    SOCKET_POLL_WRITE = 1u << 1,
    SOCKET_POLL_ERROR = 1u << 2,
} SocketPollFlags;

///
/// One slot in a `SocketPoll` call. Caller fills `fd` (from a
/// `Listener` or `Socket`) and `events_requested`; `SocketPoll` writes
/// `events_ready` back.
///
typedef struct SocketPollItem {
    SockFd fd;
    u32    events_requested;
    u32    events_ready;
} SocketPollItem;

// --- Addressing -------------------------------------------------------------

///
/// Parse a host:port string into a `SocketAddr`. Numeric host only --
/// for hostname resolution use `Sys/Dns`'s `DnsResolve`.
///
/// Accepted forms:
///   "127.0.0.1:8080"
///   "[::1]:8080"
///
/// out[out]  : Result address. Left zeroed on failure.
/// spec[in]  : Host:port specifier.
/// kind[in]  : `SOCKET_KIND_TCP` or `SOCKET_KIND_UDP`. Currently informational.
///
/// SUCCESS : returns true; `out` populated.
/// FAILURE : returns false; `out` zeroed. Failure is silent so the
///           caller can chain into DNS for the hostname case without
///           log noise.
///
/// TAGS: Socket, Address
///
bool SocketAddrParse(SocketAddr *out, const char *spec, SocketKind kind);

///
/// Render a `SocketAddr` back into a "ip:port" string. IPv6 addresses
/// are emitted with `[...]` brackets so the output round-trips through
/// `SocketAddrParse`.
///
/// addr[in]  : Address to format.
/// alloc[in] : Allocator backing the returned `Str`. Caller owns the
///             result and must `StrDeinit` it.
///
/// SUCCESS : returns initialized `Str`.
/// FAILURE : returns empty `Str` if the address cannot be formatted
///           (e.g. unknown family); logs the failure.
///
/// TAGS: Socket, Address, Format
///
Str SocketAddrFormat(const SocketAddr *addr, Allocator *alloc);

// --- Listener (server side) -------------------------------------------------

///
/// Open + bind + listen on the given address.
///
/// out[out]      : Listener to populate. Untouched on failure.
/// kind[in]      : `SOCKET_KIND_TCP` (UDP listeners do not call listen()
///                 but the same entry point applies — see notes in the
///                 implementation).
/// addr[in]      : Address to bind to.
/// backlog[in]   : `listen()` backlog. Use 128 if you are unsure.
///
/// SUCCESS : returns true; `out` is ready for `ListenerAccept`.
/// FAILURE : returns false; logs the failing syscall.
///
/// TAGS: Socket, Listener, Bind
///
bool ListenerOpen(Listener *out, SocketKind kind, const SocketAddr *addr, i32 backlog);

///
/// Query the actually-bound local address of a listener. Useful after
/// `ListenerOpen` with port 0, where the kernel picks the port.
/// Wraps `getsockname` on POSIX / Winsock identically.
///
/// self[in]   : Open listener.
/// out[out]   : Filled with the bound address. Zeroed on failure.
///
/// SUCCESS : returns true; `out` populated.
/// FAILURE : returns false; logs the failing syscall.
///
/// TAGS: Socket, Listener, Address
///
bool ListenerLocalAddr(const Listener *self, SocketAddr *out);

///
/// Accept the next pending connection on a listener.
///
/// self[in,out]   : Listener to accept on.
/// out_conn[out]  : Populated with the new connected `Socket` on success.
///
/// SUCCESS : returns true; `out_conn->fd` is the new socket fd.
/// FAILURE : returns false; logs the failing syscall. On `EINTR` /
///           `EAGAIN` the caller should retry.
///
/// TAGS: Socket, Listener, Accept
///
bool ListenerAccept(Listener *self, Socket *out_conn);

///
/// Close the listener fd. Safe to call on a zeroed `Listener`.
///
/// SUCCESS : Listener is closed and zeroed.
/// FAILURE : Function cannot fail; close errors are logged at error
///           level but do not propagate (the fd is gone either way).
///
/// TAGS: Socket, Listener, Close
///
void ListenerClose(Listener *self);

// --- Socket (client + accepted) --------------------------------------------

///
/// Open + connect to a remote endpoint.
///
/// out[out]    : Connected socket. Untouched on failure.
/// kind[in]    : `SOCKET_KIND_TCP` or `SOCKET_KIND_UDP`.
/// target[in]  : Address to connect to.
///
/// SUCCESS : returns true; `out->fd` is connected.
/// FAILURE : returns false; logs the failing syscall.
///
/// TAGS: Socket, Connect
///
bool SocketConnect(Socket *out, SocketKind kind, const SocketAddr *target);

///
/// Blocking read from a connected socket.
///
/// self[in]  : Socket to read from.
/// buf[out]  : Destination buffer.
/// n[in]     : Capacity of `buf`.
///
/// SUCCESS : returns number of bytes read (0 means peer closed).
/// FAILURE : returns -1; logs the failing syscall.
///
/// TAGS: Socket, Recv
///
i64 SocketRecv(Socket *self, void *buf, size n);

///
/// Blocking write to a connected socket. Does not partial-retry; the
/// caller decides whether to loop on short writes.
///
/// self[in]  : Socket to write to.
/// buf[in]   : Source buffer.
/// n[in]     : Number of bytes to send.
///
/// SUCCESS : returns number of bytes sent.
/// FAILURE : returns -1; logs the failing syscall.
///
/// TAGS: Socket, Send
///
i64 SocketSend(Socket *self, const void *buf, size n);

///
/// Close a connected socket. Safe to call on a zeroed `Socket`.
///
/// SUCCESS : Socket is closed and zeroed.
/// FAILURE : Function cannot fail; close errors are logged at error
///           level but do not propagate.
///
/// TAGS: Socket, Close
///
void SocketClose(Socket *self);

// --- Options ----------------------------------------------------------------
//
// Options are exposed as free-standing functions taking a raw fd, so a
// caller can apply them to either a `Socket` or a `Listener` without an
// extra wrapper. All return true on success / false on syscall failure
// (logged).
//
// Note: `SocketSetReuseAddr` maps to `SO_REUSEADDR` on POSIX but to
// `SO_EXCLUSIVEADDRUSE` on Windows — Windows's `SO_REUSEADDR` lets
// other processes hijack the port, which is the opposite of what
// callers want. The shim normalises both to "let me restart the
// server quickly without ADDRINUSE from TIME_WAIT" semantics.

bool SocketSetNonBlocking(SockFd fd, bool nonblock);
bool SocketSetNoDelay(SockFd fd, bool nodelay);     // TCP_NODELAY (Nagle off)
bool SocketSetKeepAlive(SockFd fd, bool keepalive); // SO_KEEPALIVE
bool SocketSetReuseAddr(SockFd fd, bool reuse);     // POSIX SO_REUSEADDR / Win SO_EXCLUSIVEADDRUSE
bool SocketSetRecvTimeoutMs(SockFd fd, u32 ms);
bool SocketSetSendTimeoutMs(SockFd fd, u32 ms);

// --- Multiplexing -----------------------------------------------------------

///
/// Wait until any item is ready. Backed by `poll()` on POSIX and
/// `WSAPoll()` on Windows. Signatures match closely enough that a
/// single shim works; the WSAPoll failed-connect bug from older
/// Windows builds was fixed in Windows 10 2004 (2020).
///
/// items[in,out] : Array of items. Caller fills `fd` +
///                 `events_requested`; `events_ready` is the output.
/// count[in]     : Number of items in `items`.
/// timeout_ms[in]: -1 = block forever, 0 = poll once, >0 = milliseconds.
///
/// SUCCESS : returns number of items with non-zero `events_ready`.
/// FAILURE : returns -1; logs the failing syscall. `EINTR` is
///           transparently retried.
///
/// TAGS: Socket, Poll, Multiplexing
///
i32 SocketPoll(SocketPollItem *items, u32 count, i32 timeout_ms);

#endif // MISRA_SYS_SOCKET_H
