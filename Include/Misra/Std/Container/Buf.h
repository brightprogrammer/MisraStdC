/// file      : std/container/buf.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// `Buf` is the binary analogue of `Str`: a growable container of
/// bytes built on `Vec(u8)`. Use it whenever code is producing or
/// holding raw wire data. Cursor reads use `BufIter`, which can be
/// constructed from a `Buf` or from any `(u8 *, size)` pair.

#ifndef MISRA_STD_CONTAINER_BUF_H
#define MISRA_STD_CONTAINER_BUF_H

#include <Misra/Std/Container/Vec.h>
#include <Misra/Std/Io.h>
#include <Misra/Std/Memory.h>
#include <Misra/Std/Utility/Iter.h>
#include <Misra/Types.h>

typedef Vec(u8) Buf;

/// Iterator over an immutable byte buffer. Layout matches `Iter(const u8)`
/// so the generic Iter macros work on it.
typedef Iter(const u8) BufIter;

// ---------------------------------------------------------------------------
// Construction / lifecycle
// ---------------------------------------------------------------------------

#define BufInit(...)     VecInit(__VA_ARGS__)
#define BufDeinit(b)     VecDeinit(b)
#define BufClear(b)      VecClear(b)
#define BufLength(b)     ((b)->length)
#define BufData(b)       ((b)->data)
#define BufReserve(b, n) VecReserve((b), (n))

/// Construct a BufIter over `[data, data + length)`.
#define BufIterFromMemory(data_, length_)                                                                              \
    ((BufIter) {.data = (data_), .length = (length_), .pos = 0, .alignment = 1, .dir = 1})

/// Construct a BufIter over a Buf's bytes.
#define BufIterFromBuf(b_) BufIterFromMemory((const u8 *)(b_)->data, (b_)->length)

// ---------------------------------------------------------------------------
// Single-byte / bulk push helpers
// ---------------------------------------------------------------------------

static inline bool BufPushByte(Buf *b, u8 v) {
    return VecPushBackR(b, v);
}

static inline bool BufPushBytes(Buf *b, const u8 *data, size n) {
    for (size i = 0; i < n; ++i) {
        if (!VecPushBackR(b, data[i])) {
            return false;
        }
    }
    return true;
}

// ---------------------------------------------------------------------------
// Read primitives (operate on a BufIter)
// ---------------------------------------------------------------------------

static inline bool BufReadU8(BufIter *c, u8 *out) {
    if (c->pos >= c->length) {
        return false;
    }
    *out = c->data[c->pos++];
    return true;
}

static inline bool BufReadU16LE(BufIter *c, u16 *out) {
    if (c->pos + 2 > c->length) {
        return false;
    }
    *out    = (u16)c->data[c->pos] | ((u16)c->data[c->pos + 1] << 8);
    c->pos += 2;
    return true;
}

static inline bool BufReadU16BE(BufIter *c, u16 *out) {
    if (c->pos + 2 > c->length) {
        return false;
    }
    *out    = ((u16)c->data[c->pos] << 8) | (u16)c->data[c->pos + 1];
    c->pos += 2;
    return true;
}

static inline bool BufReadU32LE(BufIter *c, u32 *out) {
    if (c->pos + 4 > c->length) {
        return false;
    }
    *out = (u32)c->data[c->pos] | ((u32)c->data[c->pos + 1] << 8) | ((u32)c->data[c->pos + 2] << 16) |
           ((u32)c->data[c->pos + 3] << 24);
    c->pos += 4;
    return true;
}

static inline bool BufReadU32BE(BufIter *c, u32 *out) {
    if (c->pos + 4 > c->length) {
        return false;
    }
    *out = ((u32)c->data[c->pos] << 24) | ((u32)c->data[c->pos + 1] << 16) | ((u32)c->data[c->pos + 2] << 8) |
           (u32)c->data[c->pos + 3];
    c->pos += 4;
    return true;
}

static inline bool BufReadU64LE(BufIter *c, u64 *out) {
    if (c->pos + 8 > c->length) {
        return false;
    }
    u64 v = 0;
    for (int i = 0; i < 8; ++i) {
        v |= ((u64)c->data[c->pos + (size)i]) << (i * 8);
    }
    *out    = v;
    c->pos += 8;
    return true;
}

static inline bool BufReadU64BE(BufIter *c, u64 *out) {
    if (c->pos + 8 > c->length) {
        return false;
    }
    u64 v = 0;
    for (int i = 0; i < 8; ++i) {
        v = (v << 8) | (u64)c->data[c->pos + (size)i];
    }
    *out    = v;
    c->pos += 8;
    return true;
}

