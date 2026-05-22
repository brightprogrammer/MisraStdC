/// file      : MachoCache.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Portable Mach-O + dSYM resolver. Given a runtime IP plus the
/// `(module_path, slide)` pair the dyld API reports, this cache:
///
///   1. Opens the binary on first sight, parsing its Mach-O headers.
///   2. Looks for a paired dSYM at the conventional location
///      (`<binary_path>.dSYM/Contents/Resources/DWARF/<basename>`).
///   3. Validates the dSYM's `LC_UUID` matches the binary's; a
///      mismatch means stale debug data and the cache refuses it.
///   4. Resolves the file-relative VA against the binary's symtab,
///      the dSYM's symtab, or the dSYM's `__DWARF,__debug_info`
///      function table -- in that order.
///
/// The cache itself is C99 -- no Darwin APIs. The macOS Backtrace
/// path provides the small shim that asks dyld for
/// `(_dyld_get_image_name, _dyld_get_image_vmaddr_slide)` per IP.

#ifndef MISRA_SYS_MACHO_CACHE_H
#define MISRA_SYS_MACHO_CACHE_H

#include <Misra/Parsers/Dwarf.h>
#include <Misra/Parsers/MachO.h>
#include <Misra/Std/Allocator.h>
#include <Misra/Std/Container/Str/Type.h>
#include <Misra/Std/Container/Vec.h>
#include <Misra/Types.h>

typedef struct MachoCacheEntry {
    Str            module_path;
    u64            slide;
    Macho          main;
    bool           main_open;
    Macho          dsym;
    bool           dsym_open;
    DwarfFunctions fns;
    bool           fns_built;
    bool           fns_ok;
} MachoCacheEntry;

typedef Vec(MachoCacheEntry) MachoCacheEntries;

typedef struct MachoCache {
    Allocator        *allocator;
    MachoCacheEntries entries;
} MachoCache;

///
/// Initialise an empty Mach-O symbol cache.
///
/// out[out]   : Cache to initialise.
/// alloc[in]  : Allocator used for the entries vector and for
///              every Mach-O / DWARF table the cache grows lazily.
///
/// SUCCESS : Returns true. `out` is a usable empty cache.
/// FAILURE : Returns false on allocator OOM. `out` is left zeroed.
///
bool macho_cache_init(MachoCache *out, Allocator *alloc);
#define MachoCacheInit(...)          MISRA_OVERLOAD(MachoCacheInit, __VA_ARGS__)
#define MachoCacheInit_1(out)        macho_cache_init((out), MisraScope)
#define MachoCacheInit_2(out, alloc) macho_cache_init((out), ALLOCATOR_OF(alloc))

///
/// Release every cached Mach-O / DWARF table and the entries vector.
/// Safe on a partially-initialised cache.
///
/// SUCCESS : Returns to the caller. `self` is zeroed.
/// FAILURE : Function cannot fail.
///
void MachoCacheDeinit(MachoCache *self);

///
/// Resolve `runtime_ip` to a function name.
///
/// self[in,out]     : Cache. Grows on first sight of `module_path`.
/// module_path[in]  : Path to the loaded Mach-O on disk.
/// slide[in]        : ASLR slide as reported by
///                    `_dyld_get_image_vmaddr_slide` -- the offset
///                    added to the binary's on-disk vmaddrs to get
///                    its runtime addresses. `runtime_ip - slide` is
///                    the file-relative VA.
/// runtime_ip[in]   : Address to resolve.
/// out_name[out]    : On success, pointer to the function name
///                    (borrowed from the cached Mach-O / DWARF
///                    strings; valid until `MachoCacheDeinit`).
/// out_offset[out]  : On success, byte offset from the function start.
///
/// SUCCESS : Returns true.
/// FAILURE : Returns false if the module can't be opened or the IP
///           falls outside any symbol / function.
///
bool macho_cache_resolve_zstr(
    MachoCache *self,
    Zstr        module_path,
    u64         slide,
    u64         runtime_ip,
    Zstr       *out_name,
    u32        *out_offset
);
bool macho_cache_resolve_str(
    MachoCache *self,
    const Str  *module_path,
    u64         slide,
    u64         runtime_ip,
    Zstr       *out_name,
    u32        *out_offset
);
#define MachoCacheResolve(self, module_path, slide, runtime_ip, out_name, out_offset)                                                                                        \
    _Generic((module_path), Str *: macho_cache_resolve_str, const Str *: macho_cache_resolve_str, char *: macho_cache_resolve_zstr, const char *: macho_cache_resolve_zstr)( \
        (self),                                                                                                                                                              \
        (module_path),                                                                                                                                                       \
        (slide),                                                                                                                                                             \
        (runtime_ip),                                                                                                                                                        \
        (out_name),                                                                                                                                                          \
        (out_offset)                                                                                                                                                         \
    )

#endif // MISRA_SYS_MACHO_CACHE_H
