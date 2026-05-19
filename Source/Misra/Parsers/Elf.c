/// file      : Elf.c
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// ELF64 little-endian parser. Reads the ELF header, section headers,
/// `.symtab` / `.dynsym` / their string tables, and resolves addresses
/// to enclosing symbols.
///
/// Binary decode goes through `StrReadFmt` with raw-byte format
/// specifiers (`{<Nr}` for little-endian N-byte reads), so we never
/// need packed-struct typedefs or manual byteswaps. Adding big-endian
/// or ELF32 later is mostly a matter of swapping the format strings.

#include <Misra/Parsers/Elf.h>

#include <Misra/Std.h>
#include <Misra/Std/File.h>
#include <Misra/Std/Io.h>
#include <Misra/Std/Log.h>
#include <Misra/Std/Memory.h>

// ---------------------------------------------------------------------------
// Spec constants
// ---------------------------------------------------------------------------

enum {
    EI_NIDENT = 16,
    EI_MAG0   = 0,
    EI_MAG1   = 1,
    EI_MAG2   = 2,
    EI_MAG3   = 3,
    EI_CLASS  = 4,
    EI_DATA   = 5,

    ELF_MAG0 = 0x7f,
    ELF_MAG1 = 'E',
    ELF_MAG2 = 'L',
    ELF_MAG3 = 'F',

    // Sizes of the on-disk records we read.
    EHDR64_SIZE_AFTER_IDENT = 48,
    SHDR64_SIZE             = 64,
    SYM64_SIZE              = 24,
};

// Little-endian field layouts for the records we read. The byte
// totals in the comment must match {EHDR64_SIZE_AFTER_IDENT,
// SHDR64_SIZE, SYM64_SIZE}.
//
// EHDR64 (post-e_ident): 48 bytes.
#define FMT_EHDR64_LE                                                                                                  \
    "{<2r}" /* e_type      */                                                                                          \
    "{<2r}" /* e_machine   */                                                                                          \
    "{<4r}" /* e_version   */                                                                                          \
    "{<8r}" /* e_entry     */                                                                                          \
    "{<8r}" /* e_phoff     */                                                                                          \
    "{<8r}" /* e_shoff     */                                                                                          \
    "{<4r}" /* e_flags     */                                                                                          \
    "{<2r}" /* e_ehsize    */                                                                                          \
    "{<2r}" /* e_phentsize */                                                                                          \
    "{<2r}" /* e_phnum     */                                                                                          \
    "{<2r}" /* e_shentsize */                                                                                          \
    "{<2r}" /* e_shnum     */                                                                                          \
    "{<2r}" /* e_shstrndx  */

// SHDR64: 64 bytes.
#define FMT_SHDR64_LE                                                                                                  \
    "{<4r}" /* sh_name      */                                                                                         \
    "{<4r}" /* sh_type      */                                                                                         \
    "{<8r}" /* sh_flags     */                                                                                         \
    "{<8r}" /* sh_addr      */                                                                                         \
    "{<8r}" /* sh_offset    */                                                                                         \
    "{<8r}" /* sh_size      */                                                                                         \
    "{<4r}" /* sh_link      */                                                                                         \
    "{<4r}" /* sh_info      */                                                                                         \
    "{<8r}" /* sh_addralign */                                                                                         \
    "{<8r}" /* sh_entsize   */

// SYM64: 24 bytes.
#define FMT_SYM64_LE                                                                                                   \
    "{<4r}" /* st_name  */                                                                                             \
    "{<1r}" /* st_info  */                                                                                             \
    "{<1r}" /* st_other */                                                                                             \
    "{<2r}" /* st_shndx */                                                                                             \
    "{<8r}" /* st_value */                                                                                             \
    "{<8r}" /* st_size  */

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static const char *elf_str_at(const ElfFile *self, u64 strtab_offset, u64 strtab_size, u32 idx) {
    if ((u64)idx >= strtab_size) {
        return "";
    }
    return (const char *)(self->data + strtab_offset + idx);
}

static bool elf_range_ok(const ElfFile *self, u64 offset, u64 size) {
    if (offset > self->data_size)
        return false;
    if (size > self->data_size)
        return false;
    if (offset + size > self->data_size)
        return false;
    return true;
}

// ---------------------------------------------------------------------------
// Header + section decoding
// ---------------------------------------------------------------------------

