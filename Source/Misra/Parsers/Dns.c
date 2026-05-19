/// file      : parsers/dns.c
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// DNS wire format (RFC 1035) encoder + decoder.

#include <Misra/Parsers/Dns.h>
#include <Misra/Std/Container/Buf.h>
#include <Misra/Std/Log.h>
#include <Misra/Std/Memory.h>
#include <Misra/Types.h>

// ---------------------------------------------------------------------------
// Encoding
// ---------------------------------------------------------------------------

// Encode a hostname into the on-wire label form:
//   [len][label-bytes...]  ...  [0]
// e.g. "example.com" -> 0x07 "example" 0x03 "com" 0x00.
// Returns false on label > 63 bytes or total > 255 bytes.
static bool encode_qname(DnsWireBuf *out, const char *name) {
    if (!name) {
        return false;
    }
    const char *p           = name;
    u64         total_bytes = 0;
    while (*p) {
        // Find next dot or end-of-string.
        const char *seg = p;
        while (*p && *p != '.') {
            ++p;
        }
        u64 seg_len = (u64)(p - seg);
        if (seg_len == 0) {
            // Trailing dot at the end is valid (means root); leading or
            // middle empty labels are not.
            if (*p == '\0') {
                break;
            }
            return false;
        }
        if (seg_len > 63) {
            return false;
        }
        total_bytes += 1 + seg_len;
        if (total_bytes > 254) {
            return false;
        }
        if (!BufWriteU8(out, (u8)seg_len)) {
            return false;
        }
        if (!BufPushBytes(out, (const u8 *)seg, seg_len)) {
            return false;
        }
        if (*p == '.') {
            ++p;
        }
    }
    // Root label terminator.
    return BufWriteU8(out, 0);
}

bool DnsBuildQuery(DnsWireBuf *out, u16 id, const char *name, DnsType type) {
    if (!out || !name) {
        return false;
    }
    // Header: id, flags=0x0100 (RD set), qdcount=1, ancount=0, nscount=0, arcount=0.
    if (!BufWriteFmt(out, "{>2r}{>2r}{>2r}{>2r}{>2r}{>2r}", id, (u16)0x0100u, (u16)1u, (u16)0u, (u16)0u, (u16)0u)) {
        return false;
    }
    // Question section: qname + qtype + qclass (IN = 1).
    if (!encode_qname(out, name)) {
        return false;
    }
    u16 qtype  = (u16)type;
    u16 qclass = 1u;
    return BufAppendFmt(out, "{>2r}{>2r}", qtype, qclass);
}

// Decode a domain name starting at the iter's current position,
// following any 0xC0-prefixed compression pointers. The iter is
// advanced PAST the name in the linear (non-pointer-followed)
// reading; that lets the caller continue reading the record after
// the name regardless of how many pointer hops the decoded form took.
//
// out_name is the caller-managed Str the decoded form is appended
// to (dotted form, NO trailing dot to keep formatting consistent
// with what callers pass into DnsBuildQuery).
//
// Returns false on cycles, oversized names, or out-of-bounds.
static bool decode_name(ByteIter *it, Str *out_name) {
    const u32 MAX_HOPS  = 64;
    u32       hops      = 0;
    bool      jumped    = false;
    u64       linear_p  = it->pos;
    u64       cur       = it->pos;
    u64       name_len  = 0;
    u64       label_idx = 0;
    while (cur < it->length) {
        u8 b = it->data[cur];
        if (b == 0) {
            ++cur;
            if (!jumped) {
                linear_p = cur;
            }
            it->pos = linear_p;
            return true;
        }
        if ((b & 0xC0u) == 0xC0u) {
            // Compression pointer: 14-bit offset.
            if (cur + 2 > it->length) {
                return false;
            }
            u16 ptr = (u16)(((b & 0x3Fu) << 8) | it->data[cur + 1]);
            if (!jumped) {
                // First jump: linear cursor advances past the 2-byte
                // pointer; everything after will be read from the
                // pointer target.
                linear_p = cur + 2;
                jumped   = true;
            }
            if (ptr >= cur) {
                // Forward / self-pointer -- reject to avoid cycles.
                return false;
            }
            cur = ptr;
            if (++hops > MAX_HOPS) {
                return false;
            }
            continue;
        }
        if ((b & 0xC0u) != 0) {
            // Reserved top-bit pattern.
            return false;
        }
        u64 label_len = (u64)b;
        if (cur + 1 + label_len > it->length) {
            return false;
        }
        if (name_len + 1 + label_len > 253) {
            return false;
        }
        if (label_idx > 0) {
            StrPushBack(out_name, '.');
            ++name_len;
        }
        for (u64 i = 0; i < label_len; ++i) {
            StrPushBack(out_name, (char)it->data[cur + 1 + i]);
        }
        name_len += label_len;
        cur      += 1 + label_len;
        if (!jumped) {
            linear_p = cur;
        }
        ++label_idx;
    }
    return false;
}

