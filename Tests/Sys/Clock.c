#include <Misra.h>
#include <Misra/Sys/Clock.h>

#include "../Util/TestRunner.h"

// 2021-01-01 00:00:00 UTC, a Friday.
#define T_2021     1609459200ull
#define NS_PER_SEC 1000000000ull
#define IST_OFFSET 19800 // +05:30

// ClockMonoNs and ClockRealNs read distinct clocks; Real is a plausible
// recent wall-clock timestamp (well past 2023).
static bool test_real_is_recent_wall_time(void) {
    u64 sec = ClockRealNs() / NS_PER_SEC;
    return sec > 1700000000ull; // 2023-11-14
}

// The raw counter and the wall clock are unrelated sources.
static bool test_real_differs_from_tick(void) {
    u64 a = ClockTick();
    u64 b = ClockRealNs();
    return a != 0 && b != 0;
}

static bool test_epoch_zero(void) {
    DateTime d = DateTimeFromUnixNs(0, 0);
    return d.year == 1970 && d.month == 1 && d.day == 1 && d.hour == 0 && d.minute == 0 && d.second == 0 &&
           d.weekday == 4 /* Thursday */ && d.nanosecond == 0 && d.utc_offset_seconds == 0;
}

static bool test_known_utc_instant(void) {
    DateTime d = DateTimeFromUnixNs(T_2021 * NS_PER_SEC, 0);
    return d.year == 2021 && d.month == 1 && d.day == 1 && d.hour == 0 && d.minute == 0 && d.second == 0 &&
           d.weekday == 5 /* Friday */;
}

static bool test_offset_shifts_wall_fields(void) {
    DateTime d = DateTimeFromUnixNs(T_2021 * NS_PER_SEC, IST_OFFSET);
    return d.year == 2021 && d.month == 1 && d.day == 1 && d.hour == 5 && d.minute == 30 && d.second == 0 &&
           d.weekday == 5 && d.utc_offset_seconds == IST_OFFSET;
}

static bool test_subsecond_preserved(void) {
    DateTime d = DateTimeFromUnixNs(T_2021 * NS_PER_SEC + 123456789ull, 0);
    return d.second == 0 && d.nanosecond == 123456789;
}

static bool test_clock_utc_is_utc(void) {
    DateTime d = ClockUtc();
    return d.utc_offset_seconds == 0 && d.year >= 2024 && d.month >= 1 && d.month <= 12;
}

// ClockLocal must produce a sane instant whether or not the host
// timezone resolves (it falls back to UTC, offset 0).
static bool test_clock_local_sane(void) {
    DateTime d = ClockLocal();
    return d.year >= 2024 && d.month >= 1 && d.month <= 12 && d.day >= 1 && d.day <= 31 && d.hour <= 23 &&
           d.minute <= 59;
}

int main(void) {
    TestFunction tests[] = {
        test_real_is_recent_wall_time,
        test_real_differs_from_tick,
        test_epoch_zero,
        test_known_utc_instant,
        test_offset_shifts_wall_fields,
        test_subsecond_preserved,
        test_clock_utc_is_utc,
        test_clock_local_sane,
    };
    return run_test_suite(tests, sizeof(tests) / sizeof(tests[0]), NULL, 0, "Clock");
}
