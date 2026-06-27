/// file      : sys/clock.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Monotonic clock. A thin, libc-free reader over the kernel's
/// monotonic time source: `clock_gettime(CLOCK_MONOTONIC_RAW)` on
/// Linux (direct syscall) and Darwin (libSystem, which Darwin always
/// links), `QueryPerformanceCounter` on Windows. The clock counts up
/// from an unspecified epoch and is immune to wall-clock adjustments
/// (NTP slew, `settimeofday`), so differences between two readings are
/// the right thing to measure elapsed durations with.
///
/// Foundation module: always built, no feature flag. Used by the
/// benchmark harness and available to any consumer that needs a
/// monotonic timestamp.

#ifndef MISRA_SYS_CLOCK_H
#define MISRA_SYS_CLOCK_H

#include <Misra/Std/DateTime.h>
#include <Misra/Types.h>

///
/// Current value of the monotonic clock, in nanoseconds.
///
/// The epoch is unspecified (boot-relative on Linux, an arbitrary
/// fixed origin elsewhere); only the difference between two readings
/// is meaningful. Resolution is the platform clock's native
/// granularity -- nanoseconds on Linux/Darwin, the
/// `QueryPerformanceFrequency` tick on Windows scaled to ns.
///
/// Thread-safety: safe to call concurrently; reads a kernel/commpage
/// time source and keeps no shared state of its own.
///
/// SUCCESS : Returns a monotonically non-decreasing nanosecond count.
/// FAILURE : Aborts via `LOG_FATAL` if the platform clock query fails
///           (a non-functional monotonic clock is treated as a fatal
///           system condition, the same fail-fast contract as `Prng64`;
///           there is no safe fallback value to return).
///
/// TAGS: Sys, Clock, Time, Monotonic
///
u64 ClockMonoNs(void);

#if defined(_MSC_VER) && !defined(__clang__)
// Real MSVC (cl.exe) has no GNU statement-expressions / inline asm; use the
// compiler intrinsic. clang-cl defines __clang__ and takes the GNU path below.
#    include <intrin.h>
#    if ARCHITECTURE_AARCH64
#        define ClockTick() ((u64)_ReadStatusReg(ARM64_CNTVCT))
#    else
#        define ClockTick() ((u64)__rdtsc())
#    endif
#elif ARCHITECTURE_X86_64
#    define ClockTick()                                                                                                \
        (__extension__({                                                                                               \
            u32 tsc_lo_, tsc_hi_;                                                                                      \
            __asm__ volatile("rdtsc"                                                                                   \
                             : "=a"(tsc_lo_), "=d"(tsc_hi_));                                                          \
            ((u64)tsc_hi_ << 32) | (u64)tsc_lo_;                                                                       \
        }))
#elif ARCHITECTURE_AARCH64
#    define ClockTick()                                                                                                \
        (__extension__({                                                                                               \
            u64 cnt_;                                                                                                  \
            __asm__ volatile("mrs %0, cntvct_el0"                                                                      \
                             : "=r"(cnt_));                                                                            \
            cnt_;                                                                                                      \
        }))
#else
#    error "ClockTick: unsupported architecture (need x86-64 or AArch64)"
#endif

///
/// Current value of the real-time (wall) clock, in nanoseconds since the
/// Unix epoch (1970-01-01 00:00:00 UTC).
///
/// Unlike `ClockMonoNs`, this clock is anchored to the civil epoch and
/// CAN jump -- it tracks NTP slew and `settimeofday`, so two readings may
/// decrease. Use it for timestamps and as the base for civil/local time,
/// never for measuring elapsed durations (use `ClockMonoNs` for that).
///
/// Thread-safety: safe to call concurrently; reads a kernel/commpage
/// time source and keeps no shared state of its own.
///
/// SUCCESS : Returns a UTC nanosecond timestamp.
/// FAILURE : Aborts via `LOG_FATAL` if the platform clock query fails,
///           the same fail-fast contract as `ClockMonoNs`.
///
/// TAGS: Sys, Clock, Time, Real, Wall, Unix
///
u64 ClockRealNs(void);

///
/// Current wall-clock instant broken down in UTC. Equivalent to
/// `DateTimeFromUnixNs(ClockRealNs(), 0)`.
///
/// SUCCESS : Returns the current UTC `DateTime` (`utc_offset_seconds == 0`).
/// FAILURE : Aborts via `LOG_FATAL` only if the underlying clock query
///           fails (see `ClockRealNs`).
///
/// TAGS: Sys, Clock, Time, Calendar, Utc
///
DateTime ClockUtc(void);

///
/// Current wall-clock instant broken down in the host's local time. The
/// local UTC offset is resolved from `/etc/localtime` (TZif) on POSIX.
/// If the host timezone cannot be determined, falls back to UTC and the
/// result's `utc_offset_seconds` is 0.
///
/// SUCCESS : Returns the current local `DateTime`; `utc_offset_seconds`
///           holds the resolved offset (0 if it fell back to UTC).
/// FAILURE : Aborts via `LOG_FATAL` only if the underlying clock query
///           fails (see `ClockRealNs`). A missing/unreadable timezone is
///           a soft fallback to UTC, not a failure.
///
/// TAGS: Sys, Clock, Time, Calendar, Local, Timezone
///
DateTime ClockLocal(void);

#endif // MISRA_SYS_CLOCK_H
