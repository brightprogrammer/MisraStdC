/// file      : parsers/proc_maps.c
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Linux `/proc/self/maps` parser. The line format (kernel doc:
/// `Documentation/filesystems/proc.rst`) is:
///
///   start-end perms offset dev_major:dev_minor inode path
///
/// where:
///   start, end : hex addresses
///   perms      : 4 chars - rwx and (p|s)
///   offset     : hex file offset
///   dev_*      : hex device numbers (we ignore)
///   inode      : decimal (we ignore)
///   path       : optional; anonymous mappings have no path field
///
/// The whole file is a grammar on the parser-combinator DSL over a
/// `StrIter` (the default cursor -- no `#define PC_ITER`): each field is
/// its own rule, `ProcMapLine` a line, and `ProcMapLines` the file --
/// which decodes each line into an entry while folding in `min_addr`,
/// and fails on the first line it cannot parse (naming the cause in the
/// diagnostics sink), so the loader just runs it. Paths can contain
/// spaces -- everything after the inode token runs to the line terminator
/// and is copied into the entry's owned `Str`, so the transient parse
/// buffer is dropped once loading returns.

#include <Misra/Parsers/ProcMaps.h>

#include <Misra/Std.h>
#include <Misra/Std/Log.h>
#include <Misra/Std/Memory.h>

#include <Misra/Std/File.h>
#include <Misra/Std/Utility/StrIter.h>

#include <Misra/ParserCombinator.h>

// The context carries only the diagnostics sink the `PcReport*` macros append to:
// a malformed line is recorded here, not silently dropped. Nothing else is
// ambient -- every field is a return value. Reports are not rolled back on
// backtracking (a recorded error stays recorded), so the savepoint is a no-op.
typedef struct PcParserCtx {
    PcReports  reports;
    Allocator *alloc; // copies each entry's path into an owned Str
} PcParserCtx;
typedef struct {
    u8 unused;
} PcParserCtxMark;
static PcParserCtxMark PcParserCtxSnapshot(PcParserCtx *ctx) {
    (void)ctx;
    return (PcParserCtxMark) {0};
}
static void PcParserCtxRollback(PcParserCtx *ctx, PcParserCtxMark mark) {
    (void)ctx;
    (void)mark;
}

// ---------------------------------------------------------------------------
// Field grammar
// ---------------------------------------------------------------------------

// One hex digit -> its 0..15 value (lower or upper case).
PcParser(HexDigit, u8) {
    PcSatisfyChar(c, (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F')) {
        *value = (u8)(c <= '9' ? c - '0' : (c | 0x20) - 'a' + 10);
    }
}

// A hex run -> u64. One-or-more, so an empty field (no digits) fails the rule.
PcParser(HexU64, u64) {
    u8  d = 0;
    u64 v = 0;
    PcSeq() {
        PcMatchOneOrMore(HexDigit, &d) {
            v = (v << 4) | (u64)d;
        }
        *value = v;
    }
}

// Inter-field whitespace (spaces/tabs), any amount including none.
PcRecognizer(SpaceCh) {
    PcSatisfyChar(c, c == ' ' || c == '\t') {}
}
PcRecognizer(Spaces) {
    PcRecognizeZeroOrMore(SpaceCh);
}

// The '-' between start and end.
PcRecognizer(Dash) {
    PcSatisfyChar(c, c == '-') {}
}

// The four permission characters -> a ProcMapPerms bitmask. Position is
// significant: r/w/x/p set their bit, anything else (a '-' or 's') clears it.
PcParser(PermCh, char) {
    PcSatisfyChar(c, c == 'r' || c == 'w' || c == 'x' || c == 'p' || c == 's' || c == '-') {
        *value = c;
    }
}
PcParser(Perms, u32) {
    char p0 = 0, p1 = 0, p2 = 0, p3 = 0;
    PcSeq() {
        PcMatch(PermCh, &p0);
        PcMatch(PermCh, &p1);
        PcMatch(PermCh, &p2);
        PcMatch(PermCh, &p3);
        *value = (u32)((p0 == 'r' ? PROC_MAP_PERM_READ : 0u) | (p1 == 'w' ? PROC_MAP_PERM_WRITE : 0u) |
                       (p2 == 'x' ? PROC_MAP_PERM_EXEC : 0u) | (p3 == 'p' ? PROC_MAP_PERM_PRIVATE : 0u));
    }
}

// A whitespace-delimited token (the dev and inode fields, which we only skip).
PcRecognizer(TokenCh) {
    PcSatisfyChar(c, c != ' ' && c != '\t' && c != '\n') {}
}
PcRecognizer(Token) {
    PcRecognizeOneOrMore(TokenCh);
}

// The line terminator, consumed after the path has been captured.
PcRecognizer(NewlineCh) {
    PcSatisfyChar(c, c == '\n') {}
}

// One `/proc/self/maps` line's fields -> an entry. Any field failing fails the
// whole rule.
PcParser(ProcMapLine, ProcMapEntry) {
    PcSeq() {
        PcMatch(HexU64, &value->start);
        PcExpect(Dash);
        PcMatch(HexU64, &value->end);
        PcRecognize(Spaces);
        PcMatch(Perms, &value->perms);
        PcRecognize(Spaces);
        PcMatch(HexU64, &value->file_offset);
        PcRecognize(Spaces);
        PcExpect(Token); // dev_major:dev_minor
        PcRecognize(Spaces);
        PcExpect(Token); // inode
        PcRecognize(Spaces);
        // Path: the rest of the line. PcCaptureUntil hands back a (pointer,
        // length) slice into the parse buffer; we copy it into an owned,
        // NUL-terminated Str after the newline is consumed, so a line that fails
        // to terminate never leaks a half-built path.
        Zstr pc_path     = NULL;
        u64  pc_path_len = 0;
        PcCaptureUntil(c, c == '\n', &pc_path, &pc_path_len);
        PcRecognize(NewlineCh);
        value->path = StrInitFromCstr(pc_path, pc_path_len, ctx->alloc);
    }
}

// The whole file: decode each line into `value->entries` (which the caller
// pre-initialized), folding `min_addr` in the same pass. This is not a compiler
// -- the first line the grammar cannot parse records its cause in the sink and
// faults (FAILED, carrying the consumed bit from wherever it got to), rather than
// skipping on; the caller fixes the input and reparses.
PcParser(ProcMapLines, ProcMaps) {
    ProcMapEntry e = {0};
    PcSeq() {
        PcMatchZeroOrMore(ProcMapLine, &e) {
            if (!VecPushBackR(&value->entries, e)) {
                StrDeinit(&e.path);
                PcReject();
            }
            if (VecLen(&value->entries) == 1 || e.start < value->min_addr)
                value->min_addr = e.start;
        }
        // The zero-or-more stops at the first line it cannot parse; anything left
        // means that line is malformed -- name the cause and fault.
        PcFailIfNotEof("malformed /proc/self/maps line");
    }
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

// Parse `raw` (already populated + NUL-terminated) into `entries` and `min_addr`
// through the `ProcMapLines` grammar, each entry owning a copy of its path. A
// line the grammar cannot parse fails the whole load: render its cause under a
// caret to the log and return false (`out` left zeroed). An allocation failure
// fails the same way, without a recorded cause. `raw` is the caller's transient
// buffer -- it is not retained past this call.
static bool proc_maps_parse(ProcMaps *out, Str *raw) {
    StrIter     si = StrIterFromStr(*raw);
    PcParserCtx cx;
    cx.reports       = VecInitT(cx.reports, VecAllocator(&out->entries));
    cx.alloc         = VecAllocator(&out->entries);
    PcParserCtx *ctx = &cx;

    bool ok = (PcRun(ProcMapLines, &si, out) & PC_PARSER_STATUS_SUCCESS);
    if (!ok && VecLen(&cx.reports) > 0) {
        StrInitStack(diag, 256) {
            PcReportsRender(&diag, raw, &cx.reports);
            LOG_ERROR("ProcMaps: {}", diag);
        }
    }
    VecDeinit(&cx.reports);

    if (!ok) {
        ProcMapsDeinit(out);
        return false;
    }
    return true;
}

// Read an open File to true EOF into `raw`. Loops because /proc seq_files
// short-read at record boundaries below the requested size (a short read is
// NOT EOF: treating it as one drops every later entry, including [stack]).
// Does not close `f`.
static bool proc_maps_read_all(File *f, Str *raw) {
    enum {
        CHUNK = 4096
    };
    while (true) {
        u64 grown_to = StrLen(raw) + CHUNK + 1;
        if (!StrReserve(raw, grown_to))
            return false;
        i64 n = FileRead(f, StrEnd(raw), CHUNK);
        if (n < 0)
            return false;
        StrResize(raw, StrLen(raw) + (u64)n);
        if (n == 0)
            break;
    }
    return true;
}

bool proc_maps_load(ProcMaps *out, Allocator *alloc) {
    if (!out || !alloc) {
        LOG_FATAL("ProcMapsLoad: NULL argument");
    }
    MemSet(out, 0, sizeof(*out));
    out->entries = VecInitT(out->entries, alloc);
    Str raw      = StrInit(alloc);

    // `/proc/self/maps` reports stat-size 0 because it's generated by the
    // kernel on read, so we loop-read into a growing buffer ourselves.
    File f = FileOpen("/proc/self/maps", "rb");
    if (!FileIsOpen(&f)) {
        LOG_ERROR("ProcMapsLoad: FileOpen(/proc/self/maps) failed");
        StrDeinit(&raw);
        ProcMapsDeinit(out);
        return false;
    }
    bool ok = proc_maps_read_all(&f, &raw);
    FileClose(&f);
    if (!ok) {
        LOG_ERROR("ProcMapsLoad: FileRead failed");
        StrDeinit(&raw);
        ProcMapsDeinit(out);
        return false;
    }
    if (StrLen(&raw) == 0) {
        LOG_ERROR("ProcMapsLoad: /proc/self/maps was empty");
        StrDeinit(&raw);
        ProcMapsDeinit(out);
        return false;
    }
    bool parsed = proc_maps_parse(out, &raw);
    StrDeinit(&raw);
    return parsed;
}

bool proc_maps_load_from_file(ProcMaps *out, File *f, Allocator *alloc) {
    if (!out || !f || !alloc) {
        LOG_FATAL("ProcMapsLoadFrom: NULL argument");
    }
    MemSet(out, 0, sizeof(*out));
    out->entries = VecInitT(out->entries, alloc);
    Str raw      = StrInit(alloc);
    if (!FileIsOpen(f)) {
        LOG_ERROR("ProcMapsLoadFrom: file is not open");
        StrDeinit(&raw);
        ProcMapsDeinit(out);
        return false;
    }
    if (!proc_maps_read_all(f, &raw)) {
        LOG_ERROR("ProcMapsLoadFrom: FileRead failed");
        StrDeinit(&raw);
        ProcMapsDeinit(out);
        return false;
    }
    bool parsed = proc_maps_parse(out, &raw);
    StrDeinit(&raw);
    return parsed;
}

bool proc_maps_load_from_bytes(ProcMaps *out, const u8 *bytes, u64 len, Allocator *alloc) {
    if (!out || !alloc || (!bytes && len)) {
        LOG_FATAL("ProcMapsLoadFrom: NULL argument");
    }
    MemSet(out, 0, sizeof(*out));
    out->entries = VecInitT(out->entries, alloc);
    Str raw      = StrInit(alloc);
    if (len) {
        if (!StrReserve(&raw, len + 1)) {
            StrDeinit(&raw);
            ProcMapsDeinit(out);
            return false;
        }
        MemCopy(StrEnd(&raw), bytes, len);
        StrResize(&raw, len);
    }
    bool parsed = proc_maps_parse(out, &raw);
    StrDeinit(&raw);
    return parsed;
}

void ProcMapsDeinit(ProcMaps *self) {
    if (!self)
        return;
    for (u64 i = 0; i < VecLen(&self->entries); ++i) {
        ProcMapEntry *e = VecPtrAt(&self->entries, i);
        StrDeinit(&e->path);
    }
    VecDeinit(&self->entries);
    MemSet(self, 0, sizeof(*self));
}

const ProcMapEntry *ProcMapsFindByAddr(const ProcMaps *self, u64 addr) {
    if (!self)
        return NULL;
    for (u64 i = 0; i < VecLen(&self->entries); ++i) {
        const ProcMapEntry *e = VecPtrAt(&self->entries, i);
        if (addr >= e->start && addr < e->end) {
            return e;
        }
    }
    return NULL;
}
