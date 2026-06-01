/// file      : sys/_ipparse.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// IP-literal parsers shared between Sys/Socket (numeric-address fast
/// path) and Sys/Dns (resolver fallback). Underscore-prefixed so the
/// build does not install it under Include/Misra/.

#ifndef MISRA_SYS_IP_PARSE_H
#define MISRA_SYS_IP_PARSE_H

#include <Misra/Std/Zstr.h>
#include <Misra/Types.h>

/// Decode one hex digit. Returns -1 on non-hex input so callers can
/// branch off the negative-sentinel; the i32 return is wider than the
/// 0..15 range to make that sentinel representable.
static inline i32 hex_nibble_value(char c) {
    if (c >= '0' && c <= '9')
        return c - '0';
    if (c >= 'a' && c <= 'f')
        return 10 + (c - 'a');
    if (c >= 'A' && c <= 'F')
        return 10 + (c - 'A');
    return -1;
}

/// Parse "a.b.c.d" into four octets. Empty / NULL / out-of-range / any
/// trailing bytes -> false.
static inline bool parse_ipv4(Zstr s, u8 octets[4]) {
    if (!s)
        return false;
    for (i32 i = 0; i < 4; ++i) {
        if (*s < '0' || *s > '9')
            return false;
        u32 v = 0;
        while (*s >= '0' && *s <= '9') {
            v = v * 10 + (u32)(*s - '0');
            if (v > 255)
                return false;
            ++s;
        }
        octets[i] = (u8)v;
        if (i < 3) {
            if (*s != '.')
                return false;
            ++s;
        }
    }
    return *s == '\0';
}

/// Parse an IPv6 textual address into 16 bytes (network order). Handles
/// RFC 5952 "::" compression. Does not handle zone IDs or embedded
/// IPv4-in-IPv6.
static inline bool parse_ipv6(Zstr s, u8 bytes[16]) {
    if (!s)
        return false;
    u16  before_cc[8];
    u16  after_cc[8];
    i32  before_n = 0;
    i32  after_n  = 0;
    bool seen_cc  = false;
    u16 *slot     = before_cc;
    i32 *slot_n   = &before_n;

    if (s[0] == ':' && s[1] == ':') {
        seen_cc  = true;
        slot     = after_cc;
        slot_n   = &after_n;
        s       += 2;
        if (*s == '\0') {
            for (i32 i = 0; i < 16; ++i)
                bytes[i] = 0;
            return true;
        }
    }

    for (;;) {
        if (*slot_n >= 8)
            return false;
        i32 v        = 0;
        i32 n_digits = 0;
        while (n_digits < 4) {
            i32 nib = hex_nibble_value(*s);
            if (nib < 0)
                break;
            v = (v << 4) | nib;
            ++s;
            ++n_digits;
        }
        if (n_digits == 0)
            return false;
        slot[(*slot_n)++] = (u16)v;
        if (*s == '\0')
            break;
        if (*s != ':')
            return false;
        ++s;
        if (*s == ':') {
            if (seen_cc)
                return false;
            seen_cc = true;
            slot    = after_cc;
            slot_n  = &after_n;
            ++s;
            if (*s == '\0')
                break;
        }
    }

    if (!seen_cc) {
        if (before_n != 8)
            return false;
        for (i32 i = 0; i < 8; ++i) {
            bytes[i * 2]     = (u8)(before_cc[i] >> 8);
            bytes[i * 2 + 1] = (u8)(before_cc[i] & 0xFFu);
        }
        return true;
    }

    if (before_n + after_n > 7)
        return false;
    i32 mid_zeros = 8 - before_n - after_n;
    i32 idx       = 0;
    for (i32 i = 0; i < before_n; ++i, ++idx) {
        bytes[idx * 2]     = (u8)(before_cc[i] >> 8);
        bytes[idx * 2 + 1] = (u8)(before_cc[i] & 0xFFu);
    }
    for (i32 i = 0; i < mid_zeros; ++i, ++idx) {
        bytes[idx * 2]     = 0;
        bytes[idx * 2 + 1] = 0;
    }
    for (i32 i = 0; i < after_n; ++i, ++idx) {
        bytes[idx * 2]     = (u8)(after_cc[i] >> 8);
        bytes[idx * 2 + 1] = (u8)(after_cc[i] & 0xFFu);
    }
    return true;
}

#endif // MISRA_SYS_IP_PARSE_H
