#ifndef MISRA_SYS_DIR_H
#define MISRA_SYS_DIR_H

#include <Misra/Std/Container/Str.h>

typedef enum SysDirEntryType {
    SYS_DIR_ENTRY_TYPE_UNKNOWN,
    SYS_DIR_ENTRY_TYPE_REGULAR_FILE,
    SYS_DIR_ENTRY_TYPE_DIRECTORY,
    SYS_DIR_ENTRY_TYPE_PIPE,
    SYS_DIR_ENTRY_TYPE_CHARACTER_DEVICE,
    SYS_DIR_ENTRY_TYPE_BLOCK_DEVICE,
    SYS_DIR_ENTRY_TYPE_SYMBOLIC_LINK
} SysDirEntryType;

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
const char *SysDirEntryTypeToZStr(SysDirEntryType type);

///
/// Directory entry structure containing type and name.
///
/// TAGS: System, Directory, Structure
///
typedef struct SysDirEntry {
    SysDirEntryType type;
    Str             name;
} SysDirEntry;

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
SysDirEntry *SysDirEntryInitCopy(SysDirEntry *dst, SysDirEntry *src);

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
SysDirEntry *SysDirEntryDeinitCopy(SysDirEntry *copy);

///
/// Vector type for directory contents.
///
/// TAGS: System, Directory, Container
///
typedef Vec(SysDirEntry) SysDirContents;

///
/// Read directory contents into a vector.
/// Current contents of the vector will be cleared out.
///
/// path[in] : Path of directory get content of.
///
/// SUCCESS : SysDirContents vector filled with directory contents data.
/// FAILURE : Returns empty vector on read error.
///
/// TAGS: System, FileSystem, Directory
///
SysDirContents SysGetDirContents(const char *path);

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
i64 SysGetFileSize(const char *filename);

#endif // MISRA_SYS_DIR_H
