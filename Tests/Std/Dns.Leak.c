/// file : tests/std/dns.leak.c
///
/// Success-path leak-guard tests for Dns: route every parse allocation through
/// an explicit DebugAllocator and assert `DebugAllocatorLiveCount(&dbg) == 0 &&
/// DebugAllocatorLiveBytes(&dbg) == 0` after `DnsResponseDeinit`, to KILL
/// `cxx_remove_void_call` survivors that drop an internal `*Deinit` on a
/// reachable cleanup branch (the leak-only proposals).
///
/// `DnsParseResponse` takes the caller's `alloc` and threads it through every
/// `StrInit` / `VecInitT` for the decoded record names, targets, raw rdata, and
/// the answer / authority / additional record Vecs. A non-empty Str / Vec has
/// no inline buffer (Str is `Vec(char)`), so its backing storage is a real
/// DebugAllocator slot -- a dropped `*Deinit` is therefore observable as a
/// non-zero live count after cleanup.
///
/// Distinct contract from the value-correctness tests in the sibling Dns.c
/// file -- these assert allocator live-count, not parsed field values.

#include <Misra.h>
#include <Misra/Std/Allocator/Debug.h>
#include <Misra/Std/Container/Str.h>
#include <Misra/Parsers/Dns.h>

#include "../Util/TestRunner.h"

// ---------------------------------------------------------------------------
// DnsRecordDeinit: StrDeinit(&self->target) (Dns.c:325).
//
// A CNAME record decodes its target name into a heap-backed Str. A removed
// StrDeinit(&self->target) leaks that target -> live count != 0 after cleanup.
// ---------------------------------------------------------------------------
bool test_leak_record_target_freed(void) {
    DebugAllocator dbg  = DebugAllocatorInit();
    Allocator     *adbg = ALLOCATOR_OF(&dbg);

    // header: id=0x0042 flags=0x8180 qd=1 an=1
    // question "www.example.com" (offset 12); answer: name -> 12, CNAME IN
    // ttl=60 rdlen=2 rdata=pointer to offset 16 ("example.com").
    static const u8 wire[] = {0x00, 0x42, 0x81, 0x80, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00,
                              0x03, 'w',  'w',  'w',  0x07, 'e',  'x',  'a',  'm',  'p',  'l',  'e',
                              0x03, 'c',  'o',  'm',  0x00, 0x00, 0x01, 0x00, 0x01, 0xc0, 0x0c, 0x00,
                              0x05, 0x00, 0x01, 0x00, 0x00, 0x00, 0x3c, 0x00, 0x02, 0xc0, 0x10};

    DnsResponse resp = {0};
    bool        ok   = DnsParseResponse(&resp, wire, sizeof(wire), adbg);
    ok               = ok && (VecLen(&resp.answers) == 1);
    DnsRecord *r     = ok ? VecPtrAt(&resp.answers, 0) : NULL;
    ok               = ok && (r->type == DNS_TYPE_CNAME);
    // Heap-backed target ("example.com") -- the Str the dropped StrDeinit leaks.
    ok = ok && (StrLen(&r->target) == 11);

    DnsResponseDeinit(&resp);
    ok = ok && (DebugAllocatorLiveCount(&dbg) == 0);
    ok = ok && (DebugAllocatorLiveBytes(&dbg) == 0);

    DebugAllocatorDeinit(&dbg);
    return ok;
}

// Build a 12-byte header with the given an/ns/ar counts (qd=0).
static void leak_hdr(u8 *w, u16 an, u16 ns, u16 ar) {
    w[0]  = 0x00;
    w[1]  = 0x00;
    w[2]  = 0x81;
    w[3]  = 0x80;
    w[4]  = 0x00;
    w[5]  = 0x00; // qd=0
    w[6]  = (u8)(an >> 8);
    w[7]  = (u8)(an & 0xff);
    w[8]  = (u8)(ns >> 8);
    w[9]  = (u8)(ns & 0xff);
    w[10] = (u8)(ar >> 8);
    w[11] = (u8)(ar & 0xff);
}

