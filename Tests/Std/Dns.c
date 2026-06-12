#include <Misra/Parsers/Dns.h>
#include <Misra/Std/Zstr.h>
#include <Misra/Std/Container/Str.h>
#include <Misra/Std/Allocator/Default.h>
#include <Misra/Std/Log.h>

#include "../Util/TestRunner.h"

// DnsBuildQuery should produce a well-formed RFC 1035 query: header
// with RD bit set, qdcount=1, the labels encoded as length-prefixed
// segments, terminator byte, qtype, qclass.
static bool test_dns_build_query_basic(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);

    DnsWireBuf buf = VecInitT(buf, a);
    bool       ok  = DnsBuildQuery(&buf, 0x1234, "example.com", DNS_TYPE_A);

    // Expected wire bytes (29 bytes total):
    //   id=0x1234, flags=0x0100 (RD), qd=1, an=0, ns=0, ar=0 -- 12 bytes
    //   "\x07example\x03com\x00"                              -- 13 bytes
    //   qtype=A(1) qclass=IN(1)                                -- 4 bytes
    static const u8 expected[] = {0x12, 0x34, 0x01, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00,
                                  0x00, 0x00, 0x07, 'e',  'x',  'a',  'm',  'p',  'l',  'e',
                                  0x03, 'c',  'o',  'm',  0x00, 0x00, 0x01, 0x00, 0x01};
    bool            match      = ok && BufLength(&buf) == sizeof(expected);
    for (u64 i = 0; match && i < sizeof(expected); ++i) {
        if (BufData(&buf)[i] != expected[i]) {
            match = false;
        }
    }

    VecDeinit(&buf);
    DefaultAllocatorDeinit(&alloc);
    return match;
}

// Trailing dot on the hostname must round-trip the same as no trailing dot.
static bool test_dns_build_query_trailing_dot(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);

    DnsWireBuf no_dot = VecInitT(no_dot, a);
    DnsWireBuf w_dot  = VecInitT(w_dot, a);
    bool ok = DnsBuildQuery(&no_dot, 1, "a.b.c", DNS_TYPE_AAAA) && DnsBuildQuery(&w_dot, 1, "a.b.c.", DNS_TYPE_AAAA);

    bool match = ok && BufLength(&no_dot) == BufLength(&w_dot);
    for (u64 i = 0; match && i < BufLength(&no_dot); ++i) {
        if (BufData(&no_dot)[i] != BufData(&w_dot)[i]) {
            match = false;
        }
    }

    VecDeinit(&no_dot);
    VecDeinit(&w_dot);
    DefaultAllocatorDeinit(&alloc);
    return match;
}

// Parsing a hand-crafted response: header + one question echo + two
// answers (one A, one AAAA pointing at the same name via compression).
static bool test_dns_parse_response_a_and_aaaa(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);

    // Wire bytes for the response to a query for "example.com" type A:
    //   id=0x1234 flags=0x8180 (response, RD, RA) qd=1 an=2 ns=0 ar=0
    //   question: "\x07example\x03com\x00" qtype=A qclass=IN
    //   answer1: name=0xC00C (pointer to offset 12, the question name)
    //            type=A class=IN ttl=300 rdlen=4 rdata=93.184.216.34
    //   answer2: name=0xC00C type=AAAA class=IN ttl=300 rdlen=16
    //            rdata=2606:2800:220:1:248:1893:25c8:1946
    static const u8 wire[] = {
        // header
        0x12,
        0x34,
        0x81,
        0x80,
        0x00,
        0x01,
        0x00,
        0x02,
        0x00,
        0x00,
        0x00,
        0x00,
        // question
        0x07,
        'e',
        'x',
        'a',
        'm',
        'p',
        'l',
        'e',
        0x03,
        'c',
        'o',
        'm',
        0x00,
        0x00,
        0x01,
        0x00,
        0x01,
        // answer 1: A record, name pointer to offset 12
        0xc0,
        0x0c,
        0x00,
        0x01,
        0x00,
        0x01,
        0x00,
        0x00,
        0x01,
        0x2c,
        0x00,
        0x04,
        0x5d,
        0xb8,
        0xd8,
        0x22,
        // answer 2: AAAA record
        0xc0,
        0x0c,
        0x00,
        0x1c,
        0x00,
        0x01,
        0x00,
        0x00,
        0x01,
        0x2c,
        0x00,
        0x10,
        0x26,
        0x06,
        0x28,
        0x00,
        0x02,
        0x20,
        0x00,
        0x01,
        0x02,
        0x48,
        0x18,
        0x93,
        0x25,
        0xc8,
        0x19,
        0x46
    };

    DnsResponse resp = {0};
    bool        ok   = DnsParseResponse(&resp, wire, sizeof(wire), a);

    bool match = ok && resp.id == 0x1234 && resp.is_response && resp.recursion_desired && resp.recursion_avail &&
                 resp.rcode == DNS_RCODE_NOERROR && VecLen(&resp.answers) == 2;

    if (match) {
        DnsRecord *r0 = VecPtrAt(&resp.answers, 0);
        match         = r0->type == DNS_TYPE_A && r0->ttl == 300 && r0->ipv4[0] == 93 && r0->ipv4[1] == 184 &&
                r0->ipv4[2] == 216 && r0->ipv4[3] == 34 && ZstrCompare(StrBegin(&r0->name), "example.com") == 0;
    }
    if (match) {
        DnsRecord *r1 = VecPtrAt(&resp.answers, 1);
        match = r1->type == DNS_TYPE_AAAA && r1->ipv6[0] == 0x26 && r1->ipv6[1] == 0x06 && r1->ipv6[14] == 0x19 &&
                r1->ipv6[15] == 0x46;
    }

    DnsResponseDeinit(&resp);
    DefaultAllocatorDeinit(&alloc);
    return match;
}

// NXDOMAIN: empty answer list, rcode == 3.
static bool test_dns_parse_response_nxdomain(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);

    static const u8 wire[] = {
        // header: id=1 flags=0x8183 (response, RD, RA, rcode=NXDOMAIN=3) qd=1 an=0 ns=0 ar=0
        0x00,
        0x01,
        0x81,
        0x83,
        0x00,
        0x01,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        // question: "nope.example.com" type=A class=IN
        0x04,
        'n',
        'o',
        'p',
        'e',
        0x07,
        'e',
        'x',
        'a',
        'm',
        'p',
        'l',
        'e',
        0x03,
        'c',
        'o',
        'm',
        0x00,
        0x00,
        0x01,
        0x00,
        0x01
    };

    DnsResponse resp  = {0};
    bool        ok    = DnsParseResponse(&resp, wire, sizeof(wire), a);
    bool        match = ok && resp.rcode == DNS_RCODE_NXDOMAIN && VecLen(&resp.answers) == 0;

    DnsResponseDeinit(&resp);
    DefaultAllocatorDeinit(&alloc);
    return match;
}

// CNAME pointing at a different name should decode the target.
static bool test_dns_parse_response_cname(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);

    // Response to "www.example.com" type A with one CNAME answer
    // pointing at "example.com".
    static const u8 wire[] = {
        // header: response, RD, RA, qd=1, an=1
        0x00,
        0x42,
        0x81,
        0x80,
        0x00,
        0x01,
        0x00,
        0x01,
        0x00,
        0x00,
        0x00,
        0x00,
        // question: "www.example.com" type=A class=IN -- begins at offset 12
        0x03,
        'w',
        'w',
        'w',
        0x07,
        'e',
        'x',
        'a',
        'm',
        'p',
        'l',
        'e',
        0x03,
        'c',
        'o',
        'm',
        0x00,
        0x00,
        0x01,
        0x00,
        0x01,
        // answer: name pointer to offset 12 (the "www.example.com")
        // type=CNAME(5) class=IN ttl=60 rdlen=14
        // rdata: pointer to offset 16 (the "example.com" suffix of the question)
        0xc0,
        0x0c,
        0x00,
        0x05,
        0x00,
        0x01,
        0x00,
        0x00,
        0x00,
        0x3c,
        0x00,
        0x02,
        0xc0,
        0x10
    };

    DnsResponse resp  = {0};
    bool        ok    = DnsParseResponse(&resp, wire, sizeof(wire), a);
    bool        match = ok && VecLen(&resp.answers) == 1;
    if (match) {
        DnsRecord *r = VecPtrAt(&resp.answers, 0);
        match        = r->type == DNS_TYPE_CNAME && ZstrCompare(StrBegin(&r->target), "example.com") == 0;
    }

    DnsResponseDeinit(&resp);
    DefaultAllocatorDeinit(&alloc);
    return match;
}

