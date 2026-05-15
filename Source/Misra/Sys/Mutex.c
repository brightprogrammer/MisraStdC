#include <Misra/Sys/Mutex.h>

#ifdef _WIN32
#    include <windows.h>
#    include <tlhelp32.h>
#    include <psapi.h>
#    include <signal.h>
#else
#    include <dirent.h>
#    include <pthread.h>
#    include <sys/stat.h>
#    include <sys/wait.h>
#    include <unistd.h>
#    ifdef __APPLE__
#        include <mach-o/dyld.h>
#    endif
#endif

#include <Misra/Std.h>
#include <Misra/Std/Log.h>

struct Mutex {
#ifdef _WIN32
    CRITICAL_SECTION lock;
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
    InitializeCriticalSection(&m->lock);
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
    DeleteCriticalSection(&m->lock);
#else
    pthread_mutex_destroy(&m->lock);
#endif
    MemSet(m, 0, sizeof(Mutex));
    AllocatorFree(alloc, m, sizeof(Mutex));
}

Mutex *MutexLock(Mutex *m) {
#ifdef _WIN32
    EnterCriticalSection(&m->lock);
#else
    pthread_mutex_lock(&m->lock);
#endif
    return m;
}

Mutex *MutexUnlock(Mutex *m) {
#ifdef _WIN32
    LeaveCriticalSection(&m->lock);
#else
    pthread_mutex_unlock(&m->lock);
#endif
    return m;
}
