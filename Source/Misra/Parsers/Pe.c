/// file      : Pe.c
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// PE/COFF parser implementation. Walks the headers in the order the
/// loader does: DOS stub -> NT signature -> File Header -> Optional
/// Header -> Data Directories -> Section Headers. Then scans the Debug
/// Directory for a CodeView (RSDS) entry and decodes (GUID, age,
/// PDB path) from it.
///
/// Spec references (everything is well-documented):
///   - "PE Format" on Microsoft Learn
///   - `<winnt.h>` IMAGE_* structure definitions
///   - LLVM's `Object/COFF.h` for the reader-side conventions

#include <Misra/Parsers/ByteIter.h>
#include <Misra/Parsers/Pe.h>

// ---------------------------------------------------------------------------
// On-disk record layouts (LE)
// ---------------------------------------------------------------------------

// IMAGE_FILE_HEADER (20 bytes), without the leading 4-byte PE\0\0
// signature -- the caller consumes that first.
#define FMT_PE_FILE_HEADER_LE                                                                                          \
    "{<2r}" /* machine                  */                                                                             \
    "{<2r}" /* number_of_sections       */                                                                             \
    "{<4r}" /* time_date_stamp          */                                                                             \
    "{<4r}" /* pointer_to_symbol_table  */                                                                             \
    "{<4r}" /* number_of_symbols        */                                                                             \
    "{<2r}" /* size_of_optional_header  */                                                                             \
    "{<2r}" /* characteristics          */

// IMAGE_SECTION_HEADER (40 bytes). Name is 8 chars; we read it as
// separate u8s via {<1r} so the format strings stays one record-shape
// per directive.
#define FMT_PE_SECTION_HEADER_LE                                                                                       \
    /* name[8] handled by caller via MemCopy */                                                                        \
    "{<4r}" /* virtual_size       */                                                                                   \
    "{<4r}" /* virtual_address    */                                                                                   \
    "{<4r}" /* raw_size           */                                                                                   \
    "{<4r}" /* raw_offset         */                                                                                   \
    "{<4r}" /* ptr_to_relocations */                                                                                   \
    "{<4r}" /* ptr_to_linenumbers */                                                                                   \
    "{<2r}" /* num_relocations    */                                                                                   \
    "{<2r}" /* num_linenumbers    */                                                                                   \
    "{<4r}" /* characteristics    */

// IMAGE_DEBUG_DIRECTORY (28 bytes).
#define FMT_PE_DEBUG_DIR_LE                                                                                            \
    "{<4r}" /* characteristics  */                                                                                     \
    "{<4r}" /* timestamp        */                                                                                     \
    "{<2r}" /* major_version    */                                                                                     \
    "{<2r}" /* minor_version    */                                                                                     \
    "{<4r}" /* type             */                                                                                     \
    "{<4r}" /* size_of_data     */                                                                                     \
    "{<4r}" /* address_of_data  */                                                                                     \
    "{<4r}" /* pointer_to_data  */
#include <Misra/Std.h>
#include <Misra/Std/File.h>
#include <Misra/Std/Log.h>
#include <Misra/Std/Memory.h>

// ---------------------------------------------------------------------------
// Spec constants
// ---------------------------------------------------------------------------

enum {
    DOS_MAGIC               = 0x5A4D,     // 'MZ'
    DOS_E_LFANEW_OFFSET     = 0x3C,
    NT_SIGNATURE            = 0x00004550, // 'PE\0\0'
    OPTIONAL_MAGIC_PE32     = 0x10B,
    OPTIONAL_MAGIC_PE32PLUS = 0x20B,
};

enum {
    DIR_INDEX_DEBUG = 6,
};

enum {
    IMAGE_DEBUG_TYPE_CODEVIEW = 2,
};

enum {
    CV_SIGNATURE_RSDS = 0x53445352, // 'RSDS' (little-endian read of 'R','S','D','S')
};

// ---------------------------------------------------------------------------
// Decoders
// ---------------------------------------------------------------------------

typedef struct PeContext {
    PeFile  *out;
    ByteIter file;      // bounds for the whole image
    u32      nt_offset; // offset of NT signature
    u16      num_sections;
    u16      opt_hdr_size;
    u32      num_dirs;
    u64      debug_dir_rva;
    u32      debug_dir_size;
} PeContext;

