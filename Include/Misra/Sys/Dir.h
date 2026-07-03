/// file      : sys/dir.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Directory enumeration and per-entry types. The POSIX backend issues
/// `getdents64` direct syscalls; the Windows backend uses
/// `FindFirstFile`/`FindNextFile`. Entries are returned as a typed
/// `DirContents` vector of `DirEntry` structs; each entry owns its
/// filename `Str` and must be released via `DirEntryDeinitCopy` (or
/// implicitly by `VecDeinit`-ing the `DirContents` if `copy_init` /
/// `copy_deinit` were wired at container init).
#ifndef MISRA_SYS_DIR_H
#define MISRA_SYS_DIR_H

#include <Misra/Std/Container/Str.h>
#include <Misra/Std/Container/Vec.h>
#include <Misra/Std/Zstr.h>

typedef enum DirEntryType {
    DIR_ENTRY_TYPE_UNKNOWN,
    DIR_ENTRY_TYPE_REGULAR_FILE,
    DIR_ENTRY_TYPE_DIRECTORY,
    DIR_ENTRY_TYPE_PIPE,
    DIR_ENTRY_TYPE_CHARACTER_DEVICE,
    DIR_ENTRY_TYPE_BLOCK_DEVICE,
    DIR_ENTRY_TYPE_SYMBOLIC_LINK
} DirEntryType;

///
/// Convert given entry type to a NULL terminated string.
/// Provided string must not be freed as it's not allocated.
///
/// type[in] : Entry type to get string of.
///
/// SUCCESS : Null terminated string describing entry type.
/// FAILURE : Function cannot fail - always returns valid string.
///
/// TAGS: System, Conversion, String, Utility
///
Zstr DirEntryTypeToZstr(DirEntryType type);

///
/// Directory entry structure containing type and name.
///
/// TAGS: System, Directory, Structure
///
typedef struct DirEntry {
    DirEntryType type;
    Str          name;
} DirEntry;

///
/// Initialize a copy of a directory entry. Copies `type` and deep-copies
/// `name` (the destination owns the new Str buffer through `src->name`'s
/// allocator).
///
/// dst[out] : Uninitialised entry receiving the copy.
/// src[in]  : Source entry.
///
/// SUCCESS : Returns `dst` with `type` set and `name` deep-copied; the
///           new `name` buffer is owned by the same allocator that
///           backed `src->name`.
/// FAILURE : Aborts via `LOG_FATAL` if either pointer is NULL.
///
/// TAGS: System, Directory, Structure, Copy
///
DirEntry *DirEntryInitCopy(DirEntry *dst, const DirEntry *src);

///
/// Deinitialize a directory entry previously produced by
/// `DirEntryInitCopy`. Releases the owned `name` buffer and zeroes the
/// `type` slot.
///
/// copy[in,out] : Entry to release.
///
/// SUCCESS : Returns `copy` with `name` freed (length and data cleared)
///           and `type` set to 0; the struct itself is left for the
///           caller to release.
/// FAILURE : Aborts via `LOG_FATAL` if `copy` is NULL.
///
/// TAGS: System, Directory, Structure, Cleanup
///
DirEntry *DirEntryDeinitCopy(DirEntry *copy);

///
/// Vector type for directory contents.
///
typedef Vec(DirEntry) DirContents;

// Path-arg dispatch. Library design: `Str *` is the canonical path
// form -- it carries length, can't silently lose the NUL terminator,
// and the in-tree allocators / fmt machinery already operate on it.
// `char *` is accepted as a bare-pointer convenience for string
// literals and borrowed NUL-terminated buffers.
//
// Each user-visible `FileX` / `DirX` macro inlines its own `_Generic`
// over the `path` argument -- deliberately, so we don't grow a
// reusable "extract zstr from X" helper. Other input types trigger
// a compile-time `_Generic` mismatch.

