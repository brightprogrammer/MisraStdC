#include <Misra.h>
#include <Misra/Std/Allocator/Default.h>
#include <Misra/Std/DateTime.h>
#include <Misra/Std/Io.h>

#include "../Util/TestRunner.h"

#define NS_PER_SEC 1000000000ull
#define T_2021     1609459200ull // 2021-01-01 00:00:00 UTC (a Friday)
#define IST        19800         // +05:30
#define MST        (-25200)      // -07:00

// FromUnixNs -> ToUnixNs is the identity, for several offsets.
static bool test_unix_roundtrip(void) {
    u64 samples[] = {0, T_2021 * NS_PER_SEC, T_2021 * NS_PER_SEC + 123456789ull, 4102444800ull * NS_PER_SEC};
    i32 offsets[] = {0, IST, MST, 3600};
    for (u32 s = 0; s < sizeof(samples) / sizeof(samples[0]); ++s) {
        for (u32 o = 0; o < sizeof(offsets) / sizeof(offsets[0]); ++o) {
            DateTime d = DateTimeFromUnixNs(samples[s], offsets[o]);
            if (DateTimeToUnixNs(d) != samples[s]) {
                return false;
            }
        }
    }
    return true;
}

static bool test_to_unix_known(void) {
    DateTime epoch = {.year = 1970, .month = 1, .day = 1};
    DateTime day1  = {.year = 1970, .month = 1, .day = 2};
    return DateTimeToUnixNs(epoch) == 0 && DateTimeToUnixNs(day1) == 86400ull * NS_PER_SEC;
}

static bool test_compare(void) {
    DateTime a = DateTimeFromUnixNs(T_2021 * NS_PER_SEC, 0);
    DateTime b = DateTimeFromUnixNs((T_2021 + 1) * NS_PER_SEC, 0);
    // Same instant rendered in two zones compares equal.
    DateTime c = DateTimeFromUnixNs(T_2021 * NS_PER_SEC, IST);
    return DateTimeCompare(a, b) == -1 && DateTimeCompare(b, a) == 1 && DateTimeCompare(a, c) == 0;
}

static bool test_diff(void) {
    DateTime a = DateTimeFromUnixNs(T_2021 * NS_PER_SEC, 0);
    DateTime b = DateTimeFromUnixNs((T_2021 + 90) * NS_PER_SEC, 0);
    return DateTimeDiffNs(b, a) == 90ll * (i64)NS_PER_SEC && DateTimeDiffNs(a, b) == -90ll * (i64)NS_PER_SEC;
}

static bool test_add(void) {
    DateTime a = DateTimeFromUnixNs(T_2021 * NS_PER_SEC, IST);
    DateTime b = DateTimeAddNs(a, 86400ll * (i64)NS_PER_SEC); // +1 day
    return b.day == 2 && b.month == 1 && b.year == 2021 && b.utc_offset_seconds == IST &&
           DateTimeDiffNs(b, a) == 86400ll * (i64)NS_PER_SEC;
}

static bool test_leap_year(void) {
    return DateTimeIsLeapYear(2000) && !DateTimeIsLeapYear(1900) && DateTimeIsLeapYear(2024) &&
           !DateTimeIsLeapYear(2023) && !DateTimeIsLeapYear(2100);
}

static bool test_days_in_month(void) {
    return DateTimeDaysInMonth(2024, 2) == 29 && DateTimeDaysInMonth(2023, 2) == 28 &&
           DateTimeDaysInMonth(2024, 4) == 30 && DateTimeDaysInMonth(2024, 1) == 31 &&
           DateTimeDaysInMonth(2024, 12) == 31 && DateTimeDaysInMonth(2024, 0) == 0 &&
           DateTimeDaysInMonth(2024, 13) == 0;
}

