/// file      : misra/std/file.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// File helper utilities

#ifndef MISRA_FILE_H
#define MISRA_FILE_H


// decompiler
#include <Misra/Std/Allocator.h>
#include <Misra/Std/Container/Str.h>
#include <Misra/Sys.h>
#include <Misra/Types.h>

///
/// Cross-platform file handle. Replaces stdio `FILE *` in the
/// project: Linux uses an fd through direct syscalls, macOS uses an
/// fd through libSystem, Windows uses a `HANDLE`. The API surface
/// (`FileOpen` / `Close` / `Read` / `Write` / `Seek` / `Tell` /
/// `Flush` / `Eof`) is identical across platforms.
///
/// Value type -- caller stack-allocates and passes by pointer. A
/// failed open leaves `fd` (or `handle`) negative / INVALID; check
/// with `FileIsOpen` after open.
///
typedef struct File {
#ifdef _WIN32
    void *handle; // HANDLE (kept as void* so we don't pull <windows.h>)
#else
    i32 fd; // -1 if not open
#endif
    bool at_eof;  // last read returned 0 bytes
    bool owns;    // true when FileClose should release the handle
} File;

///
/// Whence values for `FileSeek`, matching the libc `SEEK_*` constants
/// (numerically identical so a caller that's used to stdio
/// transitions cleanly).
///
typedef enum FileWhence {
    FILE_SEEK_SET = 0,
    FILE_SEEK_CUR = 1,
    FILE_SEEK_END = 2,
} FileWhence;

// File API path-arg dispatch. `Str *` and `const Str *` are the
// canonical forms; any other pointer type (including `char *` /
// `const char *`) routes through the `default:` arm and is treated
// as a NUL-terminated C string. Each macro inlines its own
// `_Generic` -- we intentionally do not share a dispatch helper
// across APIs.

///
/// Open a file. `mode` is libc-compatible: `"r"`/`"rb"`, `"w"`/`"wb"`,
/// `"a"`/`"ab"`, `"r+"`, `"w+"`, `"a+"`. Binary is always implied;
/// the `"b"` suffix is accepted but has no effect on the
/// implementation.
///
/// path[in] : Path to open. Prefer `Str *` (carries length, can't
///            silently drop the NUL terminator). `const char *` is
///            accepted as a literal/borrowed-buffer convenience.
///
/// SUCCESS : Returns a File where `FileIsOpen(&out)` is true.
/// FAILURE : Returns a File where `FileIsOpen(&out)` is false.
///
File file_open(const char *path, const char *mode);
#define FileOpen(path, mode)                                                                                           \
    _Generic(                                                                                                          \
        (path),                                                                                                        \
        Str *: file_open(((Str *)(path))->data, (mode)),                                                               \
        const Str *: file_open(((const Str *)(path))->data, (mode)),                                                   \
        default: file_open((const char *)(path), (mode))                                                               \
    )

///
/// Borrow a file handle wrapping an already-open fd / HANDLE. The
/// returned File has `owns = false` so `FileClose` is a no-op on it.
/// Use for wrapping stdin / stdout / stderr or fds you got from
/// elsewhere.
///
File FileFromFd(i32 fd);

///
/// stdin / stdout / stderr accessors. Wrapping the well-known fds
/// 0/1/2 on POSIX, the GetStdHandle() values on Windows.
///
File FileStdin(void);
File FileStdout(void);
File FileStderr(void);

///
/// Close a file we opened via FileOpen. Idempotent and safe on a
/// borrowed (`owns == false`) handle; in that case it just clears
/// the local state without touching the underlying fd / HANDLE.
///
/// SUCCESS : Returns true.
/// FAILURE : Returns false if the close syscall failed (logged).
///
bool FileClose(File *f);

///
/// True if the underlying handle is currently open.
///
bool FileIsOpen(const File *f);

///
/// Read bytes from a file. Two arities via `MISRA_OVERLOAD`:
///
///   `FileRead(f, buf, n)`  - low-level: up to `n` bytes into `buf`.
///                            Short reads are normal at EOF and on
///                            signal interruption (we don't retry on
///                            EINTR for you -- callers loop if they
///                            need that). Sets `at_eof` when the
///                            syscall reports zero bytes.
///   `FileRead(f, out)`     - read to EOF into the `Str *out`. The
///                            existing length is overwritten; `out`
///                            grows through its own allocator. The
///                            byte buffer is NUL-terminated for
///                            convenience.
///
/// `out` must be an already-init'd `Str *` (its allocator drives the
/// growth). Calling `FileRead(f, out)` more than once on the same
/// `Str` simply overwrites previous content.
///
/// SUCCESS : 3-arg form returns bytes read (>= 0); 2-arg form
///           returns total bytes loaded (== `out->length`).
/// FAILURE : Returns -1 on I/O error. The 2-arg form may leave `out`
///           in a partial state.
///
/// TAGS: File, Read
///
i64 file_read(File *f, void *buf, u64 n);
i64 file_read_to_str(File *f, Str *out);
#define FileRead(...)         MISRA_OVERLOAD(FileRead, __VA_ARGS__)
#define FileRead_2(f, out)    file_read_to_str((f), (out))
#define FileRead_3(f, buf, n) file_read((f), (buf), (n))