// ---------------------------------------------------------------------------
// encode_qname (via DnsBuildQuery zstr path)
// ---------------------------------------------------------------------------

// Exercise the multi-label encode loop and assert the EXACT wire bytes for
// "www.example.com": [3]www[7]example[3]com[0]. A label-length or offset
// arithmetic slip changes the emitted bytes.
static bool test_dn1_encode_exact_bytes(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);

    DnsWireBuf buf = VecInitT(buf, a);
    bool       ok  = DnsBuildQuery(&buf, 0x1234, "www.example.com", DNS_TYPE_A);

    static const u8 expected[] = {0x12, 0x34, 0x01, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00,
                                  0x00, 0x03, 'w',  'w',  'w',  0x07, 'e',  'x',  'a',  'm',  'p',
                                  'l',  'e',  0x03, 'c',  'o',  'm',  0x00, 0x00, 0x01, 0x00, 0x01};
    bool            match      = ok && BufLength(&buf) == sizeof(expected);
    for (u64 i = 0; match && i < sizeof(expected); ++i) {
        if (BufData(&buf)[i] != expected[i]) {
            match = false;
        }
    }

    VecDeinit(&buf);
    DefaultAllocatorDeinit(&alloc);
    return match;
}

// An empty name (root) encodes to a single 0 terminator byte preceded only
// by the 12-byte header; pins the empty-name / terminator path.
static bool test_dn1_encode_root(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);

    DnsWireBuf buf = VecInitT(buf, a);
    bool       ok  = DnsBuildQuery(&buf, 0x0001, "", DNS_TYPE_A);

    // header(12) + qname(just 0x00) + qtype(2) + qclass(2) = 17 bytes.
    bool match = ok && BufLength(&buf) == 17 && BufData(&buf)[12] == 0x00 && BufData(&buf)[13] == 0x00 &&
                 BufData(&buf)[14] == 0x01 && BufData(&buf)[15] == 0x00 && BufData(&buf)[16] == 0x01;

    VecDeinit(&buf);
    DefaultAllocatorDeinit(&alloc);
    return match;
}

// A label of EXACTLY 63 bytes is the legal maximum and must be accepted.
// Kills `seg_len > 63` -> `seg_len >= 63` (mutant would reject 63).
static bool test_dn1_encode_label_63_accepted(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);

    char label[64];
    for (u64 i = 0; i < 63; ++i) {
        label[i] = 'a';
    }
    label[63] = '\0';

    DnsWireBuf buf = VecInitT(buf, a);
    bool       ok  = DnsBuildQuery(&buf, 1, label, DNS_TYPE_A);

    // header(12) + [63]+63 label bytes + 0 term + 4 = 12+64+1+4 = 81; first
    // qname byte (offset 12) must be the length 63.
    bool match = ok && BufLength(&buf) == 81 && BufData(&buf)[12] == 63;

    VecDeinit(&buf);
    DefaultAllocatorDeinit(&alloc);
    return match;
}

// A trailing dot ("a.b." vs "a.b") marks the root and is accepted, producing
// the same bytes as no trailing dot. Kills the empty-trailing-label guard
// `StrIterRemainingLength == 0` -> `!= 0` (mutant rejects the trailing dot).
static bool test_dn1_encode_trailing_dot_accepted(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);

    DnsWireBuf no_dot = VecInitT(no_dot, a);
    DnsWireBuf w_dot  = VecInitT(w_dot, a);
    bool       ok     = DnsBuildQuery(&no_dot, 7, "a.b", DNS_TYPE_A) && DnsBuildQuery(&w_dot, 7, "a.b.", DNS_TYPE_A);

    bool match = ok && BufLength(&no_dot) == BufLength(&w_dot) && BufLength(&no_dot) > 0;
    for (u64 i = 0; match && i < BufLength(&no_dot); ++i) {
        if (BufData(&no_dot)[i] != BufData(&w_dot)[i]) {
            match = false;
        }
    }

    VecDeinit(&no_dot);
    VecDeinit(&w_dot);
    DefaultAllocatorDeinit(&alloc);
    return match;
}

// Build a Zstr name whose labels are `lens` (NUL-terminated, dot-joined).
static void build_label_name(char *out, const u8 *lens, u64 n) {
    u64 p = 0;
    for (u64 i = 0; i < n; ++i) {
        if (i > 0) {
            out[p++] = '.';
        }
        for (u64 j = 0; j < lens[i]; ++j) {
            out[p++] = 'a';
        }
    }
    out[p] = '\0';
}

// A name whose on-wire total length is 220 bytes (< 255, accepted). The
// `total_bytes` accumulator starts at 0; a value-substitution mutant that
// seeds it with 42 pushes the running total over the 254 cap and wrongly
// rejects. Kills `u64 total_bytes = 0` -> `= 42`.
static bool test_dn1_encode_total_init_220(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);

    // labels 63,63,63,27 -> wire total = 64+64+64+28 = 220.
    static const u8 lens[] = {63, 63, 63, 27};
    char            name[256];
    build_label_name(name, lens, 4);

    DnsWireBuf buf = VecInitT(buf, a);
    bool       ok  = DnsBuildQuery(&buf, 1, name, DNS_TYPE_A);

    // header(12) + wire 220 + 1 terminator + 4 = 237.
    bool match = ok && BufLength(&buf) == 237;

    VecDeinit(&buf);
    DefaultAllocatorDeinit(&alloc);
    return match;
}

// A name whose on-wire total length is EXACTLY 254 bytes is the legal
// maximum (`total_bytes > 254` is the cap) and must be accepted. Kills
// `total_bytes > 254` -> `>= 254` (mutant rejects at 254).
static bool test_dn1_encode_total_254_accepted(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);

    // labels 63,63,63,61 -> wire total = 64+64+64+62 = 254.
    static const u8 lens[] = {63, 63, 63, 61};
    char            name[256];
    build_label_name(name, lens, 4);

    DnsWireBuf buf = VecInitT(buf, a);
    bool       ok  = DnsBuildQuery(&buf, 1, name, DNS_TYPE_A);

    // header(12) + wire 254 + 1 terminator + 4 = 271.
    bool match = ok && BufLength(&buf) == 271;

    VecDeinit(&buf);
    DefaultAllocatorDeinit(&alloc);
    return match;
}

// ---------------------------------------------------------------------------
// dns_build_query_str (Str * overload)
// ---------------------------------------------------------------------------

// Route through the Str * overload so dns_build_query_str forwards
// StrBegin(name) into the encoder. Kills the scalar-call replacement of
// `StrBegin(name)` (mutant feeds a constant/NULL and the build fails).
static bool test_dn1_build_query_str_path(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);

    Str        name = StrInitFromZstr("example.com", a);
    DnsWireBuf buf  = VecInitT(buf, a);
    bool       ok   = DnsBuildQuery(&buf, 0x1234, &name, DNS_TYPE_A);

    static const u8 expected[] = {0x12, 0x34, 0x01, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00,
                                  0x00, 0x00, 0x07, 'e',  'x',  'a',  'm',  'p',  'l',  'e',
                                  0x03, 'c',  'o',  'm',  0x00, 0x00, 0x01, 0x00, 0x01};
    bool            match      = ok && BufLength(&buf) == sizeof(expected);
    for (u64 i = 0; match && i < sizeof(expected); ++i) {
        if (BufData(&buf)[i] != expected[i]) {
            match = false;
        }
    }

    VecDeinit(&buf);
    StrDeinit(&name);
    DefaultAllocatorDeinit(&alloc);
    return match;
}

