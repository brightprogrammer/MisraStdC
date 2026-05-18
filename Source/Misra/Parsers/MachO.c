/// file      : MachO.c
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Mach-O parser implementation. The on-disk format is well-documented
/// in `<mach-o/loader.h>`; the relevant subset is reproduced inline as
/// enum constants so we don't need any system headers.

#include <Misra/Parsers/MachO.h>
#include <Misra/Std.h>
#include <Misra/Std/File.h>
#include <Misra/Std/Log.h>
#include <Misra/Std/Memory.h>

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

// ---------------------------------------------------------------------------
// Byte readers (little-endian)
// ---------------------------------------------------------------------------

static u32 ru32(const u8 *p) {
    return (u32)p[0] | (u32)p[1] << 8 | (u32)p[2] << 16 | (u32)p[3] << 24;
}
static u64 ru64(const u8 *p) {
    u64 v = 0;
    for (int i = 0; i < 8; ++i)
        v |= (u64)p[i] << (i * 8);
    return v;
}

static void copy_fixed16(char *dst, const u8 *src) {
    // Mach-O segment / section names are 16 bytes, NUL-padded but
    // need not be NUL-terminated. We copy then forcibly NUL-terminate.
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
    u32 magic = ru32(m->data);
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
    m->cputype      = ru32(m->data + 4);
    m->filetype     = (MachoFileType)ru32(m->data + 12);
    ctx->ncmds      = ru32(m->data + 16);
    ctx->sizeofcmds = ru32(m->data + 20);
    // ru32(m->data + 24) = flags; ru32(m->data + 28) = reserved.
    return true;
}

static bool decode_segment_64(MachoContext *ctx, const u8 *cmd_p, u32 cmdsize) {
    if (cmdsize < SEG64_CMD_SIZE_MIN) {
        LOG_ERROR("MachO: LC_SEGMENT_64 truncated");
        return false;
    }
    MachoSegment seg;
    MemSet(&seg, 0, sizeof(seg));
    copy_fixed16(seg.name, cmd_p + 8);
    seg.vmaddr   = ru64(cmd_p + 24);
    seg.vmsize   = ru64(cmd_p + 32);
    seg.fileoff  = ru64(cmd_p + 40);
    seg.filesize = ru64(cmd_p + 48);
    // maxprot (4) + initprot (4) at offset 56..63.
    seg.nsects = ru32(cmd_p + 64);
    seg.flags  = ru32(cmd_p + 68);
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
        sec.addr   = ru64(s + 32);
        sec.size   = ru64(s + 40);
        sec.offset = ru32(s + 48);
        // align (4) + reloff (4) + nreloc (4) at 52..63.
        sec.flags = ru32(s + 64);
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
    ctx->symoff      = ru32(cmd_p + 8);
    ctx->nsyms       = ru32(cmd_p + 12);
    ctx->stroff      = ru32(cmd_p + 16);
    ctx->strsize     = ru32(cmd_p + 20);
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
        const u8 *cmd_p   = ctx->out->data + cur;
        u32       cmd     = ru32(cmd_p + 0);
        u32       cmdsize = ru32(cmd_p + 4);
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
    for (u32 i = 0; i < ctx->nsyms; ++i) {
        const u8 *e      = ctx->out->data + ctx->symoff + (u64)i * NLIST64_SIZE;
        u32       n_strx = ru32(e + 0);
        u8        n_type = e[4];
        u8        n_sect = e[5];
        // u16 n_desc = ru16(e + 6);
        u64 n_value = ru64(e + 8);
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

bool macho_file_open_from_memory(MachoFile *out, u8 *data, size data_size, Allocator *alloc) {
    if (!out || !data || !alloc) {
        LOG_ERROR("MachoFileOpenFromMemory: NULL argument");
        return false;
    }
    MemSet(out, 0, sizeof(*out));
    out->allocator = alloc;
    out->data      = data;
    out->data_size = data_size;
    out->owns_data = false;
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

bool macho_file_open(MachoFile *out, const char *path, Allocator *alloc) {
    if (!out || !path || !alloc) {
        LOG_ERROR("MachoFileOpen: NULL argument");
        return false;
    }
    char *buf      = NULL;
    u64   bytes    = 0;
    u64   capacity = 0;
    if (!ReadCompleteFile(path, &buf, &bytes, &capacity, alloc)) {
        LOG_ERROR("MachoFileOpen: failed to read {}", path);
        return false;
    }
    if (!MachoFileOpenFromMemory(out, (u8 *)buf, (size)bytes, alloc)) {
        AllocatorFree(alloc, buf);
        return false;
    }
    out->owns_data = true;
    out->data      = (u8 *)buf;
    out->data_size = (size)bytes;
    (void)capacity;
    return true;
}

void MachoFileDeinit(MachoFile *self) {
    if (!self)
        return;
    if (self->owns_data && self->data && self->allocator) {
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
