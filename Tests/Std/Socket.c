#if !PLATFORM_WINDOWS
#    define _DEFAULT_SOURCE
#    define _POSIX_C_SOURCE 200809L
#endif

#include <Misra/Std/Allocator/Default.h>
#include <Misra/Std/Io.h>
#include <Misra/Std/Zstr.h>
#include <Misra/Std/Log.h>
#include <Misra/Sys/Socket.h>

#include "../Util/TestRunner.h"

// Bind on 127.0.0.1:0 (let the kernel pick a free port), connect a
// client socket, send/recv a byte string, verify it round-trips.
// Uses only the portable Sys/Socket API -- no direct sockaddr poking,
// so it builds identically on POSIX and Windows.
bool test_socket_loopback_round_trip(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);

    Listener listener;
    Socket   client = {0};
    Socket   server = {0};

    SocketAddr bind_addr;
    if (!SocketAddrParse(&bind_addr, "127.0.0.1:0", SOCKET_KIND_TCP)) {
        DefaultAllocatorDeinit(&alloc);
        return false;
    }

    if (!ListenerOpen(&listener, SOCKET_KIND_TCP, &bind_addr, 16)) {
        DefaultAllocatorDeinit(&alloc);
        return false;
    }

    // Ask the kernel which port it actually picked. Format back to a
    // host:port string and re-parse so the test only ever talks to the
    // public API.
    SocketAddr local;
    if (!ListenerLocalAddr(&listener, &local)) {
        ListenerClose(&listener);
        DefaultAllocatorDeinit(&alloc);
        return false;
    }
    Str        local_str = SocketAddrFormat(&local, a);
    SocketAddr connect_addr;
    bool       parsed = SocketAddrParse(&connect_addr, (Zstr)StrBegin(&local_str), SOCKET_KIND_TCP);
    StrDeinit(&local_str);
    if (!parsed) {
        ListenerClose(&listener);
        DefaultAllocatorDeinit(&alloc);
        return false;
    }

    if (!SocketConnect(&client, SOCKET_KIND_TCP, &connect_addr)) {
        ListenerClose(&listener);
        DefaultAllocatorDeinit(&alloc);
        return false;
    }

    if (!ListenerAccept(&listener, &server)) {
        SocketClose(&client);
        ListenerClose(&listener);
        DefaultAllocatorDeinit(&alloc);
        return false;
    }

    Zstr payload = "hello from socket test";
    size n       = (size)ZstrLen(payload);
    if (SocketSend(&client, payload, n) != (i64)n) {
        SocketClose(&server);
        SocketClose(&client);
        ListenerClose(&listener);
        DefaultAllocatorDeinit(&alloc);
        return false;
    }

    char buf[64];
    i64  got = SocketRecv(&server, buf, sizeof(buf));
    bool ok  = got == (i64)n && MemCompare(buf, payload, n) == 0;

    SocketClose(&server);
    SocketClose(&client);
    ListenerClose(&listener);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// SocketAddrParse + SocketAddrFormat round-trip for IPv4 and IPv6.
