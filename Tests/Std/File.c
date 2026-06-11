#include <Misra/Std/Allocator/Default.h>
#include <Misra/Std/Container/Str.h>
#include <Misra/Std/File.h>
#include <Misra/Std/Log.h>
#include <Misra/Std/Memory.h>
#include <Misra/Std/Zstr.h>
#include <Misra/Sys/Dir.h>

#include "../Util/TestRunner.h"

// Writes `text` into a freshly-created unique temp file. `out_path`
// is the caller's Str; on success it holds the resolved name so the
// caller can read it back and remove it.
static bool write_test_file(Zstr text, Str *out_path, Allocator *alloc) {
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

bool test_file_read_into_str(void) {
    WriteFmt("Testing FileRead into Str (whole-file load)\n");

    DefaultAllocator alloc      = DefaultAllocatorInit();
    Allocator       *alloc_base = ALLOCATOR_OF(&alloc);

    Str path;
    if (!write_test_file("hello from file", &path, alloc_base)) {
        DefaultAllocatorDeinit(&alloc);
        return false;
    }

    File f = FileOpen(&path, "rb");
    if (!FileIsOpen(&f)) {
        FileRemove(&path);
        StrDeinit(&path);
        DefaultAllocatorDeinit(&alloc);
        return false;
    }

    Str body = StrInit(alloc_base);
    i64 got  = FileRead(&f, &body);
    FileClose(&f);

    bool result = (got == (i64)ZstrLen("hello from file")) && (StrLen(&body) == (size)ZstrLen("hello from file")) &&
                  ZstrCompare(StrBegin(&body), "hello from file") == 0;

    StrDeinit(&body);
    FileRemove(&path);
    StrDeinit(&path);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

bool test_file_read_grows_str(void) {
    WriteFmt("Testing FileRead grows the Str backing buffer\n");

    DefaultAllocator alloc      = DefaultAllocatorInit();
    Allocator       *alloc_base = ALLOCATOR_OF(&alloc);

    Str path;
    if (!write_test_file("this is longer than the initial buffer", &path, alloc_base)) {
        DefaultAllocatorDeinit(&alloc);
        return false;
    }

    File f = FileOpen(&path, "rb");
    if (!FileIsOpen(&f)) {
        FileRemove(&path);
        StrDeinit(&path);
        DefaultAllocatorDeinit(&alloc);
        return false;
    }

    Str body = StrInit(alloc_base);
    i64 got  = FileRead(&f, &body);
    FileClose(&f);

    Zstr expected = "this is longer than the initial buffer";
    bool result   = (got == (i64)ZstrLen(expected)) && (StrLen(&body) == (size)ZstrLen(expected)) &&
                  ZstrCompare(StrBegin(&body), expected) == 0 && StrCapacity(&body) >= StrLen(&body) + 1;

    StrDeinit(&body);
    FileRemove(&path);
    StrDeinit(&path);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// A closed (or never-opened) File reports not-open, and every read /
// write / seek / tell against it returns the documented error sentinel
// rather than touching a stale fd.
bool test_closed_file_ops_fail(void) {
    WriteFmt("Testing read/write/seek/tell on a closed file return errors\n");

    DefaultAllocator alloc      = DefaultAllocatorInit();
    Allocator       *alloc_base = ALLOCATOR_OF(&alloc);

    Str  path;
    bool ok = write_test_file("payload", &path, alloc_base);
    if (!ok) {
        DefaultAllocatorDeinit(&alloc);
        return false;
    }

    File f = FileOpen(&path, "rb");
    ok     = ok && FileIsOpen(&f);
    FileClose(&f);

    // After close the handle must read as not-open.
    ok = ok && !FileIsOpen(&f);

    char scratch[8] = {0};
    ok              = ok && (FileRead(&f, scratch, sizeof(scratch)) == -1);
    ok              = ok && (FileWrite(&f, "x", 1) == -1);
    ok              = ok && (FileSeek(&f, 0, FILE_SEEK_SET) == -1);
    ok              = ok && (FileTell(&f) == -1);

    FileRemove(&path);
    StrDeinit(&path);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// An invalid mode string yields a File that reports not-open.
bool test_open_invalid_mode(void) {
    WriteFmt("Testing FileOpen with an invalid mode returns a not-open file\n");

    DefaultAllocator alloc      = DefaultAllocatorInit();
    Allocator       *alloc_base = ALLOCATOR_OF(&alloc);

    Str  path;
    bool ok = write_test_file("data", &path, alloc_base);
    if (!ok) {
        DefaultAllocatorDeinit(&alloc);
        return false;
    }

    // "q" is not a recognised mode.
    File f = FileOpen(&path, "q");
    ok     = ok && !FileIsOpen(&f);
    FileClose(&f);

    // An empty mode is also rejected.
    File g = FileOpen(&path, "");
    ok     = ok && !FileIsOpen(&g);
    FileClose(&g);

    FileRemove(&path);
    StrDeinit(&path);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// Write then seek-to-start then read back the same bytes; FileTell
// reports the byte offset after each step. Round-trip through the raw
// 3-arg read/write surface, exercising seek/tell offsets as caller-
// observable values.
bool test_write_seek_read_roundtrip(void) {
    WriteFmt("Testing FileWrite/FileSeek/FileTell/FileRead round-trip\n");

    DefaultAllocator alloc      = DefaultAllocatorInit();
    Allocator       *alloc_base = ALLOCATOR_OF(&alloc);

    Str  path;
    File f = FileOpenTemp(&path, alloc_base);
    if (!FileIsOpen(&f)) {
        DefaultAllocatorDeinit(&alloc);
        return false;
    }

    Zstr msg   = "abcdefgh"; // 8 bytes
    i64  wrote = FileWrite(&f, msg, 8);
    bool ok    = (wrote == 8);

    // After writing 8 bytes the cursor is at offset 8.
    ok = ok && (FileTell(&f) == 8);

    // Seek to absolute offset 2 and confirm Tell agrees.
    ok = ok && (FileSeek(&f, 2, FILE_SEEK_SET) == 2);
    ok = ok && (FileTell(&f) == 2);

    char got[4] = {0};
    i64  n      = FileRead(&f, got, 4);
    ok          = ok && (n == 4) && got[0] == 'c' && got[1] == 'd' && got[2] == 'e' && got[3] == 'f';

    // Cursor advanced by the 4 read bytes (2 -> 6).
    ok = ok && (FileTell(&f) == 6);

    // Seek to end yields the file length.
    ok = ok && (FileSeek(&f, 0, FILE_SEEK_END) == 8);

    FileClose(&f);
    FileRemove(&path);
    StrDeinit(&path);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// Reading at EOF returns 0 and sets the eof flag; a zero-length read
// request returns 0 without touching eof.
bool test_eof_semantics(void) {
    WriteFmt("Testing EOF detection and zero-length read\n");

    DefaultAllocator alloc      = DefaultAllocatorInit();
    Allocator       *alloc_base = ALLOCATOR_OF(&alloc);

    Str  path;
    bool ok = write_test_file("hi", &path, alloc_base);
    if (!ok) {
        DefaultAllocatorDeinit(&alloc);
        return false;
    }

    File f = FileOpen(&path, "rb");
    ok     = ok && FileIsOpen(&f);

    // A zero-byte request returns 0 and must NOT flag eof.
    char scratch[4] = {0};
    ok              = ok && (FileRead(&f, scratch, 0) == 0) && !FileIsEof(&f);

    // Drain the 2 bytes of content.
    ok = ok && (FileRead(&f, scratch, 2) == 2) && !FileIsEof(&f);

    // The next read hits EOF: 0 bytes, eof flag set.
    ok = ok && (FileRead(&f, scratch, 2) == 0) && FileIsEof(&f);

    FileClose(&f);
    FileRemove(&path);
    StrDeinit(&path);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// "w" truncates an existing file before writing; reopening for read
// must see only the new (shorter) content, never leftover tail bytes.
bool test_write_mode_truncates(void) {
    WriteFmt("Testing \"w\" mode truncates prior content\n");

    DefaultAllocator alloc      = DefaultAllocatorInit();
    Allocator       *alloc_base = ALLOCATOR_OF(&alloc);

    Str  path;
    bool ok = write_test_file("a long original payload", &path, alloc_base);
    if (!ok) {
        DefaultAllocatorDeinit(&alloc);
        return false;
    }

    File w = FileOpen(&path, "w");
    ok     = ok && FileIsOpen(&w);
    ok     = ok && (FileWrite(&w, "new", 3) == 3);
    FileClose(&w);

    File r   = FileOpen(&path, "rb");
    ok       = ok && FileIsOpen(&r);
    Str body = StrInit(alloc_base);
    i64 got  = FileRead(&r, &body);
    FileClose(&r);

    ok = ok && (got == 3) && (StrLen(&body) == 3) && ZstrCompare(StrBegin(&body), "new") == 0;

    StrDeinit(&body);
    FileRemove(&path);
    StrDeinit(&path);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// "a" mode appends: existing content is preserved and new bytes land at
// the end.
bool test_append_mode_preserves(void) {
    WriteFmt("Testing \"a\" mode appends to prior content\n");

    DefaultAllocator alloc      = DefaultAllocatorInit();
    Allocator       *alloc_base = ALLOCATOR_OF(&alloc);

    Str  path;
    bool ok = write_test_file("head", &path, alloc_base);
    if (!ok) {
        DefaultAllocatorDeinit(&alloc);
        return false;
    }

    File a = FileOpen(&path, "a");
    ok     = ok && FileIsOpen(&a);
    ok     = ok && (FileWrite(&a, "tail", 4) == 4);
    FileClose(&a);

    File r    = FileOpen(&path, "rb");
    Str  body = StrInit(alloc_base);
    i64  got  = FileRead(&r, &body);
    FileClose(&r);

    ok = ok && (got == 8) && ZstrCompare(StrBegin(&body), "headtail") == 0;

    StrDeinit(&body);
    FileRemove(&path);
    StrDeinit(&path);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// FileReadAndClose slurps a whole file in one call and returns the byte
// count; FileWriteAndClose writes a buffer and returns the count. Tests
// the open+op+close convenience round-trip.
bool test_write_and_read_and_close(void) {
    WriteFmt("Testing FileWriteAndClose + FileReadAndClose round-trip\n");

    DefaultAllocator alloc      = DefaultAllocatorInit();
    Allocator       *alloc_base = ALLOCATOR_OF(&alloc);

    // Need a path on disk; create+remove a temp to mint a unique name,
    // then write to it through the convenience API.
    Str  path;
    File seed = FileOpenTemp(&path, alloc_base);
    if (!FileIsOpen(&seed)) {
        DefaultAllocatorDeinit(&alloc);
        return false;
    }
    FileClose(&seed);

    Zstr payload = "round-trip payload";
    u64  n       = ZstrLen(payload);

    i64  wrote = FileWriteAndClose(&path, payload, n);
    bool ok    = (wrote == (i64)n);

    Str body = StrInit(alloc_base);
    i64 got  = FileReadAndClose(&path, &body);
    ok       = ok && (got == (i64)n) && (StrLen(&body) == (size)n) && ZstrCompare(StrBegin(&body), payload) == 0;

    StrDeinit(&body);
    FileRemove(&path);
    StrDeinit(&path);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// FileReadAndClose on a path that does not exist returns -1 (open
// failed) rather than a bogus byte count.
bool test_read_and_close_missing_path(void) {
    WriteFmt("Testing FileReadAndClose on a missing path returns -1\n");

    DefaultAllocator alloc      = DefaultAllocatorInit();
    Allocator       *alloc_base = ALLOCATOR_OF(&alloc);

    // Mint a unique name then remove it so the path is guaranteed absent.
    Str  path;
    File seed = FileOpenTemp(&path, alloc_base);
    if (!FileIsOpen(&seed)) {
        DefaultAllocatorDeinit(&alloc);
        return false;
    }
    FileClose(&seed);
    FileRemove(&path);

    Str  body = StrInit(alloc_base);
    i64  got  = FileReadAndClose(&path, &body);
    bool ok   = (got == -1);

    StrDeinit(&body);
    StrDeinit(&path);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// ---------------------------------------------------------------------------
// parse_open_mode
// ---------------------------------------------------------------------------

// L46 `bool plus = false`: if plus starts true, "r" opens O_RDWR and a write
// would succeed. Real code: "r" is read-only, FileWrite returns -1.
bool test_fm_46_plus_init_false(void) {
    WriteFmt("Testing parse_open_mode: \"r\" opens read-only (plus init false)\n");

    DefaultAllocator alloc      = DefaultAllocatorInit();
    Allocator       *alloc_base = ALLOCATOR_OF(&alloc);

    Str  path;
    bool ok = write_test_file("seed", &path, alloc_base);
    if (!ok) {
        DefaultAllocatorDeinit(&alloc);
        return false;
    }

    File f = FileOpen(&path, "r");
    ok     = ok && FileIsOpen(&f);
    // A read-only handle must reject writes.
    ok = ok && (FileWrite(&f, "X", 1) == -1);
    FileClose(&f);

    FileRemove(&path);
    StrDeinit(&path);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// L48 `if (*p == '+')`: swapping to `!=` makes "r+" never set plus, so the
// file opens read-only and writes fail. Real code: "r+" is read+write.
bool test_fm_48_rplus_is_writable(void) {
    WriteFmt("Testing parse_open_mode: \"r+\" is writable (== '+' detection)\n");

    DefaultAllocator alloc      = DefaultAllocatorInit();
    Allocator       *alloc_base = ALLOCATOR_OF(&alloc);

    Str  path;
    bool ok = write_test_file("0123456789", &path, alloc_base);
    if (!ok) {
        DefaultAllocatorDeinit(&alloc);
        return false;
    }

    File f = FileOpen(&path, "r+");
    ok     = ok && FileIsOpen(&f);
    // "r+" must permit writes (does not truncate). Overwrite first 3 bytes.
    ok = ok && (FileWrite(&f, "AAA", 3) == 3);
    FileClose(&f);

    // Confirm the write landed.
    Str  body = StrInit(alloc_base);
    File r    = FileOpen(&path, "rb");
    i64  got  = FileRead(&r, &body);
    FileClose(&r);
    ok = ok && (got == 10) && ZstrCompare(StrBegin(&body), "AAA3456789") == 0;

    StrDeinit(&body);
    FileRemove(&path);
    StrDeinit(&path);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// L56 `flags = plus ? O_RDWR : O_RDONLY`: forcing flags to a constant breaks
// the "r" mode read-only contract -- a "r"-opened file either fails to open
// or stops behaving as a read-only handle on the seeded content.
bool test_fm_56_r_mode_reads_content(void) {
    WriteFmt("Testing parse_open_mode: \"r\" mode reads the seeded content back\n");

    DefaultAllocator alloc      = DefaultAllocatorInit();
    Allocator       *alloc_base = ALLOCATOR_OF(&alloc);

    Str  path;
    bool ok = write_test_file("read-me", &path, alloc_base);
    if (!ok) {
        DefaultAllocatorDeinit(&alloc);
        return false;
    }

    File f      = FileOpen(&path, "r");
    ok          = ok && FileIsOpen(&f);
    char buf[8] = {0};
    i64  got    = FileRead(&f, buf, 7);
    // Real "r" (O_RDONLY) reads the 7 seeded bytes intact; and a write is
    // rejected. A constant flags value drops O_RDONLY and/or adds O_TRUNC,
    // breaking at least one of these.
    ok = ok && (got == 7) && (MemCompare(buf, "read-me", 7) == 0);
    ok = ok && (FileWrite(&f, "X", 1) == -1);
    FileClose(&f);

    FileRemove(&path);
    StrDeinit(&path);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// ---------------------------------------------------------------------------
// FileFromFd
// ---------------------------------------------------------------------------

// L172 `f.fd = fd`: the borrowed fd must be the one passed in. FileFd reads
// it back. A constant 42 would surface 42 regardless of the argument.
bool test_fm_172_fromfd_keeps_fd(void) {
    WriteFmt("Testing FileFromFd preserves the fd value (FileFd round-trip)\n");

    // fd 1 (stdout) is a stable, known descriptor.
    File f  = FileFromFd(1);
    bool ok = (FileFd(&f) == 1);
    // A different fd surfaces distinctly too.
    File g = FileFromFd(0);
    ok     = ok && (FileFd(&g) == 0);
    return ok;
}

// L173 `f.owns = false`: a borrowed File must NOT own its fd, so FileClose is
// a no-op and the underlying fd stays usable. We borrow the fd of an open
// temp file, FileClose the borrowed wrapper, then keep using the owner.
bool test_fm_173_fromfd_does_not_own(void) {
    WriteFmt("Testing FileFromFd borrows (owns=false): close is a no-op on the fd\n");

    DefaultAllocator alloc      = DefaultAllocatorInit();
    Allocator       *alloc_base = ALLOCATOR_OF(&alloc);

    Str  path;
    File owner = FileOpenTemp(&path, alloc_base);
    if (!FileIsOpen(&owner)) {
        DefaultAllocatorDeinit(&alloc);
        return false;
    }
    bool ok = (FileWrite(&owner, "hello", 5) == 5);

    // Borrow the same fd and close the borrowed wrapper. owns==false means
    // the fd is left open; if the mutant set owns=true, this close would
    // really close the fd and the subsequent owner write would fail.
    File borrowed = FileFromFd(FileFd(&owner));
    FileClose(&borrowed);

    // Owner's fd must still be live.
    ok = ok && (FileWrite(&owner, "world", 5) == 5);
    FileClose(&owner);

    Str body = StrInit(alloc_base);
    i64 got  = FileReadAndClose(&path, &body);
    ok       = ok && (got == 10) && ZstrCompare(StrBegin(&body), "helloworld") == 0;

    StrDeinit(&body);
    FileRemove(&path);
    StrDeinit(&path);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// ---------------------------------------------------------------------------
// FileClose
// ---------------------------------------------------------------------------

// L219 `if (f->owns && f->fd >= 0)` and L222 `ok = r == 0`: a successful close
// returns true. ge_to_gt would skip the real close (fd>0 still true though),
// eq_to_ne on `r == 0` would report failure on a clean close.
bool test_fm_222_close_returns_true(void) {
    WriteFmt("Testing FileClose returns true on a clean close (r == 0)\n");

    DefaultAllocator alloc      = DefaultAllocatorInit();
    Allocator       *alloc_base = ALLOCATOR_OF(&alloc);

    Str  path;
    File f = FileOpenTemp(&path, alloc_base);
    if (!FileIsOpen(&f)) {
        DefaultAllocatorDeinit(&alloc);
        return false;
    }
    // A clean close of an owned fd returns true.
    bool ok = (FileClose(&f) == true);
    // And the handle is now not-open.
    ok = ok && !FileIsOpen(&f);

    FileRemove(&path);
    StrDeinit(&path);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// L228 `f->owns = false`: after close, a re-close must be a clean no-op
// returning true (owns cleared). If owns stayed truthy, the second close
// would attempt to close fd -1 and could report failure.
bool test_fm_228_double_close_clears_owns(void) {
    WriteFmt("Testing FileClose clears owns: second close is a no-op true\n");

    DefaultAllocator alloc      = DefaultAllocatorInit();
    Allocator       *alloc_base = ALLOCATOR_OF(&alloc);

    Str  path;
    File f = FileOpenTemp(&path, alloc_base);
    if (!FileIsOpen(&f)) {
        DefaultAllocatorDeinit(&alloc);
        return false;
    }
    bool ok = FileClose(&f);
    // Second close: fd was reset to -1 and owns to false, so it short-circuits
    // and returns true without touching any fd.
    ok = ok && (FileClose(&f) == true);
    ok = ok && !FileIsOpen(&f);

    FileRemove(&path);
    StrDeinit(&path);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// ---------------------------------------------------------------------------
// FileRead / FileWrite exact byte counts
// ---------------------------------------------------------------------------

// L249/L270 (file_read), L286/L300 (FileWrite): a round-trip pins the exact
// byte counts the read/write syscalls report.
bool test_fm_249_read_exact_count(void) {
    WriteFmt("Testing FileRead returns the exact byte count for a partial read\n");

    DefaultAllocator alloc      = DefaultAllocatorInit();
    Allocator       *alloc_base = ALLOCATOR_OF(&alloc);

    Str  path;
    File f = FileOpenTemp(&path, alloc_base);
    if (!FileIsOpen(&f)) {
        DefaultAllocatorDeinit(&alloc);
        return false;
    }
    // FileWrite must report exactly 12 bytes written.
    bool ok = (FileWrite(&f, "ABCDEFGHIJKL", 12) == 12);
    ok      = ok && (FileSeek(&f, 0, FILE_SEEK_SET) == 0);

    char buf[5] = {0};
    // Exact 5-byte read of the leading content.
    i64 got = FileRead(&f, buf, 5);
    ok      = ok && (got == 5) && (MemCompare(buf, "ABCDE", 5) == 0);
    FileClose(&f);

    FileRemove(&path);
    StrDeinit(&path);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// L270 `f->at_eof = true`: a zero-byte read at EOF sets the eof flag.
bool test_fm_270_read_sets_eof(void) {
    WriteFmt("Testing FileRead at EOF sets the eof flag\n");

    DefaultAllocator alloc      = DefaultAllocatorInit();
    Allocator       *alloc_base = ALLOCATOR_OF(&alloc);

    Str  path;
    bool ok = write_test_file("ab", &path, alloc_base);
    if (!ok) {
        DefaultAllocatorDeinit(&alloc);
        return false;
    }

    File f      = FileOpen(&path, "rb");
    ok          = ok && FileIsOpen(&f);
    char buf[4] = {0};
    ok          = ok && (FileRead(&f, buf, 2) == 2) && !FileIsEof(&f);
    // The next read hits EOF: 0 bytes and at_eof becomes true.
    ok = ok && (FileRead(&f, buf, 2) == 0) && FileIsEof(&f);
    FileClose(&f);

    FileRemove(&path);
    StrDeinit(&path);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// ---------------------------------------------------------------------------
// FileSeek
// ---------------------------------------------------------------------------

// L314/L317/L329 (FileSeek): seek returns the new absolute offset, and clears
// the eof flag so a subsequent read can succeed again.
bool test_fm_317_seek_clears_eof(void) {
    WriteFmt("Testing FileSeek clears eof and returns the new offset\n");

    DefaultAllocator alloc      = DefaultAllocatorInit();
    Allocator       *alloc_base = ALLOCATOR_OF(&alloc);

    Str  path;
    bool ok = write_test_file("abcdef", &path, alloc_base);
    if (!ok) {
        DefaultAllocatorDeinit(&alloc);
        return false;
    }

    File f      = FileOpen(&path, "rb");
    ok          = ok && FileIsOpen(&f);
    char buf[8] = {0};
    // Drain to EOF to set at_eof.
    ok = ok && (FileRead(&f, buf, 6) == 6);
    ok = ok && (FileRead(&f, buf, 2) == 0) && FileIsEof(&f);

    // Seek back to start: returns offset 0 and clears the eof flag.
    ok = ok && (FileSeek(&f, 0, FILE_SEEK_SET) == 0);
    ok = ok && !FileIsEof(&f);
    // And reading now succeeds again from the top.
    char buf2[3] = {0};
    ok           = ok && (FileRead(&f, buf2, 3) == 3) && (MemCompare(buf2, "abc", 3) == 0);
    FileClose(&f);

    FileRemove(&path);
    StrDeinit(&path);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// FileSeek with SEEK_CUR returns a distinct absolute offset (pins the lseek
// result return, line 328/332).
bool test_fm_328_seek_cur_offset(void) {
    WriteFmt("Testing FileSeek SEEK_CUR returns the running absolute offset\n");

    DefaultAllocator alloc      = DefaultAllocatorInit();
    Allocator       *alloc_base = ALLOCATOR_OF(&alloc);

    Str  path;
    File f = FileOpenTemp(&path, alloc_base);
    if (!FileIsOpen(&f)) {
        DefaultAllocatorDeinit(&alloc);
        return false;
    }
    bool ok = (FileWrite(&f, "0123456789", 10) == 10);
    ok      = ok && (FileSeek(&f, 0, FILE_SEEK_SET) == 0);
    // Advance 3 from current (0 -> 3).
    ok = ok && (FileSeek(&f, 3, FILE_SEEK_CUR) == 3);
    // Advance 4 more (3 -> 7).
    ok          = ok && (FileSeek(&f, 4, FILE_SEEK_CUR) == 7);
    char buf[2] = {0};
    ok          = ok && (FileRead(&f, buf, 1) == 1) && (buf[0] == '7');
    FileClose(&f);

    FileRemove(&path);
    StrDeinit(&path);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// ---------------------------------------------------------------------------
// file_remaining_size (via FileRead-to-container fast path)
// ---------------------------------------------------------------------------

// L394/L395/L400/L403 (file_remaining_size): the whole-file read reserves the
// exact remaining size and reads back the exact content even when the cursor
// starts mid-file. The `end - here` arithmetic and the restore-seek must be
// right or the slurp drops/duplicates bytes.
bool test_fm_403_remaining_size_from_midfile(void) {
    WriteFmt("Testing FileRead-to-EOF from a mid-file cursor reads only the tail\n");

    DefaultAllocator alloc      = DefaultAllocatorInit();
    Allocator       *alloc_base = ALLOCATOR_OF(&alloc);

    Str  path;
    bool ok = write_test_file("0123456789ABCDEF", &path, alloc_base); // 16 bytes
    if (!ok) {
        DefaultAllocatorDeinit(&alloc);
        return false;
    }

    File f = FileOpen(&path, "rb");
    ok     = ok && FileIsOpen(&f);
    // Move the cursor to offset 10; remaining size must be 16 - 10 = 6.
    ok = ok && (FileSeek(&f, 10, FILE_SEEK_SET) == 10);

    Str body = StrInit(alloc_base);
    i64 got  = FileRead(&f, &body);
    FileClose(&f);

    ok = ok && (got == 6) && (StrLen(&body) == 6) && ZstrCompare(StrBegin(&body), "ABCDEF") == 0;

    StrDeinit(&body);
    FileRemove(&path);
    StrDeinit(&path);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// Whole-file slurp from offset 0 reads the entire content (pins remaining ==
// total when here == 0; guards the `end < here` and reserve logic).
bool test_fm_394_remaining_size_full(void) {
    WriteFmt("Testing FileRead-to-EOF from start reads the whole file exactly\n");

    DefaultAllocator alloc      = DefaultAllocatorInit();
    Allocator       *alloc_base = ALLOCATOR_OF(&alloc);

    Str  path;
    Zstr payload = "the quick brown fox jumps over the lazy dog"; // 43 bytes
    bool ok      = write_test_file(payload, &path, alloc_base);
    if (!ok) {
        DefaultAllocatorDeinit(&alloc);
        return false;
    }

    File f   = FileOpen(&path, "rb");
    ok       = ok && FileIsOpen(&f);
    Str body = StrInit(alloc_base);
    i64 got  = FileRead(&f, &body);
    FileClose(&f);

    ok = ok && (got == (i64)ZstrLen(payload)) && (StrLen(&body) == (size)ZstrLen(payload)) &&
         ZstrCompare(StrBegin(&body), payload) == 0;

    StrDeinit(&body);
    FileRemove(&path);
    StrDeinit(&path);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// ---------------------------------------------------------------------------
// file_read_to_buf
// ---------------------------------------------------------------------------

// L426/L427 (file_read_to_buf): `if (remaining > 0)` reserve. A round-trip of
// a payload larger than one read chunk pins that the slurp returns the exact
// total and full content (guards the reserve-size and grow-loop).
bool test_fm_427_read_to_buf_large(void) {
    WriteFmt("Testing FileRead-to-Buf slurps a multi-chunk payload exactly\n");

    DefaultAllocator alloc      = DefaultAllocatorInit();
    Allocator       *alloc_base = ALLOCATOR_OF(&alloc);

    // Build a 10000-byte payload (> FILE_READ_CHUNK=4096) on disk.
    Str  path;
    File f = FileOpenTemp(&path, alloc_base);
    if (!FileIsOpen(&f)) {
        DefaultAllocatorDeinit(&alloc);
        return false;
    }
    enum {
        N = 10000
    };
    Buf  src = BufInit(alloc_base);
    bool ok  = BufResize(&src, N);
    for (i32 i = 0; i < N; ++i) {
        BufData(&src)[i] = (u8)('A' + (i % 26));
    }
    ok = ok && (FileWrite(&f, BufData(&src), (u64)N) == (i64)N);
    FileClose(&f);

    File r  = FileOpen(&path, "rb");
    ok      = ok && FileIsOpen(&r);
    Buf dst = BufInit(alloc_base);
    i64 got = FileRead(&r, &dst);
    FileClose(&r);

    ok = ok && (got == (i64)N) && (BufLength(&dst) == (size)N) && (MemCompare(BufData(&dst), BufData(&src), N) == 0);

    BufDeinit(&src);
    BufDeinit(&dst);
    FileRemove(&path);
    StrDeinit(&path);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// ---------------------------------------------------------------------------
// Whole-file convenience helpers
// ---------------------------------------------------------------------------

// L485/L489 (file_read_and_close_to_buf), L498/L502/L506 (write_and_close):
// round-trip a known payload through the whole-file helpers; assert exact
// content and length.
bool test_fm_498_write_close_read_close_roundtrip(void) {
    WriteFmt("Testing FileWriteAndClose + FileReadAndClose exact round-trip\n");

    DefaultAllocator alloc      = DefaultAllocatorInit();
    Allocator       *alloc_base = ALLOCATOR_OF(&alloc);

    Str  path;
    File seed = FileOpenTemp(&path, alloc_base);
    if (!FileIsOpen(&seed)) {
        DefaultAllocatorDeinit(&alloc);
        return false;
    }
    FileClose(&seed);

    Zstr payload = "payload-through-the-convenience-API";
    u64  n       = ZstrLen(payload);

    i64  wrote = FileWriteAndClose(&path, payload, n);
    bool ok    = (wrote == (i64)n);

    Str body = StrInit(alloc_base);
    i64 got  = FileReadAndClose(&path, &body);
    ok       = ok && (got == (i64)n) && (StrLen(&body) == (size)n) && ZstrCompare(StrBegin(&body), payload) == 0;

    StrDeinit(&body);
    FileRemove(&path);
    StrDeinit(&path);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// L498 `n > 0` guard: a zero-length write-and-close returns 0 and truncates
// the file to empty (so a subsequent read-and-close returns 0).
bool test_fm_498_zero_length_write(void) {
    WriteFmt("Testing FileWriteAndClose with n==0 writes nothing, truncates file\n");

    DefaultAllocator alloc      = DefaultAllocatorInit();
    Allocator       *alloc_base = ALLOCATOR_OF(&alloc);

    Str  path;
    bool ok = write_test_file("preexisting content", &path, alloc_base);
    if (!ok) {
        DefaultAllocatorDeinit(&alloc);
        return false;
    }

    // n == 0 path: returns 0, file is opened "wb" (truncated) and closed.
    i64 wrote = FileWriteAndClose(&path, "ignored", (u64)0);
    ok        = ok && (wrote == 0);

    Str body = StrInit(alloc_base);
    i64 got  = FileReadAndClose(&path, &body);
    ok       = ok && (got == 0) && (StrLen(&body) == 0);

    StrDeinit(&body);
    FileRemove(&path);
    StrDeinit(&path);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// L514 (file_write_and_close_from_buf): the Buf form writes the Buf's exact
// length and content.
bool test_fm_514_write_close_from_buf(void) {
    WriteFmt("Testing FileWriteAndClose(Buf) writes the buffer length exactly\n");

    DefaultAllocator alloc      = DefaultAllocatorInit();
    Allocator       *alloc_base = ALLOCATOR_OF(&alloc);

    Str  path;
    File seed = FileOpenTemp(&path, alloc_base);
    if (!FileIsOpen(&seed)) {
        DefaultAllocatorDeinit(&alloc);
        return false;
    }
    FileClose(&seed);

    Buf  in = BufInit(alloc_base);
    bool ok = BufResize(&in, 5);
    MemCopy(BufData(&in), "12345", 5);

    i64 wrote = FileWriteAndClose(&path, &in);
    ok        = ok && (wrote == 5);

    Str body = StrInit(alloc_base);
    i64 got  = FileReadAndClose(&path, &body);
    ok       = ok && (got == 5) && ZstrCompare(StrBegin(&body), "12345") == 0;

    StrDeinit(&body);
    BufDeinit(&in);
    FileRemove(&path);
    StrDeinit(&path);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// L521 (file_write_and_close_from_str): the Str form writes the Str's exact
// length and content.
bool test_fm_521_write_close_from_str(void) {
    WriteFmt("Testing FileWriteAndClose(Str) writes the string length exactly\n");

    DefaultAllocator alloc      = DefaultAllocatorInit();
    Allocator       *alloc_base = ALLOCATOR_OF(&alloc);

    Str  path;
    File seed = FileOpenTemp(&path, alloc_base);
    if (!FileIsOpen(&seed)) {
        DefaultAllocatorDeinit(&alloc);
        return false;
    }
    FileClose(&seed);

    Str in = StrInit(alloc_base);
    StrAppendFmt(&in, "string-content");

    i64  wrote = FileWriteAndClose(&path, &in);
    bool ok    = (wrote == (i64)StrLen(&in));

    Str body = StrInit(alloc_base);
    i64 got  = FileReadAndClose(&path, &body);
    ok       = ok && (got == (i64)StrLen(&in)) && ZstrCompare(StrBegin(&body), "string-content") == 0;

    StrDeinit(&body);
    StrDeinit(&in);
    FileRemove(&path);
    StrDeinit(&path);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// ---------------------------------------------------------------------------
// file_open_temp
// ---------------------------------------------------------------------------

// L538/L602/L604/L607 (file_open_temp): a temp open yields an open, writable
// file with a non-empty path that round-trips a payload.
bool test_fm_602_temp_open_roundtrips(void) {
    WriteFmt("Testing FileOpenTemp creates an open, writable, named file\n");

    DefaultAllocator alloc      = DefaultAllocatorInit();
    Allocator       *alloc_base = ALLOCATOR_OF(&alloc);

    Str  path;
    File f  = FileOpenTemp(&path, alloc_base);
    bool ok = FileIsOpen(&f);
    // The resolved path must be non-empty (16 hex chars).
    ok = ok && (StrLen(&path) == 16);

    // Round-trip a payload through the freshly created temp file.
    ok           = ok && (FileWrite(&f, "temp-data", 9) == 9);
    ok           = ok && (FileSeek(&f, 0, FILE_SEEK_SET) == 0);
    char buf[10] = {0};
    ok           = ok && (FileRead(&f, buf, 9) == 9) && (MemCompare(buf, "temp-data", 9) == 0);
    FileClose(&f);

    FileRemove(&path);
    StrDeinit(&path);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// L547 `attempt < 8` / `++attempt`: two successive FileOpenTemp calls produce
// distinct files (the loop body runs and a fresh draw is taken per attempt).
// This also pins that a temp open succeeds on the first attempt (path is the
// hex name, owns the new fd).
bool test_fm_547_temp_names_distinct(void) {
    WriteFmt("Testing two FileOpenTemp calls yield distinct, independent files\n");

    DefaultAllocator alloc      = DefaultAllocatorInit();
    Allocator       *alloc_base = ALLOCATOR_OF(&alloc);

    Str  p1;
    File f1 = FileOpenTemp(&p1, alloc_base);
    Str  p2;
    File f2 = FileOpenTemp(&p2, alloc_base);

    bool ok = FileIsOpen(&f1) && FileIsOpen(&f2);
    ok      = ok && (StrLen(&p1) == 16) && (StrLen(&p2) == 16);
    // Distinct names (overwhelmingly likely; the loop draws fresh entropy).
    ok = ok && (ZstrCompare(StrBegin(&p1), StrBegin(&p2)) != 0);

    // They are independent files: writing to one does not affect the other.
    ok = ok && (FileWrite(&f1, "one", 3) == 3);
    ok = ok && (FileWrite(&f2, "twotwo", 6) == 6);
    FileClose(&f1);
    FileClose(&f2);

    Str b1 = StrInit(alloc_base);
    Str b2 = StrInit(alloc_base);
    ok     = ok && (FileReadAndClose(&p1, &b1) == 3) && ZstrCompare(StrBegin(&b1), "one") == 0;
    ok     = ok && (FileReadAndClose(&p2, &b2) == 6) && ZstrCompare(StrBegin(&b2), "twotwo") == 0;

    StrDeinit(&b1);
    StrDeinit(&b2);
    FileRemove(&p1);
    FileRemove(&p2);
    StrDeinit(&p1);
    StrDeinit(&p2);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// ---------------------------------------------------------------------------
// FileFlush
// ---------------------------------------------------------------------------

// L347 `if (!FileIsOpen(f)) return false`: FileFlush succeeds (true) on an
// open handle but must report false on a closed one. Forcing the open gate
// truthy would make the closed case wrongly report success.
bool test_fm_347_flush_open_vs_closed(void) {
    WriteFmt("Testing FileFlush returns true open / false closed (open gate)\n");

    DefaultAllocator alloc      = DefaultAllocatorInit();
    Allocator       *alloc_base = ALLOCATOR_OF(&alloc);

    Str  path;
    File f = FileOpenTemp(&path, alloc_base);
    if (!FileIsOpen(&f)) {
        DefaultAllocatorDeinit(&alloc);
        return false;
    }
    // Open handle flushes true.
    bool ok = (FileFlush(&f) == true);
    FileClose(&f);
    // Closed handle must flush false (the open gate fires).
    ok = ok && (FileFlush(&f) == false);

    FileRemove(&path);
    StrDeinit(&path);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// ---------------------------------------------------------------------------
// Deadend: NULL-buffer write contract
// ---------------------------------------------------------------------------

// L498 `if (!path || (!buf && n > 0))`: FileWriteAndClose with a NULL buffer
// and a positive byte count is a contract violation and must LOG_FATAL.
// Swapping `n > 0` to `n <= 0` would let a NULL/5 call slip through; this
// deadend pins that the abort fires.
bool test_fm_498_null_buf_positive_n_aborts(void) {
    WriteFmt("Testing FileWriteAndClose(NULL, 5) aborts (NULL-buf contract)\n");

    DefaultAllocator alloc      = DefaultAllocatorInit();
    Allocator       *alloc_base = ALLOCATOR_OF(&alloc);

    Str  path;
    File seed = FileOpenTemp(&path, alloc_base);
    if (FileIsOpen(&seed)) {
        FileClose(&seed);
    }

    // NULL buffer with n > 0 -> contract violation -> LOG_FATAL.
    FileWriteAndClose(&path, (const void *)0, (u64)5);

    // Unreachable on real code.
    FileRemove(&path);
    StrDeinit(&path);
    DefaultAllocatorDeinit(&alloc);
    return true;
}

int main(void) {
    WriteFmt("[INFO] Starting File tests\n\n");

    TestFunction tests[] = {
        test_file_read_into_str,
        test_file_read_grows_str,
        test_closed_file_ops_fail,
        test_open_invalid_mode,
        test_write_seek_read_roundtrip,
        test_eof_semantics,
        test_write_mode_truncates,
        test_append_mode_preserves,
        test_write_and_read_and_close,
        test_read_and_close_missing_path,
        test_fm_46_plus_init_false,
        test_fm_48_rplus_is_writable,
        test_fm_56_r_mode_reads_content,
        test_fm_172_fromfd_keeps_fd,
        test_fm_173_fromfd_does_not_own,
        test_fm_222_close_returns_true,
        test_fm_228_double_close_clears_owns,
        test_fm_249_read_exact_count,
        test_fm_270_read_sets_eof,
        test_fm_317_seek_clears_eof,
        test_fm_328_seek_cur_offset,
        test_fm_403_remaining_size_from_midfile,
        test_fm_394_remaining_size_full,
        test_fm_427_read_to_buf_large,
        test_fm_498_write_close_read_close_roundtrip,
        test_fm_498_zero_length_write,
        test_fm_514_write_close_from_buf,
        test_fm_521_write_close_from_str,
        test_fm_602_temp_open_roundtrips,
        test_fm_547_temp_names_distinct,
        test_fm_347_flush_open_vs_closed,
    };

    TestFunction deadend_tests[] = {
        test_fm_498_null_buf_positive_n_aborts,
    };

    return run_test_suite(
        tests,
        sizeof(tests) / sizeof(tests[0]),
        deadend_tests,
        sizeof(deadend_tests) / sizeof(deadend_tests[0]),
        "File"
    );
}