static bool elf_decode_header(ElfFile *self) {
    if (self->data_size < EI_NIDENT + EHDR64_SIZE_AFTER_IDENT) {
        LOG_ERROR("ElfFile: file too small for ELF64 header ({} bytes)", (u64)self->data_size);
        return false;
    }

    const u8 *id = self->data;
    if (id[EI_MAG0] != ELF_MAG0 || id[EI_MAG1] != ELF_MAG1 || id[EI_MAG2] != ELF_MAG2 || id[EI_MAG3] != ELF_MAG3) {
        LOG_ERROR("ElfFile: bad magic");
        return false;
    }

    if (id[EI_CLASS] != (u8)ELF_CLASS_64) {
        LOG_ERROR("ElfFile: only ELF64 supported in v1 (got class {})", (u32)id[EI_CLASS]);
        return false;
    }
    if (id[EI_DATA] != (u8)ELF_DATA_LSB) {
        LOG_ERROR("ElfFile: only little-endian supported in v1 (got data {})", (u32)id[EI_DATA]);
        return false;
    }

    self->header.class = ELF_CLASS_64;
    self->header.data  = ELF_DATA_LSB;

    const char *cursor = (const char *)self->data + EI_NIDENT;

    u16 type = 0, machine = 0, ehsize = 0, phentsize = 0, phnum = 0, shentsize = 0, shnum = 0, shstrndx = 0;
    u32 version = 0, flags = 0;
    u64 entry = 0, phoff = 0, shoff = 0;

    StrReadFmt(
        cursor,
        FMT_EHDR64_LE,
        type,
        machine,
        version,
        entry,
        phoff,
        shoff,
        flags,
        ehsize,
        phentsize,
        phnum,
        shentsize,
        shnum,
        shstrndx
    );

    self->header.type     = (ElfType)type;
    self->header.machine  = machine;
    self->header.entry    = entry;
    self->header.phoff    = phoff;
    self->header.shoff    = shoff;
    self->header.phnum    = phnum;
    self->header.shnum    = shnum;
    self->header.shstrndx = shstrndx;

    if (shentsize != SHDR64_SIZE && shnum > 0) {
        LOG_ERROR("ElfFile: unexpected e_shentsize ({} vs {})", (u32)shentsize, (u32)SHDR64_SIZE);
        return false;
    }

    return true;
}

static bool elf_decode_sections(ElfFile *self) {
    u16 n      = self->header.shnum;
    u64 shoff  = self->header.shoff;
    u64 needed = (u64)n * SHDR64_SIZE;
    if (!elf_range_ok(self, shoff, needed)) {
        LOG_ERROR("ElfFile: section header table out of range");
        return false;
    }

    if (self->header.shstrndx >= n) {
        LOG_ERROR("ElfFile: shstrndx {} out of range (shnum={})", (u32)self->header.shstrndx, (u32)n);
        return false;
    }

    // Decode the shstrtab header first so we can resolve section names.
    u64 shstr_off  = 0;
    u64 shstr_size = 0;
    {
        const char *cursor = (const char *)self->data + shoff + (u64)self->header.shstrndx * SHDR64_SIZE;
        u32         name = 0, type = 0, link = 0, info = 0;
        u64         flags = 0, addr = 0, offset = 0, size_ = 0, addralign = 0, entsize = 0;
        StrReadFmt(cursor, FMT_SHDR64_LE, name, type, flags, addr, offset, size_, link, info, addralign, entsize);
        shstr_off  = offset;
        shstr_size = size_;
    }
    if (!elf_range_ok(self, shstr_off, shstr_size)) {
        LOG_ERROR("ElfFile: shstrtab out of range");
        return false;
    }

    const char *cursor = (const char *)self->data + shoff;
    for (u16 i = 0; i < n; ++i) {
        u32 name = 0, type = 0, link = 0, info = 0;
        u64 flags = 0, addr = 0, offset = 0, size_ = 0, addralign = 0, entsize = 0;

        StrReadFmt(cursor, FMT_SHDR64_LE, name, type, flags, addr, offset, size_, link, info, addralign, entsize);

        ElfSection s;
        s.name       = elf_str_at(self, shstr_off, shstr_size, name);
        s.type       = type;
        s.flags      = flags;
        s.addr       = addr;
        s.offset     = offset;
        s.size       = size_;
        s.link       = link;
        s.info       = info;
        s.entry_size = entsize;
        if (!VecPushBackR(&self->sections, s)) {
            return false;
        }
    }
    return true;
}

// ---------------------------------------------------------------------------
// Symbol-table decoding
// ---------------------------------------------------------------------------

