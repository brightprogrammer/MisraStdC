#if !PLATFORM_WINDOWS
#    define _DEFAULT_SOURCE
#    define _POSIX_C_SOURCE 200809L
#endif

#include <Misra/Std/Allocator/Default.h>
#include <Misra/Std/Io.h>
#include <Misra/Std/Log.h>
#include <Misra/Std/Zstr.h>
#include <Misra/Sys/Socket.h>

#include "../Util/TestRunner.h"

// ===========================================================================
// Blind-spot mutation-hardening for Source/Misra/Sys/Socket.c.
//
// Each NORMAL test pins observable behaviour of a pure-logic helper reached
// through the public Sys/Socket API (address parse/format, getsockname fill).
// Only a 127.0.0.1 listener is used for the getsockname path. NORMAL tests
// RETURN true on success.
// ===========================================================================

// Parse `spec`, format the result, compare against `expect` exactly.
static bool bl_fmt_eq(Zstr spec, Zstr expect) {
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

// ---------------------------------------------------------------------------
// format_ipv6 hextet-build loop bound (212:23 lt_to_le).
//
// `for (i32 i = 0; i < 8; ++i) h[i] = (bytes[2i]<<8)|bytes[2i+1];`. With
// `<` -> `<=` the loop runs a 9th iteration writing h[8], one past the
// `u16 h[8]` array. The emit loop is bounded at 8 so the visible hextet
// count is unchanged, but the OOB write clobbers the adjacent stack slot
// (best_start / cur_start / cur_len live right after `h`), perturbing the
// zero-run compression decision. We pin BOTH a no-run address (must stay
// fully expanded -- a spurious "::" would appear if compression state is
// corrupted) and a single-run address (the "::" must land at the exact
// position). If the OOB write shifts the chosen run, one of these strings
// changes.
// ---------------------------------------------------------------------------
bool test_bl_v6_loop_bound_no_run(void) {
    return bl_fmt_eq("[2001:db8:85a3:1:2:8a2e:370:7334]:0", "[2001:db8:85a3:1:2:8a2e:370:7334]:0");
}

bool test_bl_v6_loop_bound_with_run(void) {
    return bl_fmt_eq("[2001:db8:0:0:0:0:0:1]:0", "[2001:db8::1]:0");
}

// ---------------------------------------------------------------------------
// fill_socket_addr_from_sockaddr non-clamped length copy, via a real
// loopback ListenerLocalAddr. getsockname on an IPv4 listener reports
// len == 16 (sizeof sockaddr_in). This pins out->length to the real 16 so
// any stray constant on the length path is caught, and confirms the family
// fill. (The 368/369 clamp branch itself is unreachable -- see report.)
// ---------------------------------------------------------------------------
bool test_bl_fill_length_is_real_16(void) {
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

    Str s = SocketAddrFormat(&local, a);
    ok    = ok && StrLen(&s) > 0 && ZstrCompareN(StrBegin(&s), "127.0.0.1:", 10u) == 0;
    StrDeinit(&s);

    ListenerClose(&listener);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

int main(void) {
    WriteFmt("[INFO] Starting Socket.Blind tests\n\n");

    TestFunction tests[] = {
        test_bl_v6_loop_bound_no_run,
        test_bl_v6_loop_bound_with_run,
        test_bl_fill_length_is_real_16,
    };

    TestFunction deadend_tests[] = {0};
    (void)deadend_tests;

    return run_test_suite(tests, sizeof(tests) / sizeof(tests[0]), deadend_tests, 0, "Socket.Blind");
}
