/// file      : Source/Misra/_Syscall.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Internal Linux syscall plumbing. Use ONLY from .c files in
/// Source/; this header is not exposed via Include/Misra/, the
/// underscore prefix marks it as private to the implementation.
///
/// Provides misra_sys0..misra_sys6 -- thin inline-asm wrappers that
/// invoke a Linux syscall by number with the System V AMD64 / ARM64
/// ABI's argument register conventions. The kernel returns the result
/// in rax / x0; negative values < 4096 indicate -errno, anything else
/// is the success value (a return code, fd, ptr, etc.).
///
/// macOS / Windows are not handled by this header -- those platforms
/// disallow direct user-mode syscalls (macOS) or use a completely
/// different ABI (Windows NT). Callers that need cross-platform
/// behaviour should `#ifdef` per OS and route the non-Linux paths to
/// libSystem (macOS) or kernel32 (Windows) directly.

#ifndef MISRA__SYSCALL_H
#define MISRA__SYSCALL_H

#include <Misra/Types.h>

#include <stdint.h>

#if defined(__linux__) && (defined(__x86_64__) || defined(__aarch64__))

#    define FEATURE_DIRECT_SYSCALL 1

static inline long misra_sys0(long nr) {
    long ret;
#    if defined(__x86_64__)
    __asm__ volatile("syscall"
                     : "=a"(ret)
                     : "0"(nr)
                     : "rcx", "r11", "memory");
#    else
    register long x8 __asm__("x8") = nr;
    register long x0 __asm__("x0");
    __asm__ volatile("svc #0"
                     : "=r"(x0)
                     : "r"(x8)
                     : "memory");
    ret = x0;
#    endif
    return ret;
}

static inline long misra_sys1(long nr, long a) {
    long ret;
#    if defined(__x86_64__)
    __asm__ volatile("syscall"
                     : "=a"(ret)
                     : "0"(nr), "D"(a)
                     : "rcx", "r11", "memory");
#    else
    register long x8 __asm__("x8") = nr;
    register long x0 __asm__("x0") = a;
    __asm__ volatile("svc #0"
                     : "+r"(x0)
                     : "r"(x8)
                     : "memory");
    ret = x0;
#    endif
    return ret;
}

static inline long misra_sys2(long nr, long a, long b) {
    long ret;
#    if defined(__x86_64__)
    __asm__ volatile("syscall"
                     : "=a"(ret)
                     : "0"(nr), "D"(a), "S"(b)
                     : "rcx", "r11", "memory");
#    else
    register long x8 __asm__("x8") = nr;
    register long x0 __asm__("x0") = a;
    register long x1 __asm__("x1") = b;
    __asm__ volatile("svc #0"
                     : "+r"(x0)
                     : "r"(x8), "r"(x1)
                     : "memory");
    ret = x0;
#    endif
    return ret;
}

static inline long misra_sys3(long nr, long a, long b, long c) {
    long ret;
#    if defined(__x86_64__)
    __asm__ volatile("syscall"
                     : "=a"(ret)
                     : "0"(nr), "D"(a), "S"(b), "d"(c)
                     : "rcx", "r11", "memory");
#    else
    register long x8 __asm__("x8") = nr;
    register long x0 __asm__("x0") = a;
    register long x1 __asm__("x1") = b;
    register long x2 __asm__("x2") = c;
    __asm__ volatile("svc #0"
                     : "+r"(x0)
                     : "r"(x8), "r"(x1), "r"(x2)
                     : "memory");
    ret = x0;
#    endif
    return ret;
}

static inline long misra_sys4(long nr, long a, long b, long c, long d) {
    long ret;
#    if defined(__x86_64__)
    register long r10 __asm__("r10") = d;
    __asm__ volatile("syscall"
                     : "=a"(ret)
                     : "0"(nr), "D"(a), "S"(b), "d"(c), "r"(r10)
                     : "rcx", "r11", "memory");
#    else
    register long x8 __asm__("x8") = nr;
    register long x0 __asm__("x0") = a;
    register long x1 __asm__("x1") = b;
    register long x2 __asm__("x2") = c;
    register long x3 __asm__("x3") = d;
    __asm__ volatile("svc #0"
                     : "+r"(x0)
                     : "r"(x8), "r"(x1), "r"(x2), "r"(x3)
                     : "memory");
    ret = x0;
#    endif
    return ret;
}

static inline long misra_sys5(long nr, long a, long b, long c, long d, long e) {
    long ret;
#    if defined(__x86_64__)
    register long r10 __asm__("r10") = d;
    register long r8 __asm__("r8")   = e;
    __asm__ volatile("syscall"
                     : "=a"(ret)
                     : "0"(nr), "D"(a), "S"(b), "d"(c), "r"(r10), "r"(r8)
                     : "rcx", "r11", "memory");
#    else
    register long x8 __asm__("x8") = nr;
    register long x0 __asm__("x0") = a;
    register long x1 __asm__("x1") = b;
    register long x2 __asm__("x2") = c;
    register long x3 __asm__("x3") = d;
    register long x4 __asm__("x4") = e;
    __asm__ volatile("svc #0"
                     : "+r"(x0)
                     : "r"(x8), "r"(x1), "r"(x2), "r"(x3), "r"(x4)
                     : "memory");
    ret = x0;
#    endif
    return ret;
}

