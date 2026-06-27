/// file      : sys/clock.c
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Monotonic clock reader. See Sys/Clock.h for the contract.

#include <Misra/Config.h>
#include <Misra/Std/Log.h>
#include <Misra/Sys/Clock.h>

#include "../_Syscall.h"

#if FEATURE_FILE
#    include <Misra/Parsers/Tzif.h>
#    include <Misra/Std/Allocator/Default.h>
#endif

#if PLATFORM_WINDOWS
#    include <windows.h>
#endif

// `CLOCK_MONOTONIC_RAW` is clock id 4 on both Linux and Darwin: a
// monotonic source with no NTP/adjtime slew applied. Differences
// between readings are pure elapsed time.
#define CLOCK_MONOTONIC_RAW_ID 4

// `CLOCK_REALTIME` is clock id 0 on both Linux and Darwin: wall-clock
// time, epoch 1970-01-01 UTC, subject to NTP slew / settimeofday.
#define CLOCK_REALTIME_ID 0

// Kernel `struct timespec` on every 64-bit target: two longs.
typedef struct {
    long tv_sec;
    long tv_nsec;
} clock_timespec;

u64 ClockMonoNs(void) {
#if PLATFORM_WINDOWS
    LARGE_INTEGER counter;
    LARGE_INTEGER freq;
    if (!QueryPerformanceCounter(&counter) || !QueryPerformanceFrequency(&freq) || freq.QuadPart <= 0)
        LOG_FATAL("ClockMonoNs: QueryPerformanceCounter/Frequency failed");
    u64 ticks = (u64)counter.QuadPart;
    u64 hz    = (u64)freq.QuadPart;
    // Split the ns conversion to avoid overflowing the tick*1e9 product:
    // whole seconds first, then the sub-second remainder scaled to ns.
    return (ticks / hz) * 1000000000ull + ((ticks % hz) * 1000000000ull) / hz;
#elif PLATFORM_DARWIN
    // Darwin has no clock_gettime BSD syscall (libSystem implements it
    // over the commpage); Darwin builds keep the toolchain CRT, so the
    // libSystem symbol is always present. Weak-link it anyway, matching
    // Prng's entropy-source fallback shape.
    extern int     clock_gettime(int, void *) __attribute__((weak));
    clock_timespec ts;
    if (!clock_gettime || clock_gettime(CLOCK_MONOTONIC_RAW_ID, &ts) != 0)
        LOG_FATAL("ClockMonoNs: clock_gettime failed");
    return (u64)ts.tv_sec * 1000000000ull + (u64)ts.tv_nsec;
#elif FEATURE_DIRECT_SYSCALL
    clock_timespec ts;
    long           ret = direct_sys2(MISRA_SYS_clock_gettime, (long)CLOCK_MONOTONIC_RAW_ID, (long)(u64)&ts);
    if (ret < 0)
        LOG_FATAL("ClockMonoNs: clock_gettime syscall failed");
    return (u64)ts.tv_sec * 1000000000ull + (u64)ts.tv_nsec;
#else
    // Non-direct-syscall fallback: weakly-linked libc clock_gettime
    // with POSIX CLOCK_MONOTONIC (id 1).
    extern int     clock_gettime(int, void *) __attribute__((weak));
    clock_timespec ts;
    if (!clock_gettime || clock_gettime(1, &ts) != 0)
        LOG_FATAL("ClockMonoNs: clock_gettime unavailable");
    return (u64)ts.tv_sec * 1000000000ull + (u64)ts.tv_nsec;
#endif
}

u64 ClockRealNs(void) {
#if PLATFORM_WINDOWS
    FILETIME ft;
    GetSystemTimePreciseAsFileTime(&ft);
    u64 ticks = ((u64)ft.dwHighDateTime << 32) | (u64)ft.dwLowDateTime;
    // FILETIME counts 100-ns ticks since 1601-01-01; 116444736000000000
    // of them separate that epoch from 1970-01-01.
    return (ticks - 116444736000000000ull) * 100ull;
#elif PLATFORM_DARWIN
    extern int     clock_gettime(int, void *) __attribute__((weak));
    clock_timespec ts;
    if (!clock_gettime || clock_gettime(CLOCK_REALTIME_ID, &ts) != 0)
        LOG_FATAL("ClockRealNs: clock_gettime failed");
    return (u64)ts.tv_sec * 1000000000ull + (u64)ts.tv_nsec;
#elif FEATURE_DIRECT_SYSCALL
    clock_timespec ts;
    long           ret = direct_sys2(MISRA_SYS_clock_gettime, (long)CLOCK_REALTIME_ID, (long)(u64)&ts);
    if (ret < 0)
        LOG_FATAL("ClockRealNs: clock_gettime syscall failed");
    return (u64)ts.tv_sec * 1000000000ull + (u64)ts.tv_nsec;
#else
    extern int     clock_gettime(int, void *) __attribute__((weak));
    clock_timespec ts;
    if (!clock_gettime || clock_gettime(CLOCK_REALTIME_ID, &ts) != 0)
        LOG_FATAL("ClockRealNs: clock_gettime unavailable");
    return (u64)ts.tv_sec * 1000000000ull + (u64)ts.tv_nsec;
#endif
}

DateTime ClockUtc(void) {
    return DateTimeFromUnixNs(ClockRealNs(), 0);
}

DateTime ClockLocal(void) {
    u64 ns = ClockRealNs();
#if FEATURE_FILE
    DefaultAllocator alloc  = DefaultAllocatorInit();
    i32              offset = 0;
    bool             ok     = TzifLocalOffsetSeconds((i64)(ns / 1000000000ull), &offset, ALLOCATOR_OF(&alloc));
    DefaultAllocatorDeinit(&alloc);
    if (ok)
        return DateTimeFromUnixNs(ns, offset);
#endif
    return DateTimeFromUnixNs(ns, 0);
}
