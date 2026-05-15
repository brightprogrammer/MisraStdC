/// file      : SymbolResolver.c
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
///   4. `ElfFileResolveAddress(elf, file_relative_addr)`.

#include <Misra/Sys/SymbolResolver.h>

#include <Misra/Std.h>
#include <Misra/Std/Log.h>
#include <Misra/Std/Memory.h>

#include <stdint.h>

// ---------------------------------------------------------------------------
// Cache management
// ---------------------------------------------------------------------------

static ResolverCacheEntry *resolver_cache_find_or_open(SymbolResolver *self, const char *path, u64 load_base) {
    for (u64 i = 0; i < self->cache.length; ++i) {
        ResolverCacheEntry *e = &self->cache.data[i];
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
    if (!ElfFileOpen(&entry.elf, path, self->allocator)) {
        return NULL;
    }
    if (!VecPushBackR(&self->cache, entry)) {
        ElfFileDeinit(&entry.elf);
        return NULL;
    }
    return &self->cache.data[self->cache.length - 1];
}

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

bool SymbolResolverInit(SymbolResolver *out, Allocator *alloc) {
    if (!out || !alloc) {
        LOG_ERROR("SymbolResolverInit: NULL argument");
        return false;
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
    for (u64 i = 0; i < self->cache.length; ++i) {
        ElfFileDeinit(&self->cache.data[i].elf);
    }
    VecDeinit(&self->cache);
    ProcMapsDeinit(&self->maps);
    MemSet(self, 0, sizeof(*self));
}

// ---------------------------------------------------------------------------
// Resolve
// ---------------------------------------------------------------------------

bool SymbolResolverResolve(SymbolResolver *self, void *runtime_addr, ResolvedSymbol *out) {
    if (!self || !out)
        return false;
    MemSet(out, 0, sizeof(*out));

    u64 addr = (u64)(uintptr_t)runtime_addr;

    const ProcMapEntry *entry = ProcMapsFindByAddr(&self->maps, addr);
    if (!entry || !entry->path || entry->path[0] == '\0') {
        return false;
    }

    // load_base = mapping.start - mapping.file_offset
    // (covers PIE / shared objects; for ET_EXEC the symbol values are
    // absolute and load_base happens to equal them, so the math still
    // works out.)
    u64 load_base = entry->start - entry->file_offset;

    ResolverCacheEntry *cache_entry = resolver_cache_find_or_open(self, entry->path, load_base);
    if (!cache_entry) {
        return false;
    }
    cache_entry->load_base = load_base;

    out->module_path = entry->path;
    out->module_base = load_base;

    u64              file_relative = addr - load_base;
    const ElfSymbol *sym           = ElfFileResolveAddress(&cache_entry->elf, file_relative);
    if (sym && sym->name && sym->name[0]) {
        out->symbol_name  = sym->name;
        out->symbol_value = sym->value;
        out->symbol_size  = sym->size;
        out->offset       = file_relative - sym->value;
    } else {
        out->offset = file_relative;
    }
    return true;
}
