/// file      : sys/symbol_resolver.c
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// SymbolResolver implementation. The address-to-symbol pipeline:
///
///   1. `ProcMapsFindByAddr` locates the loaded mapping that contains
///      the runtime IP. From the mapping we get the backing file path
///      and we derive the file's load base as `mapping.start -
///      mapping.file_offset`.
///   2. Lookup the file path in our cache; on miss, open the file
///      through `Parsers/Elf` and stash it for future lookups.
///   3. file_relative_addr = runtime_addr - load_base.
///   4. `ElfResolveAddress(elf, file_relative_addr)`.

#include <Misra/Sys/SymbolResolver.h>

#include <Misra/Std.h>
#include <Misra/Std/File.h>
#include <Misra/Std/Log.h>
#include <Misra/Std/Memory.h>

#include <Misra/Std/File.h>

// ---------------------------------------------------------------------------
// Sidecar debug file discovery
// ---------------------------------------------------------------------------

// Format the build-id bytes as `aa/bbbb...` (first byte separator,
// rest concatenated, lowercase hex). Caller provides the Str.
static void append_build_id_path(Str *out, const u8 *id, u32 n) {
    static const char hex[] = "0123456789abcdef";
    if (n == 0)
        return;
    StrPushBackR(out, hex[id[0] >> 4]);
    StrPushBackR(out, hex[id[0] & 0xf]);
    StrPushBackR(out, '/');
    for (u32 i = 1; i < n; ++i) {
        StrPushBackR(out, hex[id[i] >> 4]);
        StrPushBackR(out, hex[id[i] & 0xf]);
    }
}

#include "_Helpers.h"

// Verify that an opened sidecar ELF actually pairs with the main file.
// For Build-ID lookups, the two files must carry identical build IDs.
// For debuglink lookups, the file's mere presence is good enough in v1
// (CRC32 cross-check is in FUTURE-PLANS).
static bool sidecar_matches(const Elf *main, const Elf *sidecar, bool by_build_id) {
    if (!by_build_id) {
        return true;
    }
    if (!main->build_id || !sidecar->build_id)
        return false;
    if (main->build_id_size != sidecar->build_id_size)
        return false;
    return MemCompare(main->build_id, sidecar->build_id, main->build_id_size) == 0;
}

// Try to open a sidecar `.debug` ELF for the main file at `path`.
// Search order, falling through on each miss:
//
//   1. /usr/lib/debug/.build-id/AA/BBBB...debug   (Build-ID, most reliable)
//   2. {binary_dir}/{debuglink_name}              (right next to binary)
//   3. {binary_dir}/.debug/{debuglink_name}
//   4. /usr/lib/debug{binary_dir}/{debuglink_name}
//
// Returns true on success; `out` is populated with an opened Elf.
static bool try_open_sidecar(Zstr main_path, const Elf *main, Elf *out, Allocator *alloc) {
    Str path = StrInit(alloc);

    // (1) Build-ID
    if (main->build_id && main->build_id_size > 0) {
        StrResize(&path, 0);
        StrPushBackMany(&path, "/usr/lib/debug/.build-id/");
        append_build_id_path(&path, main->build_id, main->build_id_size);
        StrPushBackMany(&path, ".debug");
        if (sys_path_exists(StrBegin(&path)) && ElfOpen(out, &path, alloc)) {
            if (sidecar_matches(main, out, /*by_build_id*/ true)) {
                StrDeinit(&path);
                return true;
            }
            ElfDeinit(out);
        }
    }

    // (2-4) debuglink in standard locations
    if (main->debuglink_name && main->debuglink_name[0]) {
        Zstr cand_prefix = "/usr/lib/debug";

        // (2) {dir}/{name}
        StrResize(&path, 0);
        sys_append_dirname(&path, main_path);
        StrPushBackR(&path, '/');
        StrPushBackMany(&path, main->debuglink_name);
        if (sys_path_exists(StrBegin(&path)) && ElfOpen(out, &path, alloc)) {
            if (sidecar_matches(main, out, /*by_build_id*/ false)) {
                StrDeinit(&path);
                return true;
            }
            ElfDeinit(out);
        }

        // (3) {dir}/.debug/{name}
        StrResize(&path, 0);
        sys_append_dirname(&path, main_path);
        StrPushBackMany(&path, "/.debug/");
        StrPushBackMany(&path, main->debuglink_name);
        if (sys_path_exists(StrBegin(&path)) && ElfOpen(out, &path, alloc)) {
            if (sidecar_matches(main, out, /*by_build_id*/ false)) {
                StrDeinit(&path);
                return true;
            }
            ElfDeinit(out);
        }

        // (4) /usr/lib/debug{dir}/{name}
        StrResize(&path, 0);
        StrPushBackMany(&path, cand_prefix);
        sys_append_dirname(&path, main_path);
        StrPushBackR(&path, '/');
        StrPushBackMany(&path, main->debuglink_name);
        if (sys_path_exists(StrBegin(&path)) && ElfOpen(out, &path, alloc)) {
            if (sidecar_matches(main, out, /*by_build_id*/ false)) {
                StrDeinit(&path);
                return true;
            }
            ElfDeinit(out);
        }
    }

    StrDeinit(&path);
    return false;
}

