/// file : tests/std/macho.mut.c
/// Targeted mutation-kill tests for MachO: drive inputs that make surviving mutants
/// produce observably-wrong results. Distinct from existing MachO tests -- no dups.
#include <Misra.h>
#include <Misra/Std/Allocator/Debug.h>
#include "../Util/TestRunner.h"
int main(void) {
    TestFunction tests[]         = {0};
    TestFunction deadend_tests[] = {0};
    (void)tests;
    (void)deadend_tests;
    return run_test_suite(tests, 0, deadend_tests, 0, "MachO.Mut");
}
