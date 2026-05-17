/// file      : sys.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Portable system functions

#ifndef MISRA_SYS_H
#define MISRA_SYS_H

#include <Misra/Std/Container/Str.h>
#include <errno.h>

#include <Misra/Sys/Mutex.h>

// `ProcId` is part of the foundation because `LOG_FATAL` formats it
// into the log message. `ProcGetCurrentId` (declared below) lives
// in `Sys.c`, also foundation. The full process-spawning API in
// `Sys/Proc.h` is the optional `sys_proc` feature - C11 lets the
// typedef appear in both files.
typedef u64 ProcId;

#if FEATURE_SYS_DIR
#    include <Misra/Sys/Dir.h>
#endif

#if FEATURE_SYS_PROC
#    include <Misra/Sys/Proc.h>
#endif

#if FEATURE_SYS_SOCKET
#    include <Misra/Sys/Socket.h>
#endif

#if FEATURE_SYS_PROCMAPS
#    include <Misra/Sys/ProcMaps.h>
#endif

// Sys/SymbolResolver.h is NOT pulled through the umbrella because it
// transits Parsers/Elf.h, which collides with Bin/ElfInfo.c's local
// ELF constants. Include `Misra/Sys/SymbolResolver.h` directly when
// you want the resolver. Tracked in FUTURE-PLANS.md.

#ifndef SYS_ERROR_STR_MAX_LENGTH
#    define SYS_ERROR_STR_MAX_LENGTH 128
#endif

///
/// Platform-independent method to get current process Id. Foundation
/// API: provided by `Sys.c` (always built), unlike the rest of the
/// process-spawning functions in `Sys/Proc.h` which live in the optional
/// `sys_proc` feature.
///
/// SUCCESS : Returns current process ID.
/// FAILURE : Function cannot fail - always returns valid ID.
///
/// TAGS: System, Process
///
ProcId ProcGetCurrentId(void);

///
/// Get last error using an error number.
///
/// eno[in]      : Unique error number descriptor.
/// err_str[out] : Error string will be stored in this.
///
/// SUCCESS : Error string describing last error.
/// FAILURE : Returns NULL if `err_str` is NULL.
///
/// TAGS: System, Error, String
///
Str *StrError(i32 eno, Str *err_str);

///
/// Function pointer type for Abort callback.
/// This allows custom handling of abort situations (e.g., for testing).
///
typedef void (*AbortCallback)(void);

///
/// Set a custom callback function for Abort.
/// If no callback is set, Abort will call the standard abort() function.
///
/// callback[in] : Function to call when Abort is invoked, or NULL to reset to default.
///
/// SUCCESS : Callback is set.
/// FAILURE : Function cannot fail.
///
/// TAGS: System, Testing, Callback
///
void SetAbortCallback(AbortCallback callback);

///
/// Custom abort function that can be redirected for testing purposes.
/// By default, this calls the standard abort() function.
/// If a callback is set via SetAbortCallback, it calls the callback instead.
///
/// SUCCESS : Function does not return (either aborts or calls callback).
/// FAILURE : Function cannot fail.
///
/// TAGS: System, Testing, Control
///
void Abort(void);

#endif // MISRA_SYS_H
