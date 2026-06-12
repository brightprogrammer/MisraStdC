/// file      : Tests/Std/ProcMaps.Mutants1.c
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Mutation-hardening for the LINE-PARSING path of `Sys/ProcMaps.c`:
/// `parse_one_line`, `skip_token`, `parse_hex_u64`, `hex_digit_value`.
///
/// Those parsers are `static`, reachable only through `proc_maps_load`,
/// which reads the live (non-deterministic) `/proc/self/maps`. To assert
/// EXACT parsed fields for crafted lines we include the source unit
/// directly so the static parsers become local to this object. The test
/// executable already defines the public symbols (proc_maps_load,
/// ProcMapsDeinit, ProcMapsFindByAddr) via this include, so the linker
/// never pulls the library's ProcMaps object — no duplicate definitions.

#include <Misra.h>
#include <Misra/Std/Allocator/Default.h>
#include <Misra/Sys/ProcMaps.h>
#include <Misra/Std/Utility/StrIter.h>

#include "../Util/TestRunner.h"

// Pull in the unit under test so the static parsers are callable here.
#include "../../Source/Misra/Sys/ProcMaps.c"

// ---------------------------------------------------------------------------
// Helpers — feed a crafted, MUTABLE line buffer to parse_one_line.
// parse_one_line NUL-terminates the path in place, so the buffer must be
// writable (never a string literal).
// ---------------------------------------------------------------------------

// Copy `src` into `dst` (size `cap`), NUL-terminate, return length. No libc.
static u64 copy_line(char *dst, u64 cap, const char *src) {
    u64 n = 0;
    while (src[n] != '\0' && (n + 1) < cap) {
        dst[n] = src[n];
        ++n;
    }
    dst[n] = '\0';
    return n;
}

// ---------------------------------------------------------------------------
// hex_digit_value — exact digit mapping + reject of non-hex.
// ---------------------------------------------------------------------------

bool test_pm1_hex_digit_zero(void) {
    return hex_digit_value('0') == 0;
}

bool test_pm1_hex_digit_nine(void) {
    return hex_digit_value('9') == 9;
}

bool test_pm1_hex_digit_lower_a(void) {
    return hex_digit_value('a') == 10;
}

bool test_pm1_hex_digit_lower_f(void) {
    return hex_digit_value('f') == 15;
}

bool test_pm1_hex_digit_upper_a(void) {
    return hex_digit_value('A') == 10;
}

bool test_pm1_hex_digit_upper_f(void) {
    return hex_digit_value('F') == 15;
}

// Just past the lower-hex range: 'g' must be rejected (-1), not 16.
bool test_pm1_hex_digit_reject_g(void) {
    return hex_digit_value('g') == -1;
}

// Just past the upper-hex range: 'G' must be rejected.
bool test_pm1_hex_digit_reject_upper_g(void) {
    return hex_digit_value('G') == -1;
}

// Below '0': ':' (0x3A is above '9' already; use a char below '0').
bool test_pm1_hex_digit_reject_slash(void) {
    return hex_digit_value('/') == -1; // '/' is '0'-1
}

// Between '9' and 'A' / between 'f' and rest: ':' must be rejected.
bool test_pm1_hex_digit_reject_colon(void) {
    return hex_digit_value(':') == -1;
}

// '`' is 'a'-1 — guards the lower bound of the a..f range.
bool test_pm1_hex_digit_reject_backtick(void) {
    return hex_digit_value('`') == -1;
}

// '@' is 'A'-1 — guards the lower bound of the A..F range.
bool test_pm1_hex_digit_reject_at(void) {
    return hex_digit_value('@') == -1;
}

// 'e' -> 14: distinguishes the +10/-'a' arithmetic from a swap.
bool test_pm1_hex_digit_lower_e(void) {
    return hex_digit_value('e') == 14;
}

// 'C' -> 12: distinguishes the +10/-'A' arithmetic.
bool test_pm1_hex_digit_upper_c(void) {
    return hex_digit_value('C') == 12;
}

// ---------------------------------------------------------------------------
// parse_hex_u64 — multi-digit accumulation, mixed case, reject empty.
// ---------------------------------------------------------------------------