// ---------------------------------------------------------------------------
// decode_name (via DnsParseResponse)
// ---------------------------------------------------------------------------

// Multi-label question name plus a COMPRESSED answer name (0xC0 pointer back
// to the question). Asserting the decoded answer name AND that the record's
// type/ttl/ipv4 land correctly pins:
//   - the label-copy loop reconstructs "www.example.com",
//   - the compression pointer is detected and followed,
//   - the linear cursor consumes exactly the 2 pointer bytes so the
//     type/class/ttl/rdata that follow parse from the right offset
//     (kills `linear_p = cur + 2` -> `linear_p = 42`).
static bool test_dn1_decode_compressed_answer(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);

    static const u8 wire[] = {
        // header: id=0x1234 flags=0x8180 qd=1 an=1 ns=0 ar=0
        0x12,
        0x34,
        0x81,
        0x80,
        0x00,
        0x01,
        0x00,
        0x01,
        0x00,
        0x00,
        0x00,
        0x00,
        // question "www.example.com" type=A class=IN  (name starts at offset 12)
        0x03,
        'w',
        'w',
        'w',
        0x07,
        'e',
        'x',
        'a',
        'm',
        'p',
        'l',
        'e',
        0x03,
        'c',
        'o',
        'm',
        0x00,
        0x00,
        0x01,
        0x00,
        0x01,
        // answer: name = pointer to offset 12, type=A class=IN ttl=300 rdlen=4
        0xc0,
        0x0c,
        0x00,
        0x01,
        0x00,
        0x01,
        0x00,
        0x00,
        0x01,
        0x2c,
        0x00,
        0x04,
        0x0a,
        0x0b,
        0x0c,
        0x0d
    };

    DnsResponse resp  = {0};
    bool        ok    = DnsParseResponse(&resp, wire, sizeof(wire), a);
    bool        match = ok && VecLen(&resp.answers) == 1;
    if (match) {
        DnsRecord *r = VecPtrAt(&resp.answers, 0);
        match        = r->type == DNS_TYPE_A && r->ttl == 300 && r->ipv4[0] == 0x0a && r->ipv4[1] == 0x0b &&
                r->ipv4[2] == 0x0c && r->ipv4[3] == 0x0d && ZstrCompare(StrBegin(&r->name), "www.example.com") == 0;
    }

    DnsResponseDeinit(&resp);
    DefaultAllocatorDeinit(&alloc);
    return match;
}

// A plain uncompressed multi-label name must decode. The `name_len`
// accumulator starts at 0; the cap check is `name_len + 1 + label_len > 253`.
// A mutant that turns the first `+` into `-` makes `name_len - 1` underflow
// (u64) on the very first label, so every name is wrongly rejected. This
// normal accept kills `cur + 1 + label_len` style `+`->`-` underflow on the
// name-length cap (line 155 col 22).
static bool test_dn1_decode_namelen_no_underflow(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);

    static const u8 wire[] = {
        // header: response qd=1 an=0
        0x00,
        0x05,
        0x81,
        0x80,
        0x00,
        0x01,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        // question "a.bb.ccc" type=A class=IN
        0x01,
        'a',
        0x02,
        'b',
        'b',
        0x03,
        'c',
        'c',
        'c',
        0x00,
        0x00,
        0x01,
        0x00,
        0x01
    };

    DnsResponse resp  = {0};
    bool        ok    = DnsParseResponse(&resp, wire, sizeof(wire), a);
    bool        match = ok && resp.id == 0x0005;

    DnsResponseDeinit(&resp);
    DefaultAllocatorDeinit(&alloc);
    return match;
}

// A question name whose decoded dotted length is EXACTLY 253 bytes is the
// legal maximum (cap is `name_len + 1 + label_len > 253`). Real accepts;
// a `> 253` -> `>= 253` mutant rejects at the final label. Kills line 155
// col 38.
static bool test_dn1_decode_namelen_253_accepted(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);

    // labels 63,63,63,61 -> dotted length = 250 chars + 3 dots = 253.
    // wire: 4 labels (64+64+64+62 = 254) + terminator = 255 bytes.
    static const u8 lens[] = {63, 63, 63, 61};
    u8              wire[12 + 255 + 4];
    u64             p = 0;
    // header: response qd=1 an=0
    static const u8 hdr[12] = {0x00, 0x09, 0x81, 0x80, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    for (u64 i = 0; i < 12; ++i) {
        wire[p++] = hdr[i];
    }
    for (u64 i = 0; i < 4; ++i) {
        wire[p++] = lens[i];
        for (u64 j = 0; j < lens[i]; ++j) {
            wire[p++] = 'a';
        }
    }
    wire[p++] = 0x00; // terminator
    // qtype=A qclass=IN
    wire[p++] = 0x00;
    wire[p++] = 0x01;
    wire[p++] = 0x00;
    wire[p++] = 0x01;

    DnsResponse resp  = {0};
    bool        ok    = DnsParseResponse(&resp, wire, p, a);
    bool        match = ok && resp.id == 0x0009;

    DnsResponseDeinit(&resp);
    DefaultAllocatorDeinit(&alloc);
    return match;
}

// A compression pointer whose target offset is >= 256 (so the high-byte of
// the 14-bit offset is non-zero) distinguishes:
//   - the pointer-detection mask `(b & 0xC0) == 0xC0` vs `(b | 0xC0) == 0xC0`
//     (the OR mutant fails to recognise 0xC1.. as a pointer), and
//   - the offset high-bits shift `(b & 0x3F) << 8` vs `>> 8`
//     (the RSHIFT mutant collapses the high bits to 0 -> wrong target).
// The earlier name at offset 256 says "zz"; the question name at offset 12
// says "qq". A correct decode resolves the answer name to "zz". Either mutant
// resolves it to "qq" / rejects, both observable.
static bool test_dn1_decode_bigoffset_pointer(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);

    // Layout: header(12) + Q1 (huge name padding to push Q2 to offset 256) +
    // Q2 name "zz" at offset 256 + answer whose name points to 256.
    // qd=2, an=1.
    u8  wire[700];
    u64 p = 0;
    // header: response qd=2 an=1 ns=0 ar=0
    static const u8 hdr[12] = {0x00, 0x11, 0x81, 0x80, 0x00, 0x02, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00};
    for (u64 i = 0; i < 12; ++i) {
        wire[p++] = hdr[i];
    }
    // Q1 name: labels 63,63,63,46 -> wire 64+64+64+47 = 239, +1 term = 240.
    // Q1 occupies offsets 12..251 (name) then qtype/qclass 252..255, so Q2
    // name begins at offset 256.
    static const u8 q1lens[] = {63, 63, 63, 46};
    for (u64 i = 0; i < 4; ++i) {
        wire[p++] = q1lens[i];
        for (u64 j = 0; j < q1lens[i]; ++j) {
            wire[p++] = 'a';
        }
    }
    wire[p++] = 0x00; // Q1 terminator  -> p now 252
    wire[p++] = 0x00; // qtype
    wire[p++] = 0x01;
    wire[p++] = 0x00; // qclass
    wire[p++] = 0x01; // -> p now 256

    // Q2 name "zz" at offset 256, then qtype/qclass.
    u64 q2_off = p; // expect 256
    wire[p++]  = 0x02;
    wire[p++]  = 'z';
    wire[p++]  = 'z';
    wire[p++]  = 0x00;
    wire[p++]  = 0x00; // qtype
    wire[p++]  = 0x01;
    wire[p++]  = 0x00; // qclass
    wire[p++]  = 0x01;

    // Answer: name = compression pointer to offset 256 (0x0100):
    //   high byte = 0xC0 | ((256 >> 8) & 0x3F) = 0xC0 | 0x01 = 0xC1
    //   low byte  = 256 & 0xFF = 0x00
    wire[p++] = 0xC1;
    wire[p++] = 0x00;
    // type=A class=IN ttl=1 rdlen=4 rdata=1.2.3.4
    wire[p++] = 0x00;
    wire[p++] = 0x01;
    wire[p++] = 0x00;
    wire[p++] = 0x01;
    wire[p++] = 0x00;
    wire[p++] = 0x00;
    wire[p++] = 0x00;
    wire[p++] = 0x01;
    wire[p++] = 0x00;
    wire[p++] = 0x04;
    wire[p++] = 0x01;
    wire[p++] = 0x02;
    wire[p++] = 0x03;
    wire[p++] = 0x04;

    DnsResponse resp  = {0};
    bool        ok    = DnsParseResponse(&resp, wire, p, a);
    bool        match = ok && q2_off == 256 && VecLen(&resp.answers) == 1;
    if (match) {
        DnsRecord *r = VecPtrAt(&resp.answers, 0);
        match        = r->type == DNS_TYPE_A && ZstrCompare(StrBegin(&r->name), "zz") == 0;
    }

    DnsResponseDeinit(&resp);
    DefaultAllocatorDeinit(&alloc);
    return match;
}

