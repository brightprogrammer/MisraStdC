#define _POSIX_C_SOURCE 200809L

#include <Misra/Std/File.h>
#include <Misra/Std/Memory.h>
#include <Misra/Std/Log.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "../Util/TestRunner.h"

static bool write_test_file(char *path, const char *text) {
    int   fd      = mkstemp(path);
    FILE *stream  = NULL;
    bool  success = false;

    if (fd < 0) {
        return false;
    }

    stream = fdopen(fd, "wb");
    if (!stream) {
        close(fd);
        unlink(path);
        return false;
    }

    success = fwrite(text, 1, ZstrLen(text), stream) == ZstrLen(text);
    fclose(stream);

    if (!success) {
        unlink(path);
    }

    return success;
}

bool test_read_complete_file_default_allocator(void) {
    char path[]     = "/tmp/misra-file-test-XXXXXX";
    char *buffer    = NULL;
    u64   file_size = 0;
    u64   capacity  = 0;
    bool  result    = false;

    WriteFmt("Testing ReadCompleteFile with default allocator\n");

    if (!write_test_file(path, "hello from file")) {
        return false;
    }

    result = ReadCompleteFile(path, &buffer, &file_size, &capacity) &&
             file_size == (u64)ZstrLen("hello from file") &&
             ZstrCompare(buffer, "hello from file") == 0 &&
             capacity >= file_size + 1;

    {
        Allocator allocator = DefaultAllocator();
        AllocatorFree(&allocator, buffer, capacity, 1);
    }
    unlink(path);
    return result;
}

bool test_read_complete_file_expands_existing_buffer(void) {
    char path[]        = "/tmp/misra-file-grow-test-XXXXXX";
    char *buffer       = NULL;
    u64   file_size    = 0;
    u64   capacity     = 0;
    bool  result       = false;
    Allocator allocator = DefaultAllocator();

    WriteFmt("Testing ReadCompleteFile with existing buffer allocator\n");

    if (!write_test_file(path, "this is longer than the initial buffer")) {
        return false;
    }

    buffer = (char *)AllocatorAlloc(&allocator, 4, 1, true);
    if (!buffer) {
        unlink(path);
        return false;
    }
    capacity = 4;

    result = ReadCompleteFile(path, &buffer, &file_size, &capacity, &allocator) &&
             file_size == (u64)ZstrLen("this is longer than the initial buffer") &&
             ZstrCompare(buffer, "this is longer than the initial buffer") == 0 &&
             capacity >= file_size + 1;

    AllocatorFree(&allocator, buffer, capacity, 1);
    unlink(path);
    return result;
}

int main(void) {
    WriteFmt("[INFO] Starting File tests\n\n");

    TestFunction tests[] = {
        test_read_complete_file_default_allocator,
        test_read_complete_file_expands_existing_buffer,
    };

    return run_test_suite(tests, sizeof(tests) / sizeof(tests[0]), NULL, 0, "File");
}
