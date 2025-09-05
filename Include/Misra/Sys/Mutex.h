#ifndef MISRA_SYS_MUTEX_H
#define MISRA_SYS_MUTEX_H

typedef struct SysMutex SysMutex;

///
/// Create a platform-independent mutex object.
///
/// SUCCESS : Returns valid SysMutex object.
/// FAILURE : Returns NULL if mutex creation fails.
///
/// TAGS: System, Threading, Synchronization
///
SysMutex *SysMutexCreate(void);

///
/// Destroy the provided mutex object.
/// Once a mutex is destroyed, all resources held by it will be freed.
/// Using it after this cal is UB.
///
/// m[in] : Mutex object to be destroyed.
///
/// SUCCESS : Resources released.
/// FAILURE : Function cannot fail - safe to call with NULL.
///
/// TAGS: System, Threading, Memory
///
void SysMutexDestroy(SysMutex *m);

///
/// Acquire lock on provided mutex object.
///
/// m[in,out] : Mutex to lock.
///
/// SUCCESS : Returns locked mutex.
/// FAILURE : Returns NULL if locking fails.
///
/// TAGS: System, Threading, Synchronization
///
SysMutex *SysMutexLock(SysMutex *m);

///
/// Release lock on provided mutex object.
///
/// m[in,out] : Mutex to unlock.
///
/// SUCCESS : Returns unlocked mutex.
/// FAILURE : Returns NULL if unlocking fails.
///
/// TAGS: System, Threading, Synchronization
///
SysMutex *SysMutexUnlock(SysMutex *m);

#endif // MISRA_SYS_MUTEX_H