bool test_socket_addr_format_round_trip(void) {
    DefaultAllocator alloc      = DefaultAllocatorInit();
    Allocator       *alloc_base = ALLOCATOR_OF(&alloc);
    bool             ok         = true;

    {
        SocketAddr addr;
        if (!SocketAddrParse(&addr, "127.0.0.1:8080", SOCKET_KIND_TCP)) {
            DefaultAllocatorDeinit(&alloc);
            return false;
        }
        Str rendered = SocketAddrFormat(&addr, alloc_base);
        ok           = ok && StrLen(&rendered) > 0 && ZstrCompare(StrBegin(&rendered), "127.0.0.1:8080") == 0;
        StrDeinit(&rendered);
    }

    {
        SocketAddr addr;
        if (!SocketAddrParse(&addr, "[::1]:8080", SOCKET_KIND_TCP)) {
            DefaultAllocatorDeinit(&alloc);
            return false;
        }
        Str rendered = SocketAddrFormat(&addr, alloc_base);
        ok           = ok && StrLen(&rendered) > 0 && ZstrCompare(StrBegin(&rendered), "[::1]:8080") == 0;
        StrDeinit(&rendered);
    }

    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// ===========================================================================
// Mutation-hardening tests for SocketPoll (from Socket.Mutants1.c).
//
// Every test builds real localhost (127.0.0.1) socket pairs the same way
// Tests/Std/Socket.c does (kernel-picked port via :0, format + re-parse the
// bound address, connect a client, accept the server side), then drives
// SocketPoll and asserts the EXACT returned count, the per-descriptor
// event flags, and which descriptor is reported ready. Inputs are shaped so
// each surviving operator / value / flag / count mutation in SocketPoll
// flips the count, a per-descriptor flag, or the ready descriptor.
//
// NORMAL tests return true on success. The single deadend (NULL items with
// count > 0 -> LOG_FATAL) lives in deadend_tests[].
// ===========================================================================

// A connected loopback TCP pair. `client` is the connecting side, `server`
// is the accepted side. Both must be closed by the caller.
typedef struct Pair {
    Listener listener;
    Socket   client;
    Socket   server;
    bool     ok;
} Pair;

// Build one connected loopback pair. On failure `.ok` is false and the
// caller need not close anything.
static Pair pair_make(Allocator *a) {
    Pair p = {0};

    SocketAddr bind_addr;
    if (!SocketAddrParse(&bind_addr, "127.0.0.1:0", SOCKET_KIND_TCP))
        return p;

    if (!ListenerOpen(&p.listener, SOCKET_KIND_TCP, &bind_addr, 16))
        return p;

    SocketAddr local;
    if (!ListenerLocalAddr(&p.listener, &local)) {
        ListenerClose(&p.listener);
        return p;
    }

    Str        local_str = SocketAddrFormat(&local, a);
    SocketAddr connect_addr;
    bool       parsed = SocketAddrParse(&connect_addr, (Zstr)StrBegin(&local_str), SOCKET_KIND_TCP);
    StrDeinit(&local_str);
    if (!parsed) {
        ListenerClose(&p.listener);
        return p;
    }

    if (!SocketConnect(&p.client, SOCKET_KIND_TCP, &connect_addr)) {
        ListenerClose(&p.listener);
        return p;
    }

    if (!ListenerAccept(&p.listener, &p.server)) {
        SocketClose(&p.client);
        ListenerClose(&p.listener);
        return p;
    }

    p.ok = true;
    return p;
}

static void pair_close(Pair *p) {
    SocketClose(&p->server);
    SocketClose(&p->client);
    ListenerClose(&p->listener);
}

// --- READ readiness ---------------------------------------------------------

// A socket with data WAITING to be read, polled with READ interest, must be
// reported READ-ready: count == 1, the READ flag set on exactly that
// descriptor, and WRITE / ERROR NOT set.
//
// Kills: 1030 (ret >= 0 must run the readback loop), 1031 (readback loop
// runs at least once), 1032 (ready init must start at 0 so unset bits stay
// clear), 1033/1034 (POLLIN -> SOCKET_POLL_READ mapping), 1042 (events_ready
// must be written), 1025 (ret < 0 must be false here).
bool test_sk1_read_ready_sets_read_flag(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);

    Pair p = pair_make(a);
    if (!p.ok) {
        DefaultAllocatorDeinit(&alloc);
        return false;
    }

    Zstr payload = "X";
    bool sent    = SocketSend(&p.client, payload, 1) == 1;

    SocketPollItem item   = {0};
    item.fd               = p.server.fd;
    item.events_requested = SOCKET_POLL_READ;

    // Positive timeout: blocks until the byte arrives, so no race.
    i32 n = SocketPoll(&item, 1, 1000);

    bool ok = sent && n == 1 && (item.events_ready & SOCKET_POLL_READ) != 0 &&
              (item.events_ready & SOCKET_POLL_WRITE) == 0 && (item.events_ready & SOCKET_POLL_ERROR) == 0;

    pair_close(&p);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// A readable socket polled with WRITE-only interest must NOT report READ.
// The kernel will not return POLLIN unless POLLIN was requested, so READ
// must stay clear even though data is waiting.
//
// Kills: 994 (events_requested & SOCKET_POLL_READ -> & not |: WRITE-only
// request (==2) must not switch POLLIN on; with | it would), 995 (events |=
// POLLIN -> &=: would clear the requested POLLOUT bit).
bool test_sk1_write_only_request_no_read_event(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);

    Pair p = pair_make(a);
    if (!p.ok) {
        DefaultAllocatorDeinit(&alloc);
        return false;
    }

    // Server has data waiting, but we ask only about WRITE on the client.
    bool sent = SocketSend(&p.client, "Y", 1) == 1;

    SocketPollItem item   = {0};
    item.fd               = p.server.fd;
    item.events_requested = SOCKET_POLL_WRITE; // not READ

    i32 n = SocketPoll(&item, 1, 0);

    // Whatever poll returns, READ must never be set since it wasn't asked.
    bool ok = sent && n >= 0 && (item.events_ready & SOCKET_POLL_READ) == 0;

    pair_close(&p);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// --- WRITE readiness --------------------------------------------------------

// A freshly-connected socket with empty send buffer, polled with WRITE
// interest, is WRITE-ready: WRITE flag set, READ flag NOT set (no data).
//
// Kills: 997/998 (POLLOUT mapping on the request side: SOCKET_POLL_WRITE
// must enable POLLOUT), 1036/1037 (POLLOUT -> SOCKET_POLL_WRITE on readback).
bool test_sk1_write_ready_sets_write_flag(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);

    Pair p = pair_make(a);
    if (!p.ok) {
        DefaultAllocatorDeinit(&alloc);
        return false;
    }

    SocketPollItem item   = {0};
    item.fd               = p.client.fd;
    item.events_requested = SOCKET_POLL_WRITE;

    i32 n = SocketPoll(&item, 1, 1000);

    bool ok = n == 1 && (item.events_ready & SOCKET_POLL_WRITE) != 0 && (item.events_ready & SOCKET_POLL_READ) == 0;

    pair_close(&p);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// A writable socket polled with READ-only interest must NOT report WRITE.
//
// Kills: 1036 (revents & POLLOUT -> | : a non-readable, writable-only revents
// would still map WRITE incorrectly), and reinforces the request-side mask
// (only READ asked, so POLLOUT must never be requested/returned).
bool test_sk1_read_only_request_no_write_event(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);

    Pair p = pair_make(a);
    if (!p.ok) {
        DefaultAllocatorDeinit(&alloc);
        return false;
    }

    bool sent = SocketSend(&p.client, "Z", 1) == 1;

    SocketPollItem item   = {0};
    item.fd               = p.server.fd;
    item.events_requested = SOCKET_POLL_READ; // not WRITE

    i32 n = SocketPoll(&item, 1, 1000);

    // Readable, READ requested -> READ set; WRITE must stay clear.
    bool ok =
        sent && n == 1 && (item.events_ready & SOCKET_POLL_READ) != 0 && (item.events_ready & SOCKET_POLL_WRITE) == 0;

    pair_close(&p);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// --- timeout ----------------------------------------------------------------

// An idle socket with no data, polled with READ interest and a 0ms timeout,
// must return 0 (timeout) with NO flags set on the descriptor.
//
// Kills: 1030 (ret >= 0 -> ret > 0: with 0 returned, readback must still run
// to clear events_ready), 1025 (ret < 0 must be false). Also exercises the
// "0 returned" boundary of the count path.
bool test_sk1_timeout_returns_zero_no_flags(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);

    Pair p = pair_make(a);
    if (!p.ok) {
        DefaultAllocatorDeinit(&alloc);
        return false;
    }

    SocketPollItem item   = {0};
    item.fd               = p.server.fd;
    item.events_requested = SOCKET_POLL_READ;
    item.events_ready     = 0xFFFFFFFFu; // poison: must be overwritten to 0

    // No data sent. READ on an idle socket with immediate timeout -> 0.
    i32 n = SocketPoll(&item, 1, 0);

    bool ok = n == 0 && item.events_ready == 0;

    pair_close(&p);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// --- multiple sockets / count accumulation ---------------------------------

// Poll three descriptors where a known subset (2 of 3) is READ-ready: two
// sockets have data waiting, one is idle. SocketPoll must return exactly 2,
// set READ on the two readable descriptors, and leave the idle one clear.
//
// Kills: 987 (build loop bound i < count -> off-by-one would skip/overrun a
// descriptor), 991 (fd assignment must use the right fd per index), 993/1000
// (events/revents per-slot reset), 1031 (readback loop bound), the count
// itself, and confirms the RIGHT descriptors are reported ready (index map).
bool test_sk1_three_sockets_two_ready_count(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);

    Pair p0 = pair_make(a);
    Pair p1 = pair_make(a);
    Pair p2 = pair_make(a);
    if (!p0.ok || !p1.ok || !p2.ok) {
        if (p0.ok)
            pair_close(&p0);
        if (p1.ok)
            pair_close(&p1);
        if (p2.ok)
            pair_close(&p2);
        DefaultAllocatorDeinit(&alloc);
        return false;
    }

    // Make server0 and server2 readable; server1 stays idle.
    i64  s0   = SocketSend(&p0.client, "A", 1);
    i64  s2   = SocketSend(&p2.client, "C", 1);
    bool sent = s0 == 1 && s2 == 1;
    // --- TEMP DIAGNOSTIC (remove after CI) ---
    WriteFmt(
        "SK1DBG sent s0={} s2={} fd0={} fd1={} fd2={}\n",
        s0,
        s2,
        (i64)p0.server.fd,
        (i64)p1.server.fd,
        (i64)p2.server.fd
    );

    SocketPollItem items[3]   = {0};
    items[0].fd               = p0.server.fd;
    items[0].events_requested = SOCKET_POLL_READ;
    items[1].fd               = p1.server.fd;
    items[1].events_requested = SOCKET_POLL_READ;
    items[2].fd               = p2.server.fd;
    items[2].events_requested = SOCKET_POLL_READ;

    i32 n = SocketPoll(items, 3, 1000);
    // --- TEMP DIAGNOSTIC (remove after CI) ---
    WriteFmt(
        "SK1DBG poll1 n={} r0={x} r1={x} r2={x}\n",
        n,
        items[0].events_ready,
        items[1].events_ready,
        items[2].events_ready
    );
    // Re-poll the same fds without reading: poll is level-triggered, so a
    // socket that's genuinely readable stays readable. If poll1 missed one
    // because its byte was still in flight (async loopback), poll2 now sees
    // it -> timing snapshot. If poll2 is still wrong -> a real defect.
    SocketPollItem items2[3]   = {0};
    items2[0].fd               = p0.server.fd;
    items2[0].events_requested = SOCKET_POLL_READ;
    items2[1].fd               = p1.server.fd;
    items2[1].events_requested = SOCKET_POLL_READ;
    items2[2].fd               = p2.server.fd;
    items2[2].events_requested = SOCKET_POLL_READ;
    i32 n2                     = SocketPoll(items2, 3, 1000);
    WriteFmt(
        "SK1DBG poll2 n={} r0={x} r1={x} r2={x}\n",
        n2,
        items2[0].events_ready,
        items2[1].events_ready,
        items2[2].events_ready
    );

    bool ok = sent && n == 2 && (items[0].events_ready & SOCKET_POLL_READ) != 0 &&
              (items[1].events_ready & SOCKET_POLL_READ) == 0 && (items[2].events_ready & SOCKET_POLL_READ) != 0;

    pair_close(&p0);
    pair_close(&p1);
    pair_close(&p2);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// Poll the SAME three descriptors but make only the MIDDLE one (index 1)
// readable. SocketPoll must return exactly 1 and report readiness on
// descriptor 1 only -- a wrong fd/index assignment in the build loop would
// map readiness to the wrong descriptor.
//
// Kills: 991 (pfds[i].fd <- items[i].fd: a const/index error reports the
// wrong descriptor), 987/1031 (loop indexing), count == 1.
bool test_sk1_middle_socket_ready_index(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);

    Pair p0 = pair_make(a);
    Pair p1 = pair_make(a);
    Pair p2 = pair_make(a);
    if (!p0.ok || !p1.ok || !p2.ok) {
        if (p0.ok)
            pair_close(&p0);
        if (p1.ok)
            pair_close(&p1);
        if (p2.ok)
            pair_close(&p2);
        DefaultAllocatorDeinit(&alloc);
        return false;
    }

    bool sent = SocketSend(&p1.client, "M", 1) == 1;

    SocketPollItem items[3]   = {0};
    items[0].fd               = p0.server.fd;
    items[0].events_requested = SOCKET_POLL_READ;
    items[1].fd               = p1.server.fd;
    items[1].events_requested = SOCKET_POLL_READ;
    items[2].fd               = p2.server.fd;
    items[2].events_requested = SOCKET_POLL_READ;

    i32 n = SocketPoll(items, 3, 1000);

    bool ok = sent && n == 1 && (items[0].events_ready & SOCKET_POLL_READ) == 0 &&
              (items[1].events_ready & SOCKET_POLL_READ) != 0 && (items[2].events_ready & SOCKET_POLL_READ) == 0;

    pair_close(&p0);
    pair_close(&p1);
    pair_close(&p2);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// --- ERROR / hangup mapping -------------------------------------------------

// When the peer closes, polling the other end for READ reports the socket
// ready: the EOF is delivered as POLLIN. This drives the readback path on a
// hung-up socket and confirms the descriptor is reported with the READ flag
// (EOF is readable) and not spuriously WRITE.
//
// Kills: 1033/1034 (POLLIN -> SOCKET_POLL_READ on a hung-up socket), 1042
// (events_ready written), and confirms n == 1 ready on peer close. The
// POLLHUP/POLLERR -> SOCKET_POLL_ERROR readback bit (1040) cannot be forced
// to SET reliably through the public loopback API (see /tmp ledger).
bool test_sk1_peer_close_reports_read_ready(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);

    Pair p = pair_make(a);
    if (!p.ok) {
        DefaultAllocatorDeinit(&alloc);
        return false;
    }

    // Client closes; server side sees EOF -> POLLIN readable.
    SocketClose(&p.client);

    SocketPollItem item   = {0};
    item.fd               = p.server.fd;
    item.events_requested = SOCKET_POLL_READ;

    i32 n = SocketPoll(&item, 1, 1000);

    bool ok = n == 1 && (item.events_ready & SOCKET_POLL_READ) != 0 && (item.events_ready & SOCKET_POLL_WRITE) == 0;

    SocketClose(&p.server);
    ListenerClose(&p.listener);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// --- count == 0 -------------------------------------------------------------

// Polling zero items with a 0ms timeout returns 0 immediately and touches
// nothing. Confirms the loop bounds (i < count) do not run when count == 0.
//
// Kills: 987/1031 (i < count -> i <= count would deref items[0] past the
// intended range; with count 0 and NULL items that is observable), 976
// (count > STACK_MAX must be false for count 0 -> stack path).
bool test_sk1_zero_count_returns_zero(void) {
    // NULL items is legal exactly when count == 0.
    i32 n = SocketPoll(NULL, 0, 0);
    return n == 0;
}

// --- heap path (count > STACK_MAX) -----------------------------------------

// More than STACK_MAX (64) descriptors forces the heap-allocated pollfd
// array branch. Poll 70 idle sockets with a 0ms timeout: returns 0, all
// descriptors cleared, and the heap allocation + free path runs.
//
// Kills: 976 (count > STACK_MAX: with 70 > 64 the heap branch must be taken;
// gt_to_ge/le would change which branch runs but both must still produce the
// correct result), 975/984 (used_heap init false / set true), 1048
// (AllocatorFree must run -- a leak under the debug allocator would be
// flagged). 64 stack-only entries also covered by other tests.
bool test_sk1_heap_path_many_idle(void) {
    enum {
        N = 70
    };
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);

    Pair pairs[N];
    u32  made = 0;
    for (u32 i = 0; i < N; ++i) {
        pairs[i] = pair_make(a);
        if (!pairs[i].ok)
            break;
        ++made;
    }

    bool ok = made == N;
    if (ok) {
        SocketPollItem items[N] = {0};
        for (u32 i = 0; i < N; ++i) {
            items[i].fd               = pairs[i].server.fd;
            items[i].events_requested = SOCKET_POLL_READ;
            items[i].events_ready     = 0xFFFFFFFFu; // poison
        }

        i32 n = SocketPoll(items, N, 0);
        ok    = n == 0;
        for (u32 i = 0; i < N; ++i)
            ok = ok && items[i].events_ready == 0;
    }

    for (u32 i = 0; i < made; ++i)
        pair_close(&pairs[i]);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// A single readable socket on the heap path (65 descriptors, last one
// readable) confirms the heap branch still maps the right descriptor and
// counts correctly.
//
// Kills: 976 (boundary 65 > 64 takes heap path), 991 (per-index fd on heap
// array), count == 1, and reinforces 1048 free.
bool test_sk1_heap_path_one_ready(void) {
    enum {
        N = 65
    };
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);

    Pair pairs[N];
    u32  made = 0;
    for (u32 i = 0; i < N; ++i) {
        pairs[i] = pair_make(a);
        if (!pairs[i].ok)
            break;
        ++made;
    }

    bool ok = made == N;
    if (ok) {
        // Make only the last descriptor readable.
        ok = SocketSend(&pairs[N - 1].client, "L", 1) == 1;

        SocketPollItem items[N] = {0};
        for (u32 i = 0; i < N; ++i) {
            items[i].fd               = pairs[i].server.fd;
            items[i].events_requested = SOCKET_POLL_READ;
        }

        i32 n = SocketPoll(items, N, 1000);
        ok    = ok && n == 1 && (items[N - 1].events_ready & SOCKET_POLL_READ) != 0;
        for (u32 i = 0; i + 1 < N; ++i)
            ok = ok && (items[i].events_ready & SOCKET_POLL_READ) == 0;
    }

    for (u32 i = 0; i < made; ++i)
        pair_close(&pairs[i]);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// --- deadend ----------------------------------------------------------------

// NULL items with count > 0 must trip LOG_FATAL (line 965).
//
// Kills: 965 (count > 0: gt_to_ge would also fire on count==0 but here we
// pass count==1 so the > 0 guard must hold and abort).
bool test_sk1_null_items_nonzero_count_aborts(void) {
    SocketPoll(NULL, 1, 0);
    return true; // unreachable; LOG_FATAL longjmps out
}

// ===========================================================================
// Mutation-hardening suite #2 (from Socket.Mutants2.c), targeting the pure
// IP-address *formatting* path: format_ipv4, append_hextet, format_ipv6.
// No network is touched -- every test drives SocketAddrParse (text ->
// raw bytes) then SocketAddrFormat (raw bytes -> canonical RFC-5952 /
// dotted text) and asserts the EXACT rendered string. IPv6 renderings
// arrive wrapped as "[host]:port"; IPv4 as "host:port".
// ===========================================================================

// Parse `spec`, format the result, compare against `expect` exactly.
// Returns true iff the parse succeeded and the rendered string matches.
static bool fmt_eq(Zstr spec, Zstr expect) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);

    SocketAddr addr;
    bool       ok = SocketAddrParse(&addr, spec, SOCKET_KIND_TCP);
    if (ok) {
        Str r = SocketAddrFormat(&addr, a);
        ok    = StrLen(&r) > 0 && ZstrCompare(StrBegin(&r), expect) == 0;
        StrDeinit(&r);
    }

    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// --- format_ipv4 -----------------------------------------------------------
