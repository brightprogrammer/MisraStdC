/// file      : parsers/tzif.c
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// TZif (`/etc/localtime`) offset resolver backing `ClockLocal`. See `Tzif.h`
/// for the contract and the scope note. Written on the parser combinator DSL
/// over a `BufIter` (`#define PC_ITER BufIter`): TZif is big-endian (RFC 8536).
/// The grammar reads the header (a produced value) and the version chooses the
/// block (`Dispatch`); each version arm reads transition times at its own width
/// (`FindBestTransitionTimeV1` vs `...V2`), so the width is a control-flow
/// decision, not a field anyone checks. Each resolution phase is its own parser
/// that takes what it needs and returns what it found, the arrays stream through
/// `PcMatchExactlyN`, and the context holds only the one ambient fact the phases
/// read down the stack: the instant being resolved. Every read is
/// bounds-checked and fails the rule (never aborts) on a short file --
/// `/etc/localtime` is untrusted -- surfacing as a soft `false`.

#include <Misra/Parsers/Tzif.h>

#include <Misra/Std.h>
#include <Misra/Std/File.h>
#include <Misra/Std/Io.h>
#include <Misra/Std/Log.h>

#define PC_ITER BufIter
#include <Misra/ParserCombinator.h>

// TZif fixed-layout header (RFC 8536 4.1): "TZif" magic, version, 15 reserved
// bytes, then six big-endian u32 counts. Produced by `Header`, threaded into the
// block parsers as an input.
typedef struct {
    u8  version;
    u32 isutcnt;
    u32 isstdcnt;
    u32 leapcnt;
    u32 timecnt;
    u32 typecnt;
    u32 charcnt;
} TzifHeader;

// One count-prefixed array to scan for a record at a selected index: `count`
// records, keep the one at `sel` (-1 = nothing selected).
typedef struct {
    u32 count;
    i64 sel;
} PhaseIn;

// One ttinfo record's decoded fields.
typedef struct {
    i32 utoff;
    u8  isdst;
} TtInfoVal;

// The one ambient fact the phases read down the call stack: the instant being
// resolved. Everything a phase *produces* is a return value, not a field here,
// and the block's time width is decided by which version arm runs, not stored.
typedef struct PcParserCtx {
    i64 unix_seconds;
} PcParserCtx;

// Backtracking savepoint: the context is a flat value, so a mark is a copy of it
// and rollback restores it -- a `PcChoice` arm leaves nothing behind.
typedef PcParserCtx    PcParserCtxMark;
static PcParserCtxMark PcParserCtxSnapshot(PcParserCtx *ctx) {
    return *ctx;
}
static void PcParserCtxRollback(PcParserCtx *ctx, PcParserCtxMark mark) {
    *ctx = mark;
}

// Size of a version-1 data block, used only to skip it on the way to the
// version-2+ block (transition times 4 bytes, leap records 4+4).
static u64 tzif_v1_data_size(const TzifHeader *h) {
    return (u64)h->timecnt * 4 + (u64)h->timecnt * 1 + (u64)h->typecnt * 6 + (u64)h->charcnt * 1 + (u64)h->leapcnt * 8 +
           (u64)h->isstdcnt * 1 + (u64)h->isutcnt * 1;
}

// The magic and a generic byte skip.
PcRecognizer(TzifMagic) {
    PcSatisfyStr("TZif") {}
}
PcRecognizer(Skip, u64) {
    PcSkipBytes(expect);
}

// "TZif" + version + 15 reserved + six big-endian u32 counts.
PcParser(Header, TzifHeader) {
    PcSeq() {
        PcExpect(TzifMagic);
        PcMatch(PcU8, &value->version);
        PcExpect(Skip, 15);
        PcMatch(PcU32BE, &value->isutcnt);
        PcMatch(PcU32BE, &value->isstdcnt);
        PcMatch(PcU32BE, &value->leapcnt);
        PcMatch(PcU32BE, &value->timecnt);
        PcMatch(PcU32BE, &value->typecnt);
        PcMatch(PcU32BE, &value->charcnt);
    }
}

// One ttinfo record: utoff (signed), the isdst flag, and a designation index we
// skip. `PcI32BE` reads straight into the signed field -- no unsigned temporary.
PcParser(TtInfo, TtInfoVal) {
    PcSeq() {
        PcMatch(PcI32BE, &value->utoff);
        PcMatch(PcU8, &value->isdst);
        PcExpect(Skip, 1);
    }
}

