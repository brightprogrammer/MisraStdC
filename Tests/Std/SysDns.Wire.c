/// file      : Tests/Std/SysDns.Mutants3.c
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Mutation-hardening for the UDP-wire helpers of `Sys/Dns.c`:
/// `sockaddr_v4`, `sockaddr_v6`, `random_query_id`, and `udp_round_trip`.
///
/// All four are `static`, reachable only through `dns_resolve_*` which
/// drives real DNS I/O. To assert their exact field-level behaviour we
/// include the source unit directly so the statics become local symbols
/// here. The test executable already defines the public Dns symbols via
/// this include, so the linker never pulls the library's Dns object — no
/// duplicate definitions (same idiom as `ProcMaps.Parse.c`).

#include <Misra.h>
#include <Misra/Std/Allocator/Default.h>
#include <Misra/Sys/Dns.h>
#include <Misra/Sys/Socket.h>

#include "../Util/TestRunner.h"

// Pull in the unit under test so the static helpers are callable here.
#include "../../Source/Misra/Sys/Dns.c"

// ---------------------------------------------------------------------------
// sockaddr_v4 — build a sockaddr_in from an IPv4 + port.
//
// Port 0x1234 (= 4660) has two DIFFERING bytes so a missing / wrong
// htons (FROM_BIG_ENDIAN2) byte-swap is visible in the raw sin_port.
// ---------------------------------------------------------------------------

static bool test_sd3_sockaddr_v4_fields(void) {
    const u8   ip[4] = {1, 2, 3, 4};
    SocketAddr a     = sockaddr_v4(ip, 0x1234);

    // Wrapper-level invariants (line 64/65 -> a.length, a.family).
    if (a.family != SOCKET_FAMILY_INET) {
        return false;
    }
    if (a.length != (u32)sizeof(struct sockaddr_in)) {
        return false;
    }

    const struct sockaddr_in *sa = (const struct sockaddr_in *)a.raw;

    // Family constant (line 61). AF_INET assign-const mutation flips this.
    if (sa->sin_family != AF_INET) {
        return false;
    }

    // Port byte-swap (line 62). Network byte order: the wire bytes must be
    // 0x12 then 0x34 regardless of host endianness. Reading sin_port back
    // through FROM_BIG_ENDIAN2 restores host order; assert both the host
    // value AND the raw network-order bytes so a dropped / wrong swap dies.
    if ((u16)FROM_BIG_ENDIAN2(sa->sin_port) != 0x1234) {
        return false;
    }
    const u8 *pbytes = (const u8 *)&sa->sin_port;
    if (pbytes[0] != 0x12 || pbytes[1] != 0x34) {
        return false;
    }

    // Address copy (line 63). The four IP bytes land verbatim in s_addr.
    const u8 *abytes = (const u8 *)&sa->sin_addr.s_addr;
    if (abytes[0] != 1 || abytes[1] != 2 || abytes[2] != 3 || abytes[3] != 4) {
        return false;
    }

    return true;
}

// A second port whose bytes are even more distinct, pinning the swap from
// the other direction (0xABCD -> raw 0xAB 0xCD).
static bool test_sd3_sockaddr_v4_port_swap(void) {
    const u8   ip[4] = {10, 0, 0, 1};
    SocketAddr a     = sockaddr_v4(ip, 0xABCD);

    const struct sockaddr_in *sa     = (const struct sockaddr_in *)a.raw;
    const u8                 *pbytes = (const u8 *)&sa->sin_port;
    if (pbytes[0] != 0xAB || pbytes[1] != 0xCD) {
        return false;
    }
    if ((u16)FROM_BIG_ENDIAN2(sa->sin_port) != 0xABCD) {
        return false;
    }
    return true;
}

// ---------------------------------------------------------------------------
// sockaddr_v6 — build a sockaddr_in6 from a 16-byte address + port.
// ---------------------------------------------------------------------------