///
/// Write up to `n` bytes from `buf`. Same short-write semantics as
/// FileRead.
///
/// SUCCESS : Returns the number of bytes written (>= 0).
/// FAILURE : Returns -1 on error.
///
i64 FileWrite(File *f, const void *buf, u64 n);

///
/// Adjust the file offset relative to `whence`.
///
/// SUCCESS : Returns the new absolute file offset (>= 0).
/// FAILURE : Returns -1 on error (typically ESPIPE for a pipe/tty).
///
i64 FileSeek(File *f, i64 offset, FileWhence whence);

///
/// Return the current file offset.
///
/// SUCCESS : Returns the offset (>= 0).
/// FAILURE : Returns -1 on error.
///
i64 FileTell(File *f);

///
/// Flush any kernel-side buffering for the file. On POSIX with no
/// user-side buffering this is a no-op success unless the platform
/// needs an fsync (we don't fsync today; callers that need durable
/// writes should fsync the fd themselves). On Windows it calls
/// FlushFileBuffers.
///
/// SUCCESS : Returns true.
/// FAILURE : Returns false.
///
bool FileFlush(File *f);

///
/// True if a previous FileRead returned 0 bytes.
///
bool FileIsEof(const File *f);

///
/// Return the underlying fd. POSIX-only; on Windows this returns -1.
/// Useful for syscalls that need a raw fd (e.g. isatty checks).
///
i32 FileFd(const File *f);

///
/// Open a unique temporary file for read+write. Replaces libc
/// `mkstemp`. The 16-hex-digit suffix comes from the internal
/// `Prng64` (no kernel entropy needed per call). Open is
/// `O_RDWR | O_CREAT | O_EXCL | 0600`, so two callers racing on the
/// same prefix can never collide -- one wins, the other retries.
///
/// Two forms via argument-count overload (allocator backs `out_path`):
///
/// - `FileOpenTemp(prefix, out_path)` -- inside a `Scope`; allocator
///   is `MisraScope`.
/// - `FileOpenTemp(prefix, out_path, allocator)` -- explicit allocator.
///
/// prefix[in]    : Path prefix. Prefer `Str *`; `const char *`
///                 accepted for literals. Final path is
///                 `<prefix><hex>`.
/// out_path[out] : Fresh `Str *` -- receives the resolved path.
///                 Caller `StrDeinit`s when done; the on-disk file
///                 is NOT auto-removed (use `FileRemove(out_path)`).
///
/// SUCCESS : Returns an open `File` with `FileIsOpen` true.
/// FAILURE : Returns a `File` where `FileIsOpen` is false.
///
/// TAGS: File, Temp
///
File file_open_temp(const char *prefix, Str *out_path, Allocator *alloc);
#define FileOpenTemp(...) MISRA_OVERLOAD(FileOpenTemp, __VA_ARGS__)
#define FileOpenTemp_2(prefix, out_path)                                                                               \
    _Generic(                                                                                                          \
        (prefix),                                                                                                      \
        Str *: file_open_temp(((Str *)(prefix))->data, (out_path), MisraScope),                                        \
        const Str *: file_open_temp(((const Str *)(prefix))->data, (out_path), MisraScope),                            \
        default: file_open_temp((const char *)(prefix), (out_path), MisraScope)                                        \
    )
#define FileOpenTemp_3(prefix, out_path, alloc)                                                                        \
    _Generic(                                                                                                          \
        (prefix),                                                                                                      \
        Str *: file_open_temp(((Str *)(prefix))->data, (out_path), ALLOCATOR_OF(alloc)),                               \
        const Str *: file_open_temp(((const Str *)(prefix))->data, (out_path), ALLOCATOR_OF(alloc)),                   \
        default: file_open_temp((const char *)(prefix), (out_path), ALLOCATOR_OF(alloc))                               \
    )

#endif // MISRA_FILE_H
