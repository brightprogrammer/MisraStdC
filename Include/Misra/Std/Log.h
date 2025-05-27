/// file      : std/log.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// copyright : Copyright (c) 2024, Siddharth Mishra, All rights reserved.
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

// #ifdef _WIN32
// #    define LOG_FATAL(...) (LogWrite(LOG_MESSAGE_TYPE_FATAL, __FUNCTION__, __LINE__, __VA_ARGS__), abort())
//
// #    define LOG_ERROR(...) LogWrite(LOG_MESSAGE_TYPE_ERROR, __FUNCTION__, __LINE__, __VA_ARGS__)
//
// #    define LOG_INFO(...) LogWrite(LOG_MESSAGE_TYPE_INFO, __FUNCTION__, __LINE__, __VA_ARGS__)
// #else
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
#define LOG_FATAL(...) (LogWrite(LOG_MESSAGE_TYPE_FATAL, __FUNCTION__, __LINE__, __VA_ARGS__), abort())

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
#define LOG_ERROR(...) LogWrite(LOG_MESSAGE_TYPE_ERROR, __FUNCTION__, __LINE__, __VA_ARGS__)

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
#define LOG_INFO(...) LogWrite(LOG_MESSAGE_TYPE_INFO, __FUNCTION__, __LINE__, __VA_ARGS__)
// #endif

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
/// format[in] : printf-style format string
/// ...[in]    : Variadic arguments for format string
///
/// SUCCESS: Message formatted and written to log output
/// FAILURE: Message silently dropped (output not guaranteed)
///
/// TAGS: Logging, LowLevel, System
void LogWrite(LogMessageType type, const char *tag, int line, const char *format, ...);

#endif // MISRA_STD_LOG_H
