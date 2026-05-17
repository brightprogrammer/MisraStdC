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
    if (!self || self->__magic != PAGE_ALLOCATOR_MAGIC) {
        LOG_FATAL("type-confusion: allocator passed to page_allocator_* is not a PageAllocator");
    }
}

#ifdef _WIN32
#    define PAGE_ALLOCATOR_WINDOWS 1
#    include <windows.h>
#else
#    define PAGE_ALLOCATOR_POSIX 1
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
#if defined(PAGE_ALLOCATOR_WINDOWS)
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

#include "../../_Syscall.h"

// Linux: mmap/munmap/mprotect via direct syscall (kernel returns
// negative values < 4096 as -errno; anything else is the success
// value). macOS / BSD: libSystem wrappers. Windows: kernel32.

static void *page_map(size bytes) {
#if defined(PAGE_ALLOCATOR_WINDOWS)
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

static void page_unmap(void *ptr, size bytes) {
    if (!ptr || !bytes) {
        return;
    }
#if defined(PAGE_ALLOCATOR_WINDOWS)
    (void)bytes;
    VirtualFree(ptr, 0, MEM_RELEASE);
#elif FEATURE_DIRECT_SYSCALL
    (void)misra_sys2(MISRA_SYS_munmap, (long)(uintptr_t)ptr, (long)bytes);
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

// In-place resize: succeeds only when old + new sizes round to the
// same number of pages (the kernel mapping is already that big, no
// kernel work needed). Growing past the rounded boundary would need
// mremap(); on macOS we don't have it, on Linux it can fail anyway
// if a neighbouring VMA blocks growth. Either way, we don't attempt
// it -- the caller can fall back to remap, which alloc+copy+frees.
i8 page_allocator_resize(Allocator *self, void *ptr, size old_size, size new_size) {
    page_validate_self(self);
    PageAllocator *page = (PageAllocator *)self;
    (void)ptr;
    size old_rounded = page_rounded_size(page, old_size);
    size new_rounded = page_rounded_size(page, new_size);
    return old_rounded == new_rounded ? 1 : 0;
}

void *page_allocator_remap(Allocator *self, void *ptr, size old_size, size new_size) {
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

#if defined(PAGE_ALLOCATOR_WINDOWS)
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
#    if FEATURE_DIRECT_SYSCALL
    long ret = misra_sys3(MISRA_SYS_mprotect, (long)(uintptr_t)ptr, (long)bytes, (long)posix_prot);
    if (ret != 0) {
        LOG_ERROR("PageProtect: mprotect failed (errno {})", (i32)-ret);
        return false;
    }
#    else
    if (mprotect(ptr, (size_t)bytes, posix_prot) != 0) {
        // libc path (macOS / non-direct-syscall): mprotect returns -1
        // and sets errno; SYS_ERRNO falls through to reading it here.
        LOG_SYS_ERROR(SYS_ERRNO(-1), "PageProtect: mprotect failed");
        return false;
    }
#    endif
    return true;
#endif
}
