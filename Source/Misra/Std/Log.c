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

#include "../_Syscall.h"

#include <stdint.h>

// Get the current UTC epoch seconds without libc `time()`. Linux:
// direct clock_gettime(CLOCK_REALTIME); macOS / Windows fall back to
// libc time() which is part of libSystem / UCRT (not glibc).
static u64 misra_log_epoch_seconds(void) {
#if MISRA_HAVE_DIRECT_SYSCALL
    struct {
        long sec;
        long nsec;
    } ts     = {0, 0};
    long ret = misra_sys2(MISRA_SYS_clock_gettime, 0L /* CLOCK_REALTIME */, (long)(uintptr_t)&ts);
    if (ret < 0) {
        return 0;
    }
    return (u64)ts.sec;
#else
    time_t t = time(NULL);
    return (u64)t;
#endif
}

// In-tree UTC-time formatter producing "YYYY-MM-DD-HH-MM-SS".
// Replaces libc localtime_r + strftime so the log subsystem doesn't
// drag the locale / TZ machinery in. `out` must hold at least 20
// chars (19 + NUL). Uses Howard Hinnant's proleptic-Gregorian
// algorithm for days-since-epoch -> y/m/d.
static void misra_log_format_utc(u64 epoch_secs, char out[20]) {
    u64 days  = epoch_secs / 86400u;
    u64 dsecs = epoch_secs % 86400u;
    u32 hour  = (u32)(dsecs / 3600);
    u32 min   = (u32)((dsecs / 60) % 60);
    u32 sec   = (u32)(dsecs % 60);

    // Hinnant: civil_from_days
    i64 z   = (i64)days + 719468; // shift origin to 0000-03-01
    i64 era = (z >= 0 ? z : z - 146096) / 146097;
    u32 doe = (u32)(z - era * 146097);
    u32 yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
    i64 y   = (i64)yoe + era * 400;
    u32 doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
    u32 mp  = (5 * doy + 2) / 153;
    u32 d   = doy - (153 * mp + 2) / 5 + 1;
    u32 m   = mp < 10 ? mp + 3 : mp - 9;
    if (m <= 2) {
        y += 1;
    }

    // Format: YYYY-MM-DD-HH-MM-SS (19 chars + NUL).
    u32 year = (u32)(y < 0 ? 0 : y);
    out[0]   = (char)('0' + ((year / 1000) % 10));
    out[1]   = (char)('0' + ((year / 100) % 10));
    out[2]   = (char)('0' + ((year / 10) % 10));
    out[3]   = (char)('0' + (year % 10));
    out[4]   = '-';
    out[5]   = (char)('0' + (m / 10));
    out[6]   = (char)('0' + (m % 10));
    out[7]   = '-';
    out[8]   = (char)('0' + (d / 10));
    out[9]   = (char)('0' + (d % 10));
    out[10]  = '-';
    out[11]  = (char)('0' + (hour / 10));
    out[12]  = (char)('0' + (hour % 10));
    out[13]  = '-';
    out[14]  = (char)('0' + (min / 10));
    out[15]  = (char)('0' + (min % 10));
    out[16]  = '-';
    out[17]  = (char)('0' + (sec / 10));
    out[18]  = (char)('0' + (sec % 10));
    out[19]  = '\0';
}

static FILE      *stderror             = NULL;
static Mutex     *log_mutex            = NULL;
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

    char time_buffer[20] = {0};
    misra_log_format_utc(misra_log_epoch_seconds(), time_buffer);

    DefaultAllocator init_alloc = DefaultAllocatorInit();
    Str              log_dir    = StrInit(&init_alloc);
    Str              file_name  = StrInit(&init_alloc);
    bool             redirected = false;

    if (GetEnv("TMP", &log_dir) || GetEnv("TEMP", &log_dir) || GetEnv("TMPDIR", &log_dir) ||
        GetEnv("TEMPDIR", &log_dir) || GetEnv("PWD", &log_dir)) {
        StrWriteFmt(&file_name, "{}/misra-{}-{}", log_dir, ProcGetCurrentId(), &time_buffer[0]);
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
            // No setvbuf -- we explicitly fflush after every log line
            // (see emit_log_msg). That keeps the libc dep surface
            // smaller and avoids buffering surprises when the log file
            // is on a redirected fd that ignores _IONBF.
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
        MutexDestroy(log_mutex, log_persistent_alloc);
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
        log_mutex = MutexCreate(log_persistent_alloc);
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
        MutexLock(log_mutex);
    }

    FWriteFmt(stderror, "[{}] [{}:{}] ", msg_type, tag, line);
    fputs(msg, stderror);
    fputc('\n', stderror);
    fflush(stderror);

    if (log_mutex) {
        MutexUnlock(log_mutex);
    }
}