// DOS header gives us e_lfanew, the offset to the NT headers.
static bool pe_decode_dos(PeContext *ctx) {
    if (ctx->out->data_size < 64) {
        LOG_ERROR("PE: file too small for DOS header");
        return false;
    }
    u16 mz = (u16)ctx->out->data[0] | (u16)ctx->out->data[1] << 8;
    if (mz != DOS_MAGIC) {
        LOG_ERROR("PE: bad DOS magic 0x{x}", (u32)mz);
        return false;
    }
    u32      e_lfanew;
    ByteIter c = ByteIterFromMemory(ctx->out->data + DOS_E_LFANEW_OFFSET, ctx->out->data_size - DOS_E_LFANEW_OFFSET);
    if (!bi_take_u32_le(&c, &e_lfanew))
        return false;
    if (e_lfanew >= ctx->out->data_size) {
        LOG_ERROR("PE: e_lfanew past EOF");
        return false;
    }
    ctx->nt_offset = e_lfanew;
    return true;
}

// NT signature + File Header. Returns the offset of the Optional Header.
static bool pe_decode_nt(PeContext *ctx, u64 *out_opt_offset) {
    ByteIter c = ByteIterFromMemory(ctx->out->data + ctx->nt_offset, ctx->out->data_size - ctx->nt_offset);
    u32      sig;
    if (!bi_take_u32_le(&c, &sig) || sig != NT_SIGNATURE) {
        LOG_ERROR("PE: bad NT signature");
        return false;
    }
    u16 machine, num_sec, size_opt, chars;
    u32 timestamp, sym_ptr, num_sym;
    if (!ByteIterReadFmt(
            &c, FMT_PE_FILE_HEADER_LE, machine, num_sec, timestamp, sym_ptr, num_sym, size_opt, chars
        )) {
        LOG_ERROR("PE: file header truncated");
        return false;
    }
    (void)timestamp;
    (void)sym_ptr;
    (void)num_sym;
    (void)chars;
    ctx->out->machine = (PeMachine)machine;
    ctx->num_sections = num_sec;
    ctx->opt_hdr_size = size_opt;
    *out_opt_offset   = (u64)(c.data + c.pos - ctx->out->data);
    return true;
}

// Optional Header. We need ImageBase, SizeOfImage,
// NumberOfRvaAndSizes, and the DataDirectory[DEBUG] entry.
static bool pe_decode_optional(PeContext *ctx, u64 opt_offset) {
    if (opt_offset > ctx->out->data_size || ctx->opt_hdr_size > ctx->out->data_size - opt_offset) {
        LOG_ERROR("PE: optional header overruns file");
        return false;
    }
    ByteIter c = ByteIterFromMemory(ctx->out->data + opt_offset, ctx->opt_hdr_size);

    u16 magic;
    if (!bi_take_u16_le(&c, &magic))
        return false;
    if (magic != OPTIONAL_MAGIC_PE32 && magic != OPTIONAL_MAGIC_PE32PLUS) {
        LOG_ERROR("PE: unsupported optional magic 0x{x}", (u32)magic);
        return false;
    }
    bool is64              = (magic == OPTIONAL_MAGIC_PE32PLUS);
    ctx->out->is_pe32_plus = is64;

    // Skip linker_major, linker_minor, size_of_code, size_of_init_data,
    // size_of_uninit_data, entry_point, base_of_code.
    if (!bi_skip(&c, 1 + 1 + 4 + 4 + 4 + 4 + 4))
        return false;
    if (!is64) {
        // PE32 has an extra base_of_data here.
        if (!bi_skip(&c, 4))
            return false;
    }

    // ImageBase: u32 on PE32, u64 on PE32+.
    if (is64) {
        if (!bi_take_u64_le(&c, &ctx->out->image_base))
            return false;
    } else {
        u32 base;
        if (!bi_take_u32_le(&c, &base))
            return false;
        ctx->out->image_base = base;
    }

    // Skip section_alignment, file_alignment, os_major, os_minor,
    // image_major, image_minor, subsys_major, subsys_minor,
    // win32_version.
    if (!bi_skip(&c, 4 + 4 + 2 + 2 + 2 + 2 + 2 + 2 + 4))
        return false;

    if (!bi_take_u32_le(&c, &ctx->out->size_of_image))
        return false;

    // Skip size_of_headers, checksum, subsystem, dll_chars.
    if (!bi_skip(&c, 4 + 4 + 2 + 2))
        return false;

    // Stack/heap sizes: 4 ea on PE32, 8 ea on PE32+. Four fields.
    if (!bi_skip(&c, is64 ? 32 : 16))
        return false;

    // LoaderFlags (u32), then NumberOfRvaAndSizes (u32).
    if (!bi_skip(&c, 4))
        return false;
    if (!bi_take_u32_le(&c, &ctx->num_dirs))
        return false;

    // Data Directories: 8 bytes each. We want index 6 (DEBUG).
    if (ctx->num_dirs <= DIR_INDEX_DEBUG) {
        // No debug directory present -- not fatal, but codeview info
        // stays empty.
        return true;
    }
    // Skip directories before DEBUG.
    if (!bi_skip(&c, DIR_INDEX_DEBUG * 8u))
        return false;
    if (!bi_take_u32_le(&c, (u32 *)&ctx->debug_dir_rva))
        return false;
    if (!bi_take_u32_le(&c, &ctx->debug_dir_size))
        return false;
    return true;
}

