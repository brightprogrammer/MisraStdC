/// file      : misra/sys/errno.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Project-side `errno` access without `<errno.h>`.
///
/// The numeric constants are POSIX-standardized (and stable across
/// Linux/macOS for the ones we name here). We define them ourselves
/// so callers don't have to include libc's `<errno.h>` just to compare
/// against `EINTR` or `EAGAIN`.
///
/// `Errno()` returns the thread-local errno value. On the direct
/// syscall path the kernel returns `-errno` in the return register,
/// so most callers use `ErrnoOf(ret)` from `Misra/Sys.h` directly
/// and never read the TLS slot. `Errno()` exists for the
/// libSystem / libc-bound platforms where wrappers set errno and
/// return -1.

#ifndef MISRA_SYS_ERRNO_H
#define MISRA_SYS_ERRNO_H

#include <Misra/Types.h>

#ifdef __cplusplus
extern "C" {
#endif

    // POSIX numeric constants. Same value on Linux and on macOS for
    // the names we expose here (EAGAIN and EADDRINUSE differ across
    // those platforms and are gated below).
#define EPERM  1
#define ENOENT 2
#define EINTR  4
#define EIO    5
#define EBADF  9
#define ENOMEM 12
#define EACCES 13
#define EFAULT 14
#define EBUSY  16
#define EEXIST 17
#define ENODEV 19
#define EINVAL 22
#define ENFILE 23
#define EMFILE 24
#define EPIPE  32
#define ERANGE 34

#if PLATFORM_DARWIN
#    define EAGAIN      35
#    define EADDRINUSE  48
#    define EWOULDBLOCK EAGAIN
#elif PLATFORM_WINDOWS
// Winsock error codes are higher numbered; we use those values
// because socket-path errnos come through `WSAGetLastError`. UCRT's
// `<errno.h>` (transitively pulled in by Windows system headers)
// uses lower-numbered constants for its C-runtime path; we override
// here so project code reads consistent Winsock values across TUs.
#    ifdef EAGAIN
#        undef EAGAIN
#    endif
#    ifdef EADDRINUSE
#        undef EADDRINUSE
#    endif
#    ifdef EWOULDBLOCK
#        undef EWOULDBLOCK
#    endif
#    define EAGAIN      10035 // WSAEWOULDBLOCK
#    define EADDRINUSE  10048 // WSAEADDRINUSE
#    define EWOULDBLOCK EAGAIN
#else                         // Linux + most other POSIX
#    define EAGAIN      11
#    define EADDRINUSE  98
#    define EWOULDBLOCK EAGAIN
#endif

    // Forward-declare the libc accessor for the TLS errno slot. We
    // pull the symbol by declaration so we never include `<errno.h>`;
    // each libc names this differently. On Windows the UCRT header
    // declares `_errno` as `__declspec(dllimport)`; our declaration
    // must match or the compiler flags an inconsistent-linkage
    // redeclaration (MSVC errors, clang-cl warns).
#if PLATFORM_DARWIN
    extern int *__error(void);
#elif PLATFORM_WINDOWS
__declspec(dllimport) extern int *__cdecl _errno(void);
#else
extern int *__errno_location(void);
#endif

    ///
    /// Read the current thread's errno value as `i32`. Resolves the
    /// platform-specific TLS-slot accessor inline so callers don't
    /// need to know which libc they're linked against, and there is
    /// no second symbol to keep in sync with this one.
    ///
    /// SUCCESS : Returns the current thread's errno value.
    /// FAILURE : Function cannot fail.
    ///
    /// TAGS: Errno
    ///
    static inline i32 Errno(void) {
#if PLATFORM_DARWIN
        return (i32)*__error();
#elif defined(_MSC_VER) || defined(__MSC_VER)
    return (i32)*_errno();
#else
    return (i32)*__errno_location();
#endif
    }

#ifdef __cplusplus
}
#endif

#endif // MISRA_SYS_ERRNO_H
