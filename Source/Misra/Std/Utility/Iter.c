/// file      : std/utility/iter.c
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Runtime backends for the generic `Iter` cursor (bounds checks, move,
/// peek-index, validate). The macros in `Std/Utility/Iter/*.h` wrap
/// these via `GENERIC_ITER` so element-typed iters share one body.

#include <Misra/Std/Log.h>
#include <Misra/Std/Utility/Iter.h>

size remaining_length_iter(GenericIter *mi) {
    if (mi->dir == 1) {
        if (mi->pos < IterLength(mi)) {
            return IterLength(mi) - mi->pos;
        }
        return 0;
    }
    if (mi->dir == -1) {
        if (mi->pos < IterLength(mi)) {
            return mi->pos + 1;
        }
        return 0;
    }
    LOG_FATAL("Invalid direction");
    return 0;
}

size iter_peek_index(GenericIter *it, i64 n) {
    // Compute pos + n in signed space, then bounds-check. The past-end
    // sentinel for reverse iteration is the unsigned wrap of -1, so we
    // need to read pos as signed when reasoning about offsets.
    i64 cur;
    if (it->dir == -1 && it->pos == (size)-1) {
        cur = -1;
    } else {
        cur = (i64)it->pos;
    }
    i64 target = cur + n;
    if (target < 0 || target >= (i64)it->length) {
        return (size)-1;
    }
    return (size)target;
}

bool iter_try_move(GenericIter *it, i64 n) {
    i64 cur;
    if (it->dir == -1 && it->pos == (size)-1) {
        cur = -1;
    } else {
        cur = (i64)it->pos;
    }
    i64 delta   = (i64)it->dir * n;
    i64 new_pos = cur + delta;
    if (it->dir == 1) {
        if (new_pos < 0 || new_pos > (i64)it->length) {
            return false;
        }
        it->pos = (size)new_pos;
        return true;
    }
    if (it->dir == -1) {
        // Valid range: [-1, length-1]. -1 is the past-start sentinel.
        if (new_pos < -1 || new_pos > (i64)it->length - 1) {
            return false;
        }
        it->pos = (new_pos < 0) ? (size)-1 : (size)new_pos;
        return true;
    }
    LOG_FATAL("Invalid direction");
    return false;
}

void validate_iter(GenericIter *i) {
    // Structural-validity check only. Length-0 is a legal initial state
    // (per `IterInit()`). Forward iters consider `pos == length` exhausted;
    // reverse iters use `pos == (size)-1` (== UINT64_MAX) as the past-start
    // sentinel. Both are valid here -- the position checks belong in the
    // iter-move helpers, not in the structural validator.
    if ((i->dir != -1 && i->dir != 1) || !i->alignment) {
        LOG_FATAL("Invalid iter object");
    }
}
