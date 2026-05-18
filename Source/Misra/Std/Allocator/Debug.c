/// file      : std/allocator/debug.c
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// DebugAllocator implementation. The struct is init-by-value -- it
/// owns its `heap` / `meta` / `page` allocators inline and lazily
/// binds the internal Map's allocator pointer to `&self->meta.base`
/// on first use (the compound literal doesn't know the struct's
/// final address yet).

#include <Misra/Std/Allocator/Debug.h>

#include <Misra/Std.h>
#include <Misra/Std/Container/Map.h>
#include <Misra/Std/Log.h>
#include <Misra/Std/Memory.h>
#include <Misra/Sys/Backtrace.h>

#include <stdint.h>

// ---------------------------------------------------------------------------
// Per-thread unique ID. The DebugAllocator wants a value that's
// (a) unique per thread, (b) stable for the thread's lifetime,
// (c) cheap to read on the hot allocation path.
//
// On x86_64 / aarch64 the kernel already maintains a per-thread
// pointer in a CPU register (FS base on x86_64, TPIDR_EL0 on aarch64)
// for TLS. Reading it is a single instruction, requires no syscall,
// no libc, no `__tls_get_addr` helper -- the kernel sets it during
// thread creation and the scheduler preserves it across context
// switches.
//
// Two earlier implementations of this function got it wrong in
// different directions:
//   v1: `static __thread u8 g_marker; return &g_marker;` -- correct
//       and fast, but the compiler emitted `__tls_get_addr` calls
//       under the General Dynamic TLS model, pulling in glibc's
//       dynamic loader as a runtime dependency.
//   v2: `return gettid_syscall()` -- no `__tls_get_addr`, but ~50x
//       slower per call (~50-100ns vs ~1ns).
//
// This version reads the thread-pointer register directly via inline
// asm. Same speed as v1, no libc dependency. The returned value is
// the TCB self-pointer the kernel/loader stashed there -- conceptually
// the same kind of unique-per-thread address as `&g_marker` was.
//
// macOS / Windows / other-arch platforms fall back to the TLS-marker
// trick (or libSystem's TLV path implicitly). The inline-asm path is
// gated to Linux only -- Darwin uses GS (not FS) on x86_64 and
// TPIDRRO_EL0 plus libSystem TLV thunks on aarch64; reading FS:0 or
// TPIDR_EL0 there gives garbage or SIGTRAPs the process.
// ---------------------------------------------------------------------------

#if defined(__linux__) && (defined(__x86_64__) || defined(__aarch64__))

u64 debug_current_tid(void) {
    u64 tp;
#    if defined(__x86_64__)
    // fs:0 holds the TCB self-pointer on x86_64 Linux/glibc. Single
    // mov; no syscall, no helper symbol.
    __asm__ volatile("mov %%fs:0, %0"
                     : "=r"(tp));
#    else // __aarch64__
    // TPIDR_EL0 is the user-accessible thread-pointer register on
    // Linux aarch64; mrs reads it directly.
    __asm__ volatile("mrs %0, tpidr_el0"
                     : "=r"(tp));
#    endif
    return tp;
}

#else

#    if defined(_MSC_VER)
#        define MISRA_TLS __declspec(thread)
#    else
#        define MISRA_TLS __thread
#    endif

static MISRA_TLS u8 g_thread_marker;

u64 debug_current_tid(void) {
    return (u64)(uintptr_t)&g_thread_marker;
}

#endif

// ---------------------------------------------------------------------------
// Hash / compare for void* keys (extern: the Init macro stamps these
// into the embedded Map's compound literal).
// ---------------------------------------------------------------------------

u64 debug_ptr_hash(const void *data, u32 size) {
    (void)size;
    u64 x  = (u64)(uintptr_t)(*(void *const *)data);
    x     ^= x >> 30;
    x     *= 0xbf58476d1ce4e5b9ULL;
    x     ^= x >> 27;
    x     *= 0x94d049bb133111ebULL;
    x     ^= x >> 31;
    return x;
}

i32 debug_ptr_compare(const void *lhs, const void *rhs) {
    void *a = *(void *const *)lhs;
    void *b = *(void *const *)rhs;
    if ((uintptr_t)a < (uintptr_t)b)
        return -1;
    if ((uintptr_t)a > (uintptr_t)b)
        return 1;
    return 0;
}

// ---------------------------------------------------------------------------
// Canary helpers
// ---------------------------------------------------------------------------

static const u8 DEBUG_CANARY_PATTERN[4] = {0xc1, 0xd2, 0xe3, 0xf4};

