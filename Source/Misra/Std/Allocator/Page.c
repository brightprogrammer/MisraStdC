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
/// The descriptor table itself is managed via `os_page_map` / `os_page_unmap`
/// directly, NOT through the public Allocator dispatch -- that would
/// recurse. The table's own mmap length is stored on the allocator
/// (entries_bytes) so Deinit can release it.

#include <Misra/Std/Allocator/Page.h>
#include <Misra/Std/Log.h>
#include <Misra/Std/Memory.h>
#include <Misra/Sys.h>

#include "_Os.h"

#if PLATFORM_WINDOWS
#    define PAGE_ALLOCATOR_WINDOWS 1
#    include <windows.h>
#else
#    include <sys/mman.h>
#    include "../../_Syscall.h"
#endif


// Relational invariants beyond the type-confusion magic check.
//
// The descriptor table is a Vec-shaped (ptr, len, cap) triple. Allowed
// states are exactly two: (NULL, 0, 0) for freshly-initialized or
// post-deinit, and (non-NULL, 0..cap, > 0) once any region has been
// allocated. `entries_bytes` records the rounded mmap length of the
// table itself so PageAllocatorDeinit can unmap it -- it's zero
// exactly when `entries` is NULL.
static void page_validate_self(const PageAllocator *pg) {
    if (!pg) {
        LOG_FATAL("PageAllocator: NULL self");
    }
    if (pg->base.__magic != PAGE_ALLOCATOR_MAGIC) {
        LOG_FATAL("type-confusion: allocator passed to page_allocator_* is not a PageAllocator");
    }
    if (!pg->base.allocate || !pg->base.resize || !pg->base.remap || !pg->base.deallocate) {
        LOG_FATAL("PageAllocator: vtable function pointer is NULL");
    }
    if (pg->base.alignment == 0 || (pg->base.alignment & (pg->base.alignment - 1)) != 0) {
        LOG_FATAL("PageAllocator: alignment {} is not a positive power of two", (u64)pg->base.alignment);
    }
    if (pg->len > pg->cap) {
        LOG_FATAL("PageAllocator: len {} exceeds cap {}", (u64)pg->len, (u64)pg->cap);
    }
    if ((pg->entries == NULL) != (pg->cap == 0)) {
        LOG_FATAL("PageAllocator: entries / cap mismatch (entries={x}, cap={})", (u64)pg->entries, (u64)pg->cap);
    }
    if (pg->len > 0 && !pg->entries) {
        LOG_FATAL("PageAllocator: len {} with NULL entries", (u64)pg->len);
    }
    if ((pg->entries == NULL) != (pg->entries_bytes == 0)) {
        LOG_FATAL("PageAllocator: entries / entries_bytes mismatch");
    }
    if (pg->entries && pg->entries_bytes < (size)pg->cap * sizeof(PageEntry)) {
        LOG_FATAL(
            "PageAllocator: entries_bytes {} too small for cap {} (need {})",
            (u64)pg->entries_bytes,
            (u64)pg->cap,
            (u64)((size)pg->cap * sizeof(PageEntry))
        );
    }
    // Force-read first byte of the descriptor table so a freed mapping
    // faults at the validate site, not downstream.
    if (pg->entries) {
        (void)(*(const volatile u8 *)(const void *)pg->entries);
    }
    // Same invariants for the retention table.
    if (pg->free_len > pg->free_cap) {
        LOG_FATAL("PageAllocator: free_len {} exceeds free_cap {}", (u64)pg->free_len, (u64)pg->free_cap);
    }
    if ((pg->free_entries == NULL) != (pg->free_cap == 0)) {
        LOG_FATAL("PageAllocator: free_entries / free_cap mismatch ({x} / {})",
                  (u64)pg->free_entries, (u64)pg->free_cap);
    }
    if (pg->free_len > 0 && !pg->free_entries) {
        LOG_FATAL("PageAllocator: free_len {} with NULL free_entries", (u64)pg->free_len);
    }
    if ((pg->free_entries == NULL) != (pg->free_entries_bytes == 0)) {
        LOG_FATAL("PageAllocator: free_entries / free_entries_bytes mismatch");
    }
    if (pg->free_entries) {
        (void)(*(const volatile u8 *)(const void *)pg->free_entries);
    }
}

