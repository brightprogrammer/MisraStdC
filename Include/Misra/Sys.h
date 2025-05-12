/// file      : sys.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// copyright : Copyright (c) 2025, Siddharth Mishra, All rights reserved.
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

typedef enum {
    SYS_DIR_ENTRY_TYPE_UNKNOWN,
    SYS_DIR_ENTRY_TYPE_REGULAR_FILE,
    SYS_DIR_ENTRY_TYPE_DIRECTORY,
    SYS_DIR_ENTRY_TYPE_PIPE,
    SYS_DIR_ENTRY_TYPE_SOCKET,
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
/// RETURN : Null terminated string.
///
const char *SysDirEntryTypeToZStr(SysDirEntryType type);

typedef struct {
    SysDirEntryType type;
    Str             name;
} SysDirEntry;

SysDirEntry *SysDirEntryInitCopy(SysDirEntry *dst, SysDirEntry *src);
SysDirEntry *SysDirEntryDeinitCopy(SysDirEntry *copy);
typedef Vec(SysDirEntry) SysDirContents;

///
/// Read directory contents into a vector
/// Current contents of the vector will be cleared out.
///
/// path[in]    : Path of directory get content of.
///
/// SUCCESS : SysDirContents vector filled with directory contents data.
/// FAILURE : Empty vector.
///
SysDirContents SysGetDirContents(const char *path);

///
/// Get size of file without opening it.
///
/// filename[in] : Name/path of file.
///
/// SUCCESS : Non-negative value representing size of file in bytes.
/// FAILURE : -1
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
/// FAILURE : `NULL`
///
Str *SysGetEnv(const char *name, Str *value);

///
/// Platform independent method to get current process Id
///
SysProcessId SysGetCurrentProcessId();

///
/// Create a platform-independent mutex object.
///
/// SUCCESS : A valid SysMutex object
/// FAILURE : `NULL`
///
SysMutex *SysMutexCreate();

///
/// Destroy the provided mutex object.
/// Once a mutex is destroyed, all resources held by it will be freed.
/// Using it after this cal is UB.
///
/// m[in] : Mutex object to be destroyed.
///
void SysMutexDestroy(SysMutex *m);

///
/// Acquire lock on provided mutex object.
///
/// m[in,out] : Mutex to lock.
///
/// SUCCESS : `m`
/// FAILURE : `NULL`
///
SysMutex *SysMutexLock(SysMutex *m);

///
/// Release lock on provided mutex object.
///
/// m[in,out] : Mutex to unlock.
///
/// SUCCESS : `m`
/// FAILURE : `NULL`
///
SysMutex *SysMutexUnlock(SysMutex *m);

///
/// Get last error using an error number.
///
/// eno[in]      : Unique error number descriptor.
/// err_str[out] : Error string will be stored in this.
///
/// SUCCESS : Error string describing last error.
/// FAILURE : NULL only if `err_str` is NULL
///
Str *SysStrError(i32 eno, Str *err_str);

#endif // MISRA_SYS_H
