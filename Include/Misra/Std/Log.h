/// file      : std/log.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// logging support

#ifndef MISRA_STD_LOG_H
#define MISRA_STD_LOG_H

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
        HeapAllocator UNPL(log_alloc) = HeapAllocatorInit();                                                           \
        Str           UNPL(m)         = StrInit(&UNPL(log_alloc));                                                     \
        StrAppendFmt(&UNPL(m), __VA_ARGS__);                                                                           \
        LogWrite(LOG_MESSAGE_TYPE_FATAL, __func__, __LINE__, StrBegin(&UNPL(m)));                                            \
        StrDeinit(&UNPL(m));                                                                                           \
        HeapAllocatorDeinit(&UNPL(log_alloc));                                                                         \
        Abort();                                                                                                       \
    } while (0)

///
/// Writes an error-level log message.
///
#define LOG_ERROR(...)                                                                                                 \
    do {                                                                                                               \
        HeapAllocator UNPL(log_alloc) = HeapAllocatorInit();                                                           \
        Str           UNPL(m)         = StrInit(&UNPL(log_alloc));                                                     \
        StrAppendFmt(&UNPL(m), __VA_ARGS__);                                                                           \
        LogWrite(LOG_MESSAGE_TYPE_ERROR, __func__, __LINE__, StrBegin(&UNPL(m)));                                            \
        StrDeinit(&UNPL(m));                                                                                           \
        HeapAllocatorDeinit(&UNPL(log_alloc));                                                                         \
    } while (0)

///
/// Writes an informational log message.
///
#define LOG_INFO(...)                                                                                                  \
    do {                                                                                                               \
        HeapAllocator UNPL(log_alloc) = HeapAllocatorInit();                                                           \
        Str           UNPL(m)         = StrInit(&UNPL(log_alloc));                                                     \
        StrAppendFmt(&UNPL(m), __VA_ARGS__);                                                                           \
        LogWrite(LOG_MESSAGE_TYPE_INFO, __func__, __LINE__, StrBegin(&UNPL(m)));                                             \
        StrDeinit(&UNPL(m));                                                                                           \
        HeapAllocatorDeinit(&UNPL(log_alloc));                                                                         \
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
/// Use `ErrnoOf(ret)` from `<Misra/Sys.h>` to convert a syscall
/// return value to an errno code in a platform-portable way.
///
#define LOG_SYS_FATAL(eno, ...)                                                                                        \
    do {                                                                                                               \
        i32           UNPL(sys_eno)   = (i32)(eno);                                                                    \
        HeapAllocator UNPL(log_alloc) = HeapAllocatorInit();                                                           \
        Str           UNPL(m)         = StrInit(&UNPL(log_alloc));                                                     \
        StrAppendFmt(&UNPL(m), __VA_ARGS__);                                                                           \
        StrInitStack(UNPL(syserr), 256) {                                                                              \
            StrError(UNPL(sys_eno), &UNPL(syserr));                                                                    \
            StrAppendFmt(&UNPL(m), " : {}", UNPL(syserr));                                                             \
        }                                                                                                              \
        LogWrite(LOG_MESSAGE_TYPE_FATAL, __func__, __LINE__, StrBegin(&UNPL(m)));                                            \
        StrDeinit(&UNPL(m));                                                                                           \
        HeapAllocatorDeinit(&UNPL(log_alloc));                                                                         \
        Abort();                                                                                                       \
    } while (0)

///
/// Writes an error-level log message with the caller-supplied system
/// error code explained. See `LOG_SYS_FATAL` for the errno-passing
/// rationale.
///
#define LOG_SYS_ERROR(eno, ...)                                                                                        \
    do {                                                                                                               \
        i32           UNPL(sys_eno)   = (i32)(eno);                                                                    \
        HeapAllocator UNPL(log_alloc) = HeapAllocatorInit();                                                           \
        Str           UNPL(m)         = StrInit(&UNPL(log_alloc));                                                     \
        StrAppendFmt(&UNPL(m), __VA_ARGS__);                                                                           \
        StrInitStack(UNPL(syserr), 256) {                                                                              \
            StrError(UNPL(sys_eno), &UNPL(syserr));                                                                    \
            StrAppendFmt(&UNPL(m), " : {}", UNPL(syserr));                                                             \
        }                                                                                                              \
        LogWrite(LOG_MESSAGE_TYPE_ERROR, __func__, __LINE__, StrBegin(&UNPL(m)));                                            \
        StrDeinit(&UNPL(m));                                                                                           \
        HeapAllocatorDeinit(&UNPL(log_alloc));                                                                         \
    } while (0)

///
/// Writes an informational log message with the caller-supplied system
/// error code explained.
///
#define LOG_SYS_INFO(eno, ...)                                                                                         \
    do {                                                                                                               \
        i32           UNPL(sys_eno)   = (i32)(eno);                                                                    \
        HeapAllocator UNPL(log_alloc) = HeapAllocatorInit();                                                           \
        Str           UNPL(m)         = StrInit(&UNPL(log_alloc));                                                     \
        StrAppendFmt(&UNPL(m), __VA_ARGS__);                                                                           \
        StrInitStack(UNPL(syserr), 256) {                                                                              \
            StrError(UNPL(sys_eno), &UNPL(syserr));                                                                    \
            StrAppendFmt(&UNPL(m), " : {}", UNPL(syserr));                                                             \
        }                                                                                                              \
        LogWrite(LOG_MESSAGE_TYPE_INFO, __func__, __LINE__, StrBegin(&UNPL(m)));                                             \
        StrDeinit(&UNPL(m));                                                                                           \
        HeapAllocatorDeinit(&UNPL(log_alloc));                                                                         \
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
/// Expression-form fatal assertion. When `cond` is false, logs `msg` at
/// the caller's `__func__` / `__LINE__` at FATAL severity and aborts;
/// otherwise evaluates to `0`. Designed for use inside designated-
/// initializer literals and comma expressions where the statement-style
/// `LOG_FATAL(...)` (`do { ... } while (0)`) does not fit.
///
/// Args are evaluated as a normal C ternary: `cond` once; `msg` only on
/// the failure path. The whole macro has type `int`, so it composes
/// inside ternaries and comma chains without further casting.
///
/// cond[in] : Boolean condition that must hold.
/// msg[in]  : Constant `Zstr` (string literal preferred).
///
/// SUCCESS : Evaluates to `0` when `cond` is true.
/// FAILURE : `LogWrite(...)` then `Abort()`; never returns.
///
/// TAGS: Logging, Assert, Macro, Expression
///
#define ASSERT_OR_FATAL(cond, msg)                                                                                     \
    ((cond) ? 0 : (LogWrite(LOG_MESSAGE_TYPE_FATAL, __func__, __LINE__, (msg)), Abort(), 0))

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
/// SUCCESS : Formats one line ("[LEVEL] [tag:line] msg\n") through a
///           stack-local `HeapAllocator`, writes it via a single
///           `FileWrite` to fd 1 (INFO) or fd 2 (ERROR / FATAL), and
///           for FATAL appends a captured backtrace; releases the
///           transient Str + allocator and returns.
/// FAILURE : Returns without writing if `msg` is NULL. Underlying
///           `FileWrite` errors are dropped (the logger is best-effort
///           by design; there is no upstream reporter to surface to).
///
void LogWrite(LogMessageType type, Zstr tag, u64 line, Zstr msg);

#endif // MISRA_STD_LOG_H