bool test_pm1_hex_u64_multidigit(void) {
    char    buf[] = "deadbeef12345678";
    StrIter si    = StrIterFromCstr(buf, 16);
    u64     out   = 0;
    bool    ok    = parse_hex_u64(&si, &out);
    return ok && out == 0xdeadbeef12345678ULL;
}

bool test_pm1_hex_u64_single_digit(void) {
    char    buf[] = "a";
    StrIter si    = StrIterFromCstr(buf, 1);
    u64     out   = 0;
    bool    ok    = parse_hex_u64(&si, &out);
    return ok && out == 0xaULL;
}

// Mixed-case multi-digit: exercises both lower and upper ranges and the
// (v<<4)|d accumulation order.
bool test_pm1_hex_u64_mixed_case(void) {
    char    buf[] = "AbCdEf";
    StrIter si    = StrIterFromCstr(buf, 6);
    u64     out   = 0;
    bool    ok    = parse_hex_u64(&si, &out);
    return ok && out == 0xabcdefULL;
}

// Leading zeros preserved as value, exact.
bool test_pm1_hex_u64_leading_zeros(void) {
    char    buf[] = "000ff";
    StrIter si    = StrIterFromCstr(buf, 5);
    u64     out   = 0;
    bool    ok    = parse_hex_u64(&si, &out);
    return ok && out == 0xffULL;
}

// Empty / no hex digit -> reject (consumed stays 0).
bool test_pm1_hex_u64_reject_empty(void) {
    char    buf[] = "g";
    StrIter si    = StrIterFromCstr(buf, 1);
    u64     out   = 0xdeadULL;
    bool    ok    = parse_hex_u64(&si, &out);
    // Must return false AND must NOT overwrite *out (only set on success).
    return !ok && out == 0xdeadULL;
}

// Stops at the first non-hex char; advances cursor exactly past digits.
// "12g": value 0x12, then cursor must sit on 'g'.
bool test_pm1_hex_u64_stops_at_nonhex(void) {
    char    buf[] = "12g";
    StrIter si    = StrIterFromCstr(buf, 3);
    u64     out   = 0;
    bool    ok    = parse_hex_u64(&si, &out);
    if (!ok || out != 0x12ULL)
        return false;
    char c = 0;
    // The single ++consumed per digit means index is exactly 2 now.
    return StrIterIndex(&si) == 2 && StrIterPeek(&si, &c) && c == 'g';
}

// Two-digit value where the accumulation order matters: "1f" == 0x1f == 31,
// not 0xf1. Guards the (v<<4)|d shift/accumulate.
bool test_pm1_hex_u64_order(void) {
    char    buf[] = "1f";
    StrIter si    = StrIterFromCstr(buf, 2);
    u64     out   = 0;
    bool    ok    = parse_hex_u64(&si, &out);
    return ok && out == 0x1fULL;
}

// ---------------------------------------------------------------------------
// skip_token — stops on space / tab / newline; consumes nothing else.
// ---------------------------------------------------------------------------

bool test_pm1_skip_token_space(void) {
    char    buf[] = "ab12 rest";
    StrIter si    = StrIterFromCstr(buf, 9);
    skip_token(&si);
    char c = 0;
    return StrIterIndex(&si) == 4 && StrIterPeek(&si, &c) && c == ' ';
}

bool test_pm1_skip_token_tab(void) {
    char    buf[] = "ab12\trest";
    StrIter si    = StrIterFromCstr(buf, 9);
    skip_token(&si);
    char c = 0;
    return StrIterIndex(&si) == 4 && StrIterPeek(&si, &c) && c == '\t';
}

bool test_pm1_skip_token_newline(void) {
    char    buf[] = "ab12\nrest";
    StrIter si    = StrIterFromCstr(buf, 9);
    skip_token(&si);
    char c = 0;
    return StrIterIndex(&si) == 4 && StrIterPeek(&si, &c) && c == '\n';
}

// No terminator at all: token runs to end of buffer.
bool test_pm1_skip_token_to_end(void) {
    char    buf[] = "abcdef";
    StrIter si    = StrIterFromCstr(buf, 6);
    skip_token(&si);
    return StrIterIndex(&si) == 6;
}

// ---------------------------------------------------------------------------
// parse_one_line — full known line, assert EVERY parsed field exactly.
// ---------------------------------------------------------------------------