static bool elf_decode_symbol_table(ElfFile *self, const ElfSection *symtab, ElfSymbols *out) {
    if (!symtab || symtab->size == 0) {
        return true;
    }
    if (symtab->entry_size != SYM64_SIZE) {
        LOG_ERROR("ElfFile: unexpected symbol entry size {}", (u64)symtab->entry_size);
        return false;
    }
    if (!elf_range_ok(self, symtab->offset, symtab->size)) {
        LOG_ERROR("ElfFile: symbol table out of range");
        return false;
    }

    // The link field of a SYMTAB / DYNSYM section is the index of its
    // associated string table.
    u32 strtab_idx = symtab->link;
    if (strtab_idx >= self->sections.length) {
        LOG_ERROR("ElfFile: symtab link {} out of range", (u32)strtab_idx);
        return false;
    }
    const ElfSection *strtab = &self->sections.data[strtab_idx];
    if (!elf_range_ok(self, strtab->offset, strtab->size)) {
        LOG_ERROR("ElfFile: strtab out of range");
        return false;
    }

    u64 count = symtab->size / SYM64_SIZE;
    // Sanity cap. A crafted symtab section can declare a size near
    // u64 max if the file itself is huge (mmap'd artifact), making
    // this loop walk billions of iterations with a Vec push each.
    // Real binaries top out at a few hundred thousand symbols.
    enum {
        ELF_MAX_SYMBOLS = 16u * 1024u * 1024u
    };
    if (count > ELF_MAX_SYMBOLS) {
        LOG_ERROR("ElfFile: symbol count {} exceeds sanity cap; refusing", count);
        return false;
    }

    const char *cursor = (const char *)self->data + symtab->offset;
    for (u64 i = 0; i < count; ++i) {
        u32 name = 0;
        u8  info = 0, other = 0;
        u16 shndx = 0;
        u64 value = 0, size_ = 0;

        StrReadFmt(cursor, FMT_SYM64_LE, name, info, other, shndx, value, size_);

        ElfSymbol s;
        s.name          = elf_str_at(self, strtab->offset, strtab->size, name);
        s.bind          = (ElfSymbolBind)(info >> 4);
        s.type          = (ElfSymbolType)(info & 0xf);
        s.section_index = shndx;
        s.value         = value;
        s.size          = size_;
        if (!VecPushBackR(out, s)) {
            return false;
        }
    }
    return true;
}

static bool elf_decode_symbols(ElfFile *self) {
    const ElfSection *symtab    = NULL;
    const ElfSection *dynsymtab = NULL;

    for (u64 i = 0; i < self->sections.length; ++i) {
        const ElfSection *s = &self->sections.data[i];
        if (s->type == ELF_SECTION_TYPE_SYMTAB) {
            symtab = s;
        } else if (s->type == ELF_SECTION_TYPE_DYNSYM) {
            dynsymtab = s;
        }
    }

    if (!elf_decode_symbol_table(self, symtab, &self->symbols)) {
        return false;
    }
    if (!elf_decode_symbol_table(self, dynsymtab, &self->dynamic_symbols)) {
        return false;
    }
    return true;
}

// ---------------------------------------------------------------------------
// Stripped-binary metadata: .note.gnu.build-id and .gnu_debuglink
// ---------------------------------------------------------------------------
//
// `.note.gnu.build-id` payload layout (single ELF note record):
//   u32 namesz       (=4)
//   u32 descsz       (=build-id length, typically 20 for SHA-1)
//   u32 type         (=NT_GNU_BUILD_ID = 3)
//   char name[]      ("GNU\0", 4-byte padded)
//   u8 desc[]        (descsz bytes, 4-byte padded)
//
// `.gnu_debuglink` payload layout:
//   char filename[]  (NUL-terminated, 4-byte aligned with NUL padding)
//   u32  crc32       (CRC32 of the expected sidecar file's contents)

enum {
    NT_GNU_BUILD_ID = 3
};

static void elf_decode_build_id(ElfFile *self, const ElfSection *note) {
    if (!elf_range_ok(self, note->offset, note->size) || note->size < 16) {
        return;
    }
    const u8 *p   = self->data + note->offset;
    const u8 *end = p + note->size;
    if ((u64)(end - p) < 12)
        return;

    u32 namesz  = (u32)p[0] | ((u32)p[1] << 8) | ((u32)p[2] << 16) | ((u32)p[3] << 24);
    u32 descsz  = (u32)p[4] | ((u32)p[5] << 8) | ((u32)p[6] << 16) | ((u32)p[7] << 24);
    u32 type    = (u32)p[8] | ((u32)p[9] << 8) | ((u32)p[10] << 16) | ((u32)p[11] << 24);
    p          += 12;

    if (type != NT_GNU_BUILD_ID)
        return;

    // name + (round up to 4)
    u64 name_padded = ((u64)namesz + 3u) & ~(u64)3u;
    if ((u64)(end - p) < name_padded + descsz)
        return;
    p += name_padded;
    if ((u64)(end - p) < descsz)
        return;

    self->build_id      = p;
    self->build_id_size = descsz;
}