// ---------------------------------------------------------------------------
// Cache management
// ---------------------------------------------------------------------------

static ResolverCacheEntry *resolver_cache_find_or_open(SymbolResolver *self, Zstr path, u64 load_base) {
    for (u64 i = 0; i < VecLen(&self->cache); ++i) {
        ResolverCacheEntry *e = VecPtrAt(&self->cache, i);
        if (e->path == path) {
            return e;
        }
        // Some `/proc/self/maps` lines share the same path string in
        // different positions of the raw buffer if the kernel
        // generated separate copies — fall back to string compare.
        if (e->path && path && ZstrCompare(e->path, path) == 0) {
            return e;
        }
    }

    ResolverCacheEntry entry;
    MemSet(&entry, 0, sizeof(entry));
    entry.path      = path;
    entry.load_base = load_base;
    if (!ElfOpen(&entry.elf, path, self->allocator)) {
        return NULL;
    }
    // Best-effort sidecar lookup. Silent failure is fine — we'll just
    // resolve against whatever the main file has.
    if (try_open_sidecar(path, &entry.elf, &entry.sidecar, self->allocator)) {
        entry.has_sidecar = true;
    }
    if (!VecPushBackR(&self->cache, entry)) {
        if (entry.has_sidecar)
            ElfDeinit(&entry.sidecar);
        ElfDeinit(&entry.elf);
        return NULL;
    }
    return VecPtrAt(&self->cache, VecLen(&self->cache) - 1);
}

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

bool symbol_resolver_init(SymbolResolver *out, Allocator *alloc) {
    if (!out || !alloc) {
        LOG_FATAL("SymbolResolverInit: NULL argument");
    }
    MemSet(out, 0, sizeof(*out));
    out->allocator = alloc;
    out->cache     = VecInitT(out->cache, alloc);
    if (!ProcMapsLoad(&out->maps, alloc)) {
        VecDeinit(&out->cache);
        MemSet(out, 0, sizeof(*out));
        return false;
    }
    return true;
}

void SymbolResolverDeinit(SymbolResolver *self) {
    if (!self)
        return;
    for (u64 i = 0; i < VecLen(&self->cache); ++i) {
        ResolverCacheEntry *e = VecPtrAt(&self->cache, i);
#if FEATURE_PARSER_DWARF
        if (e->dwarf_built && e->dwarf_ok) {
            DwarfLinesDeinit(&e->dwarf);
        }
        if (e->sidecar_dwarf_built && e->sidecar_dwarf_ok) {
            DwarfLinesDeinit(&e->sidecar_dwarf);
        }
        if (e->cfi_built && e->cfi_ok) {
            DwarfCfiDeinit(&e->cfi);
        }
        if (e->functions_built && e->functions_ok) {
            DwarfFunctionsDeinit(&e->functions);
        }
        if (e->sidecar_functions_built && e->sidecar_functions_ok) {
            DwarfFunctionsDeinit(&e->sidecar_functions);
        }
#endif
        if (e->has_sidecar) {
            ElfDeinit(&e->sidecar);
        }
        ElfDeinit(&e->elf);
    }
    VecDeinit(&self->cache);
    ProcMapsDeinit(&self->maps);
    MemSet(self, 0, sizeof(*self));
}