// Section headers immediately follow the Optional Header.
static bool pe_decode_sections(PeContext *ctx, u64 opt_offset) {
    u64      sec_offset = opt_offset + ctx->opt_hdr_size;
    ByteIter c          = ByteIterFromMemory(ctx->out->data + sec_offset, ctx->out->data_size - sec_offset);

    for (u32 i = 0; i < ctx->num_sections; ++i) {
        if (bi_remaining(&c) < 40) {
            LOG_ERROR("PE: section table truncated at index {}", i);
            return false;
        }
        PeSection s;
        // 8-byte name: bytes, not a numeric, so copy + advance manually.
        MemCopy(s.name, c.data + c.pos, 8);
        s.name[8] = '\0';
        c.pos    += 8;

        u32 ptr_relocs, ptr_linenums;
        u16 num_relocs, num_linenums;
        if (!ByteIterReadFmt(
                &c,
                FMT_PE_SECTION_HEADER_LE,
                s.virtual_size,
                s.virtual_address,
                s.raw_size,
                s.raw_offset,
                ptr_relocs,
                ptr_linenums,
                num_relocs,
                num_linenums,
                s.characteristics
            )) {
            LOG_ERROR("PE: section header {} truncated", i);
            return false;
        }
        (void)ptr_relocs;
        (void)ptr_linenums;
        (void)num_relocs;
        (void)num_linenums;

        if (!VecPushBackR(&ctx->out->sections, s))
            return false;
    }
    return true;
}

