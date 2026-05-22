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
/// The resolver owns a cache of opened `Elf`s so repeated calls
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
#if FEATURE_PARSER_DWARF
#    include <Misra/Parsers/Dwarf.h>
#endif
#include <Misra/Std/Allocator.h>
#include <Misra/Std/Container/Vec.h>
#include <Misra/Sys/ProcMaps.h>
#include <Misra/Types.h>

///
/// Per-resolve output. All string fields are borrowed from internal
/// state and remain valid until the next call (which may rebuild the
/// cache) or `SymbolResolverDeinit`.
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
/// - source_file  : When `FEATURE_PARSER_DWARF` is on and the
///                  module ships `.debug_line` data we understand,
///                  this is the source file containing `addr`.
///                  NULL otherwise.
/// - source_dir   : Compilation directory hint paired with
///                  `source_file`. May be NULL.
/// - source_line  : 1-based source line, or 0 if unknown.
/// - source_column: 1-based source column, or 0 if unknown.
///
typedef struct ResolvedSymbol {
    Zstr module_path;
    u64  module_base;
    Zstr symbol_name;
    u64  symbol_value;
    u64  symbol_size;
    u64  offset;
    Zstr source_file;
    Zstr source_dir;
    u32  source_line;
    u32  source_column;
} ResolvedSymbol;

typedef struct ResolverCacheEntry {
    Zstr path; // borrowed from ProcMaps.raw
    u64  load_base;
    Elf  elf;
    // Sidecar debug file found via .gnu_debuglink or .note.gnu.build-id.
    // Populated lazily for stripped binaries that have an installed
    // -dbg package or a debug file alongside them. When `has_sidecar`
    // is true, the sidecar's symbol tables (and DWARF lines, below)
    // are searched after the main file's.
    Elf  sidecar;
    bool has_sidecar;
#if FEATURE_PARSER_DWARF
    DwarfLines dwarf;
    bool       dwarf_built;
    bool       dwarf_ok;
    DwarfLines sidecar_dwarf;
    bool       sidecar_dwarf_built;
    bool       sidecar_dwarf_ok;
    // Lazily-parsed .eh_frame for the CFI-based unwinder.
    DwarfCfi cfi;
    bool     cfi_built;
    bool     cfi_ok;
    // Lazily-parsed .debug_info function-name table, used as a fallback
    // when .symtab and .dynsym yield no name. Built for both main and
    // sidecar if either lookup misses.
    DwarfFunctions functions;
    bool           functions_built;
    bool           functions_ok;
    DwarfFunctions sidecar_functions;
    bool           sidecar_functions_built;
    bool           sidecar_functions_ok;
#endif
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
bool symbol_resolver_init(SymbolResolver *out, Allocator *alloc);
#define SymbolResolverInit(...)          MISRA_OVERLOAD(SymbolResolverInit, __VA_ARGS__)
#define SymbolResolverInit_1(out)        symbol_resolver_init((out), MisraScope)
#define SymbolResolverInit_2(out, alloc) symbol_resolver_init((out), ALLOCATOR_OF(alloc))

///
/// Tear down the resolver, closing every cached `Elf` and freeing
/// the cache + ProcMaps. Safe on a zeroed struct.
///
/// SUCCESS : Returns to the caller. `self` is zeroed.
/// FAILURE : Function cannot fail.
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

#if FEATURE_PARSER_DWARF
///
/// Look up the .eh_frame FDE that describes how to unwind through the
/// function at `runtime_addr`. Populates `*out_cfi`, `*out_fde`, and
/// the module's runtime load base so the caller can run the CFI VM
/// (via `DwarfCfiBuildRow`) and compute a CFA in the runtime address
/// space.
///
/// SUCCESS : Returns true; all three output parameters set.
/// FAILURE : Returns false when `runtime_addr` falls outside any
///           loaded module, the module has no `.eh_frame`, or no FDE
///           covers the address.
///
/// TAGS: Sys, Symbol, Unwind
///
bool SymbolResolverFindFde(
    SymbolResolver  *self,
    void            *runtime_addr,
    const DwarfCfi **out_cfi,
    const DwarfFde **out_fde,
    u64             *out_module_base
);
#endif

#endif // MISRA_SYS_SYMBOL_RESOLVER_H
