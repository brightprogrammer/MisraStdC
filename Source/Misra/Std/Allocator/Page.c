/// file      : std/allocator/page.c
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Page-granular allocator implementation. Routes every allocation through
/// the OS (mmap on POSIX, VirtualAlloc on Windows) so the library does not
/// depend on libc malloc here. State is inline in the `PageAllocator`
/// struct - just a cached page size.

#include <Misra/Std/Allocator/Page.h>
#include <Misra/Std/Log.h>
#include <Misra/Std/Memory.h>
#include <Misra/Sys.h>

#include <stddef.h>

static void page_validate_self(const Allocator *self) {
    if (!self || self->__magic != MISRA_PAGE_ALLOCATOR_MAGIC) {
        LOG_FATAL("type-confusion: allocator passed to page_allocator_* is not a PageAllocator");
    }
}

#ifdef _WIN32
#    define MISRA_PAGE_ALLOCATOR_WINDOWS 1
#    include <windows.h>
#else
#    define MISRA_PAGE_ALLOCATOR_POSIX 1
#    include <sys/mman.h>
#    if !defined(MAP_ANONYMOUS) && defined(MAP_ANON)
#        define MAP_ANONYMOUS MAP_ANON
#    endif
#endif

// Linux/macOS guarantee these page sizes on the arches we support, so
// we can answer the query without a libc call (`sysconf(_SC_PAGESIZE)`
// or `getpagesize()`). Apple Silicon (arm64 darwin) uses 16 KiB
// pages; everything else on our matrix uses 4 KiB.
static size page_query_page_size(void) {
#if defined(MISRA_PAGE_ALLOCATOR_WINDOWS)
    // kernel32 -- not libc.
    SYSTEM_INFO info;
    GetSystemInfo(&info);
    if (!info.dwPageSize) {
        return 4096;
    }
    return (size)info.dwPageSize;
#elif defined(__APPLE__) && defined(__aarch64__)
    return 16384;
#else
    // Linux/macOS-x86_64/arm64-linux all use 4 KiB. If a future port
    // lands on a kernel that disagrees, this assumption needs to move
    // to a runtime query.
    return 4096;
#endif
}

size PageAllocatorPageSize(PageAllocator *self) {
    if (self && self->cached_page_size) {
        return self->cached_page_size;
    }
    size ps = page_query_page_size();
    if (self) {
        self->cached_page_size = ps;
    }
    return ps;
}

static bool page_alignment_is_pow2(size alignment) {
    return alignment != 0 && ((alignment & (alignment - 1)) == 0);
}

static size page_effective_alignment(PageAllocator *self) {
    size requested = self ? self->base.alignment : 0;
    size page_size = PageAllocatorPageSize(self);
    if (requested < page_size) {
        return page_size;
    }
    if (!page_alignment_is_pow2(requested)) {
        return page_size;
    }
    return requested;
}

static size page_round_up(size bytes, size align) {
    if (!bytes) {
        return 0;
    }
    return (bytes + align - 1) & ~(align - 1);
}

#include <stdint.h>

// Linux: invoke mmap/munmap/mprotect directly via the syscall ABI so
// libc's wrappers stay out of the undefined-symbol list. The kernel
// returns negative values < 4096 as -errno; anything else is the
// successful return (a pointer for mmap, 0 for munmap/mprotect).
// macOS / BSD: keep libSystem's wrappers (Apple disallows direct
// user-mode syscalls). Windows: kernel32 VirtualAlloc/Free/Protect.
#if defined(__linux__) && (defined(__x86_64__) || defined(__aarch64__))
static inline long misra_syscall_mmap(void *addr, unsigned long len, long prot, long flags, long fd, long offset) {
    long ret;
#    if defined(__x86_64__)
    register long r10 __asm__("r10") = flags;
    register long r8 __asm__("r8")   = fd;
    register long r9 __asm__("r9")   = offset;
    __asm__ volatile("syscall"
                     : "=a"(ret)
                     : "0"(9), "D"(addr), "S"(len), "d"(prot), "r"(r10), "r"(r8), "r"(r9) // SYS_mmap
                     : "rcx", "r11", "memory");
#    else                                                                                 // __aarch64__
    register long x8 __asm__("x8") = 222; // SYS_mmap
    register long x0 __asm__("x0") = (long)(uintptr_t)addr;
    register long x1 __asm__("x1") = (long)len;
    register long x2 __asm__("x2") = prot;
    register long x3 __asm__("x3") = flags;
    register long x4 __asm__("x4") = fd;
    register long x5 __asm__("x5") = offset;
    __asm__ volatile("svc #0"
                     : "+r"(x0)
                     : "r"(x8), "r"(x1), "r"(x2), "r"(x3), "r"(x4), "r"(x5)
                     : "memory");
    ret = x0;
#    endif
    return ret;
}

static inline long misra_syscall_munmap(void *addr, unsigned long len) {
    long ret;
#    if defined(__x86_64__)
    __asm__ volatile("syscall"
                     : "=a"(ret)
                     : "0"(11), "D"(addr), "S"(len)
                     : "rcx", "r11", "memory");
#    else
    register long x8 __asm__("x8") = 215;
    register long x0 __asm__("x0") = (long)(uintptr_t)addr;
    register long x1 __asm__("x1") = (long)len;
    __asm__ volatile("svc #0"
                     : "+r"(x0)
                     : "r"(x8), "r"(x1)
                     : "memory");
    ret = x0;
#    endif
    return ret;
}

