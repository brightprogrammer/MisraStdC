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
#include <Misra/Std/Memory.h>
#include <Misra/Std/Utility/Iter.h>
#include <Misra/Std/Zstr.h>
#include <Misra/Types.h>

typedef Vec(u8) Buf;

/// Iterator over an immutable byte buffer. Layout matches `Iter(const u8)`
/// so the generic Iter macros work on it.
typedef Iter(const u8) BufIter;

// ---------------------------------------------------------------------------
// Construction / lifecycle
// ---------------------------------------------------------------------------

///
/// Construct an empty `Buf`. The variadic form accepts an optional
/// allocator pointer; with no argument, the allocator is taken from
/// the enclosing `Scope` / `ScopeWith` block via `MisraScope`.
///
/// SUCCESS : Returns an initialized `Buf` with zero length and a
///           lazily-allocated backing store.
/// FAILURE : Macro cannot fail; the backing allocator's failure
///           behaviour applies only on subsequent growth operations.
///
/// TAGS: Buf, Init, Construct, Lifecycle
///
#define BufInit(...) VecInit(__VA_ARGS__)

///
/// Release the backing storage of a `Buf` through its inline allocator.
///
/// SUCCESS : Returns to the caller. `*b` is zeroed.
/// FAILURE : Macro cannot fail. Safe on a zeroed `Buf`.
///
/// TAGS: Buf, Deinit, Lifecycle
///
#define BufDeinit(b) VecDeinit(b)

///
/// Reset length to zero without releasing the backing storage. The
/// capacity is preserved so subsequent pushes can reuse it.
///
/// SUCCESS : Returns to the caller. `b->length` is `0`.
/// FAILURE : Macro cannot fail.
///
/// TAGS: Buf, Clear, Reuse
///
#define BufClear(b) VecClear(b)

// Read-only accessors. The leading `((void)0, ...)` makes each macro a
// comma expression, and the result of a C comma expression is not an
// lvalue -- so `BufLength(b) = n` (or the same for Data / Allocator)
// fails at compile time. Length changes go through `BufResize`; there
// is no setter macro by design.

///
/// Current byte count of `b`. Read-only (the comma-expression form
/// rejects `BufLength(b) = n` at compile time -- use `BufResize`).
///
/// TAGS: Buf, Length, Accessor
///
#define BufLength(b) ((void)0, (b)->length)

///
/// Pointer to the contiguous byte storage backing `b`. Read-only at
/// the macro level; the bytes themselves are mutable through this
/// pointer. Invalidated by any growth (`BufReserve`, `BufResize`,
/// `BufWriteU8`, `BufPushBytes`).
///
/// TAGS: Buf, Data, Accessor
///
#define BufData(b) ((void)0, (b)->data)

///
/// Allocator backing `b`'s storage. Read-only; rebinding the
/// allocator after init is not supported.
///
/// TAGS: Buf, Allocator, Accessor
///
#define BufAllocator(b) ((void)0, (b)->allocator)

///
/// Ensure `b` has capacity for at least `n` bytes without changing
/// its length. Allocates only when the existing capacity is below
/// `n`.
///
/// SUCCESS : Returns `true`; `b->data` may have moved.
/// FAILURE : Returns `false` if the underlying allocator fails to
///           grow the buffer; `*b` is unchanged.
///
/// TAGS: Buf, Reserve, Capacity, Allocation
///
#define BufReserve(b, n) VecReserve((b), (n))

///
/// Set `b`'s length to exactly `n` bytes, growing the backing store
/// if necessary. New bytes are zero-initialized; existing bytes
/// past `n` are dropped.
///
/// SUCCESS : Returns `true`; `b->length == n`. `b->data` may have
///           moved when growing.
/// FAILURE : Returns `false` if the underlying allocator fails to
///           grow the buffer; `*b` is unchanged.
///
/// TAGS: Buf, Resize, Capacity, Allocation
///
#define BufResize(b, n) VecResize((b), (n))

