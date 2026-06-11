#!/usr/bin/env bash

# Mutation testing for MisraStdC using Mull.
#
# For each component, compiles the project with clang + the mull-ir-frontend
# pass plugin scoped (via a generated MULL_CONFIG) to that component's source,
# then runs the component's test suites under mull-runner. The score measures
# how well a component's suites kill mutations in that component's
# implementation. Scoping per component keeps each run tractable (only that
# file is mutated) and gives a meaningful per-component number.
#
# Portable across environments:
#   - Local: `nix develop .#mutation` exports CC / MULL_IR_FRONTEND / MULL_RUNNER.
#   - CI (Ubuntu): clang + mull come from apt; the plugin is found under /usr/lib.
#
# Usage:
#   ./Scripts/mutation.sh [Component ...]
# With no args it runs the default component set below. A component is a test
# prefix (Map, Vec, Str, ...) whose source is Source/.../Container/<C>.c or
# Source/Misra/Std/<C>.c, and whose suites are the meson tests named "<C>.*".
#
# Environment (all optional):
#   MULL_LLVM_VERSION  LLVM major version (default: 19)
#   CC                 clang to compile with (default: clang-<v>)
#   MULL_IR_FRONTEND   path to the mull-ir-frontend plugin
#   MULL_RUNNER        mull runner binary (default: mull-runner-<v>)
#   MIN_SCORE          fail if any suite scores below this percent (default: 0)
#   MUTATION_SANITIZE  meson b_sanitize value (default: none)

set -euo pipefail

cd "$(dirname "$0")/.."

LLVMV=${MULL_LLVM_VERSION:-19}
ROOT=build_mull
CC=${CC:-clang-$LLVMV}
MULL_RUNNER=${MULL_RUNNER:-mull-runner-$LLVMV}
MIN_SCORE=${MIN_SCORE:-0}
SAN=${MUTATION_SANITIZE:-none}

