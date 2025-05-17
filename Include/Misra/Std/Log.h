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

// Misra
#include <Misra/Types.h>

#ifdef _WIN32
#    define LOG_FATAL(...) (LogWrite(LOG_MESSAGE_TYPE_FATAL, __FUNCTION__, __LINE__, __VA_ARGS__), abort())

#    define LOG_ERROR(...) LogWrite(LOG_MESSAGE_TYPE_ERROR, __FUNCTION__, __LINE__, __VA_ARGS__)

#    define LOG_INFO(...) LogWrite(LOG_MESSAGE_TYPE_INFO, __FUNCTION__, __LINE__, __VA_ARGS__)
#else
///
/// Writes a fatal log message and then aborts the program.
/// This macro uses the `LogWrite` function with the `LOG_MESSAGE_TYPE_FATAL` level,
/// the current function name (`__FUNCTION__`), the current line number (`__LINE__`),
/// and the provided variable arguments. After logging, it calls the `abort()` function
/// to terminate the program immediately. This macro is platform-independent.
///
/// ...[in]    : Variable number of arguments representing the message to be logged,
///               following the format string conventions of `LogWrite`.
///
/// SUCCESS : Does not return. The log message is written, and the program is terminated.
/// FAILURE : Not applicable as the program always aborts after logging. The failure
///           would be in the inability to write the log message itself (e.g., due to
///           system errors within `LogWrite`), but the program will still attempt to abort.
///
#    define LOG_FATAL(...) (LogWrite(LOG_MESSAGE_TYPE_FATAL, __FUNCTION__, __LINE__, __VA_ARGS__), abort())

///
/// Writes an error log message.
/// This macro uses the `LogWrite` function with the `LOG_MESSAGE_TYPE_ERROR` level,
/// the current function name (`__FUNCTION__`), the current line number (`__LINE__`),
/// and the provided variable arguments. The program continues execution after logging.
/// This macro is platform-independent.
///
/// ...[in]    : Variable number of arguments representing the message to be logged,
///               following the format string conventions of `LogWrite`.
///
/// SUCCESS : The error message is written to the logging system.
/// FAILURE : Failure would occur if the `LogWrite` function itself encounters an error
///           during the logging process (e.g., due to issues with the logging destination).
///           The program flow continues regardless of the success or failure of the log write.
///
#    define LOG_ERROR(...) LogWrite(LOG_MESSAGE_TYPE_ERROR, __FUNCTION__, __LINE__, __VA_ARGS__)

///
/// Writes an informational log message.
/// This macro uses the `LogWrite` function with the `LOG_MESSAGE_TYPE_INFO` level,
/// the current function name (`__FUNCTION__`), the current line number (`__LINE__`),
/// and the provided variable arguments. The program continues execution after logging.
/// This macro is platform-independent.
///
/// ...[in]    : Variable number of arguments representing the message to be logged,
///               following the format string conventions of `LogWrite`.
///
/// SUCCESS : The informational message is written to the logging system.
/// FAILURE : Failure would occur if the `LogWrite` function itself encounters an error
///           during the logging process (e.g., due to issues with the logging destination).
///           The program flow continues regardless of the success or failure of the log write.
///
#    define LOG_INFO(...)  LogWrite(LOG_MESSAGE_TYPE_INFO, __FUNCTION__, __LINE__, __VA_ARGS__)
#endif

typedef enum LogMessageType {
    LOG_MESSAGE_TYPE_FATAL,
    LOG_MESSAGE_TYPE_ERROR,
    LOG_MESSAGE_TYPE_INFO
} LogMessageType;

///
/// Initialize logging engine.
///
/// NOTE: If user does not call this on their own, then this will
///       be lazily called on first attempt to `LOG_INFO`, `LOG_ERROR`
///       or `LOG_FATAL`
///
/// redirect[in] : Redirect log to a temporary file
///
void LogInit(bool redirect);

///
/// Shutdown logging engine.
///
void LogDeinit();

///
/// Generate the log message
///
/// type[in]   : Log message type (info, error, fatal)
/// tag[in]    : Log message idenfifier, something like file, function, name etc...
/// line[in]   : Line number at which this log was generated.
/// format[in] : Format string and following variadic arguments
///
void LogWrite(LogMessageType type, const char *tag, int line, const char *format, ...);

#endif // MISRA_STD_LOG_H
