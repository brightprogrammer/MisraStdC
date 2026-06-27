/// file      : std/datetime.c
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Pure-arithmetic calendar conversions backing `DateTime`. The civil
/// <-> serial-day pair is Howard Hinnant's public-domain algorithm,
/// valid across the full proleptic Gregorian range with no month-length
/// tables and no branches.

#include <Misra/Std/DateTime.h>

#define NS_PER_SEC   1000000000ll
#define SECS_PER_DAY 86400ll

// Days since 1970-01-01 for a civil date.
static i64 days_from_civil(i32 year, u32 month, u32 day) {
    i64 y   = (i64)year - (month <= 2);
    i64 era = (y >= 0 ? y : y - 399) / 400;
    u64 yoe = (u64)(y - era * 400);                                            // [0, 399]
    u64 doy = (u64)((153 * (month + (month > 2 ? -3 : 9)) + 2) / 5 + day - 1); // [0, 365]
    u64 doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;                           // [0, 146096]
    return era * 146097 + (i64)doe - 719468;
}

// Civil date from a serial day count (inverse of days_from_civil).
static void civil_from_days(i64 z, i32 *year, u32 *month, u32 *day) {
    z       += 719468;
    i64 era  = (z >= 0 ? z : z - 146096) / 146097;
    u64 doe  = (u64)(z - era * 146097);                               // [0, 146096]
    u64 yoe  = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365; // [0, 399]
    i64 y    = (i64)yoe + era * 400;
    u64 doy  = doe - (365 * yoe + yoe / 4 - yoe / 100);               // [0, 365]
    u64 mp   = (5 * doy + 2) / 153;                                   // [0, 11]
    *day     = (u32)(doy - (153 * mp + 2) / 5 + 1);                   // [1, 31]
    *month   = (u32)(mp < 10 ? mp + 3 : mp - 9);                      // [1, 12]
    *year    = (i32)(y + (*month <= 2));
}

DateTime DateTimeFromUnixNs(u64 unix_ns, i32 utc_offset_seconds) {
    i64 total_sec = (i64)(unix_ns / (u64)NS_PER_SEC);
    u32 nanos     = (u32)(unix_ns % (u64)NS_PER_SEC);
    i64 local_sec = total_sec + (i64)utc_offset_seconds;

    i64 days = local_sec / SECS_PER_DAY;
    i64 secs = local_sec % SECS_PER_DAY;
    if (secs < 0) {
        secs += SECS_PER_DAY;
        days -= 1;
    }

    i32 year  = 0;
    u32 month = 0, day = 0;
    civil_from_days(days, &year, &month, &day);

    // 1970-01-01 was a Thursday (4 days after Sunday).
    i64 wd = (days + 4) % 7;
    if (wd < 0)
        wd += 7;

    DateTime dt;
    dt.year               = year;
    dt.month              = (u8)month;
    dt.day                = (u8)day;
    dt.hour               = (u8)(secs / 3600);
    dt.minute             = (u8)((secs % 3600) / 60);
    dt.second             = (u8)(secs % 60);
    dt.weekday            = (u8)wd;
    dt.nanosecond         = nanos;
    dt.utc_offset_seconds = utc_offset_seconds;
    return dt;
}

u64 DateTimeToUnixNs(DateTime dt) {
    i64 days      = days_from_civil(dt.year, dt.month, dt.day);
    i64 local_sec = days * SECS_PER_DAY + (i64)dt.hour * 3600 + (i64)dt.minute * 60 + (i64)dt.second;
    i64 utc_sec   = local_sec - (i64)dt.utc_offset_seconds;
    return (u64)utc_sec * (u64)NS_PER_SEC + (u64)dt.nanosecond;
}

i32 DateTimeCompare(DateTime a, DateTime b) {
    u64 ua = DateTimeToUnixNs(a);
    u64 ub = DateTimeToUnixNs(b);
    if (ua < ub)
        return -1;
    if (ua > ub)
        return 1;
    return 0;
}

i64 DateTimeDiffNs(DateTime a, DateTime b) {
    return (i64)DateTimeToUnixNs(a) - (i64)DateTimeToUnixNs(b);
}

DateTime DateTimeAddNs(DateTime dt, i64 delta_ns) {
    u64 base = DateTimeToUnixNs(dt);
    return DateTimeFromUnixNs((u64)((i64)base + delta_ns), dt.utc_offset_seconds);
}

bool DateTimeIsLeapYear(i32 year) {
    return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

u32 DateTimeDaysInMonth(i32 year, u32 month) {
    static const u8 lengths[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if (month < 1 || month > 12)
        return 0;
    if (month == 2 && DateTimeIsLeapYear(year))
        return 29;
    return lengths[month - 1];
}

u16 DateTimeYearDay(DateTime dt) {
    i64 today = days_from_civil(dt.year, dt.month, dt.day);
    i64 jan1  = days_from_civil(dt.year, 1, 1);
    return (u16)(today - jan1 + 1);
}
