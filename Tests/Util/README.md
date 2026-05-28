# Test Utilities

Helpers for writing tests that exercise paths which `LOG_FATAL` (aka deadend tests). The driver runs every test in the same process and uses MisraStdC's `OnAbort` hook plus `setjmp` / `longjmp` to catch an expected abort and report it as a pass.

## When to reach for what

- **Normal tests**: a function that returns `bool` and never aborts. Add it to a `TestFunction` array and call `simple_test_driver(tests, count)` (or `run_test_suite(...)`).
- **Deadend tests**: a function whose only useful outcome is that it aborts (calls `LOG_FATAL` because the validator catches an intentionally-corrupted invariant). Add it to a separate array and call `deadend_test_driver(tests, count)`; the driver expects every entry to abort.
- **Mixed suite**: pass both arrays to `run_test_suite(normal, n_normal, deadend, n_deadend, "MySuite")`.

## How `test_deadend` works

`test_deadend` runs a single test function and reports whether its outcome matched the expectation:

```c
bool test_deadend(TestFunction test_func, bool expect_failure);
```

- `expect_failure = true`: pass = test aborted via `LOG_FATAL`, fail = test returned without aborting.
- `expect_failure = false`: pass = test returned `true` without aborting, fail = anything else.

Internally:

1. Install a custom abort handler via `OnAbort(test_abort_handler)` (declared in `<Misra/Sys.h>`). The handler sets a captured flag and `longjmp`s back to the driver instead of exiting the process.
2. `setjmp(g_test_abort_jmp)` to save the resume point.
3. Run the test function. If it returns normally, control falls through. If it triggers `LOG_FATAL`, the abort handler longjmps back to the `setjmp` site with non-zero.
4. Reset `OnAbort(NULL)` so subsequent code sees the default abort.

Because everything runs in one process, a SIGSEGV / SIGBUS in a deadend test still terminates the test binary -- only `LOG_FATAL`-style aborts are recoverable. If a deadend test needs to provoke a real signal (not just a project-level fatal), use a separate test binary instead of `test_deadend`.

## Example

```c
#include "../Util/TestRunner.h"

static bool good_test(void) {
    return 1 + 1 == 2;
}

static bool bad_test(void) {
    Vec(int) v = VecInit();
    int      x = VecAt(&v, 100); // out-of-bounds, LOG_FATAL
    (void)x;
    return false; // not reached
}

int main(void) {
    TestFunction normal[]  = { good_test };
    TestFunction deadend[] = { bad_test };
    return run_test_suite(normal, 1, deadend, 1, "Example");
}
```

## Build integration

Tests in `Tests/Std/*.c` already wire the dependency via `meson.build`. For a fresh test:

```meson
test_exe = executable(
    'MyTest',
    files('MyTest.c'),
    link_with: [misra_std],          # or misra_std_no_backtrace for fatal-heavy
    dependencies: [test_util_dep],   # or *_no_sanitizers / *_no_backtrace
    c_args: test_args,
    include_directories: inc_misra,
    install: false,
)
test('MyTest', test_exe)
```

Three dependency variants exist; pick the one that matches the library link of the test:

| dependency                          | pairs with                       | when to use                                                     |
| ---                                 | ---                              | ---                                                             |
| `test_util_dep`                     | `misra_std`                      | sanitised normal tests                                          |
| `test_util_dep_no_sanitizers`       | `misra_std_no_sanitizers`        | deadend tests where ASan/UBSan would interfere with the abort   |
| `test_util_dep_no_backtrace`        | `misra_std_no_backtrace`         | sanitised tests that fire many `LOG_FATAL`s (skip backtrace fmt) |

## Limitations

- Only `LOG_FATAL` aborts are recoverable -- real signals (`SIGSEGV`, `SIGBUS`) crash the process.
- A test that aborts inside cleanup code may skip allocator teardown; the longjmp does not unwind C++-style destructors (none here, but worth noting).
- The driver is single-threaded by design. Tests that need concurrency should not call `test_deadend` from a worker thread; the `OnAbort` slot is process-global.
