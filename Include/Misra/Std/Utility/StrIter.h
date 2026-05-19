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
#define ValidateStrIter(si) ValidateIter(si)

///
/// Validate whether a given `StrIters` object is valid.
///
#define ValidateStrIters(siv) ValidateVec(siv);

// ---------------------------------------------------------------------------
// Position
// ---------------------------------------------------------------------------

/// Propagating: succeeds if the new position is in range.
#define StrIterMove(si, n) IterMove((si), (n))
/// Aborting variant of `StrIterMove`.
#define StrIterMustMove(si, n) IterMustMove((si), (n))

/// Propagating: advance one character; false if exhausted.
#define StrIterNext(si) IterNext((si))
/// Aborting variant of `StrIterNext`.
#define StrIterMustNext(si) IterMustNext((si))

/// Propagating: step back one character; false if before start.
#define StrIterPrev(si) IterPrev((si))
/// Aborting variant of `StrIterPrev`.
#define StrIterMustPrev(si) IterMustPrev((si))

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

#define StrIterFromStr(s) IterInitFromVec(s)

#define StrIterFromZstr(s) ((StrIter) {.data = (s), .length = ZstrLen((s)), .pos = 0, .alignment = 1, .dir = 1})

#define StrIterFromCstr(s, n) ((StrIter) {.data = (s), .length = n, .pos = 0, .alignment = 1, .dir = 1})

// ---------------------------------------------------------------------------
// Sizing
// ---------------------------------------------------------------------------

#define StrIterSize(mi)            IterSize(mi)
#define StrIterRemainingSize(mi)   IterRemainingSize(mi)
#define StrIterLength(mi)          IterLength(mi)
#define StrIterRemainingLength(mi) IterRemainingLength(mi)
#define StrIterPos(mi)             IterPos(mi)

// ---------------------------------------------------------------------------
// Read / peek
// ---------------------------------------------------------------------------

/// Propagating: writes `*out`, advances, returns bool.
#define StrIterRead(mi, out) IterRead((mi), (out))
/// Aborting variant of `StrIterRead`.
#define StrIterMustRead(mi, out) IterMustRead((mi), (out))

/// Propagating: writes `*out` with the current char; returns false at EOF.
#define StrIterPeek(mi, out) IterPeekAt((mi), 0, (out))
/// Aborting variant of `StrIterPeek`.
#define StrIterMustPeek(mi, out) IterMustPeekAt((mi), 0, (out))

/// Propagating: writes `*out` with the char at signed offset `n`.
#define StrIterPeekAt(mi, n, out) IterPeekAt((mi), (n), (out))
/// Aborting variant of `StrIterPeekAt`.
#define StrIterMustPeekAt(mi, n, out) IterMustPeekAt((mi), (n), (out))

/// Propagating: peek one ahead in iteration direction.
#define StrIterPeekNext(mi, out) IterPeekAt((mi), 1, (out))
/// Propagating: peek one behind in iteration direction.
#define StrIterPeekPrev(mi, out) IterPeekAt((mi), -1, (out))

#endif // MISRA_STD_UTILITY_STR_ITER_H
