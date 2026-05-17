/// file      : Misra/Sys/Mutex.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Cross-platform mutex. Stack-declared by value via the `MutexInit()`
/// macro; per-platform backends are exposed in the struct so callers
/// don't need a heap allocation. See `Source/Misra/Sys/Mutex.c` for
/// the backend selection rationale (futex on Linux, __ulock on macOS,
/// SRWLOCK on Windows, pthread_mutex_t on other POSIX).

#ifndef MISRA_SYS_MUTEX_H
#define MISRA_SYS_MUTEX_H

#include <Misra/Std/Allocator.h>
#include <Misra/Types.h>

#ifdef _WIN32
#    define _WIN32_LEAN_AND_MEAN
#    include <windows.h>
#elif defined(__linux__) || defined(__APPLE__)
#    include <stdatomic.h>
#else
#    include <pthread.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

///
/// Mutex struct. Layout is platform-conditional but the API is
/// uniform. Stack-declare with `MutexInit()`; do not poke fields.
///
typedef struct Mutex {
#ifdef _WIN32
    SRWLOCK _lock;
#elif defined(__linux__) || defined(__APPLE__)
    // Drepper-style 3-state mutex backed by futex (Linux) /
    // __ulock_wait (Darwin). 0 = unlocked, 1 = locked, 2 = locked
    // with waiters.
    _Atomic int _state;
#else
    pthread_mutex_t _lock;
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
/// TAGS: Sys, Mutex, Init
///
#ifdef _WIN32
// SRWLOCK_INIT is just {0} -- zero-init is the initialised state.
#    define MutexInit() ((Mutex) {._lock = {0}})
#elif defined(__linux__) || defined(__APPLE__)
// _Atomic int = 0 is "unlocked" in the futex / __ulock state machine.
#    define MutexInit() ((Mutex) {._state = 0})
#else
// PTHREAD_MUTEX_INITIALIZER is a static-init constant expression on
// every POSIX libc we target.
#    define MutexInit() ((Mutex) {._lock = PTHREAD_MUTEX_INITIALIZER})
#endif

///
/// Tear down a mutex. On Windows / direct-syscall paths the kernel
/// has no per-mutex resources; the call zeroes the struct. On the
/// pthread fallback path, `pthread_mutex_destroy` releases the
/// underlying kernel resource.
///
/// SUCCESS: Function returns; struct is uninitialised.
/// FAILURE: No-op for NULL.
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