static void debug_write_canary(u8 *trail, size n) {
    for (size i = 0; i < n; ++i) {
        trail[i] = DEBUG_CANARY_PATTERN[i & 3];
    }
}

static bool debug_check_canary(const u8 *trail, size n) {
    for (size i = 0; i < n; ++i) {
        if (trail[i] != DEBUG_CANARY_PATTERN[i & 3])
            return false;
    }
    return true;
}

// ---------------------------------------------------------------------------
// Validation + lazy Map allocator-pointer rebind. The Map's
// `.allocator` field is NULL inside the compound literal because the
// final struct address isn't known yet. We fix it up here on every
// entry, against the now-stable `self`.
// ---------------------------------------------------------------------------

static DebugAllocator *debug_validate_self(const Allocator *self) {
    if (!self || self->__magic != DEBUG_ALLOCATOR_MAGIC) {
        LOG_FATAL("type-confusion: allocator passed to debug_allocator_* is not a DebugAllocator");
    }
    DebugAllocator *dbg     = (DebugAllocator *)self;
    u64             cur_tid = debug_current_tid();
    if (dbg->creator_tid != cur_tid) {
        LOG_FATAL(
            "DebugAllocator: cross-thread use detected (created on {x}, called from {x}). "
            "Each thread must use its own DebugAllocator instance.",
            dbg->creator_tid,
            cur_tid
        );
    }
    if (!dbg->live.allocator) {
        dbg->live.allocator = ALLOCATOR_OF(&dbg->meta);
    }
    if (!dbg->freed.allocator) {
        dbg->freed.allocator = ALLOCATOR_OF(&dbg->meta);
    }
    return dbg;
}

// ---------------------------------------------------------------------------
// Freed history. Unbounded Vec append on every successful free.
// On a free that isn't in the live map (the underlying Heap is about
// to LOG_FATAL with a double-free / foreign-ptr diagnostic), scan
// this Vec for the ptr to emit the original alloc + first-free
// traces before Heap aborts.
//
// Memory grows with lifetime free count. Set track_freed_history =
// false in workloads that allocate/free in the millions and don't
// want the bookkeeping cost.

static const DebugFreedEntry *debug_freed_find(const DebugAllocator *dbg, void *ptr) {
    for (u32 i = 0; i < dbg->freed.length; i++) {
        if (dbg->freed.data[i].ptr == ptr)
            return &dbg->freed.data[i];
    }
    return NULL;
}

// ---------------------------------------------------------------------------
// allocate / reallocate / deallocate
// ---------------------------------------------------------------------------

void *debug_allocator_allocate(Allocator *self, size bytes, i8 zeroed) {
    DebugAllocator *dbg = debug_validate_self(self);
    if (!bytes)
        return NULL;

    size canary = dbg->config.detect_overflow ? dbg->config.canary_bytes : 0;
    size padded = bytes + canary;
    // force_page_backing routes through the internal PageAllocator;
    // normal mode through the internal HeapAllocator. Either way the
    // backing storage is owned by `self`.
    Allocator *src    = dbg->config.force_page_backing ? ALLOCATOR_OF(&dbg->page) : ALLOCATOR_OF(&dbg->heap);
    void      *user_p = AllocatorAlloc(src, padded, zeroed);
    if (!user_p)
        return NULL;

    if (canary)
        debug_write_canary((u8 *)user_p + bytes, canary);

    DebugRecord rec;
    MemSet(&rec, 0, sizeof(rec));
    rec.requested_size = bytes;
    rec.padded_size    = padded;
    if (dbg->config.capture_traces && dbg->config.trace_depth > 0) {
        u32 depth = dbg->config.trace_depth;
        if (depth > DEBUG_ALLOCATOR_MAX_TRACE)
            depth = DEBUG_ALLOCATOR_MAX_TRACE;
        rec.alloc_trace_n = (u32)CaptureStackTrace(rec.alloc_trace, depth, 2);
    }

    if (!MapInsertR(&dbg->live, user_p, rec)) {
        AllocatorFree(src, user_p, padded);
        LOG_ERROR("DebugAllocator: failed to record allocation in live map");
        return NULL;
    }
    dbg->bytes_in_use += (u64)bytes;
    return user_p;
}

static void debug_emit_trace(const StackFrame *frames, size count, const char *label, Allocator *meta) {
    if (!count) {
        LOG_ERROR("    {} trace: (none captured)", label);
        return;
    }
    Str rendered = StrInit(meta);
    FormatStackTrace(&rendered, frames, count, meta);
    LOG_ERROR("    {} trace:\n{}", label, rendered);
    StrDeinit(&rendered);
}