///
/// Construct a `BufIter` over `[data, data + length)`. The iterator
/// borrows the caller's bytes -- ownership is unchanged.
///
/// SUCCESS : Returns a `BufIter` positioned at offset 0 with stride 1
///           and forward direction.
/// FAILURE : Macro cannot fail. Passing a NULL `data_` with non-zero
///           `length_` is a usage error -- subsequent reads will
///           dereference NULL.
///
/// TAGS: Buf, Iter, Construct
///
#define BufIterFromMemory(data_, length_)                                                                              \
    ((BufIter) {.data = (data_), .length = (length_), .pos = 0, .alignment = 1, .dir = 1})

///
/// Construct a `BufIter` over the bytes of `b_`. The iterator borrows
/// `b_`'s storage and is invalidated by any growth of `b_`.
///
/// SUCCESS : Returns a `BufIter` covering `[b_->data, b_->data + b_->length)`.
/// FAILURE : Macro cannot fail.
///
/// TAGS: Buf, Iter, Construct
///
#define BufIterFromBuf(b_) BufIterFromMemory(BufData(b_), BufLength(b_))

// ---------------------------------------------------------------------------
// Single-byte / bulk push helpers
// ---------------------------------------------------------------------------

///
/// Append `n` bytes from `data` to `b` in order. Bails on the first
/// growth failure -- any bytes appended before the failure remain in
/// `b`.
///
/// SUCCESS : Returns `true`; `b->length` increases by `n`.
/// FAILURE : Returns `false` on the first allocator failure; `b->length`
///           reflects whichever bytes were successfully appended before
///           the failure.
///
/// TAGS: Buf, Push, Bytes, Append, Bulk
///
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

///
/// Read a single byte at the cursor and advance one byte.
///
/// c[in,out] : Byte cursor; read from and advanced past the field on success.
/// out[out]  : Receives the decoded value.
///
/// SUCCESS : Returns `true`; `*out` holds the byte at `c->pos` and
///           `c->pos` has advanced by one.
/// FAILURE : Returns `false` when fewer than one byte remains in `c`
///           (`c->pos >= c->length`); `*out` is unchanged and `c->pos`
///           is unchanged.
///
/// TAGS: Buf, Read, U8
///
static inline bool BufReadU8(BufIter *c, u8 *out) {
    if (c->pos >= c->length) {
        return false;
    }
    *out = c->data[c->pos++];
    return true;
}

///
/// Read two little-endian bytes (low byte first) and advance two bytes.
///
/// c[in,out] : Byte cursor; read from and advanced past the field on success.
/// out[out]  : Receives the decoded value.
///
/// SUCCESS : Returns `true`; `*out` holds the decoded `u16` and
///           `c->pos` has advanced by two.
/// FAILURE : Returns `false` when fewer than two bytes remain in `c`;
///           `*out` is unchanged and `c->pos` is unchanged.
///
/// TAGS: Buf, Read, U16, LittleEndian
///
static inline bool BufReadU16LE(BufIter *c, u16 *out) {
    if (c->pos + 2 > c->length) {
        return false;
    }
    *out    = (u16)c->data[c->pos] | ((u16)c->data[c->pos + 1] << 8);
    c->pos += 2;
    return true;
}

///
/// Read two big-endian bytes (high byte first) and advance two bytes.
///
/// c[in,out] : Byte cursor; read from and advanced past the field on success.
/// out[out]  : Receives the decoded value.
///
/// SUCCESS : Returns `true`; `*out` holds the decoded `u16` and
///           `c->pos` has advanced by two.
/// FAILURE : Returns `false` when fewer than two bytes remain in `c`;
///           `*out` is unchanged and `c->pos` is unchanged.
///
/// TAGS: Buf, Read, U16, BigEndian
///
static inline bool BufReadU16BE(BufIter *c, u16 *out) {
    if (c->pos + 2 > c->length) {
        return false;
    }
    *out    = ((u16)c->data[c->pos] << 8) | (u16)c->data[c->pos + 1];
    c->pos += 2;
    return true;
}

