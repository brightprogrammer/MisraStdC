// Required for localtime_r
// Reference : https://forums.freebsd.org/threads/strerror_r-best-practices-posix-vs-gnu.92296/
#define _POSIX_C_SOURCE 200112L

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// decompiler
#include <Misra/Std/Log.h>
#include <Misra/Sys.h>

static FILE     *stderror  = NULL;
static SysMutex *log_mutex = NULL;

void LogInit(bool redirect) {
    if (redirect) {
        // Get the current time
        time_t    raw_time;
        struct tm time_info;
        char      time_buffer[20] = {0};

        time(&raw_time);
#ifdef _WIN32
        // Order is inverted in Windows
        if (localtime_s(&time_info, &raw_time)) {
#else
        if (!localtime_r(&raw_time, &time_info)) {
#endif
            Str syserr;
            StrInitStack(syserr, SYS_ERROR_STR_MAX_LENGTH, {
                SysStrError(errno, &syserr);
                LOG_ERROR("Failed to get localtime : {}", syserr);
            });
            LOG_SYS_ERROR("Failed to get localtime");
            goto LOG_STREAM_FALLBACK;
        }
        strftime(time_buffer, sizeof(time_buffer), "%Y-%m-%d-%H-%M-%S", &time_info);

        // Get path to temp directory
        Str log_dir = StrInit();
        if (!SysGetEnv("TMP", &log_dir) && !SysGetEnv("TEMP", &log_dir) && !SysGetEnv("TMPDIR", &log_dir) &&
            !SysGetEnv("TEMPDIR", &log_dir) && !SysGetEnv("PWD", &log_dir)) {
            Str syserr;
            StrInitStack(syserr, SYS_ERROR_STR_MAX_LENGTH, {
                FWriteFmt(
                    stderr,
                    "error opening logfile : {}\n"
                    "All logs will now be redirected to stderr.\n",
                    SysStrError(errno, &syserr)->data
                );
            });
            stderror = stderr;
            goto LOG_STREAM_FALLBACK;
        }

        // generate log file name
        Str file_name = StrInit();
        StrWriteFmt(&file_name, "{}/misra-{}-{}", log_dir, SysGetCurrentProcessId(), &time_buffer[0]);
        FWriteFmtLn(stderr, "storing logs in {}", file_name.data);

        // Open the file for writing (create if it doesn't exist, overwrite if it does)
        i32 e = 0;
#ifdef _WIN32
        e = fopen_s(&stderror, file_name.data, "w");
#else
        stderror = fopen(file_name.data, "w");
        if (!stderror) {
            e = errno;
        }
#endif

        // Free resources
        StrDeinit(&file_name);
        StrDeinit(&log_dir);

        if (e || !stderror) {
            Str syserr;
            StrInitStack(syserr, SYS_ERROR_STR_MAX_LENGTH, {
                SysStrError(e, &syserr);
                LOG_ERROR("Failed to open log file : {}", syserr);
            });
            goto LOG_STREAM_FALLBACK;
        }

        // Flush buffer instantly!
        setvbuf(stderror, NULL, _IONBF, 0);
        return;

LOG_STREAM_FALLBACK: {
    Str syserr;
    StrInitStack(syserr, SYS_ERROR_STR_MAX_LENGTH, {
        FWriteFmtLn(stderr, "Error opening log file, will write logs to stderr");
    });
    stderror = stderr;
}
    } else {
        stderror = stderr;
    }
}


void LogDeinit(void) {
    if (stderror && stderror != stderr) {
        fclose(stderror);
    }

    if (log_mutex) {
        SysMutexDestroy(log_mutex);
    }
}


void LogWrite(LogMessageType type, const char *tag, int line, const char *msg) {
    if (!msg) {
        return;
    }

    // By default we have a "stdc" tag in all logs
    tag = tag ? tag : "stdc";

    // Initialize log if not already
    if (!stderror) {
        LogInit(false);
    }

    // Initialize the mutex if not already
    if (!log_mutex) {
        log_mutex = SysMutexCreate();
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

    SysMutexLock(log_mutex);

    // Print the log prefix to stderr
    FWriteFmt(stderror, "[{}] [{}:{}] ", msg_type, tag, line);

    fputs(msg, stderror);
    fputc('\n', stderror);

    SysMutexUnlock(log_mutex);
}
