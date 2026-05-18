/// file      : std/allocator/page.c
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Page-granular allocator implementation. Routes every allocation through
/// the OS (mmap on POSIX, VirtualAlloc on Windows) so the library does not
/// depend on libc malloc here. Per-allocator state is inline: a cached
/// page size plus a descriptor table that tracks (ptr, mmap-byte-length)
/// for every live region. Free recovers the byte count from the table so
/// the caller never supplies it -- the kernel-needed size is in the
/// allocator's own bookkeeping, where wrong values are structurally
/// impossible.
///
/// The descriptor table itself is managed via `page_map` / `page_unmap`
/// directly, NOT through the public Allocator dispatch -- that would
/// recurse. The table's own mmap length is stored on the allocator
/// (entries_bytes) so Deinit can release it.

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

// ---------------------------------------------------------------------------
// Descriptor-table management. The table is itself a page-backed region
// allocated via `page_map` (not through the public Allocator dispatch --
// that would recurse). Geometric growth: first grow fills exactly one
// OS page; doublings thereafter.

static u32 page_find_idx(const PageAllocator *page, const void *ptr) {
    for (u32 i = 0; i < page->len; i++) {
        if (page->entries[i].ptr == ptr) {
            return i;
        }
    }
    return (u32)-1;
}

// Grow `entries` to hold at least one more record. Returns false on
// OS-allocation failure (the caller is responsible for unwinding any
// user mmap it just made).
static bool page_table_grow(PageAllocator *page) {
    size page_size = PageAllocatorPageSize(page);
    // First grow: one OS page worth of entries. Subsequent grows:
    // double the current allocation. Either way, page_rounded_size
    // ensures the mmap'd region is page-aligned.
    size want_bytes  = page->entries_bytes ? page->entries_bytes * 2 : page_size;
    size new_rounded = page_round_up(want_bytes, page_size);
    if (!new_rounded) {
        return false;
    }
    PageEntry *new_table = (PageEntry *)page_map(new_rounded);
    if (!new_table) {
        return false;
    }
    if (page->entries) {
        MemCopy(new_table, page->entries, (size)page->len * sizeof(PageEntry));
        page_unmap(page->entries, page->entries_bytes);
    }
    page->entries       = new_table;
    page->entries_bytes = new_rounded;
    page->cap           = (u32)(new_rounded / sizeof(PageEntry));
    return true;
}

// Remove the entry at `idx` by swapping with the last live entry.
// Order in `entries` is not load-bearing (free is a linear scan).
static void page_table_remove_at(PageAllocator *page, u32 idx) {
    page->len -= 1;
    if (idx != page->len) {
        page->entries[idx] = page->entries[page->len];
    }
}

// ---------------------------------------------------------------------------
// Public alloc / resize / remap / free.

void *page_allocator_allocate(Allocator *self, size bytes, i8 zeroed) {
    page_validate_self(self);
    (void)zeroed; // OS-mapped pages are kernel-zeroed.
    if (!bytes) {
        return NULL;
    }
    PageAllocator *page    = (PageAllocator *)self;
    size           rounded = page_rounded_size(page, bytes);
    void          *ptr     = page_map(rounded);
    if (!ptr) {
        return NULL;
    }
    if (page->len == page->cap) {
        if (!page_table_grow(page)) {
            // Table grow failed: don't strand the user mmap we just made.
            page_unmap(ptr, rounded);
            return NULL;
        }
    }
    page->entries[page->len].ptr    = ptr;
    page->entries[page->len].bytes  = rounded;
    page->len                      += 1;
    return ptr;
}

// In-place resize: succeeds only when old + new sizes round to the
// same mmap extent (the kernel mapping is already that big, no kernel
// work needed). Growing past the rounded boundary would need mremap();
// on macOS we don't have it, on Linux it can fail anyway if a
// neighbouring VMA blocks growth. Either way, we don't attempt it --
// the caller can fall back to remap, which alloc+copy+frees.
//
// `old_size` is advisory only; the authoritative byte count comes from
// the descriptor table. Passed-in `old_size` is ignored.
i8 page_allocator_resize(Allocator *self, void *ptr, size old_size, size new_size) {
    page_validate_self(self);
    PageAllocator *page = (PageAllocator *)self;
    (void)old_size;
    u32 idx = page_find_idx(page, ptr);
    if (idx == (u32)-1) {
        // Unknown pointer -- resize can't succeed without knowing the
        // real mapping length; let the caller fall back to remap.
        return 0;
    }
    size old_rounded = page->entries[idx].bytes;
    size new_rounded = page_rounded_size(page, new_size);
    return old_rounded == new_rounded ? 1 : 0;
}

void *page_allocator_remap(Allocator *self, void *ptr, size old_size, size new_size) {
    page_validate_self(self);
    PageAllocator *page = (PageAllocator *)self;
    (void)old_size;

    if (new_size == 0) {
        if (ptr) {
            (void)page_allocator_deallocate(self, ptr);
        }
        return NULL;
    }
    if (!ptr) {
        return page_allocator_allocate(self, new_size, true);
    }

    u32 idx = page_find_idx(page, ptr);
    if (idx == (u32)-1) {
        LOG_FATAL("page_remap: foreign or already-freed ptr {x}", (u64)ptr);
        return NULL;
    }
    size old_rounded = page->entries[idx].bytes;
    size new_rounded = page_rounded_size(page, new_size);
    if (old_rounded == new_rounded) {
        return ptr;
    }
    void *fresh = page_allocator_allocate(self, new_size, true);
    if (!fresh) {
        return NULL;
    }
    size copy_bytes = old_rounded < new_rounded ? old_rounded : new_rounded;
    MemCopy(fresh, ptr, copy_bytes);
    (void)page_allocator_deallocate(self, ptr);
    return fresh;
}

size page_allocator_deallocate(Allocator *self, void *ptr) {
    page_validate_self(self);
    if (!ptr) {
        return 0;
    }
    PageAllocator *page = (PageAllocator *)self;
    u32            idx  = page_find_idx(page, ptr);
    if (idx == (u32)-1) {
        LOG_FATAL("page_free: foreign or already-freed ptr {x}", (u64)ptr);
        return 0;
    }
    size bytes = page->entries[idx].bytes;
    page_unmap(ptr, bytes);
    page_table_remove_at(page, idx);
    return bytes;
}

void PageAllocatorDeinit(PageAllocator *self) {
    if (!self) {
        return;
    }
    // Anything still in `entries` is a caller leak (forgot to Free).
    // Release the kernel mappings so they don't outlive the allocator.
    for (u32 i = 0; i < self->len; i++) {
        page_unmap(self->entries[i].ptr, self->entries[i].bytes);
    }
    if (self->entries) {
        page_unmap(self->entries, self->entries_bytes);
    }
    MemSet(self, 0, sizeof(*self));
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
