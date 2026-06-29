/// file      : parsers/proc_maps.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Reads and parses `/proc/self/maps` on Linux into a
/// `Vec(ProcMapEntry)`. Used by `Sys/SymbolResolver` to look up which
/// loaded ELF object contains a given runtime address. Going through
/// `/proc/self/maps` keeps the address-to-module mapping aligned with
/// the kernel's own view of the process: it covers every mapped
/// segment regardless of how it was loaded, including images mapped
/// outside the dynamic loader's bookkeeping.
///
/// POSIX (non-Linux) and Windows have no `/proc/self/maps`. They'll
/// need different backends — `dl_iterate_phdr` on glibc / FreeBSD,
/// `EnumProcessModules` on Windows. Tracked in FUTURE-PLANS.md.

#ifndef MISRA_PARSERS_PROC_MAPS_H
#define MISRA_PARSERS_PROC_MAPS_H

#include <Misra/Std/Allocator.h>
#include <Misra/Std/Container/Buf.h>
#include <Misra/Std/Container/Str.h>
#include <Misra/Std/Container/Vec.h>
#include <Misra/Std/File.h>
#include <Misra/Types.h>

typedef enum ProcMapPerms {
    PROC_MAP_PERM_READ    = 1u << 0,
    PROC_MAP_PERM_WRITE   = 1u << 1,
    PROC_MAP_PERM_EXEC    = 1u << 2,
    PROC_MAP_PERM_PRIVATE = 1u << 3, // 'p' (private/copy-on-write); absent means 's' (shared)
} ProcMapPerms;

///
/// One line of `/proc/self/maps`. `path` is borrowed from the
/// `ProcMaps.raw` buffer and stays valid until `ProcMapsDeinit`. May
/// be empty for anonymous mappings (heap, stacks, vdso, etc.).
///
typedef struct ProcMapEntry {
    u64  start;       // runtime virtual address (inclusive)
    u64  end;         // runtime virtual address (exclusive)
    u32  perms;       // bitmask of ProcMapPerms
    u64  file_offset; // offset within the backing file
    Zstr path;        // backing file path, or "" if anonymous
} ProcMapEntry;

typedef Vec(ProcMapEntry) ProcMapEntries;

typedef struct ProcMaps {
    Str            raw;      // owns the raw /proc/self/maps bytes
    ProcMapEntries entries;  // pointers into `raw`
    u64            min_addr; // lowest `start` across all entries (0 if none)
} ProcMaps;

///
/// Read and parse `/proc/self/maps`. The full file is held inside
/// `out->raw` for the lifetime of the ProcMaps so each entry's `path`
/// can borrow from it without a separate copy.
///
/// out[out]   : Populated on success.
/// alloc[in]  : Allocator for the raw buffer and entries vector.
///
/// SUCCESS : Returns true; `out->entries` is populated.
/// FAILURE : Returns false; logs the failing step. `out` is left zeroed.
///
/// TAGS: Sys, Linux, ProcMaps
///
bool proc_maps_load(ProcMaps *out, Allocator *alloc);
#define ProcMapsLoad(...)          OVERLOAD(ProcMapsLoad, __VA_ARGS__)
#define ProcMapsLoad_1(out)        proc_maps_load((out), MisraScope)
#define ProcMapsLoad_2(out, alloc) proc_maps_load((out), ALLOCATOR_OF(alloc))

///
/// Parse `/proc/self/maps`-format text from an in-memory source instead of the
/// live kernel file. The bytes are COPIED into `out->raw`, so each entry's
/// borrowed `path` stays valid for the ProcMaps lifetime. Lets callers parse
/// crafted or captured maps text without a live `/proc` (tests, and future
/// core-file / remote backends).
///
/// `ProcMapsLoadFrom(out, src[, alloc])` dispatches on the source type:
///   - `File *` : read the open file to EOF (caller still owns/closes it)
///   - `Str *`  : parse the string's bytes
///   - `Buf *`  : parse the buffer's bytes
///   - `(Zstr bytes, u64 len, alloc)` : parse `len` bytes at `bytes`
/// The `File`/`Str`/`Buf` forms take an optional trailing allocator (defaulting
/// to `MisraScope`); the raw `(bytes, len)` form takes an explicit allocator.
///
/// out[out]   : Populated on success; left zeroed on failure.
/// alloc[in]  : Allocator for `out->raw` and the entries vector.
///
/// SUCCESS : Returns true; `out->entries` is populated (possibly empty).
/// FAILURE : Returns false; logs the failing step. `out` is left zeroed.
///
/// TAGS: ProcMaps, Parse, API
///
bool proc_maps_load_from_bytes(ProcMaps *out, const u8 *bytes, u64 len, Allocator *alloc);
bool proc_maps_load_from_file(ProcMaps *out, File *f, Allocator *alloc);
#define ProcMapsLoadFrom(...) OVERLOAD(ProcMapsLoadFrom, __VA_ARGS__)
#define ProcMapsLoadFrom_2(out, src)                                                                                   \
    _Generic(                                                                                                          \
        (src),                                                                                                         \
        File *: proc_maps_load_from_file((out), (File *)(src), MisraScope),                                            \
        Str *: proc_maps_load_from_bytes((out), (const u8 *)StrBegin((Str *)(src)), StrLen((Str *)(src)), MisraScope), \
        Buf *: proc_maps_load_from_bytes((out), BufData((Buf *)(src)), BufLength((Buf *)(src)), MisraScope)            \
    )
#define ProcMapsLoadFrom_3(out, src, alloc)                                                                            \
    _Generic(                                                                                                          \
        (src),                                                                                                         \
        File *: proc_maps_load_from_file((out), (File *)(src), ALLOCATOR_OF(alloc)),                                   \
        Str *: proc_maps_load_from_bytes(                                                                              \
                 (out),                                                                                                \
                 (const u8 *)StrBegin((Str *)(src)),                                                                   \
                 StrLen((Str *)(src)),                                                                                 \
                 ALLOCATOR_OF(alloc)                                                                                   \
             ),                                                                                                        \
        Buf *: proc_maps_load_from_bytes((out), BufData((Buf *)(src)), BufLength((Buf *)(src)), ALLOCATOR_OF(alloc))   \
    )
#define ProcMapsLoadFrom_4(out, bytes, len, alloc)                                                                     \
    proc_maps_load_from_bytes((out), (const u8 *)(bytes), (len), ALLOCATOR_OF(alloc))

///
/// Release storage owned by a ProcMaps. Safe on a zeroed struct.
/// Storage is reclaimed through each container's inline allocator —
/// no separate allocator argument is needed.
///
/// SUCCESS : Returns to the caller. `*self` is zeroed.
/// FAILURE : Function cannot fail. NULL `self` is a no-op.
///
/// TAGS: Sys, ProcMaps, Deinit, Lifecycle
///
void ProcMapsDeinit(ProcMaps *self);

///
/// Find the entry whose `[start, end)` range contains `addr`. Linear
/// scan; for a few dozen mappings this is fine.
///
/// SUCCESS : Returns a pointer to the matching entry inside `self`.
/// FAILURE : Returns NULL if `addr` is not in any mapping.
///
/// TAGS: Sys, ProcMaps, Find, Lookup
///
const ProcMapEntry *ProcMapsFindByAddr(const ProcMaps *self, u64 addr);

#endif // MISRA_SYS_PROC_MAPS_H
