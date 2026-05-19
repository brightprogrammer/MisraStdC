/// file      : Sys/Mutex.c
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Cross-platform Mutex implementation that bypasses libc:
///
///   - **Linux** (x86_64 / aarch64): Drepper-style 3-state futex
///     mutex (`0` unlocked, `1` locked uncontended, `2` locked with
///     waiters). Lock fast-path is a single atomic CAS. Slow path
///     drops into FUTEX_WAIT; unlock issues FUTEX_WAKE only when
///     there were waiters.
///   - **macOS** (x86_64 / aarch64): same 3-state algorithm, but
///     the kernel primitive is XNU's `__ulock_wait` (#515) /
///     `__ulock_wake` (#516) with op = `UL_COMPARE_AND_WAIT`. That's
///     the same primitive Apple's own `os_unfair_lock` is built on
///     top of inside libSystem -- but going to it directly drops
///     the libSystem dependency. Same code path as Linux otherwise.
///   - **Windows**: `SRWLOCK` in exclusive mode. Slim Reader-Writer
///     lock, kernel32, no init call needed (just zero-init).

#include <Misra/Sys/Mutex.h>

#include <Misra/Std.h>
#include <Misra/Std/Log.h>

#include "../_Syscall.h"

#ifdef _WIN32
#    include <windows.h>
#elif FEATURE_DIRECT_SYSCALL
#    include <stdatomic.h>
#    if defined(__APPLE__)
// XNU __ulock op codes. We want plain 32-bit compare-and-wait,
// process-local. ULF_NO_ERRNO would make the kernel return -errno
// instead of setting libSystem's errno -- not strictly necessary
// here because we ignore the return value, but flipping it on
// keeps any future code that checks the return free of libSystem
// errno reads.
#        define MISRA_UL_COMPARE_AND_WAIT 1
#        define MISRA_ULF_NO_ERRNO        0x1000000
#    else
#        define FUTEX_WAIT_PRIVATE 128 // FUTEX_WAIT | FUTEX_PRIVATE_FLAG
#        define FUTEX_WAKE_PRIVATE 129 // FUTEX_WAKE | FUTEX_PRIVATE_FLAG
#    endif
#else
#    include <pthread.h>
#endif

// struct Mutex lives in <Misra/Sys/Mutex.h> -- exposed so callers can
// stack-declare and initialise via the MutexInit() macro. Header field
// names are prefixed with `_` to flag them as implementation detail.

#if FEATURE_DIRECT_SYSCALL
// Per-OS wrappers around the underlying compare-and-wait / wake
// primitive. Same semantics: wait sleeps iff *addr == expected;
// wake_one releases at most one waiter.
static inline void mutex_wait(_Atomic int *addr, int expected) {
#    if defined(__APPLE__)
    // __ulock_wait(op_and_flags, addr, value, timeout_us=0=infinite)
    (void)misra_sys4(
        MISRA_SYS___ulock_wait,
        (long)(MISRA_UL_COMPARE_AND_WAIT | MISRA_ULF_NO_ERRNO),
        (long)(u64)addr,
        (long)expected,
        0
    );
#    else
    // futex(addr, FUTEX_WAIT_PRIVATE, val=expected, timeout=NULL)
    (void)misra_sys4(MISRA_SYS_futex, (long)(u64)addr, FUTEX_WAIT_PRIVATE, expected, 0);
#    endif
}

static inline void mutex_wake_one(_Atomic int *addr) {
#    if defined(__APPLE__)
    // __ulock_wake(op_and_flags, addr, wake_value=0) -- with
    // UL_COMPARE_AND_WAIT alone (no ULF_WAKE_ALL) the kernel wakes
    // exactly one waiter, same as FUTEX_WAKE with val=1.
    (
        void
    )misra_sys3(MISRA_SYS___ulock_wake, (long)(MISRA_UL_COMPARE_AND_WAIT | MISRA_ULF_NO_ERRNO), (long)(u64)addr, 0);
#    else
    // futex(addr, FUTEX_WAKE_PRIVATE, val=1) -- wake at most one.
    (void)misra_sys3(MISRA_SYS_futex, (long)(u64)addr, FUTEX_WAKE_PRIVATE, 1);
#    endif
}
#endif

void MutexDeinit(Mutex *m) {
    if (!m) {
        return;
    }
#ifdef _WIN32
    // SRWLOCK has no destroy call.
#elif FEATURE_DIRECT_SYSCALL
    // futex/ulock int has no destroy call; zeroing happens below.
#else
    pthread_mutex_destroy(&m->_lock);
#endif
    MemSet(m, 0, sizeof(Mutex));
}

Mutex *MutexLock(Mutex *m) {
#ifdef _WIN32
    // Cast through (SRWLOCK *) -- the header keeps `_lock` as a
    // bare void* so it doesn't have to pull <windows.h>. SRWLOCK is
    // layout-compatible with a single PVOID.
    AcquireSRWLockExclusive((SRWLOCK *)&m->_lock);
#elif FEATURE_DIRECT_SYSCALL
    // Fast path: 0 -> 1 (uncontended acquire).
    int expected = 0;
    if (atomic_compare_exchange_strong_explicit(&m->_state, &expected, 1, memory_order_acquire, memory_order_relaxed)) {
        return m;
    }
    // Slow path: lock is held. Mark as contended (state = 2) and
    // block in the kernel until someone releases.
    int c = expected;
    for (;;) {
        // If state is already 2, or we successfully bumped 1 -> 2,
        // sleep until the holder releases.
        int one = 1;
        if (c == 2 ||
            atomic_compare_exchange_strong_explicit(&m->_state, &one, 2, memory_order_relaxed, memory_order_relaxed)) {
            mutex_wait(&m->_state, 2);
        }
        // Try to take the lock (and keep the contended marker so the
        // next unlocker will wake remaining waiters).
        int zero = 0;
        if (atomic_compare_exchange_strong_explicit(&m->_state, &zero, 2, memory_order_acquire, memory_order_relaxed)) {
            return m;
        }
        c = atomic_load_explicit(&m->_state, memory_order_relaxed);
    }
#else
    pthread_mutex_lock(&m->_lock);
#endif
    return m;
}

Mutex *MutexUnlock(Mutex *m) {
#ifdef _WIN32
    ReleaseSRWLockExclusive((SRWLOCK *)&m->_lock);
#elif FEATURE_DIRECT_SYSCALL
    // Fast path: if state was 1 (no waiters), atomic dec brings it to
    // 0 and we're done. Otherwise it was 2 (had waiters), zero it
    // and wake one.
    if (atomic_fetch_sub_explicit(&m->_state, 1, memory_order_release) != 1) {
        atomic_store_explicit(&m->_state, 0, memory_order_release);
        mutex_wake_one(&m->_state);
    }
#else
    pthread_mutex_unlock(&m->_lock);
#endif
    return m;
}
