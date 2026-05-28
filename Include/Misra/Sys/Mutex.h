/// file      : sys/mutex.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Cross-platform mutex. Stack-declared by value via the `MutexInit()`
/// macro; per-platform backends are exposed in the struct so callers
/// don't need a heap allocation. See `Source/Misra/Sys/Mutex.c` for
/// the backend selection rationale (futex on Linux, __ulock on macOS,
/// SRWLOCK on Windows).

#ifndef MISRA_SYS_MUTEX_H
#define MISRA_SYS_MUTEX_H

#include <Misra/Std/Allocator.h>
#include <Misra/Types.h>

// We deliberately do NOT include <windows.h> in this public header.
// Pulling it from a header that's transitively included by Sys.h ->
// File.h -> everywhere drags ~thousands of Win32 macros (e.g.
// IMAGE_DEBUG_TYPE_CODEVIEW) into TUs that have their own enums by
// the same name -- breaks unrelated code. Instead, use a layout-
// compatible opaque field and cast inside Mutex.c.
#if FEATURE_DIRECT_SYSCALL
#    include <stdatomic.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

    ///
    /// Mutex struct. Layout is platform-conditional but the API is
    /// uniform. Stack-declare with `MutexInit()`; do not poke fields.
    ///
    typedef struct Mutex {
#if PLATFORM_WINDOWS
        // Layout-compatible with Windows SRWLOCK = `struct { PVOID Ptr; }`.
        // Mutex.c casts &_lock to (SRWLOCK *) for Win32 calls. Zero-init
        // = SRWLOCK_INIT = unlocked.
        void *_lock;
#elif FEATURE_DIRECT_SYSCALL
    // Drepper-style 3-state mutex backed by futex (Linux) /
    // __ulock_wait (Darwin). 0 = unlocked, 1 = locked, 2 = locked
    // with waiters.
    _Atomic int _state;
#else
#    error "Mutex: unsupported platform/architecture (no direct-syscall path)"
#endif
    } Mutex;

///
/// Initialize a `Mutex`. Expands to a designated-initialiser struct
/// literal -- no syscall, no allocation, can't fail. Usage:
///
///     Mutex m = MutexInit();
///     MutexLock(&m);
///     ...
///     MutexUnlock(&m);
///     MutexDeinit(&m);
///
/// SUCCESS : Returns a usable, unlocked `Mutex` by value.
/// FAILURE : Function cannot fail (pure literal expansion).
///
/// TAGS: Sys, Mutex, Init
///
#if PLATFORM_WINDOWS
// _lock is a void* layout-compatible with SRWLOCK. NULL = SRWLOCK_INIT.
#    define MutexInit() ((Mutex) {._lock = NULL})
#elif FEATURE_DIRECT_SYSCALL
// _Atomic int = 0 is "unlocked" in the futex / __ulock state machine.
#    define MutexInit() ((Mutex) {._state = 0})
#endif

    ///
    /// Tear down a mutex. The kernel holds no per-mutex resource on
    /// either backend (SRWLOCK has no destroy; futex/__ulock are just
    /// addresses watched on demand), so the call only zeroes the struct.
    ///
    /// SUCCESS : Returns to the caller. `*m` is zeroed.
    /// FAILURE : Function cannot fail. NULL `m` is a no-op.
    ///
    /// TAGS: Sys, Mutex, Deinit
    ///
    void MutexDeinit(Mutex *m);

    ///
    /// Acquire the lock. Blocks until the lock is available.
    ///
    /// SUCCESS: Returns `m` (locked).
    /// FAILURE: Doesn't return -- aborts on NULL.
    ///
    /// TAGS: Sys, Mutex, Lock
    ///
    Mutex *MutexLock(Mutex *m);

    ///
    /// Release the lock. Wakes one waiter on the direct-syscall paths.
    ///
    /// SUCCESS: Returns `m` (unlocked).
    /// FAILURE: Doesn't return -- aborts on NULL.
    ///
    /// TAGS: Sys, Mutex, Unlock
    ///
    Mutex *MutexUnlock(Mutex *m);

#ifdef __cplusplus
}
#endif

#endif // MISRA_SYS_MUTEX_H