// Phase 1: transitions are ascending; the last index not in the future, or -1 if
// they are all ahead of the instant. The time is signed and its width is the
// version's (32-bit v1, 64-bit v2) -- so the two arms read via `PcI32BE` and
// `PcI64BE`, and that reader choice is the only difference between them.
PcParser(FindBestTransitionTimeV1, u32, i64) {
    i32 t = 0;
    PcSeq() {
        *value = -1;
        PcMatchExactlyN(expect, i, PcI32BE, &t) {
            if (t <= ctx->unix_seconds) // i32 t sign-extends against the i64 instant
                *value = (i64)i;
        }
    }
}
PcParser(FindBestTransitionTimeV2, u32, i64) {
    i64 t = 0;
    PcSeq() {
        *value = -1;
        PcMatchExactlyN(expect, i, PcI64BE, &t) {
            if (t <= ctx->unix_seconds)
                *value = (i64)i;
        }
    }
}

// Phase 2: the type-index array; the index recorded at `sel`, or -1 when there
// was no transition to select.
PcParser(PickTransitionType, PhaseIn, i64) {
    u8 tix = 0;
    PcSeq() {
        *value = -1;
        PcMatchExactlyN(expect.count, i, PcU8, &tix) {
            if ((i64)i == expect.sel)
                *value = (i64)tix;
        }
    }
}

// Phase 3: the ttinfo records; the selected type's utoff, else the first
// standard-time (isdst==0) type as the pre-history fallback. No usable type ->
// the rule rejects.
PcParser(PickUtOffset, PhaseIn, i32) {
    TtInfoVal tt       = {0};
    i32       off      = 0;
    i32       std_off  = 0;
    bool      have_off = false;
    bool      have_std = false;
    PcSeq() {
        PcMatchExactlyN(expect.count, i, TtInfo, &tt) {
            if ((i64)i == expect.sel) {
                off      = tt.utoff;
                have_off = true;
            }
            if (!have_std && tt.isdst == 0) {
                std_off  = tt.utoff;
                have_std = true;
            }
        }
        if (have_off)
            *value = off;
        else if (have_std)
            *value = std_off;
        else
            PcReject();
    }
}

// Version dispatch. v1: resolve the block right here (4-byte times). v2+: skip
// the v1 block, read the v2 header, resolve its block (8-byte times). Each arm's
// guard rejects before consuming, so the choice falls through cleanly.
PcParser(ResolveV1, TzifHeader, i32) {
    i64 best   = 0;
    i64 chosen = 0;
    PcSeq() {
        if (expect.version >= '2')
            PcReject();
        PcMatch(FindBestTransitionTimeV1, expect.timecnt, &best);
        PcMatch(PickTransitionType, ((PhaseIn) {expect.timecnt, best}), &chosen);
        PcMatch(PickUtOffset, ((PhaseIn) {expect.typecnt, chosen}), value);
    }
}
PcParser(ResolveV2, TzifHeader, i32) {
    TzifHeader h2     = {0};
    i64        best   = 0;
    i64        chosen = 0;
    PcSeq() {
        if (expect.version < '2')
            PcReject();
        PcExpect(Skip, tzif_v1_data_size(&expect));
        PcMatch(Header, &h2);
        PcMatch(FindBestTransitionTimeV2, h2.timecnt, &best);
        PcMatch(PickTransitionType, ((PhaseIn) {h2.timecnt, best}), &chosen);
        PcMatch(PickUtOffset, ((PhaseIn) {h2.typecnt, chosen}), value);
    }
}
PcParser(Dispatch, TzifHeader, i32) {
    PcChoice() {
        PcAlt(ResolveV1, expect, value);
        PcAlt(ResolveV2, expect, value);
    }
}

// TZif: the fixed header, then the version-appropriate data block.
PcParser(Tzif, i32) {
    TzifHeader h = {0};
    PcSeq() {
        PcMatch(Header, &h);
        PcMatch(Dispatch, h, value);
    }
}

bool TzifOffsetFromBuf(const u8 *data, size len, i64 unix_seconds, i32 *out_offset_seconds) {
    if (!data || !out_offset_seconds)
        return false;

    BufIter      in     = BufIterFromMemory((u8 *)data, len);
    PcParserCtx  cx     = {.unix_seconds = unix_seconds};
    PcParserCtx *ctx    = &cx;
    i32          offset = 0;

    if (!(PcRun(Tzif, &in, &offset) & PC_PARSER_STATUS_SUCCESS)) {
        LOG_ERROR("Tzif: malformed or unresolvable TZif data");
        return false;
    }
    *out_offset_seconds = offset;
    return true;
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
