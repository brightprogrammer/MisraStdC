#include <Misra/Std/Log.h>
#include <Misra/Std/Utility/Iter.h>

///
/// Move current reading position of Iterator.
///
/// SUCCESS : Data is copied from current read position to provided `dst`, and `mi` is returned
/// FAILURE : NULL_ITER(mi) returned
///
bool move_iter(GenericIter *mi, i64 n) {
    i64 remaining = IterRemainingLength(mi);
    i64 length    = IterLength(mi);

    if ((remaining - n <= length) && (remaining - n >= 0)) {
        mi->pos += n;
        return true;
    }

    return false;
}
