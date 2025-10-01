#include <Misra/Sys/Dir.h>
#include <Misra/Std/Log.h>

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


const char *SysDirEntryTypeToZstr(SysDirEntryType type) {
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


SysDirEntry *SysDirEntryInitCopy(SysDirEntry *dst, SysDirEntry *src) {
    if (!dst || !src) {
        LOG_FATAL("invalid arguments.");
    }

    dst->type = src->type;
    StrInitCopy(&dst->name, &src->name);

    return dst;
}


SysDirEntry *SysDirEntryDeinitCopy(SysDirEntry *copy) {
    if (!copy) {
        LOG_FATAL("invalid arguments.");
    }

    StrDeinit(&copy->name);
    copy->type = 0;

    return copy;
}

#ifdef _WIN32
// Windows-specific implementation using FindFirstFile/FindNextFile
SysDirContents SysGetDirContents(const char *path) {
    if (!path) {
        LOG_FATAL("Invalid argument");
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
SysDirContents SysGetDirContents(const char *path) {
    if (!path) {
        LOG_FATAL("invalid arguments.");
    }

    SysDirContents dc = VecInit();

    DIR *dir = opendir(path);
    if (NULL == dir) {
        LOG_SYS_ERROR("opendir(\"{}\") failed", path);
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
    struct dirent *entry = NULL;
    while (NULL != (entry = readdir(dir))) {
        if ('.' == DNAME_AT(0) && 0 == DNAME_AT(1)) {
            continue;
        } else if ('.' == DNAME_AT(0) && '.' == DNAME_AT(1) && 0 == DNAME_AT(2)) {
            continue;
        } else {
            Str         entry_path = StrInit();
            const char *dir_name   = &entry->d_name[0];
            StrWriteFmt(&entry_path, "{}/{}", path, dir_name);

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
i64 SysGetFileSize(const char *filename) {
#ifdef _WIN32
    // Windows-specific code using GetFileSizeEx
    HANDLE file = CreateFileA(filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    if (file == INVALID_HANDLE_VALUE) {
        LOG_SYS_ERROR("CreateFileA() failed");
        return -1;
    }

    LARGE_INTEGER file_size;
    if (!GetFileSizeEx(file, &file_size)) {
        LOG_SYS_ERROR("GetFileSizeEx() failed");
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
        LOG_SYS_ERROR("stat() failed");
        return -1;
    }
#endif
}
