#include <Misra/Std/Utility/Iter.h>
#include <Misra/Std/Log.h>

// libc
#include <stdlib.h>

size remaining_length_iter(GenericIter *mi) {
    if ((mi)->dir == 1) {
        if ((mi)->pos < IterLength(mi)) {
            return IterLength(mi) - (mi)->pos;
        } else {
            return 0;
        }
    } else if ((mi)->dir == -1) {
        if ((mi)->pos < IterLength(mi)) {
            return (mi)->pos + 1;
        } else {
            return 0;
        }
    } else {
        LOG_FATAL("Invalid direction");
        return 0;
    }
}

void validate_iter(GenericIter *i) {
    if (((i)->dir != -1 && (i)->dir != 1) || !(i)->alignment || !(i)->length || (i)->pos >= (i)->length) {
        LOG_FATAL("Invalid iter object.");
    }
    (void)(*(char *)(void *)((i)->data));
}