static bool test_year_day(void) {
    DateTime jan1   = {.year = 2024, .month = 1, .day = 1};
    DateTime mar1   = {.year = 2024, .month = 3, .day = 1}; // leap: 31+29+1
    DateTime dec31l = {.year = 2024, .month = 12, .day = 31};
    DateTime dec31  = {.year = 2023, .month = 12, .day = 31};
    return DateTimeYearDay(jan1) == 1 && DateTimeYearDay(mar1) == 61 && DateTimeYearDay(dec31l) == 366 &&
           DateTimeYearDay(dec31) == 365;
}

// Civil <-> epoch round-trip across every month and a spread of years,
// including leap Februarys. Years are >= 1970 because the epoch is u64
// (a negative epoch is unrepresentable, the same domain as ClockRealNs).
static bool test_civil_roundtrip_sweep(void) {
    i32 years[] = {1970, 1971, 1999, 2000, 2023, 2024, 2100, 2200};
    for (u32 yi = 0; yi < sizeof(years) / sizeof(years[0]); ++yi) {
        for (u32 m = 1; m <= 12; ++m) {
            DateTime d = {.year = years[yi], .month = (u8)m, .day = 15, .hour = 13, .minute = 7, .second = 5};
            DateTime r = DateTimeFromUnixNs(DateTimeToUnixNs(d), 0);
            if (r.year != years[yi] || r.month != (u8)m || r.day != 15 || r.hour != 13 || r.minute != 7 ||
                r.second != 5) {
                return false;
            }
        }
    }
    return true;
}

static bool test_leap_day_roundtrip(void) {
    DateTime feb29 = {.year = 2024, .month = 2, .day = 29, .hour = 23, .minute = 59, .second = 59};
    DateTime r     = DateTimeFromUnixNs(DateTimeToUnixNs(feb29), 0);
    return r.year == 2024 && r.month == 2 && r.day == 29 && r.hour == 23 && r.minute == 59 && r.second == 59;
}

static bool test_weekday_known(void) {
    // 1970-01-01 Thu(4), 2021-01-01 Fri(5), 2000-01-01 Sat(6), 2024-02-29
    // Thu(4), 2023-01-01 Sun(0) (the wd==0 case pins the `wd < 0` guard).
    return DateTimeFromUnixNs(0, 0).weekday == 4 && DateTimeFromUnixNs(T_2021 * NS_PER_SEC, 0).weekday == 5 &&
           DateTimeFromUnixNs(946684800ull * NS_PER_SEC, 0).weekday == 6 &&
           DateTimeFromUnixNs(1709164800ull * NS_PER_SEC, 0).weekday == 4 &&
           DateTimeFromUnixNs(1672531200ull * NS_PER_SEC, 0).weekday == 0;
}

// Round-trip every year-boundary day (Jan 1 and Dec 31) across the whole
// u64-representable range. This densely sweeps the era-relative day
// positions, exercising the Gregorian leap-correction terms in the civil
// conversion at every point they could diverge.
static bool test_year_boundaries_sweep(void) {
    for (i32 y = 1970; y <= 2550; ++y) {
        DateTime jan1  = {.year = y, .month = 1, .day = 1};
        DateTime dec31 = {.year = y, .month = 12, .day = 31};
        DateTime rj    = DateTimeFromUnixNs(DateTimeToUnixNs(jan1), 0);
        DateTime rd    = DateTimeFromUnixNs(DateTimeToUnixNs(dec31), 0);
        if (rj.year != y || rj.month != 1 || rj.day != 1) {
            return false;
        }
        if (rd.year != y || rd.month != 12 || rd.day != 31) {
            return false;
        }
    }
    // Exact century / 400-cycle leap boundaries (the era's last day,
    // doe==146096, and the leap Feb 29s) pin the remaining correction term.
    DateTime exact[] = {
        {.year = 2000,  .month = 2, .day = 29},
        {.year = 2100,  .month = 2, .day = 28},
        {.year = 2400,  .month = 2, .day = 29},
        {.year = 2399, .month = 12, .day = 31},
    };
    for (u32 i = 0; i < sizeof(exact) / sizeof(exact[0]); ++i) {
        DateTime r = DateTimeFromUnixNs(DateTimeToUnixNs(exact[i]), 0);
        if (r.year != exact[i].year || r.month != exact[i].month || r.day != exact[i].day) {
            return false;
        }
    }
    return true;
}

