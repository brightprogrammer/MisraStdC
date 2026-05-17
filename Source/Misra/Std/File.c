/// file      : file.c
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Cross-platform File implementation plus the existing
/// `read_complete_file` helper. Linux uses direct syscalls; macOS
/// uses libSystem's open/read/write/close/lseek; Windows uses
/// CreateFile / ReadFile / WriteFile / SetFilePointer / CloseHandle.

#include <Misra/Std/File.h>
#include <Misra/Std/Log.h>
#include <Misra/Sys.h>
#include <Misra/Types.h>

#ifdef _WIN32
#    include <windows.h>
#else
#    include <fcntl.h>
#    include <unistd.h>
#    include <sys/types.h>
#endif

#include "../_Syscall.h"

#include <stdint.h>

// ---------------------------------------------------------------------------
// Mode parsing
// ---------------------------------------------------------------------------

// Parse a libc-style mode string ("r", "w", "a", with optional '+' or 'b').
// Returns POSIX open() flags. `binary` is set true regardless -- we treat
// every mode as binary. Returns false if the mode is invalid.
static bool parse_open_mode(const char *mode, int *out_flags) {
#ifdef _WIN32
    (void)mode;
    (void)out_flags;
    return false; // Windows uses a different mode encoding (see FileOpen).
#else
    if (!mode || !*mode) {
        return false;
    }
    char primary = mode[0];
    bool plus    = false;
    for (const char *p = mode + 1; *p; ++p) {
        if (*p == '+') {
            plus = true;
        }
        // 'b' is accepted but ignored.
    }
    int flags;
    switch (primary) {
        case 'r' :
            flags = plus ? O_RDWR : O_RDONLY;
            break;
        case 'w' :
            flags = (plus ? O_RDWR : O_WRONLY) | O_CREAT | O_TRUNC;
            break;
        case 'a' :
            flags = (plus ? O_RDWR : O_WRONLY) | O_CREAT | O_APPEND;
            break;
        default :
            return false;
    }
    *out_flags = flags;
    return true;
#endif
}

// ---------------------------------------------------------------------------
// Open / close
// ---------------------------------------------------------------------------

File FileOpen(const char *path, const char *mode) {
    File f = {0};
#ifdef _WIN32
    f.handle = INVALID_HANDLE_VALUE;
    if (!path || !mode || !*mode) {
        return f;
    }
    DWORD access      = 0;
    DWORD disposition = OPEN_EXISTING;
    bool  plus        = false;
    for (const char *p = mode + 1; *p; ++p) {
        if (*p == '+')
            plus = true;
    }
    switch (mode[0]) {
        case 'r' :
            access      = plus ? (GENERIC_READ | GENERIC_WRITE) : GENERIC_READ;
            disposition = OPEN_EXISTING;
            break;
        case 'w' :
            access      = plus ? (GENERIC_READ | GENERIC_WRITE) : GENERIC_WRITE;
            disposition = CREATE_ALWAYS;
            break;
        case 'a' :
            access      = plus ? (GENERIC_READ | GENERIC_WRITE) : GENERIC_WRITE;
            disposition = OPEN_ALWAYS;
            break;
        default :
            return f;
    }
    HANDLE h = CreateFileA(path, access, FILE_SHARE_READ, NULL, disposition, FILE_ATTRIBUTE_NORMAL, NULL);
    if (h == INVALID_HANDLE_VALUE) {
        LOG_ERROR("FileOpen: CreateFileA(\"{}\") failed (err {})", path, (u32)GetLastError());
        return f;
    }
    if (mode[0] == 'a') {
        SetFilePointer(h, 0, NULL, FILE_END);
    }
    f.handle = h;
    f.owns   = true;
    return f;
#else
    f.fd = -1;
    int flags;
    if (!parse_open_mode(mode, &flags)) {
        LOG_ERROR("FileOpen: invalid mode \"{}\"", mode);
        return f;
    }
    // Kernel-ABI O_CLOEXEC value per OS. <fcntl.h>'s O_CLOEXEC needs
    // _GNU_SOURCE on Linux and we'd rather not toggle feature-test
    // macros project-wide for one bit. The remaining flags
    // (O_RDONLY/WRONLY/RDWR/CREAT/TRUNC/APPEND) come from <fcntl.h>
    // via parse_open_mode and resolve correctly without a feature
    // macro.
#    if defined(__APPLE__)
    flags |= 0x1000000; // Darwin O_CLOEXEC
#    else
    flags |= 0x80000;   // Linux O_CLOEXEC
#    endif
    long fd;
#    if FEATURE_DIRECT_SYSCALL
#        if defined(__APPLE__) || defined(__x86_64__)
    // Darwin has SYS_open on both x86_64 and aarch64. Linux x86_64
    // does too; only Linux aarch64 went openat-only.
    fd = misra_sys3(MISRA_SYS_open, (long)(uintptr_t)path, (long)flags, 0644L);
#        else
    // Linux aarch64: openat(AT_FDCWD=-100, path, flags, mode).
    fd = misra_sys4(MISRA_SYS_openat, -100L, (long)(uintptr_t)path, (long)flags, 0644L);
#        endif
#    else
    fd = open(path, flags, 0644);
#    endif
    if (fd < 0) {
        LOG_ERROR("FileOpen: open(\"{}\") failed (errno {})", path, (i32)-fd);
        return f;
    }
    f.fd   = (i32)fd;
    f.owns = true;
    return f;
#endif
}