// A compression pointer that points BACKWARD to a valid earlier name must
// resolve correctly. This is the canonical 0xC0 0x0C pointer (offset 12,
// the question name). Pins `ptr >= cur` rejection not firing on a legitimate
// backward pointer, and the `cur = ptr` jump landing on the right label.
static bool test_dn1_decode_backward_pointer_ok(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);

    static const u8 wire[] = {
        // header: response qd=1 an=1
        0x00,
        0x21,
        0x81,
        0x80,
        0x00,
        0x01,
        0x00,
        0x01,
        0x00,
        0x00,
        0x00,
        0x00,
        // question "abc.de" type=A class=IN (name at offset 12)
        0x03,
        'a',
        'b',
        'c',
        0x02,
        'd',
        'e',
        0x00,
        0x00,
        0x01,
        0x00,
        0x01,
        // answer: name pointer -> 12, type=A class=IN ttl=2 rdlen=4 rdata
        0xc0,
        0x0c,
        0x00,
        0x01,
        0x00,
        0x01,
        0x00,
        0x00,
        0x00,
        0x02,
        0x00,
        0x04,
        0x09,
        0x08,
        0x07,
        0x06
    };

    DnsResponse resp  = {0};
    bool        ok    = DnsParseResponse(&resp, wire, sizeof(wire), a);
    bool        match = ok && VecLen(&resp.answers) == 1;
    if (match) {
        DnsRecord *r = VecPtrAt(&resp.answers, 0);
        match =
            r->type == DNS_TYPE_A && r->ttl == 2 && ZstrCompare(StrBegin(&r->name), "abc.de") == 0 && r->ipv4[0] == 9;
    }

    DnsResponseDeinit(&resp);
    DefaultAllocatorDeinit(&alloc);
    return match;
}

// A label length byte that claims more bytes than remain in the buffer must
// be rejected (out-of-bounds label). Pins the `cur + 1 + label_len >
// IterLength` bounds guard: a name whose final label overruns the buffer is
// not a valid parse.
static bool test_dn1_decode_label_overrun_rejected(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);

    // Question name claims a 10-byte label but only a few bytes follow and
    // the buffer ends -> overrun, parse must fail.
    static const u8 wire[] = {
        // header: response qd=1 an=0
        0x00,
        0x31,
        0x81,
        0x80,
        0x00,
        0x01,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        // question: label length 10 but only 3 bytes then EOF
        0x0a,
        'a',
        'b',
        'c'
    };

    DnsResponse resp  = {0};
    bool        ok    = DnsParseResponse(&resp, wire, sizeof(wire), a);
    bool        match = !ok; // must be rejected

    DnsResponseDeinit(&resp);
    DefaultAllocatorDeinit(&alloc);
    return match;
}

// A forward / self compression pointer (points at or after its own position)
// must be rejected to avoid cycles. Pins `ptr >= cur` rejection.
static bool test_dn1_decode_forward_pointer_rejected(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);

    // Question name is a single pointer at offset 12 pointing to offset 12
    // itself (self-pointer) -> must be rejected.
    static const u8 wire[] = {
        // header: response qd=1 an=0
        0x00,
        0x41,
        0x81,
        0x80,
        0x00,
        0x01,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        // self-pointer at offset 12 -> 12
        0xc0,
        0x0c,
        0x00,
        0x01,
        0x00,
        0x01
    };

    DnsResponse resp  = {0};
    bool        ok    = DnsParseResponse(&resp, wire, sizeof(wire), a);
    bool        match = !ok; // self pointer must be rejected

    DnsResponseDeinit(&resp);
    DefaultAllocatorDeinit(&alloc);
    return match;
}

// ---------------------------------------------------------------------------
// Mutation-hardening tests for the DNS response / record parsing path.
//
// Each test feeds an exact wire-format byte buffer to DnsParseResponse and
// pins a parsed count / id / flag / type / class / ttl / address / rdata so
// that operator / value / shift mutations in Dns.c change an observable.
// ---------------------------------------------------------------------------

// A 12-byte header with all counts zero is the smallest VALID response.
// Pins `len < 12` at the boundary: `<=` would reject a 12-byte buffer.
static bool test_dn2_header_only_min_length(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);

    // id=0xABCD flags=0x8180 qd=0 an=0 ns=0 ar=0 -- exactly 12 bytes.
    static const u8 wire[] = {0xab, 0xcd, 0x81, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

    DnsResponse resp  = {0};
    bool        ok    = DnsParseResponse(&resp, wire, sizeof(wire), a);
    bool        match = ok && resp.id == 0xABCD && VecLen(&resp.answers) == 0 && VecLen(&resp.authority) == 0 &&
                 VecLen(&resp.additional) == 0;

    DnsResponseDeinit(&resp);
    DefaultAllocatorDeinit(&alloc);
    return match;
}

// An 11-byte buffer is shorter than the header and must be rejected. Pins
// the other side of the `len < 12` comparison.
static bool test_dn2_header_too_short_rejected(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);

    static const u8 wire[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

    DnsResponse resp = {0};
    bool        ok   = DnsParseResponse(&resp, wire, sizeof(wire), a);

    DnsResponseDeinit(&resp);
    DefaultAllocatorDeinit(&alloc);
    return ok == false;
}

// Pin every header flag bit independently. flags=0x8580 sets:
//   0x8000 -> is_response = true
//   0x0400 -> authoritative = true   (0x0500 = 0x0400|0x0100)
//   0x0200 -> truncated stays false  (bit clear)
//   0x0100 -> recursion_desired = true
//   0x0080 -> recursion_avail = true
//   0x000F -> rcode field
// Using a flag word where the AA and TC bits differ kills the
// `& -> |` mutations (an OR with the wrong mask would flip the result of a
// cleared bit) and the `!= 0 -> == 0` mutations on the AA / TC extractions.
static bool test_dn2_flags_decoded(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);

    // flags = 0x8580 = 1000 0101 1000 0000
    //   QR=1 AA=1 TC=0 RD=1 RA=1 rcode=0
    static const u8 wire[] = {0x11, 0x22, 0x85, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

    DnsResponse resp  = {0};
    bool        ok    = DnsParseResponse(&resp, wire, sizeof(wire), a);
    bool        match = ok && resp.id == 0x1122 && resp.is_response == true && resp.authoritative == true &&
                 resp.truncated == false && resp.recursion_desired == true && resp.recursion_avail == true &&
                 resp.rcode == DNS_RCODE_NOERROR;

    DnsResponseDeinit(&resp);
    DefaultAllocatorDeinit(&alloc);
    return match;
}

