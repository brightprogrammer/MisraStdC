/// file      : misra/std/file.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// File helper utilities

#ifndef MISRA_FILE_H
#define MISRA_FILE_H

#include <stddef.h>

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
/// with `FileIsValid` before doing anything else with the handle.
///
/// Open modes are libc-compatible (`"r"`, `"w"`, `"a"`, `"r+"`,
/// `"w+"`, `"a+"`), `"b"` suffix is accepted and ignored -- the
/// implementation is always binary. No buffering today.
///
/// TAGS: File, IO, Handle, Cross-Platform
///
typedef struct File {
#ifdef _WIN32
    // HANDLE -- kept as void* so the header doesn't drag in
    // <windows.h>. INVALID_HANDLE_VALUE == (void*)-1.
    void *handle;
#else
    i32 fd; // -1 if not open
#endif
    bool at_eof; // last read returned 0 bytes
    bool owns;   // true when FileClose should release the handle
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

///
/// Open a file. `mode` is libc-compatible: `"r"`/`"rb"`, `"w"`/`"wb"`,
/// `"a"`/`"ab"`, `"r+"`/`"rb+"`/`"r+b"`, `"w+"`, `"a+"`. Binary is
/// always implied; the `"b"` suffix is accepted for compatibility but
/// has no effect on the implementation.
///
/// SUCCESS : Returns a File where `FileIsValid(&out)` is true.
/// FAILURE : Returns a File where `FileIsValid(&out)` is false.
///
File FileOpen(const char *path, const char *mode);

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
/// True if the underlying handle is valid (open).
///
bool FileIsValid(const File *f);

///
/// Read up to `n` bytes into `buf`. Short reads are normal at EOF
/// and on signal interruption (we don't retry on EINTR for you --
/// callers that need that should loop). Sets the at_eof flag when
/// the syscall reports zero bytes.
///
/// SUCCESS : Returns the number of bytes read (>= 0).
/// FAILURE : Returns -1 on error.
///
i64 FileRead(File *f, void *buf, u64 n);

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

/// Snake_case runtime helper. Users call `ReadCompleteFile(...)`; the
/// macro routes through `MISRA_OVERLOAD` to one of the per-arity forms
/// below, which forward to this function.
bool read_complete_file(const char *filename, char **data, u64 *file_size, u64 *capacity, Allocator *allocator);

///
/// Read complete contents of a file at once.
///
/// Two forms via argument-count overload:
///
/// - `ReadCompleteFile(filename, data, file_size, capacity)` - inside a
///   `Scope` block; the buffer is allocated through `MisraScope`.
/// - `ReadCompleteFile(filename, data, file_size, capacity, allocator)`
///   - explicit allocator (typed handle or raw `Allocator *`).
///
/// The 4-arg form fails to compile outside any `Scope` block because
/// `MisraScope` is undeclared - the library does not accept a NULL
/// allocator anywhere.
///
/// filename[in]     : Name/path of file to be read.
/// data[in,out]     : Memory buffer where loaded file will be stored.
///                    The buffer is null-terminated for convenience.
/// file_size[out]   : Complete size of file in bytes.
/// capacity[in,out] : Current capacity of `*data`, updated on successful growth.
/// allocator[in,out]: Allocator responsible for `*data`.
///
/// SUCCESS : Returns true. `*data`, `*file_size`, and `*capacity` are updated.
/// FAILURE : Returns false on I/O or allocation failure. The buffer state
///           may be partially updated.
///
/// TAGS: Read, File, I/O, Utility, Allocator
///
#define ReadCompleteFile(...) MISRA_OVERLOAD(ReadCompleteFile, __VA_ARGS__)
#define ReadCompleteFile_4(filename, data, file_size, capacity)                                                        \
    read_complete_file((filename), (data), (file_size), (capacity), MisraScope)
#define ReadCompleteFile_5(filename, data, file_size, capacity, allocator)                                             \
    read_complete_file((filename), (data), (file_size), (capacity), (allocator))

#endif // MISRA_FILE_H
