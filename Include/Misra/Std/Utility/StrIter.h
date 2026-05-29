/// file      : std/utility/striter.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Character cursor (`StrIter` = `Iter(char)`) plus a `Vec(StrIter)`
/// alias. Most operations are namespace-inheritance aliases that
/// re-frame `Iter(char)` ops in string vocabulary; the `StrIterFromZstr`
/// / `StrIterFromCstr` constructors are the only string-specific
/// additions over the generic `Iter` surface.

#ifndef MISRA_STD_UTILITY_STR_ITER_H
#define MISRA_STD_UTILITY_STR_ITER_H

#include <Misra/Std/Container/Vec/Type.h>
#include <Misra/Std/Zstr.h>
#include <Misra/Std/Utility/Iter.h>
#include <Misra/Types.h>

typedef Iter(char) StrIter;
typedef Vec(StrIter) StrIters;

///
/// Validate whether a given `StrIter` object is valid.
/// Not foolproof but will work most of the time.
/// Aborts if provided `StrIter` is not valid.
///
/// i[in] : Pointer to `StrIter` object to validate.
///
/// SUCCESS : Continue execution.
/// FAILURE : `abort`
///
/// TAGS: StrIter, Validate, API
///
#define ValidateStrIter(si) ValidateIter(si)

///
/// Validate whether a given `StrIters` object is valid.
///
/// TAGS: StrIter, Validate, API
///
#define ValidateStrIters(siv) ValidateVec(siv)

// ---------------------------------------------------------------------------
// Position
// ---------------------------------------------------------------------------

///
/// Move the `StrIter` cursor by signed offset `n`. Propagating alias
/// for `IterMove`; see `IterMove` for the full contract.
///
/// TAGS: StrIter, Move, Position, Alias
///
#define StrIterMove(si, n) IterMove((si), (n))

///
/// Aborting variant of `StrIterMove`. Alias for `IterMustMove`; see
/// `IterMustMove` for the full contract.
///
/// TAGS: StrIter, Move, Must, Alias
///
#define StrIterMustMove(si, n) IterMustMove((si), (n))

///
/// Advance one character. Propagating alias for `IterNext`; see
/// `IterNext` for the full contract.
///
/// TAGS: StrIter, Next, Advance, Alias
///
#define StrIterNext(si) IterNext((si))

///
/// Aborting variant of `StrIterNext`. Alias for `IterMustNext`; see
/// `IterMustNext` for the full contract.
///
/// TAGS: StrIter, Next, Must, Alias
///
#define StrIterMustNext(si) IterMustNext((si))

///
/// Step back one character. Propagating alias for `IterPrev`; see
/// `IterPrev` for the full contract.
///
/// TAGS: StrIter, Prev, Reverse, Alias
///
#define StrIterPrev(si) IterPrev((si))

///
/// Aborting variant of `StrIterPrev`. Alias for `IterMustPrev`; see
/// `IterMustPrev` for the full contract.
///
/// TAGS: StrIter, Prev, Must, Alias
///
#define StrIterMustPrev(si) IterMustPrev((si))

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

///
/// Construct a forward-walking `StrIter` from a `Str` value. Alias-
/// reframe of `IterInitFromVec` in string vocabulary: `Str` is a
/// `Vec(char)`, so the borrowed-data semantics and NULL-allocator
/// alignment fallback documented on `IterInitFromVec` apply unchanged.
///
/// s[in] : Source string (by value -- the resulting iter holds a
///         non-owning pointer into `s`'s buffer).
///
/// SUCCESS : Returns a struct-literal `StrIter` covering
///           `[StrBegin(&s), StrBegin(&s) + StrLen(&s))` with `pos = 0`,
///           `dir = 1`.
/// FAILURE : Macro cannot fail; an empty `Str` yields a remaining-
///           length-0 iterator.
///
/// TAGS: StrIter, Construct, Str, Alias
///
#define StrIterFromStr(s) IterInitFromVec(s)

///
/// Construct a forward-walking `StrIter` over a NUL-terminated `Zstr`.
/// Alias-reframe of the `IterInitFromVec` shape: length is taken from
/// `ZstrLen(s)`, alignment is fixed at `1`. See `IterInitFromVec` for
/// the overall borrowed-data semantics.
///
/// s[in] : NUL-terminated string. Must outlive the iterator.
///
/// SUCCESS : Returns a compound-literal `StrIter` covering
///           `[s, s + ZstrLen(s))` with `pos = 0`, `dir = 1`. The
///           NUL terminator is *not* part of the iterated range.
/// FAILURE : Macro cannot fail. Passing a non-NUL-terminated buffer is
///           a usage error -- `ZstrLen` would run past the end.
///
/// TAGS: StrIter, Construct, Zstr
///
#define StrIterFromZstr(s) ((StrIter) {.data = (s), .length = ZstrLen((s)), .pos = 0, .alignment = 1, .dir = 1})

///
/// Construct a forward-walking `StrIter` over an explicit
/// `(char *, n)` byte range. Alias-reframe of the `IterInitFromVec`
/// shape with caller-supplied length; alignment is fixed at `1`.
/// See `IterInitFromVec` for the overall borrowed-data semantics.
///
/// s[in] : Pointer to the first character. Must outlive the iterator.
/// n[in] : Number of characters reachable through the iterator.
///
/// SUCCESS : Returns a compound-literal `StrIter` covering `[s, s + n)`
///           with `pos = 0`, `dir = 1`.
/// FAILURE : Macro cannot fail. Passing `n` larger than the actual
///           allocation is a usage error -- subsequent reads will
///           run past the end.
///
/// TAGS: StrIter, Construct, Cstr
///
#define StrIterFromCstr(s, n) ((StrIter) {.data = (s), .length = (n), .pos = 0, .alignment = 1, .dir = 1})

