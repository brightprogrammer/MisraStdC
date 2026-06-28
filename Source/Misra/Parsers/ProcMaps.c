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
/// Paths can contain spaces — we treat everything after the inode
/// field's trailing whitespace as the path, up to the line terminator.

#include <Misra/Parsers/ProcMaps.h>

#include <Misra/Std.h>
#include <Misra/Std/Log.h>
#include <Misra/Std/Memory.h>

#include <Misra/Std/File.h>
#include <Misra/Std/Utility/StrIter.h>

// ---------------------------------------------------------------------------
// Field parsers
// ---------------------------------------------------------------------------

static int hex_digit_value(char c) {
    if (c >= '0' && c <= '9')
        return c - '0';
    if (c >= 'a' && c <= 'f')
        return 10 + (c - 'a');
    if (c >= 'A' && c <= 'F')
        return 10 + (c - 'A');
    return -1;
}

// Parse a hex run at the cursor. Advances the iter past the digits.
// Returns false if no digits are consumed.
static bool parse_hex_u64(StrIter *si, u64 *out) {
    u64  v        = 0;
    int  consumed = 0;
    char c;
    while (StrIterPeek(si, &c)) {
        int d = hex_digit_value(c);
        if (d < 0)
            break;
        v = (v << 4) | (u64)d;
        StrIterMustNext(si);
        ++consumed;
    }
    if (!consumed)
        return false;
    *out = v;
    return true;
}

static bool expect_char(StrIter *si, char want) {
    char c;
    if (!StrIterPeek(si, &c) || c != want)
        return false;
    StrIterMustNext(si);
    return true;
}

static void skip_ws(StrIter *si) {
    char c;
    while (StrIterPeek(si, &c) && (c == ' ' || c == '\t')) {
        StrIterMustNext(si);
    }
}

// Read one "non-whitespace blob" (the dev/inode tokens). Just skip it.
static void skip_token(StrIter *si) {
    char c;
    while (StrIterPeek(si, &c) && c != ' ' && c != '\t' && c != '\n') {
        StrIterMustNext(si);
    }
}

// ---------------------------------------------------------------------------
// Line decode
// ---------------------------------------------------------------------------

