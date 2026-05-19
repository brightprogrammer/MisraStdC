/// file      : MachO.c
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Mach-O parser implementation. The on-disk format is well-documented
/// in `<mach-o/loader.h>`; the relevant subset is reproduced inline as
/// enum constants so we don't need any system headers.

#include <Misra/Std/Container/Buf.h>
#include <Misra/Parsers/MachO.h>
#include <Misra/Std.h>
#include <Misra/Std/File.h>
#include <Misra/Std/Log.h>
#include <Misra/Std/Memory.h>

// ---------------------------------------------------------------------------
// On-disk record layouts (LE)
// ---------------------------------------------------------------------------

// mach_header_64 (32 bytes).
#define FMT_MACHO_HEADER_LE                                                                                            \
    "{<4r}" /* magic       */                                                                                          \
    "{<4r}" /* cputype     */                                                                                          \
    "{<4r}" /* cpusubtype  */                                                                                          \
    "{<4r}" /* filetype    */                                                                                          \
    "{<4r}" /* ncmds       */                                                                                          \
    "{<4r}" /* sizeofcmds  */                                                                                          \
    "{<4r}" /* flags       */                                                                                          \
    "{<4r}" /* reserved    */

// load_command prefix (cmd + cmdsize, 8 bytes).
#define FMT_MACHO_LC_PREFIX_LE                                                                                         \
    "{<4r}" /* cmd      */                                                                                             \
    "{<4r}" /* cmdsize  */

// segment_command_64 body after the 16-byte segname (48 bytes).
#define FMT_MACHO_SEGMENT64_BODY_LE                                                                                    \
    "{<8r}" /* vmaddr   */                                                                                             \
    "{<8r}" /* vmsize   */                                                                                             \
    "{<8r}" /* fileoff  */                                                                                             \
    "{<8r}" /* filesize */                                                                                             \
    "{<4r}" /* maxprot  */                                                                                             \
    "{<4r}" /* initprot */                                                                                             \
    "{<4r}" /* nsects   */                                                                                             \
    "{<4r}" /* flags    */

// section_64 body after sectname + segname (32 bytes consumed; 48 bytes here).
#define FMT_MACHO_SECTION64_BODY_LE                                                                                    \
    "{<8r}" /* addr      */                                                                                            \
    "{<8r}" /* size      */                                                                                            \
    "{<4r}" /* offset    */                                                                                            \
    "{<4r}" /* align     */                                                                                            \
    "{<4r}" /* reloff    */                                                                                            \
    "{<4r}" /* nreloc    */                                                                                            \
    "{<4r}" /* flags     */                                                                                            \
    "{<4r}" /* reserved1 */                                                                                            \
    "{<4r}" /* reserved2 */                                                                                            \
    "{<4r}" /* reserved3 */

// symtab_command body after the cmd/cmdsize prefix (16 bytes).
#define FMT_MACHO_SYMTAB_BODY_LE                                                                                       \
    "{<4r}" /* symoff   */                                                                                             \
    "{<4r}" /* nsyms    */                                                                                             \
    "{<4r}" /* stroff   */                                                                                             \
    "{<4r}" /* strsize  */

// nlist_64 record (16 bytes).
#define FMT_MACHO_NLIST64_LE                                                                                           \
    "{<4r}" /* n_strx   */                                                                                             \
    "{<1r}" /* n_type   */                                                                                             \
    "{<1r}" /* n_sect   */                                                                                             \
    "{<2r}" /* n_desc   */                                                                                             \
    "{<8r}" /* n_value  */

// ---------------------------------------------------------------------------
// Spec constants
// ---------------------------------------------------------------------------

enum {
    MH_MAGIC_64 = 0xFEEDFACFu, // 64-bit, host-endian
    MH_CIGAM_64 = 0xCFFAEDFEu, // 64-bit, byte-swapped
    MH_MAGIC_32 = 0xFEEDFACEu,
    MH_CIGAM_32 = 0xCEFAEDFEu,
    FAT_MAGIC   = 0xCAFEBABEu, // universal binaries (big-endian on disk)
    FAT_CIGAM   = 0xBEBAFECAu,

    LC_SEGMENT_64 = 0x19,
    LC_SYMTAB     = 0x02,
    LC_DYSYMTAB   = 0x0B,
    LC_UUID       = 0x1B,
    LC_REQ_DYLD   = 0x80000000u,
};

enum {
    MH_HEADER_64_SIZE  = 32,
    SEG64_CMD_SIZE_MIN = 72, // segment_command_64 header (sections follow)
    SECT64_SIZE        = 80,
    SYMTAB_CMD_SIZE    = 24,
    NLIST64_SIZE       = 16,
    UUID_CMD_SIZE      = 24,
};

