#include <Misra.h>
#include <Misra/Parsers/Dns.h>
#include <Misra/Std/Allocator/Debug.h>
#include <Misra/Std/Container/Str.h>
#include <Misra/Std/Zstr.h>
#include "../Util/TestRunner.h"

// Lean DebugAllocator config used by leak round-trip tests: no trace
// capture, no overflow canaries, no freed history -- just the live count.
static DebugAllocator make_lean_dbg(void) {
    return DebugAllocatorInitWith(((DebugAllocatorConfig) {
        .capture_traces      = false,
        .detect_overflow     = false,
        .track_freed_history = false,
    }));
}

// ---------------------------------------------------------------------------
// Leak survivors (cxx_remove_void_call): deleting a Deinit on a failure path
// orphans owned allocations. We feed wire that drives the parser onto the
// targeted cleanup branch with a live owned allocation, assert the parse
// FAILS, deinit, and require the live count back at zero. If the Deinit is
// removed, the backing allocation leaks and the live count is non-zero.
// ---------------------------------------------------------------------------

// 299:13 -- StrDeinit(&dummy) in the question-skip loop when decode_name
// fails. The dummy question name decodes one heap-growing label and THEN
// hits an out-of-bounds label, so decode_name returns false while `dummy`
// holds a backing allocation.
static bool test_blind_question_name_fail_no_leak(void) {
    DebugAllocator dbg = make_lean_dbg();
    Allocator     *a   = ALLOCATOR_OF(&dbg);

    // header: qd=1 an=0; question name [10]"abcdefghij" then a 20-len label
    // with no bytes following -> overrun -> decode_name false.
    static const u8 wire[] = {0x00, 0x01, 0x81, 0x80, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                              0x0a, 'a',  'b',  'c',  'd',  'e',  'f',  'g',  'h',  'i',  'j',  0x14};

    DnsResponse resp = {0};
    bool        ok   = DnsParseResponse(&resp, wire, sizeof(wire), a);
    DnsResponseDeinit(&resp);

    bool match = (ok == false) && (DebugAllocatorLiveCount(&dbg) == 0);
    DebugAllocatorDeinit(&dbg);
    return match;
}

// 257:13 -- DnsRecordDeinit(&rec) when decode_record fails. The record
// decodes a real name (heap Str) and stashes rdata bytes (heap Vec), then the
// type-A length check rejects (rdlen != 4). rec->name and rec->rdata both
// hold backing allocations at that point.
static bool test_blind_record_decode_fail_no_leak(void) {
    DebugAllocator dbg = make_lean_dbg();
    Allocator     *a   = ALLOCATOR_OF(&dbg);

    // header: qd=0 an=1; answer name [10]"abcdefghij"+root, type=A class=IN
    // ttl=0 rdlen=6 (invalid for A) + 6 rdata bytes.
    static const u8 wire[] = {0x00, 0x00, 0x81, 0x80, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x0a, 'a',
                              'b',  'c',  'd',  'e',  'f',  'g',  'h',  'i',  'j',  0x00, 0x00, 0x01, 0x00, 0x01,
                              0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06};

    DnsResponse resp = {0};
    bool        ok   = DnsParseResponse(&resp, wire, sizeof(wire), a);
    DnsResponseDeinit(&resp);

    bool match = (ok == false) && (DebugAllocatorLiveCount(&dbg) == 0);
    DebugAllocatorDeinit(&dbg);
    return match;
}

// ---------------------------------------------------------------------------
// 126:21 -- compression-pointer 2-byte bounds check
//   `if (cur + 2 > IterLength(it))` -> `cur - 2 > IterLength(it)`.
// A pointer whose lead byte sits at the LAST buffer offset has no second
// offset byte and must be rejected (a pointer needs 2 bytes). Real: cur+2 >
// len -> reject. The `cur - 2` mutant underflows/stays small -> would read
// the missing 2nd byte out of bounds. Asserting rejection kills it.
// ---------------------------------------------------------------------------
static bool test_blind_pointer_truncated_rejected(void) {
    DebugAllocator dbg = make_lean_dbg();
    Allocator     *a   = ALLOCATOR_OF(&dbg);

    // header(12) then a lone 0xC0 pointer lead byte at EOF (no 2nd byte).
    static const u8 wire[] = {0x00, 0x01, 0x81, 0x80, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc0};

    DnsResponse resp = {0};
    bool        ok   = DnsParseResponse(&resp, wire, sizeof(wire), a);
    DnsResponseDeinit(&resp);

    bool match = (ok == false);
    DebugAllocatorDeinit(&dbg);
    return match;
}

// ---------------------------------------------------------------------------
// Compression-pointer chain helpers for the hop-counter survivors.
//
// The parser walks names linearly; a name field at offset 12 (the question)
// is what gets parsed first, so a long BACKWARD-pointer chain cannot be
// "entered" from offset 12 (nothing precedes it). Instead we bury the chain
// as opaque DATA inside the QUESTION name's labels (the linear walk skips
// over label payloads), then enter the chain from the ANSWER record's name
// pointer. Each chain entry points to the previous one (strictly decreasing
// offsets); the chain bottoms out at a self-contained "zz"\0 label.
//
// Following the answer name = 1 pointer hop (the answer's own pointer) plus
// one hop per chain entry, so `nptr` chain entries == nptr + 1 hops total.
// ---------------------------------------------------------------------------

