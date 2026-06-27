/// file      : parsers/tzif.c
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// TZif (`/etc/localtime`) offset resolver backing `ClockLocal`. See
/// `Tzif.h` for the contract and the scope note. All on-disk decoding
/// goes through `BufReadFmt` big-endian (`{>Nr}`) directives -- TZif is
/// a big-endian format (RFC 8536) -- so there are no packed structs and
/// no manual byteswaps, matching the rest of the Parsers/ family. Every
/// read is the non-`Must` `BufReadFmt` / `IterMove`, which return false
/// on a short file instead of aborting: `/etc/localtime` is untrusted.

#include <Misra/Parsers/Tzif.h>

#include <Misra/Std.h>
#include <Misra/Std/File.h>
#include <Misra/Std/Io.h>
#include <Misra/Std/Log.h>

// TZif fixed-layout header (RFC 8536 4.1): "TZif" magic, 1-byte
// version, 15 reserved bytes, then six big-endian u32 counts. 44 bytes.
#define TZIF_HEADER_SIZE 44

// The six u32 counts, read in one shot.
#define FMT_TZIF_COUNTS_BE                                                                                             \
    "{>4r}" /* isutcnt  */                                                                                             \
    "{>4r}" /* isstdcnt */                                                                                             \
    "{>4r}" /* leapcnt  */                                                                                             \
    "{>4r}" /* timecnt  */                                                                                             \
    "{>4r}" /* typecnt  */                                                                                             \
    "{>4r}" /* charcnt  */

// One ttinfo record (RFC 8536 4.2): i32 utoff, u8 isdst, u8 desigidx.
#define FMT_TZIF_TTINFO_BE                                                                                             \
    "{>4r}" /* utoff    */                                                                                             \
    "{>1r}" /* isdst    */                                                                                             \
    "{>1r}" /* desigidx */

typedef struct {
    u8  version;
    u32 isutcnt;
    u32 isstdcnt;
    u32 leapcnt;
    u32 timecnt;
    u32 typecnt;
    u32 charcnt;
} TzifHeader;

static bool tzif_read_header(BufIter *it, TzifHeader *h) {
    if ((u64)IterRemainingLength(it) < TZIF_HEADER_SIZE)
        return false;

    // "TZif" magic matched inline; a non-TZif file fails the read here.
    u8 ver = 0;
    if (!BufReadFmt(it, "TZif{>1r}", ver))
        return false;
    if (!IterMove(it, 15)) // reserved
        return false;

    if (!BufReadFmt(it, FMT_TZIF_COUNTS_BE, h->isutcnt, h->isstdcnt, h->leapcnt, h->timecnt, h->typecnt, h->charcnt))
        return false;
    h->version = ver;
    return true;
}

// Size of a version-1 data block, used only to skip it on the way to
// the version-2+ block. v1 transition times and leap records are 4 and
// 4+4 bytes respectively.
static u64 tzif_v1_data_size(const TzifHeader *h) {
    return (u64)h->timecnt * 4 + (u64)h->timecnt * 1 + (u64)h->typecnt * 6 + (u64)h->charcnt * 1 + (u64)h->leapcnt * 8 +
           (u64)h->isstdcnt * 1 + (u64)h->isutcnt * 1;
}

// Resolve the offset from a data block whose transition times are
// `time_width` bytes (4 for v1, 8 for v2+). The cursor must sit at the
// start of the transition-time array. Streams through the block without
// storing it: find the last transition <= `unix_seconds`, take its
// type's utoff, with the first standard-time (isdst==0) ttinfo as the
// pre-history / past-table fallback.
static bool tzif_resolve(BufIter *it, u32 time_width, u32 timecnt, u32 typecnt, i64 unix_seconds, i32 *out_offset) {
    if (typecnt == 0)
        return false;

    // Phase 1: transitions are ascending; remember the index of the
    // last one not in the future.
    bool found = false;
    i64  best  = 0;
    for (u32 i = 0; i < timecnt; ++i) {
        i64 t = 0;
        if (time_width == 8) {
            if (!BufReadFmt(it, "{>8r}", t))
                return false;
        } else {
            i32 t32 = 0;
            if (!BufReadFmt(it, "{>4r}", t32))
                return false;
            t = t32;
        }
        if (t <= unix_seconds) {
            found = true;
            best  = (i64)i;
        }
    }

    // Phase 2: the type-index array is `timecnt` bytes; pick the one at
    // `best` and step the cursor to the array's end either way.
    u8   chosen      = 0;
    bool have_chosen = false;
    if (found) {
        if (best > 0 && !IterMove(it, best))
            return false;
        if (!BufReadFmt(it, "{>1r}", chosen))
            return false;
        have_chosen = true;
        i64 rest    = (i64)timecnt - best - 1;
        if (rest > 0 && !IterMove(it, rest))
            return false;
    } else if (timecnt > 0 && !IterMove(it, (i64)timecnt)) {
        return false;
    }

    // Phase 3: ttinfo records. Grab the chosen type's utoff; remember
    // the first standard-time offset for the fallback.
    bool have_std = false;
    i32  std_off  = 0;
    bool have_off = false;
    i32  off      = 0;
    for (u32 i = 0; i < typecnt; ++i) {
        i32 utoff = 0;
        u8  isdst = 0, desigidx = 0;
        if (!BufReadFmt(it, FMT_TZIF_TTINFO_BE, utoff, isdst, desigidx))
            return false;
        if (have_chosen && i == (u32)chosen) {
            off      = utoff;
            have_off = true;
        }
        if (!have_std && isdst == 0) {
            std_off  = utoff;
            have_std = true;
        }
    }

    if (have_off) {
        *out_offset = off;
        return true;
    }
    if (have_std) {
        *out_offset = std_off;
        return true;
    }
    return false;
}

bool TzifOffsetFromBuf(const u8 *data, size len, i64 unix_seconds, i32 *out_offset_seconds) {
    if (!data || !out_offset_seconds)
        return false;

    BufIter    it = BufIterFromMemory((u8 *)data, len);
    TzifHeader h1;
    if (!tzif_read_header(&it, &h1)) {
        LOG_ERROR("Tzif: malformed header");
        return false;
    }

    bool ok;
    i32  offset = 0;
    if (h1.version >= '2') {
        // Skip the v1 data block, then parse the v2+ header and its
        // 8-byte-time data block (the authoritative one).
        if (!IterMove(&it, (i64)tzif_v1_data_size(&h1))) {
            LOG_ERROR("Tzif: truncated v1 data block");
            return false;
        }
        TzifHeader h2;
        if (!tzif_read_header(&it, &h2)) {
            LOG_ERROR("Tzif: malformed v2 header");
            return false;
        }
        ok = tzif_resolve(&it, 8, h2.timecnt, h2.typecnt, unix_seconds, &offset);
    } else {
        ok = tzif_resolve(&it, 4, h1.timecnt, h1.typecnt, unix_seconds, &offset);
    }

    if (ok)
        *out_offset_seconds = offset;
    return ok;
}

bool TzifLocalOffsetSeconds(i64 unix_seconds, i32 *out_offset_seconds, Allocator *alloc) {
    Buf data = BufInit(alloc);
    if (FileReadAndClose("/etc/localtime", &data) < 0) {
        BufDeinit(&data);
        LOG_ERROR("Tzif: cannot read /etc/localtime");
        return false;
    }
    bool ok = TzifOffsetFromBuf(BufData(&data), BufLength(&data), unix_seconds, out_offset_seconds);
    BufDeinit(&data);
    return ok;
}