// Complementary flag word: every bit flipped relative to the previous test,
// so a stuck-constant assignment (`= true`) on any flag is caught from the
// false side, and the AA/TC extractions are pinned with AA=0 TC=1.
//   flags = 0x0200 = 0000 0010 0000 0000
//   QR=0 AA=0 TC=1 RD=0 RA=0 rcode=0
static bool test_dn2_flags_all_clear_but_tc(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);

    static const u8 wire[] = {0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

    DnsResponse resp  = {0};
    bool        ok    = DnsParseResponse(&resp, wire, sizeof(wire), a);
    bool        match = ok && resp.is_response == false && resp.authoritative == false && resp.truncated == true &&
                 resp.recursion_desired == false && resp.recursion_avail == false;

    DnsResponseDeinit(&resp);
    DefaultAllocatorDeinit(&alloc);
    return match;
}

// rcode lives in the low nibble. flags=0x8183 -> rcode = 3 (NXDOMAIN).
// Pins the `& 0x000F` extraction (an `& -> |` mutation here would set
// non-rcode bits and change the cast value).
static bool test_dn2_rcode_low_nibble(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);

    static const u8 wire[] = {0x00, 0x01, 0x81, 0x83, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

    DnsResponse resp  = {0};
    bool        ok    = DnsParseResponse(&resp, wire, sizeof(wire), a);
    bool        match = ok && resp.rcode == DNS_RCODE_NXDOMAIN;

    DnsResponseDeinit(&resp);
    DefaultAllocatorDeinit(&alloc);
    return match;
}

// A single A record (last thing in the buffer) with a non-zero byte placed
// immediately after the 4 IP bytes. Pins:
//   - type / rclass / ttl exact values (192 assign-const on rclass,
//     ttl big-endian assembly)
//   - the 4 IPv4 bytes
//   - ipv6[0] == 0: an `i < 4 -> i <= 4` mutation in the A-copy loop writes
//     ipv4[4], which aliases ipv6[0] in the struct, making it non-zero.
//   - rdata raw bytes (201 BufPushBytes stash)
static bool test_dn2_a_record_fields(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);

    static const u8 wire[] = {
        // header: id=0x2233 flags=0x8180 qd=0 an=2 ns=0 ar=0
        0x22,
        0x33,
        0x81,
        0x80,
        0x00,
        0x00,
        0x00,
        0x02,
        0x00,
        0x00,
        0x00,
        0x00,
        // answer: name = "a" (1-byte label) + root
        0x01,
        'a',
        0x00,
        // type=A(1) class=IN(1) ttl=0x01020304 rdlen=4
        0x00,
        0x01,
        0x00,
        0x01,
        0x01,
        0x02,
        0x03,
        0x04,
        0x00,
        0x04,
        // rdata: 8.8.4.4
        0x08,
        0x08,
        0x04,
        0x04,
        // trailing A record: name "b" (the 0x01 label-len is the non-zero,
        // in-bounds byte immediately after the first record's 4 IP bytes),
        // A, 1.1.1.1. An `i < 4 -> i <= 4` mutation reads this 0x01 into
        // ipv4[4], aliasing ipv6[0].
        0x01,
        'b',
        0x00,
        0x00,
        0x01,
        0x00,
        0x01,
        0x00,
        0x00,
        0x00,
        0x01,
        0x00,
        0x04,
        0x01,
        0x01,
        0x01,
        0x01
    };

    DnsResponse resp  = {0};
    bool        ok    = DnsParseResponse(&resp, wire, sizeof(wire), a);
    bool        match = ok && VecLen(&resp.answers) == 2;
    if (match) {
        DnsRecord *r = VecPtrAt(&resp.answers, 0);
        match        = r->type == DNS_TYPE_A && r->rclass == 1 && r->ttl == 0x01020304u && r->ipv4[0] == 8 &&
                r->ipv4[1] == 8 && r->ipv4[2] == 4 && r->ipv4[3] == 4 && r->ipv6[0] == 0 &&
                ZstrCompare(StrBegin(&r->name), "a") == 0;
        // rdata raw stash must equal the 4 IP bytes.
        match = match && BufLength(&r->rdata) == 4 && BufData(&r->rdata)[0] == 8 && BufData(&r->rdata)[1] == 8 &&
                BufData(&r->rdata)[2] == 4 && BufData(&r->rdata)[3] == 4;
    }

    DnsResponseDeinit(&resp);
    DefaultAllocatorDeinit(&alloc);
    return match;
}

// Distinct rclass value (not 1) pins the `rec->rclass = class_bits`
// assignment against a stuck-constant mutation. class=0x00FE.
static bool test_dn2_record_class_preserved(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);

    static const u8 wire[] = {0x00, 0x00, 0x81, 0x80, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00,
                              0x00, // root name
                              0x00, 0x01, 0x00, 0xfe, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x01, 0x02, 0x03, 0x04};

    DnsResponse resp  = {0};
    bool        ok    = DnsParseResponse(&resp, wire, sizeof(wire), a);
    bool        match = ok && VecLen(&resp.answers) == 1;
    if (match) {
        DnsRecord *r = VecPtrAt(&resp.answers, 0);
        match        = r->rclass == 0x00FE;
    }

    DnsResponseDeinit(&resp);
    DefaultAllocatorDeinit(&alloc);
    return match;
}

// TTL is assembled big-endian from 4 bytes. A TTL whose every byte is
// distinct and which sets the top bit (0x80112233) pins the shift / or
// assembly: any byte-order or shift mutation changes the u32 value.
static bool test_dn2_ttl_big_endian(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);

    static const u8 wire[] = {0x00, 0x00, 0x81, 0x80, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00,
                              0x00,                                           // root name
                              0x00, 0x01, 0x00, 0x01, 0x80, 0x11, 0x22, 0x33, // ttl = 0x80112233
                              0x00, 0x04, 0x7f, 0x00, 0x00, 0x01};

    DnsResponse resp  = {0};
    bool        ok    = DnsParseResponse(&resp, wire, sizeof(wire), a);
    bool        match = ok && VecLen(&resp.answers) == 1;
    if (match) {
        DnsRecord *r = VecPtrAt(&resp.answers, 0);
        match        = r->ttl == 0x80112233u;
    }

    DnsResponseDeinit(&resp);
    DefaultAllocatorDeinit(&alloc);
    return match;
}

