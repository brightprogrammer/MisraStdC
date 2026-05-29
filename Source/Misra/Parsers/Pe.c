/// file      : parsers/pe.c
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

#include <Misra/Parsers/Pe.h>
#include <Misra/Std.h>
#include <Misra/Std/Container/Buf.h>
#include <Misra/Std/File.h>
#include <Misra/Std/Log.h>
#include <Misra/Std/Memory.h>

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

// IMAGE_OPTIONAL_HEADER, two variants. The caller consumes the
// 2-byte `magic` first (it picks which body to read), so these macros
// cover the remaining 94 (PE32) / 110 (PE32+) bytes through
// NumberOfRvaAndSizes. The two macros differ only where the spec
// differs: base_of_data exists in PE32 but not PE32+, image_base is
// u32 vs u64, and the four stack/heap reserve/commit fields are u32
// vs u64.
#define FMT_PE_OPT_HDR_PE32_LE                                                                                         \
    "{<1r}{<1r}" /* linker_major, linker_minor */                                                                      \
    "{<4r}"      /* size_of_code               */                                                                      \
    "{<4r}"      /* size_of_init_data          */                                                                      \
    "{<4r}"      /* size_of_uninit_data        */                                                                      \
    "{<4r}"      /* entry_point                */                                                                      \
    "{<4r}"      /* base_of_code               */                                                                      \
    "{<4r}"      /* base_of_data (PE32 only)   */                                                                      \
    "{<4r}"      /* image_base                 */                                                                      \
    "{<4r}"      /* section_alignment          */                                                                      \
    "{<4r}"      /* file_alignment             */                                                                      \
    "{<2r}{<2r}" /* os_major, os_minor         */                                                                      \
    "{<2r}{<2r}" /* image_major, image_minor   */                                                                      \
    "{<2r}{<2r}" /* subsys_major, subsys_minor */                                                                      \
    "{<4r}"      /* win32_version              */                                                                      \
    "{<4r}"      /* size_of_image              */                                                                      \
    "{<4r}"      /* size_of_headers            */                                                                      \
    "{<4r}"      /* checksum                   */                                                                      \
    "{<2r}{<2r}" /* subsystem, dll_chars       */                                                                      \
    "{<4r}{<4r}" /* stack_reserve, stack_commit */                                                                     \
    "{<4r}{<4r}" /* heap_reserve, heap_commit   */                                                                     \
    "{<4r}"      /* loader_flags               */                                                                      \
    "{<4r}"      /* number_of_rva_and_sizes    */

#define FMT_PE_OPT_HDR_PE32PLUS_LE                                                                                     \
    "{<1r}{<1r}" /* linker_major, linker_minor */                                                                      \
    "{<4r}"      /* size_of_code               */                                                                      \
    "{<4r}"      /* size_of_init_data          */                                                                      \
    "{<4r}"      /* size_of_uninit_data        */                                                                      \
    "{<4r}"      /* entry_point                */                                                                      \
    "{<4r}"      /* base_of_code               */                                                                      \
    "{<8r}"      /* image_base (u64)           */                                                                      \
    "{<4r}"      /* section_alignment          */                                                                      \
    "{<4r}"      /* file_alignment             */                                                                      \
    "{<2r}{<2r}" /* os_major, os_minor         */                                                                      \
    "{<2r}{<2r}" /* image_major, image_minor   */                                                                      \
    "{<2r}{<2r}" /* subsys_major, subsys_minor */                                                                      \
    "{<4r}"      /* win32_version              */                                                                      \
    "{<4r}"      /* size_of_image              */                                                                      \
    "{<4r}"      /* size_of_headers            */                                                                      \
    "{<4r}"      /* checksum                   */                                                                      \
    "{<2r}{<2r}" /* subsystem, dll_chars       */                                                                      \
    "{<8r}{<8r}" /* stack_reserve, stack_commit */                                                                     \
    "{<8r}{<8r}" /* heap_reserve, heap_commit   */                                                                     \
    "{<4r}"      /* loader_flags               */                                                                      \
    "{<4r}"      /* number_of_rva_and_sizes    */

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
    Pe     *out;
    BufIter file;      // bounds for the whole image
    u32     nt_offset; // offset of NT signature
    u16     num_sections;
    u16     opt_hdr_size;
    u32     num_dirs;
    u64     debug_dir_rva;
    u32     debug_dir_size;
} PeContext;

