/// file      : Tests/Std/Allocator.Arena.c
/// Smoke tests for the bump (arena) allocator.

#include <stdint.h>

#include <Misra/Std/Allocator.h>
#include <Misra/Std/Allocator/Arena.h>
#include <Misra/Std/Container/Vec.h>
#include <Misra/Std/Io.h>
#include <Misra/Std/Log.h>
#include <Misra/Std/Memory.h>

#include "../Util/TestRunner.h"

static bool test_basic_bump(void) {
    ArenaAllocator arena      = ArenaAllocatorInit();
    Allocator     *alloc_base = ALLOCATOR_OF(&arena);
    char          *a          = (char *)AllocatorAlloc(alloc_base, 16, true);
    char          *b          = (char *)AllocatorAlloc(alloc_base, 32, true);
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
    char          *p          = (char *)AllocatorAlloc(alloc_base, 16, true);
    bool           ok         = (p != NULL);

    if (ok) {
        p[0]        = 'h';
        p[15]       = 'i';
        char *grown = (char *)AllocatorRealloc(alloc_base, p, 32);
        // Grew in place at the same address, with content preserved.
        ok = (grown == p) && (grown[0] == 'h') && (grown[15] == 'i');
    }

    ArenaAllocatorDeinit(&arena);
    return ok;
}

static bool test_remap_non_last_refused(void) {
    // Arena's remap can only honor a non-last allocation if it knows
    // the old size -- otherwise the alloc-copy-free fallback would
    // have to over-read into adjacent allocations to fill the new
    // buffer. The bump policy doesn't track per-allocation sizes
    // (that's the whole point of being a bump allocator), so remap
    // of a non-last pointer refuses with NULL. Callers that need
    // resize-of-anything semantics should use a HeapAllocator.
    ArenaAllocator arena      = ArenaAllocatorInit();
    Allocator     *alloc_base = ALLOCATOR_OF(&arena);
    char          *a          = (char *)AllocatorAlloc(alloc_base, 16, true);
    char          *b          = (char *)AllocatorAlloc(alloc_base, 16, true);
    bool           ok         = (a != NULL) && (b != NULL);

    if (ok) {
        // `a` is no longer the tail; remap must refuse.
        char *grown = (char *)AllocatorRealloc(alloc_base, a, 64);
        ok          = (grown == NULL);
    }

    (void)b;
    ArenaAllocatorDeinit(&arena);
    return ok;
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
    char          *a          = (char *)AllocatorAlloc(alloc_base, 4096, true);
    char          *b          = (char *)AllocatorAlloc(alloc_base, 4096, true);
    bool           ok         = (a != NULL) && (b != NULL);

    ArenaAllocatorReset(&arena);
    char *c = (char *)AllocatorAlloc(alloc_base, 4096, true);
    ok      = ok && (c != NULL) && (c == a); // Reset reuses the first chunk.

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
        ok = (((uintptr_t)a & 63u) == 0) && (((uintptr_t)b & 63u) == 0);
    }

    ArenaAllocatorDeinit(&arena);
    return ok;
}

int main(void) {
    TestFunction tests[] = {
        test_basic_bump,
        test_grow_last_in_place,
        test_remap_non_last_refused,
        test_vec_on_arena,
        test_reset,
        test_alignment,
    };
    return run_test_suite(tests, (int)(sizeof(tests) / sizeof(tests[0])), NULL, 0, "Allocator.Arena");
}
