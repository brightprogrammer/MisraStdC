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

#if PLATFORM_WINDOWS
// Windows: use SetConsoleCtrlHandler (kernel32) instead of signal()
// (UCRT). SetConsoleCtrlHandler runs the registered callback on a
// dedicated thread when the user hits Ctrl-C / Ctrl-Break / the
// console window closes; we just flip the stop flag and return TRUE
// to swallow the event. No UCRT signal() means the freestanding
// Windows build can drop UCRT entirely.
#    include <windows.h>
#elif (PLATFORM_LINUX || PLATFORM_DARWIN) && (ARCHITECTURE_X86_64 || ARCHITECTURE_AARCH64)
// POSIX direct-syscall path: hand-roll sigaction so Beam doesn't drag
// libc's sigaction/sigemptyset/signal into the link. The kernel
// sigaction ABI is per-OS and per-arch:
//
//   Linux x86_64 / aarch64
//     - Single syscall (rt_sigaction). On x86_64 the caller must
//       supply a SA_RESTORER trampoline that issues rt_sigreturn;
//       on aarch64 the kernel synthesises it on the signal stack.
//     - Struct: handler / flags / restorer / mask (1 word).
//
//   Darwin x86_64 / aarch64
//     - Single syscall (sigaction #46). The kernel never returns to
//       the user's handler directly: it invokes a per-installation
//       'sa_tramp' which is responsible for calling the handler then
//       issuing sigreturn (#184). libSystem normally hides this by
//       supplying its own _sigtramp; without libSystem we write our
//       own trampoline. Struct: handler / sa_tramp / mask / flags.
#    include "../Source/Misra/_Syscall.h"

#    define BEAM_SIGINT          2
#    define BEAM_SIGPIPE         13
#    define BEAM_SIGTERM         15
#    define BEAM_SIG_IGN_HANDLER ((void (*)(int))1) // SIG_IGN

#    if PLATFORM_LINUX

#        define BEAM_SA_RESTORER 0x04000000UL

struct kernel_sigaction {
    void (*sa_handler)(int);
    unsigned long sa_flags;
    void (*sa_restorer)(void);
    unsigned long sa_mask; // single-word: covers all 64 standard signals
};

#        if ARCHITECTURE_X86_64
// x86_64 kernel jumps to this on signal return. We must issue
// rt_sigreturn ourselves; the kernel doesn't restore for us. `naked`
// keeps the compiler from emitting a prologue that'd clobber the
// kernel-built signal frame on the stack.
__attribute__((naked)) static void sigreturn_restorer(void) {
    __asm__(
        "movq $"
        "15"
        ", %rax\n"
        "syscall\n"
    );
}
#        endif

static void install_signal(int signum, void (*handler)(int)) {
    struct kernel_sigaction sa = {0};
    sa.sa_handler              = handler;
#        if ARCHITECTURE_X86_64
    sa.sa_flags    = BEAM_SA_RESTORER;
    sa.sa_restorer = sigreturn_restorer;
#        endif
    // 4th arg = sigsetsize in bytes (Linux ABI requires 8 for the
    // standard signal set).
    direct_sys4(MISRA_SYS_rt_sigaction, (long)signum, (long)(u64)&sa, 0, 8);
}

#    else     // PLATFORM_DARWIN

// Darwin's struct __sigaction (what SYS_sigaction wants). Field
// layout per <sys/signal.h>: handler / tramp / mask / flags. We use
// our own field names because Darwin's <signal.h> #defines sa_handler
// as a textual alias for __sigaction_u.__sa_handler, which would
// mangle a struct definition that uses 'sa_handler' as a field name.
// (Headers downstream of Misra/Sys/Socket.h drag <signal.h> in
// transitively, so we can't escape the macro.) Layout is what matters
// to the kernel, not the field names.
struct kernel_sigaction {
    void (*kh_handler)(int);                            // handler -> sa_handler slot
    void (*kh_tramp)(void *, int, int, void *, void *); // trampoline (calls handler+sigreturn)
    unsigned int kh_mask;                               // 32-bit signal set (sa_mask slot)
    int          kh_flags;                              // sa_flags slot
};

// SA_SIGINFO=0x40. We don't use it -- sa_handler is plain
// void(*)(int) -- so sa_flags stays 0 and the trampoline routes
// through the UC_TRAD branch (handler(sig), no siginfo/uctx args).

