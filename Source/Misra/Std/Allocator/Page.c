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
#    include <unistd.h>
#    if !defined(MAP_ANONYMOUS) && defined(MAP_ANON)
#        define MAP_ANONYMOUS MAP_ANON
#    endif
#endif

static size page_query_page_size(void) {
#if defined(MISRA_PAGE_ALLOCATOR_WINDOWS)
    SYSTEM_INFO info;
    GetSystemInfo(&info);
    if (!info.dwPageSize) {
        return 4096;
    }
    return (size)info.dwPageSize;
#else
    long ps = sysconf(_SC_PAGESIZE);
    if (ps <= 0) {
        return 4096;
    }
    return (size)ps;
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

static void *page_map(size bytes) {
#if defined(MISRA_PAGE_ALLOCATOR_WINDOWS)
    return VirtualAlloc(NULL, (SIZE_T)bytes, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
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
