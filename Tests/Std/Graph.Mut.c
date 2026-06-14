/// file : tests/std/graph.mut.c
/// Targeted mutation-kill tests for Graph: each test drives an input that makes a
/// specific surviving mutant produce an observably-wrong result. Distinct from the
/// existing Graph.* tests -- do NOT duplicate.
///
/// The only Graph survivors that are observably killable (rather than equivalent /
/// redundant / alloc-failure-only -- see the audit ledger) live in
/// graph_validate_alignment, reached from validate_graph (line 340). A graph bound
/// to an allocator with a non-power-of-two alignment floor must abort the next time
/// the validator runs its deep body. We trip that via GraphCommitChanges on an empty
/// graph: it calls ValidateGraph first (full body, since the magic VALIDATED bit is
/// set right after init) before touching any slot, so the abort is isolated to the
/// alignment check -- no node payload allocation (line 112) is reached.
///
/// Killed by the single deadend below:
///   - 31:20 cxx_gt_to_le  : '(alignment > 1)' -> '(alignment <= 1)'  (skips fatal for align 3)
///   - 31:29 cxx_replace_scalar_call : graph_alignment_is_pow2(3) -> 42 (truthy -> !42 false -> skips fatal)
///   - 340:5 cxx_remove_void_call    : drops graph_validate_alignment(graph) inside validate_graph
/// Each mutant makes graph_validate_alignment (and hence validate_graph) NOT abort
/// on the bad-alignment graph -> GraphCommitChanges returns normally -> the deadend
/// (which expects an abort) fails -> mutant killed.

#include <Misra.h>
#include <Misra/Std/Allocator/Page.h>
#include <Misra/Std/Container/Graph.h>

#include "../Util/TestRunner.h"

typedef Graph(int) IntGraph;

// =============================================================================
// graph_validate_alignment via validate_graph (line 340), reached from
// GraphCommitChanges on an EMPTY graph. A PageAllocator with alignment floor 3
// (non-power-of-two) must trip the "alignment must be 1 or a power of two"
// LOG_FATAL the first time the validator's deep body runs.
//
// Empty graph => no slots, no payload allocation, so the only reachable fatal is
// the alignment check. With any of the three targeted mutants applied the check
// is skipped and GraphCommitChanges returns 0 without aborting.

static bool deadend_commit_aborts_on_non_pow2_alignment(void) {
    PageAllocator alloc = PageAllocatorInitAligned(3);

    IntGraph graph = GraphInit(&alloc);

    // Empty graph: ValidateGraph (full body) runs before any slot work and must
    // abort at graph_validate_alignment. Reaching the return is the bug.
    (void)GraphCommitChanges(&graph);

    GraphDeinit(&graph);
    PageAllocatorDeinit(&alloc);
    return true;
}

int main(void) {
    WriteFmt("[INFO] Starting Graph.Mut tests\n\n");

    TestFunction tests[]         = {0};
    TestFunction deadend_tests[] = {
        deadend_commit_aborts_on_non_pow2_alignment,
    };
    (void)tests;

    return run_test_suite(
        tests,
        0,
        deadend_tests,
        (int)(sizeof(deadend_tests) / sizeof(deadend_tests[0])),
        "Graph.Mut"
    );
}
