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
#        define MISRA_SYS_rt_sigaction  13
#        define MISRA_SYS_rt_sigreturn  15
#        define MISRA_SYS_unlink        87
#        define MISRA_SYS_rmdir         84
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
#        define MISRA_SYS_exit_group    231
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
#        define MISRA_SYS_rt_sigaction  134
#        define MISRA_SYS_rt_sigreturn  139
#        define MISRA_SYS_unlinkat      35 // aarch64: no SYS_unlink/SYS_rmdir, use unlinkat
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
#        define MISRA_SYS_exit_group    94
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

#elif defined(__APPLE__) && (defined(__x86_64__) || defined(__aarch64__))

// Darwin direct-syscall path. Apple "deprecates" raw syscalls -- the
// stable Apple-supported ABI is libSystem.dylib -- but they still
// function for the BSD-class syscall numbers we use, and Cosmopolitan
// libc (jart/cosmopolitan) is the prior-art reference. Caveat: Apple
// has renumbered BSD syscalls historically (most notoriously in 2008
// and again across some 10.x release transitions). Pin to a tested
// macOS major version and re-verify on each Apple OS bump.
//
// Differences from Linux that show up in this header:
//   - Syscall instruction unchanged on x86_64 (`syscall`), but on
//     aarch64 Darwin uses `svc #0x80` instead of `svc #0`.
//   - Number register on aarch64 is `x16` (not `x8`).
//   - x86_64 numbers carry a `class` prefix; BSD class is `0x2000000`,
//     so `read` is encoded as `0x2000003`. aarch64 numbers are raw.
//   - Error reporting is via the carry flag, not -errno in the
//     return register. We emit `jnc 1f; neg %rax; 1:` (x86_64) or
//     `b.cc 1f; neg x0, x0; 1:` (aarch64) right after the syscall
//     so the caller-visible convention matches Linux: negative
//     return means -errno, non-negative means success.

#    define FEATURE_DIRECT_SYSCALL 1

#    if defined(__x86_64__)
#        define MISRA_DARWIN_SC(n) ((long)((n) | 0x2000000L)) // BSD class
#    else
#        define MISRA_DARWIN_SC(n) ((long)(n))
#    endif

static inline long misra_sys0(long nr) {
    long ret;
#    if defined(__x86_64__)
    __asm__ volatile(
        "syscall\n\t"
        "jnc 1f\n\t"
        "negq %%rax\n"
        "1:"
        : "=a"(ret)
        : "0"(nr)
        : "rcx", "r11", "cc", "memory"
    );
#    else
    register long x16_ __asm__("x16") = nr;
    register long x0_ __asm__("x0");
    __asm__ volatile(
        "svc #0x80\n\t"
        "b.cc 1f\n\t"
        "neg x0, x0\n"
        "1:"
        : "=r"(x0_)
        : "r"(x16_)
        : "cc", "memory"
    );
    ret = x0_;
#    endif
    return ret;
}

static inline long misra_sys1(long nr, long a) {
    long ret;
#    if defined(__x86_64__)
    __asm__ volatile(
        "syscall\n\t"
        "jnc 1f\n\t"
        "negq %%rax\n"
        "1:"
        : "=a"(ret)
        : "0"(nr), "D"(a)
        : "rcx", "r11", "cc", "memory"
    );
#    else
    register long x16_ __asm__("x16") = nr;
    register long x0_ __asm__("x0")   = a;
    __asm__ volatile(
        "svc #0x80\n\t"
        "b.cc 1f\n\t"
        "neg x0, x0\n"
        "1:"
        : "+r"(x0_)
        : "r"(x16_)
        : "cc", "memory"
    );
    ret = x0_;
#    endif
    return ret;
}

static inline long misra_sys2(long nr, long a, long b) {
    long ret;
#    if defined(__x86_64__)
    __asm__ volatile(
        "syscall\n\t"
        "jnc 1f\n\t"
        "negq %%rax\n"
        "1:"
        : "=a"(ret)
        : "0"(nr), "D"(a), "S"(b)
        : "rcx", "r11", "cc", "memory"
    );
#    else
    register long x16_ __asm__("x16") = nr;
    register long x0_ __asm__("x0")   = a;
    register long x1_ __asm__("x1")   = b;
    __asm__ volatile(
        "svc #0x80\n\t"
        "b.cc 1f\n\t"
        "neg x0, x0\n"
        "1:"
        : "+r"(x0_)
        : "r"(x16_), "r"(x1_)
        : "cc", "memory"
    );
    ret = x0_;
#    endif
    return ret;
}