static void elf_decode_debug_link(ElfFile *self, const ElfSection *dl) {
    if (!elf_range_ok(self, dl->offset, dl->size) || dl->size < 5) {
        return;
    }
    const char *base = (const char *)(self->data + dl->offset);
    // filename runs up to (and including) the NUL; CRC follows in the
    // last 4 bytes of the section, after alignment padding.
    u64 max_name = dl->size > 4 ? dl->size - 4 : 0;
    u64 name_len = 0;
    while (name_len < max_name && base[name_len] != '\0') {
        ++name_len;
    }
    if (name_len == 0 || name_len >= max_name) {
        return; // unterminated or empty
    }
    // CRC is in the last 4 bytes.
    const u8 *crc_bytes = self->data + dl->offset + dl->size - 4;
    u32 crc = (u32)crc_bytes[0] | ((u32)crc_bytes[1] << 8) | ((u32)crc_bytes[2] << 16) | ((u32)crc_bytes[3] << 24);

    self->debuglink_name = base;
    self->debuglink_crc  = crc;
}

static void elf_decode_debug_metadata(ElfFile *self) {
    const ElfSection *note = ElfFileFindSection(self, ".note.gnu.build-id");
    if (note) {
        elf_decode_build_id(self, note);
    }
    const ElfSection *dl = ElfFileFindSection(self, ".gnu_debuglink");
    if (dl) {
        elf_decode_debug_link(self, dl);
    }
}

// ---------------------------------------------------------------------------
// Open / close
// ---------------------------------------------------------------------------

bool elf_file_open_from_memory(ElfFile *out, u8 *data, size data_size, Allocator *alloc) {
    if (!out || !data || !alloc) {
        LOG_ERROR("ElfFileOpenFromMemory: NULL argument");
        return false;
    }
    MemSet(out, 0, sizeof(*out));
    out->allocator       = alloc;
    out->data            = data;
    out->data_size       = data_size;
    out->owns_data       = false;
    out->sections        = VecInitT(out->sections, alloc);
    out->symbols         = VecInitT(out->symbols, alloc);
    out->dynamic_symbols = VecInitT(out->dynamic_symbols, alloc);

    if (!elf_decode_header(out))
        goto fail;
    if (!elf_decode_sections(out))
        goto fail;
    if (!elf_decode_symbols(out))
        goto fail;
    elf_decode_debug_metadata(out);
    return true;

fail:
    ElfFileDeinit(out);
    return false;
}

bool elf_file_open(ElfFile *out, const char *path, Allocator *alloc) {
    if (!out || !path || !alloc) {
        LOG_ERROR("ElfFileOpen: NULL argument");
        return false;
    }
    char *buf      = NULL;
    u64   bytes    = 0;
    u64   capacity = 0;
    if (!ReadCompleteFile(path, &buf, &bytes, &capacity, alloc)) {
        LOG_ERROR("ElfFileOpen: failed to read {}", path);
        return false;
    }

    if (!ElfFileOpenFromMemory(out, (u8 *)buf, (size)bytes, alloc)) {
        AllocatorFree(alloc, buf);
        return false;
    }
    // Take ownership of the buffer so Deinit frees it.
    out->owns_data = true;
    out->data      = (u8 *)buf;
    out->data_size = (size)bytes;
    (void)capacity;
    return true;
}

void ElfFileDeinit(ElfFile *self) {
    if (!self)
        return;
    if (self->owns_data && self->data && self->allocator) {
        AllocatorFree(self->allocator, self->data);
    }
    VecDeinit(&self->sections);
    VecDeinit(&self->symbols);
    VecDeinit(&self->dynamic_symbols);
    MemSet(self, 0, sizeof(*self));
}

// ---------------------------------------------------------------------------
// Lookup
// ---------------------------------------------------------------------------

static const ElfSymbol *elf_search_symbols(const ElfSymbols *syms, u64 vaddr) {
    const ElfSymbol *best = NULL;
    for (u64 i = 0; i < syms->length; ++i) {
        const ElfSymbol *s = &syms->data[i];
        if (s->size == 0) {
            // Some symbols (e.g. labels) have zero size — only match
            // when the address equals the symbol exactly.
            if (s->value == vaddr) {
                if (!best || s->bind == ELF_SYMBOL_BIND_GLOBAL) {
                    best = s;
                }
            }
            continue;
        }
        if (vaddr >= s->value && vaddr < s->value + s->size) {
            if (!best || s->bind == ELF_SYMBOL_BIND_GLOBAL) {
                best = s;
            }
        }
    }
    return best;
}

const ElfSymbol *ElfFileResolveAddress(const ElfFile *self, u64 vaddr) {
    if (!self)
        return NULL;
    const ElfSymbol *hit = elf_search_symbols(&self->symbols, vaddr);
    if (hit)
        return hit;
    return elf_search_symbols(&self->dynamic_symbols, vaddr);
}

const ElfSection *ElfFileFindSection(const ElfFile *self, const char *name) {
    if (!self || !name)
        return NULL;
    for (u64 i = 0; i < self->sections.length; ++i) {
        const ElfSection *s = &self->sections.data[i];
        if (s->name && ZstrCompare(s->name, name) == 0) {
            return s;
        }
    }
    return NULL;
}
