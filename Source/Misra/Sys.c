/// file      : sys.c
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// copyright : Copyright (c) 2025, Siddharth Mishra, All rights reserved.
///
/// Portable system functions

#include <Misra/Std.h>
#include <Misra/Std/Log.h>
#include <Misra/Sys.h>
#include <stdlib.h>
#ifdef _WIN32
#    include <windows.h>
#else
#    include <dirent.h>
#    include <pthread.h>
#    include <sys/stat.h>
#    include <unistd.h>
#endif

struct SysMutex {
#ifdef _WIN32
    CRITICAL_SECTION lock;
#else
    pthread_mutex_t lock;
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
        case SYS_DIR_ENTRY_TYPE_SOCKET :
            return "Socket";
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
        if (strcmp(findFileData.cFileName, ".") == 0 || strcmp(findFileData.cFileName, "..") == 0) {
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
        StrInitStack(&err, SYS_ERROR_STR_MAX_LENGTH, {
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
            SysDirEntry direntry = {0};
            switch (entry->d_type) {
                case DT_REG :
                    direntry.type = SYS_DIR_ENTRY_TYPE_REGULAR_FILE;
                    break;
                case DT_DIR :
                    direntry.type = SYS_DIR_ENTRY_TYPE_DIRECTORY;
                    break;
                case DT_FIFO :
                    direntry.type = SYS_DIR_ENTRY_TYPE_PIPE;
                    break;
                case DT_SOCK :
                    direntry.type = SYS_DIR_ENTRY_TYPE_SOCKET;
                    break;
                case DT_CHR :
                    direntry.type = SYS_DIR_ENTRY_TYPE_CHARACTER_DEVICE;
                    break;
                case DT_BLK :
                    direntry.type = SYS_DIR_ENTRY_TYPE_BLOCK_DEVICE;
                    break;
                case DT_LNK :
                    direntry.type = SYS_DIR_ENTRY_TYPE_SYMBOLIC_LINK;
                    break;
                case DT_UNKNOWN :
                default :
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
    if (!filename) {
        LOG_ERROR("invalid arguments.\n");
        return -1;
    }

#ifdef _WIN32
    // Windows-specific code using GetFileSizeEx
    HANDLE file = CreateFileA(filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    if (file == INVALID_HANDLE_VALUE) {
        Str err;
        StrInitStack(&err, SYS_ERROR_STR_MAX_LENGTH, {
            LOG_ERROR("failed to open file: %s\n", SysStrError(errno, &err)->data);
        });
        return -1;
    }

    LARGE_INTEGER file_size;
    if (!GetFileSizeEx(file, &file_size)) {
        Str err;
        StrInitStack(&err, SYS_ERROR_STR_MAX_LENGTH, {
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
        StrInitStack(&err, SYS_ERROR_STR_MAX_LENGTH, {
            LOG_ERROR("failed to get file size: %s\n", SysStrError(errno, &err)->data);
        });
        return -1;
    }
#endif
}

Str* SysGetEnv(const char* name, Str* value) {
    if (!name || !value) {
        return NULL;
    }
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

unsigned long SysGetCurrentProcessId() {
#ifdef _WIN32
    return (unsigned long)GetCurrentProcessId(); // Windows API
#else
    return (unsigned long)getpid(); // POSIX API (Linux/macOS)
#endif
}

SysMutex* SysMutexCreate() {
    SysMutex* m = NEW(SysMutex);
#ifdef _WIN32
    InitializeCriticalSection(&m->lock);
#else
    memset(&m->lock, 0, sizeof(m->lock));
#endif
    return m;
}

void SysMutexDestroy(SysMutex* m) {
    if (!m) {
        return;
    }
#ifdef _WIN32
    DeleteCriticalSection(&m->lock);
#else
    pthread_mutex_destroy(&m->lock);
#endif
    memset(m, 0, sizeof(SysMutex));
    FREE(m);
}

SysMutex* SysMutexLock(SysMutex* m) {
    if (!m) {
        return NULL;
    }
#ifdef _WIN32
    EnterCriticalSection(&m->lock);
#else
    pthread_mutex_lock(&m->lock);
#endif
    return m;
}

SysMutex* SysMutexUnlock(SysMutex* m) {
    if (!m) {
        return NULL;
    }
#ifdef _WIN32
    LeaveCriticalSection(&m->lock);
#else
    pthread_mutex_unlock(&m->lock);
#endif
    return m;
}

Str* SysStrError(i32 eno, Str* err_str) {
    if (!err_str) {
        LOG_ERROR("Invalid arguments");
    }

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
