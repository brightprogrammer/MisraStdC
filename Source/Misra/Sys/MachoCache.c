/// file      : MachoCache.c
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// MachoCache: portable resolver that sits on top of Parsers/MachO and
/// Parsers/Dwarf. Sequences binary-open -> dSYM discovery -> UUID
/// validation -> symbol/DWARF lookup, caching results so repeated
/// frames in the same module don't pay for the open again.

#include <Misra/Sys/MachoCache.h>

#include <Misra/Std.h>
#include <Misra/Std/Container/Str.h>
#include <Misra/Std/Log.h>
#include <Misra/Std/File.h>
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

static const char *basename_of(const char *path) {
    if (!path)
        return "";
    const char *base = path;
    for (const char *p = path; *p; ++p) {
        if (*p == '/')
            base = p + 1;
    }
    return base;
}

// Compose the conventional dSYM location for `binary_path`:
//   <binary_path>.dSYM/Contents/Resources/DWARF/<basename>
static bool compose_dsym_path(const char *binary_path, Str *out) {
    if (!binary_path)
        return false;
    const char *base = basename_of(binary_path);
    if (base[0] == '\0')
        return false;
    out->length = 0;
    StrPushBackZstr(out, binary_path);
    StrPushBackZstr(out, ".dSYM/Contents/Resources/DWARF/");
    StrPushBackZstr(out, base);
    return true;
}

// ---------------------------------------------------------------------------
// Cache lifecycle
// ---------------------------------------------------------------------------

static MachoCacheEntry *cache_find_or_create(MachoCache *self, const char *module_path) {
    for (size i = 0; i < self->entries.length; ++i) {
        MachoCacheEntry *e = &self->entries.data[i];
        if (e->module_path && ZstrCompare(e->module_path, module_path) == 0) {
            return e;
        }
    }
    MachoCacheEntry entry;
    MemSet(&entry, 0, sizeof(entry));

    u64         path_len = 0;
    const char *p        = module_path;
    while (*p) {
        ++path_len;
        ++p;
    }
    entry.module_path = AllocatorAlloc(self->allocator, path_len + 1, 0);
    if (!entry.module_path)
        return NULL;
    MemCopy(entry.module_path, module_path, path_len + 1);

    if (!VecPushBackR(&self->entries, entry)) {
        AllocatorFree(self->allocator, entry.module_path);
        return NULL;
    }
    return &self->entries.data[self->entries.length - 1];
}

// Open the main Mach-O if not yet open. Returns false on persistent
// failure (file not found, malformed Mach-O, etc.).
static bool entry_open_main(MachoCacheEntry *e, Allocator *alloc) {
    if (e->main_open)
        return true;
    if (!MachoFileOpen(&e->main, e->module_path, alloc))
        return false;
    e->main_open = true;
    return true;
}

// Locate and open the dSYM, validating UUID match against the main.
// Idempotent; subsequent calls are no-ops if already attempted.
static bool entry_open_dsym(MachoCacheEntry *e, Allocator *alloc) {
    if (e->dsym_open)
        return true;
    if (!e->main_open)
        return false;
    if (!e->main.has_uuid) {
        // Without a UUID we can't validate a dSYM pairing -- bail.
        return false;
    }

    Str path = StrInit(alloc);
    if (!compose_dsym_path(e->module_path, &path)) {
        StrDeinit(&path);
        return false;
    }
    if (!path_exists(path.data) || !MachoFileOpen(&e->dsym, path.data, alloc)) {
        StrDeinit(&path);
        return false;
    }
    StrDeinit(&path);

    if (!e->dsym.has_uuid || MemCompare(e->dsym.uuid, e->main.uuid, 16) != 0) {
        LOG_ERROR("MachoCache: dSYM UUID mismatch for {}", e->module_path);
        MachoFileDeinit(&e->dsym);
        return false;
    }
    e->dsym_open = true;
    return true;
}

// Build DwarfFunctions over the dSYM's __DWARF sections. Idempotent.
static bool entry_build_dwarf(MachoCacheEntry *e, Allocator *alloc) {
    if (e->fns_built)
        return e->fns_ok;
    e->fns_built = true;
    if (!e->dsym_open)
        return false;

    const MachoSection *info_sec   = MachoFileFindSection(&e->dsym, "__DWARF", "__debug_info");
    const MachoSection *abbrev_sec = MachoFileFindSection(&e->dsym, "__DWARF", "__debug_abbrev");
    const MachoSection *str_sec    = MachoFileFindSection(&e->dsym, "__DWARF", "__debug_str");

    const u8 *info_b   = info_sec ? e->dsym.data + info_sec->offset : NULL;
    u64       info_n   = info_sec ? info_sec->size : 0;
    const u8 *abbrev_b = abbrev_sec ? e->dsym.data + abbrev_sec->offset : NULL;
    u64       abbrev_n = abbrev_sec ? abbrev_sec->size : 0;
    const u8 *str_b    = str_sec ? e->dsym.data + str_sec->offset : NULL;
    u64       str_n    = str_sec ? str_sec->size : 0;

    e->fns_ok = DwarfFunctionsBuildFromSlices(&e->fns, info_b, info_n, abbrev_b, abbrev_n, str_b, str_n, alloc);
    return e->fns_ok;
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

bool macho_cache_init(MachoCache *out, Allocator *alloc) {
    if (!out || !alloc)
        return false;
    MemSet(out, 0, sizeof(*out));
    out->allocator = alloc;
    out->entries   = VecInitT(out->entries, alloc);
    return true;
}

void MachoCacheDeinit(MachoCache *self) {
    if (!self)
        return;
    for (size i = 0; i < self->entries.length; ++i) {
        MachoCacheEntry *e = &self->entries.data[i];
        if (e->fns_built && e->fns_ok)
            DwarfFunctionsDeinit(&e->fns);
        if (e->dsym_open)
            MachoFileDeinit(&e->dsym);
        if (e->main_open)
            MachoFileDeinit(&e->main);
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

bool MachoCacheResolve(
    MachoCache  *self,
    const char  *module_path,
    u64          slide,
    u64          runtime_ip,
    const char **out_name,
    u32         *out_offset
) {
    if (!self || !module_path || !out_name)
        return false;

    MachoCacheEntry *entry = cache_find_or_create(self, module_path);
    if (!entry)
        return false;
    entry->slide = slide;

    if (!entry_open_main(entry, self->allocator))
        return false;

    if (runtime_ip < slide)
        return false;
    u64 file_relative = runtime_ip - slide;

    // (1) Main file's LC_SYMTAB.
    const MachoSymbol *s = MachoFileResolveAddress(&entry->main, file_relative);
    if (s && s->name && s->name[0]) {
        *out_name = s->name;
        if (out_offset)
            *out_offset = (u32)(file_relative - s->value);
        return true;
    }

    // (2) Sidecar dSYM's LC_SYMTAB.
    if (entry_open_dsym(entry, self->allocator)) {
        s = MachoFileResolveAddress(&entry->dsym, file_relative);
        if (s && s->name && s->name[0]) {
            *out_name = s->name;
            if (out_offset)
                *out_offset = (u32)(file_relative - s->value);
            return true;
        }

        // (3) dSYM's DWARF .debug_info function table.
        if (entry_build_dwarf(entry, self->allocator)) {
            const DwarfFunction *f = DwarfFunctionsResolve(&entry->fns, file_relative);
            if (f && f->name) {
                *out_name = f->name;
                if (out_offset)
                    *out_offset = (u32)(file_relative - f->low_pc);
                return true;
            }
        }
    }

    return false;
}
