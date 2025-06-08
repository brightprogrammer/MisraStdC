/// file      : sys.c
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Portable system functions

// required for strerror_r
// Reference : https://forums.freebsd.org/threads/strerror_r-best-practices-posix-vs-gnu.92296/
#define _POSIX_C_SOURCE 200112L


#ifdef _WIN32
#    include <windows.h>
#    include <tlhelp32.h>
#    include <psapi.h>
#    include <signal.h>
#else
#    include <dirent.h>
#    include <pthread.h>
#    include <sys/stat.h>
#    include <sys/wait.h>
#    include <signal.h>
#    include <unistd.h>
#    ifdef __APPLE__
#        include <mach-o/dyld.h>
#    endif
#endif

#include <Misra/Std.h>
#include <Misra/Std/Log.h>
#include <Misra/Sys.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

struct SysMutex {
#ifdef _WIN32
    CRITICAL_SECTION lock;
#else
    pthread_mutex_t lock;
#endif
};

struct SysProcInfo {
#ifdef _WIN32
    PROCESS_INFORMATION pi;
    DWORD               exit_code;
    bool                completed;
#else
    pid_t pid;
    int   exit_code;
    bool  completed;
#endif
};

const char* SysDirEntryTypeToZstr(SysDirEntryType type) {
    switch (type) {
        case SYS_DIR_ENTRY_TYPE_UNKNOWN :
            return "Unknown";
        case SYS_DIR_ENTRY_TYPE_REGULAR_FILE :
            return "Regular File";
        case SYS_DIR_ENTRY_TYPE_DIRECTORY :
            return "Directory";
        case SYS_DIR_ENTRY_TYPE_PIPE :
            return "Pipe";
        case SYS_DIR_ENTRY_TYPE_CHARACTER_DEVICE :
            return "Character Device";
        case SYS_DIR_ENTRY_TYPE_BLOCK_DEVICE :
            return "Block Device";
        case SYS_DIR_ENTRY_TYPE_SYMBOLIC_LINK :
            return "Symbolic Link";
        default :
            return "Invalid Type";
    }
}


SysDirEntry* SysDirEntryInitCopy(SysDirEntry* dst, SysDirEntry* src) {
    if (!dst || !src) {
        LOG_ERROR("invalid arguments.");
        return NULL;
    }

    dst->type = src->type;
    StrInitCopy(&dst->name, &src->name);

    return dst;
}


SysDirEntry* SysDirEntryDeinitCopy(SysDirEntry* copy) {
    if (!copy) {
        LOG_ERROR("invalid arguments.");
        return NULL;
    }

    StrDeinit(&copy->name);
    copy->type = 0;

    return copy;
}