static inline long misra_sys6(long nr, long a, long b, long c, long d, long e, long f) {
    long ret;
#    if defined(__x86_64__)
    register long r10 __asm__("r10") = d;
    register long r8 __asm__("r8")   = e;
    register long r9 __asm__("r9")   = f;
    __asm__ volatile("syscall"
                     : "=a"(ret)
                     : "0"(nr), "D"(a), "S"(b), "d"(c), "r"(r10), "r"(r8), "r"(r9)
                     : "rcx", "r11", "memory");
#    else
    register long x8 __asm__("x8") = nr;
    register long x0 __asm__("x0") = a;
    register long x1 __asm__("x1") = b;
    register long x2 __asm__("x2") = c;
    register long x3 __asm__("x3") = d;
    register long x4 __asm__("x4") = e;
    register long x5 __asm__("x5") = f;
    __asm__ volatile("svc #0"
                     : "+r"(x0)
                     : "r"(x8), "r"(x1), "r"(x2), "r"(x3), "r"(x4), "r"(x5)
                     : "memory");
    ret = x0;
#    endif
    return ret;
}

// Linux syscall numbers we use (per arch). The standard reference
// is asm/unistd_64.h (x86_64) and asm-generic/unistd.h (aarch64).
#    if defined(__x86_64__)
#        define MISRA_SYS_read          0
#        define MISRA_SYS_write         1
#        define MISRA_SYS_open          2
#        define MISRA_SYS_close         3
#        define MISRA_SYS_stat          4
#        define MISRA_SYS_fstat         5
#        define MISRA_SYS_lstat         6
#        define MISRA_SYS_poll          7
#        define MISRA_SYS_lseek         8
#        define MISRA_SYS_mmap          9
#        define MISRA_SYS_mprotect      10
#        define MISRA_SYS_munmap        11
#        define MISRA_SYS_ioctl         16
#        define MISRA_SYS_pipe          22
#        define MISRA_SYS_dup2          33
#        define MISRA_SYS_nanosleep     35
#        define MISRA_SYS_getpid        39
#        define MISRA_SYS_gettid        186
#        define MISRA_SYS_socket        41
#        define MISRA_SYS_connect       42
#        define MISRA_SYS_accept        43
#        define MISRA_SYS_sendto        44
#        define MISRA_SYS_recvfrom      45
#        define MISRA_SYS_bind          49
#        define MISRA_SYS_listen        50
#        define MISRA_SYS_getsockname   51
#        define MISRA_SYS_setsockopt    54
#        define MISRA_SYS_clone         56
#        define MISRA_SYS_fork          57
#        define MISRA_SYS_execve        59
#        define MISRA_SYS_exit          60
#        define MISRA_SYS_wait4         61
#        define MISRA_SYS_kill          62
#        define MISRA_SYS_fcntl         72
#        define MISRA_SYS_getdents      78
#        define MISRA_SYS_readlink      89
#        define MISRA_SYS_futex         202
#        define MISRA_SYS_clock_gettime 228
#        define MISRA_SYS_openat        257
#        define MISRA_SYS_newfstatat    262
#        define MISRA_SYS_getdents64    217
#    else // __aarch64__
// aarch64 only has the "modern" syscall set: no SYS_open (use openat),
// no SYS_stat (use newfstatat), no SYS_pipe (use pipe2), no SYS_fork
// (use clone), no SYS_dup2 (use dup3), no SYS_getdents (use getdents64).
#        define MISRA_SYS_read          63
#        define MISRA_SYS_write         64
#        define MISRA_SYS_close         57
#        define MISRA_SYS_fstat         80
#        define MISRA_SYS_lseek         62
#        define MISRA_SYS_mmap          222
#        define MISRA_SYS_mprotect      226
#        define MISRA_SYS_munmap        215
#        define MISRA_SYS_ioctl         29
#        define MISRA_SYS_pipe2         59
#        define MISRA_SYS_dup3          24
#        define MISRA_SYS_nanosleep     101
#        define MISRA_SYS_getpid        172
#        define MISRA_SYS_gettid        178
#        define MISRA_SYS_socket        198
#        define MISRA_SYS_connect       203
#        define MISRA_SYS_accept        202
#        define MISRA_SYS_sendto        206
#        define MISRA_SYS_recvfrom      207
#        define MISRA_SYS_bind          200
#        define MISRA_SYS_listen        201
#        define MISRA_SYS_getsockname   204
#        define MISRA_SYS_setsockopt    208
#        define MISRA_SYS_clone         220
#        define MISRA_SYS_execve        221
#        define MISRA_SYS_exit          93
#        define MISRA_SYS_wait4         260
#        define MISRA_SYS_kill          129
#        define MISRA_SYS_fcntl         25
#        define MISRA_SYS_readlink      78 // actually readlinkat on aarch64
#        define MISRA_SYS_readlinkat    78
#        define MISRA_SYS_futex         98
#        define MISRA_SYS_clock_gettime 113
#        define MISRA_SYS_openat        56
#        define MISRA_SYS_newfstatat    79
#        define MISRA_SYS_getdents64    61
#        define MISRA_SYS_ppoll         73
#    endif

#else

#    define FEATURE_DIRECT_SYSCALL 0

#endif // __linux__ && (__x86_64__ || __aarch64__)

#endif // MISRA__SYSCALL_H