// Module load bias from a `/proc/self/maps` mapping + the file's PT_LOAD
// table. Symbol `st_value`s and FDE PCs live in p_vaddr space, but a maps
// line gives a *file* offset. The historical `start - file_offset` only
// equals the bias when `p_vaddr == p_offset` -- true on 4 KiB-page x86,
// FALSE on AArch64 where the 64 KiB `max-page-size` leaves `p_vaddr`
// ahead of `p_offset` for later segments.
//
// Select the covering PT_LOAD by the resolved ADDRESS's own file offset,
// not the mapping's: /proc maps page-align the file offset down (to 64 KiB
// on aarch64), so one page can straddle the end of one PT_LOAD and the
// start of the next and `map_file_offset` can sit below the segment the
// address really lives in -- picking by `map_file_offset` then lands on the
// wrong segment and biases the result by a page. The address's true offset
// is unambiguous. Bias is constant per module (`dl_iterate_phdr`'s
// `dlpi_addr`).
static u64 resolver_load_bias(const Elf *elf, u64 map_start, u64 map_file_offset, u64 runtime_addr) {
    u64 addr_file_offset = map_file_offset + (runtime_addr - map_start);
    // --- TEMP DIAGNOSTIC (remove after CI) ---
    LOG_ERROR(
        "SRBIASDBG in map_start={x} map_off={x} addr={x} addr_foff={x} nseg={}",
        map_start,
        map_file_offset,
        runtime_addr,
        addr_file_offset,
        (u64)VecLen(&elf->segments)
    );
    VecForeachPtr(&elf->segments, seg) {
        if (seg->type != ELF_PT_LOAD) {
            continue;
        }
        // --- TEMP DIAGNOSTIC (remove after CI) ---
        LOG_ERROR("SRBIASDBG   PT_LOAD off={x} vaddr={x} filesz={x}", seg->offset, seg->vaddr, seg->filesz);
        if (addr_file_offset >= seg->offset && addr_file_offset < seg->offset + seg->filesz) {
            u64 b = runtime_addr - (seg->vaddr + (addr_file_offset - seg->offset));
            LOG_ERROR("SRBIASDBG   SELECTED -> bias={x}", b);
            return b;
        }
    }
    // No covering PT_LOAD (unusual) -- fall back to the historical formula.
    LOG_ERROR("SRBIASDBG   FALLBACK -> bias={x}", map_start - map_file_offset);
    return map_start - map_file_offset;
}

#if FEATURE_PARSER_DWARF
bool SymbolResolverFindFde(
    SymbolResolver  *self,
    void            *runtime_addr,
    const DwarfCfi **out_cfi,
    const DwarfFde **out_fde,
    u64             *out_module_base
) {
    if (!self || !out_cfi || !out_fde || !out_module_base)
        return false;

    u64                 addr  = (u64)runtime_addr;
    const ProcMapEntry *entry = ProcMapsFindByAddr(&self->maps, addr);
    if (!entry || !entry->path || entry->path[0] == '\0')
        return false;

    ResolverCacheEntry *cache_entry = resolver_cache_find_or_open(self, entry->path, entry->start - entry->file_offset);
    if (!cache_entry)
        return false;
    // p_vaddr-space bias (not the file-offset shortcut) -- see resolver_load_bias.
    u64 load_base          = resolver_load_bias(&cache_entry->elf, entry->start, entry->file_offset, addr);
    cache_entry->load_base = load_base;

    if (!cache_entry->cfi_built) {
        cache_entry->cfi_built = true;
        cache_entry->cfi_ok    = DwarfCfiBuildFromElf(&cache_entry->cfi, &cache_entry->elf, self->allocator);
    }
    if (!cache_entry->cfi_ok)
        return false;

    u64             file_relative = addr - load_base;
    const DwarfFde *fde           = DwarfCfiFindFde(&cache_entry->cfi, file_relative);
    if (!fde)
        return false;

    *out_cfi         = &cache_entry->cfi;
    *out_fde         = fde;
    *out_module_base = load_base;
    return true;
}
#endif

// ---------------------------------------------------------------------------
// Resolve
// ---------------------------------------------------------------------------