///
/// Read four little-endian bytes and advance four bytes.
///
/// c[in,out] : Byte cursor; read from and advanced past the field on success.
/// out[out]  : Receives the decoded value.
///
/// SUCCESS : Returns `true`; `*out` holds the decoded `u32` and
///           `c->pos` has advanced by four.
/// FAILURE : Returns `false` when fewer than four bytes remain in `c`;
///           `*out` is unchanged and `c->pos` is unchanged.
///
/// TAGS: Buf, Read, U32, LittleEndian
///
static inline bool BufReadU32LE(BufIter *c, u32 *out) {
    if (c->pos + 4 > c->length) {
        return false;
    }
    *out = (u32)c->data[c->pos] | ((u32)c->data[c->pos + 1] << 8) | ((u32)c->data[c->pos + 2] << 16) |
           ((u32)c->data[c->pos + 3] << 24);
    c->pos += 4;
    return true;
}

///
/// Read four big-endian bytes and advance four bytes.
///
/// c[in,out] : Byte cursor; read from and advanced past the field on success.
/// out[out]  : Receives the decoded value.
///
/// SUCCESS : Returns `true`; `*out` holds the decoded `u32` and
///           `c->pos` has advanced by four.
/// FAILURE : Returns `false` when fewer than four bytes remain in `c`;
///           `*out` is unchanged and `c->pos` is unchanged.
///
/// TAGS: Buf, Read, U32, BigEndian
///
static inline bool BufReadU32BE(BufIter *c, u32 *out) {
    if (c->pos + 4 > c->length) {
        return false;
    }
    *out = ((u32)c->data[c->pos] << 24) | ((u32)c->data[c->pos + 1] << 16) | ((u32)c->data[c->pos + 2] << 8) |
           (u32)c->data[c->pos + 3];
    c->pos += 4;
    return true;
}

///
/// Read eight little-endian bytes and advance eight bytes.
///
/// c[in,out] : Byte cursor; read from and advanced past the field on success.
/// out[out]  : Receives the decoded value.
///
/// SUCCESS : Returns `true`; `*out` holds the decoded `u64` and
///           `c->pos` has advanced by eight.
/// FAILURE : Returns `false` when fewer than eight bytes remain in `c`;
///           `*out` is unchanged and `c->pos` is unchanged.
///
/// TAGS: Buf, Read, U64, LittleEndian
///
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

///
/// Read eight big-endian bytes and advance eight bytes.
///
/// c[in,out] : Byte cursor; read from and advanced past the field on success.
/// out[out]  : Receives the decoded value.
///
/// SUCCESS : Returns `true`; `*out` holds the decoded `u64` and
///           `c->pos` has advanced by eight.
/// FAILURE : Returns `false` when fewer than eight bytes remain in `c`;
///           `*out` is unchanged and `c->pos` is unchanged.
///
/// TAGS: Buf, Read, U64, BigEndian
///
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

///
/// Read a single byte at the cursor as a two's-complement signed value and
/// advance one byte.
///
/// c[in,out] : Byte cursor; read from and advanced past the field on success.
/// out[out]  : Receives the decoded signed value.
///
/// SUCCESS : Returns `true`; `*out` holds the signed byte at `c->pos` and
///           `c->pos` has advanced by one.
/// FAILURE : Returns `false` when fewer than one byte remains in `c`
///           (`c->pos >= c->length`); `*out` is unchanged and `c->pos`
///           is unchanged.
///
/// TAGS: Buf, Read, I8
///
static inline bool BufReadI8(BufIter *c, i8 *out) {
    u8 raw = 0;
    if (!BufReadU8(c, &raw)) {
        return false;
    }
    *out = (i8)raw;
    return true;
}