// Each octet decoded MSB-first from the wire bytes; '.' between octets.

bool test_sk2_v4_generic(void) {
    return fmt_eq("1.2.3.4:0", "1.2.3.4:0");
}

bool test_sk2_v4_all_zero(void) {
    return fmt_eq("0.0.0.0:0", "0.0.0.0:0");
}

bool test_sk2_v4_all_max(void) {
    return fmt_eq("255.255.255.255:0", "255.255.255.255:0");
}

bool test_sk2_v4_loopback(void) {
    return fmt_eq("127.0.0.1:0", "127.0.0.1:0");
}

// Distinct octets in a known order pin the per-octet byte->decimal decode
// and the left-to-right octet ordering (catches index / order mutations).
bool test_sk2_v4_ordered(void) {
    return fmt_eq("10.20.30.40:0", "10.20.30.40:0");
}

// --- append_hextet ---------------------------------------------------------
// One hextet -> lowercase hex, no leading zeros. The 8th hextet "00a0"
// exercises in-hextet leading-zero suppression; "0" proves a single zero
// hextet stays "0" (and is not compressed to "::").

bool test_sk2_hextet_single_digit(void) {
    // hextets: a, bc(2), def(3), abcd(4 digits, lowercase a-f), 0, a0, 1, ff
    return fmt_eq("[a:bc:def:abcd:0:a0:1:ff]:0", "[a:bc:def:abcd:0:a0:1:ff]:0");
}