#ifdef _WIN32
// Windows-specific implementation using FindFirstFile/FindNextFile
SysDirContents SysGetDirContents(const char* path) {
    if (!path) {
        return (SysDirContents) {0};
    }

    SysDirContents dc = VecInit();

    // Construct the search path with a wildcard
    char search_path[MAX_PATH];
    snprintf(search_path, sizeof(search_path), "%s\\*", path);

    WIN32_FIND_DATA findFileData;
    HANDLE          hFind = FindFirstFile(search_path, &findFileData);

    if (hFind == INVALID_HANDLE_VALUE) {
        return (SysDirContents) {0};
    }

    do {
        // Skip "." and ".." entries
        if (ZstrCompare(findFileData.cFileName, ".") == 0 || ZstrCompare(findFileData.cFileName, "..") == 0) {
            continue;
        }

        SysDirEntry direntry = {0};
        // Determine file type based on attributes
        if (findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            direntry.type = SYS_DIR_ENTRY_TYPE_DIRECTORY;
        } else if (findFileData.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) {
            direntry.type = SYS_DIR_ENTRY_TYPE_SYMBOLIC_LINK;
        } else if (findFileData.dwFileAttributes & FILE_ATTRIBUTE_NORMAL) {
            direntry.type = SYS_DIR_ENTRY_TYPE_REGULAR_FILE;
        } else {
            direntry.type = SYS_DIR_ENTRY_TYPE_UNKNOWN;
        }

        direntry.name = StrInitFromZstr(findFileData.cFileName); // Copy file name
        VecPushBack(&dc, direntry);
    } while (FindNextFile(hFind, &findFileData) != 0);

    FindClose(hFind);

    return dc;
}
#else
// APPLE or Unix based system implementation using opendir/readdir
SysDirContents SysGetDirContents(const char* path) {
    if (!path) {
        LOG_ERROR("invalid arguments.");
        return (SysDirContents) {0};
    }

    SysDirContents dc = VecInit();

    DIR* dir = opendir(path);
    if (NULL == dir) {
        Str err;
        StrInitStack(err, SYS_ERROR_STR_MAX_LENGTH, {
            LOG_ERROR("opendir(\"%s\") failed : %s.", path, SysStrError(errno, &err)->data);
        });
        return (SysDirContents) {0};
    }

    // Get value at specific index in name of directory entry
#    define DNAME_AT(idx) entry->d_name[idx]

    // Get length of name of directory entry
#    if __APPLE__
#        define NAMELEN(entry) (entry)->d_namlen
#    elif __linux__
#        define NAMELEN(entry) (entry)->d_reclen
#    endif

    // Go through each directory entry
    struct dirent* entry = NULL;
    while (NULL != (entry = readdir(dir))) {
        if ('.' == DNAME_AT(0) && 0 == DNAME_AT(1)) {
            continue;
        } else if ('.' == DNAME_AT(0) && '.' == DNAME_AT(1) && 0 == DNAME_AT(2)) {
            continue;
        } else {
            Str entry_path = StrInit();
            StrWriteFmt(&entry_path, "{}/{}", FMT(path), FMT(entry->d_name));

            struct stat path_stat;
            stat(entry_path.data, &path_stat);

            StrDeinit(&entry_path);

            SysDirEntry direntry = {0};
            if (S_ISREG(path_stat.st_mode)) {
                direntry.type = SYS_DIR_ENTRY_TYPE_REGULAR_FILE;
            } else if (S_ISDIR(path_stat.st_mode)) {
                direntry.type = SYS_DIR_ENTRY_TYPE_DIRECTORY;
            } else if (S_ISFIFO(path_stat.st_mode)) {
                direntry.type = SYS_DIR_ENTRY_TYPE_PIPE;
            } else if (S_ISCHR(path_stat.st_mode)) {
                direntry.type = SYS_DIR_ENTRY_TYPE_CHARACTER_DEVICE;
            } else if (S_ISBLK(path_stat.st_mode)) {
                direntry.type = SYS_DIR_ENTRY_TYPE_BLOCK_DEVICE;
            } else if (S_ISLNK(path_stat.st_mode)) {
                direntry.type = SYS_DIR_ENTRY_TYPE_SYMBOLIC_LINK;
            } else {
                direntry.type = SYS_DIR_ENTRY_TYPE_UNKNOWN;
            }
            direntry.name = StrInitFromCstr(entry->d_name, NAMELEN(entry));
            VecPushBack(&dc, direntry);
        }
    }

#    undef DNAME_AT
#    undef NAMELEN

    closedir(dir);

    return dc;
}
#endif

