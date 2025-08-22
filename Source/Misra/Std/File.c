/// file      : file.c
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// File helper utilities implementation

#include <stdlib.h>

// decompiler
#include <Misra/Std/File.h>
#include <Misra/Std/Log.h>
#include <Misra/Sys.h>
#include <Misra/Types.h>

bool ReadCompleteFile(const char *filename, char **data, size *file_size, size *capacity) {
    if (!filename || !data || !file_size || !capacity) {
        LOG_ERROR("invalid arguments.");
        return false;
    }

    // get actual size of file
    i64 fsize = SysGetFileSize(filename);
    if (-1 == fsize) {
        LOG_ERROR("failed to get file size");
        return false;
    }

    // allocate memory to hold the file contents if required
    char *buffer = *data;
    if (*capacity < (u64)fsize) {
        buffer = realloc(buffer, fsize + 1);

        if (!buffer) {
            Str syserr;
            StrInitStack(syserr, SYS_ERROR_STR_MAX_LENGTH, {
                SysStrError(errno, &syserr);
                LOG_FATAL("malloc() failed : {}", syserr);
            });
        }

        *capacity = fsize + 1;
    }

    // Open the file in binary mode
    FILE *file = NULL;
    int   e    = 0;
#ifdef _WIN32
    e = fopen_s(&file, filename, "rb");
#else
    file = fopen(filename, "rb");
    if (!file) {
        e = errno;
    }
#endif
    if (e || !file) {
        free(buffer);

        Str syserr = StrInit();
        StrInitStack(syserr, SYS_ERROR_STR_MAX_LENGTH, {
            SysStrError(e, &syserr);
            LOG_ERROR("fopen() failed : {}", syserr);
        });

        return false;
    }

    // Read the entire file into the buffer
    if (fsize != (i64)fread(buffer, 1, fsize, file)) {
        fclose(file);
        Str syserr = StrInit();
        StrInitStack(syserr, SYS_ERROR_STR_MAX_LENGTH, {
            SysStrError(errno, &syserr);
            LOG_ERROR("failed to read complete file. : {}", syserr);
        });

        return false;
    }

    // Close the file and return the buffer
    fclose(file);

    ((char *)buffer)[fsize] = 0; // null-termination for just in case.
    *data                   = buffer;
    *file_size              = fsize;
    return true;
}
