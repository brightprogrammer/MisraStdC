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

typedef unsigned long      SysProcessId;
typedef struct SysMutex    SysMutex;
typedef struct SysProcInfo SysProcInfo;

typedef enum SysDirEntryType {
    SYS_DIR_ENTRY_TYPE_UNKNOWN,
    SYS_DIR_ENTRY_TYPE_REGULAR_FILE,
    SYS_DIR_ENTRY_TYPE_DIRECTORY,
    SYS_DIR_ENTRY_TYPE_PIPE,
    SYS_DIR_ENTRY_TYPE_CHARACTER_DEVICE,
    SYS_DIR_ENTRY_TYPE_BLOCK_DEVICE,
    SYS_DIR_ENTRY_TYPE_SYMBOLIC_LINK
} SysDirEntryType;

// Process result status enumeration
typedef enum SysProcStatus {
    SYS_PROC_STATUS_RUNNING,    // Process is still running
    SYS_PROC_STATUS_COMPLETED,  // Process completed normally
    SYS_PROC_STATUS_TERMINATED, // Process was terminated/killed
    SYS_PROC_STATUS_ERROR       // Error occurred while checking status
} SysProcStatus;

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
/// Get the path to the current executable.
///
/// exe_path[out] : Str object to store the executable path.
///
/// SUCCESS : Returns initialized Str object with executable path.
/// FAILURE : Returns NULL if path cannot be determined.
///
/// TAGS: System, Process, Path
///
Str *SysGetCurrentExecutablePath(Str *exe_path);

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

///
/// Create a new process with specified arguments and environment.
///
/// executable[in] : Path to the executable to run.
/// argv[in]       : Vector of argument strings (first should be program name).
/// env[in]        : Vector of environment strings in "VAR=value" format, or NULL to inherit.
///
/// SUCCESS : Returns SysProcInfo object for managing the process.
/// FAILURE : Returns NULL if process creation fails.
///
/// TAGS: System, Process, Creation
///
SysProcInfo *SysCreateProcess(const char *executable, Strs *argv, Strs *env);

///
/// Wait for a process to complete with optional timeout.
///
/// proc_info[in] : Process information object.
/// timeout_ms[in]: Timeout in milliseconds, 0 for infinite wait.
///
/// SUCCESS : Returns process status.
/// FAILURE : Returns SYS_PROC_STATUS_ERROR on error.
///
/// TAGS: System, Process, Synchronization
///
SysProcStatus SysWaitForProcess(SysProcInfo *proc_info, u32 timeout_ms);

///
/// Get the exit code of a completed process.
///
/// proc_info[in]  : Process information object.
/// exit_code[out] : Exit code will be stored here.
///
/// SUCCESS : Returns true and stores exit code.
/// FAILURE : Returns false if process hasn't completed or error occurred.
///
/// TAGS: System, Process, Status
///
bool SysGetProcessExitCode(SysProcInfo *proc_info, i32 *exit_code);

///
/// Terminate a running process forcefully.
///
/// proc_info[in] : Process information object.
///
/// SUCCESS : Returns true if process was terminated.
/// FAILURE : Returns false if termination failed.
///
/// TAGS: System, Process, Control
///
bool SysTerminateProcess(SysProcInfo *proc_info);

///
/// Clean up process information and free resources.
/// Process must be completed or terminated before calling this.
///
/// proc_info[in] : Process information object to clean up.
///
/// SUCCESS : Resources freed.
/// FAILURE : Function cannot fail - safe to call with NULL.
///
/// TAGS: System, Process, Memory
///
void SysDestroyProcess(SysProcInfo *proc_info);

///
/// Function pointer type for SysAbort callback.
/// This allows custom handling of abort situations (e.g., for testing).
///
typedef void (*SysAbortCallback)(void);

///
/// Set a custom callback function for SysAbort.
/// If no callback is set, SysAbort will call the standard abort() function.
///
/// callback[in] : Function to call when SysAbort is invoked, or NULL to reset to default.
///
/// SUCCESS : Callback is set.
/// FAILURE : Function cannot fail.
///
/// TAGS: System, Testing, Callback
///
void SysSetAbortCallback(SysAbortCallback callback);

///
/// Custom abort function that can be redirected for testing purposes.
/// By default, this calls the standard abort() function.
/// If a callback is set via SysSetAbortCallback, it calls the callback instead.
///
/// SUCCESS : Function does not return (either aborts or calls callback).
/// FAILURE : Function cannot fail.
///
/// TAGS: System, Testing, Control
///
void SysAbort(void);

#endif // MISRA_SYS_H