// Decode one resource record. Advances the iter past the record.
static bool decode_record(ByteIter *it, DnsRecord *rec, Allocator *alloc) {
    rec->name   = StrInit(alloc);
    rec->target = StrInit(alloc);
    rec->rdata  = VecInitT(rec->rdata, alloc);
    MemSet(rec->ipv4, 0, sizeof(rec->ipv4));
    MemSet(rec->ipv6, 0, sizeof(rec->ipv6));

    if (!decode_name(it, &rec->name)) {
        return false;
    }
    u16 type_bits, class_bits, rdlength;
    u32 ttl;
    if (!BufReadFmt(it, "{>2r}{>2r}{>4r}{>2r}", type_bits, class_bits, ttl, rdlength)) {
        return false;
    }
    rec->type   = (DnsType)type_bits;
    rec->rclass = class_bits;
    rec->ttl    = ttl;

    if (it->pos + (u64)rdlength > it->length) {
        return false;
    }
    u64 rdata_start = it->pos;

    // Stash raw rdata.
    if (!BufPushBytes(&rec->rdata, it->data + rdata_start, rdlength)) {
        return false;
    }

    // Type-specific decode.
    switch (rec->type) {
        case DNS_TYPE_A :
            if (rdlength != 4) {
                return false;
            }
            for (u64 i = 0; i < 4; ++i) {
                rec->ipv4[i] = it->data[rdata_start + i];
            }
            break;
        case DNS_TYPE_AAAA :
            if (rdlength != 16) {
                return false;
            }
            for (u64 i = 0; i < 16; ++i) {
                rec->ipv6[i] = it->data[rdata_start + i];
            }
            break;
        case DNS_TYPE_CNAME :
        case DNS_TYPE_NS :
        case DNS_TYPE_PTR : {
            // The name lives inside the rdata window; decode_name may
            // follow compression pointers back into the whole message,
            // so we use a sub-iter cloned from the main one.
            ByteIter sub = *it;
            if (!decode_name(&sub, &rec->target)) {
                return false;
            }
            // The outer cursor advances by rdlength regardless of how
            // far the sub-iter got within the rdata window.
            break;
        }
        default :
            // Unparsed type -- raw rdata above is enough for now.
            break;
    }

    it->pos = rdata_start + (u64)rdlength;
    return true;
}

// ---------------------------------------------------------------------------
// Top-level parse
// ---------------------------------------------------------------------------

static bool decode_record_list(ByteIter *it, u16 count, DnsRecords *out, Allocator *alloc) {
    for (u16 i = 0; i < count; ++i) {
        DnsRecord rec = {0};
        if (!decode_record(it, &rec, alloc)) {
            DnsRecordDeinit(&rec);
            return false;
        }
        VecPushBackR(out, rec);
    }
    return true;
}

bool DnsParseResponse(DnsResponse *out, const u8 *buf, u64 len, Allocator *alloc) {
    if (!out || !buf || !alloc) {
        return false;
    }
    MemSet(out, 0, sizeof(*out));
    out->answers    = VecInitT(out->answers, alloc);
    out->authority  = VecInitT(out->authority, alloc);
    out->additional = VecInitT(out->additional, alloc);

    if (len < 12) {
        LOG_ERROR("DNS response shorter than header (got {} bytes, need 12)", len);
        return false;
    }
    ByteIter it = ByteIterFromMemory(buf, len);
    u16      flags, qd, an, ns, ar;
    if (!BufReadFmt(&it, "{>2r}{>2r}{>2r}{>2r}{>2r}{>2r}", out->id, flags, qd, an, ns, ar)) {
        return false;
    }
    out->is_response       = (flags & 0x8000u) != 0;
    out->authoritative     = (flags & 0x0400u) != 0;
    out->truncated         = (flags & 0x0200u) != 0;
    out->recursion_desired = (flags & 0x0100u) != 0;
    out->recursion_avail   = (flags & 0x0080u) != 0;
    out->rcode             = (DnsRcode)(flags & 0x000Fu);

    // Skip echoed question section: each question is qname + qtype(2) + qclass(2).
    for (u16 i = 0; i < qd; ++i) {
        Str dummy = StrInit(alloc);
        if (!decode_name(&it, &dummy)) {
            StrDeinit(&dummy);
            return false;
        }
        StrDeinit(&dummy);
        u16 qtype, qclass;
        if (!BufReadFmt(&it, "{>2r}{>2r}", qtype, qclass)) {
            return false;
        }
    }

    if (!decode_record_list(&it, an, &out->answers, alloc) || !decode_record_list(&it, ns, &out->authority, alloc) ||
        !decode_record_list(&it, ar, &out->additional, alloc)) {
        return false;
    }
    return true;
}

// ---------------------------------------------------------------------------
// Cleanup
// ---------------------------------------------------------------------------

void DnsRecordDeinit(DnsRecord *self) {
    if (!self) {
        return;
    }
    StrDeinit(&self->name);
    StrDeinit(&self->target);
    VecDeinit(&self->rdata);
}

static void deinit_record_list(DnsRecords *list) {
    if (!list || !list->data) {
        return;
    }
    VecForeachPtr(list, r) {
        DnsRecordDeinit(r);
    }
    VecDeinit(list);
}

void DnsResponseDeinit(DnsResponse *self) {
    if (!self) {
        return;
    }
    deinit_record_list(&self->answers);
    deinit_record_list(&self->authority);
    deinit_record_list(&self->additional);
}