File FileFromFd(i32 fd) {
    File f = {0};
#ifdef _WIN32
    (void)fd;
    f.handle = INVALID_HANDLE_VALUE;
#else
    f.fd   = fd;
    f.owns = false;
#endif
    return f;
}

File FileStdin(void) {
#ifdef _WIN32
    File f = {.handle = GetStdHandle(STD_INPUT_HANDLE), .owns = false};
    return f;
#else
    return FileFromFd(0);
#endif
}

File FileStdout(void) {
#ifdef _WIN32
    File f = {.handle = GetStdHandle(STD_OUTPUT_HANDLE), .owns = false};
    return f;
#else
    return FileFromFd(1);
#endif
}

File FileStderr(void) {
#ifdef _WIN32
    File f = {.handle = GetStdHandle(STD_ERROR_HANDLE), .owns = false};
    return f;
#else
    return FileFromFd(2);
#endif
}

bool FileClose(File *f) {
    if (!f) {
        return false;
    }
#ifdef _WIN32
    bool ok = true;
    if (f->owns && f->handle && f->handle != INVALID_HANDLE_VALUE) {
        ok = CloseHandle((HANDLE)f->handle) != 0;
    }
    f->handle = INVALID_HANDLE_VALUE;
    f->owns   = false;
    return ok;
#else
    bool ok = true;
    if (f->owns && f->fd >= 0) {
#    if FEATURE_DIRECT_SYSCALL
        long r = misra_sys1(MISRA_SYS_close, (long)f->fd);
        ok     = r == 0;
#    else
        ok = close(f->fd) == 0;
#    endif
    }
    f->fd   = -1;
    f->owns = false;
    return ok;
#endif
}

bool FileIsValid(const File *f) {
    if (!f) {
        return false;
    }
#ifdef _WIN32
    return f->handle && f->handle != INVALID_HANDLE_VALUE;
#else
    return f->fd >= 0;
#endif
}

// ---------------------------------------------------------------------------
// Read / write / seek / tell
// ---------------------------------------------------------------------------

i64 FileRead(File *f, void *buf, u64 n) {
    if (!FileIsValid(f) || !buf) {
        return -1;
    }
    if (n == 0) {
        return 0;
    }
#ifdef _WIN32
    DWORD got = 0;
    if (!ReadFile((HANDLE)f->handle, buf, (DWORD)n, &got, NULL)) {
        return -1;
    }
    if (got == 0) {
        f->at_eof = true;
    }
    return (i64)got;
#elif FEATURE_DIRECT_SYSCALL
    long r = misra_sys3(MISRA_SYS_read, (long)f->fd, (long)(uintptr_t)buf, (long)n);
    if (r < 0) {
        return -1;
    }
    if (r == 0) {
        f->at_eof = true;
    }
    return (i64)r;
#else
    ssize_t r = read(f->fd, buf, (size_t)n);
    if (r < 0) {
        return -1;
    }
    if (r == 0) {
        f->at_eof = true;
    }
    return (i64)r;
#endif
}

