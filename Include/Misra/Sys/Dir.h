#ifndef MISRA_SYS_DIR_H
#define MISRA_SYS_DIR_H

#include <Misra/Std/Container/Str.h>

typedef enum DirEntryType {
    SYS_DIR_ENTRY_TYPE_UNKNOWN,
    SYS_DIR_ENTRY_TYPE_REGULAR_FILE,
    SYS_DIR_ENTRY_TYPE_DIRECTORY,
    SYS_DIR_ENTRY_TYPE_PIPE,
    SYS_DIR_ENTRY_TYPE_CHARACTER_DEVICE,
    SYS_DIR_ENTRY_TYPE_BLOCK_DEVICE,
    SYS_DIR_ENTRY_TYPE_SYMBOLIC_LINK
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
const char *DirEntryTypeToZstr(DirEntryType type);

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
/// Initialize a copy of directory entry.
///
DirEntry *DirEntryInitCopy(DirEntry *dst, DirEntry *src);

///
/// Deinitialize a copied directory entry.
///
DirEntry *DirEntryDeinitCopy(DirEntry *copy);

///
/// Vector type for directory contents.
///
typedef Vec(DirEntry) DirContents;

// Path-arg dispatch. Library design: `Str *` is the canonical path
// form -- it carries length, can't silently lose the NUL terminator,
// and the no-libc allocators / fmt machinery already operate on it.
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
/// path[in]  : Path of directory. Prefer `Str *`; `const char *`
///             accepted for literals / borrowed buffers.
/// alloc[in] : Allocator backing the returned vector + entry names
///             (omit inside a `Scope` block to use `MisraScope`).
///
/// SUCCESS : DirContents vector filled with directory contents data.
/// FAILURE : Returns empty vector on read error.
///
/// TAGS: System, FileSystem, Directory
///
DirContents dir_get_contents(const char *path, Allocator *alloc);
#define DirGetContents(...) MISRA_OVERLOAD(DirGetContents, __VA_ARGS__)
#define DirGetContents_1(path)                                                                                         \
    _Generic(                                                                                                          \
        (path),                                                                                                        \
        Str *: dir_get_contents(((Str *)(path))->data, MisraScope),                                                    \
        const Str *: dir_get_contents(((const Str *)(path))->data, MisraScope),                                        \
        default: dir_get_contents((const char *)(path), MisraScope)                                                    \
    )
#define DirGetContents_2(path, alloc)                                                                                  \
    _Generic(                                                                                                          \
        (path),                                                                                                        \
        Str *: dir_get_contents(((Str *)(path))->data, ALLOCATOR_OF(alloc)),                                           \
        const Str *: dir_get_contents(((const Str *)(path))->data, ALLOCATOR_OF(alloc)),                               \
        default: dir_get_contents((const char *)(path), ALLOCATOR_OF(alloc))                                           \
    )

///
/// Get size of file without opening it.
///
/// path[in] : Path of file. Prefer `Str *`; `const char *` accepted.
///
/// SUCCESS : Non-negative value representing size of file in bytes.
/// FAILURE : Returns -1 if file cannot be accessed.
///
/// TAGS: System, File, Metadata
///
i64 file_get_size(const char *filename);
#define FileGetSize(path)                                                                                              \
    _Generic((path), Str *: file_get_size(((Str *)(path))->data), default: file_get_size((const char *)(path)))

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
/// path[in] : Path of the file to remove. Prefer `Str *`; `const
///            char *` accepted for literals / borrowed buffers.
///
/// SUCCESS : Returns 1; the directory entry is gone.
/// FAILURE : Returns 0; logs the failing syscall.
///
/// TAGS: System, File, FileSystem
///
i8 file_remove(const char *path);
#define FileRemove(path)                                                                                               \
    _Generic(                                                                                                          \
        (path),                                                                                                        \
        Str *: file_remove(((Str *)(path))->data),                                                                     \
        const Str *: file_remove(((const Str *)(path))->data),                                                         \
        default: file_remove((const char *)(path))                                                                     \
    )

///
/// Remove an empty directory. Direct syscall (`rmdir` on Linux
/// x86_64 / Darwin, `unlinkat(... AT_REMOVEDIR)` on Linux aarch64,
/// `RemoveDirectoryA` on Windows). The directory must be empty;
/// populated directories require recursive removal via `DirRemoveAll`.
///
/// path[in] : Path of the directory. Prefer `Str *`; `const char *`
///            accepted.
///
/// SUCCESS : Returns 1; the directory is gone.
/// FAILURE : Returns 0; logs the failing syscall.
///
/// TAGS: System, Directory, FileSystem
///
i8 dir_remove(const char *path);
#define DirRemove(path)                                                                                                \
    _Generic(                                                                                                          \
        (path),                                                                                                        \
        Str *: dir_remove(((Str *)(path))->data),                                                                      \
        const Str *: dir_remove(((const Str *)(path))->data),                                                          \
        default: dir_remove((const char *)(path))                                                                      \
    )

///
/// Create a single directory. Direct syscall (`mkdir` on Linux
/// x86_64 / Darwin, `mkdirat` on Linux aarch64, `CreateDirectoryA`
/// on Windows). Mode is 0755 on POSIX. Fails if a parent component
/// is missing -- use `DirCreateAll` for `mkdir -p` semantics.
///
/// path[in] : Path of the directory. Prefer `Str *`; `const char *`
///            accepted.
///
/// SUCCESS : Returns 1; the directory now exists.
/// FAILURE : Returns 0; logs the failing syscall.
///
/// TAGS: System, Directory, FileSystem
///
i8 dir_create(const char *path);
#define DirCreate(path)                                                                                                \
    _Generic(                                                                                                          \
        (path),                                                                                                        \
        Str *: dir_create(((Str *)(path))->data),                                                                      \
        const Str *: dir_create(((const Str *)(path))->data),                                                          \
        default: dir_create((const char *)(path))                                                                      \
    )

///
/// Recursive `mkdir -p`. Creates all missing path components.
/// Existing components are treated as success (EEXIST is not an
/// error). Trailing slashes are tolerated.
///
/// path[in] : Path of the directory tree. Prefer `Str *`; `const
///            char *` accepted.
///
/// SUCCESS : Returns 1; the full path now exists as a directory.
/// FAILURE : Returns 0 on first un-recoverable error.
///
/// TAGS: System, Directory, FileSystem
///
i8 dir_create_all(const char *path);
#define DirCreateAll(path)                                                                                             \
    _Generic(                                                                                                          \
        (path),                                                                                                        \
        Str *: dir_create_all(((Str *)(path))->data),                                                                  \
        const Str *: dir_create_all(((const Str *)(path))->data),                                                      \
        default: dir_create_all((const char *)(path))                                                                  \
    )

///
/// Recursive `rm -rf`. Removes a directory tree (regular files,
/// symlinks, subdirectories). If `path` doesn't exist this is a
/// no-op success. Follows the cross-platform `i8` return convention.
///
/// path[in] : Root of the tree. Prefer `Str *`; `const char *` accepted.
///
/// SUCCESS : Returns 1; the path is gone (or never existed).
/// FAILURE : Returns 0 on first un-recoverable error.
///
/// TAGS: System, Directory, FileSystem
///
i8 dir_remove_all(const char *path);
#define DirRemoveAll(path)                                                                                             \
    _Generic(                                                                                                          \
        (path),                                                                                                        \
        Str *: dir_remove_all(((Str *)(path))->data),                                                                  \
        const Str *: dir_remove_all(((const Str *)(path))->data),                                                      \
        default: dir_remove_all((const char *)(path))                                                                  \
    )

#endif // MISRA_SYS_DIR_H