bool test_pm1_line_full_fields(void) {
    // Distinctive values so any field-offset / shift / bool mutation diverges.
    char         buf[128];
    u64          n  = copy_line(buf, sizeof(buf), "1000-2000 r-xp 0000dead 08:01 12345 /x/y\n");
    StrIter      si = StrIterFromCstr(buf, n); // includes trailing '\n'
    ProcMapEntry e  = {0};
    bool         ok = parse_one_line(&si, &e);
    if (!ok)
        return false;
    if (e.start != 0x1000ULL)
        return false;
    if (e.end != 0x2000ULL)
        return false;
    if (e.file_offset != 0xdeadULL)
        return false;
    // perms r-xp: READ|EXEC|PRIVATE set, WRITE clear.
    if (!(e.perms & PROC_MAP_PERM_READ))
        return false;
    if (e.perms & PROC_MAP_PERM_WRITE)
        return false;
    if (!(e.perms & PROC_MAP_PERM_EXEC))
        return false;
    if (!(e.perms & PROC_MAP_PERM_PRIVATE))
        return false;
    if (ZstrCompare(e.path, "/x/y") != 0)
        return false;
    return true;
}

// perms "rw-s": READ|WRITE set, EXEC clear, PRIVATE clear (shared).
bool test_pm1_line_perms_rw_shared(void) {
    char         buf[128];
    u64          n  = copy_line(buf, sizeof(buf), "0-1 rw-s 0 0:0 0 \n");
    StrIter      si = StrIterFromCstr(buf, n);
    ProcMapEntry e  = {0};
    if (!parse_one_line(&si, &e))
        return false;
    bool read_set    = (e.perms & PROC_MAP_PERM_READ) != 0;
    bool write_set   = (e.perms & PROC_MAP_PERM_WRITE) != 0;
    bool exec_set    = (e.perms & PROC_MAP_PERM_EXEC) != 0;
    bool private_set = (e.perms & PROC_MAP_PERM_PRIVATE) != 0;
    return read_set && write_set && !exec_set && !private_set;
}

// perms "---p": all rwx clear, PRIVATE set. Guards each char-compare.
bool test_pm1_line_perms_none_private(void) {
    char         buf[128];
    u64          n  = copy_line(buf, sizeof(buf), "0-1 ---p 0 0:0 0 \n");
    StrIter      si = StrIterFromCstr(buf, n);
    ProcMapEntry e  = {0};
    if (!parse_one_line(&si, &e))
        return false;
    bool read_set    = (e.perms & PROC_MAP_PERM_READ) != 0;
    bool write_set   = (e.perms & PROC_MAP_PERM_WRITE) != 0;
    bool exec_set    = (e.perms & PROC_MAP_PERM_EXEC) != 0;
    bool private_set = (e.perms & PROC_MAP_PERM_PRIVATE) != 0;
    return !read_set && !write_set && !exec_set && private_set;
}

// Each perm bit isolated: only READ. Guards p0=='r' / |= READ specifically.
bool test_pm1_line_perm_read_only(void) {
    char         buf[128];
    u64          n  = copy_line(buf, sizeof(buf), "0-1 r--s 0 0:0 0 \n");
    StrIter      si = StrIterFromCstr(buf, n);
    ProcMapEntry e  = {0};
    if (!parse_one_line(&si, &e))
        return false;
    return e.perms == (u32)PROC_MAP_PERM_READ;
}

bool test_pm1_line_perm_write_only(void) {
    char         buf[128];
    u64          n  = copy_line(buf, sizeof(buf), "0-1 -w-s 0 0:0 0 \n");
    StrIter      si = StrIterFromCstr(buf, n);
    ProcMapEntry e  = {0};
    if (!parse_one_line(&si, &e))
        return false;
    return e.perms == (u32)PROC_MAP_PERM_WRITE;
}

bool test_pm1_line_perm_exec_only(void) {
    char         buf[128];
    u64          n  = copy_line(buf, sizeof(buf), "0-1 --xs 0 0:0 0 \n");
    StrIter      si = StrIterFromCstr(buf, n);
    ProcMapEntry e  = {0};
    if (!parse_one_line(&si, &e))
        return false;
    return e.perms == (u32)PROC_MAP_PERM_EXEC;
}

