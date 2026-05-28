/// file      : std/allocator/_os.c  (INTERNAL)
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Implements the internal OS page shim declared in _Os.h. Stateless
/// wrappers over mmap/munmap (POSIX) or VirtualAlloc/VirtualFree
/// (Windows), plus a one-shot cached page-size query.

#include "_Os.h"

#include <Misra/Sys.h>

#if PLATFORM_WINDOWS
#    define OS_WINDOWS 1
#    include <windows.h>
#else
#    define OS_POSIX 1
#    include <sys/mman.h>
#    if !defined(MAP_ANONYMOUS) && defined(MAP_ANON)
#        define MAP_ANONYMOUS MAP_ANON
#    endif
#endif

#include "../../_Syscall.h"

// One-shot cache. Zero indicates "not queried yet"; the first caller
// fills it. Idempotent: any thread that beats us to the punch writes the
// same value, so a race is benign.
static size os_page_size_cached = 0;

static size os_page_size_query(void) {
#if defined(OS_WINDOWS)
    // kernel32 -- not libc.
    SYSTEM_INFO info;
    GetSystemInfo(&info);
    if (!info.dwPageSize) {
        return 4096;
    }
    return (size)info.dwPageSize;
#elif PLATFORM_DARWIN && ARCHITECTURE_AARCH64
    return 16384;
#else
    // Linux/macOS-x86_64/arm64-linux all use 4 KiB. If a future port
    // lands on a kernel that disagrees, this assumption needs to move
    // to a runtime query.
    return 4096;
#endif
}

size os_page_size(void) {
    if (os_page_size_cached) {
        return os_page_size_cached;
    }
    size ps = os_page_size_query();
    os_page_size_cached = ps;
    return ps;
}

// Linux: mmap/munmap via direct syscall when FEATURE_DIRECT_SYSCALL is
// on (kernel returns negative values < 4096 as -errno; anything else is
// the success value). macOS / BSD: libSystem wrappers. Windows: kernel32.

void *os_page_map(size bytes) {
#if defined(OS_WINDOWS)
    return VirtualAlloc(NULL, (SIZE_T)bytes, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
#elif FEATURE_DIRECT_SYSCALL
    long ret = misra_sys6(MISRA_SYS_mmap, 0, (long)bytes, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if ((unsigned long)ret >= (unsigned long)-4095) {
        return NULL;
    }
    return (void *)ret;
#else
    void *ptr = mmap(NULL, (size_t)bytes, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (ptr == MAP_FAILED) {
        return NULL;
    }
    return ptr;
#endif
}

void os_page_unmap(void *ptr, size bytes) {
    if (!ptr || !bytes) {
        return;
    }
#if defined(OS_WINDOWS)
    (void)bytes;
    VirtualFree(ptr, 0, MEM_RELEASE);
#elif FEATURE_DIRECT_SYSCALL
    (void)misra_sys2(MISRA_SYS_munmap, (long)(u64)ptr, (long)bytes);
#else
    munmap(ptr, (size_t)bytes);
#endif
}
