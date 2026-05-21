/// file      : misra/std/file.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// File helper utilities

#ifndef MISRA_FILE_H
#define MISRA_FILE_H


// decompiler
#include <Misra/Std/Allocator.h>
#include <Misra/Std/Container/Buf.h>
#include <Misra/Std/Container/Str.h>
#include <Misra/Std/Zstr.h>
#include <Misra/Sys.h>
#include <Misra/Types.h>

///
/// Cross-platform file handle. Linux uses an fd through direct
/// syscalls, macOS uses an fd through libSystem, Windows uses a
/// `HANDLE`. The API surface (`FileOpen` / `Close` / `Read` / `Write`
/// / `Seek` / `Tell` / `Flush` / `Eof`) is identical across platforms.
///
/// Value type -- caller stack-allocates and passes by pointer. A
/// failed open leaves `fd` (or `handle`) negative / INVALID; check
/// with `FileIsOpen` after open.
///
typedef struct File {
#if PLATFORM_WINDOWS
    void *handle; // HANDLE (kept as void* so we don't pull <windows.h>)
#else
    i32 fd; // -1 if not open
#endif
    bool at_eof;  // last read returned 0 bytes
    bool owns;    // true when FileClose should release the handle
} File;

///
/// Whence values for `FileSeek`. `SET` anchors to the file start,
/// `CUR` to the current position, `END` to the file end.
///
typedef enum FileWhence {
    FILE_SEEK_SET = 0,
    FILE_SEEK_CUR = 1,
    FILE_SEEK_END = 2,
} FileWhence;

// File API path-arg dispatch. `Str *` / `const Str *` are the
// canonical forms; `char *` / `const char *` are accepted as a
// NUL-terminated C-string convenience for literals and borrowed
// buffers. Any other input type triggers a compile-time `_Generic`
// mismatch -- silently casting `int *` or a struct pointer to
// `const char *` is precisely the bug we want to surface. Each
// macro inlines its own `_Generic`; we intentionally do not share
// a dispatch helper across APIs.

///
/// Open a file. `mode` accepts: `"r"`/`"rb"`, `"w"`/`"wb"`,
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
File file_open(Zstr path, Zstr mode);
#define FileOpen(path, mode)                                                                                           \
    _Generic(                                                                                                          \
        (path),                                                                                                        \
        Str *: file_open(((Str *)(path))->data, (mode)),                                                               \
        char *: file_open((const char *)(path), (mode)),                                                               \
        const char *: file_open((const char *)(path), (mode))                                                          \
    )

///
/// Borrow a file handle wrapping an already-open fd / HANDLE. The
/// returned File has `owns = false` so `FileClose` is a no-op on it.
/// Use for wrapping the well-known standard streams or fds you got
/// from elsewhere.
///
File FileFromFd(i32 fd);

///
/// Standard input / output / error accessors. Wrapping the
/// well-known fds 0/1/2 on POSIX, the GetStdHandle() values on
/// Windows.
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
///   `FileRead(f, out)`     - read to EOF into the container `out`.
///                            `out` may be a `Buf *` (binary payload --
///                            preferred for parser-input slurps) or a
///                            `Str *` (text payload -- only when the
///                            caller knows the file is text). Existing
///                            length is overwritten; `out` grows
///                            through its own allocator.
///
/// `out` must be an already-init'd `Buf *` or `Str *` (its allocator
/// drives the growth). Calling `FileRead(f, out)` more than once on
/// the same container simply overwrites previous content. Both forms
/// place a trailing zero past `length` (NUL terminator for the Str
/// form, a benign sentinel byte for Buf).
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
i64 file_read_to_buf(File *f, Buf *out);
#define FileRead(...) MISRA_OVERLOAD(FileRead, __VA_ARGS__)
#define FileRead_2(f, out)                                                                                             \
    _Generic((out), Buf *: file_read_to_buf((f), (Buf *)(out)), Str *: file_read_to_str((f), (Str *)(out)))
#define FileRead_3(f, buf, n) file_read((f), (buf), (n))

///
/// Slurp a file from disk in one call: open, read-to-EOF into `out`,
/// close. `out` may be a `Buf *` (binary) or `Str *` (text). Removes
/// the open/read/close ceremony that's nearly identical across every
/// parser caller. The fast path inside `file_read_to_{buf,str}` (one-
/// shot reserve via `FileSeek(END)`) still applies.
///
/// path[in] : Path to open. `Str *` / `char *` (NUL-terminated).
/// out[out] : Already-init'd `Buf *` or `Str *`. Existing content is
///            overwritten; the destination's allocator drives growth.
///
/// SUCCESS : Returns bytes loaded (>= 0); file is closed.
/// FAILURE : Returns -1; file is closed; `out` may be in a partial
///           state -- caller should `BufDeinit` / `StrDeinit` if it
///           was a fresh container.
///
/// TAGS: File, Read
///
i64 file_read_and_close_to_buf(Zstr path, Buf *out);
i64 file_read_and_close_to_str(Zstr path, Str *out);
#define FileReadAndClose(path, out)                                                                                    \
    _Generic(                                                                                                          \
        (out),                                                                                                         \
        Buf *: _Generic(                                                                                               \
            (path),                                                                                                    \
            Str *: file_read_and_close_to_buf(((Str *)(path))->data, (Buf *)(out)),                                    \
            char *: file_read_and_close_to_buf((const char *)(path), (Buf *)(out)),                                    \
            const char *: file_read_and_close_to_buf((const char *)(path), (Buf *)(out))                               \
        ),                                                                                                             \
        Str *: _Generic(                                                                                               \
            (path),                                                                                                    \
            Str *: file_read_and_close_to_str(((Str *)(path))->data, (Str *)(out)),                                    \
            char *: file_read_and_close_to_str((const char *)(path), (Str *)(out)),                                    \
            const char *: file_read_and_close_to_str((const char *)(path), (Str *)(out))                               \
        )                                                                                                              \
    )