size PageAllocatorPageSize(PageAllocator *self) {
    if (self && self->cached_page_size) {
        return self->cached_page_size;
    }
    size ps = os_page_size();
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

static size page_rounded_size(PageAllocator *self, size bytes) {
    size align     = page_effective_alignment(self);
    size page_size = PageAllocatorPageSize(self);
    if (align > page_size) {
        return ALIGN_UP_POW2(bytes, align);
    }
    return ALIGN_UP_POW2(bytes, page_size);
}

// ---------------------------------------------------------------------------
// Descriptor-table management. The table is itself a page-backed region
// allocated via `os_page_map` (not through the public Allocator dispatch --
// that would recurse). Geometric growth: first grow fills exactly one
// OS page; doublings thereafter.

// Binary search a sorted-by-ptr table for `ptr`. Returns (u32)-1 on miss.
// Both `entries` and `free_entries` use the same sort order so the same
// helper works for live-region lookup (foreign-ptr check) and for
// retained-region lookup (double-free check).
static u32 page_find_idx_sorted(const PageEntry *arr, u32 len, const void *ptr) {
    u32 lo = 0, hi = len;
    while (lo < hi) {
        u32 mid = lo + (hi - lo) / 2u;
        if ((u64)arr[mid].ptr < (u64)ptr) {
            lo = mid + 1u;
        } else {
            hi = mid;
        }
    }
    if (lo < len && arr[lo].ptr == ptr) {
        return lo;
    }
    return (u32)-1;
}

// Legacy name retained for the resize / remap paths that check the live
// table only.
static u32 page_find_idx(const PageAllocator *page, const void *ptr) {
    return page_find_idx_sorted(page->entries, page->len, ptr);
}

// Grow a Vec-shape (arr, len, cap, mmap_bytes) by doubling. First grow
// fills one OS page; subsequent grows double. Caller supplies all four
// field pointers so the same helper grows either `entries` or
// `free_entries`. Returns false on OS-allocation failure or overflow.
static bool page_table_grow_into(PageAllocator *page,
                                 PageEntry **arr_p, u32 *len_p, u32 *cap_p, size *bytes_p) {
    size page_size = PageAllocatorPageSize(page);
    size want_bytes;
    if (*bytes_p == 0) {
        want_bytes = page_size;
    } else if (*bytes_p > ((size)-1) / 2) {
        return false;
    } else {
        want_bytes = *bytes_p * 2;
    }
    size new_rounded = ALIGN_UP_POW2(want_bytes, page_size);
    if (!new_rounded) {
        return false;
    }
    PageEntry *new_table = (PageEntry *)os_page_map(new_rounded);
    if (!new_table) {
        return false;
    }
    if (*arr_p) {
        MemCopy(new_table, *arr_p, (size)*len_p * sizeof(PageEntry));
        os_page_unmap(*arr_p, *bytes_p);
    }
    *arr_p   = new_table;
    *bytes_p = new_rounded;
    *cap_p   = (u32)(new_rounded / sizeof(PageEntry));
    return true;
}

// Insert (ptr, bytes) into a sorted-by-ptr table at the right position.
// Grows the table if full. Returns false on grow failure.
static bool page_table_insert_sorted(PageAllocator *page,
                                     PageEntry **arr_p, u32 *len_p, u32 *cap_p, size *bytes_p,
                                     void *ptr, size bytes) {
    if (*len_p == *cap_p && !page_table_grow_into(page, arr_p, len_p, cap_p, bytes_p)) {
        return false;
    }
    PageEntry *arr = *arr_p;
    u32        lo  = 0;
    u32        hi  = *len_p;
    while (lo < hi) {
        u32 mid = lo + (hi - lo) / 2u;
        if ((u64)arr[mid].ptr < (u64)ptr) {
            lo = mid + 1u;
        } else {
            hi = mid;
        }
    }
    u32 ins      = lo;
    u32 to_move  = *len_p - ins;
    if (to_move > 0u) {
        MemMove(&arr[ins + 1u], &arr[ins], (size)to_move * sizeof(PageEntry));
    }
    arr[ins].ptr   = ptr;
    arr[ins].bytes = bytes;
    *len_p += 1u;
    return true;
}

// Remove the entry at `idx` while preserving sort order: shift the tail
// down by one. Order in both `entries` and `free_entries` IS load-bearing
// because we binary-search both.
static void page_table_remove_sorted_at(PageEntry *arr, u32 *len_p, u32 idx) {
    *len_p -= 1u;
    u32 to_move = *len_p - idx;
    if (to_move > 0u) {
        MemMove(&arr[idx], &arr[idx + 1u], (size)to_move * sizeof(PageEntry));
    }
}

// Binary search free_entries[] (sorted by `bytes` ascending) for an
// entry with exactly `bytes`. Returns (u32)-1 on miss.
// Exact-match policy: don't return a 64 KiB entry for a 16 KiB request
// (would track the wrong size and waste memory on the next free).
static u32 page_free_find_size_match(const PageAllocator *page, size bytes) {
    u32 lo = 0, hi = page->free_len;
    while (lo < hi) {
        u32 mid = lo + (hi - lo) / 2u;
        if (page->free_entries[mid].bytes < bytes) {
            lo = mid + 1u;
        } else {
            hi = mid;
        }
    }
    if (lo < page->free_len && page->free_entries[lo].bytes == bytes) {
        return lo;
    }
    return (u32)-1;
}

// Insert (ptr, bytes) into free_entries[] sorted by `bytes` ascending.
// Grows the table if full. Returns false on grow failure (caller falls
// back to immediate munmap rather than leak the live entry).
static bool page_free_insert_sorted(PageAllocator *page, void *ptr, size bytes) {
    if (page->free_len == page->free_cap &&
        !page_table_grow_into(page, &page->free_entries, &page->free_len,
                              &page->free_cap, &page->free_entries_bytes)) {
        return false;
    }
    PageEntry *arr = page->free_entries;
    u32        lo  = 0;
    u32        hi  = page->free_len;
    while (lo < hi) {
        u32 mid = lo + (hi - lo) / 2u;
        if (arr[mid].bytes < bytes) {
            lo = mid + 1u;
        } else {
            hi = mid;
        }
    }
    u32 ins     = lo;
    u32 to_move = page->free_len - ins;
    if (to_move > 0u) {
        MemMove(&arr[ins + 1u], &arr[ins], (size)to_move * sizeof(PageEntry));
    }
    arr[ins].ptr   = ptr;
    arr[ins].bytes = bytes;
    page->free_len += 1u;
    return true;
}

// ---------------------------------------------------------------------------
// Public alloc / resize / remap / free.

void *page_allocator_allocate(PageAllocator *self, size bytes, i8 zeroed) {
    page_validate_self(self);
    if (!bytes) {
        return NULL;
    }
    size rounded = page_rounded_size(self, bytes);

    // Reuse first: if we've previously handed out a region of exactly
    // this rounded size and the user freed it, take it back instead of
    // going to the kernel. The freed page's bytes have whatever the
    // user last wrote into them (page contents are never altered while
    // retained -- allocator data lives in the sibling free_entries[]
    // table). If the caller asked for zeroed memory, we explicitly
    // zero the page here; a fresh mmap from the kernel is already
    // zero, so the non-reuse path skips this.
    u32 hit = page_free_find_size_match(self, rounded);
    if (hit != (u32)-1) {
        void *ptr = self->free_entries[hit].ptr;
        if (!page_table_insert_sorted(self, &self->entries, &self->len, &self->cap,
                                      &self->entries_bytes, ptr, rounded)) {
            // Failed to register in the live table; leave the entry on
            // the free list and fall through to mmap.
        } else {
            page_table_remove_sorted_at(self->free_entries, &self->free_len, hit);
            if (zeroed) {
                MemSet(ptr, 0, rounded);
            }
            return ptr;
        }
    }

    // Cache miss -- ask the kernel.
    void *ptr = os_page_map(rounded);
    if (!ptr) {
        return NULL;
    }
    if (!page_table_insert_sorted(self, &self->entries, &self->len, &self->cap,
                                  &self->entries_bytes, ptr, rounded)) {
        // Table grow failed: don't strand the user mmap we just made.
        os_page_unmap(ptr, rounded);
        return NULL;
    }
    // OS-mapped pages are kernel-zeroed, no need to honour `zeroed`
    // explicitly on this path.
    (void)zeroed;
    return ptr;
}

// In-place resize: succeeds only when old + new sizes round to the
// same mmap extent (the kernel mapping is already that big, no kernel
// work needed). Growing past the rounded boundary would need mremap();
// on macOS we don't have it, on Linux it can fail anyway if a
// neighbouring VMA blocks growth. Either way, we don't attempt it --
// the caller can fall back to remap, which alloc+copy+frees.
i8 page_allocator_resize(PageAllocator *self, void *ptr, size new_size) {
    page_validate_self(self);
    u32 idx = page_find_idx(self, ptr);
    if (idx == (u32)-1) {
        // Unknown pointer -- resize can't succeed without knowing the
        // real mapping length; let the caller fall back to remap.
        return 0;
    }
    size old_rounded = self->entries[idx].bytes;
    size new_rounded = page_rounded_size(self, new_size);
    return old_rounded == new_rounded ? 1 : 0;
}

void *page_allocator_remap(PageAllocator *self, void *ptr, size new_size) {
    page_validate_self(self);

    if (new_size == 0) {
        if (ptr) {
            (void)page_allocator_deallocate(self, ptr);
        }
        return NULL;
    }
    if (!ptr) {
        return page_allocator_allocate(self, new_size, true);
    }

    u32 idx = page_find_idx(self, ptr);
    if (idx == (u32)-1) {
        LOG_FATAL("page_remap: foreign or already-freed ptr {x}", (u64)ptr);
        return NULL;
    }
    size old_rounded = self->entries[idx].bytes;
    size new_rounded = page_rounded_size(self, new_size);
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

size page_allocator_deallocate(PageAllocator *self, void *ptr) {
    page_validate_self(self);
    if (!ptr) {
        return 0;
    }
    u32 idx = page_find_idx(self, ptr);
    if (idx == (u32)-1) {
        // Missing from entries[] -- either foreign or already on the
        // free_entries[] retention list. We don't distinguish: the
        // free table is sorted by size for fast alloc-side lookup, so
        // ptr-based bsearch isn't available there. Combined error is
        // what this function already reported pre-retention.
        LOG_FATAL("page_free: foreign or already-freed ptr {x}", (u64)ptr);
        return 0;
    }
    size bytes = self->entries[idx].bytes;
    // Retention: move the entry from entries[] to free_entries[]
    // rather than munmap. Next allocate() of this rounded size will
    // pop it back without a syscall. The freed page's bytes are NOT
    // touched -- allocator state lives in the sibling table.
    if (!page_free_insert_sorted(self, ptr, bytes)) {
        // Free-list grow failed: fall back to munmap so we don't strand
        // the mapping. Rare path (only on extreme address-space
        // pressure for the free-list metadata buffer itself).
        os_page_unmap(ptr, bytes);
    }
    page_table_remove_sorted_at(self->entries, &self->len, idx);
    return bytes;
}

size PageAllocatorFootprintBytes(const PageAllocator *self) {
    if (!self) {
        return 0;
    }
    // Live regions still owned by the user.
    size total = 0;
    for (u32 i = 0; i < self->len; i++) {
        total += self->entries[i].bytes;
    }
    // Retained regions: user freed them, but the allocator's release
    // policy keeps the mmap around until Deinit so the kernel still
    // counts them as part of this process's address space.
    for (u32 i = 0; i < self->free_len; i++) {
        total += self->free_entries[i].bytes;
    }
    // The descriptor tables themselves are page-mapped via os_page_map
    // (see page_table_grow_into) and live for the allocator's lifetime,
    // so they belong in the kernel-visible footprint too.
    total += self->entries_bytes;
    total += self->free_entries_bytes;
    return total;
}

void PageAllocatorDeinit(PageAllocator *self) {
    if (!self) {
        return;
    }
    // Anything still in `entries` is a caller leak (forgot to Free).
    // Release the kernel mappings so they don't outlive the allocator.
    for (u32 i = 0; i < self->len; i++) {
        os_page_unmap(self->entries[i].ptr, self->entries[i].bytes);
    }
    // Retained-but-unowned regions: deinit is the only point where the
    // OS gets these pages back. Without this loop the process would
    // leak every region the allocator had ever held.
    for (u32 i = 0; i < self->free_len; i++) {
        os_page_unmap(self->free_entries[i].ptr, self->free_entries[i].bytes);
    }
    if (self->entries) {
        os_page_unmap(self->entries, self->entries_bytes);
    }
    if (self->free_entries) {
        os_page_unmap(self->free_entries, self->free_entries_bytes);
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
    long ret = misra_sys3(MISRA_SYS_mprotect, (long)(u64)ptr, (long)bytes, (long)posix_prot);
    if (ret != 0) {
        LOG_SYS_ERROR(ErrnoOf((i32)ret), "PageProtect: mprotect failed");
        return false;
    }
#    else
    if (mprotect(ptr, (size_t)bytes, posix_prot) != 0) {
        // libc path (macOS / non-direct-syscall): mprotect returns -1
        // and the system error is recovered through ErrnoOf below.
        LOG_SYS_ERROR(ErrnoOf(-1), "PageProtect: mprotect failed");
        return false;
    }
#    endif
    return true;
#endif
}