// Names are 16 bytes, NUL-padded but not guaranteed NUL-terminated.
static void copy_fixed16(char *dst, const u8 *src) {
    MemCopy(dst, src, 16);
    dst[16] = '\0';
}

// ---------------------------------------------------------------------------
// Decoders
// ---------------------------------------------------------------------------

typedef struct MachoContext {
    MachoFile *out;
    u32        ncmds;
    u32        sizeofcmds;
    u32        symoff;
    u32        nsyms;
    u32        stroff;
    u32        strsize;
    bool       have_symtab;
} MachoContext;

static bool decode_header(MachoContext *ctx) {
    MachoFile *m = ctx->out;
    if (m->data_size < MH_HEADER_64_SIZE) {
        LOG_ERROR("MachO: file too small for header");
        return false;
    }
    BufIter c = BufIterFromMemory(m->data, m->data_size);
    u32     magic, cputype, cpusubtype, filetype, ncmds, sizeofcmds, flags, reserved;
    if (!BufReadFmt(
            &c,
            FMT_MACHO_HEADER_LE,
            magic,
            cputype,
            cpusubtype,
            filetype,
            ncmds,
            sizeofcmds,
            flags,
            reserved
        )) {
        LOG_ERROR("MachO: header truncated");
        return false;
    }
    (void)cpusubtype;
    (void)flags;
    (void)reserved;
    if (magic == FAT_MAGIC || magic == FAT_CIGAM) {
        LOG_ERROR("MachO: fat/universal binary not supported in v1 -- caller must pick a slice");
        return false;
    }
    if (magic == MH_MAGIC_32 || magic == MH_CIGAM_32) {
        LOG_ERROR("MachO: 32-bit Mach-O not supported in v1");
        return false;
    }
    if (magic == MH_CIGAM_64) {
        LOG_ERROR("MachO: byte-swapped 64-bit Mach-O not supported in v1");
        return false;
    }
    if (magic != MH_MAGIC_64) {
        LOG_ERROR("MachO: bad magic 0x{x}", magic);
        return false;
    }
    m->cputype      = cputype;
    m->filetype     = (MachoFileType)filetype;
    ctx->ncmds      = ncmds;
    ctx->sizeofcmds = sizeofcmds;
    return true;
}

static bool decode_segment_64(MachoContext *ctx, const u8 *cmd_p, u32 cmdsize) {
    if (cmdsize < SEG64_CMD_SIZE_MIN) {
        LOG_ERROR("MachO: LC_SEGMENT_64 truncated");
        return false;
    }
    MachoSegment seg;
    MemSet(&seg, 0, sizeof(seg));
    // cmd_p layout: cmd(4) cmdsize(4) segname[16] then FMT_MACHO_SEGMENT64_BODY_LE.
    copy_fixed16(seg.name, cmd_p + 8);
    BufIter c = BufIterFromMemory(cmd_p + 24, cmdsize - 24);
    u32     maxprot, initprot;
    if (!BufReadFmt(
            &c,
            FMT_MACHO_SEGMENT64_BODY_LE,
            seg.vmaddr,
            seg.vmsize,
            seg.fileoff,
            seg.filesize,
            maxprot,
            initprot,
            seg.nsects,
            seg.flags
        )) {
        LOG_ERROR("MachO: LC_SEGMENT_64 body truncated");
        return false;
    }
    (void)maxprot;
    (void)initprot;
    if (!VecPushBackR(&ctx->out->segments, seg))
        return false;

    // Section table follows the segment command header.
    u64 sect_off = SEG64_CMD_SIZE_MIN;
    for (u32 i = 0; i < seg.nsects; ++i) {
        if (sect_off + SECT64_SIZE > cmdsize) {
            LOG_ERROR("MachO: section table overruns LC_SEGMENT_64");
            return false;
        }
        const u8    *s = cmd_p + sect_off;
        MachoSection sec;
        MemSet(&sec, 0, sizeof(sec));
        copy_fixed16(sec.section, s + 0);
        copy_fixed16(sec.segment, s + 16);
        BufIter sc = BufIterFromMemory(s + 32, SECT64_SIZE - 32);
        u32     align, reloff, nreloc, reserved1, reserved2, reserved3;
        if (!BufReadFmt(
                &sc,
                FMT_MACHO_SECTION64_BODY_LE,
                sec.addr,
                sec.size,
                sec.offset,
                align,
                reloff,
                nreloc,
                sec.flags,
                reserved1,
                reserved2,
                reserved3
            )) {
            LOG_ERROR("MachO: section_64 body truncated");
            return false;
        }
        (void)align;
        (void)reloff;
        (void)nreloc;
        (void)reserved1;
        (void)reserved2;
        (void)reserved3;
        if (!VecPushBackR(&ctx->out->sections, sec))
            return false;
        sect_off += SECT64_SIZE;
    }
    return true;
}