// Parses one `/proc/self/maps` line from `si`. On success the path is
// NUL-terminated in place (the trailing '\n' becomes '\0'), and
// `out->path` aliases the iter's underlying buffer. The iter is
// advanced past the line terminator so the next call resumes at the
// next line.
static bool parse_one_line(StrIter *si, ProcMapEntry *out) {
    u64 start = 0, ende = 0, offset = 0;
    if (!parse_hex_u64(si, &start))
        return false;
    if (!expect_char(si, '-'))
        return false;
    if (!parse_hex_u64(si, &ende))
        return false;
    skip_ws(si);

    // perms: 4 chars
    if (StrIterRemainingLength(si) < 4)
        return false;
    u32  perms = 0;
    char p0 = 0, p1 = 0, p2 = 0, p3 = 0;
    StrIterMustPeekAt(si, 0, &p0);
    StrIterMustPeekAt(si, 1, &p1);
    StrIterMustPeekAt(si, 2, &p2);
    StrIterMustPeekAt(si, 3, &p3);
    if (p0 == 'r')
        perms |= PROC_MAP_PERM_READ;
    if (p1 == 'w')
        perms |= PROC_MAP_PERM_WRITE;
    if (p2 == 'x')
        perms |= PROC_MAP_PERM_EXEC;
    if (p3 == 'p')
        perms |= PROC_MAP_PERM_PRIVATE;
    StrIterMustMove(si, 4);
    skip_ws(si);

    if (!parse_hex_u64(si, &offset))
        return false;
    skip_ws(si);

    // dev_major:dev_minor — we don't care, but the field must be there.
    skip_token(si);
    skip_ws(si);

    // inode — skip.
    skip_token(si);
    skip_ws(si);

    // path — optional, runs to end-of-line. We replace the newline
    // with \0 in place so the path is a usable C string aliasing the
    // iter's backing buffer.
    size path_start_pos = StrIterIndex(si);
    char c;
    while (StrIterPeek(si, &c) && c != '\n') {
        StrIterMustNext(si);
    }
    size line_terminator_pos = StrIterIndex(si);

    out->start       = start;
    out->end         = ende;
    out->perms       = perms;
    out->file_offset = offset;
    out->path        = (Zstr)StrIterDataAt(si, path_start_pos); // may be empty if anonymous

    if (line_terminator_pos < StrIterLength(si) && *StrIterDataAt(si, line_terminator_pos) == '\n') {
        // intentional bypass: in-place mutation of the iter's backing
        // buffer to NUL-terminate the path slice we just exposed via
        // `out->path`. Iter accessors are read-only; no public mutator
        // covers single-byte writes to the underlying storage.
        *StrIterDataAt(si, line_terminator_pos) = '\0';
        StrIterMustNext(si);
    }
    return true;
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

bool proc_maps_load(ProcMaps *out, Allocator *alloc) {
    if (!out || !alloc) {
        LOG_FATAL("ProcMapsLoad: NULL argument");
    }
    MemSet(out, 0, sizeof(*out));
    out->raw     = StrInit(alloc);
    out->entries = VecInitT(out->entries, alloc);

    // `/proc/self/maps` reports stat-size 0 because it's generated by
    // the kernel on read. Stat-driven readers would short-circuit to
    // empty -- we have to loop-read into a growing buffer ourselves.
    File f = FileOpen("/proc/self/maps", "rb");
    if (!FileIsOpen(&f)) {
        LOG_ERROR("ProcMapsLoad: FileOpen(/proc/self/maps) failed");
        ProcMapsDeinit(out);
        return false;
    }

    enum {
        CHUNK = 4096
    };
    while (true) {
        u64 grown_to = StrLen(&out->raw) + CHUNK + 1;
        if (!StrReserve(&out->raw, grown_to)) {
            LOG_ERROR("ProcMapsLoad: failed to grow buffer");
            FileClose(&f);
            ProcMapsDeinit(out);
            return false;
        }
        i64 n = FileRead(&f, StrEnd(&out->raw), CHUNK);
        if (n < 0) {
            LOG_ERROR("ProcMapsLoad: FileRead failed");
            FileClose(&f);
            ProcMapsDeinit(out);
            return false;
        }
        StrResize(&out->raw, StrLen(&out->raw) + (u64)n);
        if (n == 0)
            break; // true EOF. NOT `n < CHUNK`: /proc seq_files pack records
                   // into their internal buffer and stop at a record boundary
                   // below the requested size, so a short read happens
                   // mid-file (any maps over ~4 KiB), not just at the end.
                   // Treating a short read as EOF drops every later entry
                   // (including [stack]).
    }
    FileClose(&f);
    if (StrLen(&out->raw) == 0) {
        LOG_ERROR("ProcMapsLoad: /proc/self/maps was empty");
        ProcMapsDeinit(out);
        return false;
    }
    // `StrResize` already writes a NUL sentinel at index `length`,
    // so path slices that alias the buffer are usable as `Zstr`.
    StrIter si = StrIterFromStr(out->raw);
    while (StrIterRemainingLength(&si)) {
        ProcMapEntry e = {0};
        if (!parse_one_line(&si, &e)) {
            // Skip past whatever line we couldn't parse.
            char c;
            while (StrIterPeek(&si, &c) && c != '\n') {
                StrIterMustNext(&si);
            }
            if (StrIterRemainingLength(&si)) {
                StrIterMustNext(&si);
            }
            continue;
        }
        if (!VecPushBackR(&out->entries, e)) {
            ProcMapsDeinit(out);
            return false;
        }
    }

    // Cache the lowest mapped address so callers don't rescan the vector.
    for (u64 i = 0; i < VecLen(&out->entries); ++i) {
        const ProcMapEntry *e = VecPtrAt(&out->entries, i);
        if (i == 0 || e->start < out->min_addr)
            out->min_addr = e->start;
    }

    return true;
}

void ProcMapsDeinit(ProcMaps *self) {
    if (!self)
        return;
    StrDeinit(&self->raw);
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
