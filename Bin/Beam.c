/// file      : Bin/Beam.c
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// `beam` — small reverse-proxy tool. Accepts HTTP/1.1 requests on a
/// listening address and forwards each connection byte-for-byte to a
/// configured upstream. v1 is intentionally minimal:
///
///   - One connection serviced at a time (no threading, no epoll).
///   - No request rewriting (Host header is passed through as-is).
///   - No TLS.
///   - The HTTP parser is only used to log "METHOD URL" for each
///     accepted connection; the bytes themselves are proxied raw, so
///     anything HTTP/1.1-shaped will pass through.

#define _DEFAULT_SOURCE
#define _POSIX_C_SOURCE 200809L

#include <Misra.h>
#include <Misra/Parsers/Http.h>
#include <Misra/Std/Allocator/Default.h>
#include <Misra/Std/ArgParse.h>
#include <Misra/Sys/Dns.h>
#include <Misra/Sys/Socket.h>

#if defined(_WIN32)
#    include <signal.h>
#elif defined(__linux__) && (defined(__x86_64__) || defined(__aarch64__))
// Linux direct-syscall path: hand-roll rt_sigaction so Beam doesn't
// drag libc's sigaction/sigemptyset/signal into the link. POSIX
// sigaction is just a thin wrapper around the rt_sigaction syscall,
// but the kernel sigaction ABI differs from glibc's (different field
// order; x86_64 requires the caller to supply a SA_RESTORER trampoline
// because the kernel doesn't synthesize one for unwinding back to
// userspace).
#    include "../Source/Misra/_Syscall.h"

#    define MISRA_SIGINT          2
#    define MISRA_SIGPIPE         13
#    define MISRA_SIGTERM         15
#    define MISRA_SA_RESTORER     0x04000000UL
#    define MISRA_SIG_IGN_HANDLER ((void (*)(int))1) // SIG_IGN

struct misra_kernel_sigaction {
    void (*sa_handler)(int);
    unsigned long sa_flags;
    void (*sa_restorer)(void);
    unsigned long sa_mask; // single-word: covers all 64 standard signals
};

#    if defined(__x86_64__)
// x86_64 kernel jumps to this on signal return. We must issue
// rt_sigreturn ourselves; the kernel doesn't restore for us. `naked`
// keeps the compiler from emitting a prologue that'd clobber the
// kernel-built signal frame on the stack.
__attribute__((naked)) static void misra_sigreturn_restorer(void) {
    __asm__(
        "movq $"
        "15"
        ", %rax\n"
        "syscall\n"
    );
}
#    endif

static void install_signal(int signum, void (*handler)(int)) {
    struct misra_kernel_sigaction sa = {0};
    sa.sa_handler                    = handler;
#    if defined(__x86_64__)
    sa.sa_flags    = MISRA_SA_RESTORER;
    sa.sa_restorer = misra_sigreturn_restorer;
#    endif
    // 4th arg = sigsetsize in bytes (Linux ABI requires 8 for the
    // standard signal set).
    misra_sys4(MISRA_SYS_rt_sigaction, (long)signum, (long)(uintptr_t)&sa, 0, 8);
}
#else
#    include <signal.h>
#endif

// `sig_atomic_t` is `int` on every Linux ABI we target; using `int`
// directly avoids having to include <signal.h> on the Linux direct-
// syscall path (which is otherwise libc-free).
static volatile int g_stop = 0;

static void on_signal(int signum) {
    (void)signum;
    g_stop = 1;
}