// Cross-platform function to get file size
i64 SysGetFileSize(const char* filename) {
#ifdef _WIN32
    // Windows-specific code using GetFileSizeEx
    HANDLE file = CreateFileA(filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    if (file == INVALID_HANDLE_VALUE) {
        Str err;
        StrInitStack(err, SYS_ERROR_STR_MAX_LENGTH, {
            LOG_ERROR("failed to open file: %s\n", SysStrError(errno, &err)->data);
        });
        return -1;
    }

    LARGE_INTEGER file_size;
    if (!GetFileSizeEx(file, &file_size)) {
        Str err;
        StrInitStack(err, SYS_ERROR_STR_MAX_LENGTH, {
            LOG_ERROR("failed to get file size: %s\n", SysStrError(errno, &err)->data);
        });
        CloseHandle(file);
        return -1;
    }

    CloseHandle(file);
    return (i64)file_size.QuadPart;
#else
    // Unix-like systems (Linux/macOS) code using stat
    struct stat file_stat;
    if (stat(filename, &file_stat) == 0) {
        return (i64)file_stat.st_size;
    } else {
        Str err;
        StrInitStack(err, SYS_ERROR_STR_MAX_LENGTH, {
            LOG_ERROR("failed to get file size: %s\n", SysStrError(errno, &err)->data);
        });
        return -1;
    }
#endif
}

Str* SysGetEnv(const char* name, Str* value) {
#ifdef _WIN32
    char*  env_var;
    size_t requiredSize;

    getenv_s(&requiredSize, NULL, 0, name);
    if (requiredSize == 0) {
        return NULL;
    }

    env_var = (char*)malloc(requiredSize);
    if (!env_var) {
        return NULL;
    }

    // Get the value of the LIB environment variable.
    getenv_s(&requiredSize, env_var, requiredSize, name);

    *value          = StrInit();
    value->data     = env_var;
    value->length   = requiredSize;
    value->capacity = requiredSize;
    return value;
#else
    char* env_var = getenv(name);
    if (env_var) {
        *value = StrInitFromZstr(env_var);
        return value;
    }
    return NULL;
#endif
}

unsigned long SysGetCurrentProcessId(void) {
#ifdef _WIN32
    return (unsigned long)GetCurrentProcessId(); // Windows API
#else
    return (unsigned long)getpid(); // POSIX API (Linux/macOS)
#endif
}

Str* SysGetCurrentExecutablePath(Str* exe_path) {
    if (!exe_path) {
        LOG_ERROR("Invalid arguments: exe_path is NULL");
        return NULL;
    }

#ifdef _WIN32
    char  buffer[MAX_PATH];
    DWORD len = GetModuleFileNameA(NULL, buffer, MAX_PATH);
    if (len == 0 || len >= MAX_PATH) {
        LOG_ERROR("Failed to get executable path or buffer too small");
        return NULL;
    }
    *exe_path = StrInitFromZstr(buffer);
    return exe_path;
#else
    char buffer[4096]; // Large buffer for Unix paths

    // Try /proc/self/exe first (Linux)
    ssize_t len = readlink("/proc/self/exe", buffer, sizeof(buffer) - 1);
    if (len != -1) {
        buffer[len] = '\0';
        *exe_path   = StrInitFromZstr(buffer);
        return exe_path;
    }

// Fallback for macOS and other Unix systems
#    ifdef __APPLE__
    // macOS specific method
    u32 size = sizeof(buffer);
    if (_NSGetExecutablePath(buffer, &size) == 0) {
        *exe_path = StrInitFromZstr(buffer);
        return exe_path;
    }
#    endif

    // Last resort fallback
    LOG_ERROR("Could not determine executable path, using fallback");
    *exe_path = StrInitFromZstr("unknown_executable");
    return exe_path;
#endif
}

SysMutex* SysMutexCreate(void) {
    SysMutex* m = NEW(SysMutex);
#ifdef _WIN32
    InitializeCriticalSection(&m->lock);
#else
    memset(&m->lock, 0, sizeof(m->lock));
#endif
    return m;
}

void SysMutexDestroy(SysMutex* m) {
#ifdef _WIN32
    DeleteCriticalSection(&m->lock);
#else
    pthread_mutex_destroy(&m->lock);
#endif
    memset(m, 0, sizeof(SysMutex));
    FREE(m);
}

SysMutex* SysMutexLock(SysMutex* m) {
#ifdef _WIN32
    EnterCriticalSection(&m->lock);
#else
    pthread_mutex_lock(&m->lock);
#endif
    return m;
}

SysMutex* SysMutexUnlock(SysMutex* m) {
#ifdef _WIN32
    LeaveCriticalSection(&m->lock);
#else
    pthread_mutex_unlock(&m->lock);
#endif
    return m;
}

Str* SysStrError(i32 eno, Str* err_str) {
    err_str->length = err_str->capacity = 128; // I hope it's enough on all platforms
    err_str->data                       = (char*)calloc(err_str->length, 1);
#if _WIN32
    strerror_s(err_str->data, err_str->length, eno);
#else
    strerror_r(eno, err_str->data, err_str->length);
#endif

    if (!strlen(err_str->data)) {
        return NULL;
    }

    return err_str;
}

// Helper function to build command line string from argv (Windows)
#ifdef _WIN32
static char* build_command_line(Strs* argv) {
    if (!argv || argv->length == 0) {
        return NULL;
    }

    // Calculate total length needed
    size total_len = 0;
    for (size i = 0; i < argv->length; i++) {
        Str* arg   = &VecAt(argv, i);
        total_len += arg->length;
        if (i > 0)
            total_len += 1; // Space separator

        // Add quotes if argument contains spaces
        for (size j = 0; j < arg->length; j++) {
            if (arg->data[j] == ' ') {
                total_len += 2; // Opening and closing quotes
                break;
            }
        }
    }
    total_len += 1; // Null terminator

    char* cmd_line = (char*)malloc(total_len);
    if (!cmd_line)
        return NULL;

    cmd_line[0] = '\0';

    for (size i = 0; i < argv->length; i++) {
        Str* arg = &VecAt(argv, i);

        if (i > 0) {
            strcat(cmd_line, " ");
        }

        // Check if argument needs quotes
        bool needs_quotes = false;
        for (size j = 0; j < arg->length; j++) {
            if (arg->data[j] == ' ') {
                needs_quotes = true;
                break;
            }
        }

        if (needs_quotes) {
            strcat(cmd_line, "\"");
            strncat(cmd_line, arg->data, arg->length);
            strcat(cmd_line, "\"");
        } else {
            strncat(cmd_line, arg->data, arg->length);
        }
    }

    return cmd_line;
}

// Helper function to build environment block (Windows)
static char* build_environment_block(Strs* env) {
    if (!env || env->length == 0) {
        return NULL; // Inherit parent environment
    }

    // Calculate total length needed
    size total_len = 0;
    for (size i = 0; i < env->length; i++) {
        Str* env_var  = &VecAt(env, i);
        total_len    += env_var->length + 1; // +1 for null terminator
    }
    total_len += 1;                          // Final null terminator

    char* env_block = (char*)malloc(total_len);
    if (!env_block)
        return NULL;

    char* ptr = env_block;
    for (size i = 0; i < env->length; i++) {
        Str* env_var = &VecAt(env, i);
        memcpy(ptr, env_var->data, env_var->length);
        ptr    += env_var->length;
        *ptr++  = '\0';
    }
    *ptr = '\0'; // Final null terminator

    return env_block;
}
#endif

// Helper function to build argv array (Unix)
#ifndef _WIN32
static char** build_argv_array(Strs* argv) {
    if (!argv || argv->length == 0) {
        return NULL;
    }

    char** arg_array = (char**)malloc((argv->length + 1) * sizeof(char*));
    if (!arg_array)
        return NULL;

    for (size i = 0; i < argv->length; i++) {
        Str* arg     = &VecAt(argv, i);
        arg_array[i] = (char*)malloc(arg->length + 1);
        if (!arg_array[i]) {
            // Clean up on failure
            for (size j = 0; j < i; j++) {
                free(arg_array[j]);
            }
            free(arg_array);
            return NULL;
        }
        memcpy(arg_array[i], arg->data, arg->length);
        arg_array[i][arg->length] = '\0';
    }
    arg_array[argv->length] = NULL; // Null terminator

    return arg_array;
}

// Helper function to build envp array (Unix)
static char** build_envp_array(Strs* env) {
    if (!env || env->length == 0) {
        return NULL; // Use inherited environment
    }

    char** env_array = (char**)malloc((env->length + 1) * sizeof(char*));
    if (!env_array)
        return NULL;

    for (size i = 0; i < env->length; i++) {
        Str* env_var = &VecAt(env, i);
        env_array[i] = (char*)malloc(env_var->length + 1);
        if (!env_array[i]) {
            // Clean up on failure
            for (size j = 0; j < i; j++) {
                free(env_array[j]);
            }
            free(env_array);
            return NULL;
        }
        memcpy(env_array[i], env_var->data, env_var->length);
        env_array[i][env_var->length] = '\0';
    }
    env_array[env->length] = NULL; // Null terminator

    return env_array;
}
#endif

SysProcInfo* SysCreateProcess(const char* executable, Strs* argv, Strs* env) {
    if (!executable) {
        LOG_ERROR("Invalid executable path");
        return NULL;
    }

    SysProcInfo* proc_info = NEW(SysProcInfo);
    if (!proc_info) {
        LOG_ERROR("Failed to allocate process info");
        return NULL;
    }

    proc_info->completed = false;
    proc_info->exit_code = 0;

#ifdef _WIN32
    // Windows implementation using CreateProcess
    char* cmd_line  = build_command_line(argv);
    char* env_block = build_environment_block(env);

    STARTUPINFOA si = {0};
    si.cb           = sizeof(si);

    BOOL success = CreateProcessA(
        executable,    // Application name
        cmd_line,      // Command line
        NULL,          // Process security attributes
        NULL,          // Thread security attributes
        FALSE,         // Don't inherit handles
        0,             // Creation flags
        env_block,     // Environment block
        NULL,          // Use parent's current directory
        &si,           // Startup info
        &proc_info->pi // Process information
    );

    // Clean up temporary strings
    if (cmd_line)
        free(cmd_line);
    if (env_block)
        free(env_block);

    if (!success) {
        DWORD error = GetLastError();
        LOG_ERROR("CreateProcess failed with error %lu", error);
        FREE(proc_info);
        return NULL;
    }

#else
    // Unix implementation using fork/exec
    char** argv_array = build_argv_array(argv);
    char** envp_array = build_envp_array(env);

    proc_info->pid = fork();

    if (proc_info->pid == -1) {
        // Fork failed
        perror("fork failed");
        if (argv_array) {
            for (int i = 0; argv_array[i]; i++)
                free(argv_array[i]);
            free(argv_array);
        }
        if (envp_array) {
            for (int i = 0; envp_array[i]; i++)
                free(envp_array[i]);
            free(envp_array);
        }
        FREE(proc_info);
        return NULL;
    }

    if (proc_info->pid == 0) {
        // Child process
        if (envp_array) {
            execve(executable, argv_array, envp_array);
        } else {
            execv(executable, argv_array);
        }

        // If we get here, exec failed
        perror("exec failed");
        _exit(1);
    }

    // Parent process - clean up argv and envp arrays
    if (argv_array) {
        for (int i = 0; argv_array[i]; i++)
            free(argv_array[i]);
        free(argv_array);
    }
    if (envp_array) {
        for (int i = 0; envp_array[i]; i++)
            free(envp_array[i]);
        free(envp_array);
    }
#endif

    return proc_info;
}

SysProcStatus SysWaitForProcess(SysProcInfo* proc_info, u32 timeout_ms) {
    if (!proc_info) {
        return SYS_PROC_STATUS_ERROR;
    }

#ifdef _WIN32
    DWORD wait_time = (timeout_ms == 0) ? INFINITE : timeout_ms;
    DWORD result    = WaitForSingleObject(proc_info->pi.hProcess, wait_time);

    switch (result) {
        case WAIT_OBJECT_0 :
            proc_info->completed = true;
            if (GetExitCodeProcess(proc_info->pi.hProcess, &proc_info->exit_code)) {
                return SYS_PROC_STATUS_COMPLETED;
            } else {
                return SYS_PROC_STATUS_ERROR;
            }
        case WAIT_TIMEOUT :
            return SYS_PROC_STATUS_RUNNING;
        default :
            return SYS_PROC_STATUS_ERROR;
    }

#else
    int   status;
    pid_t result;

    if (timeout_ms == 0) {
        // Infinite wait
        result = waitpid(proc_info->pid, &status, 0);
    } else {
        // Non-blocking wait with timeout simulation
        // Note: This is a simplified implementation
        result = waitpid(proc_info->pid, &status, WNOHANG);
        if (result == 0) {
            return SYS_PROC_STATUS_RUNNING;
        }
    }

    if (result == -1) {
        return SYS_PROC_STATUS_ERROR;
    }

    if (result == proc_info->pid) {
        proc_info->completed = true;
        if (WIFEXITED(status)) {
            proc_info->exit_code = WEXITSTATUS(status);
            return SYS_PROC_STATUS_COMPLETED;
        } else if (WIFSIGNALED(status)) {
            proc_info->exit_code = 128 + WTERMSIG(status);
            return SYS_PROC_STATUS_TERMINATED;
        }
    }

    return SYS_PROC_STATUS_ERROR;
#endif
}

bool SysGetProcessExitCode(SysProcInfo* proc_info, i32* exit_code) {
    if (!proc_info || !exit_code) {
        return false;
    }

    if (!proc_info->completed) {
        return false;
    }

    *exit_code = (i32)proc_info->exit_code;
    return true;
}

bool SysTerminateProcess(SysProcInfo* proc_info) {
    if (!proc_info) {
        return false;
    }

#ifdef _WIN32
    if (TerminateProcess(proc_info->pi.hProcess, 1)) {
        proc_info->completed = true;
        proc_info->exit_code = 1;
        return true;
    }
    return false;

#else
    if (kill(proc_info->pid, SIGTERM) == 0) {
        proc_info->completed = true;
        proc_info->exit_code = 128 + SIGTERM;
        return true;
    }
    return false;
#endif
}

void SysDestroyProcess(SysProcInfo* proc_info) {
    if (!proc_info) {
        return;
    }

    // TODO: Terminate the process if it's still running

#ifdef _WIN32
    CloseHandle(proc_info->pi.hProcess);
    CloseHandle(proc_info->pi.hThread);
#endif

    FREE(proc_info);
}

// Global callback for SysAbort - NULL means use default abort()
static SysAbortCallback g_abort_callback = NULL;

void SysSetAbortCallback(SysAbortCallback callback) {
    g_abort_callback = callback;
}

void SysAbort(void) {
    if (g_abort_callback) {
        g_abort_callback();
    } else {
        abort();
    }
}
