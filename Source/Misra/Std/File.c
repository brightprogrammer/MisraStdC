/// file      : file.c
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Cross-platform File implementation. Linux uses direct syscalls;
/// macOS uses libSystem's open/read/write/close/lseek; Windows uses
/// CreateFile / ReadFile / WriteFile / SetFilePointer / CloseHandle.

#include <Misra/Std/Container/Buf.h>
#include <Misra/Std/Container/Str.h>
#include <Misra/Std/File.h>
#include <Misra/Std/Log.h>
#include <Misra/Std/Prng.h>
#include <Misra/Sys.h>
#include <Misra/Types.h>

#if PLATFORM_WINDOWS
#    include <windows.h>
#else
#    include <fcntl.h>
#    include <unistd.h>
#    include <sys/types.h>
#endif

#include "../_Syscall.h"

// ---------------------------------------------------------------------------
// Mode parsing
// ---------------------------------------------------------------------------

// Parse a libc-style mode string ("r", "w", "a", with optional '+' or 'b').
// Returns POSIX open() flags. `binary` is set true regardless -- we treat
// every mode as binary. Returns false if the mode is invalid.
static bool parse_open_mode(Zstr mode, int *out_flags) {
#if PLATFORM_WINDOWS
    (void)mode;
    (void)out_flags;
    return false; // Windows uses a different mode encoding (see FileOpen).
#else
    if (!mode || !*mode) {
        return false;
    }
    char primary = mode[0];
    bool plus    = false;
    for (Zstr p = mode + 1; *p; ++p) {
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

File file_open(Zstr path, Zstr mode) {
    File f = {0};
#if PLATFORM_WINDOWS
    f.handle = INVALID_HANDLE_VALUE;
    if (!path || !mode || !*mode) {
        return f;
    }
    DWORD access      = 0;
    DWORD disposition = OPEN_EXISTING;
    bool  plus        = false;
    for (Zstr p = mode + 1; *p; ++p) {
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
#    if PLATFORM_DARWIN
    flags |= 0x1000000; // Darwin O_CLOEXEC
#    else
    flags |= 0x80000; // Linux O_CLOEXEC
#    endif
    long fd;
#    if FEATURE_DIRECT_SYSCALL
#        if PLATFORM_DARWIN || ARCHITECTURE_X86_64
    // Darwin has SYS_open on both x86_64 and aarch64. Linux x86_64
    // does too; only Linux aarch64 went openat-only.
    fd = misra_sys3(MISRA_SYS_open, (long)(u64)path, (long)flags, 0644L);
#        else
    // Linux aarch64: openat(AT_FDCWD=-100, path, flags, mode).
    fd = misra_sys4(MISRA_SYS_openat, -100L, (long)(u64)path, (long)flags, 0644L);
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
#if PLATFORM_WINDOWS
    (void)fd;
    f.handle = INVALID_HANDLE_VALUE;
#else
    f.fd   = fd;
    f.owns = false;
#endif
    return f;
}

File FileStdin(void) {
#if PLATFORM_WINDOWS
    File f = {.handle = GetStdHandle(STD_INPUT_HANDLE), .owns = false};
    return f;
#else
    return FileFromFd(0);
#endif
}

File FileStdout(void) {
#if PLATFORM_WINDOWS
    File f = {.handle = GetStdHandle(STD_OUTPUT_HANDLE), .owns = false};
    return f;
#else
    return FileFromFd(1);
#endif
}

File FileStderr(void) {
#if PLATFORM_WINDOWS
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
#if PLATFORM_WINDOWS
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

bool FileIsOpen(const File *f) {
    if (!f) {
        return false;
    }
#if PLATFORM_WINDOWS
    return f->handle && f->handle != INVALID_HANDLE_VALUE;
#else
    return f->fd >= 0;
#endif
}

// ---------------------------------------------------------------------------
// Read / write / seek / tell
// ---------------------------------------------------------------------------

i64 file_read(File *f, void *buf, u64 n) {
    if (!FileIsOpen(f) || !buf) {
        return -1;
    }
    if (n == 0) {
        return 0;
    }
#if PLATFORM_WINDOWS
    DWORD got = 0;
    if (!ReadFile((HANDLE)f->handle, buf, (DWORD)n, &got, NULL)) {
        return -1;
    }
    if (got == 0) {
        f->at_eof = true;
    }
    return (i64)got;
#elif FEATURE_DIRECT_SYSCALL
    long r = misra_sys3(MISRA_SYS_read, (long)f->fd, (long)(u64)buf, (long)n);
    if (r < 0) {
        return -1;
    }
    if (r == 0) {
        f->at_eof = true;
    }
    return (i64)r;
#else
    ssize_t r = read(f->fd, buf, (size)n);
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
    if (!FileIsOpen(f) || !buf) {
        return -1;
    }
    if (n == 0) {
        return 0;
    }
#if PLATFORM_WINDOWS
    DWORD put = 0;
    if (!WriteFile((HANDLE)f->handle, buf, (DWORD)n, &put, NULL)) {
        return -1;
    }
    return (i64)put;
#elif FEATURE_DIRECT_SYSCALL
    long r = misra_sys3(MISRA_SYS_write, (long)f->fd, (long)(u64)buf, (long)n);
    if (r < 0) {
        return -1;
    }
    return (i64)r;
#else
    ssize_t r = write(f->fd, buf, (size)n);
    if (r < 0) {
        return -1;
    }
    return (i64)r;
#endif
}

i64 FileSeek(File *f, i64 offset, FileWhence whence) {
    if (!FileIsOpen(f)) {
        return -1;
    }
    f->at_eof = false;
#if PLATFORM_WINDOWS
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
    if (!FileIsOpen(f)) {
        return false;
    }
#if PLATFORM_WINDOWS
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
#if PLATFORM_WINDOWS
    (void)f;
    return -1;
#else
    return f ? f->fd : -1;
#endif
}

// ---------------------------------------------------------------------------
// FileRead-to-container helpers. Read to EOF, growing the destination
// through its own allocator. The Str / Buf forms are the canonical
// public API; the byte-pointer form (file_read) sits underneath both.
// ---------------------------------------------------------------------------

// Block size for the read loop. Large enough to amortise the syscall
// cost on big files; small enough that the dest doesn't waste memory
// on a single huge realloc when the file turns out tiny.
#define FILE_READ_CHUNK 4096

// fd-based remaining-size probe used by file_read_to_str. Returns
// -1 for non-seekable streams (pipes, sockets, /dev/*) where the
// seek-to-end dance fails -- caller falls back to a grow-loop.
// (Distinct from `Sys/Dir.c`'s path-based `file_get_size`, which
// opens its own fd; here we already have an open `File *`.)
static i64 file_remaining_size(File *f) {
    i64 here = FileTell(f);
    if (here < 0) {
        return -1;
    }
    i64 end = FileSeek(f, 0, FILE_SEEK_END);
    if (end < 0 || end < here) {
        return -1;
    }
    // Restore the read cursor regardless of whether the caller uses
    // the size.
    if (FileSeek(f, here, FILE_SEEK_SET) < 0) {
        return -1;
    }
    return end - here;
}

// Canonical slurp into a byte buffer. Fast path: for seekable files,
// learn the remaining size up front and reserve once. The geometric
// grow-loop below would otherwise do ~log2(size / FILE_READ_CHUNK)
// reallocations, each of which `MemCopy`s the buffer-so-far into a
// larger one -- 99%+ of wall-clock when called on multi-MB inputs
// under ASan (measured: ELF/DWARF/Backtrace/SymbolResolver tests all
// slurp /proc/self/exe through this path).
//
// Non-seekable streams (pipes, sockets, /dev/*) return -1 from
// `file_remaining_size`; the grow-loop below handles them.
i64 file_read_to_buf(File *f, Buf *out) {
    if (!FileIsOpen(f) || !out) {
        return -1;
    }
    // Reset out so repeated calls overwrite previous content. The
    // backing allocator stays; capacity is reused.
    if (!BufResize(out, 0)) {
        return -1;
    }

    i64 remaining = file_remaining_size(f);
    if (remaining > 0) {
        if (!VecReserve(out, (u64)remaining)) {
            LOG_ERROR("file_read_to_buf: VecReserve failed at size {}", (u64)remaining);
            return -1;
        }
    }

    i64 total = 0;
    for (;;) {
        // Vec keeps a sentinel slot at data[capacity], so VecReserve(n)
        // allocates (n + 1) bytes -- we don't add the trailing slot
        // ourselves. VecReserve only touches capacity; the read
        // syscall is what advances length. After the pre-sized
        // reserve above this is a no-op until we hit the upfront cap.
        if (!VecReserve(out, BufLength(out) + FILE_READ_CHUNK)) {
            LOG_ERROR("file_read_to_buf: VecReserve failed at length {}", (u64)BufLength(out));
            return -1;
        }
        i64 got = file_read(f, BufData(out) + BufLength(out), FILE_READ_CHUNK);
        if (got < 0) {
            return -1;
        }
        if (got == 0) {
            break; // EOF
        }
        if (!BufResize(out, BufLength(out) + (size)got)) {
            return -1;
        }
        total += got;
    }
    // Vec's reserved sentinel slot at data[capacity] is always
    // present after a successful VecReserve. Buf callers get a benign
    // sentinel byte past `length` they never observe; Str delegates
    // through and gets the NUL terminator it expects.
    BufData(out)[BufLength(out)] = 0;
    return total;
}

// `Str` is `Vec(char)`, `Buf` is `Vec(u8)`. Same layout (1-byte
// element, identical field offsets); the cast is safe. Kept as a
// thin shim so callers that legitimately know the input is text
// (e.g. JSON / config / source-file readers) get a typed `Str *`
// signature without manually casting.
i64 file_read_to_str(File *f, Str *out) {
    return file_read_to_buf(f, (Buf *)out);
}

// ---------------------------------------------------------------------------
// Convenience: open + read + close in one call. Parsers slurping a
// whole file used to do this dance by hand; centralising it removes
// the easy-to-forget close-on-error path.
// ---------------------------------------------------------------------------

i64 file_read_and_close_to_buf(Zstr path, Buf *out) {
    if (!path || !out) {
        LOG_FATAL("FileReadAndClose: NULL argument (contract violation)");
    }
    File f = file_open(path, "rb");
    if (!FileIsOpen(&f)) {
        return -1;
    }
    i64 got = file_read_to_buf(&f, out);
    FileClose(&f);
    return got;
}

i64 file_read_and_close_to_str(Zstr path, Str *out) {
    return file_read_and_close_to_buf(path, (Buf *)out);
}

i64 file_write_and_close_from_bytes(Zstr path, const void *buf, u64 n) {
    if (!path || (!buf && n > 0)) {
        LOG_FATAL("FileWriteAndClose: NULL argument (contract violation)");
    }
    File f = file_open(path, "wb");
    if (!FileIsOpen(&f)) {
        return -1;
    }
    i64 wrote = (n == 0) ? 0 : FileWrite(&f, buf, n);
    FileClose(&f);
    return wrote;
}

i64 file_write_and_close_from_buf(Zstr path, const Buf *in) {
    if (!in) {
        LOG_FATAL("FileWriteAndClose: NULL Buf (contract violation)");
    }
    return file_write_and_close_from_bytes(path, BufData(in), (u64)BufLength(in));
}

i64 file_write_and_close_from_str(Zstr path, const Str *in) {
    if (!in) {
        LOG_FATAL("FileWriteAndClose: NULL Str (contract violation)");
    }
    return file_write_and_close_from_bytes(path, StrBegin(in), (u64)StrLen(in));
}

// ---------------------------------------------------------------------------
// FileOpenTemp -- atomic-unique-create replacement for libc mkstemp.
// Filename entropy comes from the project-wide `Prng64`. Open is
// `O_RDWR | O_CREAT | O_EXCL | 0600` so two callers racing on the
// same prefix can never collide -- one wins, the other retries.
// ---------------------------------------------------------------------------

File file_open_temp(Str *out_path, Allocator *alloc) {
    File f = {0};
#if PLATFORM_WINDOWS
    f.handle = INVALID_HANDLE_VALUE;
#else
    f.fd = -1;
#endif
    if (!out_path || !alloc) {
        LOG_FATAL("FileOpenTemp: NULL argument");
    }

    // 8 attempts is overkill in practice -- collisions on a 16-hex-digit
    // namespace are vanishingly rare. The retry loop exists for
    // pathological filesystem races, not for entropy weakness.
    for (i32 attempt = 0; attempt < 8; ++attempt) {
        *out_path = StrInit(alloc);
        // {016x} = hex, zero-padded to 16 chars, no "0x" prefix. The
        // bare-hex name lands in CWD; callers that want a different
        // directory can `FileRename`/`MemCopy` the path before use.
        StrAppendFmt(out_path, "{016x}", Prng64());

#if PLATFORM_WINDOWS
        // CREATE_NEW fails with ERROR_FILE_EXISTS on collision. GENERIC_READ|WRITE
        // gives us the equivalent of POSIX O_RDWR.
        HANDLE h = CreateFileA(
            out_path->data,
            GENERIC_READ | GENERIC_WRITE,
            0, // no sharing -- mimic POSIX 0600
            NULL,
            CREATE_NEW,
            FILE_ATTRIBUTE_NORMAL,
            NULL
        );
        if (h != INVALID_HANDLE_VALUE) {
            f.handle = h;
            f.owns   = true;
            return f;
        }
        DWORD err = GetLastError();
        StrDeinit(out_path);
        if (err == ERROR_FILE_EXISTS) {
            continue; // retry with new entropy
        }
        LOG_ERROR("FileOpenTemp: CreateFileA failed (err {})", (u32)err);
        return f;
#else
        // O_RDWR | O_CREAT | O_EXCL, mode 0600. Flag values are the
        // same on Linux and Darwin for these three.
        // O_RDWR=2, O_CREAT=0x40 (Linux) / 0x200 (Darwin),
        // O_EXCL=0x80 (Linux) / 0x800 (Darwin).
#    if PLATFORM_DARWIN
        int flags = 2 | 0x200 | 0x800 | 0x1000000; // RDWR | CREAT | EXCL | CLOEXEC
#    else
        int flags = 2 | 0x40 | 0x80 | 0x80000; // RDWR | CREAT | EXCL | CLOEXEC
#    endif
        long fd;
#    if FEATURE_DIRECT_SYSCALL
#        if PLATFORM_DARWIN || ARCHITECTURE_X86_64
        fd = misra_sys3(MISRA_SYS_open, (long)(u64)out_path->data, (long)flags, 0600L);
#        else
        fd = misra_sys4(MISRA_SYS_openat, -100L, (long)(u64)out_path->data, (long)flags, 0600L);
#        endif
#    else
        extern int open(Zstr , int, ...);
        fd = open(out_path->data, flags, 0600);
        if (fd < 0) {
            fd = -Errno();
        }
#    endif
        if (fd >= 0) {
            f.fd   = (i32)fd;
            f.owns = true;
            return f;
        }
        i32 eno = ErrnoOf(fd);
        StrDeinit(out_path);
        if (eno == EEXIST) {
            continue; // collision: retry with new entropy
        }
        LOG_SYS_ERROR(eno, "FileOpenTemp: open failed");
        return f;
#endif
    }

    LOG_ERROR("FileOpenTemp: exhausted retries (8 successive Prng64 collisions -- broken PRNG?)");
    return f;
}
