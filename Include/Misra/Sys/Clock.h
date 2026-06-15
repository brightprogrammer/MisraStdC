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

#endif // MISRA_SYS_CLOCK_H