/// LEB128 unsigned. Fails on truncation or width overflow.
static inline bool BufReadULeb128(BufIter *c, u64 *out) {
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
        if (shift >= 64) {
            return false;
        }
    }
    return false;
}

/// LEB128 signed. Decoded in unsigned space and reinterpreted to
/// avoid signed-shift UB at high bit positions.
static inline bool BufReadSLeb128(BufIter *c, i64 *out) {
    u64 uresult = 0;
    u32 shift   = 0;
    u8  b       = 0;
    while (c->pos < c->length) {
        b        = c->data[c->pos++];
        uresult |= (u64)(b & 0x7f) << shift;
        shift   += 7;
        if ((b & 0x80) == 0) {
            break;
        }
        if (shift >= 64) {
            return false;
        }
    }
    if ((b & 0x80) != 0) {
        return false;
    }
    if (shift < 64 && (b & 0x40)) {
        uresult |= ~(u64)0 << shift;
    }
    *out = (i64)uresult;
    return true;
}

/// NUL-terminated string starting at the cursor. Returns the start;
/// advances past the terminator. NULL on truncation.
static inline const char *BufReadCstr(BufIter *c) {
    const char *s = (const char *)(c->data + c->pos);
    while (c->pos < c->length && c->data[c->pos] != 0) {
        c->pos++;
    }
    if (c->pos >= c->length) {
        return NULL;
    }
    c->pos++; // consume NUL
    return s;
}

// ---------------------------------------------------------------------------
// Write primitives (operate on a Buf)
// ---------------------------------------------------------------------------

static inline bool BufWriteU8(Buf *b, u8 v) {
    return VecPushBackR(b, v);
}

static inline bool BufWriteU16LE(Buf *b, u16 v) {
    return VecPushBackR(b, (u8)(v & 0xFFu)) && VecPushBackR(b, (u8)((v >> 8) & 0xFFu));
}

static inline bool BufWriteU16BE(Buf *b, u16 v) {
    return VecPushBackR(b, (u8)((v >> 8) & 0xFFu)) && VecPushBackR(b, (u8)(v & 0xFFu));
}

static inline bool BufWriteU32LE(Buf *b, u32 v) {
    for (int i = 0; i < 4; ++i) {
        if (!VecPushBackR(b, (u8)((v >> (i * 8)) & 0xFFu))) {
            return false;
        }
    }
    return true;
}

static inline bool BufWriteU32BE(Buf *b, u32 v) {
    for (int i = 3; i >= 0; --i) {
        if (!VecPushBackR(b, (u8)((v >> (i * 8)) & 0xFFu))) {
            return false;
        }
    }
    return true;
}

static inline bool BufWriteU64LE(Buf *b, u64 v) {
    for (int i = 0; i < 8; ++i) {
        if (!VecPushBackR(b, (u8)((v >> (i * 8)) & 0xFFu))) {
            return false;
        }
    }
    return true;
}

static inline bool BufWriteU64BE(Buf *b, u64 v) {
    for (int i = 7; i >= 0; --i) {
        if (!VecPushBackR(b, (u8)((v >> (i * 8)) & 0xFFu))) {
            return false;
        }
    }
    return true;
}

/// LEB128 unsigned: emit up to 10 bytes, MSB clear on the last byte.
static inline bool BufWriteULeb128(Buf *b, u64 v) {
    do {
        u8 byte_   = (u8)(v & 0x7fu);
        v        >>= 7;
        if (v) {
            byte_ |= 0x80;
        }
        if (!VecPushBackR(b, byte_)) {
            return false;
        }
    } while (v);
    return true;
}

/// LEB128 signed: stops once the sign bit of the payload matches the
/// next discarded bit.
static inline bool BufWriteSLeb128(Buf *b, i64 v) {
    bool more = true;
    while (more) {
        u8   byte_      = (u8)(v & 0x7fu);
        bool sign_bit   = (byte_ & 0x40u) != 0;
        v             >>= 7;
        if ((v == 0 && !sign_bit) || (v == -1 && sign_bit)) {
            more = false;
        } else {
            byte_ |= 0x80;
        }
        if (!VecPushBackR(b, byte_)) {
            return false;
        }
    }
    return true;
}

/// Write a NUL-terminated string + the terminator.
static inline bool BufWriteCstr(Buf *b, const char *s) {
    while (*s) {
        if (!VecPushBackR(b, (u8)*s)) {
            return false;
        }
        ++s;
    }
    return VecPushBackR(b, (u8)0);
}

