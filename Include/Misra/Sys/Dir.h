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
/// dst[out] : Destination entry to initialize.
/// src[in]  : Source entry to copy from.
///
/// SUCCESS : Returns initialized destination entry.
/// FAILURE : Returns NULL if allocation fails.
///
/// TAGS: Memory, Management, System
///
DirEntry *DirEntryInitCopy(DirEntry *dst, DirEntry *src);

///
/// Deinitialize a copied directory entry.
///
/// copy[in,out] : Entry copy to clean up.
///
/// SUCCESS : Returns NULL after cleaning resources.
/// FAILURE : Function cannot fail - safe to call with NULL.
///
/// TAGS: Memory, Management, System
///
DirEntry *DirEntryDeinitCopy(DirEntry *copy);

///
/// Vector type for directory contents.
///
/// TAGS: System, Directory, Container
///
typedef Vec(DirEntry) DirContents;

///
/// Read directory contents into a vector.
/// Current contents of the vector will be cleared out.
///
/// path[in]   : Path of directory get content of.
/// alloc[in]  : Allocator used to back the returned vector and entry names.
///
/// SUCCESS : DirContents vector filled with directory contents data.
/// FAILURE : Returns empty vector on read error.
///
/// TAGS: System, FileSystem, Directory
///
DirContents dir_get_contents(const char *path, Allocator *alloc);
#define DirGetContents(...)         MISRA_OVERLOAD(DirGetContents, __VA_ARGS__)
#define DirGetContents_1(path)       dir_get_contents((path), MisraScope)
#define DirGetContents_2(path, alloc) dir_get_contents((path), ALLOCATOR_OF(alloc))

///
/// Get size of file without opening it.
///
/// filename[in] : Name/path of file.
///
/// SUCCESS : Non-negative value representing size of file in bytes.
/// FAILURE : Returns -1 if file cannot be accessed.
///
/// TAGS: System, File, Metadata
///
i64 FileGetSize(const char *filename);

///
/// Remove a regular file. Equivalent of POSIX `unlink(2)` / Win32
/// `DeleteFileA`. Symlinks are removed, not followed.
///
/// Return type is `i8` (0/1) rather than `bool` to dodge the
/// `bool`-typedef redefinition trap Misra's Types.h documents:
/// system headers transitively `#define bool _Bool` and the SAME
/// identifier ends up meaning different types across TUs. Callers
/// can still use the result as a boolean (`if (FileRemove(p)) ...`).
///
/// path[in] : Path of the file to remove.
///
/// SUCCESS : Returns 1; the directory entry is gone.
/// FAILURE : Returns 0; logs the failing syscall. Common causes:
///           file doesn't exist, no write permission on the parent
///           directory, path is a directory (use `DirRemove`).
///
/// TAGS: System, File, FileSystem
///
i8 FileRemove(const char *path);

///
/// Remove an empty directory. Equivalent of POSIX `rmdir(2)` / Win32
/// `RemoveDirectoryA`. The directory must be empty; populated
/// directories require recursive removal (callers can walk
/// `DirGetContents` and remove entries one by one).
///
/// Return-type rationale matches `FileRemove`: `i8` to sidestep the
/// `bool`/`_Bool` cross-TU typedef hazard.
///
/// path[in] : Path of the directory to remove.
///
/// SUCCESS : Returns 1; the directory is gone.
/// FAILURE : Returns 0; logs the failing syscall. Common causes:
///           directory doesn't exist, directory is non-empty (ENOTEMPTY),
///           no write permission on the parent directory.
///
/// TAGS: System, Directory, FileSystem
///
i8 DirRemove(const char *path);

#endif // MISRA_SYS_DIR_H
