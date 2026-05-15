#include <Misra.h>
#include <Misra/Std/Allocator/Default.h>
#include <Misra/Sys/ProcMaps.h>

#include <stdint.h>

#include "../Util/TestRunner.h"

bool test_procmaps_load(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    ProcMaps         maps;

    if (!ProcMapsLoad(&maps, ALLOCATOR_OF(&alloc))) {
        DefaultAllocatorDeinit(&alloc);
        return false;
    }

    // We expect many mappings — at least the binary itself plus libc.
    bool ok = maps.entries.length > 5;

    // At least one entry should be executable (the code section of
    // either the test binary or libc).
    bool any_exec = false;
    for (u64 i = 0; i < maps.entries.length; ++i) {
        if (maps.entries.data[i].perms & PROC_MAP_PERM_EXEC) {
            any_exec = true;
            break;
        }
    }
    ok = ok && any_exec;

    ProcMapsDeinit(&maps);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

bool test_procmaps_find_self(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    ProcMaps         maps;

    if (!ProcMapsLoad(&maps, ALLOCATOR_OF(&alloc))) {
        DefaultAllocatorDeinit(&alloc);
        return false;
    }

    // The address of this function should land inside an executable
    // mapping of the test binary itself.
    u64                 self_addr = (u64)(uintptr_t)&test_procmaps_find_self;
    const ProcMapEntry *entry     = ProcMapsFindByAddr(&maps, self_addr);

    bool ok = entry != NULL && (entry->perms & PROC_MAP_PERM_EXEC) != 0;

    ProcMapsDeinit(&maps);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

int main(void) {
    WriteFmt("[INFO] Starting ProcMaps tests\n\n");

    TestFunction tests[] = {
        test_procmaps_load,
        test_procmaps_find_self,
    };

    return run_test_suite(tests, sizeof(tests) / sizeof(tests[0]), NULL, 0, "ProcMaps");
}