bool test_pm1_line_perm_private_only(void) {
    char         buf[128];
    u64          n  = copy_line(buf, sizeof(buf), "0-1 ---p 0 0:0 0 \n");
    StrIter      si = StrIterFromCstr(buf, n);
    ProcMapEntry e  = {0};
    if (!parse_one_line(&si, &e))
        return false;
    return e.perms == (u32)PROC_MAP_PERM_PRIVATE;
}

// Anonymous mapping: no pathname -> path is empty string.
bool test_pm1_line_anon_empty_path(void) {
    char         buf[128];
    u64          n  = copy_line(buf, sizeof(buf), "0-1 ---p 0 0:0 0 \n");
    StrIter      si = StrIterFromCstr(buf, n);
    ProcMapEntry e  = {0};
    if (!parse_one_line(&si, &e))
        return false;
    return e.path != NULL && e.path[0] == '\0';
}

// Pathname with a special bracket name preserved exactly.
bool test_pm1_line_path_stack(void) {
    char         buf[128];
    u64          n  = copy_line(buf, sizeof(buf), "0-1 ---p 0 0:0 0 [stack]\n");
    StrIter      si = StrIterFromCstr(buf, n);
    ProcMapEntry e  = {0};
    if (!parse_one_line(&si, &e))
        return false;
    return ZstrCompare(e.path, "[stack]") == 0;
}

// Path that contains a space must be kept whole (everything after inode ws).
bool test_pm1_line_path_with_space(void) {
    char         buf[128];
    u64          n  = copy_line(buf, sizeof(buf), "0-1 ---p 0 0:0 0 /a b/c\n");
    StrIter      si = StrIterFromCstr(buf, n);
    ProcMapEntry e  = {0};
    if (!parse_one_line(&si, &e))
        return false;
    return ZstrCompare(e.path, "/a b/c") == 0;
}

// Big addresses with hex letters: distinguishes start vs end fields and the
// hex accumulation across many digits.
bool test_pm1_line_big_addrs(void) {
    char         buf[128];
    u64          n = copy_line(buf, sizeof(buf), "7f8b8c000000-7f8b8c021000 r-xp 00001000 08:01 1234 /lib/libc.so.6\n");
    StrIter      si = StrIterFromCstr(buf, n);
    ProcMapEntry e  = {0};
    if (!parse_one_line(&si, &e))
        return false;
    if (e.start != 0x7f8b8c000000ULL)
        return false;
    if (e.end != 0x7f8b8c021000ULL)
        return false;
    if (e.file_offset != 0x1000ULL)
        return false;
    return ZstrCompare(e.path, "/lib/libc.so.6") == 0;
}

// Missing '-' between start and end -> reject. Guards expect_char('-').
bool test_pm1_line_reject_no_dash(void) {
    char         buf[128];
    u64          n  = copy_line(buf, sizeof(buf), "1000 2000 r-xp 0 0:0 0 \n");
    StrIter      si = StrIterFromCstr(buf, n);
    ProcMapEntry e  = {0};
    return !parse_one_line(&si, &e);
}

// No leading hex at all -> reject (parse_hex_u64 on start fails).
bool test_pm1_line_reject_no_start(void) {
    char         buf[128];
    u64          n  = copy_line(buf, sizeof(buf), "xyz-2000 r-xp 0 0:0 0 \n");
    StrIter      si = StrIterFromCstr(buf, n);
    ProcMapEntry e  = {0};
    return !parse_one_line(&si, &e);
}

// perms field shorter than 4 chars -> reject (RemainingLength < 4).
bool test_pm1_line_reject_short_perms(void) {
    char         buf[16];
    u64          n  = copy_line(buf, sizeof(buf), "1000-2000 rw");
    StrIter      si = StrIterFromCstr(buf, n);
    ProcMapEntry e  = {0};
    return !parse_one_line(&si, &e);
}