static inline long misra_sys3(long nr, long a, long b, long c) {
    long ret;
#    if defined(__x86_64__)
    __asm__ volatile(
        "syscall\n\t"
        "jnc 1f\n\t"
        "negq %%rax\n"
        "1:"
        : "=a"(ret)
        : "0"(nr), "D"(a), "S"(b), "d"(c)
        : "rcx", "r11", "cc", "memory"
    );
#    else
    register long x16_ __asm__("x16") = nr;
    register long x0_ __asm__("x0")   = a;
    register long x1_ __asm__("x1")   = b;
    register long x2_ __asm__("x2")   = c;
    __asm__ volatile(
        "svc #0x80\n\t"
        "b.cc 1f\n\t"
        "neg x0, x0\n"
        "1:"
        : "+r"(x0_)
        : "r"(x16_), "r"(x1_), "r"(x2_)
        : "cc", "memory"
    );
    ret = x0_;
#    endif
    return ret;
}

static inline long misra_sys4(long nr, long a, long b, long c, long d) {
    long ret;
#    if defined(__x86_64__)
    register long r10 __asm__("r10") = d;
    __asm__ volatile(
        "syscall\n\t"
        "jnc 1f\n\t"
        "negq %%rax\n"
        "1:"
        : "=a"(ret)
        : "0"(nr), "D"(a), "S"(b), "d"(c), "r"(r10)
        : "rcx", "r11", "cc", "memory"
    );
#    else
    register long x16_ __asm__("x16") = nr;
    register long x0_ __asm__("x0")   = a;
    register long x1_ __asm__("x1")   = b;
    register long x2_ __asm__("x2")   = c;
    register long x3_ __asm__("x3")   = d;
    __asm__ volatile(
        "svc #0x80\n\t"
        "b.cc 1f\n\t"
        "neg x0, x0\n"
        "1:"
        : "+r"(x0_)
        : "r"(x16_), "r"(x1_), "r"(x2_), "r"(x3_)
        : "cc", "memory"
    );
    ret = x0_;
#    endif
    return ret;
}

static inline long misra_sys5(long nr, long a, long b, long c, long d, long e) {
    long ret;
#    if defined(__x86_64__)
    register long r10 __asm__("r10") = d;
    register long r8 __asm__("r8")   = e;
    __asm__ volatile(
        "syscall\n\t"
        "jnc 1f\n\t"
        "negq %%rax\n"
        "1:"
        : "=a"(ret)
        : "0"(nr), "D"(a), "S"(b), "d"(c), "r"(r10), "r"(r8)
        : "rcx", "r11", "cc", "memory"
    );
#    else
    register long x16_ __asm__("x16") = nr;
    register long x0_ __asm__("x0")   = a;
    register long x1_ __asm__("x1")   = b;
    register long x2_ __asm__("x2")   = c;
    register long x3_ __asm__("x3")   = d;
    register long x4_ __asm__("x4")   = e;
    __asm__ volatile(
        "svc #0x80\n\t"
        "b.cc 1f\n\t"
        "neg x0, x0\n"
        "1:"
        : "+r"(x0_)
        : "r"(x16_), "r"(x1_), "r"(x2_), "r"(x3_), "r"(x4_)
        : "cc", "memory"
    );
    ret = x0_;
#    endif
    return ret;
}

static inline long misra_sys6(long nr, long a, long b, long c, long d, long e, long f) {
    long ret;
#    if defined(__x86_64__)
    register long r10 __asm__("r10") = d;
    register long r8 __asm__("r8")   = e;
    register long r9 __asm__("r9")   = f;
    __asm__ volatile(
        "syscall\n\t"
        "jnc 1f\n\t"
        "negq %%rax\n"
        "1:"
        : "=a"(ret)
        : "0"(nr), "D"(a), "S"(b), "d"(c), "r"(r10), "r"(r8), "r"(r9)
        : "rcx", "r11", "cc", "memory"
    );
#    else
    register long x16_ __asm__("x16") = nr;
    register long x0_ __asm__("x0")   = a;
    register long x1_ __asm__("x1")   = b;
    register long x2_ __asm__("x2")   = c;
    register long x3_ __asm__("x3")   = d;
    register long x4_ __asm__("x4")   = e;
    register long x5_ __asm__("x5")   = f;
    __asm__ volatile(
        "svc #0x80\n\t"
        "b.cc 1f\n\t"
        "neg x0, x0\n"
        "1:"
        : "+r"(x0_)
        : "r"(x16_), "r"(x1_), "r"(x2_), "r"(x3_), "r"(x4_), "r"(x5_)
        : "cc", "memory"
    );
    ret = x0_;
#    endif
    return ret;
}

