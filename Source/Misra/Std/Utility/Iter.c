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
    if ((i->dir != -1 && i->dir != 1) || !i->alignment || !i->length || i->pos >= i->length) {
        LOG_FATAL("Invalid iter object.");
    }
    (void)(*(char *)(void *)(i->data));
}
