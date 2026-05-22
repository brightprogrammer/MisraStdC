#include <Misra/Std/Allocator/Default.h>
#include <Misra/Std/Container/Str.h>
#include <Misra/Std/File.h>
#include <Misra/Std/Log.h>
#include <Misra/Std/Memory.h>
#include <Misra/Std/Zstr.h>
#include <Misra/Sys/Dir.h>

#include "../Util/TestRunner.h"

// Writes `text` into a freshly-created unique temp file (FileOpenTemp
// = atomic O_CREAT|O_EXCL + Prng64 16-hex name in CWD). `out_path`
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
    // (no public capacity accessor on Str — reading body.capacity directly)
    bool result = (got == (i64)ZstrLen(expected)) && (StrLen(&body) == (size)ZstrLen(expected)) &&
                  ZstrCompare(StrBegin(&body), expected) == 0 && body.capacity >= StrLen(&body) + 1;

    StrDeinit(&body);
    FileRemove(&path);
    StrDeinit(&path);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

int main(void) {
    WriteFmt("[INFO] Starting File tests\n\n");

    TestFunction tests[] = {
        test_file_read_into_str,
        test_file_read_grows_str,
    };

    return run_test_suite(tests, sizeof(tests) / sizeof(tests[0]), NULL, 0, "File");
}
