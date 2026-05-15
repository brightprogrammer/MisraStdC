#ifndef MISRA_SYS_MUTEX_H
#define MISRA_SYS_MUTEX_H

#include <Misra/Std/Allocator.h>

typedef struct Mutex Mutex;

///
/// Create a platform-independent mutex object. The mutex itself does
/// not own an allocator - the caller is responsible for remembering
/// `alloc` and passing it back to `MutexDestroy` so the handle's
/// one-shot allocation can be released through the same allocator.
///
/// alloc[in] : Allocator used to allocate the mutex handle.
///
/// SUCCESS : Returns valid Mutex object.
/// FAILURE : Returns NULL if mutex creation fails.
///
/// TAGS: System, Threading, Synchronization
///
Mutex *MutexCreate(Allocator *alloc);

///
/// Destroy the provided mutex object. Caller must pass the same
/// allocator that was originally given to `MutexCreate`.
///
/// m[in]     : Mutex object to be destroyed.
/// alloc[in] : Allocator that originally allocated the mutex handle.
///
/// SUCCESS : Resources released.
/// FAILURE : Function cannot fail - safe to call with NULL mutex.
///
/// TAGS: System, Threading, Memory
///
void MutexDestroy(Mutex *m, Allocator *alloc);

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
Mutex *MutexLock(Mutex *m);

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
Mutex *MutexUnlock(Mutex *m);

#endif // MISRA_SYS_MUTEX_H