// FileGetSize lives in `Sys/Dir.h` -- path-based size query that
// goes straight to the kernel (open + lseek(SEEK_END) + close, or
// GetFileSizeEx on Windows). Use it when you have a path; the
// `file_read_to_str` fast path uses an fd-based equivalent because
// it already has an open `File *`.

///
/// Write up to `n` bytes from `buf`. Same short-write semantics as
/// FileRead.
///
/// SUCCESS : Returns the number of bytes written (>= 0).
/// FAILURE : Returns -1 on error.
///
i64 FileWrite(File *f, const void *buf, u64 n);

///
/// Open `path` for write (truncating), write all of `out`, close.
/// Two arities via `MISRA_OVERLOAD`:
///   - `FileWriteAndClose(path, container)` -- `container` is `Buf *`
///     or `Str *`; writes its full `length`.
///   - `FileWriteAndClose(path, buf, n)`    -- explicit `void *buf`
///     and `u64 n` byte count.
///
/// SUCCESS : Returns total bytes written; file is closed.
/// FAILURE : Returns -1; file is closed (or never opened).
///
/// TAGS: File, Write
///
i64 file_write_and_close_from_buf(Zstr path, const Buf *in);
i64 file_write_and_close_from_str(Zstr path, const Str *in);
i64 file_write_and_close_from_bytes(Zstr path, const void *buf, u64 n);
#define FileWriteAndClose(...) MISRA_OVERLOAD(FileWriteAndClose, __VA_ARGS__)
#define FileWriteAndClose_2(path, container)                                                                           \
    _Generic(                                                                                                          \
        (container),                                                                                                   \
        Buf *: _Generic(                                                                                               \
            (path),                                                                                                    \
            Str *: file_write_and_close_from_buf(((Str *)(path))->data, (const Buf *)(container)),                     \
            char *: file_write_and_close_from_buf((const char *)(path), (const Buf *)(container)),                     \
            const char *: file_write_and_close_from_buf((const char *)(path), (const Buf *)(container))                \
        ),                                                                                                             \
        Str *: _Generic(                                                                                               \
            (path),                                                                                                    \
            Str *: file_write_and_close_from_str(((Str *)(path))->data, (const Str *)(container)),                     \
            char *: file_write_and_close_from_str((const char *)(path), (const Str *)(container)),                     \
            const char *: file_write_and_close_from_str((const char *)(path), (const Str *)(container))                \
        )                                                                                                              \
    )
#define FileWriteAndClose_3(path, buf, n)                                                                              \
    _Generic(                                                                                                          \
        (path),                                                                                                        \
        Str *: file_write_and_close_from_bytes(((Str *)(path))->data, (buf), (n)),                                     \
        char *: file_write_and_close_from_bytes((const char *)(path), (buf), (n)),                                     \
        const char *: file_write_and_close_from_bytes((const char *)(path), (buf), (n))                                \
    )

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
/// Open a unique temporary file for read+write. The name is the
/// 16-hex-digit of `Prng64()` -- no
/// caller-supplied prefix, no kernel entropy per call. Open is
/// `O_RDWR | O_CREAT | O_EXCL | 0600`, so two callers racing can
/// never collide -- one wins, the other retries with a new draw.
/// The file lands in the process's current working directory.
///
/// Two forms via argument-count overload (allocator backs `out_path`):
///
/// - `FileOpenTemp(out_path)` -- inside a `Scope`; allocator is
///   `MisraScope`.
/// - `FileOpenTemp(out_path, allocator)` -- explicit allocator.
///
/// out_path[out] : Fresh `Str *` -- receives the resolved 16-char
///                 hex name. Caller `StrDeinit`s when done; the
///                 on-disk file is NOT auto-removed (use
///                 `FileRemove(out_path)`).
///
/// SUCCESS : Returns an open `File` with `FileIsOpen` true.
/// FAILURE : Returns a `File` where `FileIsOpen` is false.
///
/// TAGS: File, Temp
///
File file_open_temp(Str *out_path, Allocator *alloc);
#define FileOpenTemp(...)               MISRA_OVERLOAD(FileOpenTemp, __VA_ARGS__)
#define FileOpenTemp_1(out_path)        file_open_temp((out_path), MisraScope)
#define FileOpenTemp_2(out_path, alloc) file_open_temp((out_path), ALLOCATOR_OF(alloc))

#endif // MISRA_FILE_H