// DOS header gives us e_lfanew, the offset to the NT headers.
static bool pe_decode_dos(PeContext *ctx) {
    if (BufLength(&ctx->out->data) < 64) {
        LOG_ERROR("PE: file too small for DOS header");
        return false;
    }
    BufIter mz_iter = BufIterFromMemory(BufData(&ctx->out->data), 2);
    u16     mz;
    if (!BufReadU16LE(&mz_iter, &mz)) {
        return false;
    }
    if (mz != DOS_MAGIC) {
        LOG_ERROR("PE: bad DOS magic 0x{x}", (u32)mz);
        return false;
    }
    u32     e_lfanew;
    BufIter c = BufIterFromMemory(
        BufData(&ctx->out->data) + DOS_E_LFANEW_OFFSET,
        BufLength(&ctx->out->data) - DOS_E_LFANEW_OFFSET
    );
    if (!BufReadU32LE(&c, &e_lfanew))
        return false;
    if (e_lfanew >= BufLength(&ctx->out->data)) {
        LOG_ERROR("PE: e_lfanew past EOF");
        return false;
    }
    ctx->nt_offset = e_lfanew;
    return true;
}

// NT signature + File Header. Returns the offset of the Optional Header.
static bool pe_decode_nt(PeContext *ctx, u64 *out_opt_offset) {
    BufIter c =
        BufIterFromMemory(BufData(&ctx->out->data) + ctx->nt_offset, BufLength(&ctx->out->data) - ctx->nt_offset);
    u32 sig;
    if (!BufReadU32LE(&c, &sig) || sig != NT_SIGNATURE) {
        LOG_ERROR("PE: bad NT signature");
        return false;
    }
    u16 machine, num_sec, size_opt, chars;
    u32 timestamp, sym_ptr, num_sym;
    if (!BufReadFmt(&c, FMT_PE_FILE_HEADER_LE, machine, num_sec, timestamp, sym_ptr, num_sym, size_opt, chars)) {
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
    *out_opt_offset   = (u64)(IterDataAt(&c, IterIndex(&c)) - BufData(&ctx->out->data));
    return true;
}

// Optional Header. We need ImageBase, SizeOfImage,
// NumberOfRvaAndSizes, and the DataDirectory[DEBUG] entry.
static bool pe_decode_optional(PeContext *ctx, u64 opt_offset) {
    if (opt_offset > BufLength(&ctx->out->data) || ctx->opt_hdr_size > BufLength(&ctx->out->data) - opt_offset) {
        LOG_ERROR("PE: optional header overruns file");
        return false;
    }
    BufIter c = BufIterFromMemory(BufData(&ctx->out->data) + opt_offset, ctx->opt_hdr_size);

    u16 magic;
    if (!BufReadU16LE(&c, &magic))
        return false;
    if (magic != OPTIONAL_MAGIC_PE32 && magic != OPTIONAL_MAGIC_PE32PLUS) {
        LOG_ERROR("PE: unsupported optional magic 0x{x}", (u32)magic);
        return false;
    }
    bool is64              = (magic == OPTIONAL_MAGIC_PE32PLUS);
    ctx->out->is_pe32_plus = is64;

    // Shared discards across both variants. Only image_base,
    // size_of_image, and num_dirs are kept; everything else is read
    // for layout correctness and discarded.
    u8  linker_major, linker_minor;
    u32 size_of_code, size_of_init, size_of_uninit;
    u32 entry_point, base_of_code;
    u32 section_alignment, file_alignment;
    u16 os_major, os_minor, image_major, image_minor, subsys_major, subsys_minor;
    u16 subsystem, dll_chars;
    u32 win32_version, size_of_headers, checksum, loader_flags;

    if (is64) {
        u64 stack_res, stack_com, heap_res, heap_com;
        if (!BufReadFmt(
                &c,
                FMT_PE_OPT_HDR_PE32PLUS_LE,
                linker_major,
                linker_minor,
                size_of_code,
                size_of_init,
                size_of_uninit,
                entry_point,
                base_of_code,
                ctx->out->image_base,
                section_alignment,
                file_alignment,
                os_major,
                os_minor,
                image_major,
                image_minor,
                subsys_major,
                subsys_minor,
                win32_version,
                ctx->out->size_of_image,
                size_of_headers,
                checksum,
                subsystem,
                dll_chars,
                stack_res,
                stack_com,
                heap_res,
                heap_com,
                loader_flags,
                ctx->num_dirs
            )) {
            LOG_ERROR("PE: optional (PE32+) header truncated");
            return false;
        }
        (void)stack_res;
        (void)stack_com;
        (void)heap_res;
        (void)heap_com;
    } else {
        u32 base_of_data, image_base32, stack_res, stack_com, heap_res, heap_com;
        if (!BufReadFmt(
                &c,
                FMT_PE_OPT_HDR_PE32_LE,
                linker_major,
                linker_minor,
                size_of_code,
                size_of_init,
                size_of_uninit,
                entry_point,
                base_of_code,
                base_of_data,
                image_base32,
                section_alignment,
                file_alignment,
                os_major,
                os_minor,
                image_major,
                image_minor,
                subsys_major,
                subsys_minor,
                win32_version,
                ctx->out->size_of_image,
                size_of_headers,
                checksum,
                subsystem,
                dll_chars,
                stack_res,
                stack_com,
                heap_res,
                heap_com,
                loader_flags,
                ctx->num_dirs
            )) {
            LOG_ERROR("PE: optional (PE32) header truncated");
            return false;
        }
        ctx->out->image_base = image_base32;
        (void)base_of_data;
        (void)stack_res;
        (void)stack_com;
        (void)heap_res;
        (void)heap_com;
    }

    (void)linker_major;
    (void)linker_minor;
    (void)size_of_code;
    (void)size_of_init;
    (void)size_of_uninit;
    (void)entry_point;
    (void)base_of_code;
    (void)section_alignment;
    (void)file_alignment;
    (void)os_major;
    (void)os_minor;
    (void)image_major;
    (void)image_minor;
    (void)subsys_major;
    (void)subsys_minor;
    (void)win32_version;
    (void)size_of_headers;
    (void)checksum;
    (void)subsystem;
    (void)dll_chars;
    (void)loader_flags;

    // Data Directories: 8 bytes each. We want index 6 (DEBUG).
    if (ctx->num_dirs <= DIR_INDEX_DEBUG) {
        // No debug directory present -- not fatal, but codeview info
        // stays empty.
        return true;
    }
    if (!IterMove(&c, (i64)(DIR_INDEX_DEBUG * 8u)))
        return false;
    // Read into a local u32, then widen -- aliasing a u64* through a
    // u32* and writing only the low 4 bytes is a strict-aliasing
    // violation and would also corrupt the high half on big-endian
    // hosts.
    u32 debug_rva = 0;
    if (!BufReadU32LE(&c, &debug_rva))
        return false;
    if (!BufReadU32LE(&c, &ctx->debug_dir_size))
        return false;
    ctx->debug_dir_rva = (u64)debug_rva;
    return true;
}

// Section headers immediately follow the Optional Header.
static bool pe_decode_sections(PeContext *ctx, u64 opt_offset) {
    u64     sec_offset = opt_offset + ctx->opt_hdr_size;
    BufIter c = BufIterFromMemory(BufData(&ctx->out->data) + sec_offset, BufLength(&ctx->out->data) - sec_offset);

    for (u32 i = 0; i < ctx->num_sections; ++i) {
        if (IterRemainingLength(&c) < 40) {
            LOG_ERROR("PE: section table truncated at index {}", i);
            return false;
        }
        PeSection s;
        // 8-byte name: bytes, not a numeric, so copy + advance manually.
        // IterRemainingLength >= 40 bound above proves 8 bytes are live.
        MemCopy(s.name, IterDataAt(&c, IterIndex(&c)), 8);
        s.name[8] = '\0';
        IterMustMove(&c, 8);

        u32 ptr_relocs, ptr_linenums;
        u16 num_relocs, num_linenums;
        if (!BufReadFmt(
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
    if (!PeRvaToOffset(ctx->out, (u32)ctx->debug_dir_rva, &dir_offset)) {
        LOG_ERROR("PE: debug directory RVA not in any section");
        return;
    }

    enum {
        DEBUG_ENTRY_SIZE = 28,
    };
    u32 num_entries = ctx->debug_dir_size / DEBUG_ENTRY_SIZE;
    if (dir_offset + (u64)num_entries * DEBUG_ENTRY_SIZE > BufLength(&ctx->out->data)) {
        LOG_ERROR("PE: debug directory overruns file");
        return;
    }

    for (u32 i = 0; i < num_entries; ++i) {
        u64     entry_off = dir_offset + (u64)i * DEBUG_ENTRY_SIZE;
        BufIter c = BufIterFromMemory(BufData(&ctx->out->data) + entry_off, BufLength(&ctx->out->data) - entry_off);
        u32     charac, ts, type, sz, raddr, rptr;
        u16     ver_maj, ver_min;
        if (!BufReadFmt(&c, FMT_PE_DEBUG_DIR_LE, charac, ts, ver_maj, ver_min, type, sz, raddr, rptr))
            return;
        (void)charac;
        (void)ts;
        (void)ver_maj;
        (void)ver_min;

        if (type != IMAGE_DEBUG_TYPE_CODEVIEW)
            continue;
        if (rptr + (u64)sz > BufLength(&ctx->out->data)) {
            LOG_ERROR("PE: codeview record points outside file");
            continue;
        }
        // RSDS = 4-byte sig + 16-byte GUID + 4-byte age + cstring path.
        if (sz < 4 + 16 + 4 + 1)
            continue;
        BufIter cv_cur = BufIterFromMemory(BufData(&ctx->out->data) + rptr, sz);
        u32     cv_sig;
        if (!BufReadU32LE(&cv_cur, &cv_sig))
            continue;
        if (cv_sig != CV_SIGNATURE_RSDS) {
            // NB7 (PDB 2.0) and earlier are no longer emitted by
            // toolchains we care about; treat them as unsupported.
            continue;
        }
        if (IterRemainingLength(&cv_cur) < 16 + 4)
            continue;
        // Same proof: 16 bytes are live.
        MemCopy(cv->guid, IterDataAt(&cv_cur, IterIndex(&cv_cur)), 16);
        IterMustMove(&cv_cur, 16);
        if (!BufReadU32LE(&cv_cur, &cv->age))
            continue;
        // Verify the trailing path is NUL-terminated inside the record.
        const u8 *path_start = IterDataAt(&cv_cur, IterIndex(&cv_cur));
        const u8 *region_end = IterDataAt(&cv_cur, IterLength(&cv_cur));
        bool      terminated = false;
        for (const u8 *p = path_start; p < region_end; ++p) {
            if (*p == '\0') {
                terminated = true;
                break;
            }
        }
        if (!terminated)
            continue;
        cv->pdb_path = (Zstr)path_start;
        cv->present  = true;
        return; // first CodeView entry wins
    }
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

// L-value form. Takes the caller's `Buf` by pointer, snapshots it,
// MemSets the caller's view. Anything that fails past the snapshot
// cleans up via PeDeinit -- the buffer never leaks.
bool PeOpenFromMemory(Pe *out, Buf *in) {
    if (!out || !in || !BufData(in) || !BufAllocator(in)) {
        LOG_FATAL("PeOpenFromMemory: NULL argument (contract violation)");
    }
    Buf taken = *in;
    MemSet(in, 0, sizeof(*in));

    MemSet(out, 0, sizeof(*out));
    out->data = taken;
    // Initialize the sections vec up-front so PeDeinit on a
    // failed-parse path doesn't trip ValidateVec.
    out->sections = VecInitT(out->sections, BufAllocator(&taken));

    PeContext ctx = {
        .out  = out,
        .file = BufIterFromBuf(&out->data),
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
    PeDeinit(out);
    return false;
}

// R-value form: allocate Buf, copy, hand `&copy` to the L-form.
bool pe_open_from_memory_copy(Pe *out, const u8 *data, size data_size, Allocator *alloc) {
    if (!out || !data || !alloc) {
        LOG_FATAL("PeOpenFromMemoryCopy: NULL argument (contract violation)");
    }
    Buf copy = BufInit(alloc);
    if (!BufReserve(&copy, (u64)data_size)) {
        LOG_ERROR("PeOpenFromMemoryCopy: allocation failed ({} bytes)", (u64)data_size);
        return false;
    }
    MemCopy(BufData(&copy), data, data_size);
    BufResize(&copy, (size)data_size);
    return PeOpenFromMemory(out, &copy);
}

bool pe_open(Pe *out, Zstr path, Allocator *alloc) {
    if (!out || !path || !alloc) {
        LOG_FATAL("PeOpen: NULL argument (contract violation)");
    }
    Buf data = BufInit(alloc);
    if (FileReadAndClose(path, &data) < 0) {
        BufDeinit(&data);
        LOG_ERROR("PeOpen: failed to read {}", path);
        return false;
    }
    return PeOpenFromMemory(out, &data);
}

void PeDeinit(Pe *self) {
    if (!self)
        return;
    BufDeinit(&self->data);
    VecDeinit(&self->sections);
    MemSet(self, 0, sizeof(*self));
}

const PeSection *pe_find_section_zstr(const Pe *self, Zstr name) {
    if (!self || !name)
        return NULL;
    for (size i = 0; i < VecLen(&self->sections); ++i) {
        const PeSection *s = VecPtrAt(&self->sections, i);
        if (ZstrCompare(s->name, name) == 0) {
            return s;
        }
    }
    return NULL;
}

const PeSection *pe_find_section_str(const Pe *self, const Str *name) {
    if (!self || !name)
        return NULL;
    return pe_find_section_zstr(self, StrBegin(name));
}

bool PeRvaToOffset(const Pe *self, u32 rva, u64 *out_offset) {
    if (!self || !out_offset)
        return false;
    for (size i = 0; i < VecLen(&self->sections); ++i) {
        const PeSection *s = VecPtrAt(&self->sections, i);
        // Compute the section end in u64; u32 + u32 can wrap.
        u64 vstart = (u64)s->virtual_address;
        u64 vend   = vstart + (u64)s->virtual_size;
        if (rva >= vstart && (u64)rva < vend) {
            u64 off = (u64)s->raw_offset + ((u64)rva - vstart);
            if (off >= BufLength(&self->data))
                return false;
            *out_offset = off;
            return true;
        }
    }
    return false;
}
