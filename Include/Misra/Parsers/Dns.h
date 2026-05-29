/// file      : parsers/dns.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// DNS wire-format encoder + decoder (RFC 1035). This module is pure
/// data manipulation -- it does not open sockets or talk to nameservers.
/// `Sys/Dns` is the I/O layer that uses these helpers.
///
/// The encoder builds a single-question query (recursion desired) into
/// a `Vec(u8)`. The decoder walks an incoming wire buffer into a typed
/// `DnsResponse` structure with parsed answer / authority / additional
/// record lists. Label compression pointers (RFC 1035 section 4.1.4)
/// are followed when decoding names.
///
/// v1 scope: `A`, `AAAA`, and `CNAME` records are fully decoded; other
/// types land in the record with `rdata` populated but `type`-specific
/// fields left zero. Callers needing MX / NS / TXT / SOA can grow this
/// without changing the wire-level entry points.

#ifndef MISRA_PARSERS_DNS_H
#define MISRA_PARSERS_DNS_H

#include <Misra/Std/Allocator.h>
#include <Misra/Std/Container/Str.h>
#include <Misra/Std/Container/Buf.h>
#include <Misra/Std/Container/Vec.h>
#include <Misra/Types.h>

#ifdef __cplusplus
extern "C" {
#endif

    // Wire byte buffer used by both the encoder (caller passes it in
    // for query construction) and the decoder's raw-rdata storage.
    typedef Buf DnsWireBuf;

    typedef enum DnsType {
        DNS_TYPE_A     = 1,
        DNS_TYPE_NS    = 2,
        DNS_TYPE_CNAME = 5,
        DNS_TYPE_SOA   = 6,
        DNS_TYPE_PTR   = 12,
        DNS_TYPE_MX    = 15,
        DNS_TYPE_TXT   = 16,
        DNS_TYPE_AAAA  = 28,
    } DnsType;

    typedef enum DnsRcode {
        DNS_RCODE_NOERROR  = 0,
        DNS_RCODE_FORMERR  = 1,
        DNS_RCODE_SERVFAIL = 2,
        DNS_RCODE_NXDOMAIN = 3,
        DNS_RCODE_NOTIMP   = 4,
        DNS_RCODE_REFUSED  = 5,
    } DnsRcode;

    ///
    /// One decoded resource record.
    ///
    /// FIELDS:
    /// - name    : decoded label string (e.g. "example.com.")
    /// - type    : record type
    /// - rclass  : record class (typically 1 = IN)
    /// - ttl     : seconds the record may be cached
    /// - ipv4    : valid when type == DNS_TYPE_A; otherwise zero
    /// - ipv6    : valid when type == DNS_TYPE_AAAA; otherwise zero
    /// - target  : valid when type is CNAME / NS / PTR; otherwise empty
    /// - rdata   : raw rdata bytes; always populated regardless of type
    ///
    typedef struct DnsRecord {
        Str        name;
        DnsType    type;
        u16        rclass;
        u32        ttl;
        u8         ipv4[4];
        u8         ipv6[16];
        Str        target;
        DnsWireBuf rdata;
    } DnsRecord;

    typedef Vec(DnsRecord) DnsRecords;

    ///
    /// Parsed DNS message. `id` echoes the client-supplied transaction
    /// id from the matching query so the resolver can pair responses.
    ///
    typedef struct DnsResponse {
        u16        id;
        bool       is_response;
        bool       authoritative;
        bool       truncated;
        bool       recursion_desired;
        bool       recursion_avail;
        DnsRcode   rcode;
        DnsRecords answers;
        DnsRecords authority;
        DnsRecords additional;
    } DnsResponse;

    ///
    /// Build a single-question DNS query (recursion desired) into `out`.
    ///
    /// out[out]  : Caller-managed `Vec(u8)`. Buffer is cleared and
    ///             repopulated.
    /// id[in]    : Caller-supplied transaction id, echoed back by the
    ///             nameserver. Pick randomly for cache-poisoning
    ///             resistance.
    /// name[in]  : Hostname to encode, e.g. "example.com". Prefer
    ///             `Str *`; `Zstr` accepted. Trailing dot is optional.
    ///             Each label must be 1..63 bytes; total wire length
    ///             must be < 255.
    /// type[in]  : Question type (typically `DNS_TYPE_A` or
    ///             `DNS_TYPE_AAAA`).
    ///
    /// SUCCESS : Returns true. `out` contains a valid wire query.
    /// FAILURE : Returns false on invalid arguments, label-length
    ///           violations, or Vec growth failure. `out` is left
    ///           with whatever was written.
    ///
    bool dns_build_query_zstr(DnsWireBuf *out, u16 id, Zstr name, DnsType type);
    bool dns_build_query_str(DnsWireBuf *out, u16 id, const Str *name, DnsType type);
#define DnsBuildQuery(out, id, name, type)                                                                             \
    _Generic((name), Str *: dns_build_query_str, Zstr: dns_build_query_zstr, char *: dns_build_query_zstr)(            \
        (out),                                                                                                         \
        (id),                                                                                                          \
        (name),                                                                                                        \
        (type)                                                                                                         \
    )

    ///
    /// Parse a wire-format response into `out`. The caller is
    /// responsible for `DnsResponseDeinit(out)` afterwards.
    ///
    /// out[out] : Receives parsed structure. Strings + Vecs use `alloc`.
    /// buf[in]  : Wire bytes received from the nameserver.
    /// len[in]  : Number of bytes in `buf`.
    /// alloc[in]: Allocator used for all strings + record Vecs.
    ///
    /// SUCCESS : Returns true. `out` is populated, including a possibly
    ///           empty answer list (NXDOMAIN with no records is valid).
    /// FAILURE : Returns false on header truncation, name-compression
    ///           cycles, oversized labels, or out-of-bounds reads.
    ///           `out` is left partially filled but is safe to
    ///           `DnsResponseDeinit`.
    ///
    bool DnsParseResponse(DnsResponse *out, const u8 *buf, u64 len, Allocator *alloc);

    ///
    /// Release every owned string / Vec inside a parsed `DnsResponse`.
    ///
    /// SUCCESS : Returns to the caller. `*self` is zeroed.
    /// FAILURE : Function cannot fail. NULL `self` is a no-op.
    ///
    /// TAGS: Dns, Parser, Deinit, Lifecycle
    ///
    void DnsResponseDeinit(DnsResponse *self);

    ///
    /// Release the owned string / Vec slots inside a single `DnsRecord`.
    /// Used both by `DnsResponseDeinit` (per-record) and by callers that
    /// build records by hand.
    ///
    /// SUCCESS : Returns to the caller. `*self` is zeroed.
    /// FAILURE : Function cannot fail. NULL `self` is a no-op.
    ///
    /// TAGS: Dns, Parser, Deinit, Lifecycle
    ///
    void DnsRecordDeinit(DnsRecord *self);

#ifdef __cplusplus
}
#endif

#endif // MISRA_PARSERS_DNS_H
