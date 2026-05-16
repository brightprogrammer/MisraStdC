/// file      : parsers/dns.c
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// DNS wire format (RFC 1035) encoder + decoder.

#include <Misra/Parsers/Dns.h>
#include <Misra/Std/Log.h>
#include <Misra/Std/Memory.h>
#include <Misra/Types.h>

// ---------------------------------------------------------------------------
// Encoding
// ---------------------------------------------------------------------------

// Push a big-endian u16 to the wire buffer.
static bool push_be16(DnsWireBuf *out, u16 v) {
    u8 hi = (u8)(v >> 8);
    u8 lo = (u8)(v & 0xFFu);
    return VecPushBackR(out, hi) && VecPushBackR(out, lo);
}

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
            // Empty label (consecutive dots or trailing dot). Trailing
            // dot is valid -- it just means we're at the root. Break.
            if (*p == '\0') {
                break;
            }
            // Leading or middle empty label is invalid.
            return false;
        }
        if (seg_len > 63) {
            return false;
        }
        total_bytes += 1 + seg_len;
        if (total_bytes > 254) {
            return false;
        }
        if (!VecPushBackR(out, (u8)seg_len)) {
            return false;
        }
        for (u64 i = 0; i < seg_len; ++i) {
            if (!VecPushBackR(out, (u8)seg[i])) {
                return false;
            }
        }
        if (*p == '.') {
            ++p;
        }
    }
    // Root label terminator.
    return VecPushBackR(out, (u8)0);
}

bool DnsBuildQuery(DnsWireBuf *out, u16 id, const char *name, DnsType type) {
    if (!out || !name) {
        return false;
    }
    VecClear(out);

    // Header: id, flags=0x0100 (RD set), qdcount=1, ancount=0, nscount=0, arcount=0.
    if (!push_be16(out, id) || !push_be16(out, 0x0100u) || !push_be16(out, 1u) || !push_be16(out, 0u) ||
        !push_be16(out, 0u) || !push_be16(out, 0u)) {
        return false;
    }
    // Question section: qname + qtype + qclass (IN = 1).
    if (!encode_qname(out, name)) {
        return false;
    }
    if (!push_be16(out, (u16)type) || !push_be16(out, 1u)) {
        return false;
    }
    return true;
}

// ---------------------------------------------------------------------------
// Decoding helpers
// ---------------------------------------------------------------------------

// Read a big-endian u16 from `buf` at `pos`; bumps `pos`.
// Returns false on out-of-bounds.
static bool read_be16(const u8 *buf, u64 len, u64 *pos, u16 *out) {
    if (*pos + 2 > len) {
        return false;
    }
    *out  = (u16)(((u16)buf[*pos] << 8) | buf[*pos + 1]);
    *pos += 2;
    return true;
}

static bool read_be32(const u8 *buf, u64 len, u64 *pos, u32 *out) {
    if (*pos + 4 > len) {
        return false;
    }
    *out  = ((u32)buf[*pos] << 24) | ((u32)buf[*pos + 1] << 16) | ((u32)buf[*pos + 2] << 8) | (u32)buf[*pos + 3];
    *pos += 4;
    return true;
}

