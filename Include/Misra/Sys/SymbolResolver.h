/// file      : SymbolResolver.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// In-tree `dladdr` replacement. Given a runtime instruction pointer,
/// resolves it to a `{module, symbol, offset}` triple using two
/// building blocks:
///
///   - `Sys/ProcMaps` to find which loaded ELF object contains the
///     address (and at what base it was loaded).
///   - `Parsers/Elf` to resolve the file-relative address to a symbol
///     entry from `.symtab` (static + global) and `.dynsym` (exported).
///
/// The resolver owns a cache of opened `ElfFile`s so repeated calls
/// don't re-parse the same shared object. The cache is keyed by file
/// path borrowed from the underlying `ProcMaps`.
///
/// Why not libc `dladdr`? libc only walks `.dynsym`, so static
/// functions resolve as "module+offset" without a name. Our version
/// reads `.symtab` too, which is where the names actually live in
/// non-stripped binaries.

#ifndef MISRA_SYS_SYMBOL_RESOLVER_H
#define MISRA_SYS_SYMBOL_RESOLVER_H

#include <Misra/Parsers/Elf.h>
#include <Misra/Std/Allocator.h>
#include <Misra/Std/Container/Vec.h>
#include <Misra/Sys/ProcMaps.h>
#include <Misra/Types.h>

///
/// Per-resolve output. All string fields are borrowed from internal
/// state and remain valid until the next call (which may rebuild the
/// cache) or `SymbolResolverDestroy`.
///
/// FIELDS:
/// - module_path  : Backing file of the loaded ELF that contains
///                  `addr`, or NULL if no mapping was found.
/// - module_base  : Lowest mapped virtual address for that file.
/// - symbol_name  : Name of the enclosing symbol, or NULL if the
///                  address landed inside the module but outside any
///                  named symbol's range.
/// - symbol_value : `st_value` of the matching symbol (file-relative).
/// - symbol_size  : `st_size` of the matching symbol.
/// - offset       : `addr` minus the start of the matching symbol. If
///                  no symbol matched, the offset from `module_base`.
///
typedef struct ResolvedSymbol {
    const char *module_path;
    u64         module_base;
    const char *symbol_name;
    u64         symbol_value;
    u64         symbol_size;
    u64         offset;
} ResolvedSymbol;

typedef struct ResolverCacheEntry {
    const char *path; // borrowed from ProcMaps.raw
    u64         load_base;
    ElfFile     elf;
} ResolverCacheEntry;

typedef Vec(ResolverCacheEntry) ResolverCache;

typedef struct SymbolResolver {
    Allocator    *allocator;
    ProcMaps      maps;
    ResolverCache cache;
} SymbolResolver;

///
/// Initialize a `SymbolResolver` for the current process. Loads
/// `/proc/self/maps` once at create time. New ELF files are opened
/// lazily on demand and cached for the resolver's lifetime.
///
/// out[out]   : Populated on success.
/// alloc[in]  : Allocator backing the ProcMaps, the cache, and any
///              opened ELF files. Must outlive the resolver.
///
/// SUCCESS : Returns true.
/// FAILURE : Returns false if `/proc/self/maps` can't be loaded.
///           `out` is left zeroed.
///
/// TAGS: Sys, Symbol, Resolver
///
bool SymbolResolverInit(SymbolResolver *out, Allocator *alloc);

///
/// Tear down the resolver, closing every cached `ElfFile` and freeing
/// the cache + ProcMaps. Safe on a zeroed struct.
///
void SymbolResolverDeinit(SymbolResolver *self);

///
/// Resolve a runtime instruction pointer to a symbol.
///
/// self[in,out]      : Resolver. The cache may grow on this call.
/// runtime_addr[in]  : Address to resolve, as captured at runtime.
/// out[out]          : Populated on success. Untouched on failure.
///
/// SUCCESS : Returns true. `out->module_path` is set; `out->symbol_name`
///           may still be NULL if the address falls in a module range
///           that lacks symbol coverage there.
/// FAILURE : Returns false if no loaded module contains `runtime_addr`,
///           or if the resolver fails to open the backing ELF file.
///
/// TAGS: Sys, Symbol, Resolver
///
bool SymbolResolverResolve(SymbolResolver *self, void *runtime_addr, ResolvedSymbol *out);

#endif // MISRA_SYS_SYMBOL_RESOLVER_H