// An AAAA record with all 16 bytes distinct, pinning the IPv6 copy loop and
// rdata stash. A second A record follows it so that the byte immediately
// after the 16 IPv6 bytes is an in-bounds, NON-ZERO byte (0x01, the next
// record's name length). An `i < 16 -> i <= 16` mutation in the AAAA copy
// loop writes rec->ipv6[16], which aliases the low byte of `target.length`
// in the struct -- so the AAAA record's `target` would become non-empty.
// We pin VecLen(&target) == 0 to catch that, plus every IPv6 byte and the
// rdata stash.
static bool test_dn2_aaaa_record_fields(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);

    static const u8 wire[] = {
        0x00,
        0x00,
        0x81,
        0x80,
        0x00,
        0x00,
        0x00,
        0x02,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00, // root name
        0x00,
        0x1c,
        0x00,
        0x01,
        0x00,
        0x00,
        0x00,
        0x0a, // type=AAAA class=IN ttl=10
        0x00,
        0x10, // rdlen=16
        0x20,
        0x01,
        0x0d,
        0xb8,
        0x00,
        0x11,
        0x22,
        0x33,
        0x44,
        0x55,
        0x66,
        0x77,
        0x88,
        0x99,
        0xaa,
        0xbb,
        // trailing A record: name "z" (label-len 0x01 is the non-zero byte
        // sitting right after the IPv6 rdata), A, 1.1.1.1
        0x01,
        'z',
        0x00,
        0x00,
        0x01,
        0x00,
        0x01,
        0x00,
        0x00,
        0x00,
        0x01,
        0x00,
        0x04,
        0x01,
        0x01,
        0x01,
        0x01
    };

    DnsResponse resp  = {0};
    bool        ok    = DnsParseResponse(&resp, wire, sizeof(wire), a);
    bool        match = ok && VecLen(&resp.answers) == 2;
    if (match) {
        DnsRecord *r = VecPtrAt(&resp.answers, 0);
        match        = r->type == DNS_TYPE_AAAA && r->ttl == 10 && VecLen(&r->target) == 0;
        static const u8 ip6[] =
            {0x20, 0x01, 0x0d, 0xb8, 0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xaa, 0xbb};
        for (u64 i = 0; match && i < 16; ++i) {
            if (r->ipv6[i] != ip6[i]) {
                match = false;
            }
        }
        // rdata stash must hold all 16 bytes.
        match = match && BufLength(&r->rdata) == 16;
        for (u64 i = 0; match && i < 16; ++i) {
            if (BufData(&r->rdata)[i] != ip6[i]) {
                match = false;
            }
        }
    }

    DnsResponseDeinit(&resp);
    DefaultAllocatorDeinit(&alloc);
    return match;
}

// Two A records back-to-back with different addresses. Pins
// decode_record_list boundary advance: a wrong offset after record 0 would
// corrupt record 1's name / type / address.
static bool test_dn2_record_list_two_records(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);

    static const u8 wire[] = {
        0x00,
        0x00,
        0x81,
        0x80,
        0x00,
        0x00,
        0x00,
        0x02,
        0x00,
        0x00,
        0x00,
        0x00,
        // record 0: name "x", A, ttl=1, 1.2.3.4
        0x01,
        'x',
        0x00,
        0x00,
        0x01,
        0x00,
        0x01,
        0x00,
        0x00,
        0x00,
        0x01,
        0x00,
        0x04,
        0x01,
        0x02,
        0x03,
        0x04,
        // record 1: name "y", A, ttl=2, 5.6.7.8
        0x01,
        'y',
        0x00,
        0x00,
        0x01,
        0x00,
        0x01,
        0x00,
        0x00,
        0x00,
        0x02,
        0x00,
        0x04,
        0x05,
        0x06,
        0x07,
        0x08
    };

    DnsResponse resp  = {0};
    bool        ok    = DnsParseResponse(&resp, wire, sizeof(wire), a);
    bool        match = ok && VecLen(&resp.answers) == 2;
    if (match) {
        DnsRecord *r0 = VecPtrAt(&resp.answers, 0);
        DnsRecord *r1 = VecPtrAt(&resp.answers, 1);
        match = ZstrCompare(StrBegin(&r0->name), "x") == 0 && r0->ttl == 1 && r0->ipv4[0] == 1 && r0->ipv4[3] == 4 &&
                ZstrCompare(StrBegin(&r1->name), "y") == 0 && r1->ttl == 2 && r1->ipv4[0] == 5 && r1->ipv4[3] == 8;
    }

    DnsResponseDeinit(&resp);
    DefaultAllocatorDeinit(&alloc);
    return match;
}

// Records distributed across all three sections: an=1, ns=1, ar=1. Pins the
// three decode_record_list calls in DnsParseResponse -- a scalar-replaced
// call (returns truthy without parsing) would leave that section empty, and
// counts/contents pin which counter drives which loop.
static bool test_dn2_three_sections(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);

    static const u8 wire[] = {
        // header: an=1 ns=1 ar=1
        0x00,
        0x00,
        0x81,
        0x80,
        0x00,
        0x00,
        0x00,
        0x01,
        0x00,
        0x01,
        0x00,
        0x01,
        // answer: name "an", A, 11.11.11.11
        0x02,
        'a',
        'n',
        0x00,
        0x00,
        0x01,
        0x00,
        0x01,
        0x00,
        0x00,
        0x00,
        0x01,
        0x00,
        0x04,
        0x0b,
        0x0b,
        0x0b,
        0x0b,
        // authority: name "ns", A, 22.22.22.22
        0x02,
        'n',
        's',
        0x00,
        0x00,
        0x01,
        0x00,
        0x01,
        0x00,
        0x00,
        0x00,
        0x02,
        0x00,
        0x04,
        0x16,
        0x16,
        0x16,
        0x16,
        // additional: name "ad", A, 33.33.33.33
        0x02,
        'a',
        'd',
        0x00,
        0x00,
        0x01,
        0x00,
        0x01,
        0x00,
        0x00,
        0x00,
        0x03,
        0x00,
        0x04,
        0x21,
        0x21,
        0x21,
        0x21
    };

    DnsResponse resp = {0};
    bool        ok   = DnsParseResponse(&resp, wire, sizeof(wire), a);
    bool match = ok && VecLen(&resp.answers) == 1 && VecLen(&resp.authority) == 1 && VecLen(&resp.additional) == 1;
    if (match) {
        DnsRecord *an = VecPtrAt(&resp.answers, 0);
        DnsRecord *ns = VecPtrAt(&resp.authority, 0);
        DnsRecord *ar = VecPtrAt(&resp.additional, 0);
        match         = ZstrCompare(StrBegin(&an->name), "an") == 0 && an->ipv4[0] == 11 &&
                ZstrCompare(StrBegin(&ns->name), "ns") == 0 && ns->ipv4[0] == 22 &&
                ZstrCompare(StrBegin(&ar->name), "ad") == 0 && ar->ipv4[0] == 33;
    }

    DnsResponseDeinit(&resp);
    DefaultAllocatorDeinit(&alloc);
    return match;
}

// A response with a question to skip (qd=1) followed by one answer. Pins the
// question-skip loop's `++i`: with `--i` the loop never terminates / never
// reaches the answer, so the answer count would be wrong (or it hangs). With
// the correct increment the single question is consumed and the A answer is
// parsed.
static bool test_dn2_question_skipped_then_answer(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);

    static const u8 wire[] = {
        // header: qd=1 an=1
        0x00,
        0x00,
        0x81,
        0x80,
        0x00,
        0x01,
        0x00,
        0x01,
        0x00,
        0x00,
        0x00,
        0x00,
        // question: name "q" type=A class=IN
        0x01,
        'q',
        0x00,
        0x00,
        0x01,
        0x00,
        0x01,
        // answer: name "q" via pointer to offset 12, A, 9.9.9.9
        0xc0,
        0x0c,
        0x00,
        0x01,
        0x00,
        0x01,
        0x00,
        0x00,
        0x00,
        0x05,
        0x00,
        0x04,
        0x09,
        0x09,
        0x09,
        0x09
    };

    DnsResponse resp  = {0};
    bool        ok    = DnsParseResponse(&resp, wire, sizeof(wire), a);
    bool        match = ok && VecLen(&resp.answers) == 1;
    if (match) {
        DnsRecord *r = VecPtrAt(&resp.answers, 0);
        match        = ZstrCompare(StrBegin(&r->name), "q") == 0 && r->ipv4[0] == 9 && r->ttl == 5;
    }

    DnsResponseDeinit(&resp);
    DefaultAllocatorDeinit(&alloc);
    return match;
}

