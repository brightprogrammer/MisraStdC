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
    size ps             = os_page_size_query();
    os_page_size_cached = ps;
    return ps;
}

// Linux: mmap/munmap via direct syscall when FEATURE_DIRECT_SYSCALL is
// on (kernel returns negative values in [-4095, -1] as the negated
// error code; anything else is the success value). macOS / BSD: system
// wrappers. Windows: kernel32.

// Footprint accounting is interleaved with the kernel calls below
// rather than left to per-site callers. <Misra/Std/Allocator.h> isn't
// pulled in here -- _Os.h forward-declares `Allocator`, and the only
// field we touch lives at a known offset. Hand-roll the access through
// a tiny static inline whose body is the field write.
#include <Misra/Std/Allocator.h>

static FORCE_INLINE void os_footprint_add(Allocator *owner, size bytes) {
    if (owner)
        owner->footprint_bytes += bytes;
}
static FORCE_INLINE void os_footprint_sub(Allocator *owner, size bytes) {
    if (!owner)
        return;
    if (bytes <= owner->footprint_bytes) {
        owner->footprint_bytes -= bytes;
    } else {
        owner->footprint_bytes = 0;
    }
}

void *os_page_map(Allocator *owner, size bytes) {
#if defined(OS_WINDOWS)
    void *p = VirtualAlloc(NULL, (SIZE_T)bytes, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
#elif FEATURE_DIRECT_SYSCALL
    long  ret = misra_sys6(MISRA_SYS_mmap, 0, (long)bytes, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    void *p   = ((unsigned long)ret >= (unsigned long)-4095) ? NULL : (void *)ret;
#else
    void *p = mmap(NULL, (size_t)bytes, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (p == MAP_FAILED)
        p = NULL;
#endif
    if (p)
        os_footprint_add(owner, bytes);
    return p;
}

void os_page_unmap(Allocator *owner, void *ptr, size bytes) {
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
    os_footprint_sub(owner, bytes);
}

void *os_page_remap(Allocator *owner, void *ptr, size old_bytes, size new_bytes) {
    if (!ptr || !old_bytes || !new_bytes) {
        return NULL;
    }
#if PLATFORM_LINUX && FEATURE_DIRECT_SYSCALL
    // mremap(old_addr, old_size, new_size, flags). Flag 1 = MREMAP_MAYMOVE.
    long ret = misra_sys4(MISRA_SYS_mremap, (long)(u64)ptr, (long)old_bytes, (long)new_bytes, 1L /* MREMAP_MAYMOVE */);
    if ((unsigned long)ret >= (unsigned long)-4095) {
        return NULL;
    }
    if (new_bytes >= old_bytes) {
        os_footprint_add(owner, new_bytes - old_bytes);
    } else {
        os_footprint_sub(owner, old_bytes - new_bytes);
    }
    return (void *)ret;
#else
    // No fallback: mremap is a Linux extension, not POSIX, and the
    // non-direct-syscall path has no way to reach it without dragging
    // in an external runtime dependency. Darwin has no public mremap;
    // Windows can't resize a VirtualAlloc reservation in place.
    // Callers fall back to alloc-new + MemCopy.
    (void)owner;
    (void)ptr;
    (void)old_bytes;
    (void)new_bytes;
    return NULL;
#endif
}