static inline long misra_syscall_mprotect(void *addr, unsigned long len, long prot) {
    long ret;
#    if defined(__x86_64__)
    __asm__ volatile("syscall"
                     : "=a"(ret)
                     : "0"(10), "D"(addr), "S"(len), "d"(prot)
                     : "rcx", "r11", "memory");
#    else
    register long x8 __asm__("x8") = 226;
    register long x0 __asm__("x0") = (long)(uintptr_t)addr;
    register long x1 __asm__("x1") = (long)len;
    register long x2 __asm__("x2") = prot;
    __asm__ volatile("svc #0"
                     : "+r"(x0)
                     : "r"(x8), "r"(x1), "r"(x2)
                     : "memory");
    ret = x0;
#    endif
    return ret;
}
#endif

static void *page_map(size bytes) {
#if defined(MISRA_PAGE_ALLOCATOR_WINDOWS)
    return VirtualAlloc(NULL, (SIZE_T)bytes, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
#elif defined(__linux__) && (defined(__x86_64__) || defined(__aarch64__))
    long ret =
        misra_syscall_mmap(NULL, (unsigned long)bytes, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    // Kernel encodes errno as small negative integers; valid pointers
    // are always large unsigned values.
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

static void page_unmap(void *ptr, size bytes) {
    if (!ptr || !bytes) {
        return;
    }
#if defined(MISRA_PAGE_ALLOCATOR_WINDOWS)
    (void)bytes;
    VirtualFree(ptr, 0, MEM_RELEASE);
#elif defined(__linux__) && (defined(__x86_64__) || defined(__aarch64__))
    (void)misra_syscall_munmap(ptr, (unsigned long)bytes);
#else
    munmap(ptr, (size_t)bytes);
#endif
}

static size page_rounded_size(PageAllocator *self, size bytes) {
    size align     = page_effective_alignment(self);
    size page_size = PageAllocatorPageSize(self);
    if (align > page_size) {
        return page_round_up(bytes, align);
    }
    return page_round_up(bytes, page_size);
}

void *page_allocator_allocate(Allocator *self, size bytes, i8 zeroed) {
    page_validate_self(self);
    (void)zeroed; // OS-mapped pages are kernel-zeroed.
    if (!bytes) {
        return NULL;
    }
    PageAllocator *page = (PageAllocator *)self;
    return page_map(page_rounded_size(page, bytes));
}

void *page_allocator_reallocate(Allocator *self, void *ptr, size old_size, size new_size) {
    page_validate_self(self);
    PageAllocator *page = (PageAllocator *)self;

    if (new_size == 0) {
        page_unmap(ptr, page_rounded_size(page, old_size));
        return NULL;
    }

    void *fresh = page_allocator_allocate(self, new_size, true);
    if (!fresh) {
        return NULL;
    }
    if (ptr) {
        size copy_bytes = old_size < new_size ? old_size : new_size;
        MemCopy(fresh, ptr, copy_bytes);
        page_unmap(ptr, page_rounded_size(page, old_size));
    }
    return fresh;
}

void page_allocator_deallocate(Allocator *self, void *ptr, size bytes) {
    page_validate_self(self);
    PageAllocator *page = (PageAllocator *)self;
    page_unmap(ptr, page_rounded_size(page, bytes));
}

bool PageProtect(void *ptr, size bytes, PageProtection prot) {
    if (!ptr || !bytes) {
        return false;
    }

#if defined(MISRA_PAGE_ALLOCATOR_WINDOWS)
    DWORD win_prot = 0;
    switch (prot) {
        case PAGE_PROT_NONE :
            win_prot = PAGE_NOACCESS;
            break;
        case PAGE_PROT_READ :
            win_prot = PAGE_READONLY;
            break;
        case PAGE_PROT_READ_WRITE :
            win_prot = PAGE_READWRITE;
            break;
        default :
            LOG_ERROR("PageProtect: unknown protection bit {}", (u32)prot);
            return false;
    }
    DWORD old_prot = 0;
    if (!VirtualProtect(ptr, (SIZE_T)bytes, win_prot, &old_prot)) {
        LOG_ERROR("PageProtect: VirtualProtect failed (error {})", (u32)GetLastError());
        return false;
    }
    return true;
#else
    int posix_prot = 0;
    switch (prot) {
        case PAGE_PROT_NONE :
            posix_prot = PROT_NONE;
            break;
        case PAGE_PROT_READ :
            posix_prot = PROT_READ;
            break;
        case PAGE_PROT_READ_WRITE :
            posix_prot = PROT_READ | PROT_WRITE;
            break;
        default :
            LOG_ERROR("PageProtect: unknown protection bit {}", (u32)prot);
            return false;
    }
#    if defined(__linux__) && (defined(__x86_64__) || defined(__aarch64__))
    long ret = misra_syscall_mprotect(ptr, (unsigned long)bytes, posix_prot);
    if (ret != 0) {
        LOG_ERROR("PageProtect: mprotect failed (errno {})", (i32)-ret);
        return false;
    }
#    else
    if (mprotect(ptr, (size_t)bytes, posix_prot) != 0) {
        LOG_SYS_ERROR("PageProtect: mprotect failed");
        return false;
    }
#    endif
    return true;
#endif
}
