/// file      : std/log.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// copyright : Copyright (c) 2024, Siddharth Mishra, All rights reserved.
///
/// logging suppor

#ifndef MISRA_STD_LOG_H
#define MISRA_STD_LOG_H

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Misra
#include <Misra/Types.h>

#ifdef _WIN32
#    define LOG_FATAL(...) (LogWrite(LOG_MESSAGE_TYPE_FATAL, __FUNCTION__, __LINE__, __VA_ARGS__), abort())

#    define LOG_ERROR(...) LogWrite(LOG_MESSAGE_TYPE_ERROR, __FUNCTION__, __LINE__, __VA_ARGS__)

#    define LOG_INFO(...) LogWrite(LOG_MESSAGE_TYPE_INFO, __FUNCTION__, __LINE__, __VA_ARGS__)
#else
#    define LOG_FATAL(...) (LogWrite(LOG_MESSAGE_TYPE_FATAL, __FUNCTION__, __LINE__, __VA_ARGS__), abort())

#    define LOG_ERROR(...) LogWrite(LOG_MESSAGE_TYPE_ERROR, __FUNCTION__, __LINE__, __VA_ARGS__)

#    define LOG_INFO(...) LogWrite(LOG_MESSAGE_TYPE_INFO, __FUNCTION__, __LINE__, __VA_ARGS__)
#endif

typedef enum {
    LOG_MESSAGE_TYPE_FATAL,
    LOG_MESSAGE_TYPE_ERROR,
    LOG_MESSAGE_TYPE_INFO
} LogMessageType;

///
/// Initialize logging engine.
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
