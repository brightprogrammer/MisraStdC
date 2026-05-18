/// file      : parsers/byte_iter.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.

#ifndef MISRA_PARSERS_BYTE_ITER_H
#define MISRA_PARSERS_BYTE_ITER_H

#include <Misra/Std/Utility/Iter.h>
#include <Misra/Types.h>

/// Iterator over an immutable byte buffer. Layout matches `Iter(const u8)`
/// so the generic Iter macros work on it; the helpers below also expose
/// the common binary-format reads (LE multi-byte integers, LEB/SLEB128,
/// NUL-terminated cstr).
typedef Iter(const u8) ByteIter;

/// Construct a ByteIter over `[data, data + length)`.
#define BYTE_ITER_FROM_MEMORY(data_, length_)                                                                          \
    ((ByteIter) {.data = (data_), .length = (length_), .pos = 0, .alignment = 1, .dir = 1})

static inline size bi_remaining(const ByteIter *c) {
    return c->pos < c->length ? c->length - c->pos : 0;
}

static inline bool bi_take_u8(ByteIter *c, u8 *out) {
    if (bi_remaining(c) < 1)
        return false;
    *out = c->data[c->pos++];
    return true;
}

static inline bool bi_take_u16_le(ByteIter *c, u16 *out) {
    if (bi_remaining(c) < 2)
        return false;
    *out    = (u16)c->data[c->pos] | ((u16)c->data[c->pos + 1] << 8);
    c->pos += 2;
    return true;
}

static inline bool bi_take_u32_le(ByteIter *c, u32 *out) {
    if (bi_remaining(c) < 4)
        return false;
    *out = (u32)c->data[c->pos] | ((u32)c->data[c->pos + 1] << 8) | ((u32)c->data[c->pos + 2] << 16) |
           ((u32)c->data[c->pos + 3] << 24);
    c->pos += 4;
    return true;
}

static inline bool bi_take_u64_le(ByteIter *c, u64 *out) {
    if (bi_remaining(c) < 8)
        return false;
    u64 v = 0;
    for (int i = 0; i < 8; ++i)
        v |= ((u64)c->data[c->pos + (size)i]) << (i * 8);
    *out    = v;
    c->pos += 8;
    return true;
}

/// LEB128 unsigned. Aborts on truncation or width overflow.
static inline bool bi_take_uleb128(ByteIter *c, u64 *out) {
    u64 result = 0;
    u32 shift  = 0;
    while (c->pos < c->length) {
        u8 b    = c->data[c->pos++];
        result |= (u64)(b & 0x7f) << shift;
        if ((b & 0x80) == 0) {
            *out = result;
            return true;
        }
        shift += 7;
        if (shift >= 64)
            return false;
    }
    return false;
}

/// LEB128 signed. Decoded in unsigned space and reinterpreted; the
/// natural `result |= -((i64)1 << shift)` form is signed-shift UB once
/// the shift steps into the sign bit.
static inline bool bi_take_sleb128(ByteIter *c, i64 *out) {
    u64 uresult = 0;
    u32 shift   = 0;
    u8  b       = 0;
    while (c->pos < c->length) {
        b        = c->data[c->pos++];
        uresult |= (u64)(b & 0x7f) << shift;
        shift   += 7;
        if ((b & 0x80) == 0)
            break;
        if (shift >= 64)
            return false;
    }
    if ((b & 0x80) != 0)
        return false;
    if (shift < 64 && (b & 0x40))
        uresult |= ~(u64)0 << shift;
    *out = (i64)uresult;
    return true;
}

/// NUL-terminated string starting at the cursor. Returns the start;
/// advances past the terminator. NULL on truncation.
static inline const char *bi_take_cstr(ByteIter *c) {
    const char *s = (const char *)(c->data + c->pos);
    while (c->pos < c->length && c->data[c->pos] != 0)
        c->pos++;
    if (c->pos >= c->length)
        return NULL;
    c->pos++; // consume NUL
    return s;
}

static inline bool bi_skip(ByteIter *c, u64 n) {
    if (n > bi_remaining(c))
        return false;
    c->pos += n;
    return true;
}

#endif // MISRA_PARSERS_BYTE_ITER_H