// Line with no trailing newline (last line of file): still parses, path
// runs to end of buffer.
bool test_pm1_line_no_trailing_newline(void) {
    char         buf[128];
    u64          n  = copy_line(buf, sizeof(buf), "1000-2000 r-xp dead 08:01 99 /z");
    StrIter      si = StrIterFromCstr(buf, n);
    ProcMapEntry e  = {0};
    if (!parse_one_line(&si, &e))
        return false;
    if (e.start != 0x1000ULL || e.end != 0x2000ULL || e.file_offset != 0xdeadULL)
        return false;
    return ZstrCompare(e.path, "/z") == 0;
}

// Two lines back-to-back: after the first parse the cursor must sit at the
// start of the second line (newline consumed). Guards StrIterMustNext at 160
// and the '\n' detection at 154.
bool test_pm1_line_advances_past_newline(void) {
    char         buf[128];
    u64          n  = copy_line(buf, sizeof(buf), "1000-2000 r-xp 0 0:0 0 /a\n3000-4000 rw-p 0 0:0 0 /b\n");
    StrIter      si = StrIterFromCstr(buf, n);
    ProcMapEntry e1 = {0};
    ProcMapEntry e2 = {0};
    if (!parse_one_line(&si, &e1))
        return false;
    if (e1.start != 0x1000ULL || ZstrCompare(e1.path, "/a") != 0)
        return false;
    if (!parse_one_line(&si, &e2))
        return false;
    if (e2.start != 0x3000ULL || e2.end != 0x4000ULL)
        return false;
    return ZstrCompare(e2.path, "/b") == 0;
}

// The in-place NUL: path of the first line must end exactly at "/a" (the
// '\n' became '\0'), not bleed into the next line. Guards line 159 write.
bool test_pm1_line_path_nul_terminated(void) {
    char         buf[128];
    u64          n  = copy_line(buf, sizeof(buf), "1000-2000 r-xp 0 0:0 0 /first\n3000-4000 rw-p 0 0:0 0 /second\n");
    StrIter      si = StrIterFromCstr(buf, n);
    ProcMapEntry e1 = {0};
    if (!parse_one_line(&si, &e1))
        return false;
    // Exact length: "/first" is 6 chars then NUL.
    return ZstrCompare(e1.path, "/first") == 0;
}

int main(void) {
    WriteFmt("[INFO] Starting ProcMaps.Mutants1 tests\n\n");

    TestFunction tests[] = {
        test_pm1_hex_digit_zero,
        test_pm1_hex_digit_nine,
        test_pm1_hex_digit_lower_a,
        test_pm1_hex_digit_lower_f,
        test_pm1_hex_digit_upper_a,
        test_pm1_hex_digit_upper_f,
        test_pm1_hex_digit_reject_g,
        test_pm1_hex_digit_reject_upper_g,
        test_pm1_hex_digit_reject_slash,
        test_pm1_hex_digit_reject_colon,
        test_pm1_hex_digit_reject_backtick,
        test_pm1_hex_digit_reject_at,
        test_pm1_hex_digit_lower_e,
        test_pm1_hex_digit_upper_c,

        test_pm1_hex_u64_multidigit,
        test_pm1_hex_u64_single_digit,
        test_pm1_hex_u64_mixed_case,
        test_pm1_hex_u64_leading_zeros,
        test_pm1_hex_u64_reject_empty,
        test_pm1_hex_u64_stops_at_nonhex,
        test_pm1_hex_u64_order,

        test_pm1_skip_token_space,
        test_pm1_skip_token_tab,
        test_pm1_skip_token_newline,
        test_pm1_skip_token_to_end,

        test_pm1_line_full_fields,
        test_pm1_line_perms_rw_shared,
        test_pm1_line_perms_none_private,
        test_pm1_line_perm_read_only,
        test_pm1_line_perm_write_only,
        test_pm1_line_perm_exec_only,
        test_pm1_line_perm_private_only,
        test_pm1_line_anon_empty_path,
        test_pm1_line_path_stack,
        test_pm1_line_path_with_space,
        test_pm1_line_big_addrs,
        test_pm1_line_reject_no_dash,
        test_pm1_line_reject_no_start,
        test_pm1_line_reject_short_perms,
        test_pm1_line_no_trailing_newline,
        test_pm1_line_advances_past_newline,
        test_pm1_line_path_nul_terminated,
    };

    return run_test_suite(tests, sizeof(tests) / sizeof(tests[0]), NULL, 0, "ProcMaps.Parse");
}
