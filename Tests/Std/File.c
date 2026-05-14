#ifndef _WIN32
#    define _POSIX_C_SOURCE 200809L
#endif

#include <Misra/Std/File.h>
#include <Misra/Std/Log.h>
#include <Misra/Std/Memory.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../Util/TestRunner.h"

// `mkstemp` is POSIX-only. On Windows MSVC the equivalent is `_mktemp_s` plus
// a normal `fopen`. Wrap both behind a tiny shim so the rest of the file can
// stay portable.
#ifdef _WIN32
#    include <io.h>
static FILE *open_unique_temp_file(char *path_template_inout) {
    if (_mktemp_s(path_template_inout, strlen(path_template_inout) + 1) != 0) {
        return NULL;
    }
    return fopen(path_template_inout, "wb");
}
#else
#    include <unistd.h>
static FILE *open_unique_temp_file(char *path_template_inout) {
    int fd = mkstemp(path_template_inout);
    if (fd < 0) {
        return NULL;
    }
    FILE *stream = fdopen(fd, "wb");
    if (!stream) {
        close(fd);
        remove(path_template_inout);
    }
    return stream;
}
#endif

static bool write_test_file(char *path, const char *text) {
    FILE *stream  = open_unique_temp_file(path);
    bool  success = false;

    if (!stream) {
        return false;
    }

    success = fwrite(text, 1, ZstrLen(text), stream) == ZstrLen(text);
    fclose(stream);

    if (!success) {
        remove(path);
    }

    return success;
}

// The POSIX form uses /tmp/... ; on Windows tests run from the build dir and
// the relative path lands in the build dir's filesystem - good enough for a
// scratch file.
#ifdef _WIN32
#    define MISRA_FILE_TEST_PATH_DEFAULT "misra-file-test-XXXXXX"
#    define MISRA_FILE_TEST_PATH_GROW    "misra-file-grow-test-XXXXXX"
#else
#    define MISRA_FILE_TEST_PATH_DEFAULT "/tmp/misra-file-test-XXXXXX"
#    define MISRA_FILE_TEST_PATH_GROW    "/tmp/misra-file-grow-test-XXXXXX"
#endif

bool test_read_complete_file_default_allocator(void) {
    char  path[]    = MISRA_FILE_TEST_PATH_DEFAULT;
    char *buffer    = NULL;
    u64   file_size = 0;
    u64   capacity  = 0;
    bool  result    = false;

    WriteFmt("Testing ReadCompleteFile with default allocator\n");

    if (!write_test_file(path, "hello from file")) {
        return false;
    }

    result = ReadCompleteFile(path, &buffer, &file_size, &capacity) && file_size == (u64)ZstrLen("hello from file") &&
             ZstrCompare(buffer, "hello from file") == 0 && capacity >= file_size + 1;

    {
        Allocator allocator = DefaultAllocator();
        AllocatorFree(&allocator, buffer, capacity);
    }
    remove(path);
    return result;
}

bool test_read_complete_file_expands_existing_buffer(void) {
    char      path[]    = MISRA_FILE_TEST_PATH_GROW;
    char     *buffer    = NULL;
    u64       file_size = 0;
    u64       capacity  = 0;
    bool      result    = false;
    Allocator allocator = DefaultAllocator();

    WriteFmt("Testing ReadCompleteFile with existing buffer allocator\n");

    if (!write_test_file(path, "this is longer than the initial buffer")) {
        return false;
    }

    buffer = (char *)AllocatorAlloc(&allocator, 4, true);
    if (!buffer) {
        remove(path);
        return false;
    }
    capacity = 4;

    result = ReadCompleteFile(path, &buffer, &file_size, &capacity, &allocator) &&
             file_size == (u64)ZstrLen("this is longer than the initial buffer") &&
             ZstrCompare(buffer, "this is longer than the initial buffer") == 0 && capacity >= file_size + 1;

    AllocatorFree(&allocator, buffer, capacity);
    remove(path);
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
