#include <Misra.h>
#include <Misra/Std/Allocator/Debug.h>
#include "../Util/TestRunner.h"

// Blind-mutant analysis for Source/Misra/Std/Container/Str.c.
//
// All 19 surviving mutants from /tmp/blind_Str.txt were analysed against the
// source and proven to be EQUIVALENT (no reachable input yields an observable
// difference). No detecting test is therefore possible. Reasoning summary:
//
//   69:25  sub_to_add   round_f64() negative branch is dead: its only caller
//                        (line 838) flips negatives positive at line 707, so
//                        round_f64 always takes the x>=0 branch (line 67).
//   492:5  remove_void  ValidateStr(s) in str_replace_zstr is redundant: the
//                        inner str_replace_cstr (line 475) re-validates s
//                        before any deref of s. No s read happens in between.
//   497:5  remove_void  Same: str_replace_str -> str_replace_cstr validates s.
//   595:14 init_const   bool ok = true -> 42; 42 normalises to true. Same.
//   661:5  remove_void  ValidateStr(str) in StrFromF64 masked by StrClear
//                        (line 672 -> clear_vec -> ValidateVec); only config is
//                        read between 661 and 672, never str.
//   776:18 init_const   bool ok = true -> 42 (truthy). Same.
//   807:18 init_const   bool ok = true -> 42 (truthy). Same.
//   847:39 gt_to_ge     frac_part-digit > 0.999999 -> >=; differs only at the
//                        exact f64 literal 0.999999, unreachable from public
//                        f64 inputs.
//   880:16 lt_to_le     ws-skip loop pos<length -> pos<=length; at pos==length
//                        reads the NUL terminator (IS_SPACE('\0')==false), loop
//                        stops identically. Same pos.
//   928:21 assign_const have_digits = true -> 42 (truthy). Same.
//   932:16 lt_to_le     trailing ws-skip; same NUL-terminator argument as 880.
//   971:18 assign_const negative = true -> 42 (truthy bool). Same.
//   1031:21 sub_to_add  length-pos>=3 -> length+pos>=3 (nan/inf guard); mutant
//                        only enters MORE often, on remaining<3 the NUL
//                        terminator at data[pos+2] never spells nan/inf, so it
//                        falls through to the numeric parser: identical *value.
//   1052:18 assign_const negative = true -> 42 (truthy). Same.
//   1057:25 sub_to_add  -inf guard; same argument as 1031.
//   1078:21 assign_const have_digits = true -> 42 (truthy). Same.
//   1089:26 assign_const have_digits = true -> 42 (truthy). Same.
//   1100:30 assign_const exp_negative = true -> 42 (truthy). Same.
//   1117:29 assign_const have_exp_digits = true -> 42 (truthy). Same.
//
// Result: 0 detecting tests added; all 19 mutants are equivalent.

int main(void) {
    TestFunction tests[]         = {0};
    TestFunction deadend_tests[] = {0};
    (void)tests;
    (void)deadend_tests;
    return run_test_suite(tests, 0, deadend_tests, 0, "Str.Blind");
}