// Deallocate. Memory footprint of this allocator scales with the
// number of LIVE allocations only -- no per-lifetime growth from a
// "freed" map. Double-free / foreign / misaligned / wrong-size are
// caught by the underlying HeapAllocator on the forwarded free.
void debug_allocator_deallocate(Allocator *self, void *ptr, size bytes) {
    DebugAllocator *dbg = debug_validate_self(self);
    if (!ptr)
        return;

    Allocator   *src      = dbg->config.force_page_backing ? ALLOCATOR_OF(&dbg->page) : ALLOCATOR_OF(&dbg->heap);
    DebugRecord *live_rec = MapGetFirstPtr(&dbg->live, ptr);

    if (!live_rec) {
        // Pointer is not in our live set. Before forwarding to the
        // underlying allocator (which will LOG_FATAL on double-free /
        // foreign / etc.), scan the freed-history ring and emit the
        // original alloc + first-free traces if we have them. Heap's
        // abort backtrace alone shows only the current call site.
        const DebugFreedEntry *fe = debug_freed_find(dbg, ptr);
        if (fe) {
            LOG_ERROR(
                "DebugAllocator: DOUBLE FREE of {x} (originally {} bytes); original alloc + first-free traces:",
                (u64)(uintptr_t)ptr,
                (u64)fe->requested_size
            );
            debug_emit_trace(fe->alloc_trace, fe->alloc_trace_n, "alloc", ALLOCATOR_OF(&dbg->meta));
            debug_emit_trace(fe->free_trace, fe->free_trace_n, "first-free", ALLOCATOR_OF(&dbg->meta));
        }
        AllocatorFree(src, ptr, bytes);
        return;
    }

    if (bytes && bytes != live_rec->requested_size) {
        LOG_ERROR(
            "DebugAllocator: size mismatch on free of {x} (claimed {} bytes, tracked {} bytes)",
            (u64)(uintptr_t)ptr,
            (u64)bytes,
            (u64)live_rec->requested_size
        );
    }

    if (dbg->config.detect_overflow && dbg->config.canary_bytes) {
        const u8 *trail = (const u8 *)ptr + live_rec->requested_size;
        if (!debug_check_canary(trail, dbg->config.canary_bytes)) {
            dbg->overflows += 1;
            LOG_ERROR(
                "DebugAllocator: BUFFER OVERFLOW past {x} ({} bytes requested)",
                (u64)(uintptr_t)ptr,
                (u64)live_rec->requested_size
            );
            debug_emit_trace(live_rec->alloc_trace, live_rec->alloc_trace_n, "alloc", ALLOCATOR_OF(&dbg->meta));
        }
    }

    // Append to freed history BEFORE removing from live -- captures
    // alloc_trace from live_rec and a fresh free_trace from this
    // call site. Unbounded; gated by track_freed_history config.
    if (dbg->config.track_freed_history) {
        DebugFreedEntry entry;
        entry.ptr            = ptr;
        entry.requested_size = live_rec->requested_size;
        entry.alloc_trace_n  = live_rec->alloc_trace_n;
        MemCopy(entry.alloc_trace, live_rec->alloc_trace, (size)live_rec->alloc_trace_n * sizeof(StackFrame));
        entry.free_trace_n = 0;
        if (dbg->config.capture_traces && dbg->config.trace_depth > 0) {
            u32 depth = dbg->config.trace_depth;
            if (depth > DEBUG_ALLOCATOR_MAX_TRACE)
                depth = DEBUG_ALLOCATOR_MAX_TRACE;
            entry.free_trace_n = (u32)CaptureStackTrace(entry.free_trace, depth, 3);
        }
        VecPushBack(&dbg->freed, entry);
    }

    dbg->bytes_in_use -= (u64)live_rec->requested_size;
    size padded        = live_rec->padded_size;
    MapRemoveFirst(&dbg->live, ptr);

    if (dbg->config.force_page_backing) {
        // Don't release the page. mprotect it PROT_NONE so any UAF
        // read/write traps with SIGSEGV at the moment of the bug.
        size page_size = PageAllocatorPageSize(&dbg->page);
        size rounded   = (padded + page_size - 1) & ~(page_size - 1);
        if (!PageProtect(ptr, rounded, PAGE_PROT_NONE)) {
            LOG_ERROR("DebugAllocator: PageProtect(PROT_NONE) failed on {x}", (u64)(uintptr_t)ptr);
        }
    } else {
        AllocatorFree(src, ptr, padded);
    }
}

