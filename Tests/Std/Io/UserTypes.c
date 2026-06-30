// Demonstrates the IOFMT_USER_CASE_ extension hook: plug an out-of-tree
// type into the formatted-I/O pipeline so `WriteFmt("{}", val)` dispatches
// through user-supplied _write_T / _read_T without any wrapper at the call
// site.

// Only headers that do NOT transitively pull Misra/Std/Io.h are allowed
// above the IOFMT_USER_CASE_ define below -- any earlier Io.h expansion
// would lock in the empty-fallback definition before our override is
// seen. Log.h pulls Io.h, so it lives below the include of Io.h.
#include <Misra/Std/Allocator/Default.h>
#include <Misra/Std/Container/Str.h>
#include <Misra/Std/Zstr.h>
#include <Misra/Types.h>

typedef struct {
    i32 x;
    i32 y;
} Point2D;

typedef struct {
    Point2D min;
    Point2D max;
} Bounds;

typedef struct {
    i32     id;
    Bounds  bbox;
    Point2D centroid;
} Region;

// The extension hook. Must be defined BEFORE Misra/Std/Io.h is processed in
// this TU so that IOFMT(x) picks up the extra _Generic arm. The macro body
// references `_write_T` / `_read_T` symbols which only need to be declared
// at the WriteFmt / StrReadFmt call sites further down -- the definitions
// in this file (below the Io.h include) satisfy that.
//
// All three user types live in the same hook because the writer for the
// outer types (Bounds, Region) recursively expands IOFMT on inner-type
// arguments -- every arm must be visible at every call site.
#define IOFMT_USER_CASE_(x, addr)                                                                                      \
Point2D:                                                                                                               \
    TO_TYPE_SPECIFIC_IO(Point2D, addr), Bounds : TO_TYPE_SPECIFIC_IO(Bounds, addr),                                    \
                                                 Region : TO_TYPE_SPECIFIC_IO(Region, addr),

#include <Misra/Std/Io.h>

#include "../../Util/TestRunner.h"

// Forward declarations so the `_Generic` arms inside IOFMT resolve every
// user `_write_T` / `_read_T` symbol from every call site -- including
// nested expansions from inside an outer writer's own body. Pattern A in
// the extending-io guide.
bool _write_Point2D(Str *o, FmtInfo *info, Point2D *p);
Zstr _read_Point2D(Zstr i, FmtInfo *info, Point2D *p);
bool _write_Bounds(Str *o, FmtInfo *info, Bounds *b);
Zstr _read_Bounds(Zstr i, FmtInfo *info, Bounds *b);
bool _write_Region(Str *o, FmtInfo *info, Region *r);
Zstr _read_Region(Zstr i, FmtInfo *info, Region *r);

bool _write_Point2D(Str *o, FmtInfo *info, Point2D *p) {
    (void)info;
    if (!o || !p) {
        return false;
    }
    return StrAppendFmt(o, "({}, {})", p->x, p->y);
}

Zstr _read_Point2D(Zstr i, FmtInfo *info, Point2D *p) {
    (void)info;
    if (!i || !p) {
        return i;
    }
    StrReadFmt(i, "({}, {})", p->x, p->y);
    return i;
}

// Bounds delegates to Point2D for its two corners. Verifies the
// `_Generic` arm for `Point2D` resolves correctly from inside another
// user-type writer.
bool _write_Bounds(Str *o, FmtInfo *info, Bounds *b) {
    (void)info;
    if (!o || !b) {
        return false;
    }
    return StrAppendFmt(o, "[{}..{}]", b->min, b->max);
}

Zstr _read_Bounds(Zstr i, FmtInfo *info, Bounds *b) {
    (void)info;
    if (!i || !b) {
        return i;
    }
    StrReadFmt(i, "[{}..{}]", b->min, b->max);
    return i;
}