// ---------------------------------------------------------------------------
// Formatted read/write/append/patch
//
// `BufReadFmt(it, ...)` consumes raw bytes from a cursor.
// `BufAppendFmt(buf, ...)` adds raw bytes to the end of a Buf.
// `BufWriteFmt(buf, ...)` clears the Buf first, then appends.
// `BufPatchFmt(buf, offset, ...)` overwrites existing bytes at offset.
//
// Format string accepts only `{<Nr}` (LE) and `{>Nr}` (BE) directives,
// where N is 1, 2, 4, or 8. The destination variable's natural width
// must match the spec width.
// ---------------------------------------------------------------------------

bool buf_read_fmt(BufIter *iter, const char *fmtstr, TypeSpecificIO *argv, u64 argc);
bool buf_append_fmt(Buf *out, const char *fmtstr, TypeSpecificIO *argv, u64 argc);
bool buf_write_fmt(Buf *out, const char *fmtstr, TypeSpecificIO *argv, u64 argc);
bool buf_patch_fmt(Buf *out, size offset, const char *fmtstr, TypeSpecificIO *argv, u64 argc);

#define BufReadFmt(iter, ...) BufReadFmt_IMPL1((iter), __VA_ARGS__)
#define BufReadFmt_IMPL1(iter, fmtstr, ...)                                                                            \
    BufReadFmt_IMPL2(                                                                                                  \
        (iter),                                                                                                        \
        fmtstr,                                                                                                        \
        ((TypeSpecificIO[]) {                                                                                          \
            APPLY_MACRO_FOREACH(IOFMT_APPEND_COMMA, __VA_ARGS__) {NULL, NULL, NULL}                                    \
    })                                                                                                             \
    )
#define BufReadFmt_IMPL2(iter, fmtstr, varr)                                                                           \
    buf_read_fmt((iter), (fmtstr), &(varr)[0], sizeof(varr) / sizeof(TypeSpecificIO) - 1)

#define BufAppendFmt(buf, ...) BufAppendFmt_IMPL1((buf), __VA_ARGS__)
#define BufAppendFmt_IMPL1(buf, fmtstr, ...)                                                                           \
    BufAppendFmt_IMPL2(                                                                                                \
        (buf),                                                                                                         \
        fmtstr,                                                                                                        \
        ((TypeSpecificIO[]) {                                                                                          \
            APPLY_MACRO_FOREACH(IOFMT_LVAL_APPEND_COMMA, __VA_ARGS__) {NULL, NULL, NULL}                               \
    })                                                                                                             \
    )
#define BufAppendFmt_IMPL2(buf, fmtstr, varr)                                                                          \
    buf_append_fmt((buf), (fmtstr), &(varr)[0], sizeof(varr) / sizeof(TypeSpecificIO) - 1)

#define BufWriteFmt(buf, ...) BufWriteFmt_IMPL1((buf), __VA_ARGS__)
#define BufWriteFmt_IMPL1(buf, fmtstr, ...)                                                                            \
    BufWriteFmt_IMPL2(                                                                                                 \
        (buf),                                                                                                         \
        fmtstr,                                                                                                        \
        ((TypeSpecificIO[]) {                                                                                          \
            APPLY_MACRO_FOREACH(IOFMT_LVAL_APPEND_COMMA, __VA_ARGS__) {NULL, NULL, NULL}                               \
    })                                                                                                             \
    )
#define BufWriteFmt_IMPL2(buf, fmtstr, varr)                                                                           \
    buf_write_fmt((buf), (fmtstr), &(varr)[0], sizeof(varr) / sizeof(TypeSpecificIO) - 1)

#define BufPatchFmt(buf, offset, ...) BufPatchFmt_IMPL1((buf), (offset), __VA_ARGS__)
#define BufPatchFmt_IMPL1(buf, offset, fmtstr, ...)                                                                    \
    BufPatchFmt_IMPL2(                                                                                                 \
        (buf),                                                                                                         \
        (offset),                                                                                                      \
        fmtstr,                                                                                                        \
        ((TypeSpecificIO[]) {                                                                                          \
            APPLY_MACRO_FOREACH(IOFMT_LVAL_APPEND_COMMA, __VA_ARGS__) {NULL, NULL, NULL}                               \
    })                                                                                                             \
    )
#define BufPatchFmt_IMPL2(buf, offset, fmtstr, varr)                                                                   \
    buf_patch_fmt((buf), (offset), (fmtstr), &(varr)[0], sizeof(varr) / sizeof(TypeSpecificIO) - 1)

#endif // MISRA_STD_CONTAINER_BUF_H