static bool decode_symtab(MachoContext *ctx, const u8 *cmd_p, u32 cmdsize) {
    if (cmdsize < SYMTAB_CMD_SIZE) {
        LOG_ERROR("MachO: LC_SYMTAB truncated");
        return false;
    }
    BufIter c = BufIterFromMemory(cmd_p + 8, cmdsize - 8);
    if (!BufReadFmt(&c, FMT_MACHO_SYMTAB_BODY_LE, ctx->symoff, ctx->nsyms, ctx->stroff, ctx->strsize)) {
        LOG_ERROR("MachO: LC_SYMTAB body truncated");
        return false;
    }
    ctx->have_symtab = true;
    return true;
}

static bool decode_uuid(MachoContext *ctx, const u8 *cmd_p, u32 cmdsize) {
    if (cmdsize < UUID_CMD_SIZE) {
        LOG_ERROR("MachO: LC_UUID truncated");
        return false;
    }
    MemCopy(ctx->out->uuid, cmd_p + 8, 16);
    ctx->out->has_uuid = true;
    return true;
}

static bool walk_load_commands(MachoContext *ctx) {
    if ((u64)MH_HEADER_64_SIZE + ctx->sizeofcmds > ctx->out->data_size) {
        LOG_ERROR("MachO: load commands overrun file");
        return false;
    }
    u64 cur = MH_HEADER_64_SIZE;
    u64 end = MH_HEADER_64_SIZE + ctx->sizeofcmds;
    for (u32 i = 0; i < ctx->ncmds; ++i) {
        if (cur + 8 > end)
            return false;
        const u8 *cmd_p = ctx->out->data + cur;
        BufIter   pc    = BufIterFromMemory(cmd_p, end - cur);
        u32       cmd, cmdsize;
        if (!BufReadFmt(&pc, FMT_MACHO_LC_PREFIX_LE, cmd, cmdsize)) {
            LOG_ERROR("MachO: load command prefix truncated at {}", i);
            return false;
        }
        if (cmdsize < 8 || cur + cmdsize > end) {
            LOG_ERROR("MachO: bad cmdsize at load command {}", i);
            return false;
        }
        u32 type = cmd & ~LC_REQ_DYLD;
        switch (type) {
            case LC_SEGMENT_64 :
                if (!decode_segment_64(ctx, cmd_p, cmdsize))
                    return false;
                break;
            case LC_SYMTAB :
                if (!decode_symtab(ctx, cmd_p, cmdsize))
                    return false;
                break;
            case LC_UUID :
                if (!decode_uuid(ctx, cmd_p, cmdsize))
                    return false;
                break;
            default :
                break;
        }
        cur += cmdsize;
    }
    return true;
}

