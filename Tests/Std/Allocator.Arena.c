/// file      : Tests/Std/Allocator.Arena.c
/// Smoke tests for the bump (arena) allocator.

#include <stdint.h>

#include <Misra/Std/Allocator.h>
#include <Misra/Std/Container/Vec.h>
#include <Misra/Std/Io.h>
#include <Misra/Std/Log.h>
#include <Misra/Std/Memory.h>

#include "../Util/TestRunner.h"

static bool test_basic_bump(void) {
    Allocator arena = ArenaAllocator();
    char     *a     = (char *)AllocatorAlloc(&arena, 16, true);
    char     *b     = (char *)AllocatorAlloc(&arena, 32, true);
    bool      ok    = (a != NULL) && (b != NULL) && (b > a);

    if (ok) {
        a[0]  = 'a';
        b[31] = 'z';
        ok    = (a[0] == 'a') && (b[31] == 'z');
    }

    AllocatorUnbind(&arena);
    return ok;
}

static bool test_grow_last_in_place(void) {
    Allocator arena = ArenaAllocator();
    char     *p     = (char *)AllocatorAlloc(&arena, 16, true);
    bool      ok    = (p != NULL);

    if (ok) {
        p[0]        = 'h';
        p[15]       = 'i';
        char *grown = (char *)AllocatorRealloc(&arena, p, 16, 32);
        // Grew in place at the same address, with content preserved.
        ok = (grown == p) && (grown[0] == 'h') && (grown[15] == 'i');
    }

    AllocatorUnbind(&arena);
    return ok;
}

static bool test_grow_non_last_relocates(void) {
    Allocator arena = ArenaAllocator();
    char     *a     = (char *)AllocatorAlloc(&arena, 16, true);
    char     *b     = (char *)AllocatorAlloc(&arena, 16, true);
    bool      ok    = (a != NULL) && (b != NULL);

    if (ok) {
        a[0]        = 'a';
        a[15]       = '!';
        char *grown = (char *)AllocatorRealloc(&arena, a, 16, 64);
        // `a` is no longer the tail, so realloc must move it.
        ok = (grown != NULL) && (grown != a) && (grown[0] == 'a') && (grown[15] == '!');
    }

    AllocatorUnbind(&arena);
    return ok;
}

static bool test_vec_on_arena(void) {
    Allocator arena = ArenaAllocator();
    typedef Vec(int) IntVec;
    IntVec v  = VecInit(arena);
    bool   ok = true;

    for (int i = 0; i < 4096; i++) {
        if (!VecPushBackR(&v, i)) {
            ok = false;
            break;
        }
    }

    ok = ok && VecLen(&v) == 4096 && VecAt(&v, 0) == 0 && VecAt(&v, 4095) == 4095;
    VecDeinit(&v);
    AllocatorUnbind(&arena);
    return ok;
}

static bool test_reset(void) {
    Allocator arena = ArenaAllocator();
    char     *a     = (char *)AllocatorAlloc(&arena, 4096, true);
    char     *b     = (char *)AllocatorAlloc(&arena, 4096, true);
    bool      ok    = (a != NULL) && (b != NULL);

    ArenaAllocatorReset(&arena);
    char *c = (char *)AllocatorAlloc(&arena, 4096, true);
    ok      = ok && (c != NULL) && (c == a); // Reset reuses the first chunk.

    AllocatorUnbind(&arena);
    return ok;
}

static bool test_alignment(void) {
    Allocator arena = ArenaAllocatorAligned(64);
    void     *a     = AllocatorAlloc(&arena, 1, true);
    void     *b     = AllocatorAlloc(&arena, 1, true);
    bool      ok    = (a != NULL) && (b != NULL);

    if (ok) {
        ok = (((uintptr_t)a & 63u) == 0) && (((uintptr_t)b & 63u) == 0);
    }

    AllocatorUnbind(&arena);
    return ok;
}

int main(void) {
    TestFunction tests[] = {
        test_basic_bump,
        test_grow_last_in_place,
        test_grow_non_last_relocates,
        test_vec_on_arena,
        test_reset,
        test_alignment,
    };
    return run_test_suite(tests, (int)(sizeof(tests) / sizeof(tests[0])), NULL, 0, "Allocator.Arena");
}