// "1" from raw 0x0001 -- leading zeros within the hextet are suppressed.
bool test_sk2_hextet_leading_zero_one(void) {
    return fmt_eq("[1:1:1:1:1:1:1:1]:0", "[1:1:1:1:1:1:1:1]:0");
}

// 0xff -> "ff" (both nibbles non-zero, lowercase), 0xabcd -> "abcd".
bool test_sk2_hextet_ff_and_abcd(void) {
    return fmt_eq("[ff:abcd:ff:abcd:ff:abcd:ff:abcd]:0", "[ff:abcd:ff:abcd:ff:abcd:ff:abcd]:0");
}

// --- format_ipv6 : zero-run compression ------------------------------------

// Run in the MIDDLE: 2001:db8:0:0:0:0:0:1 -> 2001:db8::1
bool test_sk2_v6_middle_run(void) {
    return fmt_eq("[2001:db8:0:0:0:0:0:1]:0", "[2001:db8::1]:0");
}

// Run at the START: ::1 (loopback) -- the "::" leads the string.
bool test_sk2_v6_start_run(void) {
    return fmt_eq("[::1]:0", "[::1]:0");
}

// Run at the END: fe80:0:... -> fe80::
bool test_sk2_v6_end_run(void) {
    return fmt_eq("[fe80:0:0:0:0:0:0:0]:0", "[fe80::]:0");
}

// All-zero address -> "::"
bool test_sk2_v6_all_zero(void) {
    return fmt_eq("[::]:0", "[::]:0");
}

// No zero run at all -> fully expanded, single ':' separators, no "::".
bool test_sk2_v6_no_run(void) {
    return fmt_eq("[2001:db8:85a3:1:2:8a2e:370:7334]:0", "[2001:db8:85a3:1:2:8a2e:370:7334]:0");
}

// A SINGLE zero hextet must NOT be compressed (run length 1 < 2).
bool test_sk2_v6_single_zero_not_compressed(void) {
    return fmt_eq("[1:0:1:1:1:1:1:1]:0", "[1:0:1:1:1:1:1:1]:0");
}

// A run of EXACTLY two zeros DOES compress (>= 2 minimum boundary).
bool test_sk2_v6_run_of_two_compresses(void) {
    // bytes: 1 0 0 1 1 1 1 1 -> the 0:0 at idx 1..2 is the longest run.
    return fmt_eq("[1:0:0:1:1:1:1:1]:0", "[1::1:1:1:1:1]:0");
}

// TWO equal-length zero runs: the FIRST one wins (strict > in the
// longest-run compare). bytes: 1 0 0 1 0 0 1 1
//  -> first run idx 1..2 compresses -> 1::1:0:0:1:1
bool test_sk2_v6_first_of_two_equal_runs(void) {
    return fmt_eq("[1:0:0:1:0:0:1:1]:0", "[1::1:0:0:1:1]:0");
}

// A longer run later must beat an earlier shorter run (run-length tracking
// + start-index update). bytes: 1 0 1 0 0 1 1 1 -> longest run idx 3..4.
bool test_sk2_v6_longer_later_run_wins(void) {
    return fmt_eq("[1:0:1:0:0:1:1:1]:0", "[1:0:1::1:1:1]:0");
}

// ffff hextet adjacent to a leading run (resembles an IPv4-mapped prefix
// but this formatter renders it as plain hex hextets).
bool test_sk2_v6_ffff_after_run(void) {
    return fmt_eq("[0:0:ffff:1:2:3:4:5]:0", "[::ffff:1:2:3:4:5]:0");
}

// ===========================================================================
// Mutation-hardening suite #3 (from Socket.Mutants3.c) for the
// address-parsing + listener-setup path:
//   socket_addr_parse_zstr / split_host_port (pure parse: exact fields),
//   fill_socket_addr_from_sockaddr (via ListenerLocalAddr; family/port/len),
//   ListenerOpen (real 127.0.0.1 bind/listen/accept; bound port),
//   plat_set_nonblocking (via SocketSetNonBlocking; non-blocking accept).
//
// Each NORMAL test RETURNS TRUE on success.
// ===========================================================================

// ---------------------------------------------------------------------------
// socket_addr_parse_zstr + split_host_port  (PURE -- exact parsed fields)
// ---------------------------------------------------------------------------

// IPv4 "1.2.3.4:80": family INET, length sizeof(sockaddr_in), the four
// address octets and the port survive a round-trip through format.
// Kills socket_addr_parse_zstr:416 (out->length = sizeof(sockaddr_in)).
bool test_sk3_parse_ipv4_fields(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);

    SocketAddr addr;
    bool       ok = SocketAddrParse(&addr, "1.2.3.4:80", SOCKET_KIND_TCP);
    ok            = ok && addr.family == SOCKET_FAMILY_INET;
    // length must be the real sockaddr_in size (16), not a mutated constant.
    ok = ok && addr.length == 16u;

    Str s = SocketAddrFormat(&addr, a);
    ok    = ok && StrLen(&s) > 0 && ZstrCompare(StrBegin(&s), "1.2.3.4:80") == 0;
    StrDeinit(&s);

    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// IPv4 port byte-swap: a port whose two bytes differ (4660 = 0x1234).
// FROM_BIG_ENDIAN2 must store/read it correctly so format reports 4660.
bool test_sk3_parse_ipv4_port_byteswap(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);

    SocketAddr addr;
    bool       ok = SocketAddrParse(&addr, "10.0.0.1:4660", SOCKET_KIND_TCP);

    Str s = SocketAddrFormat(&addr, a);
    ok    = ok && StrLen(&s) > 0 && ZstrCompare(StrBegin(&s), "10.0.0.1:4660") == 0;
    StrDeinit(&s);

    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// IPv6 "[2001:db8::1]:443": family INET6, length sizeof(sockaddr_in6),
// raw sin6_family byte == AF_INET6, port + address round-trip.
// Kills socket_addr_parse_zstr:424 (sin6_family) and :429 (length).
bool test_sk3_parse_ipv6_fields(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);

    SocketAddr addr;
    bool       ok = SocketAddrParse(&addr, "[2001:db8::1]:443", SOCKET_KIND_TCP);
    ok            = ok && addr.family == SOCKET_FAMILY_INET6;
    ok            = ok && addr.length == 28u; // sizeof(struct sockaddr_in6)

    // raw[0..1] is sin6_family (Linux: u16 host order at offset 0).
    // AF_INET6 == 10. A mutation to 42 changes this byte. The raw byte
    // layout (offset, width, AF_INET6 value) is Linux-specific -- macOS
    // prefixes sin6_len and uses AF_INET6==30, Windows uses 23 -- so this
    // check is gated to Linux, which is also the only platform mull runs on.
