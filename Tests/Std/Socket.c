#if !PLATFORM_WINDOWS
#    define _DEFAULT_SOURCE
#    define _POSIX_C_SOURCE 200809L
#endif

#include <Misra/Std/Allocator/Default.h>
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
    bool       parsed = SocketAddrParse(&connect_addr, local_str.data, SOCKET_KIND_TCP);
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

    const char *payload = "hello from socket test";
    size        n       = (size)ZstrLen(payload);
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
        ok           = ok && rendered.length > 0 && ZstrCompare(rendered.data, "127.0.0.1:8080") == 0;
        StrDeinit(&rendered);
    }

    {
        SocketAddr addr;
        if (!SocketAddrParse(&addr, "[::1]:8080", SOCKET_KIND_TCP)) {
            DefaultAllocatorDeinit(&alloc);
            return false;
        }
        Str rendered = SocketAddrFormat(&addr, alloc_base);
        ok           = ok && rendered.length > 0 && ZstrCompare(rendered.data, "[::1]:8080") == 0;
        StrDeinit(&rendered);
    }

    DefaultAllocatorDeinit(&alloc);
    return ok;
}

int main(void) {
    WriteFmt("[INFO] Starting Socket tests\n\n");

    TestFunction tests[] = {
        test_socket_addr_format_round_trip,
        test_socket_loopback_round_trip,
    };

    return run_test_suite(tests, sizeof(tests) / sizeof(tests[0]), NULL, 0, "Socket");
}
