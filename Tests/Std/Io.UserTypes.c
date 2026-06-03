// Demonstrates the IOFMT_USER_CASES_ extension hook: plug an out-of-tree
// type into the formatted-I/O pipeline so `WriteFmt("{}", val)` dispatches
// through user-supplied _write_T / _read_T without any wrapper at the call
// site.

#include <Misra/Std/Allocator/Default.h>
#include <Misra/Std/Container/Str.h>
#include <Misra/Std/Zstr.h>
#include <Misra/Types.h>

typedef struct {
    i32 x;
    i32 y;
} Point2D;

// The extension hook. Must be defined BEFORE Misra/Std/Io.h is processed in
// this TU so that IOFMT(x) picks up the extra _Generic arm. The macro body
// references symbols `_write_Point2D` / `_read_Point2D` which only need to
// be declared at the WriteFmt / StrReadFmt call sites further down -- the
// definitions in this file (below the Io.h include) satisfy that.
#define IOFMT_USER_CASES_(x, addr)                                                                                     \
Point2D:                                                                                                               \
    TO_TYPE_SPECIFIC_IO(Point2D, addr),

#include <Misra/Std/Io.h>

#include "../Util/TestRunner.h"

// Forward declarations so the `_Generic` arms inside IOFMT can resolve
// both `_write_Point2D` and `_read_Point2D` from any call site below --
// including from inside `_write_Point2D`'s own body, which calls
// StrAppendFmt and thus expands IOFMT.
bool _write_Point2D(Str *o, FmtInfo *info, Point2D *p);
Zstr _read_Point2D(Zstr i, FmtInfo *info, Point2D *p);

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

    while (*i == ' ' || *i == '\t') {
        i++;
    }
    if (*i != '(') {
        return i;
    }
    i++;

    FmtInfo inner = {0};
    i             = _read_i32(i, &inner, &p->x);
    while (*i == ' ' || *i == '\t' || *i == ',') {
        i++;
    }
    i = _read_i32(i, &inner, &p->y);
    while (*i == ' ' || *i == '\t') {
        i++;
    }
    if (*i == ')') {
        i++;
    }
    return i;
}

bool test_user_type_write_basic(void);
bool test_user_type_write_mixed_args(void);
bool test_user_type_read_basic(void);
bool test_user_type_round_trip(void);

bool test_user_type_write_basic(void) {
    WriteFmt("Testing user-type write through IOFMT_USER_CASES_\n");

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
    WriteFmt("Testing user-type read through IOFMT_USER_CASES_\n");

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

int main(void) {
    WriteFmt("[INFO] Starting Io.UserTypes tests\n\n");

    TestFunction tests[] = {
        test_user_type_write_basic,
        test_user_type_write_mixed_args,
        test_user_type_read_basic,
        test_user_type_round_trip,
    };

    int total_tests = sizeof(tests) / sizeof(tests[0]);
    return run_test_suite(tests, total_tests, NULL, 0, "Io.UserTypes");
}