// CNAME target decode (type dispatch) pins the target Str assembly.
static bool test_dn2_cname_target(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);

    static const u8 wire[] = {
        0x00,
        0x00,
        0x81,
        0x80,
        0x00,
        0x00,
        0x00,
        0x01,
        0x00,
        0x00,
        0x00,
        0x00,
        // answer: name "c" (offset 12), CNAME(5), ttl=7
        0x01,
        'c',
        0x00,
        0x00,
        0x05,
        0x00,
        0x01,
        0x00,
        0x00,
        0x00,
        0x07,
        // rdlen=4, rdata: label "to" + root
        0x00,
        0x04,
        0x02,
        't',
        'o',
        0x00
    };

    DnsResponse resp  = {0};
    bool        ok    = DnsParseResponse(&resp, wire, sizeof(wire), a);
    bool        match = ok && VecLen(&resp.answers) == 1;
    if (match) {
        DnsRecord *r = VecPtrAt(&resp.answers, 0);
        match        = r->type == DNS_TYPE_CNAME && ZstrCompare(StrBegin(&r->target), "to") == 0;
    }

    DnsResponseDeinit(&resp);
    DefaultAllocatorDeinit(&alloc);
    return match;
}

// rdlength that overruns the buffer must be rejected. The answer claims
// rdlen=8 but only 2 rdata bytes follow. Pins the bounds check
// `IterIndex(it) + rdlength > IterLength(it)`: a `+ -> -` mutation would
// compute idx - rdlength (a small in-range value) and wrongly accept,
// then over-read. Real code rejects -> ok == false.
static bool test_dn2_rdlength_overrun_rejected(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);

    static const u8 wire[] = {
        0x00, 0x00, 0x81, 0x80, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00,
        0x00,                                           // root name
        0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, // type=A class=IN ttl=0
        0x00, 0x08,                                     // rdlen=8 (claims 8 bytes)
        0xaa, 0xbb                                      // only 2 bytes present
    };

    DnsResponse resp = {0};
    bool        ok   = DnsParseResponse(&resp, wire, sizeof(wire), a);

    DnsResponseDeinit(&resp);
    DefaultAllocatorDeinit(&alloc);
    return ok == false;
}

// An A record claiming rdlen != 4 must be rejected (type-A length check).
static bool test_dn2_a_record_wrong_rdlen_rejected(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);

    static const u8 wire[] = {
        0x00, 0x00, 0x81, 0x80, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, // rdlen=6 for an A record (invalid)
        0x01, 0x02, 0x03, 0x04, 0x05, 0x06
    };

    DnsResponse resp = {0};
    bool        ok   = DnsParseResponse(&resp, wire, sizeof(wire), a);

    DnsResponseDeinit(&resp);
    DefaultAllocatorDeinit(&alloc);
    return ok == false;
}

// ---------------------------------------------------------------------------
// Site 1 -- encode_qname empty-internal-label guard (39:45 cxx_eq_to_ne)
//   `if (StrIterRemainingLength(&si) == 0) break;` -> `!= 0`
//
// Trace: a trailing dot is consumed at the `c == '.'` step (lines 57-59)
// BEFORE the loop re-enters, so the seg_len==0 branch is only ever reached
// while the iterator is sitting ON a '.' (an empty leading/middle label).
// At that point StrIterRemainingLength is >= 1 (the dot itself remains), so
// the real `== 0` test is FALSE -> falls through to `return false` (an empty
// internal label is invalid). The mutant `!= 0` is TRUE -> BREAKS, silently
// truncating the name at the empty label and emitting a (wrong) short query
// that returns success.
//
// Input "a..b": after 'a', the first '.' is consumed; the loop re-enters
// sitting on the second '.', seg_len==0, remaining = ".b" (>0). Real rejects
// (returns false); mutant breaks and encodes just "a" -> success. Asserting
// rejection (!ok) kills the mutant.
// ---------------------------------------------------------------------------
static bool test_df_encode_empty_internal_label_rejected(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);

    DnsWireBuf buf = VecInitT(buf, a);
    bool       ok  = DnsBuildQuery(&buf, 0x1234, "a..b", DNS_TYPE_A);

    bool match = !ok; // empty internal label must be rejected.

    VecDeinit(&buf);
    DefaultAllocatorDeinit(&alloc);
    return match;
}

// ---------------------------------------------------------------------------
// Site 3 -- decode_name name-length cap boundary (155:22 cxx_add_to_sub)
//   `if (name_len + 1 + label_len > 253)` -> `name_len - 1 + label_len`
// The +1 accounts for the dot separator about to be emitted before this
// label. We construct a name that lands EXACTLY in the 2-byte window the
// shift opens up: a decoded dotted length where the real `+1` form rejects
// (> 253) but the mutant `-1` form accepts (<= 253).
//
// Choose the final label so that, entering it, name_len = N (length of all
// prior labels + their separating dots) and label_len = L. Real check:
//   N + 1 + L > 253. Mutant: N - 1 + L > 253.
// We want REAL to reject and MUTANT to accept on the SAME final label, i.e.
//   N + 1 + L > 253  AND  N - 1 + L <= 253
//   => 252 < N + L <= 254  (taking the strict/loose bounds together):
//      N + 1 + L >= 254  -> N + L >= 253
//      N - 1 + L <= 253  -> N + L <= 254
//   Pick N + L = 254 (real: 255 > 253 reject; mutant: 253 <= 253 accept) OR
//   N + L = 253 (real: 254 > 253 reject; mutant: 252 <= 253 accept).
//
// Prior labels: 63,63,63 -> after the third, name_len = 63+1+63+1+63 = 191
// (dots between them counted). Entering a 4th label, the code first checks
// the cap with the CURRENT name_len (191) and the new label_len. We need
// 191 + label_len in the window. Use label_len = 62: real 191+1+62 = 254 >
// 253 -> reject; mutant 191-1+62 = 252 <= 253 -> accept (then would try to
// append). Real code therefore FAILS the parse; mutant SUCCEEDS.
//
// We feed this name as the QUESTION; the real parser rejects the response,
// the mutant parser accepts it. Assert rejection.
static bool test_df_decode_namelen_cap_rejects(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);

    // labels 63,63,63,62 -> wire 64+64+64+63 = 255, + terminator = 256.
    static const u8 lens[] = {63, 63, 63, 62};
    u8              wire[12 + 256 + 4];
    u64             p       = 0;
    static const u8 hdr[12] = {0x00, 0x09, 0x81, 0x80, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    for (u64 i = 0; i < 12; ++i) {
        wire[p++] = hdr[i];
    }
    for (u64 i = 0; i < 4; ++i) {
        wire[p++] = lens[i];
        for (u64 j = 0; j < lens[i]; ++j) {
            wire[p++] = 'a';
        }
    }
    wire[p++] = 0x00; // terminator
    wire[p++] = 0x00; // qtype
    wire[p++] = 0x01;
    wire[p++] = 0x00; // qclass
    wire[p++] = 0x01;

    DnsResponse resp  = {0};
    bool        ok    = DnsParseResponse(&resp, wire, p, a);
    bool        match = !ok; // real code rejects: 191 + 1 + 62 = 254 > 253.

    DnsResponseDeinit(&resp);
    DefaultAllocatorDeinit(&alloc);
    return match;
}