// ---------------------------------------------------------------------------
// Sizing
// ---------------------------------------------------------------------------

///
/// Total region size in bytes covered by the `StrIter`. Alias-reframe
/// of `IterSize` in string vocabulary; since `StrIter` walks `char`
/// with alignment 1, this matches the iterator's character length.
/// See `IterSize` for the full contract.
///
/// TAGS: StrIter, Size, Alias
///
#define StrIterSize(mi) IterSize(mi)

///
/// Remaining region size in bytes from the current position to the
/// end of the iteration direction. Alias-reframe of `IterRemainingSize`
/// in string vocabulary. See `IterRemainingSize` for the full contract.
///
/// TAGS: StrIter, Size, Remaining, Alias
///
#define StrIterRemainingSize(mi) IterRemainingSize(mi)

///
/// Total character length of the region the `StrIter` covers. Alias-
/// reframe of `IterLength` in string vocabulary. See `IterLength` for
/// the full contract.
///
/// TAGS: StrIter, Length, Alias
///
#define StrIterLength(mi) IterLength(mi)

///
/// Characters remaining to read in the iteration direction. Alias-
/// reframe of `IterRemainingLength` in string vocabulary. See
/// `IterRemainingLength` for the full contract.
///
/// TAGS: StrIter, Length, Remaining, Alias
///
#define StrIterRemainingLength(mi) IterRemainingLength(mi)

///
/// Pointer to the character at the current cursor position, or the
/// `NULL_ITER_DATA(mi)` sentinel when the iterator is exhausted.
/// Alias-reframe of `IterPos` in string vocabulary. See `IterPos` for
/// the full contract.
///
/// TAGS: StrIter, Position, Alias
///
#define StrIterPos(mi) IterPos(mi)

///
/// Absolute cursor index within the `StrIter`'s backing region. Alias-
/// reframe of `IterIndex` in string vocabulary. See `IterIndex` for
/// the full contract.
///
/// TAGS: StrIter, Position, Alias
///
#define StrIterIndex(mi) IterIndex(mi)

///
/// Pointer to the character at absolute index `idx` in the `StrIter`'s
/// backing region. Alias-reframe of `IterDataAt` in string vocabulary;
/// the one-past-end pointer at `idx == length` is the standard
/// half-open-range upper bound and is well-defined. See `IterDataAt`
/// for the full contract.
///
/// TAGS: StrIter, Position, Alias
///
#define StrIterDataAt(mi, idx) IterDataAt((mi), (idx))

// ---------------------------------------------------------------------------
// Read / peek
// ---------------------------------------------------------------------------

///
/// Read the current character into `*out` and advance the cursor.
/// Propagating alias for `IterRead`; see `IterRead` for the full
/// contract.
///
/// TAGS: StrIter, Read, Advance, Alias
///
#define StrIterRead(mi, out) IterRead((mi), (out))

///
/// Aborting variant of `StrIterRead`. Alias for `IterMustRead`; see
/// `IterMustRead` for the full contract.
///
/// TAGS: StrIter, Read, Must, Alias
///
#define StrIterMustRead(mi, out) IterMustRead((mi), (out))

///
/// Read the current character into `*out` without advancing.
/// Propagating alias for `IterPeekAt(mi, 0, out)`; see `IterPeekAt`
/// for the full contract.
///
/// TAGS: StrIter, Peek, Alias
///
#define StrIterPeek(mi, out) IterPeekAt((mi), 0, (out))

///
/// Aborting variant of `StrIterPeek`. Alias for `IterMustPeekAt(mi,
/// 0, out)`; see `IterMustPeekAt` for the full contract.
///
/// TAGS: StrIter, Peek, Must, Alias
///
#define StrIterMustPeek(mi, out) IterMustPeekAt((mi), 0, (out))

///
/// Read the character at signed offset `n` into `*out` without
/// advancing. Propagating alias for `IterPeekAt`; see `IterPeekAt`
/// for the full contract.
///
/// TAGS: StrIter, Peek, Offset, Alias
///
#define StrIterPeekAt(mi, n, out) IterPeekAt((mi), (n), (out))

///
/// Aborting variant of `StrIterPeekAt`. Alias for `IterMustPeekAt`;
/// see `IterMustPeekAt` for the full contract.
///
/// TAGS: StrIter, Peek, Offset, Must, Alias
///
#define StrIterMustPeekAt(mi, n, out) IterMustPeekAt((mi), (n), (out))

///
/// Peek one position ahead in the iteration direction. Honours the
/// iter's `dir` so that on a reverse iter "next" means the byte the
/// cursor would land on after one `IterNext` step (i.e. behind the
/// current position in memory), matching the forward-iter intuition.
///
/// TAGS: StrIter, Peek, Next
///
#define StrIterPeekNext(mi, out) IterPeekAt((mi), (mi)->dir, (out))

///
/// Peek one position behind in the iteration direction. Honours the
/// iter's `dir` so "prev" stays opposite of `StrIterPeekNext` for both
/// forward and reverse iters.
///
/// TAGS: StrIter, Peek, Prev
///
#define StrIterPeekPrev(mi, out) IterPeekAt((mi), -((mi)->dir), (out))

#endif // MISRA_STD_UTILITY_STR_ITER_H