// XNU BSD syscall numbers. Source: apple-oss-distributions/xnu
// bsd/kern/syscalls.master. Numbers are the same on x86_64 and
// aarch64; only the class-prefix encoding differs (handled by
// MISRA_DARWIN_SC).
#    define MISRA_SYS_exit            MISRA_DARWIN_SC(1)
#    define MISRA_SYS_fork            MISRA_DARWIN_SC(2)
#    define MISRA_SYS_read            MISRA_DARWIN_SC(3)
#    define MISRA_SYS_write           MISRA_DARWIN_SC(4)
#    define MISRA_SYS_open            MISRA_DARWIN_SC(5)
#    define MISRA_SYS_close           MISRA_DARWIN_SC(6)
#    define MISRA_SYS_wait4           MISRA_DARWIN_SC(7)
#    define MISRA_SYS_unlink          MISRA_DARWIN_SC(10)
#    define MISRA_SYS_getpid          MISRA_DARWIN_SC(20)
#    define MISRA_SYS_recvfrom        MISRA_DARWIN_SC(29)
#    define MISRA_SYS_accept          MISRA_DARWIN_SC(30)
#    define MISRA_SYS_getsockname     MISRA_DARWIN_SC(32)
#    define MISRA_SYS_kill            MISRA_DARWIN_SC(37)
#    define MISRA_SYS_pipe            MISRA_DARWIN_SC(42)
#    define MISRA_SYS_rt_sigaction    MISRA_DARWIN_SC(46) // XNU `sigaction`
#    define MISRA_SYS_ioctl           MISRA_DARWIN_SC(54)
#    define MISRA_SYS_execve          MISRA_DARWIN_SC(59)
#    define MISRA_SYS_munmap          MISRA_DARWIN_SC(73)
#    define MISRA_SYS_mprotect        MISRA_DARWIN_SC(74)
#    define MISRA_SYS_dup2            MISRA_DARWIN_SC(90)
#    define MISRA_SYS_fcntl           MISRA_DARWIN_SC(92)
#    define MISRA_SYS_socket          MISRA_DARWIN_SC(97)
#    define MISRA_SYS_connect         MISRA_DARWIN_SC(98)
#    define MISRA_SYS_bind            MISRA_DARWIN_SC(104)
#    define MISRA_SYS_setsockopt      MISRA_DARWIN_SC(105)
#    define MISRA_SYS_listen          MISRA_DARWIN_SC(106)
#    define MISRA_SYS_sendto          MISRA_DARWIN_SC(133)
#    define MISRA_SYS_rmdir           MISRA_DARWIN_SC(137)
#    define MISRA_SYS_rt_sigreturn    MISRA_DARWIN_SC(184) // XNU `sigreturn`
#    define MISRA_SYS_mmap            MISRA_DARWIN_SC(197)
#    define MISRA_SYS_lseek           MISRA_DARWIN_SC(199)
#    define MISRA_SYS_poll            MISRA_DARWIN_SC(230)
#    define MISRA_SYS_stat64          MISRA_DARWIN_SC(338)
#    define MISRA_SYS_fstat64         MISRA_DARWIN_SC(339)
#    define MISRA_SYS_getdirentries64 MISRA_DARWIN_SC(344)
#    define MISRA_SYS_thread_selfid   MISRA_DARWIN_SC(372) // = Linux gettid
#    define MISRA_SYS_openat          MISRA_DARWIN_SC(463)
#    define MISRA_SYS_fstatat64       MISRA_DARWIN_SC(470)
#    define MISRA_SYS_unlinkat        MISRA_DARWIN_SC(472)
// Aliases so files written for the Linux name still compile on Darwin.
#    define MISRA_SYS_exit_group      MISRA_SYS_exit // Darwin has no exit_group; plain exit terminates the whole task
#    define MISRA_SYS_gettid          MISRA_SYS_thread_selfid
#    define MISRA_SYS_fstat           MISRA_SYS_fstat64
#    define MISRA_SYS_stat            MISRA_SYS_stat64
#    define MISRA_SYS_newfstatat      MISRA_SYS_fstatat64
#    define MISRA_SYS_getdents64      MISRA_SYS_getdirentries64

#else

#    define FEATURE_DIRECT_SYSCALL 0

#endif // __linux__ && (__x86_64__ || __aarch64__)

#endif // MISRA__SYSCALL_H