static void install_signal_handlers(void) {
#if defined(_WIN32)
    // Windows has signal() but not sigaction/sigemptyset, and there's
    // no SIGPIPE on Windows -- send() to a closed peer returns
    // WSAECONNRESET (mapped to -1 by SocketSend) so the proxy loop
    // exits cleanly on its own.
    signal(SIGINT, on_signal);
    signal(SIGTERM, on_signal);
#elif defined(__linux__) && (defined(__x86_64__) || defined(__aarch64__))
    install_signal(MISRA_SIGINT, on_signal);
    install_signal(MISRA_SIGTERM, on_signal);
    // SIGPIPE on a hung-up peer would terminate us; mask it and rely
    // on send() returning EPIPE instead.
    install_signal(MISRA_SIGPIPE, MISRA_SIG_IGN_HANDLER);
#else
    // Other POSIX (macOS, BSD): fall back to libc sigaction.
    struct sigaction sa;
    MemSet(&sa, 0, sizeof(sa));
    sa.sa_handler = on_signal;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
    signal(SIGPIPE, SIG_IGN);
#endif
}

static void log_request_summary(Allocator *alloc, const char *client_addr, const char *prefix_bytes, size prefix_len) {
    Scope(scope, DefaultAllocator) {
        (void)alloc;
        Str raw = StrInit(scope);
        StrPushBackZstr(&raw, prefix_bytes);

        HttpRequest req = HttpRequestInit(scope);
        const char *end = HttpRequestParse(&req, raw.data);
        if (end == raw.data) {
            LOG_INFO("[{}] (unparseable request, {} bytes)", client_addr, (u64)prefix_len);
        } else {
            const char *method = "?";
            switch (req.method) {
                case HTTP_REQUEST_METHOD_GET :
                    method = "GET";
                    break;
                case HTTP_REQUEST_METHOD_POST :
                    method = "POST";
                    break;
                case HTTP_REQUEST_METHOD_DELETE :
                    method = "DELETE";
                    break;
                case HTTP_REQUEST_METHOD_PUT :
                    method = "PUT";
                    break;
                case HTTP_REQUEST_METHOD_PATCH :
                    method = "PATCH";
                    break;
                case HTTP_REQUEST_METHOD_HEAD :
                    method = "HEAD";
                    break;
                case HTTP_REQUEST_METHOD_OPTIONS :
                    method = "OPTIONS";
                    break;
                case HTTP_REQUEST_METHOD_CONNECT :
                    method = "CONNECT";
                    break;
                case HTTP_REQUEST_METHOD_TRACE :
                    method = "TRACE";
                    break;
                default :
                    method = "UNKNOWN";
                    break;
            }
            LOG_INFO("[{}] {} {}", client_addr, method, req.url);
        }
        HttpRequestDeinit(&req);
        StrDeinit(&raw);
    }
}

// Pump bytes between `a` and `b` until either side errors or both
// directions close. `first_chunk` (if non-NULL) is sent toward `b`
// before the poll loop runs, so the initial client read isn't lost.
static void proxy_pump(Socket *a, Socket *b, const char *first_chunk, size first_len) {
    if (first_chunk && first_len > 0) {
        if (SocketSend(b, first_chunk, first_len) < 0) {
            return;
        }
    }

    SocketPollItem items[2];
    char           buf[8192];
    while (!g_stop) {
        items[0] = (SocketPollItem) {.fd = a->fd, .events_requested = SOCKET_POLL_READ};
        items[1] = (SocketPollItem) {.fd = b->fd, .events_requested = SOCKET_POLL_READ};

        i32 ready = SocketPoll(items, 2, 1000);
        if (ready < 0) {
            return;
        }
        if (ready == 0) {
            continue;
        }

        if (items[0].events_ready & SOCKET_POLL_READ) {
            i64 n = SocketRecv(a, buf, sizeof(buf));
            if (n <= 0) {
                return;
            }
            if (SocketSend(b, buf, (size)n) < 0) {
                return;
            }
        }
        if (items[1].events_ready & SOCKET_POLL_READ) {
            i64 n = SocketRecv(b, buf, sizeof(buf));
            if (n <= 0) {
                return;
            }
            if (SocketSend(a, buf, (size)n) < 0) {
                return;
            }
        }
        if (items[0].events_ready & SOCKET_POLL_ERROR || items[1].events_ready & SOCKET_POLL_ERROR) {
            return;
        }
    }
}