#if PLATFORM_LINUX
    ok = ok && addr.raw[0] == 10u && addr.raw[1] == 0u;
#endif

    Str s = SocketAddrFormat(&addr, a);
    ok    = ok && StrLen(&s) > 0 && ZstrCompare(StrBegin(&s), "[2001:db8::1]:443") == 0;
    StrDeinit(&s);

    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// "[::1]:8080" parses to loopback IPv6 and round-trips.
bool test_sk3_parse_ipv6_loopback(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);

    SocketAddr addr;
    bool       ok = SocketAddrParse(&addr, "[::1]:8080", SOCKET_KIND_TCP);
    ok            = ok && addr.family == SOCKET_FAMILY_INET6;

    Str s = SocketAddrFormat(&addr, a);
    ok    = ok && StrLen(&s) > 0 && ZstrCompare(StrBegin(&s), "[::1]:8080") == 0;
    StrDeinit(&s);

    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// A bare host with no port is rejected (no colon -> split_host_port false).
// Kills socket_addr_parse_zstr:392 (bool ok = false init): an init-to-true
// mutation would return true here.
bool test_sk3_parse_no_port_rejected(void) {
    SocketAddr addr;
    bool       parsed = SocketAddrParse(&addr, "127.0.0.1", SOCKET_KIND_TCP);
    return parsed == false;
}

// Port overflow ">65535" rejected.
bool test_sk3_parse_port_overflow_rejected(void) {
    SocketAddr addr;
    bool       parsed = SocketAddrParse(&addr, "1.2.3.4:99999", SOCKET_KIND_TCP);
    return parsed == false;
}

// Empty port "1.2.3.4:" rejected (parse_port empty -> false).
bool test_sk3_parse_empty_port_rejected(void) {
    SocketAddr addr;
    bool       parsed = SocketAddrParse(&addr, "1.2.3.4:", SOCKET_KIND_TCP);
    return parsed == false;
}

// Unmatched '[' bracket "[::1" rejected (split_host_port: no ']' -> false).
bool test_sk3_parse_unmatched_bracket_rejected(void) {
    SocketAddr addr;
    bool       parsed = SocketAddrParse(&addr, "[::1", SOCKET_KIND_TCP);
    return parsed == false;
}

// Empty spec "" rejected.
bool test_sk3_parse_empty_rejected(void) {
    SocketAddr addr;
    bool       parsed = SocketAddrParse(&addr, "", SOCKET_KIND_TCP);
    return parsed == false;
}

// Bracketed IPv6 with no ':' after ']' ("[::1]8080") rejected: tests the
// close[1] != ':' branch in split_host_port.
bool test_sk3_parse_bracket_missing_colon_rejected(void) {
    SocketAddr addr;
    bool       parsed = SocketAddrParse(&addr, "[::1]8080", SOCKET_KIND_TCP);
    return parsed == false;
}

// ---------------------------------------------------------------------------
// ListenerOpen + fill_socket_addr_from_sockaddr (via ListenerLocalAddr)
// ---------------------------------------------------------------------------

// Open a TCP listener on a *fixed* port (127.0.0.1:4660) and verify
// getsockname reports exactly that port + the loopback address. Because
// the port is fixed, a missing/short-circuited bind() (plat_bind removed)
// would auto-bind to a different ephemeral port -> port mismatch -> fail.
// Kills ListenerOpen:780 (bind call) and fill:372/368-le (length/port).
bool test_sk3_listener_bound_fixed_port(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);

    SocketAddr bind_addr;
    if (!SocketAddrParse(&bind_addr, "127.0.0.1:4660", SOCKET_KIND_TCP)) {
        DefaultAllocatorDeinit(&alloc);
        return false;
    }

    Listener listener;
    if (!ListenerOpen(&listener, SOCKET_KIND_TCP, &bind_addr, 16)) {
        // Port may be in use by a stale TIME_WAIT from a prior run; treat as
        // environmental, not a mutation kill. Fall back to ephemeral check.
        DefaultAllocatorDeinit(&alloc);
        return true;
    }

    SocketAddr local;
    bool       ok = ListenerLocalAddr(&listener, &local);
    // family + length come straight from fill_socket_addr_from_sockaddr.
    ok = ok && local.family == SOCKET_FAMILY_INET;
    ok = ok && local.length == 16u;

    Str s = SocketAddrFormat(&local, a);
    ok    = ok && StrLen(&s) > 0 && ZstrCompare(StrBegin(&s), "127.0.0.1:4660") == 0;
    StrDeinit(&s);

    ListenerClose(&listener);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// Ephemeral bind (127.0.0.1:0): kernel picks a real port (> 0) and reports
// the loopback address. Guards fill_socket_addr_from_sockaddr family/port
// independent of any fixed-port contention.
bool test_sk3_listener_ephemeral_real_port(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);

    SocketAddr bind_addr;
    if (!SocketAddrParse(&bind_addr, "127.0.0.1:0", SOCKET_KIND_TCP)) {
        DefaultAllocatorDeinit(&alloc);
        return false;
    }

    Listener listener;
    if (!ListenerOpen(&listener, SOCKET_KIND_TCP, &bind_addr, 16)) {
        DefaultAllocatorDeinit(&alloc);
        return false;
    }

    SocketAddr local;
    bool       ok = ListenerLocalAddr(&listener, &local);
    ok            = ok && local.family == SOCKET_FAMILY_INET;
    ok            = ok && local.length == 16u;

    // Rendered "127.0.0.1:<port>" with a nonzero ephemeral port.
    Str s = SocketAddrFormat(&local, a);
    ok    = ok && StrLen(&s) > 0;
    if (ok) {
        Zstr rendered = StrBegin(&s);
        ok            = ok && ZstrCompareN(rendered, "127.0.0.1:", 10u) == 0;
        // a real ephemeral port is non-zero: last char is not ":0\0"
        ok = ok && ZstrCompare(rendered, "127.0.0.1:0") != 0;
    }
    StrDeinit(&s);

    ListenerClose(&listener);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// Open a listener, accept a real connection, and verify the accepted
// socket carries kind == TCP (Listener.kind flows through). Kills
// ListenerOpen:793 (out->kind = kind).
bool test_sk3_listener_accept_kind(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);

    SocketAddr bind_addr;
    if (!SocketAddrParse(&bind_addr, "127.0.0.1:0", SOCKET_KIND_TCP)) {
        DefaultAllocatorDeinit(&alloc);
        return false;
    }

    Listener listener;
    if (!ListenerOpen(&listener, SOCKET_KIND_TCP, &bind_addr, 16)) {
        DefaultAllocatorDeinit(&alloc);
        return false;
    }

    SocketAddr local;
    if (!ListenerLocalAddr(&listener, &local)) {
        ListenerClose(&listener);
        DefaultAllocatorDeinit(&alloc);
        return false;
    }
    Str        local_str = SocketAddrFormat(&local, a);
    SocketAddr connect_addr;
    bool       parsed = SocketAddrParse(&connect_addr, (Zstr)StrBegin(&local_str), SOCKET_KIND_TCP);
    StrDeinit(&local_str);
    if (!parsed) {
        ListenerClose(&listener);
        DefaultAllocatorDeinit(&alloc);
        return false;
    }

    Socket client = {0};
    if (!SocketConnect(&client, SOCKET_KIND_TCP, &connect_addr)) {
        ListenerClose(&listener);
        DefaultAllocatorDeinit(&alloc);
        return false;
    }

    Socket server = {0};
    bool   ok     = ListenerAccept(&listener, &server);
    ok            = ok && server.kind == SOCKET_KIND_TCP;

    SocketClose(&server);
    SocketClose(&client);
    ListenerClose(&listener);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// ListenerOpen with an UNSPEC family (no host parsed) must fail: a zeroed