// Companion accept at the boundary just BELOW: labels 63,63,63,61 ->
// entering the 4th label name_len = 191, label_len = 61: real 191+1+61 =
// 253, NOT > 253 -> accept. This decoded dotted length is exactly 253. It
// guards against a `> 253` -> `>= 253` style off-by-one AND confirms the
// rejecting case above is genuinely at the boundary (not a buffer issue).
static bool test_df_decode_namelen_253_accepts(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);

    static const u8 lens[] = {63, 63, 63, 61};
    u8              wire[12 + 255 + 4];
    u64             p       = 0;
    static const u8 hdr[12] = {0x00, 0x0a, 0x81, 0x80, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    for (u64 i = 0; i < 12; ++i) {
        wire[p++] = hdr[i];
    }
    for (u64 i = 0; i < 4; ++i) {
        wire[p++] = lens[i];
        for (u64 j = 0; j < lens[i]; ++j) {
            wire[p++] = 'a';
        }
    }
    wire[p++] = 0x00; // terminator
    wire[p++] = 0x00; // qtype
    wire[p++] = 0x01;
    wire[p++] = 0x00; // qclass
    wire[p++] = 0x01;

    DnsResponse resp  = {0};
    bool        ok    = DnsParseResponse(&resp, wire, p, a);
    bool        match = ok && resp.id == 0x000a;

    DnsResponseDeinit(&resp);
    DefaultAllocatorDeinit(&alloc);
    return match;
}

// ---------------------------------------------------------------------------
// Site 4 -- decode_record rdata-bounds check (195:23 cxx_add_to_sub)
//   `if (IterIndex(it) + (u64)rdlength > IterLength(it)) return false;`
//     -> `IterIndex(it) - rdlength > IterLength(it)`
// Truncated record: the answer's rdlength claims MORE bytes than actually
// remain in the buffer. Real code computes idx + rdlength > len -> rejects.
// The mutant computes idx - rdlength (unsigned): to make the mutant ACCEPT
// (rather than underflow to a huge reject) we need idx >= rdlength, so the
// mutant's `idx - rdlength` is a small value <= len. We arrange BOTH
// idx >= rdlength AND idx + rdlength > len (truncation):
//   idx = 90, rdlength = 40, len = 94 -> real 130 > 94 reject;
//   mutant 90 - 40 = 50, not > 94 -> ACCEPT (then over-reads garbage).
//
// We use a TXT record (type 16) so the type-specific switch hits the
// `default` branch -- no rdlength-shape check (unlike A/AAAA) intervenes to
// reject first. With the bounds check defeated, the mutant returns a parsed
// response with one answer; the real code returns failure. Asserting !ok
// kills the mutant.
static bool test_df_decode_truncated_rdata_rejected(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);

    u8              wire[256];
    u64             p       = 0;
    static const u8 hdr[12] = {0x00, 0x77, 0x81, 0x80, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00};
    for (u64 i = 0; i < 12; ++i) {
        wire[p++] = hdr[i];
    }
    // Question: one 60-byte label (offsets 12..72), terminator (73), then
    // qtype/qclass (74..77).
    wire[p++] = 60;
    for (u64 i = 0; i < 60; ++i) {
        wire[p++] = 'a';
    }
    wire[p++] = 0x00; // name terminator
    wire[p++] = 0x00; // qtype
    wire[p++] = 0x01;
    wire[p++] = 0x00; // qclass
    wire[p++] = 0x01; // p now 78

    // Answer: name pointer back to offset 12 (the question name).
    wire[p++] = 0xc0;
    wire[p++] = 0x0c;
    wire[p++] = 0x00; // type = TXT (16)
    wire[p++] = 0x10;
    wire[p++] = 0x00; // class = IN
    wire[p++] = 0x01;
    wire[p++] = 0x00; // ttl
    wire[p++] = 0x00;
    wire[p++] = 0x00;
    wire[p++] = 0x3c;
    wire[p++] = 0x00; // rdlength = 40 (claims 40 bytes of rdata)
    wire[p++] = 0x28; // p now 90 -> rdata-start index (idx) = 90
    // Only 4 actual rdata bytes follow, then the buffer ends -> truncated.
    wire[p++] = 0x01;
    wire[p++] = 0x02;
    wire[p++] = 0x03;
    wire[p++] = 0x04; // p now 94 = len
    // idx = 90, rdlength = 40, len = 94.
    // Real:   90 + 40 = 130 > 94 -> reject.
    // Mutant: 90 - 40 = 50,  not > 94 -> accept (over-reads 40 garbage bytes).

    DnsResponse resp  = {0};
    bool        ok    = DnsParseResponse(&resp, wire, p, a);
    bool        match = !ok; // real code rejects the truncated rdata.

    DnsResponseDeinit(&resp);
    DefaultAllocatorDeinit(&alloc);
    return match;
}

// Companion: an UNtruncated record at the boundary -- rdlength exactly fits
// the remaining buffer -- must be ACCEPTED. Pins `>` (not `>=`) on the
// bounds check so the exact-fit case isn't wrongly rejected, and confirms
// the rejection above is about truncation, not the record shape.
static bool test_df_decode_exact_rdata_accepted(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *a     = ALLOCATOR_OF(&alloc);

    static const u8 wire[] = {
        // header: response qd=1 an=1
        0x00,
        0x78,
        0x81,
        0x80,
        0x00,
        0x01,
        0x00,
        0x01,
        0x00,
        0x00,
        0x00,
        0x00,
        // question "a" type=A class=IN (name at offset 12)
        0x01,
        'a',
        0x00,
        0x00,
        0x01,
        0x00,
        0x01,
        // answer: name pointer -> 12, type=A class=IN ttl=5 rdlen=4 rdata=1.2.3.4
        0xc0,
        0x0c,
        0x00,
        0x01,
        0x00,
        0x01,
        0x00,
        0x00,
        0x00,
        0x05,
        0x00,
        0x04,
        0x01,
        0x02,
        0x03,
        0x04
    };

    DnsResponse resp  = {0};
    bool        ok    = DnsParseResponse(&resp, wire, sizeof(wire), a);
    bool        match = ok && VecLen(&resp.answers) == 1;
    if (match) {
        DnsRecord *r = VecPtrAt(&resp.answers, 0);
        match        = r->type == DNS_TYPE_A && r->ipv4[0] == 1 && r->ipv4[3] == 4;
    }

    DnsResponseDeinit(&resp);
    DefaultAllocatorDeinit(&alloc);
    return match;
}

int main(void) {
    WriteFmt("[INFO] Starting Dns tests\n\n");

    TestFunction tests[] = {
        test_dns_build_query_basic,
        test_dns_build_query_trailing_dot,
        test_dns_parse_response_a_and_aaaa,
        test_dns_parse_response_nxdomain,
        test_dns_parse_response_cname,
        test_dn1_encode_exact_bytes,
        test_dn1_encode_root,
        test_dn1_encode_label_63_accepted,
        test_dn1_encode_trailing_dot_accepted,
        test_dn1_encode_total_init_220,
        test_dn1_encode_total_254_accepted,
        test_dn1_build_query_str_path,
        test_dn1_decode_compressed_answer,
        test_dn1_decode_namelen_no_underflow,
        test_dn1_decode_namelen_253_accepted,
        test_dn1_decode_bigoffset_pointer,
        test_dn1_decode_backward_pointer_ok,
        test_dn1_decode_label_overrun_rejected,
        test_dn1_decode_forward_pointer_rejected,
        test_dn2_header_only_min_length,
        test_dn2_header_too_short_rejected,
        test_dn2_flags_decoded,
        test_dn2_flags_all_clear_but_tc,
        test_dn2_rcode_low_nibble,
        test_dn2_a_record_fields,
        test_dn2_record_class_preserved,
        test_dn2_ttl_big_endian,
        test_dn2_aaaa_record_fields,
        test_dn2_record_list_two_records,
        test_dn2_three_sections,
        test_dn2_question_skipped_then_answer,
        test_dn2_cname_target,
        test_dn2_rdlength_overrun_rejected,
        test_dn2_a_record_wrong_rdlen_rejected,
        test_df_encode_empty_internal_label_rejected,
        test_df_decode_namelen_cap_rejects,
        test_df_decode_namelen_253_accepts,
        test_df_decode_truncated_rdata_rejected,
        test_df_decode_exact_rdata_accepted,
    };

    return run_test_suite(tests, sizeof(tests) / sizeof(tests[0]), NULL, 0, "Dns");
}