// In-place resize: always refused. The debug allocator keeps a
// canary on every allocation + a live-map keyed by pointer; trying
// to resize in place would mean re-stamping the canary, updating
// the live-map entry's recorded size, and (in page-backed mode)
// potentially remapping pages -- none of that is "in place" in any
// useful sense. Refuse and force the caller through remap, which
// does the clean alloc-fresh + copy + free dance with full canary +
// live-map maintenance.
i8 debug_allocator_resize(Allocator *self, void *ptr, size old_size, size new_size) {
    (void)debug_validate_self(self);
    (void)ptr;
    (void)old_size;
    (void)new_size;
    return 0;
}

void *debug_allocator_remap(Allocator *self, void *ptr, size old_size, size new_size) {
    DebugAllocator *dbg = debug_validate_self(self);
    if (new_size == 0) {
        debug_allocator_deallocate(self, ptr, old_size);
        return NULL;
    }
    if (!ptr) {
        return debug_allocator_allocate(self, new_size, false);
    }
    // alloc-fresh + memcpy + free, keeping canary + record invariants
    // simple. The cost in debug mode is fine.
    void *fresh = debug_allocator_allocate(self, new_size, false);
    if (!fresh)
        return NULL;
    size copy = old_size < new_size ? old_size : new_size;
    MemCopy(fresh, ptr, copy);
    debug_allocator_deallocate(self, ptr, old_size);
    (void)dbg;
    return fresh;
}

// ---------------------------------------------------------------------------
// Deinit
// ---------------------------------------------------------------------------

void DebugAllocatorDeinit(DebugAllocator *self) {
    if (!self || self->base.__magic != DEBUG_ALLOCATOR_MAGIC)
        return;

    u64 cur_tid = debug_current_tid();
    if (self->creator_tid != cur_tid) {
        LOG_FATAL(
            "DebugAllocator: Deinit called from a different thread (created on {x}, called from {x}).",
            self->creator_tid,
            cur_tid
        );
    }

    // Report leaks for anything still in `live`.
    if (self->live.allocator && self->live.length > 0) {
        LOG_ERROR("DebugAllocator: {} live allocation(s) at deinit time:", (u64)self->live.length);
        MapForeachPairPtr(&self->live, key_ptr, val_ptr) {
            LOG_ERROR("  leaked {x} ({} bytes)", (u64)(uintptr_t)*key_ptr, (u64)val_ptr->requested_size);
            debug_emit_trace(val_ptr->alloc_trace, val_ptr->alloc_trace_n, "alloc", ALLOCATOR_OF(&self->meta));
        }
    }

    // Tear down the live map (bucket storage from `&self->meta`).
    // force_page_backing'd allocations are NOT released here -- they
    // remain mprotected and the pages stay mapped until process exit.
    // Acceptable: force_page_backing is opt-in for short-running tests
    // / fuzz, the memory cost is the documented trade-off.
    if (self->live.allocator)
        MapDeinit(&self->live);
    if (self->freed.allocator)
        VecDeinit(&self->freed);

    HeapAllocatorDeinit(&self->meta);
    HeapAllocatorDeinit(&self->heap);
    // PageAllocator has no per-instance state; nothing to deinit.

    MemSet(self, 0, sizeof(*self));
}

// ---------------------------------------------------------------------------
// Public query / report API
// ---------------------------------------------------------------------------

size DebugAllocatorLiveCount(const DebugAllocator *self) {
    if (!self)
        return 0;
    return (size)self->live.length;
}

size DebugAllocatorLiveBytes(const DebugAllocator *self) {
    if (!self)
        return 0;
    return (size)self->bytes_in_use;
}

size DebugAllocatorOverflows(const DebugAllocator *self) {
    if (!self)
        return 0;
    return (size)self->overflows;
}

void DebugAllocatorReportLeaks(DebugAllocator *self, Str *out) {
    if (!self || !out)
        return;
    if (self->live.length == 0)
        return;

    StrWriteFmt(out, "DebugAllocator: {} live allocation(s):\n", (u64)self->live.length);
    MapForeachPairPtr(&self->live, key_ptr, val_ptr) {
        StrWriteFmt(out, "  leak: {x} ({} bytes)\n", (u64)(uintptr_t)*key_ptr, (u64)val_ptr->requested_size);
        if (val_ptr->alloc_trace_n > 0) {
            FormatStackTrace(out, val_ptr->alloc_trace, val_ptr->alloc_trace_n, ALLOCATOR_OF(&self->meta));
        }
    }
}