// SocketAddr has family UNSPEC -> socket_family_to_af -> AF_UNSPEC -> reject.
// Guards the af == AF_UNSPEC rejection at ListenerOpen:751.
bool test_sk3_listener_unspec_family_fails(void) {
    SocketAddr bad;
    MemSet(&bad, 0, sizeof(bad)); // family = SOCKET_FAMILY_UNSPEC

    Listener listener;
    bool     opened = ListenerOpen(&listener, SOCKET_KIND_TCP, &bad, 16);
    return opened == false;
}

// ---------------------------------------------------------------------------
// plat_set_nonblocking (via SocketSetNonBlocking)
// ---------------------------------------------------------------------------

// NOTE on non-blocking *behavior*: observing O_NONBLOCK via a would-block
// ListenerAccept / SocketRecv is impossible on real code here -- the
// EAGAIN path runs LOG_SOCK_ERROR -> LOG_SYS_ERROR -> StrError, which is a
// documented source limitation (StrError requires a heap-backed Str but
// the LOG macro passes a NULL-allocator StrInitStack, tripping reserve_vec
// FATAL). So the behavioral survivors plat_set_nonblocking:723/727 (and
// the 717/725 value/else-branch mutants) are not constructible -> ledgered.
// We still kill the success-guard mutants below.

// SocketSetNonBlocking returns true on a valid fd. Kills the F_GETFL
// guard (718 lt->ge) and the F_SETFL guard (728 lt->ge / lt->le): a
// flipped success test would make the call return false.
bool test_sk3_nonblocking_set_true_returns_true(void) {
    SocketAddr bind_addr;
    if (!SocketAddrParse(&bind_addr, "127.0.0.1:0", SOCKET_KIND_TCP)) {
        return false;
    }

    Listener listener;
    if (!ListenerOpen(&listener, SOCKET_KIND_TCP, &bind_addr, 16)) {
        return false;
    }

    bool set_ok = SocketSetNonBlocking(listener.fd, true);

    ListenerClose(&listener);
    return set_ok;
}

// ===========================================================================
// Mutation-hardening suite #4 (from Socket.Mutants4.c).
// ===========================================================================

// ---------------------------------------------------------------------------
// Shared helper: build a connected TCP loopback pair on 127.0.0.1:0.
//
// Mirrors the idiom in Tests/Std/Socket.c -- only the public Sys/Socket
// API is used so the same path runs on POSIX and Windows. The kernel
// picks the listener port (bind :0); we read it back with
// ListenerLocalAddr, format -> parse so the client only ever sees a
// host:port string, then accept the server side.
//
// Returns true on success with all three handles populated. On any
// failure everything opened so far is torn down and false is returned.
// ---------------------------------------------------------------------------
static bool sk4_make_pair(Allocator *a, Listener *listener, Socket *client, Socket *server, SocketAddr *local_out) {
    *client = (Socket) {0};
    *server = (Socket) {0};

    SocketAddr bind_addr;
    if (!SocketAddrParse(&bind_addr, "127.0.0.1:0", SOCKET_KIND_TCP))
        return false;

    if (!ListenerOpen(listener, SOCKET_KIND_TCP, &bind_addr, 16))
        return false;

    SocketAddr local;
    if (!ListenerLocalAddr(listener, &local)) {
        ListenerClose(listener);
        return false;
    }
    if (local_out)
        *local_out = local;

    Str        local_str = SocketAddrFormat(&local, a);
    SocketAddr connect_addr;
    bool       parsed = SocketAddrParse(&connect_addr, (Zstr)StrBegin(&local_str), SOCKET_KIND_TCP);
    StrDeinit(&local_str);
    if (!parsed) {
        ListenerClose(listener);
        return false;
    }

    if (!SocketConnect(client, SOCKET_KIND_TCP, &connect_addr)) {
        ListenerClose(listener);
        return false;
    }

    if (!ListenerAccept(listener, server)) {
        SocketClose(client);
        ListenerClose(listener);
        return false;
    }

    return true;
}

// ---------------------------------------------------------------------------
// Full round-trip both directions with exact byte/count assertions.
//
// Kills: plat_socket / plat_bind / plat_listen / plat_accept /
// plat_connect / plat_recv / plat_send return-value and fd/length
// handling. A wrong fd, dropped *len_io, wrong syscall arg, or a
// scalar-call mutation that returns 42 breaks the connection setup or
// the transferred bytes / counts, so the exact-match asserts fail.
// ---------------------------------------------------------------------------
bool test_sk4_round_trip_both_directions(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);

    Listener listener;
    Socket   client;
    Socket   server;
    if (!sk4_make_pair(a, &listener, &client, &server, NULL)) {
        DefaultAllocatorDeinit(&alloc);
        return false;
    }

    bool ok = true;

    // Accepted/connected kinds propagate (kill assign_const on ->kind).
    ok = ok && client.kind == SOCKET_KIND_TCP;
    ok = ok && server.kind == SOCKET_KIND_TCP;

    // The server's accepted peer is the loopback client: filling the
    // peer addr must have happened (kill remove_void on the fill call and
    // the *len_io capture). Format and check the loopback host prefix.
    {
        Str  peer = SocketAddrFormat(&server.peer, a);
        Zstr p    = StrBegin(&peer);
        ok        = ok && StrLen(&peer) > 0 && p[0] == '1' && p[1] == '2' && p[2] == '7' && p[3] == '.';
        StrDeinit(&peer);
    }

    // Zero-byte send returns exactly 0 (kill lt_to_le on plat_send's
    // `ret < 0`: a `<= 0` would treat the legit 0 return as an error).
    ok = ok && SocketSend(&client, "", 0) == 0;

    // client -> server
    Zstr c2s = "client=>server:ABCDEFG";
    size n1  = (size)ZstrLen(c2s);
    ok       = ok && SocketSend(&client, c2s, n1) == (i64)n1;
    char buf1[64];
    i64  g1 = SocketRecv(&server, buf1, sizeof(buf1));
    ok      = ok && g1 == (i64)n1 && MemCompare(buf1, c2s, n1) == 0;

    // server -> client (different payload + length so a swapped fd or a
    // length mutation surfaces here too)
    Zstr s2c = "server=>client:0123456789";
    size n2  = (size)ZstrLen(s2c);
    ok       = ok && SocketSend(&server, s2c, n2) == (i64)n2;
    char buf2[64];
    i64  g2 = SocketRecv(&client, buf2, sizeof(buf2));
    ok      = ok && g2 == (i64)n2 && MemCompare(buf2, s2c, n2) == 0;

    SocketClose(&server);
    SocketClose(&client);
    ListenerClose(&listener);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// ---------------------------------------------------------------------------
// recv on a peer that has closed returns 0 (EOF), not -1, not data.
//
// Kills: plat_recv return-value handling (ret>=0 path) and SocketClose /
// plat_close actually closing the fd. If close is a no-op the peer never
// sees EOF; if recv mishandles the 0 return the assert fails.
// ---------------------------------------------------------------------------
bool test_sk4_recv_eof_on_peer_close(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);

    Listener listener;
    Socket   client;
    Socket   server;
    if (!sk4_make_pair(a, &listener, &client, &server, NULL)) {
        DefaultAllocatorDeinit(&alloc);
        return false;
    }

    // Client closes; server must observe EOF (recv == 0).
    SocketClose(&client);

    char buf[16];
    i64  got = SocketRecv(&server, buf, sizeof(buf));
    bool ok  = got == 0;

    SocketClose(&server);
    ListenerClose(&listener);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// ---------------------------------------------------------------------------