// Walk the Debug Directory looking for a CodeView entry. The
// directory is found via the RVA recorded earlier; we need to convert
// it to a file offset first.
static void pe_decode_codeview(PeContext *ctx) {
    PeCodeViewInfo *cv = &ctx->out->codeview;
    cv->present        = false;

    if (ctx->debug_dir_rva == 0 || ctx->debug_dir_size == 0)
        return;

    u64 dir_offset;
    if (!PeFileRvaToOffset(ctx->out, (u32)ctx->debug_dir_rva, &dir_offset)) {
        LOG_ERROR("PE: debug directory RVA not in any section");
        return;
    }

    enum {
        DEBUG_ENTRY_SIZE = 28,
    };
    u32 num_entries = ctx->debug_dir_size / DEBUG_ENTRY_SIZE;
    if (dir_offset + (u64)num_entries * DEBUG_ENTRY_SIZE > ctx->out->data_size) {
        LOG_ERROR("PE: debug directory overruns file");
        return;
    }

    for (u32 i = 0; i < num_entries; ++i) {
        u64      entry_off = dir_offset + (u64)i * DEBUG_ENTRY_SIZE;
        ByteIter c         = ByteIterFromMemory(ctx->out->data + entry_off, ctx->out->data_size - entry_off);
        u32      charac, ts, type, sz, raddr, rptr;
        u16      ver_maj, ver_min;
        if (!ByteIterReadFmt(
                &c, FMT_PE_DEBUG_DIR_LE, charac, ts, ver_maj, ver_min, type, sz, raddr, rptr
            ))
            return;
        (void)charac;
        (void)ts;
        (void)ver_maj;
        (void)ver_min;

        if (type != IMAGE_DEBUG_TYPE_CODEVIEW)
            continue;
        if (rptr + (u64)sz > ctx->out->data_size) {
            LOG_ERROR("PE: codeview record points outside file");
            continue;
        }
        // RSDS = 4-byte sig + 16-byte GUID + 4-byte age + cstring path.
        if (sz < 4 + 16 + 4 + 1)
            continue;
        ByteIter cv_cur = ByteIterFromMemory(ctx->out->data + rptr, sz);
        u32      cv_sig;
        if (!bi_take_u32_le(&cv_cur, &cv_sig))
            continue;
        if (cv_sig != CV_SIGNATURE_RSDS) {
            // NB7 (PDB 2.0) and earlier are no longer emitted by
            // toolchains we care about; treat them as unsupported.
            continue;
        }
        if (bi_remaining(&cv_cur) < 16 + 4)
            continue;
        MemCopy(cv->guid, cv_cur.data + cv_cur.pos, 16);
        cv_cur.pos += 16;
        if (!bi_take_u32_le(&cv_cur, &cv->age))
            continue;
        // Verify the trailing path is NUL-terminated inside the record.
        const u8 *path_start = cv_cur.data + cv_cur.pos;
        const u8 *region_end = cv_cur.data + cv_cur.length;
        bool      terminated = false;
        for (const u8 *p = path_start; p < region_end; ++p) {
            if (*p == '\0') {
                terminated = true;
                break;
            }
        }
        if (!terminated)
            continue;
        cv->pdb_path = (const char *)path_start;
        cv->present  = true;
        return; // first CodeView entry wins
    }
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

bool pe_file_open_from_memory(PeFile *out, u8 *data, size data_size, Allocator *alloc) {
    if (!out || !data || !alloc) {
        LOG_ERROR("PeFileOpenFromMemory: NULL argument");
        return false;
    }
    MemSet(out, 0, sizeof(*out));
    out->allocator = alloc;
    out->data      = data;
    out->data_size = data_size;
    out->owns_data = false;
    // Initialize the sections vec up-front so PeFileDeinit on a
    // failed-parse path doesn't trip ValidateVec.
    out->sections = VecInitT(out->sections, alloc);

    PeContext ctx = {
        .out  = out,
        .file = ByteIterFromMemory(data, data_size),
    };

    if (!pe_decode_dos(&ctx))
        goto fail;
    u64 opt_offset = 0;
    if (!pe_decode_nt(&ctx, &opt_offset))
        goto fail;
    if (!pe_decode_optional(&ctx, opt_offset))
        goto fail;
    if (!pe_decode_sections(&ctx, opt_offset))
        goto fail;
    pe_decode_codeview(&ctx); // codeview is best-effort
    return true;

fail:
    PeFileDeinit(out);
    return false;
}

bool pe_file_open(PeFile *out, const char *path, Allocator *alloc) {
    if (!out || !path || !alloc) {
        LOG_ERROR("PeFileOpen: NULL argument");
        return false;
    }
    char *buf      = NULL;
    u64   bytes    = 0;
    u64   capacity = 0;
    if (!ReadCompleteFile(path, &buf, &bytes, &capacity, alloc)) {
        LOG_ERROR("PeFileOpen: failed to read {}", path);
        return false;
    }
    if (!PeFileOpenFromMemory(out, (u8 *)buf, (size)bytes, alloc)) {
        AllocatorFree(alloc, buf);
        return false;
    }
    out->owns_data = true;
    out->data      = (u8 *)buf;
    out->data_size = (size)bytes;
    (void)capacity;
    return true;
}

void PeFileDeinit(PeFile *self) {
    if (!self)
        return;
    if (self->owns_data && self->data && self->allocator) {
        AllocatorFree(self->allocator, self->data);
    }
    VecDeinit(&self->sections);
    MemSet(self, 0, sizeof(*self));
}

const PeSection *PeFileFindSection(const PeFile *self, const char *name) {
    if (!self || !name)
        return NULL;
    for (size i = 0; i < self->sections.length; ++i) {
        if (ZstrCompare(self->sections.data[i].name, name) == 0) {
            return &self->sections.data[i];
        }
    }
    return NULL;
}

bool PeFileRvaToOffset(const PeFile *self, u32 rva, u64 *out_offset) {
    if (!self || !out_offset)
        return false;
    for (size i = 0; i < self->sections.length; ++i) {
        const PeSection *s = &self->sections.data[i];
        // Compute the section end in u64; u32 + u32 can wrap.
        u64 vstart = (u64)s->virtual_address;
        u64 vend   = vstart + (u64)s->virtual_size;
        if (rva >= vstart && (u64)rva < vend) {
            u64 off = (u64)s->raw_offset + ((u64)rva - vstart);
            if (off >= self->data_size)
                return false;
            *out_offset = off;
            return true;
        }
    }
    return false;
}