static void put_ptr(u8 *w, u64 p, u64 off) {
    w[p]     = (u8)(0xC0u | ((off >> 8) & 0x3Fu));
    w[p + 1] = (u8)(off & 0xFFu);
}

// Build a response whose single ANSWER name resolves through nptr + 1 pointer
// hops onto a real "zz" label. Returns the wire length.
static u64 build_hop_chain(u8 *w, u64 nptr) {
    u64             p       = 0;
    static const u8 hdr[12] = {0x00, 0x55, 0x81, 0x80, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00};
    for (u64 i = 0; i < 12; ++i) {
        w[p++] = hdr[i];
    }

    // The question name's labels carry, as opaque data, the terminal "zz"\0
    // label followed by the chain pointers. We never split a 2-byte pointer
    // or the 4-byte terminal across a 63-byte label boundary.
    u64 chain_off[300];
    u64 term_off       = 0;
    u64 label_len_pos  = (u64)-1;
    u64 cur_label_data = 0;

    // Open a fresh label whenever none is open or the next item won't fit.
    // (Inlined below to keep the helper self-contained.)
    // terminal "zz"\0 first.
    if (label_len_pos == (u64)-1 || cur_label_data + 4 > 63) {
        if (label_len_pos != (u64)-1) {
            w[label_len_pos] = (u8)cur_label_data;
        }
        label_len_pos  = p;
        w[p++]         = 0;
        cur_label_data = 0;
    }
    term_off        = p;
    w[p++]          = 0x02;
    w[p++]          = 'z';
    w[p++]          = 'z';
    w[p++]          = 0x00;
    cur_label_data += 4;

    u64 prev = term_off;
    for (u64 k = 0; k < nptr; ++k) {
        if (cur_label_data + 2 > 63) {
            w[label_len_pos] = (u8)cur_label_data;
            label_len_pos    = p;
            w[p++]           = 0;
            cur_label_data   = 0;
        }
        u64 here = p;
        put_ptr(w, p, prev);
        p              += 2;
        cur_label_data += 2;
        chain_off[k]    = here;
        prev            = here;
    }
    if (label_len_pos != (u64)-1) {
        w[label_len_pos] = (u8)cur_label_data;
    }

    w[p++] = 0x00; // question name terminator
    w[p++] = 0x00; // qtype
    w[p++] = 0x01;
    w[p++] = 0x00; // qclass
    w[p++] = 0x01;

    // Answer name = pointer to the TOP chain entry (or straight to the
    // terminal when nptr == 0), then A record fields.
    u64 target = (nptr == 0) ? term_off : chain_off[nptr - 1];
    put_ptr(w, p, target);
    p      += 2;
    w[p++]  = 0x00; // type=A
    w[p++]  = 0x01;
    w[p++]  = 0x00; // class=IN
    w[p++]  = 0x01;
    w[p++]  = 0x00; // ttl=7
    w[p++]  = 0x00;
    w[p++]  = 0x00;
    w[p++]  = 0x07;
    w[p++]  = 0x00; // rdlen=4
    w[p++]  = 0x04;
    w[p++]  = 0x0a;
    w[p++]  = 0x0b;
    w[p++]  = 0x0c;
    w[p++]  = 0x0d;
    return p;
}

// 142:24 cxx_gt_to_ge -- `if (++hops > MAX_HOPS)` -> `>=`. A chain of EXACTLY
// 64 hops (nptr=63) is the legal maximum: real code does `++hops` to 64 and
// `64 > 64` is false -> continues and resolves. The `>=` mutant trips
// `64 >= 64` and rejects the boundary-legal name. Asserting a SUCCESSFUL
// resolve to "zz" kills it.
static bool test_blind_hops_max_boundary(void) {
    DebugAllocator dbg = make_lean_dbg();
    Allocator     *a   = ALLOCATOR_OF(&dbg);

    u8  wire[4096];
    u64 len = build_hop_chain(wire, 63); // 64 hops total

    DnsResponse resp  = {0};
    bool        ok    = DnsParseResponse(&resp, wire, len, a);
    bool        match = ok && VecLen(&resp.answers) == 1;
    if (match) {
        DnsRecord *r = VecPtrAt(&resp.answers, 0);
        match        = r->type == DNS_TYPE_A && r->ttl == 7 && ZstrCompare(StrBegin(&r->name), "zz") == 0;
    }

    DnsResponseDeinit(&resp);
    DebugAllocatorDeinit(&dbg);
    return match;
}

int main(void) {
    TestFunction tests[] = {
        test_blind_question_name_fail_no_leak,
        test_blind_record_decode_fail_no_leak,
        test_blind_pointer_truncated_rejected,
        test_blind_hops_max_boundary,
    };
    return run_test_suite(tests, sizeof(tests) / sizeof(tests[0]), NULL, 0, "Dns.Blind");
}
