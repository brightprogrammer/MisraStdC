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
#include <Misra/Sys/Dns.h>
#include <Misra/Sys/Socket.h>

#include <signal.h>
#include <string.h>

static volatile sig_atomic_t g_stop = 0;

static void on_signal(int signum) {
    (void)signum;
    g_stop = 1;
}

// Resolve a "host:port" spec into a SocketAddr. Tries SocketAddrParse
// first (covers numeric IPv4 / bracketed IPv6 with no network); falls
// back to DnsResolve through `r` for hostnames. Takes the first address
// the resolver returns when multiple are available.
static bool resolve_spec(DnsResolver *r, const char *spec, SocketKind kind, SocketAddr *out, Allocator *scratch) {
    if (SocketAddrParse(out, spec, kind)) {
        return true;
    }
    // SocketAddrParse rejected -- spec is either a hostname:port or
    // bare hostname. Split on the last ':' for port; hostnames don't
    // contain colons, IPv6 has been ruled out by SocketAddrParse failing.
    u64 spec_len = ZstrLen(spec);
    u64 colon_at = spec_len;
    for (u64 i = spec_len; i > 0; --i) {
        if (spec[i - 1] == ':') {
            colon_at = i - 1;
            break;
        }
    }
    if (colon_at >= spec_len) {
        LOG_ERROR("resolve: no \":port\" in spec \"{}\"", (const char *)spec);
        return false;
    }
    char host[256];
    if (colon_at >= sizeof(host)) {
        LOG_ERROR("resolve: host portion of \"{}\" exceeds 255 bytes", (const char *)spec);
        return false;
    }
    MemCopy(host, spec, colon_at);
    host[colon_at] = '\0';

    u16 port = 0;
    for (u64 i = colon_at + 1; i < spec_len; ++i) {
        char c = spec[i];
        if (c < '0' || c > '9') {
            LOG_ERROR("resolve: non-numeric port in \"{}\"", (const char *)spec);
            return false;
        }
        u32 next = (u32)port * 10 + (u32)(c - '0');
        if (next > 0xFFFFu) {
            LOG_ERROR("resolve: port in \"{}\" out of range", (const char *)spec);
            return false;
        }
        port = (u16)next;
    }

    DnsAddrs addrs = VecInitT(addrs, scratch);
    bool     ok    = DnsResolve(r, host, port, kind, &addrs);
    if (ok && addrs.length > 0) {
        *out = addrs.data[0];
        VecDeinit(&addrs);
        return true;
    }
    VecDeinit(&addrs);
    return false;
}

static void install_signal_handlers(void) {
    struct sigaction sa;
    MemSet(&sa, 0, sizeof(sa));
    sa.sa_handler = on_signal;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
    // SIGPIPE on a hung-up peer would terminate us; mask it and rely on
    // send() returning EPIPE instead.
    signal(SIGPIPE, SIG_IGN);
}

static const char *flag_value(int argc, char **argv, const char *flag) {
    for (int i = 1; i + 1 < argc; ++i) {
        if (ZstrCompare(argv[i], flag) == 0) {
            return argv[i + 1];
        }
    }
    return NULL;
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
    const char *listen_spec   = flag_value(argc, argv, "--listen");
    const char *upstream_spec = flag_value(argc, argv, "--upstream");

    if (!listen_spec || !upstream_spec) {
        WriteFmt("usage: {} --listen <host:port> --upstream <host:port>\n", argv[0]);
        WriteFmt("example: {} --listen 127.0.0.1:8080 --upstream 127.0.0.1:3000\n", argv[0]);
        return 1;
    }

    install_signal_handlers();

    Scope(alloc, DefaultAllocator) {
        DnsResolver resolver;
        if (!DnsResolverInit(&resolver, alloc)) {
            LOG_ERROR("failed to init DNS resolver");
            return 1;
        }

        SocketAddr listen_addr;
        if (!resolve_spec(&resolver, listen_spec, SOCKET_KIND_TCP, &listen_addr, alloc)) {
            LOG_ERROR("invalid --listen address: {}", listen_spec);
            DnsResolverDeinit(&resolver);
            return 1;
        }

        SocketAddr upstream_addr;
        if (!resolve_spec(&resolver, upstream_spec, SOCKET_KIND_TCP, &upstream_addr, alloc)) {
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
