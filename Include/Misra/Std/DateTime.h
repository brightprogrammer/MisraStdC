/// file      : std/datetime.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Calendar value type and its conversions. `DateTime` is a plain POD
/// instant -- it owns nothing, carries no allocator, needs no Deinit.
/// All conversions here are pure arithmetic (no kernel, no timezone
/// database); the wall-clock / local-time readers that produce a
/// `DateTime` from the system clock live in `Sys/Clock.h`.
///
/// `DateTime` is a first-class formatted-I/O type: `WriteFmt("{}", dt)`
/// and `StrReadFmt(in, "{}", dt)` render / parse it as ISO 8601 (see the
/// `_write_DateTime` / `_read_DateTime` arms wired into `Std/Io.h`).

#ifndef MISRA_STD_DATETIME_H
#define MISRA_STD_DATETIME_H

#include <Misra/Types.h>

///
/// A broken-down calendar instant. Plain value type -- copy and discard
/// freely; there is no Init/Deinit.
///
/// year                : Proleptic Gregorian year (e.g. 2026).
/// month               : 1..12.
/// day                 : 1..31.
/// hour                : 0..23.
/// minute              : 0..59.
/// second              : 0..60 (60 only across a leap second).
/// weekday             : 0=Sunday .. 6=Saturday.
/// nanosecond          : 0..999999999.
/// utc_offset_seconds  : Offset of this rendering from UTC, east-positive
///                       (0 for a UTC instant; +19800 for IST).
///
/// TAGS: Std, DateTime, Time, Calendar
///
typedef struct DateTime {
    i32 year;
    u8  month;
    u8  day;
    u8  hour;
    u8  minute;
    u8  second;
    u8  weekday;
    u32 nanosecond;
    i32 utc_offset_seconds;
} DateTime;

///
/// Break a UTC nanoseconds-since-epoch value down into a `DateTime`,
/// rendered at the fixed offset `utc_offset_seconds` (east-positive).
/// Pass 0 to render in UTC.
///
/// unix_ns[in]            : Nanoseconds since the Unix epoch (UTC).
/// utc_offset_seconds[in] : Offset to apply before breaking down; recorded
///                          verbatim in the result's `utc_offset_seconds`.
///
/// SUCCESS : Returns the corresponding `DateTime`.
/// FAILURE : Cannot fail.
///
/// TAGS: Std, DateTime, Convert
///
DateTime DateTimeFromUnixNs(u64 unix_ns, i32 utc_offset_seconds);

///
/// Inverse of `DateTimeFromUnixNs`: collapse a `DateTime` back to UTC
/// nanoseconds since the Unix epoch. The instant's `utc_offset_seconds`
/// is subtracted so the result is always UTC, independent of how the
/// `DateTime` was rendered. Round-trips exactly with `DateTimeFromUnixNs`.
///
/// dt[in] : Instant to collapse.
///
/// SUCCESS : Returns UTC nanoseconds since the Unix epoch.
/// FAILURE : Cannot fail.
///
/// TAGS: Std, DateTime, Convert
///
u64 DateTimeToUnixNs(DateTime dt);

///
/// Order two instants on the absolute (UTC) timeline, regardless of each
/// one's `utc_offset_seconds`.
///
/// SUCCESS : Returns -1 if `a` precedes `b`, +1 if it follows, 0 if they
///           are the same instant.
/// FAILURE : Cannot fail.
///
/// TAGS: Std, DateTime, Compare
///
i32 DateTimeCompare(DateTime a, DateTime b);

///
/// Signed nanosecond difference `a - b` on the absolute (UTC) timeline.
///
/// SUCCESS : Returns `DateTimeToUnixNs(a) - DateTimeToUnixNs(b)` as i64.
/// FAILURE : Cannot fail.
///
/// TAGS: Std, DateTime, Compare
///
i64 DateTimeDiffNs(DateTime a, DateTime b);

///
/// Advance an instant by `delta_ns` nanoseconds (negative to rewind),
/// preserving its `utc_offset_seconds` in the result.
///
/// SUCCESS : Returns the shifted `DateTime`.
/// FAILURE : Cannot fail.
///
/// TAGS: Std, DateTime, Arithmetic
///
DateTime DateTimeAddNs(DateTime dt, i64 delta_ns);

///
/// Whether `year` is a Gregorian leap year.
///
/// SUCCESS : Returns `true` for a leap year, `false` otherwise.
/// FAILURE : Cannot fail.
///
/// TAGS: Std, DateTime, Calendar
///
bool DateTimeIsLeapYear(i32 year);

///
/// Number of days in month `month` (1..12) of `year`, accounting for
/// leap years.
///
/// SUCCESS : Returns 28..31. Returns 0 if `month` is outside 1..12.
/// FAILURE : Cannot fail (out-of-range month yields 0).
///
/// TAGS: Std, DateTime, Calendar
///
u32 DateTimeDaysInMonth(i32 year, u32 month);

///
/// Ordinal day of the year for `dt` (Jan 1 == 1).
///
/// SUCCESS : Returns 1..366.
/// FAILURE : Cannot fail.
///
/// TAGS: Std, DateTime, Calendar
///
u16 DateTimeYearDay(DateTime dt);

#endif // MISRA_STD_DATETIME_H