///
/// Read directory contents into a vector.
///
/// path[in]  : Path of directory. Prefer `Str *`; `Zstr `
///             accepted for literals / borrowed buffers.
/// alloc[in] : Allocator backing the returned vector + entry names
///             (omit inside a `Scope` block to use `MisraScope`).
///
/// SUCCESS : DirContents vector filled with directory contents data.
/// FAILURE : Returns empty vector on read error.
///
/// TAGS: System, FileSystem, Directory
///
DirContents dir_get_contents(Zstr path, Allocator *alloc);
DirContents dir_get_contents_cstr(Zstr path, size len, Allocator *alloc);
#define DirGetContents(...) OVERLOAD(DirGetContents, __VA_ARGS__)
#define DirGetContents_1(path)                                                                                         \
    _Generic(                                                                                                          \
        (path),                                                                                                        \
        Str *: dir_get_contents((Zstr)StrBegin((Str *)(path)), MisraScope),                                            \
        Zstr: dir_get_contents((Zstr)(path), MisraScope),                                                              \
        char *: dir_get_contents((Zstr)(path), MisraScope)                                                             \
    )
#define DirGetContents_2(path, alloc)                                                                                  \
    _Generic(                                                                                                          \
        (path),                                                                                                        \
        Str *: dir_get_contents((Zstr)StrBegin((Str *)(path)), ALLOCATOR_OF(alloc)),                                   \
        Zstr: dir_get_contents((Zstr)(path), ALLOCATOR_OF(alloc)),                                                     \
        char *: dir_get_contents((Zstr)(path), ALLOCATOR_OF(alloc))                                                    \
    )
#define DirGetContents_3(path, len, alloc) dir_get_contents_cstr((Zstr)(path), (len), ALLOCATOR_OF(alloc))

///
/// Get size of file without opening it.
///
/// path[in] : Path of file. Prefer `Str *`; `Zstr ` accepted.
///
/// SUCCESS : Non-negative value representing size of file in bytes.
/// FAILURE : Returns -1 if file cannot be accessed.
///
/// TAGS: System, File, Metadata
///
i64 file_get_size(Zstr filename);
i64 file_get_size_cstr(Zstr filename, size len);
#define FileGetSize(...) OVERLOAD(FileGetSize, __VA_ARGS__)
#define FileGetSize_1(path)                                                                                            \
    _Generic(                                                                                                          \
        (path),                                                                                                        \
        Str *: file_get_size((Zstr)StrBegin((Str *)(path))),                                                           \
        Zstr: file_get_size((Zstr)(path)),                                                                             \
        char *: file_get_size((Zstr)(path))                                                                            \
    )
#define FileGetSize_2(path, len) file_get_size_cstr((Zstr)(path), (len))

///
/// Remove a regular file. Direct syscall (`unlink` on Linux x86_64 /
/// Darwin, `unlinkat` on Linux aarch64, `DeleteFileA` on Windows).
/// Symlinks are removed, not followed.
///
/// Return type is `i8` (0/1) rather than `bool` to dodge the
/// `bool`-typedef redefinition trap Misra's Types.h documents:
/// system headers transitively `#define bool _Bool` and the SAME
/// identifier ends up meaning different types across TUs. Callers
/// can still use the result as a boolean (`if (FileRemove(p)) ...`).
///
/// path[in] : Path of the file to remove. Prefer `Str *`; `Zstr`
///            accepted for literals / borrowed buffers.
///
/// SUCCESS : Returns 1; the directory entry is gone.
/// FAILURE : Returns 0; logs the failing syscall.
///
/// TAGS: System, File, FileSystem
///
i8 file_remove(Zstr path);
i8 file_remove_cstr(Zstr path, size len);
#define FileRemove(...) OVERLOAD(FileRemove, __VA_ARGS__)
#define FileRemove_1(path)                                                                                             \
    _Generic(                                                                                                          \
        (path),                                                                                                        \
        Str *: file_remove((Zstr)StrBegin((Str *)(path))),                                                             \
        Zstr: file_remove((Zstr)(path)),                                                                               \
        char *: file_remove((Zstr)(path))                                                                              \
    )
#define FileRemove_2(path, len) file_remove_cstr((Zstr)(path), (len))

