/// file      : std/allocator/page.c
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Page-granular allocator implementation. Routes every allocation through
/// the OS (mmap on POSIX, VirtualAlloc on Windows) so the library does not
/// depend on libc malloc here.

#include <Misra/Std/Allocator/Page.h>
#include <Misra/Std/Memory.h>

#include <stddef.h>

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

static size cached_page_size = 0;

static size page_allocator_query_page_size(void) {
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

size PageAllocatorPageSize(void) {
    if (!cached_page_size) {
        cached_page_size = page_allocator_query_page_size();
    }
    return cached_page_size;
}

static bool page_alignment_is_pow2(size alignment) {
    return alignment != 0 && ((alignment & (alignment - 1)) == 0);
}

static size page_allocator_effective_alignment(const Allocator *alloc) {
    size requested = alloc ? alloc->alignment : 0;
    size page_size = PageAllocatorPageSize();
    if (requested < page_size) {
        return page_size;
    }
    if (!page_alignment_is_pow2(requested)) {
        // Fall back to page size if caller asked for non-pow2.
        return page_size;
    }
    return requested;
}

static size page_allocator_round_up(size bytes, size page_size) {
    if (!bytes) {
        return 0;
    }
    return (bytes + page_size - 1) & ~(page_size - 1);
}

static void *page_allocator_map(size bytes) {
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

static void page_allocator_unmap(void *ptr, size bytes) {
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

static void *page_allocator_allocate(Allocator *alloc, size bytes, bool zeroed) {
    size  page_size = PageAllocatorPageSize();
    size  rounded;
    void *ptr;

    (void)alloc;
    (void)zeroed; // OS-mapped pages are zeroed by the kernel.

    if (!bytes) {
        return NULL;
    }

    // Honor stricter alignment by simply rounding up to the alignment, which is
    // always >= page_size after page_allocator_effective_alignment().
    size align = page_allocator_effective_alignment(alloc);
    if (align > page_size) {
        // Over-aligned allocations: just round size up to the alignment too.
        rounded = (bytes + align - 1) & ~(align - 1);
    } else {
        rounded = page_allocator_round_up(bytes, page_size);
    }

    ptr = page_allocator_map(rounded);
    if (!ptr) {
        return NULL;
    }
    return ptr;
}

static void *page_allocator_reallocate(Allocator *alloc, void *ptr, size old_size, size new_size) {
    void *new_ptr;
    size  copy_bytes;

    if (new_size == 0) {
        size align     = page_allocator_effective_alignment(alloc);
        size page_size = PageAllocatorPageSize();
        size rounded =
            align > page_size ? ((old_size + align - 1) & ~(align - 1)) : page_allocator_round_up(old_size, page_size);
        page_allocator_unmap(ptr, rounded);
        return NULL;
    }

    new_ptr = page_allocator_allocate(alloc, new_size, true);
    if (!new_ptr) {
        return NULL;
    }

    if (ptr) {
        copy_bytes = old_size < new_size ? old_size : new_size;
        MemCopy(new_ptr, ptr, copy_bytes);

        size align     = page_allocator_effective_alignment(alloc);
        size page_size = PageAllocatorPageSize();
        size rounded =
            align > page_size ? ((old_size + align - 1) & ~(align - 1)) : page_allocator_round_up(old_size, page_size);
        page_allocator_unmap(ptr, rounded);
    }

    return new_ptr;
}

static void page_allocator_deallocate(Allocator *alloc, void *ptr, size bytes) {
    size align     = page_allocator_effective_alignment(alloc);
    size page_size = PageAllocatorPageSize();
    size rounded = align > page_size ? ((bytes + align - 1) & ~(align - 1)) : page_allocator_round_up(bytes, page_size);
    page_allocator_unmap(ptr, rounded);
}

Allocator PageAllocator(void) {
    return (Allocator) {
        .state        = NULL,
        .state_init   = NULL,
        .state_deinit = NULL,
        .allocate     = page_allocator_allocate,
        .reallocate   = page_allocator_reallocate,
        .deallocate   = page_allocator_deallocate,
        .effort       = ALLOCATOR_EFFORT_ONCE,
        .retry_limit  = 0,
        .flags        = 0,
        .alignment    = 1,
    };
}

Allocator PageAllocatorAligned(size alignment) {
    Allocator alloc = PageAllocator();
    if (alignment) {
        alloc.alignment = alignment;
    }
    return alloc;
}