// One uncompressed single-label A record: name (1-byte label `c`), A IN ttl=1,
// 1.2.3.4. Heap-backs the record's name Str + raw rdata Vec, and the record is
// pushed into the section Vec (which also heap-backs its own storage).
static u64 leak_put_a_record(u8 *w, u64 p, char c) {
    w[p++] = 0x01;
    w[p++] = (u8)c;
    w[p++] = 0x00; // root terminator
    w[p++] = 0x00;
    w[p++] = 0x01; // type A
    w[p++] = 0x00;
    w[p++] = 0x01; // class IN
    w[p++] = 0x00;
    w[p++] = 0x00;
    w[p++] = 0x00;
    w[p++] = 0x01; // ttl=1
    w[p++] = 0x00;
    w[p++] = 0x04; // rdlen=4
    w[p++] = 0x01;
    w[p++] = 0x02;
    w[p++] = 0x03;
    w[p++] = 0x04;
    return p;
}

// ---------------------------------------------------------------------------
// DnsResponseDeinit: deinit_record_list(&self->authority) (Dns.c:344).
//
// A single authority record (no answers / additional). A removed
// deinit_record_list on the authority section leaks that whole list.
// ---------------------------------------------------------------------------
bool test_leak_authority_list_freed(void) {
    DebugAllocator dbg  = DebugAllocatorInit();
    Allocator     *adbg = ALLOCATOR_OF(&dbg);

    u8  wire[64];
    u64 p = 12;
    leak_hdr(wire, 0, 1, 0);
    p = leak_put_a_record(wire, p, 'n');

    DnsResponse resp = {0};
    bool        ok   = DnsParseResponse(&resp, wire, p, adbg);
    ok               = ok && (VecLen(&resp.authority) == 1);
    ok               = ok && (VecLen(&resp.answers) == 0) && (VecLen(&resp.additional) == 0);

    DnsResponseDeinit(&resp);
    ok = ok && (DebugAllocatorLiveCount(&dbg) == 0);
    ok = ok && (DebugAllocatorLiveBytes(&dbg) == 0);

    DebugAllocatorDeinit(&dbg);
    return ok;
}

// ---------------------------------------------------------------------------
// DnsResponseDeinit: deinit_record_list(&self->additional) (Dns.c:345).
//
// A single additional record (no answers / authority). A removed
// deinit_record_list on the additional section leaks that whole list.
// ---------------------------------------------------------------------------
bool test_leak_additional_list_freed(void) {
    DebugAllocator dbg  = DebugAllocatorInit();
    Allocator     *adbg = ALLOCATOR_OF(&dbg);

    u8  wire[64];
    u64 p = 12;
    leak_hdr(wire, 0, 0, 1);
    p = leak_put_a_record(wire, p, 'd');

    DnsResponse resp = {0};
    bool        ok   = DnsParseResponse(&resp, wire, p, adbg);
    ok               = ok && (VecLen(&resp.additional) == 1);
    ok               = ok && (VecLen(&resp.answers) == 0) && (VecLen(&resp.authority) == 0);

    DnsResponseDeinit(&resp);
    ok = ok && (DebugAllocatorLiveCount(&dbg) == 0);
    ok = ok && (DebugAllocatorLiveBytes(&dbg) == 0);

    DebugAllocatorDeinit(&dbg);
    return ok;
}

int main(void) {
    TestFunction tests[] = {
        test_leak_record_target_freed,
        test_leak_authority_list_freed,
        test_leak_additional_list_freed,
    };
    TestFunction deadend_tests[] = {
        0,
    };
    (void)deadend_tests;
    return run_test_suite(tests, (int)(sizeof(tests) / sizeof(tests[0])), deadend_tests, 0, "Dns.Leak");
}