static void handle_connection(Allocator *alloc, Socket *client, const SocketAddr *upstream_addr) {
    Str peer_str = SocketAddrFormat(&client->peer, alloc);

    Socket upstream;
    if (!SocketConnect(&upstream, SOCKET_KIND_TCP, upstream_addr)) {
        LOG_ERROR("failed to dial upstream for client [{}]", peer_str);
        SocketClose(client);
        StrDeinit(&peer_str);
        return;
    }

    // Read the first chunk so we can log the request line.
    char first[4096];
    i64  first_n = SocketRecv(client, first, sizeof(first));
    if (first_n <= 0) {
        SocketClose(&upstream);
        SocketClose(client);
        StrDeinit(&peer_str);
        return;
    }

    // Ensure we have a Zstr terminator for parser logging only — the
    // parser scans a NUL-terminated buffer.
    if ((size)first_n < sizeof(first)) {
        first[first_n] = 0;
        log_request_summary(alloc, peer_str.data, first, (size)first_n);
    } else {
        LOG_INFO("[{}] (request larger than {} bytes, not logging line)", peer_str, (u64)sizeof(first));
    }

    proxy_pump(client, &upstream, first, (size)first_n);

    SocketClose(&upstream);
    SocketClose(client);
    StrDeinit(&peer_str);
}

int main(int argc, char **argv) {
    install_signal_handlers();

    Scope(alloc, DefaultAllocator) {
        const char *listen_spec   = NULL;
        const char *upstream_spec = NULL;

        ArgParse ap = ArgParseInit("beam", "small reverse-proxy");
        ArgRequired(&ap, "-l", "--listen", &listen_spec, "host:port to listen on");
        ArgRequired(&ap, "-u", "--upstream", &upstream_spec, "upstream host:port");

        ArgRun rc = ArgParseRun(&ap, argc, argv);
        ArgParseDeinit(&ap);
        if (rc != ARG_RUN_OK) {
            return rc == ARG_RUN_HELP ? 0 : 1;
        }

        DnsResolver resolver;
        if (!DnsResolverInit(&resolver, alloc)) {
            LOG_ERROR("failed to init DNS resolver");
            return 1;
        }

        SocketAddr listen_addr;
        if (!DnsResolve(&resolver, listen_spec, SOCKET_KIND_TCP, &listen_addr)) {
            LOG_ERROR("invalid --listen address: {}", listen_spec);
            DnsResolverDeinit(&resolver);
            return 1;
        }

        SocketAddr upstream_addr;
        if (!DnsResolve(&resolver, upstream_spec, SOCKET_KIND_TCP, &upstream_addr)) {
            LOG_ERROR("invalid --upstream address: {}", upstream_spec);
            DnsResolverDeinit(&resolver);
            return 1;
        }
        DnsResolverDeinit(&resolver);

        Listener listener;
        if (!ListenerOpen(&listener, SOCKET_KIND_TCP, &listen_addr, 128)) {
            LOG_ERROR("failed to open listener on {}", listen_spec);
            return 1;
        }

        LOG_INFO("beam listening on {} → upstream {}", listen_spec, upstream_spec);

        while (!g_stop) {
            // Wait for an incoming connection with a 1s timeout so the
            // signal handler gets a chance to flip g_stop without
            // accept() blocking forever (and without the EINTR error
            // log noise when it does).
            SocketPollItem listen_item = {.fd = listener.fd, .events_requested = SOCKET_POLL_READ};
            i32            ready       = SocketPoll(&listen_item, 1, 1000);
            if (ready <= 0) {
                continue;
            }
            if (!(listen_item.events_ready & SOCKET_POLL_READ)) {
                continue;
            }

            Socket client;
            if (!ListenerAccept(&listener, &client)) {
                continue;
            }
            handle_connection(alloc, &client, &upstream_addr);
        }

        LOG_INFO("beam shutting down");
        ListenerClose(&listener);
    }

    return 0;
}
