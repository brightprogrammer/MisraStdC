/// file      : sys/pdb_cache.c
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// PdbCache implementation. See `Misra/Sys/PdbCache.h` for the contract; this file
/// just sequences PE-open -> CodeView extraction -> PDB-path discovery
/// -> PDB-open -> RVA lookup, and caches results so each subsequent
/// frame in the same module doesn't pay for the open again.

#include <Misra/Sys/PdbCache.h>

#include <Misra/Std.h>
#include <Misra/Std/Container/Str.h>
#include <Misra/Std/File.h>
#include <Misra/Std/Log.h>
#include <Misra/Std/Memory.h>

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

#include "_Helpers.h"

// Find the PDB referenced by a PE's CodeView record. We try, in order:
//   1. The exact path stored in CodeView (build-machine path, usually
//      only useful on the same machine that built the binary).
//   2. Basename of (1) alongside the PE.
//
// On success populates `out_path` (an owned Str the caller frees).
static bool find_pdb(const Pe *pe, Zstr pe_path, Str *out_path) {
    const PeCodeViewInfo *cv = PeCodeView(pe);
    if (!cv->present || !cv->pdb_path)
        return false;

    // (1) exact CodeView path
    *out_path = StrInit(StrAllocator(out_path));
    StrPushBackMany(out_path, cv->pdb_path);
    if (sys_path_exists(StrBegin(out_path)))
        return true;

    // (2) basename alongside PE
    Zstr pdb_base = sys_basename_of(cv->pdb_path);
    if (pdb_base[0] == '\0')
        return false;

    StrResize(out_path, 0);
    sys_append_dirname(out_path, pe_path);
    if (StrLen(out_path) > 0)
        StrPushBackR(out_path, '/');
    StrPushBackMany(out_path, pdb_base);
    if (sys_path_exists(StrBegin(out_path)))
        return true;

    return false;
}

// Open the PE then look for its paired PDB. Either step may fail;
// flags on the cache entry record what succeeded so a second resolve
// of the same module doesn't retry the misses.
static bool entry_open(PdbCacheEntry *entry, Allocator *alloc) {
    if (!entry->pe_open) {
        if (!PeOpen(&entry->pe, &entry->module_path, alloc))
            return false;
        entry->pe_open = true;
    }
    if (entry->pdb_open)
        return true;

    Str pdb_path = StrInit(alloc);
    if (!find_pdb(&entry->pe, StrBegin(&entry->module_path), &pdb_path)) {
        StrDeinit(&pdb_path);
        return false;
    }
    bool ok = PdbOpen(&entry->pdb, &pdb_path, alloc);
    StrDeinit(&pdb_path);
    if (!ok)
        return false;

    // Validate the (GUID, age) pair matches. If the PE and PDB
    // disagree the names are probably stale -- worse than no symbols.
    const PeCodeViewInfo *pe_cv   = PeCodeView(&entry->pe);
    const PdbInfo        *pdb_inf = PdbInfoStream(&entry->pdb);
    if (pe_cv->age != pdb_inf->age || MemCompare(pe_cv->guid, pdb_inf->guid, 16) != 0) {
        LOG_ERROR("PdbCache: GUID/age mismatch between PE and PDB for {}", entry->module_path);
        PdbDeinit(&entry->pdb);
        return false;
    }
    entry->pdb_open = true;
    return true;
}

// Find an existing entry for `module_path` or create a fresh one.
static PdbCacheEntry *cache_find_or_open(PdbCache *self, Zstr module_path) {
    for (size i = 0; i < VecLen(&self->entries); ++i) {
        PdbCacheEntry *e = VecPtrAt(&self->entries, i);
        if (StrBegin(&e->module_path) && ZstrCompare(StrBegin(&e->module_path), module_path) == 0) {
            return e;
        }
    }
    PdbCacheEntry entry;
    MemSet(&entry, 0, sizeof(entry));

    if (!StrTryInitFromCstr(&entry.module_path, module_path, ZstrLen(module_path), self->allocator)) {
        return NULL;
    }

    if (!VecPushBackR(&self->entries, entry)) {
        StrDeinit(&entry.module_path);
        return NULL;
    }
    return VecPtrAt(&self->entries, VecLen(&self->entries) - 1);
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

void PdbCacheDeinit(PdbCache *self) {
    if (!self)
        return;
    for (size i = 0; i < VecLen(&self->entries); ++i) {
        PdbCacheEntry *e = VecPtrAt(&self->entries, i);
        if (e->pdb_open)
            PdbDeinit(&e->pdb);
        if (e->pe_open)
            PeDeinit(&e->pe);
        StrDeinit(&e->module_path);
    }
    VecDeinit(&self->entries);
    MemSet(self, 0, sizeof(*self));
}

bool pdb_cache_resolve_zstr(
    PdbCache *self,
    Zstr      module_path,
    u64       module_base,
    u64       runtime_ip,
    Zstr     *out_name,
    u32      *out_offset
) {
    if (!self || !module_path || !out_name)
        return false;

    PdbCacheEntry *entry = cache_find_or_open(self, module_path);
    if (!entry)
        return false;

    if (!entry_open(entry, self->allocator))
        return false;

    if (runtime_ip < module_base)
        return false;
    u64 rva64 = runtime_ip - module_base;
    if (rva64 > 0xFFFFFFFFu)
        return false;
    u32 rva = (u32)rva64;

    const PdbFunction *f = PdbResolveRva(&entry->pdb, rva);
    if (!f)
        return false;
    *out_name = f->name;
    if (out_offset)
        *out_offset = rva - f->rva;
    return true;
}

bool pdb_cache_resolve_str(
    PdbCache  *self,
    const Str *module_path,
    u64        module_base,
    u64        runtime_ip,
    Zstr      *out_name,
    u32       *out_offset
) {
    if (!module_path) {
        return false;
    }
    return pdb_cache_resolve_zstr(self, StrBegin(module_path), module_base, runtime_ip, out_name, out_offset);
}
