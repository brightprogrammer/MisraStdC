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

#include "../_Syscall.h"

#include <stdint.h>


const char *DirEntryTypeToZstr(DirEntryType type) {
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


DirEntry *DirEntryInitCopy(DirEntry *dst, DirEntry *src) {
    if (!dst || !src) {
        LOG_FATAL("invalid arguments.");
    }

    dst->type = src->type;
    StrInitCopy(&dst->name, &src->name);

    return dst;
}


DirEntry *DirEntryDeinitCopy(DirEntry *copy) {
    if (!copy) {
        LOG_FATAL("invalid arguments.");
    }

    StrDeinit(&copy->name);
    copy->type = 0;

    return copy;
}

#ifdef _WIN32
// Windows-specific implementation using FindFirstFile/FindNextFile
DirContents DirGetContents(const char *path, Allocator *alloc) {
    if (!path || !alloc) {
        LOG_FATAL("Invalid argument");
    }

    DirContents dc = (DirContents)VecInit(alloc);

    // Construct the search path with a wildcard
    char search_path[MAX_PATH];
    snprintf(search_path, sizeof(search_path), "%s\\*", path);

    WIN32_FIND_DATA findFileData;
    HANDLE          hFind = FindFirstFile(search_path, &findFileData);

    if (hFind == INVALID_HANDLE_VALUE) {
        return dc;
    }

    do {
        // Skip "." and ".." entries
        if (ZstrCompare(findFileData.cFileName, ".") == 0 || ZstrCompare(findFileData.cFileName, "..") == 0) {
            continue;
        }

        DirEntry direntry = {0};
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

        direntry.name = StrInitFromCstr(findFileData.cFileName, ZstrLen(findFileData.cFileName), alloc);
        VecPushBack(&dc, direntry);
    } while (FindNextFile(hFind, &findFileData) != 0);

    FindClose(hFind);

    return dc;
}
#elif FEATURE_DIRECT_SYSCALL
// Linux: open + getdents64 syscalls, parse `struct linux_dirent64`
// records out of the kernel's buffer ourselves. dirent.d_type tells
// us the entry kind (regular file / dir / symlink / ...), so we
// don't need a separate stat() call per entry. DT_UNKNOWN entries
// stay as SYS_DIR_ENTRY_TYPE_UNKNOWN (some filesystems don't fill
// d_type).

// File-type bits from POSIX dirent.h (also the values the kernel
// returns in d_type via getdents64).
#    define DIRENT_TYPE_UNKNOWN 0
#    define DIRENT_TYPE_FIFO    1
#    define DIRENT_TYPE_CHR     2
#    define DIRENT_TYPE_DIR     4
#    define DIRENT_TYPE_BLK     6
#    define DIRENT_TYPE_REG     8
#    define DIRENT_TYPE_LNK     10

// Layout the kernel writes into the getdents64 buffer. The trailing
// d_name is null-terminated; the next record starts d_reclen bytes
// from the start of this one.
struct misra_linux_dirent64 {
    u64  d_ino;
    i64  d_off;
    u16  d_reclen;
    u8   d_type;
    char d_name[]; // flexible
};

// Map kernel/dirent d_type values to our enum.
static DirEntryType dirent_type_to_misra(u8 dt) {
    switch (dt) {
        case DIRENT_TYPE_REG :
            return SYS_DIR_ENTRY_TYPE_REGULAR_FILE;
        case DIRENT_TYPE_DIR :
            return SYS_DIR_ENTRY_TYPE_DIRECTORY;
        case DIRENT_TYPE_FIFO :
            return SYS_DIR_ENTRY_TYPE_PIPE;
        case DIRENT_TYPE_CHR :
            return SYS_DIR_ENTRY_TYPE_CHARACTER_DEVICE;
        case DIRENT_TYPE_BLK :
            return SYS_DIR_ENTRY_TYPE_BLOCK_DEVICE;
        case DIRENT_TYPE_LNK :
            return SYS_DIR_ENTRY_TYPE_SYMBOLIC_LINK;
        default :
            return SYS_DIR_ENTRY_TYPE_UNKNOWN;
    }
}

DirContents DirGetContents(const char *path, Allocator *alloc) {
    if (!path || !alloc) {
        LOG_FATAL("invalid arguments.");
    }

    DirContents dc = (DirContents)VecInit(alloc);

    // O_RDONLY | O_DIRECTORY | O_CLOEXEC = 0 | 0x10000 | 0x80000 on Linux.
    const long O_RDONLY    = 0;
    const long O_DIRECTORY = 0x10000;
    const long O_CLOEXEC   = 0x80000;
#    if defined(__x86_64__)
    long fd = misra_sys3(MISRA_SYS_open, (long)(uintptr_t)path, O_RDONLY | O_DIRECTORY | O_CLOEXEC, 0);
#    else
    // aarch64: openat(AT_FDCWD=-100, path, flags, mode)
    long fd = misra_sys4(MISRA_SYS_openat, -100L, (long)(uintptr_t)path, O_RDONLY | O_DIRECTORY | O_CLOEXEC, 0);
#    endif
    if (fd < 0) {
        LOG_ERROR("DirGetContents: open(\"{}\") failed (errno {})", path, (i32)-fd);
        return dc;
    }

    char buf[8192];
    for (;;) {
        long n = misra_sys3(MISRA_SYS_getdents64, fd, (long)(uintptr_t)buf, (long)sizeof(buf));
        if (n == 0) {
            break; // end of stream
        }
        if (n < 0) {
            LOG_ERROR("DirGetContents: getdents64 failed (errno {})", (i32)-n);
            break;
        }
        for (long off = 0; off < n;) {
            struct misra_linux_dirent64 *de = (struct misra_linux_dirent64 *)(void *)(buf + off);
            const char                  *nm = de->d_name;
            // Skip "." and "..".
            if (!(nm[0] == '.' && (nm[1] == '\0' || (nm[1] == '.' && nm[2] == '\0')))) {
                DirEntry direntry = {0};
                direntry.type     = dirent_type_to_misra(de->d_type);
                direntry.name     = StrInitFromCstr(nm, ZstrLen(nm), alloc);
                VecPushBack(&dc, direntry);
            }
            off += de->d_reclen;
        }
    }

    (void)misra_sys1(MISRA_SYS_close, fd);
    return dc;
}
#else
// APPLE or other Unix-based system implementation using opendir/readdir.
DirContents DirGetContents(const char *path, Allocator *alloc) {
    if (!path || !alloc) {
        LOG_FATAL("invalid arguments.");
    }

    DirContents dc = (DirContents)VecInit(alloc);

    DIR *dir = opendir(path);
    if (NULL == dir) {
        // macOS-only path -- opendir is libc and sets errno on failure.
        // SYS_ERRNO routes to errno here since FEATURE_DIRECT_SYSCALL
        // is off on macOS.
        LOG_SYS_ERROR(SYS_ERRNO(-1), "opendir(\"{}\") failed", path);
        return dc;
    }

    // Go through each directory entry
    struct dirent *entry = NULL;
    while (NULL != (entry = readdir(dir))) {
        if ('.' == entry->d_name[0] && 0 == entry->d_name[1]) {
            continue;
        } else if ('.' == entry->d_name[0] && '.' == entry->d_name[1] && 0 == entry->d_name[2]) {
            continue;
        } else {
            Str         entry_path = StrInit(alloc);
            const char *dir_name   = &entry->d_name[0];
            StrWriteFmt(&entry_path, "{}/{}", path, dir_name);

            struct stat path_stat;
            stat(entry_path.data, &path_stat);

            StrDeinit(&entry_path);

            DirEntry direntry = {0};
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
#    if defined(__APPLE__)
            direntry.name = StrInitFromCstr(entry->d_name, entry->d_namlen, alloc);
#    else
            direntry.name = StrInitFromCstr(entry->d_name, ZstrLen(entry->d_name), alloc);
#    endif
            VecPushBack(&dc, direntry);
        }
    }

    closedir(dir);

    return dc;
}
#endif

// Cross-platform function to get file size
i64 FileGetSize(const char *filename) {
#ifdef _WIN32
    // Windows-specific code using GetFileSizeEx
    HANDLE file = CreateFileA(filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    if (file == INVALID_HANDLE_VALUE) {
        // Win32 sets GetLastError, not errno. Log it explicitly.
        LOG_ERROR("CreateFileA() failed (GetLastError={})", (i32)GetLastError());
        return -1;
    }

    LARGE_INTEGER file_size;
    if (!GetFileSizeEx(file, &file_size)) {
        LOG_ERROR("GetFileSizeEx() failed (GetLastError={})", (i32)GetLastError());
        CloseHandle(file);
        return -1;
    }

    CloseHandle(file);
    return (i64)file_size.QuadPart;
#elif FEATURE_DIRECT_SYSCALL
    // Open + lseek(SEEK_END) + close. Avoids needing the kernel's
    // arch-specific `struct stat` layout: lseek returns the offset
    // value the kernel computes, which equals file size at SEEK_END.
    const long O_RDONLY  = 0;
    const long O_CLOEXEC = 0x80000;
    const long SEEK_END_ = 2;
#    if defined(__x86_64__)
    long fd = misra_sys3(MISRA_SYS_open, (long)(uintptr_t)filename, O_RDONLY | O_CLOEXEC, 0);
#    else
    long fd = misra_sys4(MISRA_SYS_openat, -100L, (long)(uintptr_t)filename, O_RDONLY | O_CLOEXEC, 0);
#    endif
    if (fd < 0) {
        LOG_ERROR("FileGetSize: open(\"{}\") failed (errno {})", filename, (i32)-fd);
        return -1;
    }
    long sz = misra_sys3(MISRA_SYS_lseek, fd, 0, SEEK_END_);
    (void)misra_sys1(MISRA_SYS_close, fd);
    if (sz < 0) {
        LOG_ERROR("FileGetSize: lseek failed on \"{}\" (errno {})", filename, (i32)-sz);
        return -1;
    }
    return (i64)sz;
#else
    // Unix-like systems (Linux/macOS) code using stat. Only reached
    // when FEATURE_DIRECT_SYSCALL is off (i.e., macOS); SYS_ERRNO
    // collapses to reading errno on that path.
    struct stat file_stat;
    if (stat(filename, &file_stat) == 0) {
        return (i64)file_stat.st_size;
    } else {
        LOG_SYS_ERROR(SYS_ERRNO(-1), "stat() failed");
        return -1;
    }
#endif
}
