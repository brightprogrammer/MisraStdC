/// file      : Sys/Mutex.c
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Cross-platform Mutex implementation that bypasses libc:
///
///   - **Linux** (x86_64 / aarch64): a Drepper-style 3-state futex
///     mutex (`0` unlocked, `1` locked uncontended, `2` locked with
///     waiters). Lock fast-path is a single atomic CAS. The slow
///     path drops into the FUTEX_WAIT syscall; unlock issues
///     FUTEX_WAKE only when there were waiters. No pthread, no libc.
///   - **macOS / BSD**: `os_unfair_lock` (`<os/lock.h>`). Apple's
///     sanctioned primitive; lives in libSystem, not libc.
///   - **Windows**: `SRWLOCK` in exclusive mode. Slim Reader-Writer
///     lock, kernel32, no init call needed (just zero-init).

#include <Misra/Sys/Mutex.h>

#include <Misra/Std.h>
#include <Misra/Std/Log.h>

#include "../_Syscall.h"

#ifdef _WIN32
#    include <windows.h>
#elif defined(__APPLE__)
#    include <os/lock.h>
#elif MISRA_HAVE_DIRECT_SYSCALL
#    include <stdatomic.h>
#    include <stdint.h>
#    define MISRA_FUTEX_WAIT_PRIVATE 128 // FUTEX_WAIT | FUTEX_PRIVATE_FLAG
#    define MISRA_FUTEX_WAKE_PRIVATE 129 // FUTEX_WAKE | FUTEX_PRIVATE_FLAG
#else
#    include <pthread.h>
#endif

struct Mutex {
#ifdef _WIN32
    SRWLOCK lock;
#elif defined(__APPLE__)
    os_unfair_lock lock;
#elif MISRA_HAVE_DIRECT_SYSCALL
    _Atomic int state; // 0 unlocked, 1 locked, 2 locked+waiters
#else
    pthread_mutex_t lock;
#endif
};

Mutex *MutexCreate(Allocator *alloc) {
    if (!alloc) {
        LOG_FATAL("MutexCreate requires an allocator");
    }
    Mutex *m = (Mutex *)AllocatorAlloc(alloc, sizeof(Mutex), true);
    if (!m) {
        LOG_ERROR("Failed to allocate mutex");
        return NULL;
    }
#ifdef _WIN32
    InitializeSRWLock(&m->lock);
#elif defined(__APPLE__)
    m->lock = (os_unfair_lock)OS_UNFAIR_LOCK_INIT;
#elif MISRA_HAVE_DIRECT_SYSCALL
    atomic_store_explicit(&m->state, 0, memory_order_relaxed);
#else
    MemSet(&m->lock, 0, sizeof(m->lock));
#endif
    return m;
}

void MutexDestroy(Mutex *m, Allocator *alloc) {
    if (!m) {
        return;
    }
    if (!alloc) {
        LOG_FATAL("MutexDestroy requires the allocator that created the mutex");
    }
#ifdef _WIN32
    // SRWLOCK has no destroy call.
#elif defined(__APPLE__)
    // os_unfair_lock has no destroy call.
#elif MISRA_HAVE_DIRECT_SYSCALL
    // futex int has no destroy call; zeroing happens below.
#else
    pthread_mutex_destroy(&m->lock);
#endif
    MemSet(m, 0, sizeof(Mutex));
    AllocatorFree(alloc, m, sizeof(Mutex));
}

Mutex *MutexLock(Mutex *m) {
#ifdef _WIN32
    AcquireSRWLockExclusive(&m->lock);
#elif defined(__APPLE__)
    os_unfair_lock_lock(&m->lock);
#elif MISRA_HAVE_DIRECT_SYSCALL
    // Fast path: 0 -> 1 (uncontended acquire).
    int expected = 0;
    if (atomic_compare_exchange_strong_explicit(&m->state, &expected, 1, memory_order_acquire, memory_order_relaxed)) {
        return m;
    }
    // Slow path: lock is held. Mark as contended (state = 2) and
    // block in FUTEX_WAIT until someone releases.
    int c = expected;
    for (;;) {
        // If state is already 2, or we successfully bumped 1 -> 2,
        // sleep until the holder releases.
        int one = 1;
        if (c == 2 ||
            atomic_compare_exchange_strong_explicit(&m->state, &one, 2, memory_order_relaxed, memory_order_relaxed)) {
            (void)misra_sys4(MISRA_SYS_futex, (long)(uintptr_t)&m->state, MISRA_FUTEX_WAIT_PRIVATE, 2, 0);
        }
        // Try to take the lock (and keep the contended marker so the
        // next unlocker will wake remaining waiters).
        int zero = 0;
        if (atomic_compare_exchange_strong_explicit(&m->state, &zero, 2, memory_order_acquire, memory_order_relaxed)) {
            return m;
        }
        c = atomic_load_explicit(&m->state, memory_order_relaxed);
    }
#else
    pthread_mutex_lock(&m->lock);
#endif
    return m;
}

Mutex *MutexUnlock(Mutex *m) {
#ifdef _WIN32
    ReleaseSRWLockExclusive(&m->lock);
#elif defined(__APPLE__)
    os_unfair_lock_unlock(&m->lock);
#elif MISRA_HAVE_DIRECT_SYSCALL
    // Fast path: if state was 1 (no waiters), atomic dec brings it to
    // 0 and we're done. Otherwise it was 2 (had waiters), zero it
    // and wake one.
    if (atomic_fetch_sub_explicit(&m->state, 1, memory_order_release) != 1) {
        atomic_store_explicit(&m->state, 0, memory_order_release);
        (void)misra_sys3(MISRA_SYS_futex, (long)(uintptr_t)&m->state, MISRA_FUTEX_WAKE_PRIVATE, 1);
    }
#else
    pthread_mutex_unlock(&m->lock);
#endif
    return m;
}
