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
/// The unit under test is source-included ONCE so the cache-entry internals
/// (`ResolverCacheEntry`, the cache vector) are visible for hand-building the
/// teardown fixture. The public symbols come from the same include, so the
/// linker never pulls the library object (no duplicate definitions).

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
        test_srmut_deinit_frees_sidecar,
    };

    return run_test_suite(tests, sizeof(tests) / sizeof(tests[0]), NULL, 0, "SymbolResolver.Mut");
}
