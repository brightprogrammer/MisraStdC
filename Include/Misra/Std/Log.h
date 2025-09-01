/// file      : std/log.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// logging support

#ifndef MISRA_STD_LOG_H
#define MISRA_STD_LOG_H

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// Misra
#include <Misra/Types.h>
#include <Misra/Std/Io.h>

// Forward declaration to avoid circular includes
void SysAbort(void);

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
        Str m_##__LINE__ = StrInit();                                                                                  \
        StrWriteFmt(&m_##__LINE__, __VA_ARGS__);                                                                       \
        LogWrite(LOG_MESSAGE_TYPE_FATAL, __func__, __LINE__, m_##__LINE__.data);                                       \
        StrDeinit(&m_##__LINE__);                                                                                      \
        SysAbort();                                                                                                    \
    } while (0)

///
/// Writes an error-level log message.
///
/// ...[in] : Format string and arguments following printf-style syntax.
///
/// SUCCESS: Error message written to log output
/// FAILURE: Logging fails silently (output not guaranteed)
///
/// TAGS: Logging, Macro, Error, System
///
#define LOG_ERROR(...)                                                                                                 \
    do {                                                                                                               \
        Str m_##__LINE__ = StrInit();                                                                                  \
        StrWriteFmt(&m_##__LINE__, __VA_ARGS__);                                                                       \
        LogWrite(LOG_MESSAGE_TYPE_ERROR, __func__, __LINE__, m_##__LINE__.data);                                       \
        StrDeinit(&m_##__LINE__);                                                                                      \
    } while (0)

///
/// Writes an informational log message.
///
/// ...[in] : Format string and arguments following printf-style syntax.
///
/// SUCCESS: Informational message written to log output
/// FAILURE: Logging fails silently (output not guaranteed)
///
/// TAGS: Logging, Macro, Info, System
///
#define LOG_INFO(...)                                                                                                  \
    do {                                                                                                               \
        Str m_##__LINE__ = StrInit();                                                                                  \
        StrWriteFmt(&m_##__LINE__, __VA_ARGS__);                                                                       \
        LogWrite(LOG_MESSAGE_TYPE_INFO, __func__, __LINE__, m_##__LINE__.data);                                        \
        StrDeinit(&m_##__LINE__);                                                                                      \
    } while (0)

///
/// Writes a fatal log message and aborts the program, with `errno` explanation appended
/// at the end of final string.
///
/// INFO: Think of this as `perror()` with `LOG`
///
/// ...[in] : Format string and arguments following printf-style syntax.
///
/// SUCCESS: Message logged and program aborted via abort()
/// FAILURE: Logging may fail silently, but abort() will still execute
///
/// TAGS: Logging, Macro, Fatal, System
///
#define LOG_SYS_FATAL(...)                                                                                             \
    do {                                                                                                               \
        Str m_##__LINE__ = StrInit();                                                                                  \
        StrWriteFmt(&m_##__LINE__, __VA_ARGS__);                                                                       \
        Str syserr_##__LINE__;                                                                                         \
        StrInitStack(syserr_##__LINE__, 256, {                                                                         \
            SysStrError(errno, &syserr_##__LINE__);                                                                    \
            StrWriteFmt(&m_##__LINE__, " : {}", syserr_##__LINE__);                                                    \
        });                                                                                                            \
        LogWrite(LOG_MESSAGE_TYPE_FATAL, __func__, __LINE__, m_##__LINE__.data);                                       \
        StrDeinit(&m_##__LINE__);                                                                                      \
        SysAbort();                                                                                                    \
    } while (0)

///
/// Writes an error-level log message with `errno` explanation appended
/// at the end of final string.
///
/// INFO: Think of this as `perror()` with `LOG`
///
/// ...[in] : Format string and arguments following printf-style syntax.
///
/// SUCCESS: Error message written to log output
/// FAILURE: Logging fails silently (output not guaranteed)
///
/// TAGS: Logging, Macro, Error, System
///
#define LOG_SYS_ERROR(...)                                                                                             \
    do {                                                                                                               \
        Str m_##__LINE__ = StrInit();                                                                                  \
        StrWriteFmt(&m_##__LINE__, __VA_ARGS__);                                                                       \
        Str syserr_##__LINE__;                                                                                         \
        StrInitStack(syserr_##__LINE__, 256, {                                                                         \
            SysStrError(errno, &syserr_##__LINE__);                                                                    \
            StrWriteFmt(&m_##__LINE__, " : {}", syserr_##__LINE__);                                                    \
        });                                                                                                            \
        LogWrite(LOG_MESSAGE_TYPE_ERROR, __func__, __LINE__, m_##__LINE__.data);                                       \
        StrDeinit(&m_##__LINE__);                                                                                      \
    } while (0)

///
/// Writes an informational log message along with errno explanation appended
/// at then end of final string.
///
/// INFO: Think of this like `perror()` but as `LOG` macros
///
/// ...[in] : Format string and arguments following printf-style syntax.
///
/// SUCCESS: Informational message written to log output
/// FAILURE: Logging fails silently (output not guaranteed)
///
/// TAGS: Logging, Macro, Info, System
///
#define LOG_SYS_INFO(...)                                                                                              \
    do {                                                                                                               \
        Str m_##__LINE__ = StrInit();                                                                                  \
        StrWriteFmt(&m_##__LINE__, __VA_ARGS__);                                                                       \
        Str syserr_##__LINE__;                                                                                         \
        StrInitStack(syserr_##__LINE__, 256, {                                                                         \
            SysStrError(errno, &syserr_##__LINE__);                                                                    \
            StrWriteFmt(&m_##__LINE__, " : {}", syserr_##__LINE__);                                                    \
        });                                                                                                            \
        LogWrite(LOG_MESSAGE_TYPE_INFO, __func__, __LINE__, m_##__LINE__.data);                                        \
        StrDeinit(&m_##__LINE__);                                                                                      \
    } while (0)

///
/// Enumeration of log message severity levels
///
/// TAGS: Logging, Enum
///
typedef enum LogMessageType {
    LOG_MESSAGE_TYPE_FATAL,
    LOG_MESSAGE_TYPE_ERROR,
    LOG_MESSAGE_TYPE_INFO
} LogMessageType;

///
/// Initialize logging subsystem
///
/// NOTE: Is lazily called if not called by user at start of program.
///
/// redirect[in] : When true, redirect output to temporary file
///
/// SUCCESS: Logging system ready for use
/// FAILURE: Logging remains uninitialized (will lazy-init later)
///
/// TAGS: Logging, Initialization, System
void LogInit(bool redirect);

///
/// Shut down logging subsystem and release resources
///
/// SUCCESS: All logging resources released
/// FAILURE: Some resources may leak (safe to call multiple times)
///
/// TAGS: Logging, Cleanup, System
void LogDeinit(void);

///
/// Core log message generation function
///
/// type[in]   : Severity level of message
/// tag[in]    : Source identifier (typically function name)
/// line[in]   : Source line number
/// msg[in]    : Constant string to be printed
///
/// SUCCESS: Message formatted and written to log output
/// FAILURE: Message silently dropped (output not guaranteed)
///
/// TAGS: Logging, LowLevel, System
void LogWrite(LogMessageType type, const char *tag, int line, const char *msg);

#endif // MISRA_STD_LOG_H
