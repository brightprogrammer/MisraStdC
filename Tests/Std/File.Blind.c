#include <Misra/Std/Allocator/Default.h>
#include <Misra/Std/Container/Str.h>
#include <Misra/Std/File.h>
#include <Misra/Std/Log.h>
#include <Misra/Std/Memory.h>
#include <Misra/Std/Zstr.h>
#include <Misra/Sys/Dir.h>

#include "../Util/TestRunner.h"

// ---------------------------------------------------------------------------
// L219 `if (f->owns && f->fd >= 0)` -- ge_to_lt makes this `f->fd < 0`, which
// is false for every real owned fd, so the actual close() syscall is skipped
// and the fd is leaked while FileClose still returns true and resets the
// struct. The leak is observable through fd-number reuse: POSIX hands out the
// lowest free fd, so closing one file and reopening must reuse its slot. With
// close skipped the slot stays occupied and the reopen lands on a higher fd.
// ---------------------------------------------------------------------------
bool test_close_releases_fd_slot(void) {
    WriteFmt("Testing FileClose actually releases the fd (slot is reused)\n");

    DefaultAllocator alloc      = DefaultAllocatorInit();
    Allocator       *alloc_base = ALLOCATOR_OF(&alloc);

    // Open file A and keep a second file B open so A's fd slot sits in the
    // middle of the table. Record A's fd.
    Str  pa;
    File a = FileOpenTemp(&pa, alloc_base);
    Str  pb;
    File b = FileOpenTemp(&pb, alloc_base);
    if (!FileIsOpen(&a) || !FileIsOpen(&b)) {
        if (FileIsOpen(&a)) {
            FileClose(&a);
            FileRemove(&pa);
            StrDeinit(&pa);
        }
        if (FileIsOpen(&b)) {
            FileClose(&b);
            FileRemove(&pb);
            StrDeinit(&pb);
        }
        DefaultAllocatorDeinit(&alloc);
        return false;
    }

    i32 fd_a = FileFd(&a);

    // Really close A. On clean source this frees fd_a; the ge_to_lt mutant
    // skips close() so the kernel still holds fd_a open.
    bool ok = FileClose(&a);

    // Reopen a fresh temp. POSIX returns the lowest free fd: on clean source
    // that is exactly fd_a (just released); with the mutant fd_a is still
    // occupied so we get a strictly higher number.
    Str  pc;
    File c     = FileOpenTemp(&pc, alloc_base);
    ok         = ok && FileIsOpen(&c);
    i32  fd_c  = FileFd(&c);
    bool reuse = (fd_c == fd_a);

    FileClose(&b);
    FileClose(&c);
    FileRemove(&pa);
    FileRemove(&pb);
    FileRemove(&pc);
    StrDeinit(&pa);
    StrDeinit(&pb);
    StrDeinit(&pc);
    DefaultAllocatorDeinit(&alloc);
    return ok && reuse;
}

int main(void) {
    WriteFmt("[INFO] Starting File.Blind tests\n\n");

    TestFunction tests[] = {
        test_close_releases_fd_slot,
    };
    TestFunction deadend_tests[] = {0};

    return run_test_suite(tests, sizeof(tests) / sizeof(tests[0]), deadend_tests, 0, "File.Blind");
}
