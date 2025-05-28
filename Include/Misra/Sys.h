/// file      : sys.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Portable system functions

#ifndef MISRA_SYS_H
#define MISRA_SYS_H

#include <Misra/Std/Container/Str.h>
#include <Misra/Types.h>
#include <errno.h>

#ifndef SYS_ERROR_STR_MAX_LENGTH
#    define SYS_ERROR_STR_MAX_LENGTH 128
#endif

typedef unsigned long   SysProcessId;
typedef struct SysMutex SysMutex;

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

///
/// Get environment value value in a `Str` object.
/// Object must be destroyed after use.
///
/// name[in]   : Name of environment variable.
/// value[out] : Value of environment variable.
///
/// SUCCESS : `Str` object containing value of environment variable.
/// FAILURE : Returns NULL if variable not found.
///
/// TAGS: System, Environment, Memory
///
Str *SysGetEnv(const char *name, Str *value);

///
/// Platform independent method to get current process Id.
///
/// SUCCESS : Returns current process ID.
/// FAILURE : Function cannot fail - always returns valid ID.
///
/// TAGS: System, Process
///
SysProcessId SysGetCurrentProcessId(void);

///
/// Create a platform-independent mutex object.
///
/// SUCCESS : Returns valid SysMutex object.
/// FAILURE : Returns NULL if mutex creation fails.
///
/// TAGS: System, Threading, Synchronization
///
SysMutex *SysMutexCreate(void);

///
/// Destroy the provided mutex object.
/// Once a mutex is destroyed, all resources held by it will be freed.
/// Using it after this cal is UB.
///
/// m[in] : Mutex object to be destroyed.
///
/// SUCCESS : Resources released.
/// FAILURE : Function cannot fail - safe to call with NULL.
///
/// TAGS: System, Threading, Memory
///
void SysMutexDestroy(SysMutex *m);

///
/// Acquire lock on provided mutex object.
///
/// m[in,out] : Mutex to lock.
///
/// SUCCESS : Returns locked mutex.
/// FAILURE : Returns NULL if locking fails.
///
/// TAGS: System, Threading, Synchronization
///
SysMutex *SysMutexLock(SysMutex *m);

///
/// Release lock on provided mutex object.
///
/// m[in,out] : Mutex to unlock.
///
/// SUCCESS : Returns unlocked mutex.
/// FAILURE : Returns NULL if unlocking fails.
///
/// TAGS: System, Threading, Synchronization
///
SysMutex *SysMutexUnlock(SysMutex *m);

///
/// Get last error using an error number.
///
/// eno[in]      : Unique error number descriptor.
/// err_str[out] : Error string will be stored in this.
///
/// SUCCESS : Error string describing last error.
/// FAILURE : Returns NULL if `err_str` is NULL.
///
/// TAGS: System, Error, String
///
Str *SysStrError(i32 eno, Str *err_str);

#endif // MISRA_SYS_H