///
/// Read two little-endian bytes (low byte first) as a two's-complement signed
/// value and advance two bytes.
///
/// c[in,out] : Byte cursor; read from and advanced past the field on success.
/// out[out]  : Receives the decoded signed value.
///
/// SUCCESS : Returns `true`; `*out` holds the decoded `i16` and
///           `c->pos` has advanced by two.
/// FAILURE : Returns `false` when fewer than two bytes remain in `c`;
///           `*out` is unchanged and `c->pos` is unchanged.
///
/// TAGS: Buf, Read, I16, LittleEndian
///
static inline bool BufReadI16LE(BufIter *c, i16 *out) {
    u16 raw = 0;
    if (!BufReadU16LE(c, &raw)) {
        return false;
    }
    *out = (i16)raw;
    return true;
}

///
/// Read two big-endian bytes (high byte first) as a two's-complement signed
/// value and advance two bytes.
///
/// c[in,out] : Byte cursor; read from and advanced past the field on success.
/// out[out]  : Receives the decoded signed value.
///
/// SUCCESS : Returns `true`; `*out` holds the decoded `i16` and
///           `c->pos` has advanced by two.
/// FAILURE : Returns `false` when fewer than two bytes remain in `c`;
///           `*out` is unchanged and `c->pos` is unchanged.
///
/// TAGS: Buf, Read, I16, BigEndian
///
static inline bool BufReadI16BE(BufIter *c, i16 *out) {
    u16 raw = 0;
    if (!BufReadU16BE(c, &raw)) {
        return false;
    }
    *out = (i16)raw;
    return true;
}

///
/// Read four little-endian bytes as a two's-complement signed value and advance
/// four bytes.
///
/// c[in,out] : Byte cursor; read from and advanced past the field on success.
/// out[out]  : Receives the decoded signed value.
///
/// SUCCESS : Returns `true`; `*out` holds the decoded `i32` and
///           `c->pos` has advanced by four.
/// FAILURE : Returns `false` when fewer than four bytes remain in `c`;
///           `*out` is unchanged and `c->pos` is unchanged.
///
/// TAGS: Buf, Read, I32, LittleEndian
///
static inline bool BufReadI32LE(BufIter *c, i32 *out) {
    u32 raw = 0;
    if (!BufReadU32LE(c, &raw)) {
        return false;
    }
    *out = (i32)raw;
    return true;
}

///
/// Read four big-endian bytes as a two's-complement signed value and advance
/// four bytes.
///
/// c[in,out] : Byte cursor; read from and advanced past the field on success.
/// out[out]  : Receives the decoded signed value.
///
/// SUCCESS : Returns `true`; `*out` holds the decoded `i32` and
///           `c->pos` has advanced by four.
/// FAILURE : Returns `false` when fewer than four bytes remain in `c`;
///           `*out` is unchanged and `c->pos` is unchanged.
///
/// TAGS: Buf, Read, I32, BigEndian
///
static inline bool BufReadI32BE(BufIter *c, i32 *out) {
    u32 raw = 0;
    if (!BufReadU32BE(c, &raw)) {
        return false;
    }
    *out = (i32)raw;
    return true;
}

///
/// Read eight little-endian bytes as a two's-complement signed value and advance
/// eight bytes.
///
/// c[in,out] : Byte cursor; read from and advanced past the field on success.
/// out[out]  : Receives the decoded signed value.
///
/// SUCCESS : Returns `true`; `*out` holds the decoded `i64` and
///           `c->pos` has advanced by eight.
/// FAILURE : Returns `false` when fewer than eight bytes remain in `c`;
///           `*out` is unchanged and `c->pos` is unchanged.
///
/// TAGS: Buf, Read, I64, LittleEndian
///
static inline bool BufReadI64LE(BufIter *c, i64 *out) {
    u64 raw = 0;
    if (!BufReadU64LE(c, &raw)) {
        return false;
    }
    *out = (i64)raw;
    return true;
}

