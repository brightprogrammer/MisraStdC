/// file      : Elf.c
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// ELF64 little-endian parser. Reads the ELF header, section headers,
/// `.symtab` / `.dynsym` / their string tables, and resolves addresses
/// to enclosing symbols.

#include <Misra/Parsers/Elf.h>

#include <Misra/Std.h>
#include <Misra/Std/File.h>
#include <Misra/Std/Log.h>
#include <Misra/Std/Memory.h>

#include <stdint.h>

// ---------------------------------------------------------------------------
// On-disk record layouts. Defined locally so we don't depend on the
// system `<elf.h>` (it isn't on every platform we want to build on,
// and the spec is fixed anyway).
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
};

typedef struct __attribute__((packed)) RawEhdr64 {
    u8  e_ident[EI_NIDENT];
    u16 e_type;
    u16 e_machine;
    u32 e_version;
    u64 e_entry;
    u64 e_phoff;
    u64 e_shoff;
    u32 e_flags;
    u16 e_ehsize;
    u16 e_phentsize;
    u16 e_phnum;
    u16 e_shentsize;
    u16 e_shnum;
    u16 e_shstrndx;
} RawEhdr64;

typedef struct __attribute__((packed)) RawShdr64 {
    u32 sh_name;
    u32 sh_type;
    u64 sh_flags;
    u64 sh_addr;
    u64 sh_offset;
    u64 sh_size;
    u32 sh_link;
    u32 sh_info;
    u64 sh_addralign;
    u64 sh_entsize;
} RawShdr64;

typedef struct __attribute__((packed)) RawSym64 {
    u32 st_name;
    u8  st_info;
    u8  st_other;
    u16 st_shndx;
    u64 st_value;
    u64 st_size;
} RawSym64;

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
    if (self->data_size < sizeof(RawEhdr64)) {
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

    const RawEhdr64 *raw  = (const RawEhdr64 *)self->data;
    self->header.class    = ELF_CLASS_64;
    self->header.data     = ELF_DATA_LSB;
    self->header.type     = (ElfType)raw->e_type;
    self->header.machine  = raw->e_machine;
    self->header.entry    = raw->e_entry;
    self->header.phoff    = raw->e_phoff;
    self->header.shoff    = raw->e_shoff;
    self->header.phnum    = raw->e_phnum;
    self->header.shnum    = raw->e_shnum;
    self->header.shstrndx = raw->e_shstrndx;

    if (raw->e_shentsize != sizeof(RawShdr64) && raw->e_shnum > 0) {
        LOG_ERROR("ElfFile: unexpected e_shentsize ({} vs {})", (u32)raw->e_shentsize, (u64)sizeof(RawShdr64));
        return false;
    }

    return true;
}

static bool elf_decode_sections(ElfFile *self) {
    u16 n      = self->header.shnum;
    u64 shoff  = self->header.shoff;
    u64 needed = (u64)n * sizeof(RawShdr64);
    if (!elf_range_ok(self, shoff, needed)) {
        LOG_ERROR("ElfFile: section header table out of range");
        return false;
    }

    if (self->header.shstrndx >= n) {
        LOG_ERROR("ElfFile: shstrndx {} out of range (shnum={})", (u32)self->header.shstrndx, (u32)n);
        return false;
    }

    const RawShdr64 *raws       = (const RawShdr64 *)(self->data + shoff);
    const RawShdr64 *shstr_raw  = &raws[self->header.shstrndx];
    u64              shstr_off  = shstr_raw->sh_offset;
    u64              shstr_size = shstr_raw->sh_size;
    if (!elf_range_ok(self, shstr_off, shstr_size)) {
        LOG_ERROR("ElfFile: shstrtab out of range");
        return false;
    }

    for (u16 i = 0; i < n; ++i) {
        const RawShdr64 *r = &raws[i];
        ElfSection       s;
        s.name       = elf_str_at(self, shstr_off, shstr_size, r->sh_name);
        s.type       = r->sh_type;
        s.flags      = r->sh_flags;
        s.addr       = r->sh_addr;
        s.offset     = r->sh_offset;
        s.size       = r->sh_size;
        s.link       = r->sh_link;
        s.info       = r->sh_info;
        s.entry_size = r->sh_entsize;
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
    if (symtab->entry_size != sizeof(RawSym64)) {
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

    const RawSym64 *raws  = (const RawSym64 *)(self->data + symtab->offset);
    u64             count = symtab->size / sizeof(RawSym64);

    for (u64 i = 0; i < count; ++i) {
        const RawSym64 *r = &raws[i];
        ElfSymbol       s;
        s.name          = elf_str_at(self, strtab->offset, strtab->size, r->st_name);
        s.bind          = (ElfSymbolBind)(r->st_info >> 4);
        s.type          = (ElfSymbolType)(r->st_info & 0xf);
        s.section_index = r->st_shndx;
        s.value         = r->st_value;
        s.size          = r->st_size;
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
// Open / close
// ---------------------------------------------------------------------------

bool ElfFileOpenFromMemory(ElfFile *out, u8 *data, size data_size, Allocator *alloc) {
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
    return true;

fail:
    ElfFileDeinit(out);
    return false;
}

bool ElfFileOpen(ElfFile *out, const char *path, Allocator *alloc) {
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
        AllocatorFree(alloc, buf, capacity);
        return false;
    }
    // Take ownership of the buffer so Deinit frees it.
    out->owns_data = true;
    out->data_size = (size)capacity; // remember the real allocation so free returns the right size
    out->data      = (u8 *)buf;
    // Re-point the parser at the actual data length (capacity may be
    // larger than bytes when ReadCompleteFile reused an existing
    // buffer). We already validated against `bytes` inside
    // ElfFileOpenFromMemory; restore data_size to the file length so
    // range checks remain correct.
    out->data_size = (size)bytes;
    (void)capacity;
    return true;
}

void ElfFileDeinit(ElfFile *self) {
    if (!self)
        return;
    if (self->owns_data && self->data && self->allocator) {
        AllocatorFree(self->allocator, self->data, self->data_size);
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
