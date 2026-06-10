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
    };

    return run_test_suite(tests, sizeof(tests) / sizeof(tests[0]), NULL, 0, "File");
}