///
/// Remove an empty directory. Direct syscall (`rmdir` on Linux
/// x86_64 / Darwin, `unlinkat(... AT_REMOVEDIR)` on Linux aarch64,
/// `RemoveDirectoryA` on Windows). The directory must be empty;
/// populated directories require recursive removal via `DirRemoveAll`.
///
/// path[in] : Path of the directory. Prefer `Str *`; `Zstr `
///            accepted.
///
/// SUCCESS : Returns 1; the directory is gone.
/// FAILURE : Returns 0; logs the failing syscall.
///
/// TAGS: System, Directory, FileSystem
///
i8 dir_remove(Zstr path);
i8 dir_remove_cstr(Zstr path, size len);
#define DirRemove(...) OVERLOAD(DirRemove, __VA_ARGS__)
#define DirRemove_1(path)                                                                                              \
    _Generic(                                                                                                          \
        (path),                                                                                                        \
        Str *: dir_remove((Zstr)StrBegin((Str *)(path))),                                                              \
        Zstr: dir_remove((Zstr)(path)),                                                                                \
        char *: dir_remove((Zstr)(path))                                                                               \
    )
#define DirRemove_2(path, len) dir_remove_cstr((Zstr)(path), (len))

///
/// Create a single directory. Direct syscall (`mkdir` on Linux
/// x86_64 / Darwin, `mkdirat` on Linux aarch64, `CreateDirectoryA`
/// on Windows). Mode is 0755 on POSIX. Fails if a parent component
/// is missing -- use `DirCreateAll` for `mkdir -p` semantics.
///
/// path[in] : Path of the directory. Prefer `Str *`; `Zstr `
///            accepted.
///
/// SUCCESS : Returns 1; the directory now exists.
/// FAILURE : Returns 0; logs the failing syscall.
///
/// TAGS: System, Directory, FileSystem
///
i8 dir_create(Zstr path);
i8 dir_create_cstr(Zstr path, size len);
#define DirCreate(...) OVERLOAD(DirCreate, __VA_ARGS__)
#define DirCreate_1(path)                                                                                              \
    _Generic(                                                                                                          \
        (path),                                                                                                        \
        Str *: dir_create((Zstr)StrBegin((Str *)(path))),                                                              \
        Zstr: dir_create((Zstr)(path)),                                                                                \
        char *: dir_create((Zstr)(path))                                                                               \
    )
#define DirCreate_2(path, len) dir_create_cstr((Zstr)(path), (len))

///
/// Recursive `mkdir -p`. Creates all missing path components.
/// Existing components are treated as success (EEXIST is not an
/// error). Trailing slashes are tolerated.
///
/// path[in] : Path of the directory tree. Prefer `Str *`; `Zstr`
///            accepted.
///
/// SUCCESS : Returns 1; the full path now exists as a directory.
/// FAILURE : Returns 0 on first un-recoverable error.
///
/// TAGS: System, Directory, FileSystem
///
i8 dir_create_all(Zstr path);
i8 dir_create_all_cstr(Zstr path, size len);
#define DirCreateAll(...) OVERLOAD(DirCreateAll, __VA_ARGS__)
#define DirCreateAll_1(path)                                                                                           \
    _Generic(                                                                                                          \
        (path),                                                                                                        \
        Str *: dir_create_all((Zstr)StrBegin((Str *)(path))),                                                          \
        Zstr: dir_create_all((Zstr)(path)),                                                                            \
        char *: dir_create_all((Zstr)(path))                                                                           \
    )
#define DirCreateAll_2(path, len) dir_create_all_cstr((Zstr)(path), (len))

///
/// Recursive `rm -rf`. Removes a directory tree (regular files,
/// symlinks, subdirectories). If `path` doesn't exist this is a
/// no-op success. Follows the cross-platform `i8` return convention.
///
/// path[in] : Root of the tree. Prefer `Str *`; `Zstr ` accepted.
///
/// SUCCESS : Returns 1; the path is gone (or never existed).
/// FAILURE : Returns 0 on first un-recoverable error.
///
/// TAGS: System, Directory, FileSystem
///
i8 dir_remove_all(Zstr path);
i8 dir_remove_all_cstr(Zstr path, size len);
#define DirRemoveAll(...) OVERLOAD(DirRemoveAll, __VA_ARGS__)
#define DirRemoveAll_1(path)                                                                                           \
    _Generic(                                                                                                          \
        (path),                                                                                                        \
        Str *: dir_remove_all((Zstr)StrBegin((Str *)(path))),                                                          \
        Zstr: dir_remove_all((Zstr)(path)),                                                                            \
        char *: dir_remove_all((Zstr)(path))                                                                           \
    )
#define DirRemoveAll_2(path, len) dir_remove_all_cstr((Zstr)(path), (len))

#endif // MISRA_SYS_DIR_H
