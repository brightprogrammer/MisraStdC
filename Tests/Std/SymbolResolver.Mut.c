/// file      : Tests/Std/SymbolResolver.Mut.c
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Mutation-survivor kills for `Sys/SymbolResolver.c` that hold a NORMAL
/// mull baseline (stable under -O0 + the IR plugin). Build-sensitive
/// kills (exact `.eh_frame` FDE coverage, DWARF function/line resolution)
/// live in `SymbolResolver.BestEffort.c`, which mull auto-skips; the
/// matching survivors there are ledgered bucket-B.
///
/// The unit under test is source-included ONCE so the static cache helper
/// `resolver_cache_find_or_open` and the cache-entry internals are visible.
/// The public symbols come from the same include, so the linker never
/// pulls the library object (no duplicate definitions).

#include <Misra.h>
#include <Misra/Std/Allocator/Debug.h>
#include <Misra/Std/Allocator/Default.h>
#include <Misra/Std/Container/Buf.h>
#include <Misra/Std/File.h>
#include <Misra/Std/Zstr.h>
#include <Misra/Sys/SymbolResolver.h>

#include "../Util/TestRunner.h"

#include "../../Source/Misra/Sys/SymbolResolver.c"

// Copy /proc/self/exe to `dst` so we have a genuine, openable ELF on disk.
static bool copy_self_exe(Zstr dst) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Buf              bytes = BufInit(ALLOCATOR_OF(&alloc));
    bool             ok    = false;
    if (FileReadAndClose("/proc/self/exe", &bytes) >= 0)
        ok = FileWriteAndClose(dst, BufData(&bytes), BufLength(&bytes)) >= 0;
    BufDeinit(&bytes);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// ---------------------------------------------------------------------------
// resolver_cache_find_or_open — ZstrCompare string-equality fallback
//
// The find loop first compares `e->path == path` by POINTER (l.144). When
// two `/proc/self/maps` lines carry the same path text in different raw-
// buffer positions the pointers differ, so the loop falls through to the
// `ZstrCompare(e->path, path) == 0` fallback (l.150) to still treat them as
// the same module and reuse the cached entry. We drive that fallback with
// two distinct char buffers holding identical path text: the second lookup
// must HIT the first entry (one cache slot, same returned pointer, module
// not re-opened). A `cxx_replace_scalar_call` that forces ZstrCompare to a
// non-zero constant breaks the `== 0` test -> the second buffer MISSES and
// a duplicate entry is opened (cache len 2), which this test catches.
// ---------------------------------------------------------------------------
bool test_srmut_cache_zstrcompare_fallback_hits(void) {
    Zstr file = "/tmp/sr_mut_zcmp";
    if (!copy_self_exe(file))
        return false;

    DebugAllocator alloc = DebugAllocatorInit();
    SymbolResolver res;
    if (!SymbolResolverInit(&res, ALLOCATOR_OF(&alloc)))
        return false;

    // Two separate heap buffers with byte-identical path text -> distinct
    // pointers, so the pointer compare (l.144) misses and the ZstrCompare
    // fallback (l.150) is the only thing that can match them.
    Str buf1 = StrInit(ALLOCATOR_OF(&alloc));
    Str buf2 = StrInit(ALLOCATOR_OF(&alloc));
    StrPushBackMany(&buf1, file);
    StrPushBackR(&buf1, '\0');
    StrPushBackMany(&buf2, file);
    StrPushBackR(&buf2, '\0');
    Zstr p1 = StrBegin(&buf1);
    Zstr p2 = StrBegin(&buf2);

    bool ok = (p1 != p2); // genuinely distinct pointers

    // Miss -> opens + inserts entry with path == p1.
    ResolverCacheEntry *e1 = resolver_cache_find_or_open(&res, p1);
    ok                     = ok && e1 != NULL;
    ok                     = ok && VecLen(&res.cache) == 1;

    // Lookup with the OTHER buffer: pointer differs from p1, so only the
    // ZstrCompare fallback can recognise it as the same module.
    ResolverCacheEntry *e2 = resolver_cache_find_or_open(&res, p2);
    ok                     = ok && e2 == e1;                // same cached entry reused
    ok                     = ok && VecLen(&res.cache) == 1; // no duplicate open

    StrDeinit(&buf1);
    StrDeinit(&buf2);
    SymbolResolverDeinit(&res);
    DebugAllocatorDeinit(&alloc);
    return ok;
}

// ---------------------------------------------------------------------------
// SymbolResolverDeinit — the sidecar ElfDeinit arm frees the sidecar ELF
//
// When a cache entry carries an opened sidecar (`has_sidecar == true`) the
// teardown loop runs `ElfDeinit(&e->sidecar)` (l.218) before closing the
// main ELF. We craft an entry by hand: a real opened main ELF AND a real
// opened sidecar ELF, mark `has_sidecar`, push it, then Deinit and require
// the DebugAllocator live count to return to baseline. A
// `cxx_remove_void_call` that drops the sidecar `ElfDeinit` leaks the
// sidecar's tables -> live count stays above baseline.
// ---------------------------------------------------------------------------
bool test_srmut_deinit_frees_sidecar(void) {
    Zstr file = "/tmp/sr_mut_side";
    if (!copy_self_exe(file))
        return false;

    DebugAllocator alloc    = DebugAllocatorInit();
    size           baseline = DebugAllocatorLiveCount(&alloc);

    SymbolResolver res;
    if (!SymbolResolverInit(&res, ALLOCATOR_OF(&alloc)))
        return false;

    ResolverCacheEntry entry;
    MemSet(&entry, 0, sizeof(entry));
    entry.path = file;

    bool ok = ElfOpen(&entry.elf, file, ALLOCATOR_OF(&alloc));
    if (ok)
        ok = ElfOpen(&entry.sidecar, file, ALLOCATOR_OF(&alloc));
    entry.has_sidecar = true;
    if (ok)
        ok = VecPushBackR(&res.cache, entry);

    // A populated cache with two opened ELFs sits strictly above baseline.
    ok = ok && DebugAllocatorLiveCount(&alloc) > baseline;

    SymbolResolverDeinit(&res);
    // Correct teardown (main + sidecar ElfDeinit + Vec free) returns to 0.
    ok = ok && DebugAllocatorLiveCount(&alloc) == baseline;

    DebugAllocatorDeinit(&alloc);
    return ok;
}

int main(void) {
    WriteFmt("[INFO] Starting SymbolResolver.Mut tests\n\n");

    TestFunction tests[] = {
        test_srmut_cache_zstrcompare_fallback_hits,
        test_srmut_deinit_frees_sidecar,
    };

    return run_test_suite(tests, sizeof(tests) / sizeof(tests[0]), NULL, 0, "SymbolResolver.Mut");
}
