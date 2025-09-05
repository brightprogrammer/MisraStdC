/// file      : sys.c
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Portable system functions

// required for strerror_r
// Reference : https://forums.freebsd.org/threads/strerror_r-best-practices-posix-vs-gnu.92296/
#define _POSIX_C_SOURCE 200112L

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
#    include <signal.h>
#    include <unistd.h>
#    ifdef __APPLE__
#        include <mach-o/dyld.h>
#    endif
#endif

#include <Misra/Std.h>
#include <Misra/Std/Log.h>
#include <Misra/Sys.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

Str* SysGetEnv(const char* name, Str* value) {
#ifdef _WIN32
    char*  env_var;
    size_t requiredSize;

    getenv_s(&requiredSize, NULL, 0, name);
    if (requiredSize == 0) {
        return NULL;
    }

    env_var = (char*)malloc(requiredSize);
    if (!env_var) {
        return NULL;
    }

    // Get the value of the LIB environment variable.
    getenv_s(&requiredSize, env_var, requiredSize, name);

    *value          = StrInit();
    value->data     = env_var;
    value->length   = requiredSize;
    value->capacity = requiredSize;
    return value;
#else
    char* env_var = getenv(name);
    if (env_var) {
        *value = StrInitFromZstr(env_var);
        return value;
    }
    return NULL;
#endif
}

Str* SysStrError(i32 eno, Str* err_str) {
    err_str->capacity = 128; // I hope it's enough on all platforms
    err_str->data     = (char*)calloc(err_str->length, 1);
#if _WIN32
    strerror_s(err_str->data, err_str->capacity, eno);
#else
    strerror_r(eno, err_str->data, err_str->capacity);
#endif

    if (!(err_str->length = ZstrLen(err_str->data))) {
        return NULL;
    }

    return err_str;
}

// Global callback for SysAbort - NULL means use default abort()
static SysAbortCallback g_abort_callback = NULL;

void SysSetAbortCallback(SysAbortCallback callback) {
    g_abort_callback = callback;
}

void SysAbort(void) {
    if (g_abort_callback) {
        g_abort_callback();
    } else {
        abort();
    }
}
