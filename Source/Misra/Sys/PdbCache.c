/// file      : PdbCache.c
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// PdbCache implementation. See PdbCache.h for the contract; this file
/// just sequences PE-open -> CodeView extraction -> PDB-path discovery
/// -> PDB-open -> RVA lookup, and caches results so each subsequent
/// frame in the same module doesn't pay for the open again.

#include <Misra/Sys/PdbCache.h>

#include <Misra/Std.h>
#include <Misra/Std/Container/Str.h>
#include <Misra/Std/File.h>
#include <Misra/Std/File.h>
#include <Misra/Std/Log.h>
#include <Misra/Std/Memory.h>

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static bool path_exists(const char *path) {
    File f = FileOpen(path, "rb");
    if (!FileIsOpen(&f)) {
        return false;
    }
    FileClose(&f);
    return true;
}

// Return the directory portion of `path` (no trailing separator).
// Appended into `out`. If `path` has no separator the function leaves
// `out` empty so the caller can fall back to the same directory.
static void append_dirname(Str *out, const char *path) {
    if (!path)
        return;
    const char *last_sep = NULL;
    for (const char *p = path; *p; ++p) {
        if (*p == '/' || *p == '\\')
            last_sep = p;
    }
    if (!last_sep)
        return;
    u64 len = (u64)(last_sep - path);
    for (u64 i = 0; i < len; ++i)
        StrPushBack(out, path[i]);
}

// Return the basename portion (last component) of `path`. The returned
// pointer is into `path` itself.
static const char *basename_of(const char *path) {
    if (!path)
        return "";
    const char *base = path;
    for (const char *p = path; *p; ++p) {
        if (*p == '/' || *p == '\\')
            base = p + 1;
    }
    return base;
}

// Find the PDB referenced by a PE's CodeView record. We try, in order:
//   1. The exact path stored in CodeView (build-machine path, usually
//      only useful on the same machine that built the binary).
//   2. Basename of (1) alongside the PE.
//
// On success populates `out_path` (an owned Str the caller frees).
static bool find_pdb(const PeFile *pe, const char *pe_path, Str *out_path) {
    if (!pe->codeview.present || !pe->codeview.pdb_path)
        return false;

    // (1) exact CodeView path
    *out_path = StrInit(out_path->allocator);
    StrPushBackZstr(out_path, pe->codeview.pdb_path);
    if (path_exists(out_path->data))
        return true;

    // (2) basename alongside PE
    const char *pdb_base = basename_of(pe->codeview.pdb_path);
    if (pdb_base[0] == '\0')
        return false;

    out_path->length = 0;
    append_dirname(out_path, pe_path);
    if (out_path->length > 0)
        StrPushBack(out_path, '/');
    StrPushBackZstr(out_path, pdb_base);
    if (path_exists(out_path->data))
        return true;

    return false;
}

// Open the PE then look for its paired PDB. Either step may fail;
// flags on the cache entry record what succeeded so a second resolve
// of the same module doesn't retry the misses.
static bool entry_open(PdbCacheEntry *entry, Allocator *alloc) {
    if (!entry->pe_open) {
        if (!PeFileOpen(&entry->pe, entry->module_path, alloc))
            return false;
        entry->pe_open = true;
    }
    if (entry->pdb_open)
        return true;

    Str pdb_path = StrInit(alloc);
    if (!find_pdb(&entry->pe, entry->module_path, &pdb_path)) {
        StrDeinit(&pdb_path);
        return false;
    }
    bool ok = PdbFileOpen(&entry->pdb, pdb_path.data, alloc);
    StrDeinit(&pdb_path);
    if (!ok)
        return false;

    // Validate the (GUID, age) pair matches. If the PE and PDB
    // disagree the names are probably stale -- worse than no symbols.
    if (entry->pe.codeview.age != entry->pdb.info.age ||
        MemCompare(entry->pe.codeview.guid, entry->pdb.info.guid, 16) != 0) {
        LOG_ERROR("PdbCache: GUID/age mismatch between PE and PDB for {}", entry->module_path);
        PdbFileDeinit(&entry->pdb);
        return false;
    }
    entry->pdb_open = true;
    return true;
}

// Find an existing entry for `module_path` or create a fresh one.
static PdbCacheEntry *cache_find_or_open(PdbCache *self, const char *module_path) {
    for (size i = 0; i < self->entries.length; ++i) {
        PdbCacheEntry *e = &self->entries.data[i];
        if (e->module_path && ZstrCompare(e->module_path, module_path) == 0) {
            return e;
        }
    }
    PdbCacheEntry entry;
    MemSet(&entry, 0, sizeof(entry));

    u64         path_len = 0;
    const char *p        = module_path;
    while (*p) {
        ++path_len;
        ++p;
    }
    entry.module_path = AllocatorAlloc(self->allocator, path_len + 1, /*zeroed=*/0);
    if (!entry.module_path)
        return NULL;
    MemCopy(entry.module_path, module_path, path_len + 1);

    if (!VecPushBackR(&self->entries, entry)) {
        AllocatorFree(self->allocator, entry.module_path);
        return NULL;
    }
    return &self->entries.data[self->entries.length - 1];
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

bool pdb_cache_init(PdbCache *out, Allocator *alloc) {
    if (!out || !alloc)
        return false;
    MemSet(out, 0, sizeof(*out));
    out->allocator = alloc;
    out->entries   = VecInitT(out->entries, alloc);
    return true;
}

void PdbCacheDeinit(PdbCache *self) {
    if (!self)
        return;
    for (size i = 0; i < self->entries.length; ++i) {
        PdbCacheEntry *e = &self->entries.data[i];
        if (e->pdb_open)
            PdbFileDeinit(&e->pdb);
        if (e->pe_open)
            PeFileDeinit(&e->pe);
        if (e->module_path && self->allocator) {
            u64 n = 0;
            for (const char *p = e->module_path; *p; ++p)
                ++n;
            AllocatorFree(self->allocator, e->module_path);
        }
    }
    VecDeinit(&self->entries);
    MemSet(self, 0, sizeof(*self));
}

bool PdbCacheResolve(
    PdbCache    *self,
    const char  *module_path,
    u64          module_base,
    u64          runtime_ip,
    const char **out_name,
    u32         *out_offset
) {
    if (!self || !module_path || !out_name)
        return false;

    PdbCacheEntry *entry = cache_find_or_open(self, module_path);
    if (!entry)
        return false;
    entry->module_base = module_base;

    if (!entry_open(entry, self->allocator))
        return false;

    if (runtime_ip < module_base)
        return false;
    u64 rva64 = runtime_ip - module_base;
    if (rva64 > 0xFFFFFFFFu)
        return false;
    u32 rva = (u32)rva64;

    const PdbFunction *f = PdbFileResolveRva(&entry->pdb, rva);
    if (!f)
        return false;
    *out_name = f->name;
    if (out_offset)
        *out_offset = rva - f->rva;
    return true;
}
