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
// Direct-syscall path. Two flavours under the same hood:
//   - Linux: open + getdents64 syscall; parses `struct linux_dirent64`
//     records out of the kernel's buffer.
//   - Darwin: open + getdirentries64 syscall (#344) which has the
//     same shape but writes Darwin's own `struct dirent` layout
//     (different field order + sizes) and takes an extra in/out
//     `basep` (file position) pointer arg.
// Both kernels fill d_type so we don't need a separate stat() per
// entry; DT_UNKNOWN entries stay as SYS_DIR_ENTRY_TYPE_UNKNOWN (some
// filesystems don't populate d_type and force the caller to stat).

// File-type bits. POSIX dirent.h constants -- shared between Linux
// (d_type in linux_dirent64) and Darwin (d_type in struct dirent).
#    define DIRENT_TYPE_UNKNOWN 0
#    define DIRENT_TYPE_FIFO    1
#    define DIRENT_TYPE_CHR     2
#    define DIRENT_TYPE_DIR     4
#    define DIRENT_TYPE_BLK     6
#    define DIRENT_TYPE_REG     8
#    define DIRENT_TYPE_LNK     10

#    if defined(__APPLE__)
// Darwin's struct dirent (per sys/dirent.h, the __DARWIN_64_BIT_INO_T
// variant the kernel emits via getdirentries64). Field order is
// d_ino / d_seekoff / d_reclen / d_namlen / d_type / d_name. d_name
// is fixed-size in the system header but in the kernel-emitted
// records only the first d_namlen bytes are valid.
struct misra_kernel_dirent {
    u64  d_ino;
    u64  d_seekoff;
    u16  d_reclen;
    u16  d_namlen;
    u8   d_type;
    char d_name[]; // variable; valid for d_namlen bytes
};
#    else
// Linux's struct linux_dirent64. d_name is null-terminated; the next
// record starts d_reclen bytes from the start of this one.
struct misra_kernel_dirent {
    u64  d_ino;
    i64  d_off;
    u16  d_reclen;
    u8   d_type;
    char d_name[]; // null-terminated
};
#    endif

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

    // O_RDONLY | O_DIRECTORY | O_CLOEXEC. Values match between Linux
    // and Darwin for O_RDONLY (0) but DIFFER for the others. Use the
    // per-OS values.
#    if defined(__APPLE__)
    //   Darwin: O_RDONLY=0, O_DIRECTORY=0x100000, O_CLOEXEC=0x1000000.
    const long O_RDONLY    = 0;
    const long O_DIRECTORY = 0x100000;
    const long O_CLOEXEC   = 0x1000000;
#    else
    //   Linux:  O_RDONLY=0, O_DIRECTORY=0x10000,  O_CLOEXEC=0x80000.
    const long O_RDONLY    = 0;
    const long O_DIRECTORY = 0x10000;
    const long O_CLOEXEC   = 0x80000;
#    endif

#    if defined(__APPLE__) || defined(__x86_64__)
    // Both Darwin and Linux-x86_64 have plain SYS_open. Darwin aarch64
    // also has SYS_open (the legacy BSD numbering is intact on Apple
    // even on Apple Silicon). Linux-x86_64 has SYS_open. Linux-aarch64
    // does NOT (was removed; openat-only).
    long fd = misra_sys3(MISRA_SYS_open, (long)(uintptr_t)path, O_RDONLY | O_DIRECTORY | O_CLOEXEC, 0);
#    else
    // Linux-aarch64: openat(AT_FDCWD=-100, path, flags, mode).
    long fd =
        misra_sys4(MISRA_SYS_openat, -100L, (long)(uintptr_t)path, O_RDONLY | O_DIRECTORY | O_CLOEXEC, 0);
#    endif
    if (fd < 0) {
        LOG_ERROR("DirGetContents: open(\"{}\") failed (errno {})", path, (i32)-fd);
        return dc;
    }

    char buf[8192];
#    if defined(__APPLE__)
    // Darwin getdirentries64 needs an in/out file-position arg
    // (`basep`). Initial 0; kernel updates it after each call.
    i64 basep = 0;
