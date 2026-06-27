#include <Misra.h>
#include <Misra/Std/Allocator/Default.h>
#include <Misra/Std/Allocator/Debug.h>
#include <Misra/Std/Zstr.h>
#include <Misra/Std/ArgParse.h>
#include <Misra/Std/Container/Str.h>
#include <Misra/Std/File.h>
#include <Misra/Std/Log.h>

#include "../Util/TestRunner.h"

// ---------------------------------------------------------------------------
// ArgParse writes errors / --help to its output sink (`p->out`, default
// FileStderr()). To assert error wording (which distinguishes several "both
// branches ERROR but different message" mutants) we point the sink at a temp
// file, run, then read it back. Fully portable -- no fd/handle redirection.
// ---------------------------------------------------------------------------
static ArgRun capture_run(ArgParse *p, int argc, char **argv, Str *out) {
    Str  tmp_path = StrInit(p->alloc);
    File tmp      = FileOpenTemp(&tmp_path, p->alloc);
    StrDeinit(&tmp_path);
    if (!FileIsOpen(&tmp))
        LOG_FATAL("capture_run: FileOpenTemp failed");

    p->out    = &tmp;
    ArgRun rc = ArgParseRun(p, argc, argv);
    p->out    = NULL;

    FileSeek(&tmp, 0, FILE_SEEK_SET);
    FileRead(&tmp, out);
    FileClose(&tmp);
    return rc;
}

static bool str_has(Str *hay, Zstr needle) {
    u64  hn  = StrLen(hay);
    Zstr beg = StrBegin(hay);
    u64  nn  = ZstrLen(needle);
    if (nn == 0)
        return true;
    if (nn > hn)
        return false;
    for (u64 i = 0; i + nn <= hn; ++i) {
        u64 j = 0;
        while (j < nn && beg[i + j] == needle[j])
            ++j;
        if (j == nn)
            return true;
    }
    return false;
}

// ---------------------------------------------------------------------------
// 720:29 (>= -> >), 653:9 (n_positionals init 0 -> 42), 656:13 (++ -> --):
// Registering one positional and supplying TWO positional tokens must error
// with the "unexpected positional argument" message. All three mutants
// instead fall through the overflow guard and reach the pos==NULL branch,
// printing "internal error: positional slot ... missing". Asserting the exact
// wording (and the absence of the internal-error wording) kills all three.
// ---------------------------------------------------------------------------
static bool test_blind_too_many_positionals_message(void) {
    DefaultAllocator a = DefaultAllocatorInit();
    ArgParse         p = ArgParseInit("prog", NULL, &a);

    Zstr x = NULL;
    ArgPositional(&p, "x", &x, "");

    char  *argv[] = {(char *)"prog", (char *)"first", (char *)"extra"};
    Str    out    = StrInit(&a);
    ArgRun rc     = capture_run(&p, 3, argv, &out);

    bool ok =
        (rc == ARG_RUN_ERROR) && str_has(&out, "unexpected positional argument") && !str_has(&out, "internal error");
    StrDeinit(&out);
    ArgParseDeinit(&p);
    DefaultAllocatorDeinit(&a);
    return ok;
}

// ---------------------------------------------------------------------------
// 758:26 (== ARG_ROLE_POSITIONAL -> !=) in the missing-required validation.
// A missing REQUIRED option must report "missing required option", NOT the
// positional wording. The mutant flips the message-selection branch and emits
// "missing required positional argument" for the unseen REQUIRED instead.
// ---------------------------------------------------------------------------
static bool test_blind_missing_required_message(void) {
    DefaultAllocator a = DefaultAllocatorInit();
    ArgParse         p = ArgParseInit("prog", NULL, &a);

    Zstr listen = NULL;
    ArgRequired(&p, "-l", "--listen", &listen, "host:port");

    char  *argv[] = {(char *)"prog"};
    Str    out    = StrInit(&a);
    ArgRun rc     = capture_run(&p, 1, argv, &out);

    bool ok = (rc == ARG_RUN_ERROR) && str_has(&out, "missing required option") &&
              !str_has(&out, "missing required positional");
    StrDeinit(&out);
    ArgParseDeinit(&p);
    DefaultAllocatorDeinit(&a);
    return ok;
}

// ---------------------------------------------------------------------------
// 496:19 (n >= 128 -> n > 128). The split-flag-name buffer is 128 bytes; a
// flag-name part of exactly 128 chars needs 129 bytes (name + NUL) and must be
// rejected with "flag name too long". The mutant lets n==128 through. We use a
// registered long option whose name is exactly 128 chars; on real code the
// "--<...>=v" form is rejected as too-long (ERROR + that message). The mutant
// would instead copy 128 bytes and proceed. Assert the too-long message.
// ---------------------------------------------------------------------------
static bool test_blind_flag_name_too_long_boundary(void) {
    DefaultAllocator a = DefaultAllocatorInit();
    ArgParse         p = ArgParseInit("prog", NULL, &a);

    // Build "--" + 126 'a' = 128-char long name, then register it.
    static char name[129];
    name[0] = '-';
    name[1] = '-';
    for (int i = 2; i < 128; ++i)
        name[i] = 'a';
    name[128] = '\0';

    Zstr v = NULL;
    ArgRequired(&p, NULL, name, &v, "x");

    // token = name + "=val": flag-name slice length is exactly 128.
    static char tok[131];
    for (int i = 0; i < 128; ++i)
        tok[i] = name[i];
    tok[128] = '=';
    tok[129] = 'V';
    tok[130] = '\0';

    char  *argv[] = {(char *)"prog", tok};
    Str    out    = StrInit(&a);
    ArgRun rc     = capture_run(&p, 2, argv, &out);

    bool ok = (rc == ARG_RUN_ERROR) && str_has(&out, "flag name too long");
    StrDeinit(&out);
    ArgParseDeinit(&p);
    DefaultAllocatorDeinit(&a);
    return ok;
}

int main(void) {
    TestFunction tests[] = {
        test_blind_too_many_positionals_message,
        test_blind_missing_required_message,
        test_blind_flag_name_too_long_boundary,
    };
    TestFunction deadend_tests[] = {0};
    (void)deadend_tests;
    return run_test_suite(tests, sizeof(tests) / sizeof(tests[0]), deadend_tests, 0, "ArgParse.Blind");
}