#        if ARCHITECTURE_AARCH64
// Darwin aarch64 signal trampoline.
//   Entry registers (kernel-provided, per dyld/libplatform sigtramp):
//     x0 = handler           (sa_handler or sa_sigaction, per sigstyle)
//     w1 = sigstyle          (UC_TRAD=1 for plain handler, others for siginfo)
//     w2 = sig               (signal number)
//     x3 = sinfo             (siginfo_t*, unused for UC_TRAD)
//     x4 = uctx              (ucontext_t* -- pass back to sigreturn)
//   Action:
//     Call handler(sig) for UC_TRAD; or handler(sig, sinfo, uctx) for SIGINFO.
//     Then SYS_sigreturn(uctx, sigstyle). The syscall never returns.
//
// We only register UC_TRAD-style handlers, so just dispatch that case.
__attribute__((naked)) static void darwin_sigtramp(void) {
    __asm__(
        // Save handler, uctx, sigstyle on stack.
        "stp x0, x4, [sp, #-32]!\n" // handler @0, uctx @8
        "str w1, [sp, #16]\n"       // sigstyle @16
        // handler(sig)
        "mov w0, w2\n"   // sig -> arg0
        "ldr x9, [sp]\n" // handler
        "blr x9\n"
        // sigreturn(uctx, sigstyle)
        "ldr x0, [sp, #8]\n"  // uctx
        "ldr w1, [sp, #16]\n" // sigstyle
        "mov x16, #184\n"     // SYS_sigreturn
        "svc #0x80\n"
        // Should not return.
        "udf #0\n"
    );
}
#        else // ARCHITECTURE_X86_64
// Darwin x86_64 signal trampoline. Same shape as aarch64 above.
//   Entry registers:
//     %rdi = handler
//     %esi = sigstyle
//     %edx = sig
//     %rcx = sinfo
//     %r8  = uctx
//   Action: handler(sig); then sigreturn(uctx, sigstyle).
__attribute__((naked)) static void darwin_sigtramp(void) {
    __asm__(
        "subq $32, %rsp\n"        // 16-aligned scratch (kernel guarantees 16-aligned entry)
        "movq %rdi, 0(%rsp)\n"    // save handler
        "movq %r8,  8(%rsp)\n"    // save uctx
        "movl %esi, 16(%rsp)\n"   // save sigstyle
        "movl %edx, %edi\n"       // sig -> arg0
        "callq *0(%rsp)\n"        // handler(sig)
        "movq 8(%rsp), %rdi\n"    // uctx -> arg0
        "movl 16(%rsp), %esi\n"   // sigstyle -> arg1
        "movq $0x20000B8, %rax\n" // SYS_sigreturn = DARWIN_SC(184) = 0x20000B8
        "syscall\n"
        "ud2\n"                   // should not return
    );
}
#        endif

static void install_signal(int signum, void (*handler)(int)) {
    struct kernel_sigaction sa = {0};
    sa.kh_handler              = handler;
    // SIG_IGN doesn't actually invoke the trampoline, but the kernel
    // still copies sa_tramp into per-thread state. Point it at our
    // trampoline unconditionally -- safe even when unused. Cast
    // through (void(*)(void)) to silence the trampoline-signature
    // mismatch (declared as 5-arg in the struct; defined as naked
    // void() with kernel-shaped register entry).
    sa.kh_tramp = (void (*)(void *, int, int, void *, void *))(void (*)(void))darwin_sigtramp;
    direct_sys3(MISRA_SYS_rt_sigaction, (long)signum, (long)(u64)&sa, 0);
}

#    endif    // PLATFORM_LINUX / PLATFORM_DARWIN
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

#if PLATFORM_WINDOWS
// Windows console control callback. Runs in a dedicated thread the
// system spins up when the user hits Ctrl-C / Ctrl-Break, or the
// console window is closed / user logs off / system shuts down. We
// flip the stop flag (volatile, so the main loop sees it) and return
// TRUE to mark the event handled. No SIGPIPE equivalent on Windows;
// send() to a hung-up peer returns WSAECONNRESET which SocketSend
// maps to -1.
static BOOL WINAPI on_console_ctrl(DWORD ctrl_type) {
    switch (ctrl_type) {
        case CTRL_C_EVENT :
        case CTRL_BREAK_EVENT :
        case CTRL_CLOSE_EVENT :
        case CTRL_LOGOFF_EVENT :
        case CTRL_SHUTDOWN_EVENT :
            g_stop = 1;
            return TRUE;
        default :
            return FALSE;
    }
}
#endif

static void install_signal_handlers(void) {
#if PLATFORM_WINDOWS
    SetConsoleCtrlHandler(on_console_ctrl, TRUE);
#elif (PLATFORM_LINUX || PLATFORM_DARWIN) && (ARCHITECTURE_X86_64 || ARCHITECTURE_AARCH64)
    install_signal(BEAM_SIGINT, on_signal);
    install_signal(BEAM_SIGTERM, on_signal);
    // SIGPIPE on a hung-up peer would terminate us; mask it and rely
    // on send() returning EPIPE instead.
    install_signal(BEAM_SIGPIPE, BEAM_SIG_IGN_HANDLER);
#else
    // Other POSIX (BSD on non-x86_64/aarch64 hardware, etc.): fall
    // back to libc sigaction.
    struct sigaction sa = {0};
    sa.sa_handler       = on_signal;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
    signal(SIGPIPE, SIG_IGN);
#endif
}

static void log_request_summary(Zstr client_addr, Zstr prefix_bytes, size prefix_len) {
    Scope(scope, DefaultAllocator) {
        Str raw = StrInit(scope);
        StrPushBackMany(&raw, prefix_bytes);

        HttpRequest req = HttpRequestInit(scope);
        Zstr        end = HttpRequestParse(&req, (Zstr)StrBegin(&raw));
        if (end == StrBegin(&raw)) {
            LOG_INFO("[{}] (unparseable request, {} bytes)", client_addr, (u64)prefix_len);
        } else {
            Zstr method = "?";
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
static void proxy_pump(Socket *a, Socket *b, Zstr first_chunk, size first_len) {
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
        log_request_summary(StrBegin(&peer_str), first, (size)first_n);
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
        Zstr listen_spec   = NULL;
        Zstr upstream_spec = NULL;

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