// Region is 3-deep: it embeds Bounds (which itself embeds Point2D) plus
// a standalone Point2D plus an in-tree i32. Exercises a writer that mixes
// nested user types, repeated user types, and built-ins in one call.
bool _write_Region(Str *o, FmtInfo *info, Region *r) {
    (void)info;
    if (!o || !r) {
        return false;
    }
    return StrAppendFmt(o, "{}:{}@{}", r->id, r->bbox, r->centroid);
}

Zstr _read_Region(Zstr i, FmtInfo *info, Region *r) {
    (void)info;
    if (!i || !r) {
        return i;
    }
    StrReadFmt(i, "{}:{}@{}", r->id, r->bbox, r->centroid);
    return i;
}

bool test_user_type_write_basic(void);
bool test_user_type_write_mixed_args(void);
bool test_user_type_read_basic(void);
bool test_user_type_round_trip(void);
bool test_nested_user_type_write(void);
bool test_nested_user_type_round_trip(void);
bool test_deep_nested_user_type_write(void);
bool test_deep_nested_user_type_round_trip(void);
bool test_nested_user_type_mixed_with_builtins(void);

bool test_user_type_write_basic(void) {
    WriteFmt("Testing user-type write through IOFMT_USER_CASE_\n");

    DefaultAllocator alloc = DefaultAllocatorInit();
    Str              out   = StrInit(&alloc);

    Point2D p  = {.x = 3, .y = 4};
    bool    ok = StrAppendFmt(&out, "{}", p);
    ok         = ok && (ZstrCompare(StrBegin(&out), "(3, 4)") == 0);

    StrDeinit(&out);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

bool test_user_type_write_mixed_args(void) {
    WriteFmt("Testing user-type mixed with in-tree types\n");

    DefaultAllocator alloc = DefaultAllocatorInit();
    Str              out   = StrInit(&alloc);

    Point2D p     = {.x = -1, .y = 2};
    i32     count = 7;
    bool    ok    = StrAppendFmt(&out, "got {} hits at {}", count, p);
    ok            = ok && (ZstrCompare(StrBegin(&out), "got 7 hits at (-1, 2)") == 0);

    StrDeinit(&out);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

bool test_user_type_read_basic(void) {
    WriteFmt("Testing user-type read through IOFMT_USER_CASE_\n");

    Zstr    in = "(42, -9)";
    Point2D p  = {0};
    StrReadFmt(in, "{}", p);
    bool ok = (p.x == 42) && (p.y == -9);

    return ok;
}

bool test_user_type_round_trip(void) {
    WriteFmt("Testing user-type write/read round-trip\n");

    DefaultAllocator alloc = DefaultAllocatorInit();
    Str              out   = StrInit(&alloc);

    Point2D src = {.x = 100, .y = -200};
    Point2D dst = {0};

    bool ok = StrAppendFmt(&out, "{}", src);
    Zstr in = StrBegin(&out);
    StrReadFmt(in, "{}", dst);
    ok = ok && (dst.x == src.x) && (dst.y == src.y);

    StrDeinit(&out);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

bool test_nested_user_type_write(void) {
    WriteFmt("Testing user-type-in-user-type writer (Bounds embeds Point2D)\n");

    DefaultAllocator alloc = DefaultAllocatorInit();
    Str              out   = StrInit(&alloc);

    Bounds b = {
        .min = { .x = 0,  .y = 0},
        .max = {.x = 10, .y = 20}
    };
    bool ok = StrAppendFmt(&out, "{}", b);
    ok      = ok && (ZstrCompare(StrBegin(&out), "[(0, 0)..(10, 20)]") == 0);

    StrDeinit(&out);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

bool test_nested_user_type_round_trip(void) {
    WriteFmt("Testing nested user-type round-trip (Bounds)\n");

    DefaultAllocator alloc = DefaultAllocatorInit();
    Str              out   = StrInit(&alloc);

    Bounds src = {
        .min = {.x = -5, .y = -7},
        .max = { .x = 3, .y = 11}
    };
    Bounds dst = {0};

    bool ok = StrAppendFmt(&out, "{}", src);
    Zstr in = StrBegin(&out);
    StrReadFmt(in, "{}", dst);

    ok = ok && (dst.min.x == src.min.x) && (dst.min.y == src.min.y);
    ok = ok && (dst.max.x == src.max.x) && (dst.max.y == src.max.y);

    StrDeinit(&out);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

bool test_deep_nested_user_type_write(void) {
    WriteFmt("Testing 3-level nested user-type writer (Region -> Bounds -> Point2D)\n");

    DefaultAllocator alloc = DefaultAllocatorInit();
    Str              out   = StrInit(&alloc);

    Region r = {
        .id       = 42,
        .bbox     = {.min = {.x = 0, .y = 0}, .max = {.x = 100, .y = 50}},
        .centroid = {                .x = 50,                    .y = 25},
    };
    bool ok = StrAppendFmt(&out, "{}", r);
    ok      = ok && (ZstrCompare(StrBegin(&out), "42:[(0, 0)..(100, 50)]@(50, 25)") == 0);

    StrDeinit(&out);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

bool test_deep_nested_user_type_round_trip(void) {
    WriteFmt("Testing 3-level nested user-type round-trip (Region)\n");

    DefaultAllocator alloc = DefaultAllocatorInit();
    Str              out   = StrInit(&alloc);

    Region src = {
        .id       = -1234,
        .bbox     = {.min = {.x = -10, .y = -20}, .max = {.x = 30, .y = 40}},
        .centroid = {                    .x = 10,                   .y = 10},
    };
    Region dst = {0};

    bool ok = StrAppendFmt(&out, "{}", src);
    Zstr in = StrBegin(&out);
    StrReadFmt(in, "{}", dst);

    ok = ok && (dst.id == src.id);
    ok = ok && (dst.bbox.min.x == src.bbox.min.x) && (dst.bbox.min.y == src.bbox.min.y);
    ok = ok && (dst.bbox.max.x == src.bbox.max.x) && (dst.bbox.max.y == src.bbox.max.y);
    ok = ok && (dst.centroid.x == src.centroid.x) && (dst.centroid.y == src.centroid.y);

    StrDeinit(&out);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

bool test_nested_user_type_mixed_with_builtins(void) {
    WriteFmt("Testing nested user types mixed with built-ins in one call\n");

    DefaultAllocator alloc = DefaultAllocatorInit();
    Str              out   = StrInit(&alloc);

    Region region = {
        .id       = 7,
        .bbox     = {.min = {.x = 1, .y = 2}, .max = {.x = 3, .y = 4}},
        .centroid = {                 .x = 2,                  .y = 3},
    };
    Point2D origin = {.x = 0, .y = 0};
    f64     score  = 1.5;

    // Single format call mixing: built-in f64 + user Region (which itself
    // expands to an i32 plus a nested Bounds plus a Point2D) + a separate
    // Point2D arg. Forces the per-arg _Generic to dispatch four distinct
    // arms (i32 implicit, Region, Point2D, f64) and proves nested user
    // types and built-ins coexist in one IOFMT expansion.
    bool ok = StrAppendFmt(&out, "score={.1} region={} origin={}", score, region, origin);
    ok      = ok && (ZstrCompare(StrBegin(&out), "score=1.5 region=7:[(1, 2)..(3, 4)]@(2, 3) origin=(0, 0)") == 0);

    StrDeinit(&out);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

int main(void) {
    WriteFmt("[INFO] Starting Io.UserTypes tests\n\n");

    TestFunction tests[] = {
        test_user_type_write_basic,
        test_user_type_write_mixed_args,
        test_user_type_read_basic,
        test_user_type_round_trip,
        test_nested_user_type_write,
        test_nested_user_type_round_trip,
        test_deep_nested_user_type_write,
        test_deep_nested_user_type_round_trip,
        test_nested_user_type_mixed_with_builtins,
    };

    int total_tests = sizeof(tests) / sizeof(tests[0]);
    return run_test_suite(tests, total_tests, NULL, 0, "Io.UserTypes");
}