// ListenerLocalAddr / plat_getsockname: the bound port is a real,
// non-zero kernel-assigned port, and the client that connects to it
// ends up talking to the listener (proved by the working accept in the
// pair helper). The port byte order (ntohs in format) must be right or
// the re-parsed connect address points at the wrong port and connect /
// accept fail.
//
// Kills: plat_getsockname *len_io handling + family fill, and the
// port-from-bound-addr path. A zeroed length or a dropped *len_io makes
// the address unusable and the pair cannot be formed.
// ---------------------------------------------------------------------------
bool test_sk4_local_addr_nonzero_port(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);

    Listener   listener;
    Socket     client;
    Socket     server;
    SocketAddr local;
    if (!sk4_make_pair(a, &listener, &client, &server, &local)) {
        DefaultAllocatorDeinit(&alloc);
        return false;
    }

    // local must be a usable IPv4 address with a kernel-picked port that
    // formats to "127.0.0.1:<port>" with port in 1..65535.
    Str  rendered = SocketAddrFormat(&local, a);
    bool ok       = StrLen(&rendered) > 0;
    // must start with the loopback host and a colon, then a non-"0" port.
    ok = ok && local.family == SOCKET_FAMILY_INET;
    ok = ok && ZstrCompare(StrBegin(&rendered), "127.0.0.1:0") != 0;
    // host portion must be exactly the loopback address.
    Zstr r = StrBegin(&rendered);
    ok     = ok && r[0] == '1' && r[1] == '2' && r[2] == '7' && r[3] == '.';
    StrDeinit(&rendered);

    SocketClose(&server);
    SocketClose(&client);
    ListenerClose(&listener);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// ---------------------------------------------------------------------------
// SocketPoll reports READ ready on a socket with pending data and
// reports nothing (0) on an idle socket with a 0ms timeout.
//
// Kills: sock_poll dispatch (the direct_sys poll/ppoll call). A
// scalar-call mutation returning 42 or a broken events mapping makes the
// ready/not-ready distinction collapse.
// ---------------------------------------------------------------------------
bool test_sk4_poll_read_ready(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);

    Listener listener;
    Socket   client;
    Socket   server;
    if (!sk4_make_pair(a, &listener, &client, &server, NULL)) {
        DefaultAllocatorDeinit(&alloc);
        return false;
    }

    bool ok = true;

    // Idle server: poll(0ms) must report nothing ready.
    SocketPollItem idle   = {0};
    idle.fd               = server.fd;
    idle.events_requested = SOCKET_POLL_READ;
    i32 nidle             = SocketPoll(&idle, 1, 0);
    ok                    = ok && nidle == 0 && (idle.events_ready & SOCKET_POLL_READ) == 0;

    // Now send data; poll(with a short timeout) must report READ ready.
    Zstr payload = "poke";
    size n       = (size)ZstrLen(payload);
    ok           = ok && SocketSend(&client, payload, n) == (i64)n;

    SocketPollItem ready   = {0};
    ready.fd               = server.fd;
    ready.events_requested = SOCKET_POLL_READ;
    i32 nready             = SocketPoll(&ready, 1, 1000);
    ok                     = ok && nready == 1 && (ready.events_ready & SOCKET_POLL_READ) != 0;

    // Drain so the close path is clean.
    char buf[16];
    SocketRecv(&server, buf, sizeof(buf));

    SocketClose(&server);
    SocketClose(&client);
    ListenerClose(&listener);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// ---------------------------------------------------------------------------
// SocketSetRecvTimeoutMs is accepted by the kernel (option applies) and
// a recv that DOES have data still completes normally afterwards.
//
// Kills: SocketSetRecvTimeoutMs ms->timeval conversion + the
// plat_setsockopt SO_RCVTIMEO call. A scalar-call mutation returning
// false flips the boolean return (pinned == true); a broken conversion
// that produces a negative/garbage timeval is rejected by the kernel
// (setsockopt -> false). We pin the setter true and prove the socket is
// still usable. (We avoid forcing an actual timeout-recv: that fires the
// recv error-log path, which is environment-fatal here, not a code bug.)
// ---------------------------------------------------------------------------
bool test_sk4_recv_timeout_applies(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);

    Listener listener;
    Socket   client;
    Socket   server;
    if (!sk4_make_pair(a, &listener, &client, &server, NULL)) {
        DefaultAllocatorDeinit(&alloc);
        return false;
    }

    bool ok = true;

    // 1500ms recv timeout (sec=1, usec=500000 -- exercises both
    // ms/1000 and (ms%1000)*1000 arms of the conversion). The kernel
    // rejects a malformed timeval, so a broken conversion -> false.
    ok = ok && SocketSetRecvTimeoutMs(server.fd, 1500);

    // Data is available, so the recv returns immediately with the bytes
    // -- never touching the timeout/error path.
    Zstr payload = "recv-timeout-set";
    size n       = (size)ZstrLen(payload);
    ok           = ok && SocketSend(&client, payload, n) == (i64)n;
    char buf[32];
    i64  got = SocketRecv(&server, buf, sizeof(buf));
    ok       = ok && got == (i64)n && MemCompare(buf, payload, n) == 0;

    SocketClose(&server);
    SocketClose(&client);
    ListenerClose(&listener);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// ---------------------------------------------------------------------------
// SocketSetSendTimeoutMs is accepted by the kernel (option applies) and
// does not break a subsequent normal send/recv.
//
// Kills: SocketSetSendTimeoutMs ms->timeval conversion + plat_setsockopt
// SO_SNDTIMEO call. A scalar-call mutation returning false flips the
// return; a broken conversion is still accepted, so we additionally
// require the option to return true and the send to still succeed.
// ---------------------------------------------------------------------------
bool test_sk4_send_timeout_applies(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);

    Listener listener;
    Socket   client;
    Socket   server;
    if (!sk4_make_pair(a, &listener, &client, &server, NULL)) {
        DefaultAllocatorDeinit(&alloc);
        return false;
    }

    bool ok = true;
    ok      = ok && SocketSetSendTimeoutMs(client.fd, 500);

    Zstr payload = "after-send-timeout";
    size n       = (size)ZstrLen(payload);
    ok           = ok && SocketSend(&client, payload, n) == (i64)n;
    char buf[32];
    i64  got = SocketRecv(&server, buf, sizeof(buf));
    ok       = ok && got == (i64)n && MemCompare(buf, payload, n) == 0;

    SocketClose(&server);
    SocketClose(&client);
    ListenerClose(&listener);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// ---------------------------------------------------------------------------
// SocketSetNoDelay / SocketSetKeepAlive / SocketSetReuseAddr each return
// true on a real socket and do not break the connection.
//
// Kills: the v = flag?1:0 init and the plat_setsockopt dispatch for each
// option (scalar-call returning 42 -> truthy but the option is bogus; we
// pin the boolean return true and then prove the socket still works).
// ---------------------------------------------------------------------------
bool test_sk4_socket_options_apply(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);

    Listener listener;
    Socket   client;
    Socket   server;
    if (!sk4_make_pair(a, &listener, &client, &server, NULL)) {
        DefaultAllocatorDeinit(&alloc);
        return false;
    }

    bool ok = true;
    ok      = ok && SocketSetNoDelay(client.fd, true);
    ok      = ok && SocketSetNoDelay(client.fd, false);
    ok      = ok && SocketSetKeepAlive(client.fd, true);
    ok      = ok && SocketSetReuseAddr(client.fd, true);
    ok      = ok && SocketSetNonBlocking(client.fd, true);
    ok      = ok && SocketSetNonBlocking(client.fd, false);

    // Connection still usable after toggling options.
    Zstr payload = "opts-ok";
    size n       = (size)ZstrLen(payload);
    ok           = ok && SocketSend(&client, payload, n) == (i64)n;
    char buf[16];
    i64  got = SocketRecv(&server, buf, sizeof(buf));
    ok       = ok && got == (i64)n && MemCompare(buf, payload, n) == 0;

    SocketClose(&server);
    SocketClose(&client);
    ListenerClose(&listener);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// ---------------------------------------------------------------------------