static bool decode_symbols(MachoContext *ctx) {
    if (!ctx->have_symtab || ctx->nsyms == 0)
        return true;
    // Sanity cap. `nsyms` is u32 from the LC_SYMTAB command; a crafted
    // file can declare ~4B symbols, each turning into a Vec push.
    // Real binaries don't approach this.
    enum {
        MACHO_MAX_SYMBOLS = 16u * 1024u * 1024u
    };
    if (ctx->nsyms > MACHO_MAX_SYMBOLS) {
        LOG_ERROR("MachO: nsyms {} exceeds sanity cap; refusing", (u64)ctx->nsyms);
        return false;
    }
    u64 tab_end = (u64)ctx->symoff + (u64)ctx->nsyms * NLIST64_SIZE;
    if (tab_end > ctx->out->data_size) {
        LOG_ERROR("MachO: symtab past EOF");
        return false;
    }
    u64 str_end = (u64)ctx->stroff + ctx->strsize;
    if (str_end > ctx->out->data_size) {
        LOG_ERROR("MachO: symbol strtab past EOF");
        return false;
    }
    const u8 *str_base = ctx->out->data + ctx->stroff;
    BufIter   tab      = BufIterFromMemory(ctx->out->data + ctx->symoff, (u64)ctx->nsyms * NLIST64_SIZE);
    for (u32 i = 0; i < ctx->nsyms; ++i) {
        u32 n_strx;
        u8  n_type, n_sect;
        u16 n_desc;
        u64 n_value;
        if (!BufReadFmt(&tab, FMT_MACHO_NLIST64_LE, n_strx, n_type, n_sect, n_desc, n_value)) {
            LOG_ERROR("MachO: nlist_64 truncated at index {}", i);
            return false;
        }
        (void)n_desc;
        if (n_strx >= ctx->strsize)
            continue; // bad index; skip
        MachoSymbol sym;
        sym.name          = (const char *)(str_base + n_strx);
        sym.value         = n_value;
        sym.type          = n_type;
        sym.section_index = n_sect;
        if (!VecPushBackR(&ctx->out->symbols, sym))
            return false;
    }
    return true;
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

// L-value form. `data` is `u8 **` -- ownership of the pointer moves
// from caller to parser. On exit `*data == NULL` (success or failure).
bool macho_file_open_from_memory(MachoFile *out, u8 **data, size data_size, Allocator *alloc) {
    if (!out || !data || !*data || !alloc) {
        LOG_FATAL("MachoFileOpenFromMemory: NULL argument (contract violation)");
    }
    u8 *taken = *data;
    *data     = NULL;

    MemSet(out, 0, sizeof(*out));
    out->allocator = alloc;
    out->data      = taken;
    out->data_size = data_size;
    out->segments  = VecInitT(out->segments, alloc);
    out->sections  = VecInitT(out->sections, alloc);
    out->symbols   = VecInitT(out->symbols, alloc);

    MachoContext ctx = {.out = out};
    if (!decode_header(&ctx))
        goto fail;
    if (!walk_load_commands(&ctx))
        goto fail;
    if (!decode_symbols(&ctx))
        goto fail;
    return true;

fail:
    MachoFileDeinit(out);
    return false;
}

// R-value form: allocate, copy, hand `&copy` to the L-form.
bool macho_file_open_from_memory_copy(MachoFile *out, const u8 *data, size data_size, Allocator *alloc) {
    if (!out || !data || !alloc) {
        LOG_FATAL("MachoFileOpenFromMemoryCopy: NULL argument (contract violation)");
    }
    u8 *copy = (u8 *)AllocatorAlloc(alloc, data_size, false);
    if (!copy) {
        LOG_ERROR("MachoFileOpenFromMemoryCopy: allocation failed ({} bytes)", (u64)data_size);
        return false;
    }
    MemCopy(copy, data, data_size);
    return macho_file_open_from_memory(out, &copy, data_size, alloc);
}

bool macho_file_open(MachoFile *out, const char *path, Allocator *alloc) {
    if (!out || !path || !alloc) {
        LOG_FATAL("MachoFileOpen: NULL argument (contract violation)");
    }
    File f = FileOpen(path, "rb");
    if (!FileIsValid(&f)) {
        LOG_ERROR("MachoFileOpen: failed to open {}", path);
        return false;
    }
    Str data = StrInit(alloc);
    i64 got  = FileRead(&f, &data);
    FileClose(&f);
    if (got < 0) {
        StrDeinit(&data);
        LOG_ERROR("MachoFileOpen: failed to read {}", path);
        return false;
    }
    u8  *buf       = (u8 *)data.data;
    size buf_n     = data.length;
    data.data      = NULL;
    data.length    = 0;
    data.capacity  = 0;
    data.allocator = NULL;
    return macho_file_open_from_memory(out, &buf, buf_n, alloc);
}

void MachoFileDeinit(MachoFile *self) {
    if (!self)
        return;
    if (self->data && self->allocator) {
        AllocatorFree(self->allocator, self->data);
    }
    VecDeinit(&self->segments);
    VecDeinit(&self->sections);
    VecDeinit(&self->symbols);
    MemSet(self, 0, sizeof(*self));
}

const MachoSection *MachoFileFindSection(const MachoFile *self, const char *segment, const char *section) {
    if (!self || !segment || !section)
        return NULL;
    for (size i = 0; i < self->sections.length; ++i) {
        const MachoSection *s = &self->sections.data[i];
        if (ZstrCompare(s->segment, segment) == 0 && ZstrCompare(s->section, section) == 0) {
            return s;
        }
    }
    return NULL;
}

// Mach-O nlist_64 entries carry no size. Pick the symbol with the
// largest `value <= vaddr`, then bound it by the next symbol in the
// same section (or the section end). N_STAB entries are skipped:
// stab iff any of the high three bits of n_type is set.
const MachoSymbol *MachoFileResolveAddress(const MachoFile *self, u64 vaddr) {
    if (!self || self->symbols.length == 0)
        return NULL;
    enum {
        N_STAB_MASK = 0xE0
    };

    const MachoSymbol *best       = NULL;
    u64                best_value = 0;
    const MachoSymbol *next_above = NULL;
    u64                next_value = (u64)-1;

    for (size i = 0; i < self->symbols.length; ++i) {
        const MachoSymbol *s = &self->symbols.data[i];
        if (s->type & N_STAB_MASK)
            continue; // any high bit set => STAB (debug) entry
        if (s->section_index == 0)
            continue; // NO_SECT (absolute / external)
        if (s->value <= vaddr) {
            if (!best || s->value >= best_value) {
                best       = s;
                best_value = s->value;
            }
        } else if (s->value < next_value) {
            next_above = s;
            next_value = s->value;
        }
    }
    if (!best)
        return NULL;
    // Bound the match by the next symbol (best-effort size).
    if (next_above && vaddr >= next_value)
        return NULL;
    return best;
}
