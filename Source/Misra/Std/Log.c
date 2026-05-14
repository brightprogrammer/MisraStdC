// Required for localtime_r
// Reference : https://forums.freebsd.org/threads/strerror_r-best-practices-posix-vs-gnu.92296/
#define _POSIX_C_SOURCE 200112L

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <Misra/Std/Log.h>
#include <Misra/Std/Allocator/Default.h>
#include <Misra/Sys.h>

static FILE      *stderror             = NULL;
static SysMutex  *log_mutex            = NULL;
static Allocator *log_persistent_alloc = NULL;

void close_log_file(void) {
    if (stderror) {
        fclose(stderror);
    }
}

void LogInit(bool redirect, Allocator *alloc) {
    if (!alloc) {
        LOG_FATAL("LogInit requires an allocator");
    }
    log_persistent_alloc = alloc;

    if (!redirect) {
        stderror = stderr;
        return;
    }

    time_t    raw_time;
    struct tm time_info;
    char      time_buffer[20] = {0};

    time(&raw_time);
#ifdef _WIN32
    if (localtime_s(&time_info, &raw_time)) {
#else
    if (!localtime_r(&raw_time, &time_info)) {
#endif
        DefaultAllocator err_alloc = DefaultAllocatorInit();
        Str              syserr;
        StrInitStack(syserr, &err_alloc, SYS_ERROR_STR_MAX_LENGTH, {
            SysStrError(errno, &syserr);
            LOG_ERROR("Failed to get localtime : {}", syserr);
        });
        DefaultAllocatorDeinit(&err_alloc);
        LOG_SYS_ERROR("Failed to get localtime");
        stderror = stderr;
        return;
    }
    strftime(time_buffer, sizeof(time_buffer), "%Y-%m-%d-%H-%M-%S", &time_info);

    DefaultAllocator init_alloc = DefaultAllocatorInit();
    Str              log_dir    = StrInit(&init_alloc);
    Str              file_name  = StrInit(&init_alloc);
    bool             redirected = false;

    if (SysGetEnv("TMP", &log_dir) || SysGetEnv("TEMP", &log_dir) || SysGetEnv("TMPDIR", &log_dir) ||
        SysGetEnv("TEMPDIR", &log_dir) || SysGetEnv("PWD", &log_dir)) {
        StrWriteFmt(&file_name, "{}/misra-{}-{}", log_dir, SysGetCurrentProcessId(), &time_buffer[0]);
        FWriteFmtLn(stderr, "storing logs in {}", file_name.data);

        i32 e = 0;
#ifdef _WIN32
        e = fopen_s(&stderror, file_name.data, "w");
#else
        stderror = fopen(file_name.data, "w");
        if (!stderror) {
            e = errno;
        }
#endif
        if (!e && stderror) {
            setvbuf(stderror, NULL, _IONBF, 0);
            atexit(close_log_file);
            redirected = true;
        }
    }

    StrDeinit(&file_name);
    StrDeinit(&log_dir);
    DefaultAllocatorDeinit(&init_alloc);

    if (!redirected) {
        FWriteFmtLn(stderr, "Error opening log file, will write logs to stderr");
        stderror = stderr;
    }
}

static void free_log_mutex(void) {
    if (log_mutex && log_persistent_alloc) {
        SysMutexDestroy(log_mutex, log_persistent_alloc);
        log_mutex = NULL;
    }
}

void LogDeinit(void) {
    if (stderror && stderror != stderr) {
        fclose(stderror);
    }
    free_log_mutex();
    log_persistent_alloc = NULL;
}

void LogWrite(LogMessageType type, const char *tag, u64 line, const char *msg) {
    if (!msg) {
        return;
    }

    tag = tag ? tag : "stdc";

    if (!stderror) {
        stderror = stderr;
    }

    if (!log_mutex && log_persistent_alloc) {
        log_mutex = SysMutexCreate(log_persistent_alloc);
        if (log_mutex) {
            atexit(free_log_mutex);
        }
    }

    const char *msg_type = NULL;
    switch (type) {
        case LOG_MESSAGE_TYPE_INFO :
            msg_type = "INFO";
            break;
        case LOG_MESSAGE_TYPE_ERROR :
            msg_type = "ERROR";
            break;
        case LOG_MESSAGE_TYPE_FATAL :
            msg_type = "FATAL";
            break;
        default :
            msg_type = "UNKNOWN_MESSAGE_TYPE";
            break;
    }

    if (log_mutex) {
        SysMutexLock(log_mutex);
    }

    FWriteFmt(stderror, "[{}] [{}:{}] ", msg_type, tag, line);
    fputs(msg, stderror);
    fputc('\n', stderror);

    if (log_mutex) {
        SysMutexUnlock(log_mutex);
    }
}
