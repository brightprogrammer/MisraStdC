#include <Misra/Parsers/Dns.h>
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
    bool            match      = ok && buf.length == sizeof(expected);
    for (u64 i = 0; match && i < sizeof(expected); ++i) {
        if (buf.data[i] != expected[i]) {
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

    bool match = ok && no_dot.length == w_dot.length;
    for (u64 i = 0; match && i < no_dot.length; ++i) {
        if (no_dot.data[i] != w_dot.data[i]) {
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
                 resp.rcode == DNS_RCODE_NOERROR && resp.answers.length == 2;

    if (match) {
        DnsRecord *r0 = &resp.answers.data[0];
        match         = r0->type == DNS_TYPE_A && r0->ttl == 300 && r0->ipv4[0] == 93 && r0->ipv4[1] == 184 &&
                r0->ipv4[2] == 216 && r0->ipv4[3] == 34 && r0->name.length > 0 &&
                ZstrCompare(r0->name.data, "example.com") == 0;
    }
    if (match) {
        DnsRecord *r1 = &resp.answers.data[1];
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
    bool        match = ok && resp.rcode == DNS_RCODE_NXDOMAIN && resp.answers.length == 0;

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
    bool        match = ok && resp.answers.length == 1;
    if (match) {
        DnsRecord *r = &resp.answers.data[0];
        match = r->type == DNS_TYPE_CNAME && r->target.length > 0 && ZstrCompare(r->target.data, "example.com") == 0;
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
    };

    return run_test_suite(tests, sizeof(tests) / sizeof(tests[0]), NULL, 0, "Dns");
}