#    endif
    for (;;) {
#    if defined(__APPLE__)
        long n = misra_sys4(
            MISRA_SYS_getdents64,
            fd,
            (long)(uintptr_t)buf,
            (long)sizeof(buf),
            (long)(uintptr_t)&basep
        );
#    else
        long n = misra_sys3(MISRA_SYS_getdents64, fd, (long)(uintptr_t)buf, (long)sizeof(buf));
#    endif
        if (n == 0) {
            break; // end of stream
        }
        if (n < 0) {
            LOG_ERROR("DirGetContents: getdents64 failed (errno {})", (i32)-n);
            break;
        }
        for (long off = 0; off < n;) {
            struct misra_kernel_dirent *de = (struct misra_kernel_dirent *)(void *)(buf + off);
            const char                 *nm = de->d_name;
#    if defined(__APPLE__)
            // Darwin gives d_namlen explicitly (not null-terminated
            // beyond it).
            size name_len = (size)de->d_namlen;
#    else
            // Linux name is null-terminated; compute length.
            size name_len = ZstrLen(nm);
#    endif
            // Skip "." and "..".
            int is_dot    = (name_len == 1) && (nm[0] == '.');
            int is_dotdot = (name_len == 2) && (nm[0] == '.') && (nm[1] == '.');
            if (!is_dot && !is_dotdot) {
                DirEntry direntry = {0};
                direntry.type     = dirent_type_to_misra(de->d_type);
                direntry.name     = StrInitFromCstr(nm, name_len, alloc);
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
#    if defined(__APPLE__)
    const long O_CLOEXEC = 0x1000000;
#    else
    const long O_CLOEXEC = 0x80000;
#    endif
    const long SEEK_END_ = 2;
#    if defined(__APPLE__) || defined(__x86_64__)
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

// ---------------------------------------------------------------------------
// FileRemove / DirRemove. Linux uses the direct-syscall path when
// FEATURE_DIRECT_SYSCALL is set (x86_64 -> SYS_unlink/SYS_rmdir;
// aarch64 -> SYS_unlinkat with AT_REMOVEDIR for directories). macOS
// goes through libSystem `unlink` / `rmdir`. Windows uses Win32
// `DeleteFileA` / `RemoveDirectoryA`.
// ---------------------------------------------------------------------------

i8 FileRemove(const char *path) {
    if (!path) {
        LOG_ERROR("FileRemove: NULL path");
        return 0;
    }
#if defined(_WIN32)
    if (!DeleteFileA(path)) {
        LOG_ERROR("FileRemove(\"{}\"): DeleteFileA failed (GetLastError={})", path, (i32)GetLastError());
        return 0;
    }
    return 1;
#elif FEATURE_DIRECT_SYSCALL
#    if defined(__APPLE__) || defined(__x86_64__)
    // Darwin has SYS_unlink (#10, BSD) on both x86_64 and aarch64.
    // Linux x86_64 also has SYS_unlink. Linux aarch64 doesn't.
    long ret = misra_sys1(MISRA_SYS_unlink, (long)(uintptr_t)path);
#    else
    // Linux aarch64: AT_FDCWD = -100, flags = 0 (regular unlink).
    long ret = misra_sys3(MISRA_SYS_unlinkat, -100L, (long)(uintptr_t)path, 0);
#    endif
    if (ret < 0) {
        LOG_SYS_ERROR(SYS_ERRNO(ret), "FileRemove(\"{}\")", path);
        return 0;
    }
    return 1;
#else
    // macOS / non-direct-syscall: libSystem unlink. errno set on
    // failure; SYS_ERRNO falls back to reading it.
    extern int unlink(const char *);
    if (unlink(path) != 0) {
        LOG_SYS_ERROR(SYS_ERRNO(-1), "FileRemove(\"{}\")", path);
        return 0;
    }
    return 1;
#endif
}

i8 DirRemove(const char *path) {
    if (!path) {
        LOG_ERROR("DirRemove: NULL path");
        return 0;
    }
#if defined(_WIN32)
    if (!RemoveDirectoryA(path)) {
        LOG_ERROR("DirRemove(\"{}\"): RemoveDirectoryA failed (GetLastError={})", path, (i32)GetLastError());
        return 0;
    }
    return 1;
#elif FEATURE_DIRECT_SYSCALL
#    if defined(__APPLE__) || defined(__x86_64__)
    // Darwin has SYS_rmdir (#137, BSD) on both arches. Linux x86_64
    // has SYS_rmdir; Linux aarch64 went unlinkat-only.
    long ret = misra_sys1(MISRA_SYS_rmdir, (long)(uintptr_t)path);
#    else
    // Linux aarch64: AT_FDCWD = -100, AT_REMOVEDIR = 0x200.
    long ret = misra_sys3(MISRA_SYS_unlinkat, -100L, (long)(uintptr_t)path, 0x200);
#    endif
    if (ret < 0) {
        LOG_SYS_ERROR(SYS_ERRNO(ret), "DirRemove(\"{}\")", path);
        return 0;
    }
    return 1;
#else
    // macOS / non-direct-syscall: libSystem rmdir.
    extern int rmdir(const char *);
    if (rmdir(path) != 0) {
        LOG_SYS_ERROR(SYS_ERRNO(-1), "DirRemove(\"{}\")", path);
        return 0;
    }
    return 1;
#endif
}