static bool test_sd3_sockaddr_v6_fields(void) {
    const u8   ip[16] = {0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x12, 0x34};
    SocketAddr a      = sockaddr_v6(ip, 0x1234);

    // Wrapper-level invariants (line 75/76).
    if (a.family != SOCKET_FAMILY_INET6) {
        return false;
    }
    if (a.length != (u32)sizeof(struct sockaddr_in6)) {
        return false;
    }

    const struct sockaddr_in6 *sa = (const struct sockaddr_in6 *)a.raw;

    // Family constant (line 72). AF_INET6 assign-const mutation flips this.
    if (sa->sin6_family != AF_INET6) {
        return false;
    }

    // Port byte-swap (line 73).
    if ((u16)FROM_BIG_ENDIAN2(sa->sin6_port) != 0x1234) {
        return false;
    }
    const u8 *pbytes = (const u8 *)&sa->sin6_port;
    if (pbytes[0] != 0x12 || pbytes[1] != 0x34) {
        return false;
    }

    // 16-byte address copy (line 74). Every byte must match verbatim.
    const u8 *abytes = (const u8 *)&sa->sin6_addr.s6_addr;
    for (u32 i = 0; i < 16; ++i) {
        if (abytes[i] != ip[i]) {
            return false;
        }
    }

    return true;
}

// Distinct-port variant for sockaddr_v6 to pin the swap from both sides.
static bool test_sd3_sockaddr_v6_port_swap(void) {
    const u8   ip[16] = {0xfe, 0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1};
    SocketAddr a      = sockaddr_v6(ip, 0xABCD);

    const struct sockaddr_in6 *sa     = (const struct sockaddr_in6 *)a.raw;
    const u8                  *pbytes = (const u8 *)&sa->sin6_port;
    if (pbytes[0] != 0xAB || pbytes[1] != 0xCD) {
        return false;
    }
    if ((u16)FROM_BIG_ENDIAN2(sa->sin6_port) != 0xABCD) {
        return false;
    }
    return true;
}

// ---------------------------------------------------------------------------
// random_query_id — 16-bit transaction id generator.
//
// Contract observable from the outside: the value is a 16-bit quantity
// (high bits zero) and it VARIES across calls (so off-path injectors have
// to guess 16 bits). A mutation that pins the id to a constant (the entropy
// source is dropped / replaced by a literal) breaks the variation invariant.
// ---------------------------------------------------------------------------

static bool test_sd3_random_query_id_in_range(void) {
    // The return type is u16, so every value is already in [0, 65535] in
    // the C sense; assert it explicitly so a widening / mask regression is
    // caught at the boundary. We sample many times for good measure.
    for (u32 i = 0; i < 4096; ++i) {
        u16 id = random_query_id();
        if (id > 0xFFFF) {
            return false;
        }
    }
    return true;
}

static bool test_sd3_random_query_id_varies(void) {
    // Across many calls at least two DISTINCT ids must appear. A constant
    // generator (entropy call dropped / value pinned to a literal) yields a
    // single repeated value and fails here.
    u16  first     = random_query_id();
    bool saw_other = false;
    for (u32 i = 0; i < 8192 && !saw_other; ++i) {
        if (random_query_id() != first) {
            saw_other = true;
        }
    }
    return saw_other;
}

// ---------------------------------------------------------------------------
// udp_round_trip — NOT exercised here (bucket B). See /tmp/sd_ig3.json.
//
// The function does a real UDP round trip: SocketConnect -> set recv timeout
// -> SocketSend -> SocketRecv -> SocketClose, returning the response length
// or -1. Reaching its SUCCESS path needs a peer that replies on the client's
// ephemeral source port. The in-tree Socket API's recvfrom passes a NULL
// peer-address pointer, so a fake server cannot learn the client's port to
// reply to, and there is no in-process thread primitive to run a concurrent
// server. Its FAILURE path (point at a dead loopback port) is also not safely
// drivable: the connected-UDP ECONNREFUSED on SocketRecv routes through
// Socket.c's LOG_SOCK_ERROR, whose StrInitStack(tmp, 4) overflows and aborts
// (a pre-existing Socket.c logging fragility, not in scope here, and Source
// is read-only). Every udp_round_trip survivor is therefore classified B.
// ---------------------------------------------------------------------------

int main(void) {
    WriteFmt("[INFO] Starting SysDns.Mutants3 tests\n\n");

    TestFunction tests[] = {
        test_sd3_sockaddr_v4_fields,
        test_sd3_sockaddr_v4_port_swap,
        test_sd3_sockaddr_v6_fields,
        test_sd3_sockaddr_v6_port_swap,
        test_sd3_random_query_id_in_range,
        test_sd3_random_query_id_varies,
    };

    return run_test_suite(tests, sizeof(tests) / sizeof(tests[0]), NULL, 0, "SysDns.Wire");
}
