/// file : tests/std/file.mut.c
/// Targeted mutation-kill tests for File: drive inputs that make surviving mutants
/// produce observably-wrong results. Distinct from existing File tests -- no dups.
#include <Misra/Std/Allocator/Default.h>
#include <Misra/Std/Container/Str.h>
#include <Misra/Std/File.h>
#include <Misra/Std/Log.h>
#include <Misra/Std/Zstr.h>
#include <Misra/Sys/Dir.h>

#include "../Util/TestRunner.h"

// Local: write `text` into a freshly-created unique temp file so we have
// a real on-disk path to operate on. On success `out_path` holds the
// resolved name. Mirrors the helper in the main File suite.
static bool write_test_file_local(Zstr text, Str *out_path, Allocator *alloc) {
    File f = FileOpenTemp(out_path, alloc);
    if (!FileIsOpen(&f)) {
        return false;
    }
    size n       = ZstrLen(text);
    i64  written = FileWrite(&f, text, (u64)n);
    bool ok      = (written == (i64)n);
    FileClose(&f);
    if (!ok) {
        FileRemove(out_path);
        StrDeinit(out_path);
    }
    return ok;
}

// L240 `return f->fd >= 0` in FileIsOpen: ge_to_gt would treat fd == 0
// (the borrowed stdin slot) as not-open. fd 0 is a live descriptor in
// every normal process, so FileIsOpen on a File wrapping it must be
// true. The existing FileFromFd tests only check FileFd's value, never
// FileIsOpen on fd 0, so the `>= 0` vs `> 0` boundary is unpinned there.
bool test_mut_240_isopen_fd_zero(void) {
    WriteFmt("Testing FileIsOpen reports a borrowed fd 0 as open (>= 0 boundary)\n");

#if PLATFORM_WINDOWS
    // The `fd >= 0` boundary is POSIX-only: on Windows FileIsOpen tests a
    // HANDLE, and FileFromFd always yields INVALID_HANDLE_VALUE (the fd is
    // ignored), so there is no fd-0 boundary to pin here.
    return true;
#else
    // fd 0 (stdin) is a valid, open descriptor. `>= 0` must accept it;
    // a `> 0` mutant would wrongly report it not-open.
    File f  = FileFromFd(0);
    bool ok = (FileIsOpen(&f) == true);
    // Sanity: a genuinely-negative fd is not open under either form, so
    // the discriminating case really is fd == 0.
    File g = FileFromFd(-1);
    ok     = ok && (FileIsOpen(&g) == false);
    return ok;
#endif
}

// L498 `if (!path || (!buf && n > 0))` in file_write_and_close_from_bytes:
// gt_to_ge flips `n > 0` to `n >= 0`, which makes (buf == NULL, n == 0)
// satisfy the `!buf && n >= 0` clause and LOG_FATAL -- yet a NULL buffer
// with a zero byte count is a *legal* call (write nothing, truncate the
// file). Real code returns 0. The existing zero-length test passes a
// non-NULL buffer, so it never exercises the `!buf` arm at n == 0.
bool test_mut_498_null_buf_zero_n_ok(void) {
    WriteFmt("Testing FileWriteAndClose(path, NULL, 0) returns 0 (n > 0 guard)\n");

    DefaultAllocator alloc      = DefaultAllocatorInit();
    Allocator       *alloc_base = ALLOCATOR_OF(&alloc);

    // Seed a temp path that exists on disk (so the open succeeds).
    Str  path;
    bool ok = write_test_file_local("preexisting", &path, alloc_base);
    if (!ok) {
        DefaultAllocatorDeinit(&alloc);
        return false;
    }

    // NULL buffer with n == 0 is a legal no-op write: it must NOT abort
    // and must return 0 (the file is opened "wb", truncated, closed).
    i64 wrote = FileWriteAndClose(&path, (const void *)0, (u64)0);
    ok        = ok && (wrote == 0);

    // The file was truncated to empty.
    Str body = StrInit(alloc_base);
    i64 got  = FileReadAndClose(&path, &body);
    ok       = ok && (got == 0) && (StrLen(&body) == 0);

    StrDeinit(&body);
    FileRemove(&path);
    StrDeinit(&path);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

int main(void) {
    WriteFmt("[INFO] Starting File.Mut tests\n\n");

    TestFunction tests[] = {
        test_mut_240_isopen_fd_zero,
        test_mut_498_null_buf_zero_n_ok,
    };
    TestFunction deadend_tests[] = {0};
    (void)deadend_tests;
    return run_test_suite(tests, sizeof(tests) / sizeof(tests[0]), deadend_tests, 0, "File.Mut");
}
