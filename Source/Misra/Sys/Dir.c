#include <Misra/Sys/Dir.h>
#include <Misra/Std/Log.h>

#if PLATFORM_WINDOWS
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
#    if PLATFORM_DARWIN
#        include <mach-o/dyld.h>
#    endif
#endif

#include <Misra/Std.h>
#include <Misra/Std/Log.h>
#include <Misra/Sys.h>

#include "../_Syscall.h"

Zstr DirEntryTypeToZstr(DirEntryType type) {
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

#if PLATFORM_WINDOWS
// Windows-specific implementation using FindFirstFile/FindNextFile
DirContents dir_get_contents(Zstr path, Allocator *alloc) {
    if (!path || !alloc) {
        LOG_FATAL("Invalid argument");
    }

    DirContents dc = (DirContents)VecInit(alloc);

    // Construct the search path: "<path>\*". Done by hand so we
    // don't pull `<stdio.h>` for `snprintf` (libc-free goal; clang-cl
    // with `-Werror=implicit-function-declaration` would also fail
    // the implicit decl).
    char search_path[MAX_PATH];
    size path_len = ZstrLen(path);
    if (path_len + 3 > sizeof(search_path)) {
        LOG_ERROR("dir_get_contents: path too long for MAX_PATH");
        return dc;
    }
    MemCopy(search_path, path, path_len);
    search_path[path_len]     = '\\';
    search_path[path_len + 1] = '*';
    search_path[path_len + 2] = '\0';

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

#    if PLATFORM_DARWIN
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

DirContents dir_get_contents(Zstr path, Allocator *alloc) {
    if (!path || !alloc) {
        LOG_FATAL("invalid arguments.");
    }

    DirContents dc = (DirContents)VecInit(alloc);

    // O_RDONLY | O_DIRECTORY | O_CLOEXEC. Values match between Linux
    // and Darwin for O_RDONLY (0) but DIFFER for the others. Use the
    // per-OS values.
#    if PLATFORM_DARWIN
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

#    if PLATFORM_DARWIN || ARCHITECTURE_X86_64
    // Both Darwin and Linux-x86_64 have plain SYS_open. Darwin aarch64
    // also has SYS_open (the legacy BSD numbering is intact on Apple
    // even on Apple Silicon). Linux-x86_64 has SYS_open. Linux-aarch64
    // does NOT (was removed; openat-only).
    long fd = misra_sys3(MISRA_SYS_open, (long)(u64)path, O_RDONLY | O_DIRECTORY | O_CLOEXEC, 0);
#    else
    // Linux-aarch64: openat(AT_FDCWD=-100, path, flags, mode).
    long fd = misra_sys4(MISRA_SYS_openat, -100L, (long)(u64)path, O_RDONLY | O_DIRECTORY | O_CLOEXEC, 0);
#    endif
    if (fd < 0) {
        LOG_ERROR("DirGetContents: open(\"{}\") failed (errno {})", path, (i32)-fd);
        return dc;
    }

    char buf[8192];
#    if PLATFORM_DARWIN
    // Darwin getdirentries64 needs an in/out file-position arg
    // (`basep`). Initial 0; kernel updates it after each call.
    i64 basep = 0;
#    endif
    for (;;) {
#    if PLATFORM_DARWIN
        long n = misra_sys4(MISRA_SYS_getdents64, fd, (long)(u64)buf, (long)sizeof(buf), (long)(u64)&basep);
#    else
        long n = misra_sys3(MISRA_SYS_getdents64, fd, (long)(u64)buf, (long)sizeof(buf));
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
#    if PLATFORM_DARWIN
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
DirContents dir_get_contents(Zstr path, Allocator *alloc) {
    if (!path || !alloc) {
        LOG_FATAL("invalid arguments.");
    }

    DirContents dc = (DirContents)VecInit(alloc);

    DIR *dir = opendir(path);
    if (NULL == dir) {
        // macOS-only path -- opendir is libc and sets errno on failure.
        // ErrnoOf routes to errno here since FEATURE_DIRECT_SYSCALL
        // is off on macOS.
        LOG_SYS_ERROR(ErrnoOf(-1), "opendir(\"{}\") failed", path);
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
            Zstr dir_name   = &entry->d_name[0];
            StrAppendFmt(&entry_path, "{}/{}", path, dir_name);

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
#    if PLATFORM_DARWIN
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
i64 file_get_size(Zstr filename) {
#if PLATFORM_WINDOWS
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
    const long O_RDONLY = 0;
#    if PLATFORM_DARWIN
    const long O_CLOEXEC = 0x1000000;
#    else
    const long O_CLOEXEC = 0x80000;
#    endif
    const long SEEK_END_ = 2;
#    if PLATFORM_DARWIN || ARCHITECTURE_X86_64
    long fd = misra_sys3(MISRA_SYS_open, (long)(u64)filename, O_RDONLY | O_CLOEXEC, 0);
#    else
    long fd = misra_sys4(MISRA_SYS_openat, -100L, (long)(u64)filename, O_RDONLY | O_CLOEXEC, 0);
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
    // when FEATURE_DIRECT_SYSCALL is off (i.e., macOS); ErrnoOf
    // collapses to reading errno on that path.
    struct stat file_stat;
    if (stat(filename, &file_stat) == 0) {
        return (i64)file_stat.st_size;
    } else {
        LOG_SYS_ERROR(ErrnoOf(-1), "stat() failed");
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

i8 file_remove(Zstr path) {
    if (!path) {
        LOG_FATAL("FileRemove: NULL path");
    }
#if PLATFORM_WINDOWS
    if (!DeleteFileA(path)) {
        LOG_ERROR("FileRemove(\"{}\"): DeleteFileA failed (GetLastError={})", path, (i32)GetLastError());
        return 0;
    }
    return 1;
#elif FEATURE_DIRECT_SYSCALL
#    if PLATFORM_DARWIN || ARCHITECTURE_X86_64
    // Darwin has SYS_unlink (#10, BSD) on both x86_64 and aarch64.
    // Linux x86_64 also has SYS_unlink. Linux aarch64 doesn't.
    long ret = misra_sys1(MISRA_SYS_unlink, (long)(u64)path);
#    else
    // Linux aarch64: AT_FDCWD = -100, flags = 0 (regular unlink).
    long ret = misra_sys3(MISRA_SYS_unlinkat, -100L, (long)(u64)path, 0);
#    endif
    if (ret < 0) {
        LOG_SYS_ERROR(ErrnoOf(ret), "FileRemove(\"{}\")", path);
        return 0;
    }
    return 1;
#else
#    error "FileRemove: no direct-syscall path. Add the right MISRA_SYS_unlink* number in _Syscall.h for this arch."
#endif
}

i8 dir_remove(Zstr path) {
    if (!path) {
        LOG_FATAL("DirRemove: NULL path");
    }
#if PLATFORM_WINDOWS
    if (!RemoveDirectoryA(path)) {
        LOG_ERROR("DirRemove(\"{}\"): RemoveDirectoryA failed (GetLastError={})", path, (i32)GetLastError());
        return 0;
    }
    return 1;
#elif FEATURE_DIRECT_SYSCALL
#    if PLATFORM_DARWIN || ARCHITECTURE_X86_64
    // Darwin has SYS_rmdir (#137, BSD) on both arches. Linux x86_64
    // has SYS_rmdir; Linux aarch64 went unlinkat-only.
    long ret = misra_sys1(MISRA_SYS_rmdir, (long)(u64)path);
#    else
    // Linux aarch64: AT_FDCWD = -100, AT_REMOVEDIR = 0x200.
    long ret = misra_sys3(MISRA_SYS_unlinkat, -100L, (long)(u64)path, 0x200);
#    endif
    if (ret < 0) {
        LOG_SYS_ERROR(ErrnoOf(ret), "DirRemove(\"{}\")", path);
        return 0;
    }
    return 1;
#else
#    error                                                                                                             \
        "DirRemove: no direct-syscall path. Add MISRA_SYS_rmdir / MISRA_SYS_unlinkat numbers in _Syscall.h for this arch."
#endif
}

// ---------------------------------------------------------------------------
// DirCreate / DirCreateAll / DirRemoveAll. Linux x86_64 has SYS_mkdir;
// aarch64 dropped it -- use SYS_mkdirat with AT_FDCWD. Darwin keeps
// SYS_mkdir (#136, BSD) on both arches. Windows uses CreateDirectoryA.
// ---------------------------------------------------------------------------

// 0755 is the POSIX-conventional permission for newly-created
// directories: owner rwx, group rx, world rx. Subject to the process
// umask, which the kernel applies after we pass `mode`.
#define DIR_CREATE_MODE 0755

i8 dir_create(Zstr path) {
    if (!path) {
        LOG_FATAL("DirCreate: NULL path");
    }
#if PLATFORM_WINDOWS
    if (!CreateDirectoryA(path, NULL)) {
        LOG_ERROR("DirCreate(\"{}\"): CreateDirectoryA failed (GetLastError={})", path, (i32)GetLastError());
        return 0;
    }
    return 1;
#elif FEATURE_DIRECT_SYSCALL
#    if PLATFORM_DARWIN || ARCHITECTURE_X86_64
    long ret = misra_sys2(MISRA_SYS_mkdir, (long)(u64)path, DIR_CREATE_MODE);
#    else
    // Linux aarch64: AT_FDCWD = -100.
    long ret = misra_sys3(MISRA_SYS_mkdirat, -100L, (long)(u64)path, DIR_CREATE_MODE);
#    endif
    if (ret < 0) {
        LOG_SYS_ERROR(ErrnoOf(ret), "DirCreate(\"{}\")", path);
        return 0;
    }
    return 1;
#else
#    error                                                                                                             \
        "DirCreate: no direct-syscall path. Add MISRA_SYS_mkdir / MISRA_SYS_mkdirat numbers in _Syscall.h for this arch."
#endif
}

// Check whether the given path already exists as a directory. Used by
// DirCreateAll to make EEXIST tolerant (idempotent). Avoids re-walking
// the existing tree on the second invocation.
static bool dir_already_exists(Zstr path) {
#if PLATFORM_WINDOWS
    DWORD attrs = GetFileAttributesA(path);
    return attrs != INVALID_FILE_ATTRIBUTES && (attrs & FILE_ATTRIBUTE_DIRECTORY);
#elif FEATURE_DIRECT_SYSCALL
    // newfstatat with AT_FDCWD (= -100); buf is opaque, we only need
    // the syscall to succeed. 256 bytes is enough for `struct stat` /
    // `stat64` on both Linux and Darwin x86_64/aarch64.
    u8   buf[256] = {0};
    long ret;
#    if PLATFORM_DARWIN
    ret = misra_sys4(MISRA_SYS_fstatat64, -100L, (long)(u64)path, (long)(u64)buf, 0);
#    else
    ret = misra_sys4(MISRA_SYS_newfstatat, -100L, (long)(u64)path, (long)(u64)buf, 0);
#    endif
    return ret >= 0;
#else
#    error                                                                                                             \
        "dir_already_exists: no direct-syscall path. Add the right MISRA_SYS_*stat number in _Syscall.h for this arch."
#endif
}

i8 dir_create_all(Zstr path) {
    if (!path) {
        LOG_FATAL("DirCreateAll: NULL path");
    }
    size n = ZstrLen(path);
    if (n == 0) {
        return 1;
    }
    if (n >= 4096) {
        LOG_ERROR("DirCreateAll(\"{}\"): path too long ({} bytes)", path, (u64)n);
        return 0;
    }
    // Walk a stack copy, NUL-terminating at each '/' to create
    // intermediate components. Restores the slash before continuing.
    char buf[4096];
    MemCopy(buf, path, n);
    buf[n] = 0;
    // Skip leading slash so the loop doesn't try to mkdir("").
    size i = (buf[0] == '/') ? 1 : 0;
    for (; i <= n; ++i) {
        if (i == n || buf[i] == '/') {
            // Trim duplicate slashes (//) and trailing slash.
            if (i < n && i > 0 && buf[i - 1] == '/') {
                continue;
            }
            char saved = buf[i];
            buf[i]     = 0;
            if (!dir_already_exists(buf)) {
                if (!DirCreate((Zstr)buf)) {
                    // DirCreate logged the syscall error; re-check
                    // in case a concurrent process beat us to it.
                    if (!dir_already_exists(buf)) {
                        buf[i] = saved;
                        return 0;
                    }
                }
            }
            buf[i] = saved;
        }
    }
    return 1;
}

// Per-entry "parent/child" path buffer cap for the recursive removal
// loop. Kept well under 4 KiB on purpose: on macOS, Clang emits an
// implicit `___chkstk_darwin` call in the prologue of any function
// whose stack frame exceeds ~4 KiB (see `_Freestanding.c`), and
// `dir_remove_all` recurses one frame per directory level -- a 4 KiB
// buffer per frame would compound into deep-tree stack pressure.
// 512 covers any realistic single path component plus the parent
// prefix; overflow spills through `StrInitStack`'s fallback allocator.
#define DIR_REMOVE_ALL_PATH_CAP 512

i8 dir_remove_all(Zstr path) {
    if (!path) {
        LOG_FATAL("DirRemoveAll: NULL path");
    }
    // Stat first: a missing path is a successful no-op (mirrors
    // `rm -rf` semantics that callers rely on for test cleanup).
    if (!dir_already_exists(path)) {
        return 1;
    }

    // `DirContents` is variable-sized so it has to be heap-backed --
    // we don't know the entry count in advance. The per-entry path
    // string is stack-backed via StrInitStack below.
    HeapAllocator ha = HeapAllocatorInit();
    Allocator    *al = ALLOCATOR_OF(&ha);
    DirContents   dc = dir_get_contents(path, al);

    bool ok        = true;
    size path_len  = ZstrLen(path);
    bool trail_sep = (path_len > 0 && path[path_len - 1] == '/');
    for (size i = 0; i < dc.length; ++i) {
        DirEntry *e = &dc.data[i];
        if (ZstrCompare(e->name.data, ".") == 0 || ZstrCompare(e->name.data, "..") == 0) {
            continue;
        }
        // Per-iteration "parent/child" path. Stack-backed buffer so we
        // don't allocate; `al` is the overflow-fallback only.
        bool inner_ok = false;
        Str  child;
        StrInitStack(child, al, DIR_REMOVE_ALL_PATH_CAP, {
            StrAppendFmt(&child, trail_sep ? "{}{}" : "{}/{}", path, e->name.data);
            if (e->type == SYS_DIR_ENTRY_TYPE_DIRECTORY) {
                inner_ok = DirRemoveAll(&child);
            } else {
                inner_ok = FileRemove(&child);
            }
        });
        ok = ok && inner_ok;
        if (!ok) {
            break;
        }
    }
    VecDeinit(&dc);
    HeapAllocatorDeinit(&ha);

    if (!ok) {
        return 0;
    }
    return DirRemove(path);
}
