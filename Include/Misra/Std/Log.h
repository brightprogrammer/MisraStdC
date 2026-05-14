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
void SysAbort(void);

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
        StrWriteFmt(&m_, __VA_ARGS__);                                                                                 \
        LogWrite(LOG_MESSAGE_TYPE_FATAL, __func__, __LINE__, m_.data);                                                 \
        StrDeinit(&m_);                                                                                                \
        HeapAllocatorDeinit(&log_alloc_);                                                                              \
        SysAbort();                                                                                                    \
    } while (0)

///
/// Writes an error-level log message.
///
#define LOG_ERROR(...)                                                                                                 \
    do {                                                                                                               \
        HeapAllocator log_alloc_ = HeapAllocatorInit();                                                                \
        Str           m_         = StrInit(&log_alloc_);                                                               \
        StrWriteFmt(&m_, __VA_ARGS__);                                                                                 \
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
        StrWriteFmt(&m_, __VA_ARGS__);                                                                                 \
        LogWrite(LOG_MESSAGE_TYPE_INFO, __func__, __LINE__, m_.data);                                                  \
        StrDeinit(&m_);                                                                                                \
        HeapAllocatorDeinit(&log_alloc_);                                                                              \
    } while (0)

///
/// Writes a fatal log message and aborts the program, with `errno`
/// explanation appended.
///
#define LOG_SYS_FATAL(...)                                                                                             \
    do {                                                                                                               \
        HeapAllocator log_alloc_ = HeapAllocatorInit();                                                                \
        Str           m_         = StrInit(&log_alloc_);                                                               \
        StrWriteFmt(&m_, __VA_ARGS__);                                                                                 \
        Str syserr_;                                                                                                   \
        StrInitStack(syserr_, &log_alloc_, 256, {                                                                      \
            SysStrError(errno, &syserr_);                                                                              \
            StrWriteFmt(&m_, " : {}", syserr_);                                                                        \
        });                                                                                                            \
        LogWrite(LOG_MESSAGE_TYPE_FATAL, __func__, __LINE__, m_.data);                                                 \
        StrDeinit(&m_);                                                                                                \
        HeapAllocatorDeinit(&log_alloc_);                                                                              \
        SysAbort();                                                                                                    \
    } while (0)

///
/// Writes an error-level log message with `errno` explanation appended.
///
#define LOG_SYS_ERROR(...)                                                                                             \
    do {                                                                                                               \
        HeapAllocator log_alloc_ = HeapAllocatorInit();                                                                \
        Str           m_         = StrInit(&log_alloc_);                                                               \
        StrWriteFmt(&m_, __VA_ARGS__);                                                                                 \
        Str syserr_;                                                                                                   \
        StrInitStack(syserr_, &log_alloc_, 256, {                                                                      \
            SysStrError(errno, &syserr_);                                                                              \
            StrWriteFmt(&m_, " : {}", syserr_);                                                                        \
        });                                                                                                            \
        LogWrite(LOG_MESSAGE_TYPE_ERROR, __func__, __LINE__, m_.data);                                                 \
        StrDeinit(&m_);                                                                                                \
        HeapAllocatorDeinit(&log_alloc_);                                                                              \
    } while (0)

///
/// Writes an informational log message with errno explanation appended.
///
#define LOG_SYS_INFO(...)                                                                                              \
    do {                                                                                                               \
        HeapAllocator log_alloc_ = HeapAllocatorInit();                                                                \
        Str           m_         = StrInit(&log_alloc_);                                                               \
        StrWriteFmt(&m_, __VA_ARGS__);                                                                                 \
        Str syserr_;                                                                                                   \
        StrInitStack(syserr_, &log_alloc_, 256, {                                                                      \
            SysStrError(errno, &syserr_);                                                                              \
            StrWriteFmt(&m_, " : {}", syserr_);                                                                        \
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
/// Initialize logging subsystem
///
void LogInit(bool redirect);

///
/// Deinitialize logging subsystem
///
void LogDeinit(void);

///
/// Direct log message writer
///
void LogWrite(LogMessageType type, const char *func, u64 line, const char *msg);

#endif // MISRA_STD_LOG_H
