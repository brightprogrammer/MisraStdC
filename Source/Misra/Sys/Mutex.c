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

struct SysMutex {
#ifdef _WIN32
    CRITICAL_SECTION lock;
#else
    pthread_mutex_t lock;
#endif
};

SysMutex *SysMutexCreate(void) {
    Allocator allocator = DefaultAllocator();
    SysMutex *m         = (SysMutex *)AllocatorAlloc(&allocator, sizeof(SysMutex), _Alignof(SysMutex), true);

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

void SysMutexDestroy(SysMutex *m) {
    Allocator allocator = DefaultAllocator();

#ifdef _WIN32
    DeleteCriticalSection(&m->lock);
#else
    pthread_mutex_destroy(&m->lock);
#endif
    MemSet(m, 0, sizeof(SysMutex));
    AllocatorFree(&allocator, m, sizeof(SysMutex), _Alignof(SysMutex));
}

SysMutex *SysMutexLock(SysMutex *m) {
#ifdef _WIN32
    EnterCriticalSection(&m->lock);
#else
    pthread_mutex_lock(&m->lock);
#endif
    return m;
}

SysMutex *SysMutexUnlock(SysMutex *m) {
#ifdef _WIN32
    LeaveCriticalSection(&m->lock);
#else
    pthread_mutex_unlock(&m->lock);
#endif
    return m;
}
