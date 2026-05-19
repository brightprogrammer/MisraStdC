/// file      : std/log.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// logging support

#ifndef MISRA_STD_LOG_H
#define MISRA_STD_LOG_H

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Misra
#include <Misra/Std/Allocator.h>
#include <Misra/Std/Allocator/Heap.h>
#include <Misra/Std/Io.h>
#include <Misra/Types.h>

// Forward declaration to avoid circular includes
void Abort(void);

///
/// Each LOG_* macro builds its message string through a stack-local
/// `HeapAllocator` declared inside the do-while body. This keeps the macros
/// usable from any context (no user-supplied allocator argument needed)
/// while honouring the no-library-globals rule.
///

///
/// Writes a fatal log message and aborts the program.
///
/// ...[in] : Format string and arguments following printf-style syntax.
///
/// SUCCESS: Message logged and program aborted via abort()
/// FAILURE: Logging may fail silently, but abort() will still execute
///
/// TAGS: Logging, Macro, Fatal, System
///
#define LOG_FATAL(...)                                                                                                 \
    do {                                                                                                               \
        HeapAllocator log_alloc_ = HeapAllocatorInit();                                                                \
        Str           m_         = StrInit(&log_alloc_);                                                               \
        StrAppendFmt(&m_, __VA_ARGS__);                                                                                 \
        LogWrite(LOG_MESSAGE_TYPE_FATAL, __func__, __LINE__, m_.data);                                                 \
        StrDeinit(&m_);                                                                                                \
        HeapAllocatorDeinit(&log_alloc_);                                                                              \
        Abort();                                                                                                       \
    } while (0)

///
/// Writes an error-level log message.
///
#define LOG_ERROR(...)                                                                                                 \
    do {                                                                                                               \
        HeapAllocator log_alloc_ = HeapAllocatorInit();                                                                \
        Str           m_         = StrInit(&log_alloc_);                                                               \
        StrAppendFmt(&m_, __VA_ARGS__);                                                                                 \
        LogWrite(LOG_MESSAGE_TYPE_ERROR, __func__, __LINE__, m_.data);                                                 \
        StrDeinit(&m_);                                                                                                \
        HeapAllocatorDeinit(&log_alloc_);                                                                              \
    } while (0)

///
/// Writes an informational log message.
///
#define LOG_INFO(...)                                                                                                  \
    do {                                                                                                               \
        HeapAllocator log_alloc_ = HeapAllocatorInit();                                                                \
        Str           m_         = StrInit(&log_alloc_);                                                               \
        StrAppendFmt(&m_, __VA_ARGS__);                                                                                 \
        LogWrite(LOG_MESSAGE_TYPE_INFO, __func__, __LINE__, m_.data);                                                  \
        StrDeinit(&m_);                                                                                                \
        HeapAllocatorDeinit(&log_alloc_);                                                                              \
    } while (0)

///
/// Writes a fatal log message and aborts the program, with the
/// caller-supplied system error code explained.
///
/// First arg is the error number (usually an `errno` value, or a
/// `-syscall_return` value when the syscall ABI returns -errno
/// directly). Caller passes it explicitly so we don't have to read
/// the libc `errno` TLS slot here -- pulling `__errno_location` into
/// every binary that uses LOG_SYS_* defeats the libc-diet effort.
/// Use `SYS_ERRNO(ret)` from `<Misra/Sys.h>` to convert a syscall
/// return value to an errno code in a platform-portable way.
///
#define LOG_SYS_FATAL(eno, ...)                                                                                        \
    do {                                                                                                               \
        i32           sys_eno_   = (i32)(eno);                                                                         \
        HeapAllocator log_alloc_ = HeapAllocatorInit();                                                                \
        Str           m_         = StrInit(&log_alloc_);                                                               \
        StrAppendFmt(&m_, __VA_ARGS__);                                                                                 \
        Str syserr_;                                                                                                   \
        StrInitStack(syserr_, &log_alloc_, 256, {                                                                      \
            StrError(sys_eno_, &syserr_);                                                                              \
            StrAppendFmt(&m_, " : {}", syserr_);                                                                        \
        });                                                                                                            \
        LogWrite(LOG_MESSAGE_TYPE_FATAL, __func__, __LINE__, m_.data);                                                 \
        StrDeinit(&m_);                                                                                                \
        HeapAllocatorDeinit(&log_alloc_);                                                                              \
        Abort();                                                                                                       \
    } while (0)

///
/// Writes an error-level log message with the caller-supplied system
/// error code explained. See `LOG_SYS_FATAL` for the errno-passing
/// rationale.
///
#define LOG_SYS_ERROR(eno, ...)                                                                                        \
    do {                                                                                                               \
        i32           sys_eno_   = (i32)(eno);                                                                         \
        HeapAllocator log_alloc_ = HeapAllocatorInit();                                                                \
        Str           m_         = StrInit(&log_alloc_);                                                               \
        StrAppendFmt(&m_, __VA_ARGS__);                                                                                 \
        Str syserr_;                                                                                                   \
        StrInitStack(syserr_, &log_alloc_, 256, {                                                                      \
            StrError(sys_eno_, &syserr_);                                                                              \
            StrAppendFmt(&m_, " : {}", syserr_);                                                                        \
        });                                                                                                            \
        LogWrite(LOG_MESSAGE_TYPE_ERROR, __func__, __LINE__, m_.data);                                                 \
        StrDeinit(&m_);                                                                                                \
        HeapAllocatorDeinit(&log_alloc_);                                                                              \
    } while (0)

///
/// Writes an informational log message with the caller-supplied system
/// error code explained.
///
#define LOG_SYS_INFO(eno, ...)                                                                                         \
    do {                                                                                                               \
        i32           sys_eno_   = (i32)(eno);                                                                         \
        HeapAllocator log_alloc_ = HeapAllocatorInit();                                                                \
        Str           m_         = StrInit(&log_alloc_);                                                               \
        StrAppendFmt(&m_, __VA_ARGS__);                                                                                 \
        Str syserr_;                                                                                                   \
        StrInitStack(syserr_, &log_alloc_, 256, {                                                                      \
            StrError(sys_eno_, &syserr_);                                                                              \
            StrAppendFmt(&m_, " : {}", syserr_);                                                                        \
        });                                                                                                            \
        LogWrite(LOG_MESSAGE_TYPE_INFO, __func__, __LINE__, m_.data);                                                  \
        StrDeinit(&m_);                                                                                                \
        HeapAllocatorDeinit(&log_alloc_);                                                                              \
    } while (0)

///
/// Enumeration of log message severity levels
///
typedef enum LogMessageType {
    LOG_MESSAGE_TYPE_FATAL,
    LOG_MESSAGE_TYPE_ERROR,
    LOG_MESSAGE_TYPE_INFO
} LogMessageType;

///
/// Direct log message writer. Stateless: no setup, no teardown, no
/// globals. INFO lines go to the normal output channel (fd 1 on
/// POSIX); ERROR and FATAL go to the diagnostic channel (fd 2). FATAL
/// additionally appends a captured stack trace before returning to
/// the caller -- the LOG_FATAL macro then calls Abort().
///
/// type[in] : Severity selector.
/// tag[in]  : Caller identifier; defaulted to "misra" if NULL. The
///            LOG_* macros pass `__func__` here.
/// line[in] : Source line; the LOG_* macros pass `__LINE__`.
/// msg[in]  : Pre-formatted message string (no trailing newline; the
///            implementation adds one). NULL-safe (no-op).
///
void LogWrite(LogMessageType type, const char *tag, u64 line, const char *msg);

#endif // MISRA_STD_LOG_H