i64 FileWrite(File *f, const void *buf, u64 n) {
    if (!FileIsValid(f) || !buf) {
        return -1;
    }
    if (n == 0) {
        return 0;
    }
#ifdef _WIN32
    DWORD put = 0;
    if (!WriteFile((HANDLE)f->handle, buf, (DWORD)n, &put, NULL)) {
        return -1;
    }
    return (i64)put;
#elif FEATURE_DIRECT_SYSCALL
    long r = misra_sys3(MISRA_SYS_write, (long)f->fd, (long)(uintptr_t)buf, (long)n);
    if (r < 0) {
        return -1;
    }
    return (i64)r;
#else
    ssize_t r = write(f->fd, buf, (size_t)n);
    if (r < 0) {
        return -1;
    }
    return (i64)r;
#endif
}

i64 FileSeek(File *f, i64 offset, FileWhence whence) {
    if (!FileIsValid(f)) {
        return -1;
    }
    f->at_eof = false;
#ifdef _WIN32
    LARGE_INTEGER off;
    off.QuadPart = offset;
    LARGE_INTEGER newpos;
    DWORD         method = whence == FILE_SEEK_SET ? FILE_BEGIN : whence == FILE_SEEK_CUR ? FILE_CURRENT : FILE_END;
    if (!SetFilePointerEx((HANDLE)f->handle, off, &newpos, method)) {
        return -1;
    }
    return (i64)newpos.QuadPart;
#elif FEATURE_DIRECT_SYSCALL
    long r = misra_sys3(MISRA_SYS_lseek, (long)f->fd, (long)offset, (long)whence);
    if (r < 0) {
        return -1;
    }
    return (i64)r;
#else
    off_t r = lseek(f->fd, (off_t)offset, (int)whence);
    if (r < 0) {
        return -1;
    }
    return (i64)r;
#endif
}

i64 FileTell(File *f) {
    return FileSeek(f, 0, FILE_SEEK_CUR);
}

bool FileFlush(File *f) {
    if (!FileIsValid(f)) {
        return false;
    }
#ifdef _WIN32
    return FlushFileBuffers((HANDLE)f->handle) != 0;
#else
    // POSIX: with no user-side buffering the kernel already sees the
    // writes. Callers that need durability should fsync the fd
    // themselves via the platform's fsync syscall.
    return true;
#endif
}

bool FileIsEof(const File *f) {
    return f && f->at_eof;
}

i32 FileFd(const File *f) {
#ifdef _WIN32
    (void)f;
    return -1;
#else
    return f ? f->fd : -1;
#endif
}

// ---------------------------------------------------------------------------
// Legacy convenience: read_complete_file
// ---------------------------------------------------------------------------

bool read_complete_file(const char *filename, char **data, u64 *file_size, u64 *capacity, Allocator *allocator) {
    if (!filename || !data || !file_size || !capacity) {
        LOG_FATAL("invalid arguments.");
    }
    if (!allocator) {
        LOG_FATAL("read_complete_file requires an allocator");
    }

    i64 fsize = FileGetSize(filename);
    if (-1 == fsize) {
        LOG_ERROR("failed to get file size");
        return false;
    }

    char *buffer            = *data;
    u64   required_capacity = (u64)fsize + 1;
    if (*capacity < required_capacity) {
        buffer = AllocatorRealloc(allocator, buffer, *capacity, required_capacity);

        if (!buffer) {
            LOG_ERROR("allocator reallocation failed");
            return false;
        }

        *data     = buffer;
        *capacity = required_capacity;
    }

    File f = FileOpen(filename, "rb");
    if (!FileIsValid(&f)) {
        return false;
    }

    i64 got = FileRead(&f, buffer, (u64)fsize);
    FileClose(&f);
    if (got != fsize) {
        LOG_ERROR("read_complete_file: short read on \"{}\" (got {} of {})", filename, got, fsize);
        return false;
    }

    ((char *)buffer)[fsize] = 0;
    *data                   = buffer;
    *file_size              = (u64)fsize;
    return true;
}
