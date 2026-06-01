/// file      : sys/pdb_cache.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Portable PE-binary -> PDB-symbol resolver. Given a runtime IP plus
/// the `(module_path, module_base)` pair the OS knows about for that
/// IP, this cache:
///
///   1. Opens the PE on first sight of `module_path` and stashes it.
///   2. Extracts the CodeView record (PDB path + GUID + age).
///   3. Locates the matching `.pdb` -- tries the path recorded in
///      CodeView, then falls back to the same directory as the PE
///      using the recorded basename.
///   4. Opens the PDB, builds the function-name table, and stashes
///      it next to the PE.
///   5. Resolves subsequent IPs via `PdbResolveRva`.
///
/// The whole chain is in C: no `dbghelp`, no `<windows.h>`. The
/// caller is responsible for figuring out `(module_path, module_base)`
/// for an arbitrary IP -- on Windows that's `GetModuleHandle` +
/// `GetModuleFileNameA`; this header doesn't touch the OS itself.

#ifndef MISRA_SYS_PDB_CACHE_H
#define MISRA_SYS_PDB_CACHE_H

#include <Misra/Parsers/Pdb.h>
#include <Misra/Parsers/Pe.h>
#include <Misra/Std/Allocator.h>
#include <Misra/Std/Container/Str/Type.h>
#include <Misra/Std/Container/Vec.h>
#include <Misra/Types.h>

typedef struct PdbCacheEntry {
    Str  module_path; // owned; cleaned via StrDeinit
    u64  module_base; // last-seen runtime load base
    Pe   pe;
    Pdb  pdb;
    bool pe_open;
    bool pdb_open;
} PdbCacheEntry;

typedef Vec(PdbCacheEntry) PdbCacheEntries;

typedef struct PdbCache {
    Allocator      *allocator;
    PdbCacheEntries entries;
} PdbCache;

///
/// Initialize an empty cache. The allocator argument is optional
/// inside a `Scope` block (defaults to `MisraScope`).
///
/// SUCCESS : Yields a `PdbCache` whose `entries` Vec is empty and
///           ready for `Resolve`.
/// FAILURE : Cannot fail at construction; first allocator OOM
///           surfaces from later `entries` growth.
///
/// TAGS: Sys, PDB, Cache, Init, Lifecycle
///
#define PdbCacheInit(...)        OVERLOAD(PdbCacheInit, __VA_ARGS__)
#define PdbCacheInit_0()         PdbCacheInit_1(MisraScope)
#define PdbCacheInit_1(alloc_ptr) ((PdbCache) {.allocator = ALLOCATOR_OF(alloc_ptr), .entries = VecInit_1(alloc_ptr)})

///
/// Tear down the cache, releasing every cached `Pe` and `Pdb`.
/// Safe on a zeroed struct.
///
/// SUCCESS : Returns to the caller. `self` is zeroed.
/// FAILURE : Function cannot fail.
///
/// TAGS: Sys, PDB, Cache, Deinit, Lifecycle
///
void PdbCacheDeinit(PdbCache *self);

///
/// Resolve `runtime_ip` to a function name.
///
/// self[in,out]     : Cache. Grows on first sight of `module_path`.
/// module_path[in]  : Path to the loaded PE for the address being
///                    resolved. Caller-owned; we copy it.
/// module_base[in]  : Runtime virtual address of the module's load
///                    point. `ip - module_base` is the RVA.
/// runtime_ip[in]   : Address to resolve.
/// out_name[out]    : On success, pointer to the function name
///                    (borrowed from the cached PDB; valid until the
///                    next `PdbCacheDeinit`).
/// out_offset[out]  : On success, byte offset from the function start
///                    (`rva - function.rva`).
///
/// SUCCESS : Returns true.
/// FAILURE : Returns false if the module can't be opened, no PDB
///           pairs with it, or the RVA falls outside every public
///           function.
///
/// TAGS: Sys, PDB, Cache, Resolve
///
bool pdb_cache_resolve_zstr(
    PdbCache *self,
    Zstr module_path,
    u64       module_base,
    u64       runtime_ip,
    Zstr *out_name,
    u32      *out_offset
);
bool pdb_cache_resolve_str(
    PdbCache  *self,
    const Str *module_path,
    u64        module_base,
    u64        runtime_ip,
    Zstr *out_name,
    u32       *out_offset
);
#define PdbCacheResolve(self, module_path, module_base, runtime_ip, out_name, out_offset)                              \
    _Generic(                                                                                                          \
        (module_path),                                                                                                 \
        Str *: pdb_cache_resolve_str((self), (const Str *)(module_path), (module_base), (runtime_ip), (out_name), (out_offset)),  \
        Zstr: pdb_cache_resolve_zstr((self), (Zstr)(module_path), (module_base), (runtime_ip), (out_name), (out_offset)),         \
        char *: pdb_cache_resolve_zstr((self), (Zstr)(module_path), (module_base), (runtime_ip), (out_name), (out_offset))        \
    )

#endif // MISRA_SYS_PDB_CACHE_H