bool SymbolResolverResolve(SymbolResolver *self, void *runtime_addr, ResolvedSymbol *out) {
    if (!self || !out)
        return false;
    MemSet(out, 0, sizeof(*out));

    u64 addr = (u64)runtime_addr;

    const ProcMapEntry *entry = ProcMapsFindByAddr(&self->maps, addr);
    if (!entry || !entry->path || entry->path[0] == '\0') {
        return false;
    }

    ResolverCacheEntry *cache_entry = resolver_cache_find_or_open(self, entry->path, entry->start - entry->file_offset);
    if (!cache_entry) {
        return false;
    }
    // Correct load bias in p_vaddr space (see resolver_load_bias). Covers
    // PIE / shared objects; for ET_EXEC the first PT_LOAD's p_vaddr already
    // equals the mapping start, so the absolute base still falls out.
    u64 load_base          = resolver_load_bias(&cache_entry->elf, entry->start, entry->file_offset, addr);
    cache_entry->load_base = load_base;

    out->module_path = entry->path;
    out->module_base = load_base;

    u64 file_relative = addr - load_base;

    // Symbol resolution: try the main file first, fall through to the
    // sidecar (full `.symtab` for stripped binaries) if nothing
    // matches.
    const ElfSymbol *sym = ElfResolveAddress(&cache_entry->elf, file_relative);
    if (!(sym && sym->name && sym->name[0]) && cache_entry->has_sidecar) {
        sym = ElfResolveAddress(&cache_entry->sidecar, file_relative);
    }
    if (sym && sym->name && sym->name[0]) {
        out->symbol_name  = sym->name;
        out->symbol_value = sym->value;
        out->symbol_size  = sym->size;
        out->offset       = file_relative - sym->value;
    } else {
        out->offset = file_relative;
    }
    // --- TEMP DIAGNOSTIC (remove after CI) ---
    LOG_ERROR(
        "SRRESDBG addr={x} map[start={x} off={x} path={}] base={x} frel={x} nsym={} ndyn={} sym={} symval={x} "
        "symsz={x} sidecar={}",
        addr,
        entry->start,
        entry->file_offset,
        entry->path ? entry->path : "<null>",
        load_base,
        file_relative,
        (u64)VecLen(&cache_entry->elf.symbols),
        (u64)VecLen(&cache_entry->elf.dynamic_symbols),
        (sym && sym->name) ? sym->name : "<null>",
        sym ? sym->value : (u64)0,
        sym ? sym->size : (u64)0,
        cache_entry->has_sidecar ? 1 : 0
    );

#if FEATURE_PARSER_DWARF
    // .debug_info function-name fallback. Only consulted when neither
    // .symtab nor .dynsym (main + sidecar) produced a name. DWARF
    // subprogram DIEs cover stripped binaries' function bodies even
    // when no ELF symbol exists.
    if (!out->symbol_name) {
        if (!cache_entry->functions_built) {
            cache_entry->functions_built = true;
            cache_entry->functions_ok =
                DwarfFunctionsBuildFromElf(&cache_entry->functions, &cache_entry->elf, self->allocator);
        }
        const DwarfFunction *f = NULL;
        if (cache_entry->functions_ok) {
            f = DwarfFunctionsResolve(&cache_entry->functions, file_relative);
        }
        if (!f && cache_entry->has_sidecar) {
            if (!cache_entry->sidecar_functions_built) {
                cache_entry->sidecar_functions_built = true;
                cache_entry->sidecar_functions_ok =
                    DwarfFunctionsBuildFromElf(&cache_entry->sidecar_functions, &cache_entry->sidecar, self->allocator);
            }
            if (cache_entry->sidecar_functions_ok) {
                f = DwarfFunctionsResolve(&cache_entry->sidecar_functions, file_relative);
            }
        }
        if (f) {
            out->symbol_name  = f->name;
            out->symbol_value = f->low_pc;
            out->symbol_size  = f->high_pc - f->low_pc;
            out->offset       = file_relative - f->low_pc;
        }
    }
#endif

#if FEATURE_PARSER_DWARF
    if (!cache_entry->dwarf_built) {
        cache_entry->dwarf_built = true;
        cache_entry->dwarf_ok    = DwarfLinesBuildFromElf(&cache_entry->dwarf, &cache_entry->elf, self->allocator);
    }
    const DwarfLineEntry *de = NULL;
    if (cache_entry->dwarf_ok) {
        de = DwarfLinesResolve(&cache_entry->dwarf, file_relative);
    }
    if (!de && cache_entry->has_sidecar) {
        if (!cache_entry->sidecar_dwarf_built) {
            cache_entry->sidecar_dwarf_built = true;
            cache_entry->sidecar_dwarf_ok =
                DwarfLinesBuildFromElf(&cache_entry->sidecar_dwarf, &cache_entry->sidecar, self->allocator);
        }
        if (cache_entry->sidecar_dwarf_ok) {
            de = DwarfLinesResolve(&cache_entry->sidecar_dwarf, file_relative);
        }
    }
    if (de) {
        out->source_file   = de->file;
        out->source_dir    = de->dir;
        out->source_line   = de->line;
        out->source_column = de->column;
    }
#endif

    return true;
}