// ISO 8601 rendering through the formatted-I/O machinery.
static bool test_iso_write_utc(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Str              s     = StrInit(&alloc);
    DateTime         d     = DateTimeFromUnixNs(T_2021 * NS_PER_SEC, 0);
    StrAppendFmt(&s, "{}", d);
    bool ok = ZstrCompare(StrBegin(&s), "2021-01-01T00:00:00Z") == 0;
    StrDeinit(&s);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

static bool test_iso_write_offset(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Str              s     = StrInit(&alloc);
    DateTime         d     = DateTimeFromUnixNs(T_2021 * NS_PER_SEC, IST);
    StrAppendFmt(&s, "{}", d);
    bool ok = ZstrCompare(StrBegin(&s), "2021-01-01T05:30:00+05:30") == 0;
    StrDeinit(&s);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

static bool test_iso_write_nanos(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Str              s     = StrInit(&alloc);
    DateTime         d     = DateTimeFromUnixNs(T_2021 * NS_PER_SEC + 123456789ull, 0);
    StrAppendFmt(&s, "{}", d);
    bool ok = ZstrCompare(StrBegin(&s), "2021-01-01T00:00:00.123456789Z") == 0;
    StrDeinit(&s);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// Write then read back through the I/O layer: value survives round-trip.
static bool test_iso_io_roundtrip(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Str              s     = StrInit(&alloc);
    DateTime         a     = DateTimeFromUnixNs(T_2021 * NS_PER_SEC + 123456789ull, MST);
    StrAppendFmt(&s, "{}", a);

    DateTime b = {0};
    Zstr     p = StrBegin(&s);
    StrReadFmt(p, "{}", b);

    bool ok = DateTimeCompare(a, b) == 0 && b.utc_offset_seconds == MST && b.nanosecond == 123456789u &&
              b.year == a.year && b.month == a.month && b.day == a.day && b.hour == a.hour;
    StrDeinit(&s);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// Parse an externally-supplied ISO string directly.
static bool test_iso_read_direct(void) {
    Zstr     in = "2026-06-26T17:34:11-07:00";
    DateTime d  = {0};
    StrReadFmt(in, "{}", d);
    return d.year == 2026 && d.month == 6 && d.day == 26 && d.hour == 17 && d.minute == 34 && d.second == 11 &&
           d.nanosecond == 0 && d.utc_offset_seconds == MST;
}

static bool test_days_from_civil_year_zero_leap(void) {
    DateTime y0      = {.year = 0, .month = 1, .day = 1};
    DateTime y1      = {.year = 1, .month = 1, .day = 1};
    i64      span_ns = (i64)(DateTimeToUnixNs(y1) - DateTimeToUnixNs(y0));
    return span_ns == 366LL * 86400LL * (i64)NS_PER_SEC;
}

static bool test_weekday_negative_local_offset(void) {
    return DateTimeFromUnixNs(0, -432000).weekday == 6;
}

int main(void) {
    TestFunction tests[] = {
        test_unix_roundtrip,
        test_days_from_civil_year_zero_leap,
        test_weekday_negative_local_offset,
        test_to_unix_known,
        test_compare,
        test_diff,
        test_add,
        test_leap_year,
        test_days_in_month,
        test_year_day,
        test_civil_roundtrip_sweep,
        test_leap_day_roundtrip,
        test_weekday_known,
        test_year_boundaries_sweep,
        test_iso_write_utc,
        test_iso_write_offset,
        test_iso_write_nanos,
        test_iso_io_roundtrip,
        test_iso_read_direct,
    };
    return run_test_suite(tests, sizeof(tests) / sizeof(tests[0]), NULL, 0, "DateTime");
}
