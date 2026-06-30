/// file      : tests/std/allocator.arena.c
/// Smoke tests for the bump (arena) allocator.

#include <Misra/Std/Allocator.h>
#include <Misra/Std/Allocator/Arena.h>
#include <Misra/Std/Container/Vec.h>
#include <Misra/Std/Io.h>
#include <Misra/Std/Log.h>
#include <Misra/Std/Memory.h>

#include "../../Util/TestRunner.h"

static bool test_basic_bump(void) {
    ArenaAllocator arena      = ArenaAllocatorInit();
    Allocator     *alloc_base = ALLOCATOR_OF(&arena);
    u8            *a          = (u8 *)AllocatorAlloc(alloc_base, 16, true);
    u8            *b          = (u8 *)AllocatorAlloc(alloc_base, 32, true);
    bool           ok         = (a != NULL) && (b != NULL) && (b > a);

    if (ok) {
        a[0]  = 'a';
        b[31] = 'z';
        ok    = (a[0] == 'a') && (b[31] == 'z');
    }

    ArenaAllocatorDeinit(&arena);
    return ok;
}

static bool test_grow_last_in_place(void) {
    ArenaAllocator arena      = ArenaAllocatorInit();
    Allocator     *alloc_base = ALLOCATOR_OF(&arena);
    u8            *p          = (u8 *)AllocatorAlloc(alloc_base, 16, true);
    bool           ok         = (p != NULL);

    if (ok) {
        p[0]      = 'h';
        p[15]     = 'i';
        u8 *grown = (u8 *)AllocatorRealloc(alloc_base, p, 32);
        // Grew in place at the same address, with content preserved.
        ok = (grown == p) && (grown[0] == 'h') && (grown[15] == 'i');
    }

    ArenaAllocatorDeinit(&arena);
    return ok;
}

static bool test_reject_remap_non_last(void) {
    // Arena's remap can only honor a non-last allocation if it knows
    // the old size. The bump policy doesn't track per-allocation
    // sizes (that's the whole point), so remap of a non-last pointer
    // is a caller bug and aborts via LOG_FATAL.
    ArenaAllocator arena      = ArenaAllocatorInit();
    Allocator     *alloc_base = ALLOCATOR_OF(&arena);
    u8            *a          = (u8 *)AllocatorAlloc(alloc_base, 16, true);
    u8            *b          = (u8 *)AllocatorAlloc(alloc_base, 16, true);
    (void)b;
    (void)AllocatorRealloc(alloc_base, a, 64); // -> LOG_FATAL
    return false;                              // unreachable
}

static bool test_reject_foreign_free(void) {
    // Free of a pointer not in any arena chunk is a caller bug.
    ArenaAllocator arena      = ArenaAllocatorInit();
    Allocator     *alloc_base = ALLOCATOR_OF(&arena);
    char           stack_byte = 0;
    AllocatorFree(alloc_base, &stack_byte); // -> LOG_FATAL
    return false;                           // unreachable
}

static bool test_vec_on_arena(void) {
    ArenaAllocator arena = ArenaAllocatorInit();
    typedef Vec(int) IntVec;
    IntVec v  = VecInit(&arena);
    bool   ok = true;

    for (int i = 0; i < 4096; i++) {
        if (!VecPushBackR(&v, i)) {
            ok = false;
            break;
        }
    }

    ok = ok && VecLen(&v) == 4096 && VecAt(&v, 0) == 0 && VecAt(&v, 4095) == 4095;
    VecDeinit(&v);
    ArenaAllocatorDeinit(&arena);
    return ok;
}

static bool test_reset(void) {
    ArenaAllocator arena      = ArenaAllocatorInit();
    Allocator     *alloc_base = ALLOCATOR_OF(&arena);
    u8            *a          = (u8 *)AllocatorAlloc(alloc_base, 4096, true);
    u8            *b          = (u8 *)AllocatorAlloc(alloc_base, 4096, true);
    bool           ok         = (a != NULL) && (b != NULL);

    ArenaAllocatorReset(&arena);
    u8 *c = (u8 *)AllocatorAlloc(alloc_base, 4096, true);
    ok    = ok && (c != NULL) && (c == a); // Reset reuses the first chunk.

    ArenaAllocatorDeinit(&arena);
    return ok;
}

static bool test_alignment(void) {
    ArenaAllocator arena      = ArenaAllocatorInitAligned(64);
    Allocator     *alloc_base = ALLOCATOR_OF(&arena);
    void          *a          = AllocatorAlloc(alloc_base, 1, true);
    void          *b          = AllocatorAlloc(alloc_base, 1, true);
    bool           ok         = (a != NULL) && (b != NULL);

    if (ok) {
        ok = (((u64)a & 63u) == 0) && (((u64)b & 63u) == 0);
    }

    ArenaAllocatorDeinit(&arena);
    return ok;
}

int main(void) {
    TestFunction normal[] = {
        test_basic_bump,
        test_grow_last_in_place,
        test_vec_on_arena,
        test_reset,
        test_alignment,
    };
    TestFunction deadend[] = {
        test_reject_remap_non_last,
        test_reject_foreign_free,
    };
    return run_test_suite(
        normal,
        (int)(sizeof(normal) / sizeof(normal[0])),
        deadend,
        (int)(sizeof(deadend) / sizeof(deadend[0])),
        "Allocator.Arena"
    );
}