COMPONENTS=("$@")
if [ ${#COMPONENTS[@]} -eq 0 ]; then
  COMPONENTS=(
    # containers
    Map Vec Str List BitVec Graph Int Float
    # std subsystems
    Io File ArgParse Iter Allocator AllocDebug
    # parsers
    Elf MachO Pe Pdb Dwarf Json KvConfig Http Dns
    # sys
    Backtrace Socket ProcMaps SymbolResolver MachoCache PdbCache SysDns
  )
fi

# Locate the pass plugin if the environment did not hand it to us.
if [ -z "${MULL_IR_FRONTEND:-}" ]; then
  for cand in "/usr/lib/mull-ir-frontend-$LLVMV" "/usr/lib/llvm-$LLVMV/lib/mull-ir-frontend-$LLVMV"; do
    [ -f "$cand" ] && MULL_IR_FRONTEND="$cand" && break
  done
fi
if [ -z "${MULL_IR_FRONTEND:-}" ] || [ ! -f "$MULL_IR_FRONTEND" ]; then
  echo "error: mull-ir-frontend-$LLVMV not found. Set MULL_IR_FRONTEND, run inside" >&2
  echo "       'nix develop .#mutation', or apt-install mull-$LLVMV." >&2
  exit 1
fi

echo "==> compiler    : $CC"
echo "==> mull plugin : $MULL_IR_FRONTEND"
echo "==> runner      : $MULL_RUNNER"
echo "==> components  : ${COMPONENTS[*]}"
echo

# Explicit component -> source overrides, for components whose test prefix does
# not match a unique <Component>.c basename under Source/ (case, ambiguity, or
# a differently-named file).
declare -A SRC_OVERRIDE=(
  [Json]="Source/Misra/Parsers/JSON.c"
  [Dns]="Source/Misra/Parsers/Dns.c"
  [SysDns]="Source/Misra/Sys/Dns.c"
  [AllocDebug]="Source/Misra/Std/Allocator/Debug.c"
  [Iter]="Source/Misra/Std/Utility/Iter.c"
)

# Resolve a component name to its implementation source file: an explicit
# override, else a unique <Component>.c anywhere under Source/. Returns nonzero
# (caller skips) when there is no match or an ambiguous one.
component_source() {
  local c=$1
  if [ -n "${SRC_OVERRIDE[$c]:-}" ]; then
    [ -f "${SRC_OVERRIDE[$c]}" ] && { echo "${SRC_OVERRIDE[$c]}"; return 0; }
    return 1
  fi
  local hits n
  hits=$(find Source -type f -name "$c.c" 2>/dev/null)
  n=$(printf '%s\n' "$hits" | grep -c .)
  if [ "$n" = "1" ]; then
    echo "$hits"
    return 0
  fi
  return 1
}

# One build dir per invocation, named by the first component so parallel runs
# over disjoint component sets don't collide. The library is built ONCE; each
# component is then scoped incrementally -- we delete just that source's library
# object so ninja recompiles that single file (seconds) under the component's
# MULL_CONFIG, instead of rebuilding the whole library per component (minutes).
BUILD_DIR="$ROOT/${COMPONENTS[0]}"
mkdir -p "$BUILD_DIR"

# Source/Misra/Parsers/Elf.c -> Source_Misra_Parsers_Elf.c.o (meson object name)
obj_name() { printf '%s.o' "$(printf '%s' "$1" | tr '/' '_')"; }

overall_fail=0
prev_src=""
for comp in "${COMPONENTS[@]}"; do
  src=$(component_source "$comp") || { echo "::warning::no source for component '$comp', skipping"; continue; }

  cfg="$BUILD_DIR/mull-$comp.yml"
  printf 'includePaths:\n  - %s\nmutators:\n  - cxx_all\n' "$src" > "$cfg"
  export MULL_CONFIG="$PWD/$cfg"

  # Force recompile of the current target (to embed its mutants) and the
  # previously-mutated one (to drop its mutants, now out of includePaths).
  # Deleting the library object -- not touching the shared source -- avoids
  # churning a parallel run's build of the same file.
  for s in "$src" "$prev_src"; do
    [ -n "$s" ] && find "$BUILD_DIR" -path "*.a.p/$(obj_name "$s")" -delete 2>/dev/null
    true
  done
  prev_src="$src"

  # Configure once; MULL_CONFIG is already set so the initial full build embeds
  # the first component's mutants.
  if [ ! -f "$BUILD_DIR/build.ninja" ]; then
    CC="$CC" meson setup "$BUILD_DIR" \
      -Db_sanitize="$SAN" \
      -Db_lundef=false \
      -Dc_args="-g -O0 -grecord-command-line -fno-discard-value-names -fpass-plugin=$MULL_IR_FRONTEND"
  fi

  # Suites for this component: meson tests named exactly "<comp>" or "<comp>.*"
  # (single-token like Elf, dotted like Io.Write, multi-dot like
  # Json.Read.Simple). meson prints "<project>:<testname>"; strip the prefix.
  mapfile -t suites < <(meson test -C "$BUILD_DIR" --list 2>/dev/null | sed 's/^[^:]*://' | grep -E "^$comp(\.|\$)" | sort -u)
  if [ ${#suites[@]} -eq 0 ]; then
    echo "::warning::no suites found for component '$comp', skipping"
    continue
  fi

  echo "==================== $comp  ($src)  [${#suites[@]} suites] ===================="
  for suite in "${suites[@]}"; do
    echo "-------------------- $suite --------------------"
    bin="$BUILD_DIR/Tests/$suite"
    report="$BUILD_DIR/mutation-$suite.txt"

    # Most suites build as a "Tests/<name>" target; a few (custom test exes)
    # don't -- skip those rather than aborting the whole sweep.
    if ! ninja -C "$BUILD_DIR" "Tests/$suite" 2>/dev/null || [ ! -x "$bin" ]; then
      echo "::warning::$suite: no buildable 'Tests/$suite' binary -- skipping"
      echo "$suite: no binary" > "$report"
      continue
    fi

    if ! "$bin" >/dev/null 2>&1; then
      echo "::warning::$suite baseline FAILS unmutated -- skipping mutation run"
      echo "$suite: baseline failed" > "$report"
      continue
    fi

    # Mutators + includePaths come from MULL_CONFIG at COMPILE time; the runner
    # only discovers the embedded mutants. mull-runner exits nonzero when any
    # mutant survives -- expected, so don't let it trip `set -e`.
    #
    # --timeout bounds the unmutated WARMUP run; the per-mutant timeout is then
    # max(baseline*10, --minimum-timeout). Large suites under the debug
    # allocator (Float/Int/Io) blow past mull's ~3s warmup default, and a
    # timed-out warmup yields an EMPTY report that silently corrupts the
    # true-survivor intersection -- so keep --timeout generous (it only gates
    # the single warmup run). Conversely, --minimum-timeout must stay LOW:
    # bignum mutants can spin near-infinitely, and a high floor makes each one
    # burn the full floor before being timeout-killed, so a big suite (Int.Math
    # ~1500 mutants) never finishes. Let baseline*10 govern; 8s is just a floor
    # for suites whose baseline is tiny. Override via env.
    set +e
    "$MULL_RUNNER" --workers "$(nproc)" \
      --timeout "${MULL_TIMEOUT:-120000}" \
      --minimum-timeout "${MULL_MIN_TIMEOUT:-8000}" \
      "$bin" 2>&1 | tee "$report"
    set -e

    score=$(grep -oiE 'mutation score: *[0-9]+' "$report" | grep -oE '[0-9]+' | tail -1 || true)
    if [ -n "$score" ] && [ "$score" -lt "$MIN_SCORE" ]; then
      echo "::error::$suite mutation score ${score}% < required ${MIN_SCORE}%"
      overall_fail=1
    fi
    echo
  done
done

# mull-runner drops per-mutant artifact files (16-hex-char names) in the CWD;
# clean them so they don't litter the repo root.
find . -maxdepth 1 -type f -regextype posix-extended -regex '\./[0-9a-f]{16}' -delete 2>/dev/null || true

exit $overall_fail
