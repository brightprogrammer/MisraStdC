#include <Misra/Std/Utility/Iter.h>
#include <Misra/Std/Log.h>

// libc
#include <stdlib.h>

size remaining_length_iter(GenericIter* mi) {
    return ((mi)->dir == 1)  ? (mi)->pos < IterLength(mi) ? (IterLength(mi) - (mi)->pos) : 0 :
           ((mi)->dir == -1) ? (mi)->pos < IterLength(mi) ? ((mi)->pos + 1) : 0 :
                               (LOG_FATAL("Iter: Invalid direction"), 0);
}