///
/// Read eight big-endian bytes as a two's-complement signed value and advance
/// eight bytes.
///
/// c[in,out] : Byte cursor; read from and advanced past the field on success.
/// out[out]  : Receives the decoded signed value.
///
/// SUCCESS : Returns `true`; `*out` holds the decoded `i64` and
///           `c->pos` has advanced by eight.
/// FAILURE : Returns `false` when fewer than eight bytes remain in `c`;
///           `*out` is unchanged and `c->pos` is unchanged.
///
/// TAGS: Buf, Read, I64, BigEndian
///
static inline bool BufReadI64BE(BufIter *c, i64 *out) {
    u64 raw = 0;
    if (!BufReadU64BE(c, &raw)) {
        return false;
    }
    *out = (i64)raw;
    return true;
}

/// LEB128 unsigned. Fails on truncation or width overflow.
///
/// SUCCESS : Returns `true`; `*out` holds the decoded value and `c->pos`
///           has advanced past the last consumed byte (the one whose
///           high bit was clear).
/// FAILURE : Returns `false` when the encoded value runs past
///           `c->length` before its terminator byte (truncation) or
///           when the shift would reach 64 bits without ever seeing a
///           terminator (width overflow). `*out` is not written; bytes
///           already consumed remain consumed (`c->pos` advances up to
///           the failure point).
///
/// TAGS: Buf, Read, LEB128, Unsigned
///
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
///
/// SUCCESS : Returns `true`; `*out` holds the sign-extended decoded
///           value and `c->pos` has advanced past the terminator
///           byte.
/// FAILURE : Returns `false` on truncation (encoded value runs past
///           `c->length` before the terminator) or width overflow
///           (shift would reach 64 bits without a terminator). `*out`
///           is not written; bytes consumed before the failure remain
///           consumed.
///
/// TAGS: Buf, Read, LEB128, Signed
///
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
///
/// SUCCESS : Returns a `Zstr` pointing into `c->data` at the original
///           cursor position; `c->pos` has advanced past the NUL
///           terminator. The returned pointer is borrowed -- it stays
///           valid for as long as the underlying buffer does.
/// FAILURE : Returns `NULL` when no NUL byte is found before
///           `c->length` (truncation). `c->pos` is left at the end of
///           the buffer.
///
/// TAGS: Buf, Read, Zstr, String
///
static inline Zstr BufReadZstr(BufIter *c) {
    Zstr s = (Zstr)(c->data + c->pos);
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

///
/// Append a single byte to `b`.
///
/// SUCCESS : Returns `true`; `b->length` increases by one.
/// FAILURE : Returns `false` if the underlying allocator fails to
///           grow `b`; `*b` is unchanged.
///
/// TAGS: Buf, Write, Byte
///
static inline bool BufWriteU8(Buf *b, u8 v) {
    return VecPushBackR(b, v);
}

///
/// Append `v` as two little-endian bytes (low byte first).
///
/// SUCCESS : Returns `true`; `b->length` increases by two.
/// FAILURE : Returns `false` on the first allocator failure; any byte
///           appended before the failure remains in `b`.
///
/// TAGS: Buf, Write, U16, LittleEndian
///
static inline bool BufWriteU16LE(Buf *b, u16 v) {
    return VecPushBackR(b, (u8)(v & 0xFFu)) && VecPushBackR(b, (u8)((v >> 8) & 0xFFu));
}

///
/// Append `v` as two big-endian bytes (high byte first).
///
/// SUCCESS : Returns `true`; `b->length` increases by two.
/// FAILURE : Returns `false` on the first allocator failure; any byte
///           appended before the failure remains in `b`.
///
/// TAGS: Buf, Write, U16, BigEndian
///
static inline bool BufWriteU16BE(Buf *b, u16 v) {
    return VecPushBackR(b, (u8)((v >> 8) & 0xFFu)) && VecPushBackR(b, (u8)(v & 0xFFu));
}

///
/// Append `v` as four little-endian bytes.
///
/// SUCCESS : Returns `true`; `b->length` increases by four.
/// FAILURE : Returns `false` on the first allocator failure; bytes
///           appended before the failure remain in `b`.
///
/// TAGS: Buf, Write, U32, LittleEndian
///
static inline bool BufWriteU32LE(Buf *b, u32 v) {
    for (int i = 0; i < 4; ++i) {
        if (!VecPushBackR(b, (u8)((v >> (i * 8)) & 0xFFu))) {
            return false;
        }
    }
    return true;
}

///
/// Append `v` as four big-endian bytes.
///
/// SUCCESS : Returns `true`; `b->length` increases by four.
/// FAILURE : Returns `false` on the first allocator failure; bytes
///           appended before the failure remain in `b`.
///
/// TAGS: Buf, Write, U32, BigEndian
///
static inline bool BufWriteU32BE(Buf *b, u32 v) {
    for (int i = 3; i >= 0; --i) {
        if (!VecPushBackR(b, (u8)((v >> (i * 8)) & 0xFFu))) {
            return false;
        }
    }
    return true;
}

///
/// Append `v` as eight little-endian bytes.
///
/// SUCCESS : Returns `true`; `b->length` increases by eight.
/// FAILURE : Returns `false` on the first allocator failure; bytes
///           appended before the failure remain in `b`.
///
/// TAGS: Buf, Write, U64, LittleEndian
///
static inline bool BufWriteU64LE(Buf *b, u64 v) {
    for (int i = 0; i < 8; ++i) {
        if (!VecPushBackR(b, (u8)((v >> (i * 8)) & 0xFFu))) {
            return false;
        }
    }
    return true;
}

///
/// Append `v` as eight big-endian bytes.
///
/// SUCCESS : Returns `true`; `b->length` increases by eight.
/// FAILURE : Returns `false` on the first allocator failure; bytes
///           appended before the failure remain in `b`.
///
/// TAGS: Buf, Write, U64, BigEndian
///
static inline bool BufWriteU64BE(Buf *b, u64 v) {
    for (int i = 7; i >= 0; --i) {
        if (!VecPushBackR(b, (u8)((v >> (i * 8)) & 0xFFu))) {
            return false;
        }
    }
    return true;
}

/// LEB128 unsigned: emit up to 10 bytes, MSB clear on the last byte.
///
/// SUCCESS : Returns `true`; `b->length` increases by between one and
///           ten bytes depending on the magnitude of `v`. The last
///           emitted byte has its high bit clear.
/// FAILURE : Returns `false` on the first allocator failure; bytes
///           emitted before the failure remain in `b`.
///
/// TAGS: Buf, Write, LEB128, Unsigned
///
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
///
/// SUCCESS : Returns `true`; `b->length` increases by however many
///           bytes the signed LEB128 encoding of `v` requires. The
///           last emitted byte has its high bit clear.
/// FAILURE : Returns `false` on the first allocator failure; bytes
///           emitted before the failure remain in `b`.
///
/// TAGS: Buf, Write, LEB128, Signed
///
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
///
/// SUCCESS : Returns `true`; `b->length` grows by the byte count of `s`
///           up to but not including its NUL, plus one for the NUL
///           terminator itself.
/// FAILURE : Returns `false` on the first allocator failure; bytes
///           emitted before the failure remain in `b`.
///
/// TAGS: Buf, Write, Zstr, String
///
static inline bool BufWriteZstr(Buf *b, Zstr s) {
    while (*s) {
        if (!VecPushBackR(b, (u8)*s)) {
            return false;
        }
        ++s;
    }
    return VecPushBackR(b, (u8)0);
}

// Formatted Buf I/O (BufReadFmt / BufAppendFmt / BufWriteFmt /
// BufPatchFmt) lives in `<Misra/Std/Io.h>` -- those operations
// belong to the I/O layer, not the Buf container itself. Splitting
// them out keeps Buf.h free of Io.h's TypeSpecificIO machinery, which
// transitively pulls File.h and would form an include cycle for any
// header (e.g. File.h's FileRead overload) that names `Buf *`.

#endif // MISRA_STD_CONTAINER_BUF_H
