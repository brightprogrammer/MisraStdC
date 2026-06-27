/// file      : parsers/tzif.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Minimal TZif (`/etc/localtime`) reader, just enough to answer "what
/// is the host's local UTC offset at instant T". Backs `ClockLocal` in
/// Sys/Clock.c.
///
/// Scope is deliberately small (see CODING-CONVENTIONS "over-engineering
/// is not entertained"): it resolves the offset from the transition
/// table only. The trailing POSIX-TZ footer string (RFC 8536 3.3),
/// which governs instants past the last baked transition, is NOT yet
/// evaluated -- past-table instants fall back to the last/standard
/// ttinfo. The transition table on a current tzdata build extends years
/// out, so "now" is covered.

#ifndef MISRA_PARSERS_TZIF_H
#define MISRA_PARSERS_TZIF_H

#include <Misra/Std/Allocator.h>
#include <Misra/Types.h>

///
/// Resolve the host's local UTC offset (seconds east of UTC, so IST is
/// +19800) in effect at `unix_seconds`, by reading and parsing
/// `/etc/localtime`.
///
/// unix_seconds[in]       : Instant to resolve, in seconds since the
///                          Unix epoch (UTC).
/// out_offset_seconds[out]: Receives the offset on success; untouched on
///                          failure.
/// alloc[in]              : Allocator backing the file read + transient
///                          parse tables. Fully released before return.
///
/// SUCCESS : Returns `true`; `*out_offset_seconds` holds the offset for
///           the time type in effect at `unix_seconds`.
/// FAILURE : Returns `false` and leaves `*out_offset_seconds` unchanged
///           when `/etc/localtime` is absent, unreadable, or not a valid
///           TZif v1/v2+ stream. Logs via `LOG_ERROR`; never aborts on
///           malformed file data.
///
/// TAGS: Parsers, Tzif, Time, Local, Timezone
///
bool TzifLocalOffsetSeconds(i64 unix_seconds, i32 *out_offset_seconds, Allocator *alloc);

///
/// Resolve the local UTC offset in effect at `unix_seconds` from an
/// in-memory TZif image (`data`, `len` bytes) -- the parse core behind
/// `TzifLocalOffsetSeconds`, exposed for callers that already hold the
/// bytes (and for tests that feed synthetic images).
///
/// data[in]               : TZif v1/v2+ image bytes.
/// len[in]                : Byte length of `data`.
/// unix_seconds[in]       : Instant to resolve (seconds since Unix epoch).
/// out_offset_seconds[out]: Receives the offset on success; untouched on
///                          failure.
///
/// SUCCESS : Returns `true`; `*out_offset_seconds` holds the resolved
///           offset (seconds east of UTC).
/// FAILURE : Returns `false` (and leaves the output untouched) on a NULL
///           argument or a malformed / truncated TZif image. Logs via
///           `LOG_ERROR`; never aborts on bad data.
///
/// TAGS: Parsers, Tzif, Time, Local, Timezone
///
bool TzifOffsetFromBuf(const u8 *data, size len, i64 unix_seconds, i32 *out_offset_seconds);

#endif // MISRA_PARSERS_TZIF_H