// Decode a domain name starting at `*pos` in `buf`, following any
// 0xC0-prefixed compression pointers. `*pos` is advanced PAST the
// name in the linear (non-pointer-followed) reading; that lets the
// caller continue reading the record after the name regardless of
// how many pointer hops the decoded form took.
//
// out_name is the caller-managed Str the decoded form is appended
// to (dotted form, NO trailing dot to keep formatting consistent
// with what callers pass into DnsBuildQuery).
//
// Returns false on cycles, oversized names, or out-of-bounds.
static bool decode_name(const u8 *buf, u64 len, u64 *pos, Str *out_name) {
    const u32 MAX_HOPS  = 64;
    u32       hops      = 0;
    bool      jumped    = false;
    u64       linear_p  = *pos;
    u64       cur       = *pos;
    u64       name_len  = 0;
    u64       label_idx = 0;
    while (cur < len) {
        u8 b = buf[cur];
        if (b == 0) {
            ++cur;
            if (!jumped) {
                linear_p = cur;
            }
            *pos = linear_p;
            return true;
        }
        if ((b & 0xC0u) == 0xC0u) {
            // Compression pointer: 14-bit offset.
            if (cur + 2 > len) {
                return false;
            }
            u16 ptr = (u16)(((b & 0x3Fu) << 8) | buf[cur + 1]);
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
        if (cur + 1 + label_len > len) {
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
            StrPushBack(out_name, (char)buf[cur + 1 + i]);
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

// Decode one resource record. Advances `*pos` past the record.
static bool decode_record(const u8 *buf, u64 len, u64 *pos, DnsRecord *rec, Allocator *alloc) {
    rec->name   = StrInit(alloc);
    rec->target = StrInit(alloc);
    rec->rdata  = VecInitT(rec->rdata, alloc);
    MemSet(rec->ipv4, 0, sizeof(rec->ipv4));
    MemSet(rec->ipv6, 0, sizeof(rec->ipv6));

    if (!decode_name(buf, len, pos, &rec->name)) {
        return false;
    }
    u16 type_bits  = 0;
    u16 class_bits = 0;
    u32 ttl        = 0;
    u16 rdlength   = 0;
    if (!read_be16(buf, len, pos, &type_bits) || !read_be16(buf, len, pos, &class_bits) ||
        !read_be32(buf, len, pos, &ttl) || !read_be16(buf, len, pos, &rdlength)) {
        return false;
    }
    rec->type   = (DnsType)type_bits;
    rec->rclass = class_bits;
    rec->ttl    = ttl;

    if (*pos + (u64)rdlength > len) {
        return false;
    }
    u64 rdata_start = *pos;

    // Stash raw rdata.
    for (u64 i = 0; i < rdlength; ++i) {
        VecPushBackR(&rec->rdata, buf[rdata_start + i]);
    }

    // Type-specific decode.
    switch (rec->type) {
        case DNS_TYPE_A :
            if (rdlength != 4) {
                return false;
            }
            for (u64 i = 0; i < 4; ++i) {
                rec->ipv4[i] = buf[rdata_start + i];
            }
            break;
        case DNS_TYPE_AAAA :
            if (rdlength != 16) {
                return false;
            }
            for (u64 i = 0; i < 16; ++i) {
                rec->ipv6[i] = buf[rdata_start + i];
            }
            break;
        case DNS_TYPE_CNAME :
        case DNS_TYPE_NS :
        case DNS_TYPE_PTR : {
            u64 sub_pos = rdata_start;
            if (!decode_name(buf, len, &sub_pos, &rec->target)) {
                return false;
            }
            // sub_pos lands inside the rdata window; the linear cursor
            // for the outer record list advances by rdlength regardless.
            break;
        }
        default :
            // Unparsed type -- raw rdata above is enough for now.
            break;
    }

    *pos = rdata_start + (u64)rdlength;
    return true;
}

// ---------------------------------------------------------------------------
// Top-level parse
// ---------------------------------------------------------------------------

static bool decode_record_list(const u8 *buf, u64 len, u64 *pos, u16 count, DnsRecords *out, Allocator *alloc) {
    for (u16 i = 0; i < count; ++i) {
        DnsRecord rec = {0};
        if (!decode_record(buf, len, pos, &rec, alloc)) {
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
    u64 pos = 0;
    u16 flags, qd, an, ns, ar;
    if (!read_be16(buf, len, &pos, &out->id) || !read_be16(buf, len, &pos, &flags) || !read_be16(buf, len, &pos, &qd) ||
        !read_be16(buf, len, &pos, &an) || !read_be16(buf, len, &pos, &ns) || !read_be16(buf, len, &pos, &ar)) {
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
        if (!decode_name(buf, len, &pos, &dummy)) {
            StrDeinit(&dummy);
            return false;
        }
        StrDeinit(&dummy);
        u16 qtype, qclass;
        if (!read_be16(buf, len, &pos, &qtype) || !read_be16(buf, len, &pos, &qclass)) {
            return false;
        }
    }

    if (!decode_record_list(buf, len, &pos, an, &out->answers, alloc) ||
        !decode_record_list(buf, len, &pos, ns, &out->authority, alloc) ||
        !decode_record_list(buf, len, &pos, ar, &out->additional, alloc)) {
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