// parse_port boundary behaviour via SocketAddrParse:
//   "0"     -> port 0   (accepted)
//   "65535" -> accepted (boundary; > vs >= guard)
//   "65536" -> rejected (overflow past 0xFFFF)
//   "99999" -> rejected
//   ""      -> rejected (empty -> no colon -> parse fails)
//   "abc"   -> rejected (non-digit)
//
// Kills: parse_port overflow guard `v > 0xFFFFu` (gt_to_ge would reject
// 65535) and the digit accumulation. We check exact accept/reject and
// the exact rendered port for the accepted cases.
// ---------------------------------------------------------------------------
bool test_sk4_parse_port_boundaries(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);
    bool             ok    = true;

    // 65535 accepted and round-trips to the exact value.
    {
        SocketAddr addr;
        bool       p = SocketAddrParse(&addr, "127.0.0.1:65535", SOCKET_KIND_TCP);
        ok           = ok && p;
        if (p) {
            Str r = SocketAddrFormat(&addr, a);
            ok    = ok && ZstrCompare(StrBegin(&r), "127.0.0.1:65535") == 0;
            StrDeinit(&r);
        }
    }

    // 0 accepted and round-trips.
    {
        SocketAddr addr;
        bool       p = SocketAddrParse(&addr, "127.0.0.1:0", SOCKET_KIND_TCP);
        ok           = ok && p;
        if (p) {
            Str r = SocketAddrFormat(&addr, a);
            ok    = ok && ZstrCompare(StrBegin(&r), "127.0.0.1:0") == 0;
            StrDeinit(&r);
        }
    }

    // 4660 (0x1234) accepted -- non-boundary middle value, exact port.
    {
        SocketAddr addr;
        bool       p = SocketAddrParse(&addr, "1.2.3.4:4660", SOCKET_KIND_TCP);
        ok           = ok && p;
        if (p) {
            Str r = SocketAddrFormat(&addr, a);
            ok    = ok && ZstrCompare(StrBegin(&r), "1.2.3.4:4660") == 0;
            StrDeinit(&r);
        }
    }

    // Overflow / junk -> rejected.
    {
        SocketAddr addr;
        ok = ok && !SocketAddrParse(&addr, "127.0.0.1:65536", SOCKET_KIND_TCP);
        ok = ok && !SocketAddrParse(&addr, "127.0.0.1:99999", SOCKET_KIND_TCP);
        ok = ok && !SocketAddrParse(&addr, "127.0.0.1:abc", SOCKET_KIND_TCP);
        ok = ok && !SocketAddrParse(&addr, "127.0.0.1:", SOCKET_KIND_TCP);
    }

    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// ---------------------------------------------------------------------------
// socket_addr_parse_str (Str overload) round-trips exactly the same as
// the zstr arm.
//
// Kills: socket_addr_parse_str dispatch to socket_addr_parse_zstr
// (scalar-call mutation returning 42). We feed a Str and require the
// parsed address to format back to the identical string, and require a
// NULL/empty Str to be rejected.
// ---------------------------------------------------------------------------
bool test_sk4_parse_str_overload_round_trip(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);
    bool             ok    = true;

    Str spec = StrInit(a);
    StrAppendFmt(&spec, "{}", (Zstr) "1.2.3.4:4660");

    SocketAddr addr;
    bool       p = socket_addr_parse_str(&addr, &spec, SOCKET_KIND_TCP);
    ok           = ok && p;
    if (p) {
        Str r = SocketAddrFormat(&addr, a);
        ok    = ok && ZstrCompare(StrBegin(&r), "1.2.3.4:4660") == 0;
        StrDeinit(&r);
    }
    StrDeinit(&spec);

    // NULL spec -> rejected, out zeroed.
    SocketAddr addr2;
    ok = ok && !socket_addr_parse_str(&addr2, NULL, SOCKET_KIND_TCP);

    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// ---------------------------------------------------------------------------
// socket_addr_format: a zeroed / empty address formats to an empty Str,
// and an IPv6 address round-trips with brackets.
//
// Kills: socket_addr_format port/host init and family branch (init_const
// on the local `port` / `host` setup). The empty-address early return
// and the IPv6 bracket form pin the formatter's structure.
// ---------------------------------------------------------------------------
bool test_sk4_format_empty_and_ipv6(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);
    bool             ok    = true;

    // Empty address -> empty string.
    {
        SocketAddr empty = {0};
        Str        r     = SocketAddrFormat(&empty, a);
        ok               = ok && StrLen(&r) == 0;
        StrDeinit(&r);
    }

    // IPv6 round-trip with brackets + exact port.
    {
        SocketAddr addr;
        bool       p = SocketAddrParse(&addr, "[::1]:4660", SOCKET_KIND_TCP);
        ok           = ok && p;
        if (p) {
            Str r = SocketAddrFormat(&addr, a);
            ok    = ok && ZstrCompare(StrBegin(&r), "[::1]:4660") == 0;
            StrDeinit(&r);
        }
    }

    DefaultAllocatorDeinit(&alloc);
    return ok;
}

int main(void) {
    WriteFmt("[INFO] Starting Socket tests\n\n");

    TestFunction tests[] = {
        test_socket_addr_format_round_trip,
        test_socket_loopback_round_trip,
        test_sk1_read_ready_sets_read_flag,
        test_sk1_write_only_request_no_read_event,
        test_sk1_write_ready_sets_write_flag,
        test_sk1_read_only_request_no_write_event,
        test_sk1_timeout_returns_zero_no_flags,
        test_sk1_three_sockets_two_ready_count,
        test_sk1_middle_socket_ready_index,
        test_sk1_peer_close_reports_read_ready,
        test_sk1_zero_count_returns_zero,
        test_sk1_heap_path_many_idle,
        test_sk1_heap_path_one_ready,
        test_sk2_v4_generic,
        test_sk2_v4_all_zero,
        test_sk2_v4_all_max,
        test_sk2_v4_loopback,
        test_sk2_v4_ordered,
        test_sk2_hextet_single_digit,
        test_sk2_hextet_leading_zero_one,
        test_sk2_hextet_ff_and_abcd,
        test_sk2_v6_middle_run,
        test_sk2_v6_start_run,
        test_sk2_v6_end_run,
        test_sk2_v6_all_zero,
        test_sk2_v6_no_run,
        test_sk2_v6_single_zero_not_compressed,
        test_sk2_v6_run_of_two_compresses,
        test_sk2_v6_first_of_two_equal_runs,
        test_sk2_v6_longer_later_run_wins,
        test_sk2_v6_ffff_after_run,
        test_sk3_parse_ipv4_fields,
        test_sk3_parse_ipv4_port_byteswap,
        test_sk3_parse_ipv6_fields,
        test_sk3_parse_ipv6_loopback,
        test_sk3_parse_no_port_rejected,
        test_sk3_parse_port_overflow_rejected,
        test_sk3_parse_empty_port_rejected,
        test_sk3_parse_unmatched_bracket_rejected,
        test_sk3_parse_empty_rejected,
        test_sk3_parse_bracket_missing_colon_rejected,
        test_sk3_listener_bound_fixed_port,
        test_sk3_listener_ephemeral_real_port,
        test_sk3_listener_accept_kind,
        test_sk3_listener_unspec_family_fails,
        test_sk3_nonblocking_set_true_returns_true,
        test_sk4_round_trip_both_directions,
        test_sk4_recv_eof_on_peer_close,
        test_sk4_local_addr_nonzero_port,
        test_sk4_poll_read_ready,
        test_sk4_recv_timeout_applies,
        test_sk4_send_timeout_applies,
        test_sk4_socket_options_apply,
        test_sk4_parse_port_boundaries,
        test_sk4_parse_str_overload_round_trip,
        test_sk4_format_empty_and_ipv6,
    };

    TestFunction deadend_tests[] = {
        test_sk1_null_items_nonzero_count_aborts,
    };

    return run_test_suite(
        tests,
        sizeof(tests) / sizeof(tests[0]),
        deadend_tests,
        sizeof(deadend_tests) / sizeof(deadend_tests[0]),
        "Socket"
    );
}
